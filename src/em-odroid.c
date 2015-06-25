/**
 * Energy reading for an ODROID with INA231 power sensors.
 *
 * @author Connor Imes
 * @date 2014-06-30
 */

#include "em-generic.h"
#include "em-odroid.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>

// sensor files
#define ODROID_INA231_DIR "/sys/bus/i2c/drivers/INA231/"
#define ODROID_PWR_FILENAME_TEMPLATE ODROID_INA231_DIR"%s/sensor_W"
#define ODROID_SENSOR_ENABLE_FILENAME_TEMPLATE ODROID_INA231_DIR"%s/enable"
#define ODROID_SENSOR_UPDATE_INTERVAL_FILENAME_TEMPLATE ODROID_INA231_DIR"%s/update_period"

// sensor update interval in microseconds
#define ODROID_SENSOR_READ_DELAY_US_DEFAULT 263808
static int odroid_read_delay_us;

// sensor file descriptors
static int* odroid_pwr_ids = NULL;
static unsigned int odroid_pwr_id_count;

static int64_t odroid_start_time;
static double odroid_total_energy;

// thread variables
static double odroid_pwr_avg;
static double odroid_pwr_avg_last;
static int odroid_pwr_avg_count;
static pthread_mutex_t odroid_sensor_mutex;
static pthread_t odroid_sensor_thread;
static int odroid_read_sensors;

#ifdef EM_GENERIC
int em_init(void) {
  return em_init_odroid();
}

double em_read_total(int64_t last_time, int64_t curr_time) {
  return em_read_total_odroid(last_time, curr_time);
}

int em_finish(void) {
  return em_finish_odroid();
}

char* em_get_source(void) {
  return em_get_source_odroid();
}

em_impl* em_impl_alloc(void) {
  return em_impl_alloc_odroid();
}
#endif

static inline int odroid_open_file(char* filename) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    perror("odroid_open_file");
    fprintf(stderr, "Trying to open %s\n", filename);
  }
  return fd;
}

static inline long get_sensor_update_interval(char* sensor) {
  char ui_filename[BUFSIZ];
  int fd;
  char cdata[BUFSIZ];
  long val;
  int read_ret;

  sprintf(ui_filename, ODROID_SENSOR_UPDATE_INTERVAL_FILENAME_TEMPLATE, sensor);
  fd = odroid_open_file(ui_filename);
  if (fd < 0) {
    return -1;
  }
  read_ret = read(fd, cdata, sizeof(val));
  close(fd);
  if (read_ret < 0) {
    return -1;
  }
  val = atol(cdata);
  return val;
}

/**
 * Return 0 if the sensor is enabled, 1 if not, -1 on error.
 */
static inline int odroid_check_sensor_enabled(char* sensor) {
  char enable_filename[BUFSIZ];
  int fd;
  char cdata[BUFSIZ];
  int val;
  int read_ret;

  sprintf(enable_filename, ODROID_SENSOR_ENABLE_FILENAME_TEMPLATE, sensor);
  fd = odroid_open_file(enable_filename);
  if (fd < 0) {
    return -1;
  }
  read_ret = read(fd, cdata, sizeof(val));
  close(fd);
  if (read_ret < 0) {
    return -1;
  }
  val = atoi(cdata);
  return val == 0 ? 1 : 0;
}

static inline long get_update_interval(char** sensors, unsigned int num) {
  long ret = 0;
  long tmp = 0;
  unsigned int i;

  for (i = 0; i < num; i++) {
    tmp = get_sensor_update_interval(sensors[i]);
    if (tmp < 0) {
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
int em_finish_odroid(void) {
  int ret = 0;
  int i;
  // stop sensors polling thread and cleanup
  odroid_read_sensors = 0;
  if(pthread_join(odroid_sensor_thread, NULL)) {
    fprintf(stderr, "Error joining ODROID sensor polling thread.\n");
    ret -= 1;
  }
  if(pthread_mutex_destroy(&odroid_sensor_mutex)) {
    fprintf(stderr, "Error destroying pthread mutex for ODROID sensor polling thread.\n");
    ret -= 1;
  }

  if (odroid_pwr_ids != NULL) {
    // close individual sensor files
    for (i = 0; i < odroid_pwr_id_count; i++) {
      if (odroid_pwr_ids[i] > 0) {
        ret += close(odroid_pwr_ids[i]);
      }
    }
    free(odroid_pwr_ids);
  }
  return ret;
}

/**
 * Read power from a sensor file
 */
static inline double odroid_read_pwr(int fd) {
  double val;
  char cdata[sizeof(double)];
  int data_size = pread(fd, cdata, sizeof(double), 0);
  if (data_size != sizeof(double)) {
    perror("odroid_read_pwr");
    return -1;
  }
  val = strtod(cdata, NULL);
  return val;
}

static inline char** odroid_get_sensor_directories(unsigned int* count) {
  unsigned int i = 0;
  DIR* sensors_dir;
  struct dirent* entry;
  char** directories = NULL;
  *count = 0;
  if ((sensors_dir = opendir(ODROID_INA231_DIR)) != NULL) {
    while ((entry = readdir(sensors_dir)) != NULL) {
      // ignore non-directories and hidden/relative directories (. and ..)
      if (entry->d_type == DT_LNK && entry->d_name[0] != '.') {
        (*count)++;
      }
    }
    rewinddir(sensors_dir);
    if (*count > 0) {
      directories = malloc(*count * sizeof(char*));
      while ((entry = readdir(sensors_dir)) != NULL && i < *count) {
        if (entry->d_type == DT_LNK && entry->d_name[0] != '.') {
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
 * pthread function to poll the sensors at regular intervals and
 * keep a running average of power between heartbeats.
 */
void* odroid_poll_sensors(void* args) {
  double sum;
  double readings[odroid_pwr_id_count];
  int i;
  int bad_reading;
  struct timespec ts_interval;
  ts_interval.tv_sec = odroid_read_delay_us / (1000 * 1000);
  ts_interval.tv_nsec = (odroid_read_delay_us % (1000 * 1000) * 1000);
  while(odroid_read_sensors > 0) {
    bad_reading = 0;
    sum = 0;
    // read individual sensors
    for (i = 0; i < odroid_pwr_id_count; i++) {
      readings[i] = odroid_read_pwr(odroid_pwr_ids[i]);
      if (readings[i] < 0) {
        fprintf(stderr, "At least one ODROID power sensor returned bad value - skipping this reading\n");
        bad_reading = 1;
        break;
      } else {
        // sum the power values
        sum += readings[i];
      }
    }
    if (bad_reading == 0) {
      // keep running average between heartbeats
      pthread_mutex_lock(&odroid_sensor_mutex);
      odroid_pwr_avg = (sum + odroid_pwr_avg_count * odroid_pwr_avg) / (odroid_pwr_avg_count + 1);
      odroid_pwr_avg_count++;
      pthread_mutex_unlock(&odroid_sensor_mutex);
    }
    // sleep for the update interval of the sensors
    // TODO: Should use a conditional here so thread can be woken up to end immediately
    nanosleep(&ts_interval, NULL);
  }
  return (void*) NULL;
}

/**
 * Open all sensor files and start the thread to poll the sensors.
 */
int em_init_odroid(void) {
  int ret = 0;
  int i;
  char odroid_pwr_filename[BUFSIZ];

  // reset the total energy reading
  odroid_total_energy = 0;

  // find the sensors
  odroid_pwr_id_count = 0;
  char** sensor_dirs = odroid_get_sensor_directories(&odroid_pwr_id_count);
  if (odroid_pwr_id_count == 0) {
    fprintf(stderr, "em_init: Failed to find power sensors\n");
    return -1;
  }
  printf("Found %u INA231 sensors:", odroid_pwr_id_count);
  for (i = 0; i < odroid_pwr_id_count; i++) {
    printf(" %s", sensor_dirs[i]);
  }
  printf("\n");

  // ensure that the sensors are enabled
  for (i = 0; i < odroid_pwr_id_count; i++) {
    if (odroid_check_sensor_enabled(sensor_dirs[i])) {
      fprintf(stderr, "em_init: power sensor not enabled: %s\n", sensor_dirs[i]);
      free(sensor_dirs);
      return -1;
    }
  }

  // open individual sensor files
  odroid_pwr_ids = malloc(odroid_pwr_id_count * sizeof(int));
  for (i = 0; i < odroid_pwr_id_count; i++) {
    sprintf(odroid_pwr_filename, ODROID_PWR_FILENAME_TEMPLATE, sensor_dirs[i]);
    odroid_pwr_ids[i] = odroid_open_file(odroid_pwr_filename);
    if (odroid_pwr_ids[i] < 0) {
      free(sensor_dirs);
      em_finish_odroid();
      return -1;
    }
  }

  // get the delay time between reads
  odroid_read_delay_us = get_update_interval(sensor_dirs, odroid_pwr_id_count);

  // we're finished with this variable
  free(sensor_dirs);

  // track start time
  struct timespec now;
  clock_gettime( CLOCK_REALTIME, &now );
  odroid_start_time = to_nanosec(&now);

  // start sensors polling thread
  odroid_read_sensors = 1;
  odroid_pwr_avg = 0;
  odroid_pwr_avg_last = 0;
  odroid_pwr_avg_count = 0;
  ret = pthread_mutex_init(&odroid_sensor_mutex, NULL);
  if(ret) {
    fprintf(stderr, "Failed to create ODROID sensors mutex.\n");
    em_finish_odroid();
    return ret;
  }
  ret = pthread_create(&odroid_sensor_thread, NULL, odroid_poll_sensors, NULL);
  if (ret) {
    fprintf(stderr, "Failed to start ODROID sensors thread.\n");
    em_finish_odroid();
    return ret;
  }

  return ret;
}

/**
 * Estimate energy from the average power since last heartbeat.
 */
double em_read_total_odroid(int64_t last_time, int64_t curr_time) {
  double result;
  last_time = last_time < 0 ? odroid_start_time : last_time;
  // it's also assumed that curr_time >= last_time

  pthread_mutex_lock(&odroid_sensor_mutex);
  // convert from power to energy using timestamps
  odroid_pwr_avg_last = odroid_pwr_avg > 0 ? odroid_pwr_avg : odroid_pwr_avg_last;
  odroid_total_energy += odroid_pwr_avg_last * diff_sec(last_time, curr_time);
  result = odroid_total_energy;
  // reset running power average
  odroid_pwr_avg = 0;
  odroid_pwr_avg_count = 0;
  pthread_mutex_unlock(&odroid_sensor_mutex);

  return result;
}

char* em_get_source_odroid(void) {
  return "ODROID INA231 Power Sensors";
}

em_impl* em_impl_alloc_odroid(void) {
  em_impl* hei = (em_impl*) malloc(sizeof(em_impl));
  hei->finit = &em_init_odroid;
  hei->fread = &em_read_total_odroid;
  hei->ffinish = &em_finish_odroid;
  hei->fsource = &em_get_source_odroid;
  return hei;
}
