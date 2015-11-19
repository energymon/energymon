/**
 * Read energy from Intel RAPL via sysfs.
 *
 * @author Connor Imes
 * @date 2015-08-04
 */
#ifndef _ENERGYMON_RAPL_H_
#define _ENERGYMON_RAPL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

int energymon_init_rapl(energymon* impl);

uint64_t energymon_read_total_rapl(const energymon* impl);

int energymon_finish_rapl(energymon* impl);

char* energymon_get_source_rapl(char* buffer, size_t n);

uint64_t energymon_get_interval_rapl(const energymon* em);

int energymon_get_rapl(energymon* impl);

#ifdef __cplusplus
}
#endif

#endif
