/**
 * Utility functions.
 *
 * @author Connor Imes
 * @date 2015-12-24
 */
#define _GNU_SOURCE
#include <inttypes.h>
#include <time.h>
#include "energymon-time-util.h"
#include "ptime/ptime.h"

uint64_t energymon_gettime_ns(void) {
  return ptime_gettime_ns(PTIME_MONOTONIC);
}

int energymon_clock_gettime(struct timespec* ts) {
  return ptime_clock_gettime(PTIME_MONOTONIC, ts);
}

int64_t energymon_gettime_us(struct timespec* ts) {
  return ptime_gettime_elapsed_us(PTIME_MONOTONIC, ts);
}

int energymon_sleep_us(int64_t us, volatile const int* ignore_interrupt) {
  return us <= 0 ? 0 : ptime_sleep_us_no_interrupt((int64_t) us, ignore_interrupt);
}
