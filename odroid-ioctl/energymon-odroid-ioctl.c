/**
 * Energy reading for an ODROID with INA231 power sensors, using ioctl on
 * device files instead of sysfs.
 *
 * @author Connor Imes
 * @date 2015-10-14
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
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
int energymon_get_default(energymon* impl) {
  return energymon_get_odroid_ioctl(impl);
}
#endif

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

#define INA231_IOCGREG    _IOR('i', 1, ina231_iocreg_t *)
#define INA231_IOCSSTATUS _IOW('i', 2, ina231_iocreg_t *)
#define INA231_IOCGSTATUS _IOR('i', 3, ina231_iocreg_t *)

#define DEV_SENSOR_ARM "/dev/sensor_arm" // big cluster
#define DEV_SENSOR_KFC "/dev/sensor_kfc" // LITTLE cluster
#define DEV_SENSOR_MEM "/dev/sensor_mem" // memory
#define DEV_SENSOR_G3D "/dev/sensor_g3d" // GPU

#ifndef ODROID_IOCTL_SENSOR_POLL_DELAY_US_DEFAULT
  #define ODROID_IOCTL_SENSOR_POLL_DELAY_US_DEFAULT 263808
#endif

enum {
  SENSOR_ARM = 0,
  SENSOR_KFC,
  SENSOR_MEM,
  SENSOR_G3D,
  SENSOR_MAX
};

typedef struct energymon_odroid_ioctl {
  // sensors
  ina231_sensor_t sensor[SENSOR_MAX];
  // sensor update interval in microseconds
  unsigned long poll_delay_us;
  // total energy estimate
  unsigned long long total_uj;
  // thread variables
  pthread_t sensor_thread;
  int poll_sensors;
} energymon_odroid_ioctl;

/**
 * Open the sensor device file.
 */
static inline int open_sensor_file(const char* file, ina231_sensor_t* sensor) {
  if ((sensor->fd = open(file, O_RDWR)) < 0) {
    fprintf(stderr, "open_sensor_file: Failed to open %s: %s\n", file,
            strerror(errno));
    return -1;
  }
  return 0;
}

/**
 * Close the sensor device file.
 */
static inline int close_sensor_file(ina231_sensor_t* sensor) {
  if (sensor->fd > 0 && close(sensor->fd)) {
    fprintf(stderr, "close_sensor_file: Failed to close sensor %s: %s\n",
            sensor->data.name, strerror(errno));
    return -1;
  }
  return 0;
}

/**
 * Enable/disable sensor.
 */
static inline int enable_sensor(ina231_sensor_t* sensor, int enable) {
  sensor->data.enable = enable ? 1 : 0;
  if (ioctl(sensor->fd, INA231_IOCSSTATUS, &sensor->data) < 0) {
    fprintf(stderr, "enable_sensor: Failed to enable/disable sensor %s: %s\n",
            sensor->data.name, strerror(errno));
    return -1;
  }
  return 0;
}

/**
 * Get the sensor status.
 */
static inline int read_sensor_status(ina231_sensor_t* sensor) {
  if (ioctl(sensor->fd, INA231_IOCGSTATUS, &sensor->data) < 0) {
    fprintf(stderr,
            "read_sensor_status: Failed to read status from sensor %s: %s\n",
            sensor->data.name, strerror(errno));
    return -1;
  }
  return 0;
}

/**
 * Read data from the sensor.
 */
static inline int read_sensor(ina231_sensor_t* sensor) {
  if (ioctl(sensor->fd, INA231_IOCGREG, &sensor->data) < 0) {
    fprintf(stderr, "read_sensor: Failed to read sensor %s: %s\n",
            sensor->data.name, strerror(errno));
    return -1;
  }
  return 0;
}

/**
 * Close all the sensor device files.
 * If ODROID_IOCTL_DISABLE_ON_CLOSE is defined, the sensors will be stopped.
 */
static inline int close_all_sensors(energymon_odroid_ioctl* em) {
  int ret = 0;
  int i = 0;
#ifdef ODROID_IOCTL_DISABLE_ON_CLOSE
  for (i = 0; i < SENSOR_MAX; i++) {
    if (em->sensor[i].data.enable) {
      ret += enable_sensor(&em->sensor[i], 0);
    }
  }
#endif
  for (i = 0; i < SENSOR_MAX; i++) {
    ret += close_sensor_file(&em->sensor[i]);
  }
  return ret;
}

/**
 * Open all the sensor device files, check their status, and enable them.
 */
static inline int open_all_sensors(energymon_odroid_ioctl* em) {
  int ret = 0;
  int i;

  if (open_sensor_file(DEV_SENSOR_ARM, &em->sensor[SENSOR_ARM]) ||
      open_sensor_file(DEV_SENSOR_KFC, &em->sensor[SENSOR_KFC]) ||
      open_sensor_file(DEV_SENSOR_MEM, &em->sensor[SENSOR_MEM]) ||
      open_sensor_file(DEV_SENSOR_G3D, &em->sensor[SENSOR_G3D])) {
    return -1;
  }

  for (i = 0; i < SENSOR_MAX; i++) {
    if (read_sensor_status(&em->sensor[i])) {
      return -1;
    }
  }

  for (i = 0; i < SENSOR_MAX; i++) {
    if (!em->sensor[i].data.enable) {
      ret += enable_sensor(&em->sensor[i], 1);
    }
  }

  return ret;
}

/**
 * Stop the sensors polling pthread, close sensor files, and cleanup.
 */
int energymon_finish_odroid_ioctl(energymon* impl) {
  if (impl == NULL || impl->state == NULL) {
    return -1;
  }

  int ret = 0;
  energymon_odroid_ioctl* em = (energymon_odroid_ioctl*) impl->state;
  // stop sensors polling thread and cleanup
  em->poll_sensors = 0;
  if(pthread_join(em->sensor_thread, NULL)) {
    fprintf(stderr, "energymon_finish_odroid_ioctl: Error joining ODROID "
            "sensor polling thread\n");
    ret += -1;
  }
  ret += close_all_sensors(em);
  impl->state = NULL;
  free(em);
  return ret;
}

/**
 * pthread function to poll the sensors at regular intervals.
 */
void* odroid_ioctl_poll_sensors(void* args) {
  int bad_reading = 0;
  unsigned long long sum_uw = 0;
  int i = 0;
  energymon_odroid_ioctl* em = (energymon_odroid_ioctl*) args;
  struct timespec ts_interval;
  ts_interval.tv_sec = em->poll_delay_us / (1000 * 1000);
  ts_interval.tv_nsec = (em->poll_delay_us % (1000 * 1000) * 1000);
  while(em->poll_sensors > 0) {
    // sleep for the update interval of the sensors
    // TODO: use conditional here so thread can be woken up to end immediately
    nanosleep(&ts_interval, NULL);
    if (!em->poll_sensors) {
      break;
    }

    bad_reading = 0;
    sum_uw = 0;
    for (i = 0; i < SENSOR_MAX; i++) {
      bad_reading += read_sensor(&em->sensor[i]);
      sum_uw += em->sensor[i].data.cur_uW;
    }

    if (bad_reading) {
        fprintf(stderr, "odroid_ioctl_poll_sensors: At least one ODROID power"
                " sensor returned bad value - skipping this reading\n");
    } else {
      em->total_uj += sum_uw * em->poll_delay_us / 1000000;
    }
  }
  return (void*) NULL;
}

/**
 * Open all sensor files and start the thread to poll the sensors.
 */
int energymon_init_odroid_ioctl(energymon* impl) {
  if (impl == NULL || impl->state != NULL) {
    return -1;
  }

  energymon_odroid_ioctl* em = malloc(sizeof(energymon_odroid_ioctl));
  if (em == NULL) {
    return -1;
  }

  // initial values
  em->total_uj = 0;
  memset(&em->sensor, 0, SENSOR_MAX * sizeof(ina231_iocreg_t));
  // TODO: determine at runtime
  em->poll_delay_us = ODROID_IOCTL_SENSOR_POLL_DELAY_US_DEFAULT;

  // open and enable the sensors
  if (open_all_sensors(em)) {
    close_all_sensors(em);
    free(em);
    return -1;
  }

  // start sensors polling thread
  em->poll_sensors = 1;
  if (pthread_create(&em->sensor_thread, NULL,
                     odroid_ioctl_poll_sensors, em)) {
    fprintf(stderr,
            "energymon_init_odroid_ioctl: Failed to start polling thread\n");
    close_all_sensors(em);
    free(em);
    return -1;
  }

  impl->state = em;
  return 0;
}

unsigned long long energymon_read_total_odroid_ioctl(const energymon* impl) {
  if (impl == NULL || impl->state == NULL) {
    return -1;
  }
  return ((energymon_odroid_ioctl*) impl->state)->total_uj;
}

char* energymon_get_source_odroid_ioctl(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "ODROID INA231 Power Sensors via ioctl", n);
}

unsigned long long energymon_get_interval_odroid_ioctl(const energymon* em) {
  return ((energymon_odroid_ioctl*) em->state)->poll_delay_us;
}

int energymon_get_odroid_ioctl(energymon* impl) {
  if (impl == NULL) {
    return -1;
  }
  impl->finit = &energymon_init_odroid_ioctl;
  impl->fread = &energymon_read_total_odroid_ioctl;
  impl->ffinish = &energymon_finish_odroid_ioctl;
  impl->fsource = &energymon_get_source_odroid_ioctl;
  impl->finterval = &energymon_get_interval_odroid_ioctl;
  impl->state = NULL;
  return 0;
}
