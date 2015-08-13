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

#include <stddef.h>
#include "energymon.h"

int energymon_init_osp(energymon* em);

unsigned long long energymon_read_total_osp(const energymon* em);

int energymon_finish_osp(energymon* em);

char* energymon_get_source_osp(char* buffer, size_t n);

unsigned long long energymon_get_interval_osp(const energymon* em);

int energymon_get_osp(energymon* impl);

#ifdef __cplusplus
}
#endif

#endif
