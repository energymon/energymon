/**
 * Energy reading for an ODROID with INA231 power sensors.
 *
 * @author Connor Imes
 * @date 2014-06-30
 */

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "energymon.h"
#include "energymon-odroid.h"
#include "energymon-time-util.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_odroid(em);
}
#endif

#define INA231_DIR "/sys/bus/i2c/drivers/INA231"
#define INA231_FILE_TEMPLATE_POWER INA231_DIR"/%s/sensor_W"
#define INA231_FILE_TEMPLATE_ENABLE INA231_DIR"/%s/enable"
#define INA231_FILE_TEMPLATE_UPDATE_PERIOD INA231_DIR"/%s/update_period"
#define INA231_DEFAULT_UPDATE_INTERVAL_US 263808

typedef struct energymon_odroid {
  // sensor update interval in microseconds
  unsigned long read_delay_us;
  // thread variables
  pthread_t thread;
  int poll_sensors;
  // total energy estimate
  uint64_t total_uj;
  // sensor file descriptors
  unsigned int count;
  int fds[];
} energymon_odroid;

/**
 * Returns 0 if sensor is disabled or on I/O failure (check errno).
 */
static inline int is_sensor_enabled(const char* file) {
  int fd;
  char cdata[24];
  int err_save = 0;
  if ((fd = open(file, O_RDONLY)) > 0) {
    if (read(fd, cdata, sizeof(cdata)) < 0) {
      err_save = errno;
    }
    if (close(fd)) {
      perror(file);
    }
    errno = err_save;
  }
  return errno ? 0 : atoi(cdata);
}

static inline long get_update_interval(char** sensors, unsigned int num) {
  unsigned long ret = 0;
  unsigned long tmp;
  unsigned int i;
  char file[64];
  int fd;
  char cdata[24];
  int read_ret;

  for (i = 0; i < num; i++) {
    snprintf(file, sizeof(file), INA231_FILE_TEMPLATE_UPDATE_PERIOD, sensors[i]);
    if ((fd = open(file, O_RDONLY)) <= 0) {
      perror(file);
      continue;
    }
    if ((read_ret = read(fd, cdata, sizeof(cdata))) <= 0) {
      perror(file);
    }
    if (close(fd)) {
      perror(file);
    }
    if (read_ret > 0) {
      errno = 0;
      tmp = strtoul(cdata, NULL, 0);
      if (errno) {
        perror(file);
      } else {
        // keep the largest update_interval
        ret = tmp > ret ? tmp : ret;
      }
    }
  }
  if (!ret) {
    ret = INA231_DEFAULT_UPDATE_INTERVAL_US;
#ifdef ENERGYMON_DEBUG
    fprintf(stderr, "get_update_interval: Using default value: %lu us\n", ret);
#endif
  }
  return ret;
}

int energymon_finish_odroid(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }

  int err_save = 0;
  unsigned int i;
  energymon_odroid* state = (energymon_odroid*) em->state;

  if (state->poll_sensors) {
    // stop sensors polling thread and cleanup
    state->poll_sensors = 0;
#ifndef __ANDROID__
    pthread_cancel(state->thread);
#endif
    err_save = pthread_join(state->thread, NULL);
  }

  // close individual sensor files
  for (i = 0; i < state->count; i++) {
    if (state->fds[i] > 0 && close(state->fds[i])) {
      err_save = err_save ? err_save : errno;
    }
  }
  free(em->state);
  em->state = NULL;
  errno = err_save;
  return errno ? -1 : 0;
}

/**
 * Ignore non-directories and hidden/relative directories (. and ..).
 * We expect the folder name to be something like 3-0040 or 4-0045.
 */
static inline int is_sensor_dir(struct dirent* entry) {
  return (entry->d_type == DT_LNK || entry->d_type == DT_DIR)
          && entry->d_name[0] != '.'
          && entry->d_name[1] == '-';
}

static inline void free_sensor_directories(char** dirs, unsigned int n) {
  while (n > 0) {
    free(dirs[--n]);
  }
  free(dirs);
}

/**
 * Set the count value to the number of sensors found.
 * Returns a list of directories of size 'count', or NULL on failure.
 */
static inline char** get_sensor_directories(unsigned int* count) {
  unsigned int i;
  int err_save;
  struct dirent* entry;
  char** directories = NULL;
  *count = 0;
  errno = 0;
  DIR* sensors_dir = opendir(INA231_DIR);
  if (sensors_dir == NULL) {
    return NULL;
  }
  while ((entry = readdir(sensors_dir)) != NULL) {
    (*count) += is_sensor_dir(entry) ? 1 : 0;
  }
  if (errno) { // from readdir
    *count = 0;
  } else if (*count > 0) {
    directories = calloc(*count, sizeof(char*));
    if (directories != NULL) {
      rewinddir(sensors_dir);
      for (i = 0; (entry = readdir(sensors_dir)) != NULL && i < *count;) {
        if (is_sensor_dir(entry)) {
          directories[i] = strdup(entry->d_name);
          if (directories[i] == NULL) {
            break; // strdup failed
          }
          i++;
        }
      }
      if (errno) { // from readdir or strdup
        free_sensor_directories(directories, i);
        directories = NULL;
        *count = 0;
      }
    }
  }
  err_save = errno;
  if (closedir(sensors_dir)) {
    perror(INA231_DIR);
  }
  // ENOENT if no error but didn't find any sensor directories
  errno = (!err_save && *count == 0) ? ENOENT : err_save;
  return directories;
}

/**
 * pthread function to poll the sensors at regular intervals.
 */
static void* odroid_poll_sensors(void* args) {
  energymon_odroid* state = (energymon_odroid*) args;
  char cdata[8];
  double sum_w;
  unsigned int i;
  int64_t exec_us;
  int err_save;
  struct timespec ts;
  if (energymon_clock_gettime(&ts)) {
    // must be that CLOCK_MONOTONIC is not supported
    perror("odroid_poll_sensors");
    return (void*) NULL;
  }
  energymon_sleep_us(state->read_delay_us, &state->poll_sensors);
  while (state->poll_sensors) {
    // read individual sensors
    for (sum_w = 0, errno = 0, i = 0; i < state->count && !errno; i++) {
      if (pread(state->fds[i], cdata, sizeof(cdata), 0) > 0) {
        sum_w += strtod(cdata, NULL);
      }
    }
    err_save = errno;
    exec_us = energymon_gettime_us(&ts);
    if (err_save) {
      errno = err_save;
      perror("odroid_poll_sensors: skipping power sensor reading");
    } else {
      state->total_uj += sum_w * exec_us;
    }
    // sleep for the update interval of the sensors (minus most overhead)
    energymon_sleep_us(2 * state->read_delay_us - exec_us, &state->poll_sensors);
    errno = 0;
  }
  return (void*) NULL;
}

/**
 * Open all sensor files and start the thread to poll the sensors.
 */
int energymon_init_odroid(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  unsigned int i;
  char file[64];
  unsigned int count;

  // find the sensors
  char** sensor_dirs = get_sensor_directories(&count);
  if (count == 0) {
    perror("energymon_init_odroid: Failed to find power sensors: "INA231_DIR);
    return -1;
  }

  // ensure that the sensors are enabled
  for (i = 0; i < count; i++) {
    snprintf(file, sizeof(file), INA231_FILE_TEMPLATE_ENABLE, sensor_dirs[i]);
    if (!is_sensor_enabled(file)) {
      if (errno) {
        perror(file);
      } else {
        fprintf(stderr, "energymon_init_odroid: sensor not enabled: %s\n",
                sensor_dirs[i]);
      }
      free_sensor_directories(sensor_dirs, count);
      return -1;
    }
  }

  size_t size = sizeof(energymon_odroid) + count * sizeof(int);
  energymon_odroid* state = calloc(1, size);
  if (state == NULL) {
    free_sensor_directories(sensor_dirs, count);
    return -1;
  }
  state->count = count;

  // open individual sensor files
  em->state = state;
  for (i = 0; i < state->count; i++) {
    snprintf(file, sizeof(file), INA231_FILE_TEMPLATE_POWER, sensor_dirs[i]);
    state->fds[i] = open(file, O_RDONLY);
    if (state->fds[i] < 0) {
      perror(file);
      free_sensor_directories(sensor_dirs, state->count);
      energymon_finish_odroid(em);
      return -1;
    }
  }

  // get the delay time between reads
  state->read_delay_us = get_update_interval(sensor_dirs, state->count);

  // we're finished with this variable
  free_sensor_directories(sensor_dirs, state->count);

  // start sensors polling thread
  state->poll_sensors = 1;
  errno = pthread_create(&state->thread, NULL, odroid_poll_sensors, state);
  if (errno) {
    energymon_finish_odroid(em);
    return -1;
  }

  return 0;
}

uint64_t energymon_read_total_odroid(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  return ((energymon_odroid*) em->state)->total_uj;
}

char* energymon_get_source_odroid(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "ODROID INA231 Power Sensors", n);
}

uint64_t energymon_get_interval_odroid(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  return ((energymon_odroid*) em->state)->read_delay_us;
}

uint64_t energymon_get_precision_odroid(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  // watts to 6 decimal places (microwatts) at refresh interval
  uint64_t prec = energymon_get_interval_odroid(em) / 1000000;
  return prec ? prec : 1;
}

int energymon_is_exclusive_odroid(void) {
  return 0;
}

int energymon_get_odroid(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_odroid;
  em->fread = &energymon_read_total_odroid;
  em->ffinish = &energymon_finish_odroid;
  em->fsource = &energymon_get_source_odroid;
  em->finterval = &energymon_get_interval_odroid;
  em->fprecision = &energymon_get_precision_odroid;
  em->fexclusive = &energymon_is_exclusive_odroid;
  em->state = NULL;
  return 0;
}
