/**
 * Energy reading for an ODROID with INA231 power sensors, using ioctl on
 * device files instead of sysfs.
 *
 * @author Connor Imes
 * @date 2015-10-14
 */
#ifndef _ENERGYMON_ODROID_IOCTL_H_
#define _ENERGYMON_ODROID_IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "energymon.h"

int energymon_init_odroid_ioctl(energymon* em);

unsigned long long energymon_read_total_odroid_ioctl(const energymon* em);

int energymon_finish_odroid_ioctl(energymon* em);

char* energymon_get_source_odroid_ioctl(char* buffer, size_t n);

unsigned long long energymon_get_interval_odroid_ioctl(const energymon* em);

int energymon_get_odroid_ioctl(energymon* impl);

#ifdef __cplusplus
}
#endif

#endif
