/**
* Energy reading from a Watts Up? device.
*
* @author Connor Imes
* @date 2016-02-08
*/
#ifndef _ENERGYMON_WATTSUP_H_
#define _ENERGYMON_WATTSUP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

// Environment variable for specifying the device file to read from
#define ENERGYMON_WATTSUP_DEV_FILE "ENERGYMON_WATTSUP_DEV_FILE"
#ifndef ENERGYMON_WATTSUP_DEV_FILE_DEFAULT
  #define ENERGYMON_WATTSUP_DEV_FILE_DEFAULT "/dev/ttyUSB0"
#endif

int energymon_init_wattsup(energymon* em);

uint64_t energymon_read_total_wattsup(const energymon* em);

int energymon_finish_wattsup(energymon* em);

char* energymon_get_source_wattsup(char* buffer, size_t n);

uint64_t energymon_get_interval_wattsup(const energymon* em);

uint64_t energymon_get_precision_wattsup(const energymon* em);

int energymon_is_exclusive_wattsup(void);

int energymon_get_wattsup(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
