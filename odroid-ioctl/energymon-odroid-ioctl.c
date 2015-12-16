/**
 * Energy reading for an ODROID with INA231 power sensors, using ioctl on
 * device files instead of sysfs.
 *
 * @author Connor Imes
 * @date 2015-10-14
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "energymon.h"
#include "energymon-odroid-ioctl.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_odroid_ioctl(em);
}
#endif

#define SENSOR_POLL_DELAY_US_DEFAULT 263808
#define SENSOR_COUNT 4

#define INA231_IOCGREG    _IOR('i', 1, ina231_iocreg_t *)
#define INA231_IOCSSTATUS _IOW('i', 2, ina231_iocreg_t *)
#define INA231_IOCGSTATUS _IOR('i', 3, ina231_iocreg_t *)

/**
 * This struct is defined in the kernel: drivers/hardkernel/ina231-misc.h
 * See the branches in HardKernel's fork.
 */
typedef struct ina231_iocreg {
  char name[20];
  unsigned int enable;
  unsigned int cur_uV;
  unsigned int cur_uA;
  unsigned int cur_uW;
} ina231_iocreg_t;

typedef struct ina231_sensor {
  int fd;
  ina231_iocreg_t data;
} ina231_sensor_t;

static const char* dev_sensor[] = {
  "/dev/sensor_arm", // big cluster
  "/dev/sensor_kfc", // LITTLE cluster
  "/dev/sensor_mem", // memory
  "/dev/sensor_g3d"  // GPU
};

typedef struct energymon_odroid_ioctl {
  // sensors
  ina231_sensor_t sensor[SENSOR_COUNT];
  // sensor update interval in microseconds
  unsigned long poll_delay_us;
  // total energy estimate
  uint64_t total_uj;
  // thread variables
  pthread_t thread;
  int poll_sensors;
} energymon_odroid_ioctl;

static inline int set_sensor_enable(ina231_sensor_t* sensor, int enable) {
  return ioctl(sensor->fd, INA231_IOCSSTATUS, &sensor->data) < 0;
}

static inline int read_sensor_status(ina231_sensor_t* sensor) {
  return ioctl(sensor->fd, INA231_IOCGSTATUS, &sensor->data) < 0;
}

static inline int read_sensor_data(ina231_sensor_t* sensor) {
  return ioctl(sensor->fd, INA231_IOCGREG, &sensor->data) < 0;
}

/**
 * Close all the sensor device files.
 * If ODROID_IOCTL_DISABLE_ON_CLOSE is defined, the sensors will be stopped.
 */
static inline int close_all_sensors(energymon_odroid_ioctl* em) {
  int ret = 0;
  unsigned int i;
  for (i = 0; i < SENSOR_COUNT; i++) {
#ifdef ODROID_IOCTL_DISABLE_ON_CLOSE
    if (em->sensor[i].fd > 0 &&
        em->sensor[i].data.enable && set_sensor_enable(&em->sensor[i], 0)) {
      perror(em->sensor[i].data.name);
      ret = -1;
    }
#endif
    if (em->sensor[i].fd > 0 && close(em->sensor[i].fd)) {
      perror(dev_sensor[i]);
      ret = -1;
    }
  }
  return ret;
}

/**
 * Open all the sensor device files, check their status, and enable them.
 */
static inline int open_all_sensors(energymon_odroid_ioctl* em) {
  unsigned int i;
  for (i = 0; i < SENSOR_COUNT; i++) {
    if ((em->sensor[i].fd = open(dev_sensor[i], O_RDWR)) <= 0 ||
        read_sensor_status(&em->sensor[i]) ||
        (!em->sensor[i].data.enable && set_sensor_enable(&em->sensor[i], 1))) {
      perror(dev_sensor[i]);
      return -1;
    }
  }
  return 0;
}

int energymon_finish_odroid_ioctl(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }

  int err_save;
  energymon_odroid_ioctl* state = (energymon_odroid_ioctl*) em->state;
  // stop sensors polling thread and cleanup
  state->poll_sensors = 0;
  err_save = pthread_join(state->thread, NULL);
  if (close_all_sensors(state)) {
    err_save = err_save ? err_save : errno;
  }
  free(em->state);
  em->state = NULL;
  errno = err_save;
  return errno ? -1 : 0;
}

/**
 * pthread function to poll the sensors at regular intervals.
 */
static void* odroid_ioctl_poll_sensors(void* args) {
  energymon_odroid_ioctl* state = (energymon_odroid_ioctl*) args;
  uint64_t sum_uw;
  unsigned int i;
  struct timespec ts_interval;
  ts_interval.tv_sec = state->poll_delay_us / (1000 * 1000);
  ts_interval.tv_nsec = (state->poll_delay_us % (1000 * 1000) * 1000);
  nanosleep(&ts_interval, NULL);
  while (state->poll_sensors) {
    // read individual sensors
    for (errno = 0, sum_uw = 0, i = 0; i < SENSOR_COUNT && !errno; i++) {
      if (!read_sensor_data(&state->sensor[i])) {
        sum_uw += state->sensor[i].data.cur_uW;
      }
    }
    if (errno) {
      perror("odroid_ioctl_poll_sensors: skipping power sensor reading");
    } else {
      state->total_uj += sum_uw * state->poll_delay_us / 1000000;
    }
    // sleep for the update interval of the sensors
    nanosleep(&ts_interval, NULL);
  }
  return (void*) NULL;
}

/**
 * Open all sensor files and start the thread to poll the sensors.
 */
int energymon_init_odroid_ioctl(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  int err_save;
  energymon_odroid_ioctl* state = calloc(1, sizeof(energymon_odroid_ioctl));
  if (state == NULL) {
    return -1;
  }

  // TODO: determine at runtime
  state->poll_delay_us = SENSOR_POLL_DELAY_US_DEFAULT;

  // open and enable the sensors
  if (open_all_sensors(state)) {
    close_all_sensors(state);
    free(state);
    return -1;
  }

  // start sensors polling thread
  state->poll_sensors = 1;
  errno = pthread_create(&state->thread, NULL, odroid_ioctl_poll_sensors,
                         state);
  if (errno) {
    err_save = errno;
    close_all_sensors(state);
    free(state);
    errno = err_save;
    return -1;
  }

  em->state = state;
  return 0;
}

uint64_t energymon_read_total_odroid_ioctl(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  return ((energymon_odroid_ioctl*) em->state)->total_uj;
}

char* energymon_get_source_odroid_ioctl(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "ODROID INA231 Power Sensors via ioctl", n);
}

uint64_t energymon_get_interval_odroid_ioctl(const energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return 0;
  }
  return ((energymon_odroid_ioctl*) em->state)->poll_delay_us;
}

int energymon_get_odroid_ioctl(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_odroid_ioctl;
  em->fread = &energymon_read_total_odroid_ioctl;
  em->ffinish = &energymon_finish_odroid_ioctl;
  em->fsource = &energymon_get_source_odroid_ioctl;
  em->finterval = &energymon_get_interval_odroid_ioctl;
  em->state = NULL;
  return 0;
}
