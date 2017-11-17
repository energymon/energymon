/**
 * Energy reading for a ZCU102 with INA226 power sensors.
 *
 * @author Jason Miller
 * @date 2017-02-14
 */
#ifndef _ENERGYMON_ZCU102_H_
#define _ENERGYMON_ZCU102_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

int energymon_init_zcu102(energymon* em);

uint64_t energymon_read_total_zcu102(const energymon* em);

int energymon_finish_zcu102(energymon* em);

char* energymon_get_source_zcu102(char* buffer, size_t n);

uint64_t energymon_get_interval_zcu102(const energymon* em);

uint64_t energymon_get_precision_zcu102(const energymon* em);

int energymon_is_exclusive_zcu102(void);

int energymon_get_zcu102(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
