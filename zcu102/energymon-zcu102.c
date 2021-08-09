/**
 * Energy reading for a ZCU102 with INA226 power sensors.
 *
 * @author Jason Miller, Connor Imes
 * @date 2017-02-14
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "energymon.h"
#include "energymon-zcu102.h"
#include "energymon-time-util.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_zcu102(em);
}
#endif

#define INA226_DIR "/sys/class/hwmon"
// Alternate directory for power sensors (see get_sensor_directories)
//#define INA226_DIR "/sys/bus/i2c/drivers/ina2xx"
#define INA226_FILE_TEMPLATE_POWER INA226_DIR"/%s/power1_input"
#define INA226_FILE_TEMPLATE_UPDATE_PERIOD INA226_DIR"/%s/update_interval"
#define HWMON_DIR_EXAMPLE "hwmon999" // NOTE: allows for up to 1000 sensors
#define INA226_DEFAULT_UPDATE_INTERVAL_US 35200

//#define ENERGYMON_DEBUG 1

typedef struct energymon_zcu102 {
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
} energymon_zcu102;

static inline long get_update_interval(char** sensors, unsigned int num) {
  unsigned long ret = 0;
  unsigned long tmp;
  unsigned int i;
  char file[64];
  int fd;
  char cdata[24];
  int read_ret;

  for (i = 0; i < num; i++) {
    snprintf(file, sizeof(file), INA226_FILE_TEMPLATE_UPDATE_PERIOD, sensors[i]);
    if ((fd = open(file, O_RDONLY)) <= 0) {
      perror(file);
      continue;
    }
    // Read udpate time interval (values in milliseconds)
    if ((read_ret = read(fd, cdata, sizeof(cdata))) <= 0) {
      perror(file);
    }
#ifdef ENERGYMON_DEBUG
      fprintf(stderr, "get_update_interval: Read value %s bytes from %s\n", cdata, file);
#endif    
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
  ret = ret * 1000;  // Convert milliseconds to microseconds
  if (!ret) {
    ret = INA226_DEFAULT_UPDATE_INTERVAL_US;
#ifdef ENERGYMON_DEBUG
    fprintf(stderr, "get_update_interval: Using default value: %lu us\n", ret);
#endif
  }
  return ret;
}

int energymon_finish_zcu102(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }

  int err_save = 0;
  unsigned int i;
  energymon_zcu102* state = (energymon_zcu102*) em->state;

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
  /*
   * TODO: This implementation works for now but is brittle.  It assumes that
   *   there are 18 sensors and that they are sequentially numbered from 0 to 17.
   *   A better implementation would search the device directories to find all
   *   the INA226's, just in case a future software update changes things.  The
   *   odroid implementation does something like this but the sysfs directory
   *   structure is different on the ZCU102 so the same exact code will not work.
   *   If implementing this, it might be easier to use the alternate INA226_DIR
   *   commented out above.
   */
  unsigned int i;
  char** directories;
  *count = 18;

  directories = calloc(*count, sizeof(char*));
  if (directories == NULL) {
    *count = 0;
    return NULL;
  }

  for (i = 0; i < *count; i++) {
    directories[i] = calloc(1, sizeof(HWMON_DIR_EXAMPLE));
    if (directories[i] == NULL) {
      free_sensor_directories(directories, i);
      *count = 0;
      return NULL;
    }
    snprintf(directories[i], sizeof(HWMON_DIR_EXAMPLE), "hwmon%d", i);
  }

  return directories;
}

/**
 * pthread function to poll the sensors at regular intervals.
 */
static void* zcu102_poll_sensors(void* args) {
  energymon_zcu102* state = (energymon_zcu102*) args;
  char cdata[10];
  double sum_w;
  unsigned long sum_uw;
  unsigned int i;
  uint64_t exec_us;
  uint64_t last_us;
  uint64_t delta_uj;
  int err_save;
  if (!(last_us = energymon_gettime_us())) {
    // must be that CLOCK_MONOTONIC is not supported
    perror("zcu102_poll_sensors");
    return (void*) NULL;
  }
  energymon_sleep_us(state->read_delay_us, &state->poll_sensors);
  while (state->poll_sensors) {
    // read individual sensors (values in microWatts)
    for (sum_uw = 0, errno = 0, i = 0; i < state->count && !errno; i++) {
      if (pread(state->fds[i], cdata, sizeof(cdata), 0) > 0) {
        sum_uw += strtoul(cdata, NULL, 0);
      }
    }
    err_save = errno;
    exec_us = energymon_gettime_elapsed_us(&last_us);
    if (err_save) {
      errno = err_save;
      perror("zcu102_poll_sensors: skipping power sensor reading");
    } else {
      sum_w = sum_uw / 1e6;
      delta_uj = sum_w * exec_us;
#ifdef ENERGYMON_DEBUG
      fprintf(stderr, "zcu102_poll_sensors: Read total power: %lu uW (%f W)\n", sum_uw, sum_w);
      fprintf(stderr, "zcu102_poll_sensors: Calculated energy: %f W * %"PRIi64" us = %"PRIu64" uJ\n", sum_w, exec_us, delta_uj);
#endif
      state->total_uj += delta_uj;
    }
    // sleep for the update interval of the sensors
    if (state->poll_sensors) {
      energymon_sleep_us(state->read_delay_us, &state->poll_sensors);
    }
    errno = 0;
  }
  return (void*) NULL;
}

/**
 * Open all sensor files and start the thread to poll the sensors.
 */
int energymon_init_zcu102(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  unsigned int i;
  char file[64];
  unsigned int count;
  int err_save;

  // find the sensors
  char** sensor_dirs = get_sensor_directories(&count);
  if (count == 0) {
    perror("energymon_init_zcu102: Failed to find power sensors: "INA226_DIR);
    return -1;
  }

  size_t size = sizeof(energymon_zcu102) + count * sizeof(int);
  energymon_zcu102* state = calloc(1, size);
  if (state == NULL) {
    free_sensor_directories(sensor_dirs, count);
    return -1;
  }
  state->count = count;

  // open individual sensor files
  em->state = state;
  for (i = 0; i < state->count; i++) {
    snprintf(file, sizeof(file), INA226_FILE_TEMPLATE_POWER, sensor_dirs[i]);
#ifdef ENERGYMON_DEBUG
    fprintf(stderr, "energymon_init_zcu102: Opening sensor file: %s\n", file);	
#endif
    state->fds[i] = open(file, O_RDONLY);
    if (state->fds[i] < 0) {
      perror(file);
      err_save = errno;
      free_sensor_directories(sensor_dirs, state->count);
      energymon_finish_zcu102(em);
      errno = err_save;
      return -1;
    }
  }

  // get the delay time between reads
  state->read_delay_us = get_update_interval(sensor_dirs, state->count);

  // we're finished with this variable
  free_sensor_directories(sensor_dirs, state->count);

  // start sensors polling thread
  state->poll_sensors = 1;
  errno = pthread_create(&state->thread, NULL, zcu102_poll_sensors, state);
  if (errno) {
    err_save = errno;
    energymon_finish_zcu102(em);
    errno = err_save;
    return -1;
  }

  return 0;
}

uint64_t energymon_read_total_zcu102(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  errno = 0;
  return ((energymon_zcu102*) em->state)->total_uj;
}

char* energymon_get_source_zcu102(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "ZCU102 INA226 Power Sensors", n);
}

uint64_t energymon_get_interval_zcu102(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  return ((energymon_zcu102*) em->state)->read_delay_us;
}

uint64_t energymon_get_precision_zcu102(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  // On the ZCU102, the sensors change in increments of 25 mW so the
  //   minimum energy delta = 25 mW * the update interval time.
  uint64_t prec = 0.025 * energymon_get_interval_zcu102(em);
  return prec ? prec : 1;
}

int energymon_is_exclusive_zcu102(void) {
  return 0;
}

int energymon_get_zcu102(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_zcu102;
  em->fread = &energymon_read_total_zcu102;
  em->ffinish = &energymon_finish_zcu102;
  em->fsource = &energymon_get_source_zcu102;
  em->finterval = &energymon_get_interval_zcu102;
  em->fprecision = &energymon_get_precision_zcu102;
  em->fexclusive = &energymon_is_exclusive_zcu102;
  em->state = NULL;
  return 0;
}
