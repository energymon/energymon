#ifndef _ENERGYMON_JETSON_INA3221X_H_
#define _ENERGYMON_JETSON_INA3221X_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#pragma GCC visibility push(hidden)

int ina3221x_exists(void);

int ina3221x_walk_i2c_drivers_dir(const char* const* names, int* fds, size_t len, unsigned long* polling_delay_us_max);

#pragma GCC visibility pop

#ifdef __cplusplus
}
#endif

#endif
