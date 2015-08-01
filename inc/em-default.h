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

int em_impl_get(em_impl* impl);

#ifdef __cplusplus
}
#endif

#endif
