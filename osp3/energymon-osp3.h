/**
 * Read energy from an ODROID Smart Power 3 USB device.
 *
 * @author Connor Imes
 */
#ifndef _ENERGYMON_OSP3_H_
#define _ENERGYMON_OSP3_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

// Environment variable for specifying the device file to read from.
#define ENERGYMON_OSP3_DEV_FILE_ENV_VAR "ENERGYMON_OSP3_DEV_FILE"

// Environment variable for specifying the serial baud.
#define ENERGYMON_OSP3_BAUD_ENV_VAR "ENERGYMON_OSP3_BAUD"

// Environment variable for specifying which device power source to monitor.
#define ENERGYMON_OSP3_POWER_SOURCE_ENV_VAR "ENERGYMON_OSP3_POWER_SOURCE"
#define ENERGYMON_OSP3_POWER_SOURCE_CH0 "0"
#define ENERGYMON_OSP3_POWER_SOURCE_CH1 "1"
#define ENERGYMON_OSP3_POWER_SOURCE_IN "IN"

int energymon_init_osp3(energymon* em);

uint64_t energymon_read_total_osp3(const energymon* em);

int energymon_finish_osp3(energymon* em);

char* energymon_get_source_osp3(char* buffer, size_t n);

uint64_t energymon_get_interval_osp3(const energymon* em);

uint64_t energymon_get_precision_osp3(const energymon* em);

int energymon_is_exclusive_osp3(void);

int energymon_get_osp3(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
