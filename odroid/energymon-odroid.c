/**
 * Energy reading for an ODROID with INA231 power sensors.
 *
 * @author Connor Imes
 * @date 2014-06-30
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "energymon.h"
#include "energymon-odroid.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* impl) {
  return energymon_get_odroid(impl);
}
#endif

// sensor files
#define ODROID_INA231_DIR "/sys/bus/i2c/drivers/INA231"
#define ODROID_PWR_FILENAME_TEMPLATE ODROID_INA231_DIR"/%s/sensor_W"
#define ODROID_SENSOR_ENABLE_FILENAME_TEMPLATE ODROID_INA231_DIR"/%s/enable"
#define ODROID_SENSOR_UPDATE_INTERVAL_FILENAME_TEMPLATE ODROID_INA231_DIR"/%s/update_period"

#define ODROID_SENSOR_READ_DELAY_US_DEFAULT 263808

typedef struct energymon_odroid {
  // sensor update interval in microseconds
  unsigned long odroid_read_delay_us;

  // sensor file descriptors
  int* odroid_pwr_ids;
  unsigned int odroid_pwr_id_count;

  // thread variables
  pthread_t odroid_sensor_thread;
  int odroid_read_sensors;

  unsigned long long odroid_total_energy;
} energymon_odroid;

/**
 * Convert a timespec struct into a microsecond value.
 */
static inline int64_t to_usec(struct timespec* ts) {
  return ts->tv_sec * 1000000 + (ts->tv_nsec / 1000);
}

static inline int odroid_open_file(char* filename) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "odroid_open_file:open: %s: %s\n",
            filename, strerror(errno));
  }
  return fd;
}

static inline unsigned long get_sensor_update_interval(char* sensor) {
  char ui_filename[64];
  int fd;
  char cdata[24];
  unsigned long val;
  int read_ret;

  sprintf(ui_filename,
          ODROID_SENSOR_UPDATE_INTERVAL_FILENAME_TEMPLATE,
          sensor);
  fd = odroid_open_file(ui_filename);
  if (fd < 0) {
    return 0;
  }
  read_ret = read(fd, cdata, sizeof(cdata));
  close(fd);
  if (read_ret < 0) {
    return 0;
  }
  val = strtoul(cdata, NULL, 0);
  return val;
}

/**
 * Return 0 if the sensor is enabled, 1 if not, -1 on error.
 */
static inline int odroid_check_sensor_enabled(char* sensor) {
  char enable_filename[64];
  int fd;
  char cdata[24];
  int val;
  int read_ret;

  sprintf(enable_filename, ODROID_SENSOR_ENABLE_FILENAME_TEMPLATE, sensor);
  fd = odroid_open_file(enable_filename);
  if (fd < 0) {
    return -1;
  }
  read_ret = read(fd, cdata, sizeof(cdata));
  close(fd);
  if (read_ret < 0) {
    return -1;
  }
  val = atoi(cdata);
  return val == 0 ? 1 : 0;
}

static inline long get_update_interval(char** sensors, unsigned int num) {
  unsigned long ret = 0;
  unsigned long tmp = 0;
  unsigned int i;

  for (i = 0; i < num; i++) {
    tmp = get_sensor_update_interval(sensors[i]);
    if (tmp == 0) {
      fprintf(stderr, "get_update_interval: Warning: could not read update "
              "interval from sensor %s\n", sensors[i]);
    }
    // keep the largest update_interval
    ret = tmp > ret ? tmp : ret;
  }

  if (ret == 0) {
    // failed to read values - use default
    ret = ODROID_SENSOR_READ_DELAY_US_DEFAULT;
    fprintf(stderr, "get_update_interval: Using default value: %ld us\n", ret);
  }
  return ret;
}

/**
 * Stop the sensors polling pthread, cleanup, and close sensor files.
 */
int energymon_finish_odroid(energymon* impl) {
  if (impl == NULL || impl->state == NULL) {
    return -1;
  }

  int ret = 0;
  int i;
  energymon_odroid* em = (energymon_odroid*) impl->state;
  // stop sensors polling thread and cleanup
  em->odroid_read_sensors = 0;
  if(pthread_join(em->odroid_sensor_thread, NULL)) {
    fprintf(stderr, "energymon_finish_odroid: Error joining ODROID sensor "
            "polling thread\n");
    ret -= 1;
  }

  if (em->odroid_pwr_ids != NULL) {
    // close individual sensor files
    for (i = 0; i < em->odroid_pwr_id_count; i++) {
      if (em->odroid_pwr_ids[i] > 0) {
        ret += close(em->odroid_pwr_ids[i]);
      }
    }
    free(em->odroid_pwr_ids);
    free(impl->state);
    impl->state = NULL;
  }
  return ret;
}

/**
 * Read power from a sensor file
 */
static inline double odroid_read_pwr(int fd) {
  double val;
  char cdata[sizeof(double)];
  int data_size = pread(fd, cdata, sizeof(cdata), 0);
  if (data_size != sizeof(cdata)) {
    perror("odroid_read_pwr");
    return -1;
  }
  val = strtod(cdata, NULL);
  return val;
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

static inline char** odroid_get_sensor_directories(unsigned int* count) {
  unsigned int i = 0;
  DIR* sensors_dir;
  struct dirent* entry;
  char** directories = NULL;
  *count = 0;
  if ((sensors_dir = opendir(ODROID_INA231_DIR)) != NULL) {
    while ((entry = readdir(sensors_dir)) != NULL) {
      if (is_sensor_dir(entry)) {
        (*count)++;
      }
    }
    rewinddir(sensors_dir);
    if (*count > 0) {
      directories = malloc(*count * sizeof(char*));
      while ((entry = readdir(sensors_dir)) != NULL && i < *count) {
        if (is_sensor_dir(entry)) {
          directories[i++] = entry->d_name;
        }
      }
    }
    closedir(sensors_dir);
  } else {
    perror("get_sensors");
  }
  return directories;
}

/**
 * pthread function to poll the sensors at regular intervals.
 */
void* odroid_poll_sensors(void* args) {
  energymon_odroid* em = (energymon_odroid*) args;
  double sum;
  double readings[em->odroid_pwr_id_count];
  int i;
  int bad_reading;
  struct timespec ts_interval;
  ts_interval.tv_sec = em->odroid_read_delay_us / (1000 * 1000);
  ts_interval.tv_nsec = (em->odroid_read_delay_us % (1000 * 1000) * 1000);
  while(em->odroid_read_sensors > 0) {
    bad_reading = 0;
    sum = 0;
    // read individual sensors
    for (i = 0; i < em->odroid_pwr_id_count; i++) {
      readings[i] = odroid_read_pwr(em->odroid_pwr_ids[i]);
      if (readings[i] < 0) {
        fprintf(stderr, "odroid_poll_sensors: At least one ODROID power sensor"
                " returned bad value - skipping this reading\n");
        bad_reading = 1;
        break;
      } else {
        // sum the power values
        sum += readings[i];
      }
    }
    if (bad_reading == 0) {
      em->odroid_total_energy += sum * to_usec(&ts_interval);
    }
    // sleep for the update interval of the sensors
    // TODO: use conditional here so thread can be woken up to end immediately
    nanosleep(&ts_interval, NULL);
  }
  return (void*) NULL;
}

/**
 * Open all sensor files and start the thread to poll the sensors.
 */
int energymon_init_odroid(energymon* impl) {
  if (impl == NULL || impl->state != NULL) {
    return -1;
  }

  int ret = 0;
  int i;
  char odroid_pwr_filename[64];

  energymon_odroid* em = malloc(sizeof(energymon_odroid));
  if (em == NULL) {
    return -1;
  }
  impl->state = em;

  // reset the total energy reading
  em->odroid_total_energy = 0;

  // find the sensors
  em->odroid_pwr_id_count = 0;
  char** sensor_dirs = odroid_get_sensor_directories(&em->odroid_pwr_id_count);
  if (em->odroid_pwr_id_count == 0) {
    fprintf(stderr, "energymon_init_odroid: Failed to find power sensors\n");
    return -1;
  }
  printf("energymon_init_odroid: Found %u INA231 sensors:",
         em->odroid_pwr_id_count);
  for (i = 0; i < em->odroid_pwr_id_count; i++) {
    printf(" %s", sensor_dirs[i]);
  }
  printf("\n");

  // ensure that the sensors are enabled
  for (i = 0; i < em->odroid_pwr_id_count; i++) {
    if (odroid_check_sensor_enabled(sensor_dirs[i])) {
      fprintf(stderr, "energymon_init_odroid: power sensor not enabled: %s\n",
              sensor_dirs[i]);
      free(sensor_dirs);
      return -1;
    }
  }

  // open individual sensor files
  em->odroid_pwr_ids = malloc(em->odroid_pwr_id_count * sizeof(int));
  if (em->odroid_pwr_ids == NULL) {
    free(impl->state);
    impl->state = NULL;
    return -1;
  }
  for (i = 0; i < em->odroid_pwr_id_count; i++) {
    sprintf(odroid_pwr_filename, ODROID_PWR_FILENAME_TEMPLATE, sensor_dirs[i]);
    em->odroid_pwr_ids[i] = odroid_open_file(odroid_pwr_filename);
    if (em->odroid_pwr_ids[i] < 0) {
      free(sensor_dirs);
      energymon_finish_odroid(impl);
      return -1;
    }
  }

  // get the delay time between reads
  em->odroid_read_delay_us = get_update_interval(sensor_dirs,
                                                 em->odroid_pwr_id_count);

  // we're finished with this variable
  free(sensor_dirs);

  // start sensors polling thread
  em->odroid_read_sensors = 1;
  ret = pthread_create(&em->odroid_sensor_thread,
                       NULL,
                       odroid_poll_sensors,
                       em);
  if (ret) {
    fprintf(stderr,
            "energymon_init_odroid: Failed to start ODROID sensors thread\n");
    energymon_finish_odroid(impl);
    return ret;
  }

  return ret;
}

unsigned long long energymon_read_total_odroid(const energymon* impl) {
  if (impl == NULL || impl->state == NULL) {
    return -1;
  }
  return ((energymon_odroid*) impl->state)->odroid_total_energy;
}

char* energymon_get_source_odroid(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "ODROID INA231 Power Sensors", n);
}

unsigned long long energymon_get_interval_odroid(const energymon* em) {
  return ((energymon_odroid*) em->state)->odroid_read_delay_us;
}

int energymon_get_odroid(energymon* impl) {
  if (impl == NULL) {
    return -1;
  }
  impl->finit = &energymon_init_odroid;
  impl->fread = &energymon_read_total_odroid;
  impl->ffinish = &energymon_finish_odroid;
  impl->fsource = &energymon_get_source_odroid;
  impl->finterval = &energymon_get_interval_odroid;
  impl->state = NULL;
  return 0;
}
