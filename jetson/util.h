#ifndef _ENERGYMON_JETSON_UTIL_H_
#define _ENERGYMON_JETSON_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <dirent.h>
#include <limits.h>
#include <stddef.h>

#pragma GCC visibility push(hidden)

/* PATH_MAX should be defined in limits.h */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// The hardware supports up to 3 channels per instance
#define INA3221_CHANNELS_MAX 3

int is_dir(const char* path);

int read_string(const char* file, char* str, size_t len);

long read_long(const char* file);

int is_i2c_bus_addr_dir(const struct dirent* entry);

#pragma GCC visibility pop

#ifdef __cplusplus
}
#endif

#endif
