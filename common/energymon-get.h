/**
 * Utility for getting an energymon implementation defined at compile time.
 *
 * @author Connor Imes
 * @date 2020-02-02
 */
#ifndef _ENERGYMON_GET_H_
#define _ENERGYMON_GET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "energymon.h"

#pragma GCC visibility push(hidden)

int energymon_get(energymon* em);

#pragma GCC visibility pop

#ifdef __cplusplus
}
#endif

#endif
