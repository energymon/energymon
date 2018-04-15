/**
* Energy reading from a Watts Up? Power Meter.
* Written for communications protocol serial data format 1.8.
* See:
* https://www.wattsupmeters.com/secure/downloads/CommunicationsProtocol090824.pdf
*
* @author Connor Imes
* @date 2016-02-08
*/
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "energymon.h"
#include "energymon-time-util.h"
#include "energymon-wattsup.h"
#include "wattsup-driver.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_wattsup(em);
}
#endif

// Environment variable to enable updating energy estimates b/w device reads.
// This can provide faster energy data, but risks the total energy being more
// inaccurate in the long run.
// Keeping this as an undocumented feature for now.
#define ENERGYMON_WATTSUP_ENABLE_ESTIMATES "ENERGYMON_WATTSUP_ENABLE_ESTIMATES"

// must wait at least one second between WattsUp polls
#define WU_MIN_INTERVAL_US 1000000
#define WU_POWER_INDEX 3
#define WU_BUFSIZE 256
// chosen empirically as a ridiculous value - never needed more than ~4 reads
#define WU_MAX_RETRIES 5

typedef struct energymon_wattsup {
  energymon_wattsup_ctx* ctx;

  int poll;
  pthread_t thread;
  int use_estimates;

  uint64_t exec_us;
  uint64_t last_us;
  unsigned int deciwatts;
  int lock;
  uint64_t total_uj;
} energymon_wattsup;

static void lock_acquire(int* lock) {
  assert(lock != NULL);
#if defined(_WIN32)
  while (InterlockedExchange((long*) lock, 1)) {
#else
  while (__sync_lock_test_and_set(lock, 1)) {
#endif
    while (*lock);
  }
}

static void lock_release(int* lock) {
#if defined(_WIN32)
  InterlockedExchange((long*) lock, 0);
#else
  __sync_lock_release(lock);
#endif
}

/**
 * pthread function to poll the device at regular intervals.
 */
static void* wattsup_poll_sensors(void* args) {
  energymon_wattsup* state = (energymon_wattsup*) args;
  unsigned int i;
  unsigned int tmp;
  char buf[WU_BUFSIZE] = { 0 };
  char* saveptr;
  char* tok;
  int ret;
#ifndef __ANDROID__
  int dummy_old_state; // not used but kept for portability (see pthread docs)
#endif

  state->deciwatts = 0;
  if (!(state->last_us = energymon_gettime_us())) {
    // must be that CLOCK_MONOTONIC is not supported
    perror("wattsup_poll_sensors");
    return (void*) NULL;
  }
  energymon_sleep_us(WU_MIN_INTERVAL_US, &state->poll);
  while (state->poll) {
#ifndef __ANDROID__
    // deadlock can occur during disconnect in some wattsup_driver impls if thread is canceled during I/O
    // disable thread cancel while we do I/O and later while we hold the lock
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &dummy_old_state);
#endif
    ret = wattsup_read(state->ctx, buf, WU_BUFSIZE);
    if (ret < 0) {
      perror("wattsup_poll_sensors:wattsup_read");
    } else {
      if (ret < WU_BUFSIZE) {
        buf[ret] = '\0';
      } else {
        // TODO: There is probably more data that needs to be read
        buf[WU_BUFSIZE - 1] = '\0';
      }
#ifdef ENERGYMON_WATTSUP_DEBUG
      fprintf(stdout, "Read %d characters:\n%s\n", ret, buf);
#endif
      // sometimes we get multiple (or at least parts of old) packets
      // skip over old data to beginning of most recent packet
      tok = strrchr(buf, '#');
      if (tok) {
        // TODO: It's possible that this is not a complete packet
        tok = strtok_r(tok, ",", &saveptr);
        for (i = 0; tok; i++) {
          // 1st index = command, starting with a '#'
          // 2nd index = subcommand (should be a '-', but we don't care)
          if (i == WU_POWER_INDEX) {
            // treat this is as the average power since last successful read
            errno = 0;
            tmp = strtoul(tok, NULL, 0);
            if (errno) {
              // keep old value of deciwatts
              perror("wattsup_poll_sensors:strtoul");
            } else {
              state->deciwatts = tmp;
            }
            break;
          }
          tok = strtok_r(NULL, ",", &saveptr);
        }
        if (i != WU_POWER_INDEX) {
          fprintf(stderr, "Failed to get power from WattsUp data: %s\n", buf);
        }
      } else {
        fprintf(stderr, "Bad data from WattsUp: %s\n", buf);
      }
    }

    // WattsUps are cranky - don't read for a whole second
    if (state->use_estimates) {
      lock_acquire(&state->lock);
    }
    state->exec_us = energymon_gettime_elapsed_us(&state->last_us);
    state->total_uj += state->deciwatts * state->exec_us / 10;
    if (state->use_estimates) {
      lock_release(&state->lock);
    }
#ifndef __ANDROID__
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &dummy_old_state);
#endif
    energymon_sleep_us(WU_MIN_INTERVAL_US, &state->poll);
  }
  return (void*) NULL;
}

static int wattsup_flush_read(energymon_wattsup_ctx* ctx) {
  assert(ctx != NULL);
  char buf[WU_BUFSIZE] = { 0 };
  const int IGNORE_INTERRUPT = 0;
  int i;
  int ret;
  // try to get one good data packet from the device
  for (i = 0; i <= WU_MAX_RETRIES; i++) {
    if ((ret = wattsup_read(ctx, buf, WU_BUFSIZE)) < 0) {
      return -1;
    }
    buf[ret < WU_BUFSIZE ? ret : WU_BUFSIZE - 1] = '\0';
#ifdef ENERGYMON_WATTSUP_DEBUG
    fprintf(stdout, "Looking for good data packet: %d: %s\n", i, buf);
#endif
    if (ret == WU_BUFSIZE) {
      // cut through the data backlog (shouldn't be a problem if buffers were properly flushed)
      continue;
    }
    // good packets start with a '#' and end with a ';'
    if (strrchr(buf, '#') && strrchr(buf, ';')) {
      break;
    }
    if (i == WU_MAX_RETRIES) {
      return -1; // avoid unnecessary sleep
    }
    energymon_sleep_us(WU_MIN_INTERVAL_US, &IGNORE_INTERRUPT);
  }
  return 0;
}

int energymon_init_wattsup(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  int err_save;
  const char* dev_file = getenv(ENERGYMON_WATTSUP_DEV_FILE);
  if (dev_file == NULL) {
    dev_file = ENERGYMON_WATTSUP_DEV_FILE_DEFAULT;
  }

  // create the state struct
  energymon_wattsup* state = calloc(1, sizeof(energymon_wattsup));
  if (state == NULL) {
    return -1;
  }

  // connect to the device
  if ((state->ctx = wattsup_connect(dev_file, ENERGYMON_WATTSUP_TIMEOUT_MS)) == NULL) {
    free(state);
    return -1;
  }

  // clear device memory
  if (wattsup_write(state->ctx, WU_CLEAR, strlen(WU_CLEAR)) < 0) {
    wattsup_disconnect(state->ctx);
    free(state);
    return -1;
  }

  // start device logging
  if (wattsup_write(state->ctx, WU_LOG_START_EXTERNAL, strlen(WU_LOG_START_EXTERNAL)) < 0) {
    wattsup_disconnect(state->ctx);
    free(state);
    return -1;
  }

  // dummy reads - sometimes we get a bunch of junk to start with
  if (wattsup_flush_read(state->ctx)) {
    fprintf(stderr, "energymon_init_wattsup: Too much or no data from WattsUp\n");
    wattsup_disconnect(state->ctx);
    free(state);
    errno = ENODATA;
    return -1;
  }

  // set state properties
  state->use_estimates = getenv(ENERGYMON_WATTSUP_ENABLE_ESTIMATES) != NULL;

  // start polling thread
  state->poll = 1;
  err_save = pthread_create(&state->thread, NULL, wattsup_poll_sensors, state);
  if (err_save) {
    wattsup_disconnect(state->ctx);
    free(state);
    errno = err_save;
    return -1;
  }

  // give the polling thread a head start
  energymon_sleep_us(WU_MIN_INTERVAL_US / 100, &state->poll);

  em->state = state;
  return 0;
}

int energymon_finish_wattsup(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }

  int err_save = 0;
  energymon_wattsup* state = (energymon_wattsup*) em->state;

  // stop sensors polling thread and cleanup
  if (state->poll) {
    state->poll = 0;
#ifndef __ANDROID__
    pthread_cancel(state->thread);
#endif
    err_save = pthread_join(state->thread, NULL);
  }

  if (state->ctx != NULL) {
    // stop logging
    wattsup_write(state->ctx, WU_LOG_STOP, strlen(WU_LOG_STOP));
    // disconnect from device
    if (wattsup_disconnect(state->ctx)) {
      err_save = err_save ? err_save : errno;
    }
  }

  em->state = NULL;
  free(state);
  errno = err_save;

  return errno ? -1 : 0;
}

uint64_t energymon_read_total_wattsup(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  energymon_wattsup* state = (energymon_wattsup*) em->state;
  if (state->use_estimates) {
    lock_acquire(&state->lock);
    state->exec_us = energymon_gettime_elapsed_us(&state->last_us);
    state->total_uj += state->deciwatts * state->exec_us / 10;
    lock_release(&state->lock);
  }
  return state->total_uj;
}

char* energymon_get_source_wattsup(char* buffer, size_t n) {
  return wattsup_get_implementation(buffer, n);
}

uint64_t energymon_get_interval_wattsup(const energymon* em) {
  if (em == NULL) {
    // we don't need to access em, but it's still a programming error
    errno = EINVAL;
    return 0;
  }
  return WU_MIN_INTERVAL_US;
}

uint64_t energymon_get_precision_wattsup(const energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return 0;
  }
  // deciwatts at 1 second interval
  return WU_MIN_INTERVAL_US / 10;
}

int energymon_is_exclusive_wattsup(void) {
  return 1;
}

int energymon_get_wattsup(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_wattsup;
  em->fread = &energymon_read_total_wattsup;
  em->ffinish = &energymon_finish_wattsup;
  em->fsource = &energymon_get_source_wattsup;
  em->finterval = &energymon_get_interval_wattsup;
  em->fprecision = &energymon_get_precision_wattsup;
  em->fexclusive = &energymon_is_exclusive_wattsup;
  em->state = NULL;
  return 0;
}
