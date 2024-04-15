/**
 * Read energy from an ODROID Smart Power 3 USB device.
 *
 * @author Connor Imes
 * @date 2024-03-30
 */
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osp3.h>
#include "energymon.h"
#include "energymon-osp3.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_osp3(em);
}
#endif

#ifndef ENERGYMON_OSP3_PATH_DEFAULT
#define ENERGYMON_OSP3_PATH_DEFAULT "/dev/ttyUSB0"
#endif

#ifndef ENERGYMON_OSP3_TIMEOUT_MS_DEFAULT
#define ENERGYMON_OSP3_TIMEOUT_MS_DEFAULT (OSP3_INTERVAL_MS_MAX * 2)
#endif

#ifndef ENERGYMON_OSP3_INTERVAL_US_DEFAULT
#define ENERGYMON_OSP3_INTERVAL_US_DEFAULT (OSP3_INTERVAL_MS_DEFAULT * 1000)
#endif

typedef enum energymon_osp3_power_source {
  CHANNEL0, CHANNEL1, INPUT
} energymon_osp3_power_source;

typedef struct energymon_osp3 {
  osp3_device* dev;
  uint64_t total_uj;
  pthread_t thread;
  int poll;
  energymon_osp3_power_source src;
  uint64_t interval_us;
} energymon_osp3;

// Bigger than anything an OSP3 should produce.
#define OSP3_LINE_LEN_MAX 256

static uint64_t log_entry_to_uj(const osp3_log_entry* log_entry, unsigned int ms_last, energymon_osp3_power_source src) {
  uint64_t mw;
  switch (src) {
  case CHANNEL0:
    mw = log_entry->mW_0;
    break;
  case CHANNEL1:
    mw = log_entry->mW_1;
    break;
  case INPUT:
    mw = log_entry->mW_in;
    break;
  default:
    assert(0);
    mw = 0;
    break;
  }
  if (ms_last == 0) {
    // First read.
    ms_last = log_entry->ms;
  }
  return mw * (log_entry->ms - ms_last);
}

static unsigned int log_entry_to_interval_us(const osp3_log_entry* log_entry, unsigned int ms_last) {
  assert(log_entry->ms >= ms_last);
  unsigned int ms = log_entry->ms - ms_last;
  // Round to nearest known supported value.
  // TODO: This is very much a heuristic - is there a better way?
  if (ms <= 8) {
    ms = 5;
  } else if (ms <= 25) {
    ms = 10;
  } else if (ms <= 75) {
    ms = 50;
  } else if (ms <= 250) {
    ms = 100;
  } else if (ms <= 750) {
    ms = 500;
  } else if (ms <= 1500) {
    ms = 1000;
  } else {
    ms = ENERGYMON_OSP3_INTERVAL_US_DEFAULT / 1000;
  }
  return ms * 1000;
}

static void* osp3_poll_device(void* args) {
  energymon_osp3* state = (energymon_osp3*) args;
  osp3_log_entry log_entry;
  uint8_t cs8_2s;
  uint8_t cs8_xor;
  unsigned int ms_last = 0;
  if (osp3_flush(state->dev) < 0) {
    // Continue anyway...
    perror("osp3_poll_device: osp3_flush");
  }
  while (state->poll) {
    // TODO: verify time reported in a line vs the elapsed time we measure?
    char line[OSP3_LINE_LEN_MAX + 1] = { 0 };
    size_t line_written = 0;
    errno = 0;
    if (osp3_read_line(state->dev, (unsigned char*) line, sizeof(line) - 1, &line_written,
                       ENERGYMON_OSP3_TIMEOUT_MS_DEFAULT) < 0) {
      if (errno != ETIME) {
        perror("osp3_poll_device: osp3_read");
        continue;
      }
    }
    if (line_written < OSP3_LOG_PROTOCOL_SIZE) {
      fprintf(stderr, "osp3_poll_device: shorter line than expected: %s", line);
    } else if (line_written > OSP3_LOG_PROTOCOL_SIZE) {
      fprintf(stderr, "osp3_poll_device: longer line than expected: %s", line);
    } else if (osp3_log_parse(line, OSP3_LOG_PROTOCOL_SIZE, &log_entry)) {
      fprintf(stderr, "osp3_poll_device: parsing failed: %s", line);
    } else if (osp3_log_checksum(line, OSP3_LOG_PROTOCOL_SIZE, &cs8_2s, &cs8_xor)) {
      fprintf(stderr, "osp3_poll_device: checksum failed (cs8_2s=%02x, cs8_xor=%02x): %s", cs8_2s, cs8_xor, line);
    } else {
      // This could be wrong if we miss a log entry, but it's as good as we can do...
      state->total_uj += log_entry_to_uj(&log_entry, ms_last, state->src);
      state->interval_us = log_entry_to_interval_us(&log_entry, ms_last);
      ms_last = log_entry.ms;
    }
  }
  return (void*) NULL;
}

int energymon_init_osp3(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  const char* path = getenv(ENERGYMON_OSP3_DEV_FILE_ENV_VAR);
  if (path == NULL) {
    path = ENERGYMON_OSP3_PATH_DEFAULT;
  }
  unsigned int baud = OSP3_BAUD_DEFAULT;
  const char* baud_env = getenv(ENERGYMON_OSP3_BAUD_ENV_VAR);
  if (baud_env != NULL) {
    baud = (unsigned int) atoi(baud_env);
  }

  energymon_osp3_power_source src;
  const char* src_env = getenv(ENERGYMON_OSP3_POWER_SOURCE_ENV_VAR);
  if (src_env == NULL || !strncmp(src_env, ENERGYMON_OSP3_POWER_SOURCE_CH0, sizeof(ENERGYMON_OSP3_POWER_SOURCE_CH0))) {
    src = CHANNEL0;
  } else if (!strncmp(src_env, ENERGYMON_OSP3_POWER_SOURCE_CH1, sizeof(ENERGYMON_OSP3_POWER_SOURCE_CH1))) {
    src = CHANNEL1;
  } else if (!strncmp(src_env, ENERGYMON_OSP3_POWER_SOURCE_IN, sizeof(ENERGYMON_OSP3_POWER_SOURCE_IN))) {
    src = INPUT;
  } else {
    fprintf(stderr, "energymon_init_osp3: Unsupported power source: %s\n", src_env);
    errno = EINVAL;
    return -1;
  }

  energymon_osp3* state = calloc(1, sizeof(energymon_osp3));
  if (state == NULL) {
    return -1;
  }
  if ((state->dev = osp3_open_path(path, baud)) == NULL) {
    perror("energymon_init_osp3: osp3_open_path");
    free(state);
    return -1;
  }
  state->src = src;
  state->interval_us = ENERGYMON_OSP3_INTERVAL_US_DEFAULT;
  em->state = state;

  // start device polling thread
  state->poll = 1;
  errno = pthread_create(&state->thread, NULL, osp3_poll_device, state);
  if (errno) {
    state->poll = 0;
    energymon_finish_osp3(em);
    return -1;
  }

  return 0;
}

uint64_t energymon_read_total_osp3(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  energymon_osp3* state = (energymon_osp3*) em->state;
  errno = 0;
  return state->total_uj;
}

int energymon_finish_osp3(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }
  energymon_osp3* state = (energymon_osp3*) em->state;
  int err_save = 0;

  if (state->poll) {
    state->poll = 0;
#ifndef __ANDROID__
    pthread_cancel(state->thread);
#endif
    err_save = pthread_join(state->thread, NULL);
  }

  if (osp3_close(state->dev) < 0) {
    err_save = err_save ? err_save : errno;
  }
  free(state);
  errno = err_save;
  return errno ? -1 : 0;
}

char* energymon_get_source_osp3(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "ODROID Smart Power 3", n);
}

uint64_t energymon_get_interval_osp3(const energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return 0;
  }
  energymon_osp3* state = (energymon_osp3*) em->state;
  return state == NULL ? ENERGYMON_OSP3_INTERVAL_US_DEFAULT : state->interval_us;
}

uint64_t energymon_get_precision_osp3(const energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return 0;
  }
  // Millwatts at refresh interval.
  return energymon_get_interval_osp3(em) / 1000;
}

int energymon_is_exclusive_osp3(void) {
  return 1;
}

int energymon_get_osp3(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_osp3;
  em->fread = &energymon_read_total_osp3;
  em->ffinish = &energymon_finish_osp3;
  em->fsource = &energymon_get_source_osp3;
  em->finterval = &energymon_get_interval_osp3;
  em->fprecision = &energymon_get_precision_osp3;
  em->fexclusive = &energymon_is_exclusive_osp3;
  em->state = NULL;
  return 0;
}
