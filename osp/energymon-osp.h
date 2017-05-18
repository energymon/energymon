/**
 * Read energy from an ODROID Smart Power USB device.
 *
 * @author Connor Imes
 */
#ifndef _ENERGYMON_OSP_H_
#define _ENERGYMON_OSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

int energymon_init_osp(energymon* em);

uint64_t energymon_read_total_osp(const energymon* em);

int energymon_finish_osp(energymon* em);

char* energymon_get_source_osp(char* buffer, size_t n);

uint64_t energymon_get_interval_osp(const energymon* em);

uint64_t energymon_get_precision_osp(const energymon* em);

int energymon_is_exclusive_osp(void);

int energymon_get_osp(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
