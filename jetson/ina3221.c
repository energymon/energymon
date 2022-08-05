/*
 * Support for ina3221 driver.
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ina3221.h"
#include "util.h"

#define INA3221_DIR "/sys/bus/i2c/drivers/ina3221"
#define INA3221_DIR_TEMPLATE_BUS_ADDR INA3221_DIR"/%s/hwmon"
#define INA3221_FILE_TEMPLATE_CHANNEL_NAME INA3221_DIR"/%s/hwmon/%s/in%d_label"
#define INA3221_FILE_TEMPLATE_VOLTAGE INA3221_DIR"/%s/hwmon/%s/in%d_input"
#define INA3221_FILE_TEMPLATE_CURR INA3221_DIR"/%s/hwmon/%s/curr%d_input"
#define INA3221_FILE_TEMPLATE_UPDATE_INTERVAL INA3221_DIR"/%s/hwmon/%s/update_interval"

static int ina3221_read_channel_name(const char* bus_addr, const char* hwmon, int channel, char* name, size_t len) {
  char file[PATH_MAX];
  snprintf(file, sizeof(file), INA3221_FILE_TEMPLATE_CHANNEL_NAME, bus_addr, hwmon, channel);
  return read_string(file, name, len);
}

static long ina3221_read_update_interval_us(const char* bus_addr, const char* hwmon) {
  char file[PATH_MAX];
  snprintf(file, sizeof(file), INA3221_FILE_TEMPLATE_UPDATE_INTERVAL, bus_addr, hwmon);
  return read_long(file) * 1000;
}

static int ina3221_open_voltage_file(const char* bus_addr, const char* hwmon, int channel) {
  char file[PATH_MAX];
  int fd;
  snprintf(file, sizeof(file), INA3221_FILE_TEMPLATE_VOLTAGE, bus_addr, hwmon, channel);
  if ((fd = open(file, O_RDONLY)) < 0) {
    perror(file);
  }
  return fd;
}

static int ina3221_open_curr_file(const char* bus_addr, const char* hwmon, int channel) {
  char file[PATH_MAX];
  int fd;
  snprintf(file, sizeof(file), INA3221_FILE_TEMPLATE_CURR, bus_addr, hwmon, channel);
  if ((fd = open(file, O_RDONLY)) < 0) {
    perror(file);
  }
  return fd;
}

static int is_hwmon_dir(const struct dirent* entry) {
  // format: hwmonN
  return entry->d_type == DT_DIR && !strncmp(entry->d_name, "hwmon", 5);
}

static int ina3221_walk_device_dir(const char* const* names, int* fds_mv, int* fds_ma, size_t len,
                                   unsigned long* update_interval_us_max, const char* bus_addr, const char* hwmon) {
  // for each channel name, open voltage and current files if its name is in list
  // there are 3 channels per device, and all must exist (name is "NC" for channels that aren't connected)
  char name[64];
  size_t i;
  int channel;
  long update_interval_us = -1;
  int err_save;
  // starts channel count at 1
  for (channel = 1; channel <= INA3221_CHANNELS_MAX; channel++) {
    errno = 0;
    if (ina3221_read_channel_name(bus_addr, hwmon, channel, name, sizeof(name)) < 0) {
      return -1;
    }
    for (i = 0; i < len; i++) {
      if (!strncmp(name, names[i], sizeof(name))) {
        if (fds_mv[i]) {
          fprintf(stderr, "Duplicate sensor name: %s\n", name);
          errno = EEXIST;
          return -1;
        }
        if ((fds_mv[i] = ina3221_open_voltage_file(bus_addr, hwmon, channel)) < 0) {
          return -1;
        }
        if ((fds_ma[i] = ina3221_open_curr_file(bus_addr, hwmon, channel)) < 0) {
          err_save = errno;
          close(fds_mv[i]);
          // enforce that fds_mv[i] is set back to 0 so caller doesn't try to close again
          fds_mv[i] = 0;
          errno = err_save;
          return -1;
        }
        // check update interval for device - only need to do once though
        if (update_interval_us < 0) {
          update_interval_us = ina3221_read_update_interval_us(bus_addr, hwmon);
          if (update_interval_us > 0 && (unsigned long) update_interval_us > *update_interval_us_max) {
            *update_interval_us_max = (unsigned long) update_interval_us;
          }
        }
        break;
      }
    }
  }
  return 0;
}

static int ina3221_walk_bus_addr_dir(const char* const* names, int* fds_mv, int* fds_ma, size_t len,
                                     unsigned long* update_interval_us_max, const char* bus_addr) {
  // for each name format hwmon/hwmonX
  DIR* dir;
  const struct dirent* entry;
  char path[PATH_MAX];
  int ret = 0;
  // Path contains a static '/hwmon' suffix - should we really fail if the bus dir exists but not the hwmon subdir?
  snprintf(path, sizeof(path), INA3221_DIR_TEMPLATE_BUS_ADDR, bus_addr);
  if ((dir = opendir(path)) == NULL) {
    perror(path);
    return -1;
  }
  while ((entry = readdir(dir)) != NULL) {
    if (is_hwmon_dir(entry)) {
      if ((ret = ina3221_walk_device_dir(names, fds_mv, fds_ma, len, update_interval_us_max, bus_addr,
           entry->d_name)) < 0) {
        break;
      }
    }
  }
  if (closedir(dir)) {
    perror(path);
  }
  return ret;
}

int ina3221_exists(void) {
  return is_dir(INA3221_DIR);
}

int ina3221_walk_i2c_drivers_dir(const char* const* names, int* fds_mv, int* fds_ma, size_t len,
                                 unsigned long* update_interval_us_max) {
  // for each name format X-ABCDE
  DIR* dir;
  const struct dirent* entry;
  int ret = 0;
  if ((dir = opendir(INA3221_DIR)) == NULL) {
    perror(INA3221_DIR);
    return -1;
  }
  while ((entry = readdir(dir)) != NULL) {
    if (is_i2c_bus_addr_dir(entry)) {
      if ((ret = ina3221_walk_bus_addr_dir(names, fds_mv, fds_ma, len, update_interval_us_max, entry->d_name)) < 0) {
        break;
      }
    }
  }
  if (closedir(dir)) {
    perror(INA3221_DIR);
  }
  return ret;
}
