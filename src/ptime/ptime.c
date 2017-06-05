/*
 * Some various functions for dealing with time and sleeping.
 *
 * These are meant to be as portable as possible, though "struct timespec" must be defined.
 *
 * Many thanks to online sources, like:
 *   http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x
 *   https://janus.conf.meetecho.com/docs/mach__gettime_8h_source.html
 *   http://nadeausoftware.com/articles/2012/04/c_c_tip_how_measure_elapsed_real_time_benchmarking
 *   http://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
 *   http://stackoverflow.com/questions/5801813/c-usleep-is-obsolete-workarounds-for-windows-mingw
 *
 * @author Connor Imes
 * @date 2017-02-01
 */
#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include "ptime.h"

/* begin platform-specific headers and definitions */
#if defined(__MACH__)

#include <mach/clock.h>
#include <mach/mach.h>

#elif defined(_WIN32)

#include <Windows.h>

#else

static const clockid_t PTIME_CLOCKID_T_MONOTONIC =
#if defined(CLOCK_MONOTONIC_PRECISE)
  // BSD
  CLOCK_MONOTONIC_PRECISE
// #elif defined(CLOCK_MONOTONIC_RAW)
//   // Linux
//   CLOCK_MONOTONIC_RAW
#elif defined(CLOCK_HIGHRES)
  // Solaris
  CLOCK_HIGHRES;
#elif defined(CLOCK_MONOTONIC)
  // AIX, BSD, Linux, POSIX, Solaris
  CLOCK_MONOTONIC
#else
  // AIX, BSD, HP-UX, Linux, POSIX
  CLOCK_REALTIME
#endif
;

#endif
/* end platform-specific headers and definitions */

#define ONE_THOUSAND 1000
#define ONE_MILLION  1000000
#define ONE_BILLION  1000000000

#if defined(__MACH__)
static int clock_gettime_mach(clock_id_t clk_id, struct timespec* ts) {
  clock_serv_t cclock;
  mach_timespec_t mts;
  int ret;
  host_get_clock_service(mach_host_self(), clk_id, &cclock);
  ret = clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  if (!ret) {
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
  }
  return ret;
}
#endif // __MACH__

#if defined(_WIN32)
static LARGE_INTEGER getFILETIMEoffset(void) {
  SYSTEMTIME s;
  FILETIME f;
  LARGE_INTEGER t;
  s.wYear = 1970;
  s.wMonth = 1;
  s.wDay = 1;
  s.wHour = 0;
  s.wMinute = 0;
  s.wSecond = 0;
  s.wMilliseconds = 0;
  SystemTimeToFileTime(&s, &f);
  t.QuadPart = f.dwHighDateTime;
  t.QuadPart <<= 32;
  t.QuadPart |= f.dwLowDateTime;
  return t;
}

static int clock_gettime_realtime_win32(struct timespec* ts) {
  static LONG g_first_time = 1;
  static LARGE_INTEGER offset;
  static double frequencyToNanoseconds;
  FILETIME f;
  LARGE_INTEGER t;
  double nanoseconds;
  // TODO: thread-safe initializer
  if (g_first_time) {
    offset = getFILETIMEoffset();
    frequencyToNanoseconds = 0.010;
    g_first_time = 0;
  }
  GetSystemTimeAsFileTime(&f);
  t.QuadPart = f.dwHighDateTime;
  t.QuadPart <<= 32;
  t.QuadPart |= f.dwLowDateTime;
  t.QuadPart -= offset.QuadPart;
  nanoseconds = (double) t.QuadPart / frequencyToNanoseconds;
  t.QuadPart = nanoseconds;
  ts->tv_sec = t.QuadPart / ONE_BILLION;
  ts->tv_nsec = t.QuadPart % ONE_BILLION;
  return 0;
}

static int clock_gettime_monotonic_win32(struct timespec* ts) {
  static LONG g_first_time = 1;
  static LARGE_INTEGER g_counts_per_sec;
  LARGE_INTEGER count;
  // TODO: thread-safe initializer
  if (g_first_time) {
    if (QueryPerformanceFrequency(&g_counts_per_sec) == 0) {
      g_counts_per_sec.QuadPart = 0;
    }
    g_first_time = 0;
  }
  if (g_counts_per_sec.QuadPart <= 0 || QueryPerformanceCounter(&count) == 0) {
    return -1;
  }
  ts->tv_sec = count.QuadPart / g_counts_per_sec.QuadPart;
  ts->tv_nsec = ((count.QuadPart % g_counts_per_sec.QuadPart) * 1E9) / g_counts_per_sec.QuadPart;
  return 0;
}

static int nanosleep_win32(struct timespec* ts, struct timespec* rem) {
  (void) rem;
  uint64_t ns = ptime_timespec_to_ns(ts);
  HANDLE timer;
  LARGE_INTEGER li;
  // negative value indicates relative time
  // rounds up to the next 100th nanosecond if needed
  li.QuadPart = -((ns / 100) + (ns % 100 > 0 ? 1 : 0));
  if ((timer = CreateWaitableTimer(NULL, TRUE, NULL)) == NULL) {
    return -1;
  }
  if (!SetWaitableTimer(timer, &li, 0, NULL, NULL, 0)) {
    CloseHandle(timer);
    return -1;
  }
  WaitForSingleObject(timer, INFINITE);
  CloseHandle(timer);
  return 0;
}
#endif // _WIN32

uint64_t ptime_timespec_to_ns(struct timespec* ts) {
  // must cast as a simple literal can overflow!
  return ts->tv_sec * (uint64_t) ONE_BILLION + ts->tv_nsec;
}

uint64_t ptime_timespec_to_us(struct timespec* ts) {
  // must cast as a simple literal can overflow!
  return ts->tv_sec * (uint64_t) ONE_MILLION + (ts->tv_nsec / (uint64_t) ONE_THOUSAND);
}

void ptime_ns_to_timespec(uint64_t ns, struct timespec* ts) {
  ts->tv_sec = ns / (time_t) ONE_BILLION;
  ts->tv_nsec = ns % (long) ONE_BILLION;
}

void ptime_us_to_timespec(uint64_t us, struct timespec* ts) {
  ts->tv_sec = us / (time_t) ONE_MILLION;
  ts->tv_nsec = (us % (long) ONE_MILLION) * (long) ONE_THOUSAND;
}

int ptime_clock_gettime(ptime_clock_id clk_id, struct timespec* ts) {
  switch(clk_id) {
#if defined(__MACH__)
    case PTIME_REALTIME:
      return clock_gettime_mach(CALENDAR_CLOCK, ts);
    case PTIME_MONOTONIC:
      return clock_gettime_mach(SYSTEM_CLOCK, ts);
#elif defined(_WIN32)
    case PTIME_REALTIME:
      return clock_gettime_realtime_win32(ts);
    case PTIME_MONOTONIC:
      return clock_gettime_monotonic_win32(ts);
#else
    case PTIME_REALTIME:
      return clock_gettime(CLOCK_REALTIME, ts);
    case PTIME_MONOTONIC:
      return clock_gettime(PTIME_CLOCKID_T_MONOTONIC, ts);
#endif
    default:
      break;
  }
  errno = EINVAL;
  return -1;
}

uint64_t ptime_gettime_ns(ptime_clock_id clk_id) {
  struct timespec ts;
  if (ptime_clock_gettime(clk_id, &ts)) {
    return 0;
  }
  return ptime_timespec_to_ns(&ts);
}

uint64_t ptime_gettime_us(ptime_clock_id clk_id) {
  return ptime_gettime_ns(clk_id) / (uint64_t) ONE_THOUSAND;
}

int64_t ptime_gettime_elapsed_ns(ptime_clock_id clk_id, struct timespec* ts) {
  struct timespec now;
  if (ptime_clock_gettime(clk_id, &now)) {
    return 0;
  }
  int64_t result = (now.tv_sec - ts->tv_sec) * (int64_t) ONE_BILLION + ((int64_t) now.tv_nsec - ts->tv_nsec);
  ts->tv_sec = now.tv_sec;
  ts->tv_nsec = now.tv_nsec;
  return result;
}

int64_t ptime_gettime_elapsed_us(ptime_clock_id clk_id, struct timespec* ts) {
  return ptime_gettime_elapsed_ns(clk_id, ts) / (int64_t) ONE_THOUSAND;
}

int ptime_nanosleep(struct timespec* ts, struct timespec* rem) {
#if defined(__MACH__)
  return nanosleep(ts, rem);
#elif defined(_WIN32)
  return nanosleep_win32(ts, rem);
#else
  errno = clock_nanosleep(PTIME_CLOCKID_T_MONOTONIC, 0, ts, rem);
  return errno ? -1 : 0;
#endif
}

int ptime_sleep_us(uint64_t us) {
  struct timespec ts;
  ptime_us_to_timespec(us, &ts);
  return ptime_nanosleep(&ts, NULL);
}

int ptime_sleep_us_no_interrupt(uint64_t us, volatile const int* ignore_interrupt) {
  int ret;
  struct timespec ts;
#if defined(__MACH__) || defined(_WIN32)
  struct timespec rem;
  ptime_us_to_timespec(us, &rem);
  do {
    ts.tv_sec = rem.tv_sec;
    ts.tv_nsec = rem.tv_nsec;
    // try to sleep for the requested period of time
    errno = 0;
    ret = ptime_nanosleep(&ts, &rem);
  } while (ret != 0 && errno == EINTR && (ignore_interrupt == NULL ? 1 : *ignore_interrupt));
#else
  // keep sleeping until a time in the future (now + requested time interval)
  if (clock_gettime(PTIME_CLOCKID_T_MONOTONIC, &ts)) {
    return -1;
  }
  ts.tv_sec += us / (time_t) ONE_MILLION;
  ts.tv_nsec += (us % (long) ONE_MILLION) * (long) ONE_THOUSAND;
  if (ts.tv_nsec >= (long) ONE_BILLION) {
    ts.tv_nsec -= (long) ONE_BILLION;
    ts.tv_sec++;
  }
  do {
    ret = clock_nanosleep(PTIME_CLOCKID_T_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
  }
  while (ret == EINTR && (ignore_interrupt == NULL ? 1 : *ignore_interrupt));
#endif
  return ret ? -1 : 0;
}
