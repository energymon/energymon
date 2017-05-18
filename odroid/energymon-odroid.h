/**
 * Energy reading for an ODROID with INA231 power sensors.
 *
 * @author Connor Imes
 * @date 2014-06-30
 */
#ifndef _ENERGYMON_ODROID_H_
#define _ENERGYMON_ODROID_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

int energymon_init_odroid(energymon* em);

uint64_t energymon_read_total_odroid(const energymon* em);

int energymon_finish_odroid(energymon* em);

char* energymon_get_source_odroid(char* buffer, size_t n);

uint64_t energymon_get_interval_odroid(const energymon* em);

uint64_t energymon_get_precision_odroid(const energymon* em);

int energymon_is_exclusive_odroid(void);

int energymon_get_odroid(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
