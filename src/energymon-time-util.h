/**
 * Internal utility functions that depend on librt.
 */
#include <inttypes.h>
#include <time.h>

/**
 * Compute the elapsed time in microseconds since the given timespec.
 * If clk_id is supported, no errors should occur.
 * The given timespec will be updated with the current time.
 *
 * @param clk_id
 *  the type of clock used to create the timespec
 * @param ts
 *  pointer to timespec (must not be NULL)
 * @return microseconds elapsed since ts, or 0 on failure (check errno)
 */
int64_t energymon_gettime_us(clockid_t clk_id, struct timespec* ts);

/**
 * Sleep for the specified number of microseconds. If us <= 0, no sleep occurs.
 * If CLOCK_MONOTONIC is supported, no errors should occur.
 *
 * @param us
 *  the number of microseconds
 * @return 0 on success, error code on failure
 */
int energymon_sleep_us(uint64_t us);
