/*
 * Support for ina3221x driver.
 *
 * Note: The kernel driver used is NOT the INA3221 driver in the mainline Linux kernel.
 *       Rather, it is from the NVIDIA Linux for Tegra (Linux4Tegra, L4T) tree, at
 *       'nvidia/drivers/staging/iio/meter/ina3221.c'.
 * See: https://developer.nvidia.com/embedded/linux-tegra
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ina3221x.h"
#include "util.h"

#define INA3221X_DIR "/sys/bus/i2c/drivers/ina3221x"
#define INA3221X_DIR_TEMPLATE_BUS_ADDR INA3221X_DIR"/%s"
#define INA3221X_FILE_TEMPLATE_RAIL_NAME INA3221X_DIR"/%s/%s/rail_name_%d"
#define INA3221X_FILE_TEMPLATE_POWER INA3221X_DIR"/%s/%s/in_power%d_input"
#define INA3221X_FILE_TEMPLATE_POLLING_DELAY INA3221X_DIR"/%s/%s/polling_delay_%d"

static int ina3221x_try_read_rail_name(const char* bus_addr, const char* device, int channel, char* name, size_t len) {
  char file[PATH_MAX];
  snprintf(file, sizeof(file), INA3221X_FILE_TEMPLATE_RAIL_NAME, bus_addr, device, channel);
  return read_string(file, name, len);
}

static long ina3221x_try_read_polling_delay_us(const char* bus_addr, const char* device, int channel) {
  char file[PATH_MAX];
  snprintf(file, sizeof(file), INA3221X_FILE_TEMPLATE_POLLING_DELAY, bus_addr, device, channel);
  // TODO: output includes precision (e.g., "0 ms") - set endptr in the read and parse this
  return read_long(file) * 1000;
}

static int ina3221x_open_power_file(const char* bus_addr, const char* device, int channel) {
  char file[PATH_MAX];
  int fd;
  snprintf(file, sizeof(file), INA3221X_FILE_TEMPLATE_POWER, bus_addr, device, channel);
  if ((fd = open(file, O_RDONLY)) < 0) {
    perror(file);
  }
  return fd;
}

static int is_iio_device_dir(const struct dirent* entry) {
  // format: iio:deviceN
  return entry->d_type == DT_DIR
         && strlen(entry->d_name) > 10
         && entry->d_name[0] != '.'
         && entry->d_name[3] == ':';
}

static int ina3221x_walk_device_dir(const char* const* names, int* fds, size_t len, unsigned long* polling_delay_us_max,
                                    const char* bus_addr, const char* device) {
  // for each rail_name_X, open power file if its name is in list
  // there are 3 channels per device, but it's possible they aren't all connected
  char name[64];
  size_t i;
  int channel;
  long polling_delay_us;
  for (channel = 0; channel < INA3221_CHANNELS_MAX; channel++) {
    errno = 0;
    if (ina3221x_try_read_rail_name(bus_addr, device, channel, name, sizeof(name)) < 0) {
      if (errno == ENOENT) {
        // assume this means the channel isn't connected, which is not an error
        continue;
      }
      return -1;
    } else {
      for (i = 0; i < len; i++) {
        if (!strncmp(name, names[i], sizeof(name))) {
          if (fds[i]) {
            fprintf(stderr, "Duplicate sensor name: %s\n", name);
            errno = EEXIST;
            return -1;
          }
          if ((fds[i] = ina3221x_open_power_file(bus_addr, device, channel)) < 0) {
            return -1;
          }
          polling_delay_us = ina3221x_try_read_polling_delay_us(bus_addr, device, channel);
          if (polling_delay_us > 0 && (unsigned long) polling_delay_us > *polling_delay_us_max) {
            *polling_delay_us_max = (unsigned long) polling_delay_us;
          }
          break;
        }
      }
    }
  }
  return 0;
}

static int ina3221x_walk_bus_addr_dir(const char* const* names, int* fds, size_t len,
                                      unsigned long* polling_delay_us_max, const char* bus_addr) {
  // for each name format iio:deviceX
  DIR* dir;
  const struct dirent* entry;
  char path[PATH_MAX];
  int ret = 0;
  snprintf(path, sizeof(path), INA3221X_DIR_TEMPLATE_BUS_ADDR, bus_addr);
  if ((dir = opendir(path)) == NULL) {
    perror(path);
    return -1;
  }
  while ((entry = readdir(dir)) != NULL) {
    if (is_iio_device_dir(entry)) {
      if ((ret = ina3221x_walk_device_dir(names, fds, len, polling_delay_us_max, bus_addr, entry->d_name)) < 0) {
        break;
      }
    }
  }
  if (closedir(dir)) {
    perror(path);
  }
  return ret;
}

int ina3221x_exists(void) {
  return is_dir(INA3221X_DIR);
}

int ina3221x_walk_i2c_drivers_dir(const char* const* names, int* fds, size_t len, unsigned long* polling_delay_us_max) {
  // for each name format X-ABCDE
  DIR* dir;
  const struct dirent* entry;
  int ret = 0;
  if ((dir = opendir(INA3221X_DIR)) == NULL) {
    perror(INA3221X_DIR);
    return -1;
  }
  while ((entry = readdir(dir)) != NULL) {
    if (is_i2c_bus_addr_dir(entry)) {
      if ((ret = ina3221x_walk_bus_addr_dir(names, fds, len, polling_delay_us_max, entry->d_name)) < 0) {
        break;
      }
    }
  }
  if (closedir(dir)) {
    perror(INA3221X_DIR);
  }
  return ret;
}
