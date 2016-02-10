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
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#define ONE_THOUSAND 1000
#define ONE_MILLION  1000000
#define ONE_BILLION  1000000000

uint64_t energymon_gettime_ns(void) {
static const uint64_t ONE_BILLION_U64 = 1000000000;
#ifdef __MACH__
  // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  return mts.tv_sec * ONE_BILLION_U64 + mts.tv_nsec;
#else
  struct timespec ts;
  // CLOCK_REALTIME is always supported, this should never fail
  clock_gettime(CLOCK_REALTIME, &ts);
  // must use a const or cast a literal - using a simple literal can overflow!
  return ts.tv_sec * ONE_BILLION_U64 + ts.tv_nsec;
#endif
}

int energymon_clock_gettime(struct timespec* ts) {
#ifdef __MACH__
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
  return 0;
#else
  return clock_gettime(CLOCK_MONOTONIC, ts);
#endif
}

static inline int energymon_clock_nanosleep(struct timespec* ts) {
#ifdef __MACH__
  // TODO: Sleep for any remaining time if interrupted
  return nanosleep(ts, NULL);
#else
  int ret;
  // continue sleeping only if interrupted
  while ((ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                ts, NULL)) == EINTR);
  return ret;
#endif
}

int64_t energymon_gettime_us(struct timespec* ts) {
  struct timespec now;
  int64_t result;
  if (energymon_clock_gettime(&now)) {
    // CLOCK_MONOTONIC is not supported (errno will be EINVAL)
    return 0;
  }
  result = (now.tv_sec - ts->tv_sec) * (int64_t) ONE_MILLION +
           (now.tv_nsec - ts->tv_nsec) / (int64_t) ONE_THOUSAND;
  memcpy(ts, &now, sizeof(struct timespec));
  return result;
}

int energymon_sleep_us(int64_t us) {
  struct timespec ts;
  if (us <= 0) {
    return 0;
  }
  if (energymon_clock_gettime(&ts)) {
    // only happens if CLOCK_MONOTONIC isn't supported
    return errno;
  }
  ts.tv_sec += us / (time_t) ONE_MILLION;
  ts.tv_nsec += (us % (long) ONE_MILLION) * (long) ONE_THOUSAND;
  if (ts.tv_nsec >= (long) ONE_BILLION) {
    ts.tv_nsec -= (long) ONE_BILLION;
    ts.tv_sec++;
  }
  return energymon_clock_nanosleep(&ts);
}
