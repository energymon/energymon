/**
 * Utility functions.
 *
 * @author Connor Imes
 * @date 2015-12-24
 */
#include <inttypes.h>
#include "energymon-time-util.h"
#include "ptime/ptime.h"

uint64_t energymon_gettime_ns(void) {
  return ptime_gettime_ns(PTIME_MONOTONIC);
}

uint64_t energymon_gettime_us(void) {
  return ptime_gettime_us(PTIME_MONOTONIC);
}

uint64_t energymon_gettime_elapsed_us(uint64_t* since) {
  return ptime_gettime_elapsed_us(PTIME_MONOTONIC, since);
}

int energymon_sleep_us(uint64_t us, volatile const int* ignore_interrupt) {
  return ptime_sleep_us_no_interrupt(us, ignore_interrupt);
}
