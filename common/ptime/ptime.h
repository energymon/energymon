/*
 * Some various functions for dealing with time and sleeping.
 *
 * @author Connor Imes
 * @date 2017-02-01
 */
#ifndef _PTIME_H_
#define _PTIME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#pragma GCC visibility push(hidden)

typedef enum ptime_clock_id {
  PTIME_REALTIME = 0,
  PTIME_MONOTONIC
} ptime_clock_id;

/*
 * Get time
 */

/**
 * Get the current time in nanoseconds as supplied by the given clock.
 *
 * @param clk_id
 *
 * @return the time on success, 0 on failure
 */
uint64_t ptime_gettime_ns(ptime_clock_id clk_id);

/**
 * Get the current time in microseconds as supplied by the given clock.
 *
 * @param clk_id
 *
 * @return the time on success, 0 on failure
 */
uint64_t ptime_gettime_us(ptime_clock_id clk_id);

/**
 * Get the elapsed nanoseconds since the time specified in "since_ns", which is updated with the current time.
 * Returns 0 and sets errno if the time could not be determined or "since_ns" is in the future.
 *
 * @param clk_id
 * @param since_ns
 *
 * @return nanoseconds elapsed, or 0 on failure
 */
uint64_t ptime_gettime_elapsed_ns(ptime_clock_id clk_id, uint64_t* since_ns);

/**
 * Get the elapsed microseconds since the time specified in "since_us", which is updated with the current time.
 * Returns 0 and sets errno if the time could not be determined or "since_us" is in the future.
 *
 * @param clk_id
 * @param since_us
 *
 * @return microseconds elapsed, or 0 on failure
 */
uint64_t ptime_gettime_elapsed_us(ptime_clock_id clk_id, uint64_t* since_us);

/*
 * Sleeping
 */

/**
 * Sleep for the nanoseconds specified.
 * If sleep is interrupted, errno is set to EINTR and the remaining time is returned, otherwise 0 is returned.
 *
 * @param ns
 *
 * @return 0 on success, remaining nanoseconds otherwise
 */
uint64_t ptime_sleep_ns(uint64_t ns);

/**
 * Sleep for the given number of microseconds.
 *
 * @param us
 *
 * @return 0 on success, remaining microseconds otherwise
 */
uint64_t ptime_sleep_us(uint64_t us);

/**
 * Sleep for the given number of microseconds.
 * If the sleep is interrupted with EINTR, goes back to sleep if ignore_interrupt is NULL or evaluates to true.
 *
 * @param us
 * @param ignore_interrupt may be NULL
 *
 * @return 0 on success, -1 otherwise
 */
int ptime_sleep_us_no_interrupt(uint64_t us, volatile const int* ignore_interrupt);

#pragma GCC visibility pop

#ifdef __cplusplus
}
#endif

#endif
