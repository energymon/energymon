#ifndef _ENERGYMON_JETSON_INA3221_H_
#define _ENERGYMON_JETSON_INA3221_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#pragma GCC visibility push(hidden)

int ina3221_exists(void);

int ina3221_walk_i2c_drivers_dir(const char* const* names, int* fds_mv, int* fds_ma, size_t len,
                                 unsigned long* update_interval_us_max);

#pragma GCC visibility pop

#ifdef __cplusplus
}
#endif

#endif
