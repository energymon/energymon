/**
 * Read energy from the ibmpowernv kernel module "energy" subfeature.
 *
 * @author Connor Imes
 * @date 2021-08-26
 */
#ifndef _ENERGYMON_IBMPOWERNV_H_
#define _ENERGYMON_IBMPOWERNV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

int energymon_init_ibmpowernv(energymon* em);

uint64_t energymon_read_total_ibmpowernv(const energymon* em);

int energymon_finish_ibmpowernv(energymon* em);

char* energymon_get_source_ibmpowernv(char* buffer, size_t n);

uint64_t energymon_get_interval_ibmpowernv(const energymon* em);

uint64_t energymon_get_precision_ibmpowernv(const energymon* em);

int energymon_is_exclusive_ibmpowernv(void);

int energymon_get_ibmpowernv(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
