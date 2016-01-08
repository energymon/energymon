/**
 * Utility functions.
 *
 * @author Connor Imes
 * @date 2015-12-24
 */

#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

#define ONE_THOUSAND 1000
#define ONE_MILLION  1000000
#define ONE_BILLION  1000000000

int64_t energymon_gettime_us(clockid_t clk_id, struct timespec* ts) {
  struct timespec now;
  int64_t result;
  if (clock_gettime(clk_id, &now)) {
    // clk_id is not supported (errno will be EINVAL)
    return 0;
  }
  result = (now.tv_sec - ts->tv_sec) * (int64_t) ONE_MILLION +
           (now.tv_nsec - ts->tv_nsec) / (int64_t) ONE_THOUSAND;
  memcpy(ts, &now, sizeof(struct timespec));
  return result;
}

int energymon_sleep_us(int64_t us) {
  struct timespec ts;
  int ret;
  if (us <= 0) {
    return 0;
  }
  if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
    // only happens if CLOCK_MONOTONIC isn't supported
    return errno;
  }
  ts.tv_sec += us / (time_t) ONE_MILLION;
  ts.tv_nsec += (us % (long) ONE_MILLION) * (long) ONE_THOUSAND;
  if (ts.tv_nsec >= (long) ONE_BILLION) {
    ts.tv_nsec -= (long) ONE_BILLION;
    ts.tv_sec++;
  }
  // continue sleeping only if interrupted
  while ((ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                &ts, NULL)) == EINTR) {}
  return ret;
}
