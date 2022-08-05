/**
 * Energy reading for NVIDIA Jetson devices with TI INA3221 power sensors.
 *
 * @author Connor Imes
 * @date 2021-12-16
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "energymon.h"
#include "energymon-jetson.h"
#include "energymon-time-util.h"
#include "energymon-util.h"
#include "ina3221.h"
#include "ina3221x.h"
#include "util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_jetson(em);
}
#endif

#define NUM_RAILS_DEFAULT_MAX 6

// Environment variable to request a minimum polling interval.
// Keeping this as an undocumented feature for now.
#define ENERGYMON_JETSON_INTERVAL_US "ENERGYMON_JETSON_INTERVAL_US"

// The hardware can refresh at microsecond granularity, with "conversion times" for both the shunt- and bus-voltage
// measurements ranging from 140 us to 8.244 ms, per the INA3221 data sheet.
// However, the sysfs interface reports polling delay at millisecond granularity, so use min = 1 ms.
#define INA3221_MIN_POLLING_DELAY_US 1000

// 100k seems to be an empirically good value with very low overhead
#define INA3221_DEFAULT_POLLING_DELAY_US 100000
// 10k might also be considered good (generally <1% CPU utilization on an AGX Xavier)

typedef struct energymon_jetson {
  // sensor update interval in microseconds
  unsigned long polling_delay_us;
  // thread variables
  pthread_t thread;
  int poll_sensors;
  // total energy estimate
  uint64_t total_uj;
  // sensor file descriptors
  // INA3221X provides power (mw) files; INA3221 provides voltage (mv) and current (ma) files
  size_t count;
  int* fds_mw;
  int* fds_mv;
  int* fds_ma;
} energymon_jetson;

static void close_and_clear_fds(int* fds, size_t max_fds) {
  int err_save = errno;
  size_t i;
  for (i = 0; i < max_fds; i++) {
    if (fds[i] > 0) {
      close(fds[i]);
      fds[i] = 0;
    }
  }
  errno = err_save;
}

/*
 * There doesn't seem to be a consistent approach to determine the Jetson model by direct system inquiry.
 * Instead, we use a try/catch heuristic to find default rails with an ordered search.
 * This only works if the name set in an earlier array isn't a subset of any name set in later arrays searched.
 * It's important to order the search s.t. this invariant isn't violated, o/w sensors will be skipped.
 */
// The TX1 and most TX2 models have power power rails VDD_IN for the main board and VDD_MUX for the carrier board
static const char* DEFAULT_RAIL_NAMES_TX[NUM_RAILS_DEFAULT_MAX] = {"VDD_IN", "VDD_MUX"};
// The Xavier NX and TX2 NX have parent power rail VDD_IN
static const char* DEFAULT_RAIL_NAMES_VDD_IN[NUM_RAILS_DEFAULT_MAX] = {"VDD_IN"};
// The Nano has parent power rail POM_5V_IN
static const char* DEFAULT_RAIL_NAMES_POM_5V_IN[NUM_RAILS_DEFAULT_MAX] = {"POM_5V_IN"};
// The AGX Xavier doesn't have a parent power rail - all its rails are in parallel
static const char* DEFAULT_RAIL_NAMES_AGX_XAVIER[NUM_RAILS_DEFAULT_MAX] = {"GPU", "CPU", "SOC", "CV", "VDDRQ", "SYS5V"};
// The AGX Orin HV input supplies CPU, GPU, SOC, and CV; MV input is VIN_SYS_5V0, which then supplies VDDQ_VDD2_1V8AO
static const char* DEFAULT_RAIL_NAMES_AGX_ORIN[NUM_RAILS_DEFAULT_MAX] = {"VDD_GPU_SOC", "VDD_CPU_CV", "VIN_SYS_5V0"};
static const char* const* DEFAULT_RAIL_NAMES[] = {
  DEFAULT_RAIL_NAMES_TX,
  DEFAULT_RAIL_NAMES_VDD_IN,
  DEFAULT_RAIL_NAMES_POM_5V_IN,
  DEFAULT_RAIL_NAMES_AGX_XAVIER,
  DEFAULT_RAIL_NAMES_AGX_ORIN,
};

static int ina3221_walk_i2c_drivers_dir_for_default(int* fds_mv, int* fds_ma, size_t* n_fds,
                                                    unsigned long* update_interval_us_max) {
  size_t i;
  size_t j;
#ifndef NDEBUG
  size_t max_fds = *n_fds;
#endif
  assert(max_fds == NUM_RAILS_DEFAULT_MAX);
  for (i = 0; i < sizeof(DEFAULT_RAIL_NAMES) / sizeof(DEFAULT_RAIL_NAMES[0]); i++) {
    for (*n_fds = 0; *n_fds < NUM_RAILS_DEFAULT_MAX && DEFAULT_RAIL_NAMES[i][*n_fds] != NULL; (*n_fds)++) {
      // *n_fds is being incremented
    }
    assert(*n_fds > 0);
    assert(*n_fds <= max_fds);
    if (ina3221_walk_i2c_drivers_dir(DEFAULT_RAIL_NAMES[i], fds_mv, fds_ma, *n_fds, update_interval_us_max)) {
      // cleanup is done in the calling function
      return -1;
    }
    if (fds_mv[0] > 0) {
      // check if all channels are found
      for (j = 0; j < *n_fds; j++) {
        if (fds_mv[j] <= 0 || fds_ma[j] <= 0) {
          close_and_clear_fds(fds_mv, *n_fds);
          close_and_clear_fds(fds_ma, *n_fds);
          break; // continue outer loop
        }
      }
      return 0;
    }
  }
  errno = ENODEV;
  return -1;
}

static int ina3221x_walk_i2c_drivers_dir_for_default(int* fds, size_t* n_fds, unsigned long* polling_delay_us_max) {
  size_t i;
  size_t j;
#ifndef NDEBUG
  size_t max_fds = *n_fds;
#endif
  assert(max_fds == NUM_RAILS_DEFAULT_MAX);
  for (i = 0; i < sizeof(DEFAULT_RAIL_NAMES) / sizeof(DEFAULT_RAIL_NAMES[0]); i++) {
    for (*n_fds = 0; *n_fds < NUM_RAILS_DEFAULT_MAX && DEFAULT_RAIL_NAMES[i][*n_fds] != NULL; (*n_fds)++) {
      // *n_fds is being incremented
    }
    assert(*n_fds > 0);
    assert(*n_fds <= max_fds);
    if (ina3221x_walk_i2c_drivers_dir(DEFAULT_RAIL_NAMES[i], fds, *n_fds, polling_delay_us_max)) {
      // cleanup is done in the calling function
      return -1;
    }
    if (fds[0] > 0) {
      // check if all channels are found
      for (j = 0; j < *n_fds; j++) {
        if (fds[j] <= 0) {
          close_and_clear_fds(fds, *n_fds);
          break; // continue outer loop
        }
      }
      return 0;
    }
  }
  errno = ENODEV;
  return -1;
}

/**
 * pthread function to poll the sensor(s) at regular intervals.
 */
static void* jetson_poll_sensors(void* args) {
  energymon_jetson* state = (energymon_jetson*) args;
  char cdata[8];
  char cdata2[8];
  unsigned long sum_mw;
  unsigned long mv;
  unsigned long ma;
  size_t i;
  uint64_t exec_us;
  uint64_t last_us;
  int err_save;
  if (!(last_us = energymon_gettime_us())) {
    // must be that CLOCK_MONOTONIC is not supported
    perror("jetson_poll_sensors");
    return (void*) NULL;
  }
  energymon_sleep_us(state->polling_delay_us, &state->poll_sensors);
  while (state->poll_sensors) {
    // read individual sensors
    for (sum_mw = 0, errno = 0, i = 0; i < state->count && !errno; i++) {
      if (state->fds_mw[i] > 0) {
        if (pread(state->fds_mw[i], cdata, sizeof(cdata), 0) > 0) {
          sum_mw += strtoul(cdata, NULL, 0);
        }
      } else {
        if (pread(state->fds_mv[i], cdata, sizeof(cdata), 0) > 0) {
          if (pread(state->fds_ma[i], cdata2, sizeof(cdata2), 0) > 0) {
            mv = strtoul(cdata, NULL, 0);
            ma = strtoul(cdata2, NULL, 0);
            sum_mw += mv * ma / 1000;
          }
        }
      }
    }
    err_save = errno;
    exec_us = energymon_gettime_elapsed_us(&last_us);
    if (err_save) {
      errno = err_save;
      perror("jetson_poll_sensors: skipping power sensor reading");
    } else {
      state->total_uj += (uint64_t) (sum_mw * exec_us / 1000);
    }
    // sleep for the update interval of the sensors
    if (state->poll_sensors) {
      energymon_sleep_us(state->polling_delay_us, &state->poll_sensors);
    }
    errno = 0;
  }
  return (void*) NULL;
}

// Uses strtok_r to parse the input string into an array (str may be free'd afterward)
static char** strtok_to_arr_r(char* str, const char* delim, size_t *n) {
  char* tok;
  char* saveptr;
  char** arr = NULL;
  char** arr_realloc;
  *n = 0;
  tok = strtok_r(str, delim, &saveptr);
  while (tok) {
    // inefficient to realloc every iteration, but straightforward
    if (!(arr_realloc = realloc(arr, (*n + 1) * sizeof(char*)))) {
      goto fail;
    }
    arr = arr_realloc;
    if (!(arr[*n] = strdup(tok))) {
      goto fail;
    }
    (*n)++;
    tok = strtok_r(NULL, delim, &saveptr);
  }
  return arr;
fail:
  while (*n > 0) {
    (*n)--;
    free(arr[*n]);
  }
  free(arr);
  return NULL;
}

static void free_rail_names(char** rail_names, size_t n) {
  while (n > 0) {
    free(rail_names[--n]);
  }
  free(rail_names);
}

static char** get_rail_names(const char* rail_names_str, size_t* n) {
  char* tmp;
  char** rail_names;
  size_t i;
  size_t j;
  // duplicate rail_names_str b/c strtok_r modifies the input string
  if ((tmp = strdup(rail_names_str)) == NULL) {
    return NULL;
  }
  rail_names = strtok_to_arr_r(tmp, ",", n);
  free(tmp);
  if (!rail_names) {
    if (!errno) {
      // must be a bad env var value, o/w errno would be ENOMEM for problems in strtok_to_arr_r
      fprintf(stderr, "energymon_init_jetson: parsing error: "ENERGYMON_JETSON_RAIL_NAMES"=%s\n", rail_names_str);
      errno = EINVAL;
    }
    return NULL;
  }
  // disallow duplicate entries, otherwise we'd get duplicate power readings
  for (i = 0; i < *n - 1; i++) {
    for (j = i + 1; j < *n; j++) {
      if (!strcmp(rail_names[i], rail_names[j])) {
        fprintf(stderr, "energymon_init_jetson: duplicate rail name specified: %s\n",
                rail_names[i]);
        free_rail_names(rail_names, *n);
        *n = 0;
        errno = EINVAL;
        return NULL;
      }
    }
  }
  return rail_names;
}

static unsigned long get_polling_delay_us(unsigned long sysfs_polling_delay_us) {
  unsigned long us;
  const char* polling_interval_env = getenv(ENERGYMON_JETSON_INTERVAL_US);
  if (polling_interval_env) {
    // start with the value specified in the environment variable
    errno = 0;
    us = strtoul(polling_interval_env, NULL, 0);
    if (errno) {
      fprintf(stderr, "energymon_init_jetson: failed to parse environment variable value: "
              ENERGYMON_JETSON_INTERVAL_US"=%s\n", polling_interval_env);
      return 0;
    }
  } else {
    // start with the value captured from sysfs (might be 0)
    us = sysfs_polling_delay_us;
    // enforce a reasonable minimum default since actual update interval may be too fast
    if (us < INA3221_DEFAULT_POLLING_DELAY_US) {
      us = INA3221_DEFAULT_POLLING_DELAY_US;
    }
  }
  // always enforce an lower bound
  if (us < INA3221_MIN_POLLING_DELAY_US) {
    us = INA3221_MIN_POLLING_DELAY_US;
  }
  return us;
}

static int energymon_jetson_init_ina3221(energymon_jetson* state, char** rail_names, size_t n_rails,
                                         unsigned long* polling_delay_us) {
  size_t i;
  if (rail_names) {
    if (ina3221_walk_i2c_drivers_dir((const char* const*) rail_names, state->fds_mv, state->fds_ma, n_rails, polling_delay_us)) {
      close_and_clear_fds(state->fds_mv, state->count);
      close_and_clear_fds(state->fds_ma, state->count);
      return -1;
    }
    for (i = 0; i < n_rails; i++) {
      if (state->fds_mv[i] <= 0) {
        fprintf(stderr, "energymon_init_jetson: did not find requested rail: %s\n", rail_names[i]);
        errno = ENODEV;
        close_and_clear_fds(state->fds_mv, state->count);
        close_and_clear_fds(state->fds_ma, state->count);
        return -1;
      }
    }
  } else {
    if (ina3221_walk_i2c_drivers_dir_for_default(state->fds_mv, state->fds_ma, &n_rails, polling_delay_us)) {
      if (errno == ENODEV) {
        fprintf(stderr, "energymon_init_jetson: did not find default rail(s) - is this a supported model?\n"
                "Try setting "ENERGYMON_JETSON_RAIL_NAMES"\n");
      }
      close_and_clear_fds(state->fds_mv, state->count);
      close_and_clear_fds(state->fds_ma, state->count);
      return -1;
    }
    // possibly (even probably) reduce count
    state->count = n_rails;
  }
  return 0;
}

static int energymon_jetson_init_ina3221x(energymon_jetson* state, char** rail_names, size_t n_rails,
                                          unsigned long* polling_delay_us) {
  size_t i;
  if (rail_names) {
    if (ina3221x_walk_i2c_drivers_dir((const char* const*) rail_names, state->fds_mw, n_rails, polling_delay_us)) {
      close_and_clear_fds(state->fds_mw, state->count);
      return -1;
    }
    for (i = 0; i < n_rails; i++) {
      if (state->fds_mw[i] <= 0) {
        fprintf(stderr, "energymon_init_jetson: did not find requested rail: %s\n", rail_names[i]);
        errno = ENODEV;
        close_and_clear_fds(state->fds_mw, state->count);
        return -1;
      }
    }
  } else {
    if (ina3221x_walk_i2c_drivers_dir_for_default(state->fds_mw, &n_rails, polling_delay_us)) {
      if (errno == ENODEV) {
        fprintf(stderr, "energymon_init_jetson: did not find default rail(s) - is this a supported model?\n"
                "Try setting "ENERGYMON_JETSON_RAIL_NAMES"\n");
      }
      close_and_clear_fds(state->fds_mw, state->count);
      return -1;
    }
    // possibly (even probably) reduce count
    state->count = n_rails;
  }
  return 0;
}

/**
 * Open all sensor files and start the thread to poll the sensors.
 */
int energymon_init_jetson(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  unsigned long polling_delay_us = 0;
  int err_save;
  size_t n_rails;
  char** rail_names = NULL;

  int is_ina3221 = ina3221_exists();
  if (is_ina3221 < 0) {
    return -1;
  }

  const char* rail_names_str = getenv(ENERGYMON_JETSON_RAIL_NAMES);
  if (rail_names_str) {
    if (!(rail_names = get_rail_names(rail_names_str, &n_rails))) {
      return -1;
    }
  } else {
    // look for a default rail below
    n_rails = NUM_RAILS_DEFAULT_MAX;
  }

  energymon_jetson* state = calloc(1, sizeof(energymon_jetson));
  if (state == NULL) {
    goto fail_alloc;
  }
  state->fds_mw = calloc(n_rails, sizeof(int));
  state->fds_mv = calloc(n_rails, sizeof(int));
  state->fds_ma = calloc(n_rails, sizeof(int));
  if (!state->fds_mw || !state->fds_mv || !state->fds_ma) {
    goto fail_state_init;
  }
  state->count = n_rails;

  if (is_ina3221) {
    if (energymon_jetson_init_ina3221(state, rail_names, n_rails, &polling_delay_us) < 0) {
      goto fail_state_init;
    }
  } else {
    if (energymon_jetson_init_ina3221x(state, rail_names, n_rails, &polling_delay_us) < 0) {
      goto fail_state_init;
    }
  }
  if (rail_names) {
    free_rail_names(rail_names, n_rails);
  }
  em->state = state;

  state->polling_delay_us = get_polling_delay_us(polling_delay_us);
  if (!state->polling_delay_us) {
    err_save = errno;
    energymon_finish_jetson(em);
    errno = err_save;
    return -1;
  }

  // start sensors polling thread
  state->poll_sensors = 1;
  errno = pthread_create(&state->thread, NULL, jetson_poll_sensors, state);
  if (errno) {
    err_save = errno;
    energymon_finish_jetson(em);
    errno = err_save;
    return -1;
  }

  return 0;

fail_state_init:
  free(state->fds_mw);
  free(state->fds_mv);
  free(state->fds_ma);
  free(state);
fail_alloc:
  if (rail_names) {
    free_rail_names(rail_names, n_rails);
  }
  return -1;
}

static int close_fds(int* fds, size_t count) {
  size_t i;
  int err_save = 0;
  for (i = 0; i < count; i++) {
    if (fds[i] > 0 && close(fds[i])) {
      err_save = err_save ? err_save : errno;
    }
  }
  errno = err_save;
  return err_save;
}

int energymon_finish_jetson(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }

  int err_save = 0;
  energymon_jetson* state = (energymon_jetson*) em->state;

  if (state->poll_sensors) {
    // stop sensors polling thread and cleanup
    state->poll_sensors = 0;
#ifndef __ANDROID__
    pthread_cancel(state->thread);
#endif
    err_save = pthread_join(state->thread, NULL);
  }

  // close individual sensor files
  if (close_fds(state->fds_mw, state->count)) {
    err_save = err_save ? err_save : errno;
  }
  if (close_fds(state->fds_mv, state->count)) {
    err_save = err_save ? err_save : errno;
  }
  if (close_fds(state->fds_ma, state->count)) {
    err_save = err_save ? err_save : errno;
  }
  free(state->fds_mw);
  free(state->fds_mv);
  free(state->fds_ma);
  free(em->state);
  em->state = NULL;
  errno = err_save;
  return errno ? -1 : 0;
}

uint64_t energymon_read_total_jetson(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  errno = 0;
  return ((energymon_jetson*) em->state)->total_uj;
}

char* energymon_get_source_jetson(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "NVIDIA Jetson INA3221 Power Monitors", n);
}

uint64_t energymon_get_interval_jetson(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  return ((energymon_jetson*) em->state)->polling_delay_us;
}

uint64_t energymon_get_precision_jetson(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  // milliwatts at refresh interval
  uint64_t prec = energymon_get_interval_jetson(em) / 1000;
  return prec ? prec : 1;
}

int energymon_is_exclusive_jetson(void) {
  return 0;
}

int energymon_get_jetson(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_jetson;
  em->fread = &energymon_read_total_jetson;
  em->ffinish = &energymon_finish_jetson;
  em->fsource = &energymon_get_source_jetson;
  em->finterval = &energymon_get_interval_jetson;
  em->fprecision = &energymon_get_precision_jetson;
  em->fexclusive = &energymon_is_exclusive_jetson;
  em->state = NULL;
  return 0;
}
