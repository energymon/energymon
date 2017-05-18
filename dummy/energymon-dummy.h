/**
 * Dummy implementation - doesn't read from any source.
 *
 * @author Connor Imes
 * @date 2014-07-30
 */
#ifndef _ENERGYMON_DUMMY_H_
#define _ENERGYMON_DUMMY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

int energymon_init_dummy(energymon* em);

uint64_t energymon_read_total_dummy(const energymon* em);

int energymon_finish_dummy(energymon* em);

char* energymon_get_source_dummy(char* buffer, size_t n);

uint64_t energymon_get_interval_dummy(const energymon* em);

uint64_t energymon_get_precision_dummy(const energymon* em);

int energymon_is_exclusive_dummy(void);

int energymon_get_dummy(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
