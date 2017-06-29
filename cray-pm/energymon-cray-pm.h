/**
 * Read energy from Cray PM files specified by an environment variable.
 *
 * @author Connor Imes
 * @date 2017-06-28
 */
#ifndef _ENERGYMON_CRAY_PM_H_
#define _ENERGYMON_CRAY_PM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

// Environment variable with a comma-delimited list of counter names to read and sum together
#define ENERGYMON_CRAY_PM_COUNTERS_ENV_VAR "ENERGYMON_CRAY_PM_COUNTERS"
// Available counter names to use
#define ENERGYMON_CRAY_PM_COUNTER_ENERGY "energy"
#define ENERGYMON_CRAY_PM_COUNTER_ACCEL_ENERGY "accel_energy"
#define ENERGYMON_CRAY_PM_COUNTER_CPU_ENERGY "cpu_energy"
#define ENERGYMON_CRAY_PM_COUNTER_MEMORY_ENERGY "memory_energy"

int energymon_init_cray_pm(energymon* em);

uint64_t energymon_read_total_cray_pm(const energymon* em);

int energymon_finish_cray_pm(energymon* em);

char* energymon_get_source_cray_pm(char* buffer, size_t n);

uint64_t energymon_get_interval_cray_pm(const energymon* em);

uint64_t energymon_get_precision_cray_pm(const energymon* em);

int energymon_is_exclusive_cray_pm(void);

int energymon_get_cray_pm(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
