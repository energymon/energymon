/**
 * Common code for reading Cray PM energy files.
 *
 * @author Connor Imes
 * @date 2017-06-28
 */
#ifndef _ENERGYMON_CRAY_PM_COMMON_H_
#define _ENERGYMON_CRAY_PM_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include "energymon.h"

#define CRAY_PM_BASE_DIR "/sys/cray/pm_counters"

typedef struct energymon_cray_pm_common {
  FILE* f;
} energymon_cray_pm_common;

int energymon_cray_pm_common_init(energymon* em, const char* file);

uint64_t energymon_cray_pm_common_read_total(const energymon* em);

int energymon_cray_pm_common_finish(energymon* em);

uint64_t energymon_cray_pm_common_get_interval(const energymon* em);

uint64_t energymon_cray_pm_common_get_precision(const energymon* em);

#ifdef __cplusplus
}
#endif

#endif
