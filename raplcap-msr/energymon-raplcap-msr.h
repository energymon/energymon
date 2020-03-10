/**
 * Read energy from Intel RAPL using the raplcap-msr library.
 *
 * To specify which RAPL zone to use, set the environment variable ENERGYMON_RAPLCAP_MSR_ZONE to one of:
 *   "PACKAGE" (default)
 *   "CORE"
 *   "UNCORE"
 *   "DRAM"
 *   "PSYS" or "PLATFORM"
 *
 * By default, all RAPL instances are read. To configure for only specific instances, set the
 * ENERGYMON_RAPLCAP_MSR_INSTANCES environment variable with a comma-delimited list of IDs, e.g., on a quad-socket
 * system, to use only instances (sockets) 0 and 2 (and ignore 1 and 3):
 *   export ENERGYMON_RAPLCAP_MSR_INSTANCES=0,2
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
/* Environment variable for specifying the RAPL instances (e.g., sockets) to use */
#define ENERGYMON_RAPLCAP_MSR_INSTANCES "ENERGYMON_RAPLCAP_MSR_INSTANCES"

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
