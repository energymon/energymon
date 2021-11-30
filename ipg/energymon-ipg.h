/**
 * Intel Power Gadget implementation.
 *
 * To specify which RAPL zone to use, set the environment variable ENERGYMON_IPG_ZONE to one of:
 *   "PACKAGE" (default)
 *   "CORE" or "IA"
 *   "DRAM"
 *   "PSYS" or "PLATFORM"
 *
 * @author Connor Imes
 * @date 2021-11-29
 */
#ifndef _ENERGYMON_IPG_H_
#define _ENERGYMON_IPG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

/* Environment variable for specifying the zone to use */
#define ENERGYMON_IPG_ZONE "ENERGYMON_IPG_ZONE"

int energymon_init_ipg(energymon* em);

uint64_t energymon_read_total_ipg(const energymon* em);

int energymon_finish_ipg(energymon* em);

char* energymon_get_source_ipg(char* buffer, size_t n);

uint64_t energymon_get_interval_ipg(const energymon* em);

uint64_t energymon_get_precision_ipg(const energymon* em);

int energymon_is_exclusive_ipg(void);

int energymon_get_ipg(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
