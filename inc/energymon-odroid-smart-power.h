/**
 * Read energy from an ODROID Smart Power USB device.
 *
 * @author Connor Imes
 */
#ifndef _EM_ODROID_SMART_POWER_H_
#define _EM_ODROID_SMART_POWER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "energymon.h"

int energymon_init_osp(energymon* em);

unsigned long long energymon_read_total_osp(const energymon* em);

int energymon_finish_osp(energymon* em);

char* energymon_get_source_osp(char* buffer);

unsigned long long energymon_get_interval_osp(const energymon* em);

int energymon_get_osp(energymon* impl);

#ifdef __cplusplus
}
#endif

#endif
