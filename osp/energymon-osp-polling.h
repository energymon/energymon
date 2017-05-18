/**
 * Read energy from an ODROID Smart Power USB device by polling power data.
 *
 * @author Connor Imes
 */
#ifndef _ENERGYMON_OSP_POLLING_H_
#define _ENERGYMON_OSP_POLLING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

int energymon_init_osp_polling(energymon* em);

uint64_t energymon_read_total_osp_polling(const energymon* em);

int energymon_finish_osp_polling(energymon* em);

char* energymon_get_source_osp_polling(char* buffer, size_t n);

uint64_t energymon_get_interval_osp_polling(const energymon* em);

uint64_t energymon_get_precision_osp_polling(const energymon* em);

int energymon_is_exclusive_osp_polling(void);

int energymon_get_osp_polling(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
