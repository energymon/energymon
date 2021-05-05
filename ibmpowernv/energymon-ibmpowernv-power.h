/**
 * Read energy from the ibmpowernv kernel module "power" subfeature.
 *
 * @author Connor Imes
 * @date 2021-04-02
 */
#ifndef _ENERGYMON_IBMPOWERNV_POWER_H_
#define _ENERGYMON_IBMPOWERNV_POWER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

int energymon_init_ibmpowernv_power(energymon* em);

uint64_t energymon_read_total_ibmpowernv_power(const energymon* em);

int energymon_finish_ibmpowernv_power(energymon* em);

char* energymon_get_source_ibmpowernv_power(char* buffer, size_t n);

uint64_t energymon_get_interval_ibmpowernv_power(const energymon* em);

uint64_t energymon_get_precision_ibmpowernv_power(const energymon* em);

int energymon_is_exclusive_ibmpowernv_power(void);

int energymon_get_ibmpowernv_power(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
