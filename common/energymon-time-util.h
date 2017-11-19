/**
 * Internal utility functions that depend on librt.
 */
#ifndef _ENERGYMON_TIME_UTIL_H_
#define _ENERGYMON_TIME_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#pragma GCC visibility push(hidden)

/**
 * Get monotonic time in nanoseconds.
 *
 * @return system clock in nanoseconds
 */
uint64_t energymon_gettime_ns(void);

/**
 * Get monotonic time in microseconds.
 *
 * @return system clock in microseconds
 */
uint64_t energymon_gettime_us(void);

/**
 * Get elapsed time in microseconds.
 *
 * @param since_us
 *  previous time in microseconds, must not be NULL, is updated with current time
 * @return elapsed microseconds, or 0 on failure
 */
uint64_t energymon_gettime_elapsed_us(uint64_t* since_us);

/**
 * Sleep for the specified number of microseconds.
 *
 * @param us
 *  the number of microseconds
 * @return 0 on success, error code on failure
 */
int energymon_sleep_us(uint64_t us, volatile const int* ignore_interrupt);

#pragma GCC visibility pop

#ifdef __cplusplus
}
#endif

#endif
