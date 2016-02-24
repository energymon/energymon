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
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "energymon.h"
#include "energymon-wattsup.h"
#include "energymon-time-util.h"
#include "energymon-util.h"

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
#define WU_MAX_INITIAL_READS 100

static const char* const WU_CLEAR = "#R,W,0;";
static const char* const WU_LOG_START_EXTERNAL = "#L,W,3,E,1,1;";

typedef struct energymon_wattsup {
  int fd;
  int poll;
  pthread_t thread;
  int use_estimates;

  int64_t exec_us;
  struct timespec ts;
  unsigned int decawatts;
  int lock;
  uint64_t total_uj;
} energymon_wattsup;

/**
 * Read data from the WattsUp into the buffer.
 * Returns the number of characters read, or -1 on failure (errno is set).
 */
static inline int wattsup_read(int fd, char* buf, size_t buflen) {
  fd_set set;
  struct timeval timeout;
  int ret = -1;

  assert(fd > 0);
  assert(buf != NULL);
  assert(buflen > 0);

  FD_ZERO(&set);
  FD_SET(fd, &set);
  // documentation specifies response within 2 seconds
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;

  switch (select(fd + 1, &set, NULL, NULL, &timeout)) {
    case -1:
      // failed
      break;
    case 0:
      // timed out
      errno = ETIME;
      break;
    default:
      ret = read(fd, buf, buflen);
      if (!ret) {
        // no data was read
        ret = -1;
        errno = ENODATA;
      }
      break;
  }
  return ret;
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

  state->decawatts = 0;
  if (energymon_clock_gettime(&state->ts)) {
    // must be that CLOCK_MONOTONIC is not supported
    perror("wattsup_poll_sensors");
    return (void*) NULL;
  }
  energymon_sleep_us(WU_MIN_INTERVAL_US);
  while (state->poll) {
    ret = wattsup_read(state->fd, buf, WU_BUFSIZE);
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
              // keep old value of decawatts
              perror("wattsup_poll_sensors:strtoul");
            } else {
              state->decawatts = tmp;
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
      // disable thread cancel while we hold the lock
      pthread_setcancelstate(PTHREAD_CANCEL_DEFERRED, NULL);
      while (__sync_lock_test_and_set(&state->lock, 1)) {
        while (state->lock);
      }
    }
    state->exec_us = energymon_gettime_us(&state->ts);
    state->total_uj += state->decawatts * state->exec_us / 10;
    if (state->use_estimates) {
      __sync_lock_release(&state->lock);
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
    energymon_sleep_us(WU_MIN_INTERVAL_US);
  }
  return (void*) NULL;
}

static inline int wattsup_open(const char* filename, int* fd) {
  struct stat s;
  char buf[32] = { 0 };
  const char* shortname;

  assert(filename != NULL);
  assert(fd != NULL);

  // Check if device node exists and is writable
  if (stat(filename, &s) < 0) {
    perror(filename);
    return -1;
  }
  if (!S_ISCHR(s.st_mode)) {
    errno = ENOTTY;
    perror("Not a TTY character device");
    return -1;
  }
  if (access(filename, R_OK | W_OK)) {
    perror(filename);
    return -1;
  }

  // Get shortname by dropping leading "/dev/"
  if (!(shortname = strrchr(filename, '/'))) {
    // shouldn't happen since we've already checked filename
    errno = EINVAL;
    perror(filename);
    return -1;
  }
  shortname++;

  // Check if "/sys/class/tty/<shortname>" exists and is correct type
  snprintf(buf, sizeof(buf), "/sys/class/tty/%s", shortname);
  if (stat(buf, &s) < 0) {
    perror(buf);
    return -1;
  }
  if (!S_ISDIR(s.st_mode)) {
    errno = ENODEV;
    perror("Not a TTY device");
    return -1;
  }

  // Open the device file
  *fd = open(filename, O_RDWR | O_NONBLOCK);
  if (*fd < 0) {
    perror(filename);
    return -1;
  }
  return 0;
}

static inline int wattsup_set_serial_attributes(int fd) {
  struct termios t;
  assert(fd > 0);
  // get attributes
  if (tcgetattr(fd, &t)) {
    return -1;
  }
  // set "raw" mode
  cfmakeraw(&t);
  // set input/output baud rate
  cfsetispeed(&t, B115200);
  cfsetospeed(&t, B115200);
  // flush any data received but not read
  tcflush(fd, TCIFLUSH);
  // ignore framing and parity errors (there is no parity bit)
  t.c_iflag |= IGNPAR;
  // Turn off double stop bits (documentation specifies only one is used)
  t.c_cflag &= ~CSTOPB;

  // set the parameters (immediately)
  return tcsetattr(fd, TCSANOW, &t);
}

int energymon_init_wattsup(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  int fd;
  int i;
  int err_save = 0;
  char buf[WU_BUFSIZE] = { '\0' };
  const char* wu_filename = getenv(ENERGYMON_WATTSUP_DEV_FILE);
  if (wu_filename == NULL) {
    wu_filename = ENERGYMON_WATTSUP_DEV_FILE_DEFAULT;
  }

  // open the file descriptor and check device properties
  if (wattsup_open(wu_filename, &fd)) {
    return -1;
  }

  // set serial port configuration
  if (wattsup_set_serial_attributes(fd)) {
    perror(wu_filename);
    close(fd);
    return -1;
  }

  // clear device memory
  if (write(fd, WU_CLEAR, strlen(WU_CLEAR)) < 0) {
    perror(wu_filename);
    close(fd);
    return -1;
  }

  // TODO: This seems necessary to prevent early bad reads in polling thread...
  energymon_sleep_us(WU_MIN_INTERVAL_US);

  // start device logging
  if (write(fd, WU_LOG_START_EXTERNAL, strlen(WU_LOG_START_EXTERNAL)) < 0) {
    perror(wu_filename);
    close(fd);
    return -1;
  }

  // dummy reads - sometimes we get a bunch of junk to start with
  // do at most WU_MAX_INITIAL_READS reads, just so we don't read indefinitely
  for (i = 0; i < WU_MAX_INITIAL_READS; i++) {
    if (wattsup_read(fd, buf, sizeof(buf)) != sizeof(buf)) {
      break;
    }
  }
  if (i == WU_MAX_INITIAL_READS) {
    // the device is just giving us garbage
    fprintf(stderr, "energymon_init_wattsup: Too much data from WattsUp\n");
    close(fd);
    errno = ENODATA;
    return -1;
  }
  // read (hopefully) the last of the junk
  wattsup_read(fd, buf, sizeof(buf));

  // create the state struct
  energymon_wattsup* state = calloc(1, sizeof(energymon_wattsup));
  if (state == NULL) {
    close(fd);
    return -1;
  }

  // set state properties
  state->fd = fd;
  state->use_estimates = getenv(ENERGYMON_WATTSUP_ENABLE_ESTIMATES) != NULL;

  // start polling thread
  state->poll = 1;
  err_save = pthread_create(&state->thread, NULL, wattsup_poll_sensors, state);
  if (err_save) {
    close(fd);
    free(state);
    errno = err_save;
    return -1;
  }

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
    pthread_cancel(state->thread);
    err_save = pthread_join(state->thread, NULL);
  }

  // close file
  if (state->fd > 0 && close(state->fd)) {
    err_save = err_save ? err_save : errno;
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
    while (__sync_lock_test_and_set(&state->lock, 1)) {
      while (state->lock);
    }
    state->exec_us = energymon_gettime_us(&state->ts);
    state->total_uj += state->decawatts * state->exec_us / 10;
    __sync_lock_release(&state->lock);
  }
  return state->total_uj;
}

char* energymon_get_source_wattsup(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "WattsUp? Power Meter", n);
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

int energymon_is_exclusive_wattsup() {
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
