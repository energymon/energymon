/**
 * Energy reading for NVIDIA Jetson systems with INA3221x power sensors.
 *
 * @author Connor Imes
 * @date 2021-12-16
 */
#ifndef _ENERGYMON_JETSON_H_
#define _ENERGYMON_JETSON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

/*
 * Environment variable for specifying a comma-delimited list of sensor rails to use.
 */
#define ENERGYMON_JETSON_RAIL_NAMES "ENERGYMON_JETSON_RAIL_NAMES"

int energymon_init_jetson(energymon* em);

uint64_t energymon_read_total_jetson(const energymon* em);

int energymon_finish_jetson(energymon* em);

char* energymon_get_source_jetson(char* buffer, size_t n);

uint64_t energymon_get_interval_jetson(const energymon* em);

uint64_t energymon_get_precision_jetson(const energymon* em);

int energymon_is_exclusive_jetson(void);

int energymon_get_jetson(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
