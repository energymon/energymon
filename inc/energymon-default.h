/**
 * Get the default energymon implementation.
 *
 * @author Connor Imes
 * @date 2015-08-01
 */
#ifndef _ENERGYMON_DEFAULT_H_
#define _ENERGYMON_DEFAULT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "energymon.h"

/**
 * Get the default energymon implementation.
 * Only fails if em is NULL.
 *
 * @return 0 on success, failure code otherwise
 */
int energymon_get_default(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
