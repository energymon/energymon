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

#include <stddef.h>
#include "energymon.h"

int energymon_init_dummy(energymon* em);

unsigned long long energymon_read_total_dummy(const energymon* em);

int energymon_finish_dummy(energymon* em);

char* energymon_get_source_dummy(char* buffer, size_t n);

unsigned long long energymon_get_interval_dummy(const energymon* em);

int energymon_get_dummy(energymon* impl);

#ifdef __cplusplus
}
#endif

#endif
