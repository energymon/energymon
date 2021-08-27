/**
 * Read energy from the ibmpowernv kernel module "energy" or "power" subfeature.
 * See: https://www.kernel.org/doc/html/latest/hwmon/ibmpowernv.html
 *
 * Note: hwmon sysfs exposes power in microWatts, but libsensors uses Watts.
 *
 * @author Connor Imes
 * @date 2021-04-02
 */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// libsensors
#include <sensors.h>
#include <error.h>
#include "energymon.h"
#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
#include <pthread.h>
#include "energymon-ibmpowernv-power.h"
#include "energymon-time-util.h"
#else
#include "energymon-ibmpowernv.h"
#endif
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
  return energymon_get_ibmpowernv_power(em);
#else
  return energymon_get_ibmpowernv(em);
#endif
}
#endif

#define ENERGYMON_IBMPOWERNV_CHIP_NAME_PREFIX "ibmpowernv"

// hwmon sysfs API allows for an `update_interval` file, but the ibmpowernv module doesn't implement it.
// 10x/sec determined experimentally on Summit, but could be even faster or vary by platform.
#ifndef ENERGYMON_IBMPOWERNV_UPDATE_INTERVAL_US
#define ENERGYMON_IBMPOWERNV_UPDATE_INTERVAL_US 100000
#endif

// Environment variable to not call libsensors static lifecycle functions.
// Keeping this as an undocumented feature for now.
#define ENERGYMON_IBMPOWERNV_SENSORS_SKIP_LIFECYCLE "ENERGYMON_IBMPOWERNV_SENSORS_SKIP_LIFECYCLE"

// Environment variable to use a non-NULL sensors init file.
// Keeping this as an undocumented feature for now.
#define ENERGYMON_IBMPOWERNV_SENSORS_INIT_FILE "ENERGYMON_IBMPOWERNV_SENSORS_INIT_FILE"

// Environment variable to specify desired feature label.
// Keeping this as an undocumented feature for now.
#define ENERGYMON_IBMPOWERNV_FEATURE_LABEL "ENERGYMON_IBMPOWERNV_FEATURE_LABEL"

// It's unknown if there's a label that's always available.
// "System" is found on OLCF Summit and is defaulted here b/c it covers the largest scope possible and is unique.
#ifndef ENERGYMON_IBMPOWERNV_FEATURE_LABEL_DEFAULT
#define ENERGYMON_IBMPOWERNV_FEATURE_LABEL_DEFAULT "System"
#endif

typedef struct energymon_ibmpowernv {
  // libsensors context
  const sensors_chip_name* cn;
  int subfeat_nr;
#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
  // thread variables
  pthread_t thread;
  int poll_sensors;
  // total energy estimate
  uint64_t total_uj;
#endif
} energymon_ibmpowernv;

#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
/**
 * pthread function to poll the sensors at regular intervals.
 */
static void* ibmpowernv_poll_sensor(void* args) {
  energymon_ibmpowernv* state = (energymon_ibmpowernv*) args;
  double w;
  uint64_t exec_us;
  uint64_t last_us;
  int rc;
  if (!(last_us = energymon_gettime_us())) {
    // must be that CLOCK_MONOTONIC is not supported
    perror("ibmpowernv_poll_sensor: energymon_gettime_us");
    return (void*) NULL;
  }
  energymon_sleep_us(ENERGYMON_IBMPOWERNV_UPDATE_INTERVAL_US, &state->poll_sensors);
  while (state->poll_sensors) {
    if ((rc = sensors_get_value(state->cn, state->subfeat_nr, &w))) {
      fprintf(stderr, "ibmpowernv_poll_sensor: sensors_get_value: %s\n", sensors_strerror(rc));
    }
    exec_us = energymon_gettime_elapsed_us(&last_us);
    if (!rc) {
      state->total_uj += w * exec_us;
    }
    // sleep for the update interval of the sensors
    if (state->poll_sensors) {
      energymon_sleep_us(ENERGYMON_IBMPOWERNV_UPDATE_INTERVAL_US, &state->poll_sensors);
    }
  }
  return (void*) NULL;
}
#endif

static int init_libsensors(void) {
  const char* input;
  FILE* f = NULL;
  int rc;
  int err_save;
  if (getenv(ENERGYMON_IBMPOWERNV_SENSORS_SKIP_LIFECYCLE)) {
    return 0;
  }
  if ((input = getenv(ENERGYMON_IBMPOWERNV_SENSORS_INIT_FILE))) {
    if (!(f = fopen(input, "r"))) {
      fprintf(stderr, "fopen: %s: %s\n", input, strerror(errno));
      return -1;
    }
  }
  if ((rc = sensors_init(f))) {
    fprintf(stderr, "sensors_init: %s\n", sensors_strerror(rc));
    if (!errno) {
      // we don't really know the error, but we'll encourage user to retry
      errno = EAGAIN;
    }
  }
  if (f) {
    err_save = errno;
    if (fclose(f)) {
      fprintf(stderr, "fclose: %s: %s\n", input, strerror(errno));
    }
    errno = err_save;
  }
  return rc;
}

static void cleanup_libsensors(void) {
  if (!getenv(ENERGYMON_IBMPOWERNV_SENSORS_SKIP_LIFECYCLE)) {
    sensors_cleanup();
  }
}

static int open_sensor(energymon_ibmpowernv* state) {
  const sensors_chip_name* cn;
  const sensors_feature* feat;
  const sensors_subfeature *subfeat;
  const char* feat_label_search;
  char* label;
  int c;
  int f;
  int s;
  if (!(feat_label_search = getenv(ENERGYMON_IBMPOWERNV_FEATURE_LABEL))) {
    feat_label_search = ENERGYMON_IBMPOWERNV_FEATURE_LABEL_DEFAULT;
  }
  for (c = 0; (cn = sensors_get_detected_chips(NULL, &c));) {
    if (strcmp(cn->prefix, ENERGYMON_IBMPOWERNV_CHIP_NAME_PREFIX)) {
      continue;
    }
    for (f = 0; (feat = sensors_get_features(cn, &f));) {
#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
      if (feat->type != SENSORS_FEATURE_POWER) {
#else
      if (feat->type != SENSORS_FEATURE_ENERGY) {
#endif
        continue;
      }
      if (!(label = sensors_get_label(cn, feat))) {
        // probably a strdup error (ENOMEM), but API doesn't document errno
        if (!errno) {
          // we don't really know the error, but we'll encourage user to retry
          errno = EAGAIN;
        }
        perror("sensors_get_label");
        return -1;
      }
      if (strcmp(label, feat_label_search)) {
        free(label);
        continue;
      }
      free(label);
      for (s = 0; (subfeat = sensors_get_all_subfeatures(cn, feat, &s));) {
#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
        if (subfeat->type != SENSORS_SUBFEATURE_POWER_INPUT) {
#else
        if (subfeat->type != SENSORS_SUBFEATURE_ENERGY_INPUT) {
#endif
          continue;
        }
        if (!(subfeat->flags & SENSORS_MODE_R)) {
          fprintf(stderr, "Warning: subfeature found for label, but not readable: %s\n", feat_label_search);
          continue;
        }
        state->cn = cn;
        state->subfeat_nr = subfeat->number;
        return 0;
      }
    }
  }
  errno = ENODEV;
  return -1;
}

static void close_sensor(energymon_ibmpowernv* state) {
  // nothing to actually cleanup - handled internally by libsensors
  state->cn = NULL;
}

#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
int energymon_init_ibmpowernv_power(energymon* em) {
#else
int energymon_init_ibmpowernv(energymon* em) {
#endif
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  energymon_ibmpowernv* state = calloc(1, sizeof(energymon_ibmpowernv));
  if (state == NULL) {
    return -1;
  }

  // open sensor
  if (init_libsensors()) {
    free(state);
    return -1;
  }
  if (open_sensor(state)) {
    cleanup_libsensors();
    free(state);
    return -1;
  }
  em->state = state;

#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
  int err_save;
  // start sensor polling thread
  state->poll_sensors = 1;
  errno = pthread_create(&state->thread, NULL, ibmpowernv_poll_sensor, state);
  if (errno) {
    err_save = errno;
    close_sensor(state);
    cleanup_libsensors();
    free(state);
    em->state = NULL;
    errno = err_save;
    return -1;
  }
#endif

  return 0;
}

#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
uint64_t energymon_read_total_ibmpowernv_power(const energymon* em) {
#else
uint64_t energymon_read_total_ibmpowernv(const energymon* em) {
#endif
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  errno = 0;
  const energymon_ibmpowernv* state = (energymon_ibmpowernv*) em->state;
#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
  return state->total_uj;
#else
  double j = 0;
  int rc;
  if ((rc = sensors_get_value(state->cn, state->subfeat_nr, &j))) {
    fprintf(stderr, "sensors_get_value: %s\n", sensors_strerror(rc));
    if (!errno) {
      // we don't really know the error, but we'll encourage user to retry
      errno = EAGAIN;
    }
    return 0;
  }
  // sysfs value was a u64 in uJ, which libsensors converted to a double in J
  // some precision may have been lost, but there shouldn't be any rollover
  return (uint64_t) (j * 1000000);
#endif
}

#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
int energymon_finish_ibmpowernv_power(energymon* em) {
#else
int energymon_finish_ibmpowernv(energymon* em) {
#endif
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }

  int err_save = 0;
  energymon_ibmpowernv* state = (energymon_ibmpowernv*) em->state;
#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
  if (state->poll_sensors) {
    // stop sensors polling thread and cleanup
    state->poll_sensors = 0;
#ifndef __ANDROID__
    pthread_cancel(state->thread);
#endif
    err_save = pthread_join(state->thread, NULL);
  }
#endif
  close_sensor(state);
  cleanup_libsensors();
  free(em->state);
  em->state = NULL;
  errno = err_save;
  return errno ? -1 : 0;
}

#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
char* energymon_get_source_ibmpowernv_power(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "IBM PowerNV Power Sensors", n);
#else
char* energymon_get_source_ibmpowernv(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "IBM PowerNV Energy Sensors", n);
#endif
}

#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
uint64_t energymon_get_interval_ibmpowernv_power(const energymon* em) {
#else
uint64_t energymon_get_interval_ibmpowernv(const energymon* em) {
#endif
  if (em == NULL) {
    // we don't need to access em, but it's still a programming error
    errno = EINVAL;
    return 0;
  }
  return ENERGYMON_IBMPOWERNV_UPDATE_INTERVAL_US;
}

#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
uint64_t energymon_get_precision_ibmpowernv_power(const energymon* em) {
#else
uint64_t energymon_get_precision_ibmpowernv(const energymon* em) {
#endif
  if (em == NULL) {
    errno = EINVAL;
    return 0;
  }
  // Documentation claims microWatt, but experimentally we see 6 trailing zeros, so Watt-level precision...
  // Divide power precision by the update *rate* to get uJ precision:
  // 1,000,000 uJ/sec precision / (1,000,000 us / update_interval_us) = update_interval_us
  return ENERGYMON_IBMPOWERNV_UPDATE_INTERVAL_US;
}

#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
int energymon_is_exclusive_ibmpowernv_power(void) {
#else
int energymon_is_exclusive_ibmpowernv(void) {
#endif
  return 0;
}

#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
int energymon_get_ibmpowernv_power(energymon* em) {
#else
int energymon_get_ibmpowernv(energymon* em) {
#endif
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
#ifdef ENERGYMON_IBMPOWERNV_USE_POWER
  em->finit = &energymon_init_ibmpowernv_power;
  em->fread = &energymon_read_total_ibmpowernv_power;
  em->ffinish = &energymon_finish_ibmpowernv_power;
  em->fsource = &energymon_get_source_ibmpowernv_power;
  em->finterval = &energymon_get_interval_ibmpowernv_power;
  em->fprecision = &energymon_get_precision_ibmpowernv_power;
  em->fexclusive = &energymon_is_exclusive_ibmpowernv_power;
#else
  em->finit = &energymon_init_ibmpowernv;
  em->fread = &energymon_read_total_ibmpowernv;
  em->ffinish = &energymon_finish_ibmpowernv;
  em->fsource = &energymon_get_source_ibmpowernv;
  em->finterval = &energymon_get_interval_ibmpowernv;
  em->fprecision = &energymon_get_precision_ibmpowernv;
  em->fexclusive = &energymon_is_exclusive_ibmpowernv;
  #endif
  em->state = NULL;
  return 0;
}
