/**
 * Internal utility functions that depend on librt.
 */
#include <inttypes.h>
#include <time.h>

/**
 * Get monotonic time in nanoseconds.
 *
 * @return system clock in nanoseconds
 */
uint64_t energymon_gettime_ns(void);

/**
 * Get the current monotonic time.
 *
 * @param ts
 *  pointer to timespec (must not be NULL)
 * @return 0 on success, error code on failure
 */
int energymon_clock_gettime(struct timespec* ts);

/**
 * Compute the elapsed monotonic time in microseconds since the given timespec.
 * The given timespec will be updated with the current time.
 *
 * @param ts
 *  pointer to timespec (must not be NULL)
 * @return microseconds elapsed since ts, or 0 on failure (check errno)
 */
int64_t energymon_gettime_us(struct timespec* ts);

/**
 * Sleep for the specified number of microseconds. If us <= 0, no sleep occurs.
 *
 * @param us
 *  the number of microseconds
 * @return 0 on success, error code on failure
 */
int energymon_sleep_us(int64_t us, volatile const int* ignore_interrupt);
