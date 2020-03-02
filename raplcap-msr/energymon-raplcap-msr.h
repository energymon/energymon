/**
 * Read energy from X86 MSRs (Model-Specific Registers) using the RAPLCap library for MSR portability and discovery.
 *
 * To specify which RAPL zone to use, set the environment variable ENERGYMON_RAPLCAP_MSR_ZONE to one of:
 *   "PACKAGE" (default)
 *   "CORE"
 *   "UNCORE"
 *   "DRAM"
 *   "PSYS" or "PLATFORM"
 *
 * @author Connor Imes
 * @date 2018-05-19
 */
#ifndef _ENERGYMON_RAPLCAP_MSR_H_
#define _ENERGYMON_RAPLCAP_MSR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

/* Environment variable for specifying the zone to use */
#define ENERGYMON_RAPLCAP_MSR_ZONE "ENERGYMON_RAPLCAP_MSR_ZONE"

int energymon_init_raplcap_msr(energymon* em);

uint64_t energymon_read_total_raplcap_msr(const energymon* em);

int energymon_finish_raplcap_msr(energymon* em);

char* energymon_get_source_raplcap_msr(char* buffer, size_t n);

uint64_t energymon_get_interval_raplcap_msr(const energymon* em);

uint64_t energymon_get_precision_raplcap_msr(const energymon* em);

int energymon_is_exclusive_raplcap_msr(void);

int energymon_get_raplcap_msr(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
