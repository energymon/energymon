/*
 * Some various functions for dealing with time and sleeping.
 *
 * These are meant to be as portable as possible, though "struct timespec" must be defined.
 *
 * @author Connor Imes
 * @date 2017-02-01
 */
#ifndef _PTIME_H_
#define _PTIME_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined _GNU_SOURCE
#error "Files using this header must be compiled with _GNU_SOURCE, specified before the first include of time.h"
#endif

#define _GNU_SOURCE
#include <inttypes.h>
#include <time.h>

typedef enum ptime_clock_id {
  PTIME_REALTIME = 0,
  PTIME_MONOTONIC
} ptime_clock_id;

/*
 * Conversions
 */

/**
 * Get the time specified by the timespec struct in nanoseconds.
 *
 * @param ts not NULL
 *
 * @return time in nanoseconds
 */
uint64_t ptime_timespec_to_ns(struct timespec* ts);

/**
 * Get the time specified by the timespec struct in microseconds.
 *
 * @param ts not NULL
 *
 * @return time in microseconds
 */
uint64_t ptime_timespec_to_us(struct timespec* ts);

/**
 * Populate the timespec struct with the total number of given nanoseconds.
 *
 * @param ns
 * @param ts not NULL
 */
void ptime_ns_to_timespec(uint64_t ns, struct timespec* ts);

/**
 * Populate the timespec struct with the total number of given microseconds.
 *
 * @param us
 * @param ts not NULL
 */
void ptime_us_to_timespec(uint64_t us, struct timespec* ts);

/*
 * Get time
 */

/**
 * Populate the timespec struct with the current time as supplied by the given clock.
 *
 * @param clk_id
 * @param ts not NULL
 *
 * @return 0 on success, something else on failure
 */
int ptime_clock_gettime(ptime_clock_id clk_id, struct timespec* ts);

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
 * Populate the timespec struct with the current time as supplied by the given clock and return the elapsed time since
 * the old timespec value.
 *
 * @param clk_id
 * @param ts not NULL
 *
 * @return nanoseconds elapsed since the old timespec value
 */
int64_t ptime_gettime_elapsed_ns(ptime_clock_id clk_id, struct timespec* ts);

/**
 * Populate the timespec struct with the current time as supplied by the given clock and return the elapsed time since
 * the old timespec value.
 *
 * @param clk_id
 * @param ts not NULL
 *
 * @return microseconds elapsed since the old timespec value
 */
int64_t ptime_gettime_elapsed_us(ptime_clock_id clk_id, struct timespec* ts);

/*
 * Sleeping
 */

/**
 * Sleep for the amount of time specified in ts.
 * If sleep is interrupted, errno is set to EINTR, and if rem is not NULL, it is populated with the remaining time.
 *
 * ts not NULL
 * rem may be NULL
 *
 * @return 0 on success, -1 otherwise
 */
int ptime_nanosleep(struct timespec* ts, struct timespec* rem);

/**
 * Sleep for the given number of microseconds.
 *
 * @param us
 *
 * @return 0 on success, -1 otherwise
 */
int ptime_sleep_us(uint64_t us);

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

#ifdef __cplusplus
}
#endif

#endif
