/**
 * Read energy from Cray PM "energy" file.
 *
 * @author Connor Imes
 * @date 2017-06-28
 */
#ifndef _ENERGYMON_CRAY_PM_ENERGY_H_
#define _ENERGYMON_CRAY_PM_ENERGY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

int energymon_init_cray_pm_energy(energymon* em);

uint64_t energymon_read_total_cray_pm_energy(const energymon* em);

int energymon_finish_cray_pm_energy(energymon* em);

char* energymon_get_source_cray_pm_energy(char* buffer, size_t n);

uint64_t energymon_get_interval_cray_pm_energy(const energymon* em);

uint64_t energymon_get_precision_cray_pm_energy(const energymon* em);

int energymon_is_exclusive_cray_pm_energy(void);

int energymon_get_cray_pm_energy(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
