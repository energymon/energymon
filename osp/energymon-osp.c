/**
 * Read energy from an ODROID Smart Power USB device.
 * Uses the HID API.
 * The default implementation just fetches an energy reading when requested.
 * To enable polling of power readings instead, define compile flag:
 *   ENERGYMON_OSP_USE_POLLING
 *
 * There is no (known) document that defines the data format / protocol.
 * Much of this is designed from reference implementations, trial and error,
 * and the device firmware source code, esp. main.c, timer.c, and LCD.c.
 *
 * The maximum Wh string length the firmware can write back is 10 digits.
 * Of 16 total chars, 6 appear reserved for the Watt field; no '\0' guarantee.
 * Given that it also writes a decimal point and three units of precision, it
 * would likely crash from a char buffer overflow well before we detect it.
 * It takes so long for the Wh counter to grow anyway, it could take years to
 * overflow, which is unlikely to ever happen and impossible to test/verify.
 * Additional note: there would be a loss of precision as decimal places are
 * dropped prior to us detecting overflow (assuming the device didn't crash).
 * Instead, we force an overflow at 10k Wh and restart the device's counter.
 *
 * @author Connor Imes
 * @date 2015-01-27
 * @see http://odroid.com/dokuwiki/doku.php?id=en:odroidsmartpower
 */

#include <errno.h>
#include <hidapi.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "energymon.h"
#ifdef ENERGYMON_OSP_USE_POLLING
#include "energymon-osp-polling.h"
#else
#include "energymon-osp.h"
#endif
#include "energymon-time-util.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
#ifdef ENERGYMON_OSP_USE_POLLING
  return energymon_get_osp_polling(em);
#else
  return energymon_get_osp(em);
#endif
}
#endif

#define OSP_BUF_SIZE            65
#define OSP_VENDOR_ID           0x04d8
#define OSP_PRODUCT_ID          0x003f
#define OSP_REQUEST_DATA        0x37
#define OSP_REQUEST_ONOFF       0x82
#define OSP_REQUEST_STARTSTOP   0x80
#define OSP_REQUEST_STATUS      0x81

#define OSP_STATUS_ON           0x01
#define OSP_STATUS_STARTED      0x01

// The display would overflow at 10k Wh, but stops incrementing at 8192.0 Wh.
// Firmware doesn't check for overflows, so we must manage counter safely.
// Force an overflow at 1k Wh (a round value with more than enough headroom).
#define OSP_WATTHOUR_MAX 1000.0

// Time wait between writing a command the reading the response
#define OSP_WRITE_READ_DELAY_US 10

// documentation says USB refresh rate is 10 Hz
#define OSP_USB_REFRESH_US 100000

// sensor polling interval in microseconds
#ifndef ENERGYMON_OSP_POLL_DELAY_US
  #define ENERGYMON_OSP_POLL_DELAY_US OSP_USB_REFRESH_US
#endif

#define UJOULES_PER_WATTHOUR 3600000000.0

// number of retries when bad data is returned from the device
#ifndef ENERGYMON_OSP_RETRIES
  #define ENERGYMON_OSP_RETRIES 1
#endif

// It takes roughly a dozen reads to actually get the status of the device
#define ENERGYMON_OSP_STATUS_RETRIES 100

// Environment variable to request turning on the device if it's off during init.
// Keeping this as an undocumented feature for now.
#define ENERGYMON_OSP_REQUEST_ON "ENERGYMON_OSP_REQUEST_ON"

// Environment variable to stop the device recording on finish.
// Keeping this as an undocumented feature for now.
#define ENERGYMON_OSP_STOP_ON_FINISH "ENERGYMON_OSP_STOP_ON_FINISH"

// Environment variable to not call HID API static lifecycle functions.
// Keeping this as an undocumented feature for now.
#define ENERGYMON_OSP_HID_SKIP_LIFECYCLE "ENERGYMON_OSP_HID_SKIP_LIFECYCLE"

typedef struct energymon_osp {
  hid_device* device;
  unsigned char buf[OSP_BUF_SIZE];
#ifdef ENERGYMON_OSP_USE_POLLING
  uint64_t total_uj;
  pthread_t thread;
  int poll;
#else
  uint64_t overflow_surplus;
  unsigned int n_overflow;
#endif
} energymon_osp;

static int em_osp_request_status(energymon_osp* em, volatile const int* ignore_interrupt) {
  int i;
  memset((void*) &em->buf, 0x00, sizeof(em->buf));
  em->buf[1] = OSP_REQUEST_STATUS;
  if (hid_write(em->device, em->buf, sizeof(em->buf)) == -1) {
    return -1;
  }
  for (i = 0; i < ENERGYMON_OSP_STATUS_RETRIES; i++) {
    if (energymon_sleep_us(OSP_WRITE_READ_DELAY_US, ignore_interrupt) ||
        hid_read(em->device, em->buf, sizeof(em->buf)) == -1) {
      return -1;
    }
    if (em->buf[0] == OSP_REQUEST_STATUS) {
      return 0;
    }
  }
  return -1;
}

static int em_osp_request_onoff(energymon_osp* em, volatile const int* ignore_interrupt) {
  memset((void*) &em->buf, 0x00, sizeof(em->buf));
  em->buf[1] = OSP_REQUEST_ONOFF;
  if (hid_write(em->device, em->buf, sizeof(em->buf)) == -1 ||
      energymon_sleep_us(OSP_WRITE_READ_DELAY_US, ignore_interrupt)) {
    return -1;
  }
  return 0;
}

static int em_osp_request_startstop(energymon_osp* em, volatile const int* ignore_interrupt) {
  memset((void*) &em->buf, 0x00, sizeof(em->buf));
  em->buf[1] = OSP_REQUEST_STARTSTOP;
  if (hid_write(em->device, em->buf, sizeof(em->buf)) == -1 ||
      energymon_sleep_us(OSP_WRITE_READ_DELAY_US, ignore_interrupt)) {
    return -1;
  }
  return 0;
}

static int em_osp_request_data(energymon_osp* em, volatile const int* ignore_interrupt) {
  memset((void*) &em->buf, 0x00, sizeof(em->buf));
  em->buf[1] = OSP_REQUEST_DATA;
  return hid_write(em->device, em->buf, sizeof(em->buf)) == -1 ||
         energymon_sleep_us(OSP_WRITE_READ_DELAY_US, ignore_interrupt) ||
         hid_read(em->device, em->buf, sizeof(em->buf)) == -1;
}

static int em_osp_request_data_retry(energymon_osp* em, unsigned int retries, volatile const int* ignore_interrupt) {
  do {
    errno = 0;
    if (em_osp_request_data(em, ignore_interrupt)) {
      // a HID error, we won't try to recover
      // hidapi not guaranteed to set errno, so we should set it to something
      if (!errno) {
        errno = EIO;
      }
      return -1;
    }
    if (em->buf[0] == OSP_REQUEST_DATA) {
      return 0; // data was good
    }
  } while (retries--);
  // no more retries
  errno = ENODATA;
  return -1;
}

static int em_osp_finish(energymon* em, int start_errno) {
  energymon_osp* state = (energymon_osp*) em->state;
  int err_save = start_errno;

#ifdef ENERGYMON_OSP_USE_POLLING
  if (state->poll) {
    // stop sensors polling thread and cleanup
    state->poll = 0;
#ifndef __ANDROID__
   pthread_cancel(state->thread);
#endif
    errno = pthread_join(state->thread, NULL);
    if (errno && !err_save) {
      err_save = errno;
    }
  }
#endif

  if (state->device != NULL) {
    // stop the device, if desired
    if (getenv(ENERGYMON_OSP_STOP_ON_FINISH) != NULL &&
        em_osp_request_startstop(state, NULL)) {
      perror("em_osp_finish: em_osp_request_startstop");
      if (!err_save) {
        err_save = errno;
      }
    }
    // close the HID device handle
    errno = 0;
    hid_close(state->device);
    if (errno) {
      perror("em_osp_finish: hid_close");
      if (!err_save) {
        err_save = errno;
      }
    }
  }

  // teardown HID API, unless told not to
  if (getenv(ENERGYMON_OSP_HID_SKIP_LIFECYCLE) == NULL && hid_exit()) {
    if (errno) {
      perror("em_osp_finish: hid_exit");
    } else {
      fprintf(stderr, "em_osp_finish: hid_exit: Unknown error\n");
      // at least set some errno
      errno = EIO;
    }
    if (!err_save) {
      err_save = errno;
    }
  }

  // final cleanup
  free(em->state);
  em->state = NULL;
  errno = err_save;
  return errno ? -1 : 0;
}

#ifdef ENERGYMON_OSP_USE_POLLING
/**
 * pthread function to poll the device at regular intervals
 */
static void* osp_poll_device(void* args) {
  energymon_osp* state = (energymon_osp*) args;
  double watts;
  uint64_t exec_us;
  uint64_t last_us;
#ifndef __ANDROID__
  int dummy_old_state;
#endif
  if (!(last_us = energymon_gettime_us())) {
    // must be that CLOCK_MONOTONIC is not supported
    perror("osp_poll_device: energymon_gettime_us");
    return (void*) NULL;
  }
  while (state->poll) {
#ifndef __ANDROID__
    // Deadlock can occur during disconnect if thread is canceled during I/O
    // Enable thread cancel while sleeping, disable during I/O
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &dummy_old_state);
#endif
    errno = 0;
    if (em_osp_request_data_retry(state, ENERGYMON_OSP_RETRIES, &state->poll) == 0) {
      // Watt value always starts at index 17
      state->buf[OSP_BUF_SIZE - 1] = '\0';
      errno = 0;
      watts = strtod((const char*) &state->buf[17], NULL);
      if (errno) {
        perror("osp_poll_device: strtod");
        watts = 0;
      }
    } else {
      perror("osp_poll_device: em_osp_request_data_retry");
      watts = 0;
    }
    exec_us = energymon_gettime_elapsed_us(&last_us);
    state->total_uj += watts * exec_us;
    // sleep for the polling delay
    if (state->poll) {
#ifndef __ANDROID__
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &dummy_old_state);
#endif
      energymon_sleep_us(ENERGYMON_OSP_POLL_DELAY_US, &state->poll);
    }
  }
  return (void*) NULL;
}
#endif

static int em_osp_init_fail(energymon* em, const char* src, int alt_errno) {
  if (errno) {
    perror(src);
  } else {
    fprintf(stderr, "%s: Unknown error\n", src);
    // at least set some errno
    errno = alt_errno;
  }
  if (em != NULL) {
    em_osp_finish(em, errno);
  }
  return -1;
}

#ifdef ENERGYMON_OSP_USE_POLLING
int energymon_init_osp_polling(energymon* em) {
#else
int energymon_init_osp(energymon* em) {
#endif
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }
  int is_started;
  int is_on;

  energymon_osp* state = calloc(1, sizeof(energymon_osp));
  if (state == NULL) {
    return -1;
  }

  // initialize HID API, unless told not to
  if (getenv(ENERGYMON_OSP_HID_SKIP_LIFECYCLE) == NULL && hid_init()) {
    free(state);
    return em_osp_init_fail(NULL, "energymon_init_osp: hid_init", EIO);
  }

  em->state = state;

  // get the HID device handle
  state->device = hid_open(OSP_VENDOR_ID, OSP_PRODUCT_ID, NULL);
  if (state->device == NULL) {
    return em_osp_init_fail(em, "energymon_init_osp: hid_open", ENODEV);
  }

  // set nonblocking
  if (hid_set_nonblocking(state->device, 1)) {
    return em_osp_init_fail(em, "energymon_init_osp: hid_set_nonblocking", EIO);
  }

  // get the status
  if (em_osp_request_status(state, NULL)) {
    return em_osp_init_fail(em, "energymon_init_osp: em_osp_request_status", EIO);
  }

  is_on = (state->buf[1] == OSP_STATUS_ON);
  is_started = (state->buf[1] == OSP_STATUS_STARTED);
#ifdef VERBOSE
  printf("ODROID Smart Power: already on: %s\n", is_on ? "yes" : "no");
  printf("ODROID Smart Power: already started: %s\n", is_started ? "yes" : "no");
#endif
  if (!is_on) {
    // currently hiding behind an environment variable so we don't risk turning off active devices if we're wrong
    if (getenv(ENERGYMON_OSP_REQUEST_ON) == NULL) {
      fprintf(stderr, "ODROID Smart Power appears to be off, continuing anyway...\n");
    } else {
      // turn on the device
      if (em_osp_request_onoff(state, NULL)) {
        return em_osp_init_fail(em, "energymon_init_osp: em_osp_request_onoff", EIO);
      }
      // get the status again
      if (em_osp_request_status(state, NULL)) {
        return em_osp_init_fail(em, "energymon_init_osp: em_osp_request_status (after ON)", EIO);
      }
      is_started = (state->buf[1] == OSP_STATUS_STARTED);
    }
  }
  if (!is_started) {
    // start the device
    if (em_osp_request_startstop(state, NULL)) {
      return em_osp_init_fail(em, "energymon_init_osp: em_osp_request_startstop", EIO);
    }
  }

#ifdef ENERGYMON_OSP_USE_POLLING
  // start device polling thread
  state->poll = 1;
  errno = pthread_create(&state->thread, NULL, osp_poll_device, state);
  if (errno) {
    return em_osp_init_fail(em, "energymon_init_osp: pthread_create", errno);
  }
#endif

  return 0;
}

#ifdef ENERGYMON_OSP_USE_POLLING
uint64_t energymon_read_total_osp_polling(const energymon* em) {
#else
uint64_t energymon_read_total_osp(const energymon* em) {
#endif
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  energymon_osp* state = (energymon_osp*) em->state;
#ifdef ENERGYMON_OSP_USE_POLLING
  return state->total_uj;
#else
  double wh;
  if (em_osp_request_data_retry(state, ENERGYMON_OSP_RETRIES, NULL)) {
    perror("energymon_read_total_osp: em_osp_request_data_retry");
    return 0;
  }
  // Wh value always starts at index 24
  state->buf[OSP_BUF_SIZE - 1] = '\0';
  errno = 0;
  wh = strtod((const char*) &state->buf[24], NULL);
  if (errno) {
    perror("energymon_read_total_osp: strtod");
    return 0;
  }
  if (wh >= OSP_WATTHOUR_MAX) {
#ifdef VERBOSE
    printf("ODROID Smart Power: detected overflow at %f Wh\n", wh);
#endif
    // force an overflow
    while (wh >= OSP_WATTHOUR_MAX) {
      wh -= OSP_WATTHOUR_MAX;
      state->n_overflow++;
    }
    // save diff between wh and OSP_WATTHOUR_MAX so it's not lost after reset
    state->overflow_surplus += wh;
    wh = 0;
    // restart device counter
    if (em_osp_request_startstop(state, NULL)) {
      perror("energymon_read_total_osp: em_osp_request_startstop: stop");
    }
    if (em_osp_request_startstop(state, NULL)) {
      perror("energymon_read_total_osp: em_osp_request_startstop: start");
    }
  }
  return UJOULES_PER_WATTHOUR *
         (wh + state->n_overflow * OSP_WATTHOUR_MAX + state->overflow_surplus);
#endif
}

#ifdef ENERGYMON_OSP_USE_POLLING
int energymon_finish_osp_polling(energymon* em) {
#else
int energymon_finish_osp(energymon* em) {
#endif
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }
  return em_osp_finish(em, 0);
}

#ifdef ENERGYMON_OSP_USE_POLLING
char* energymon_get_source_osp_polling(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "ODROID Smart Power with Polling", n);
#else
char* energymon_get_source_osp(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "ODROID Smart Power", n);
#endif
}

#ifdef ENERGYMON_OSP_USE_POLLING
uint64_t energymon_get_interval_osp_polling(const energymon* em) {
#else
uint64_t energymon_get_interval_osp(const energymon* em) {
#endif
  if (em == NULL) {
    // we don't need to access em, but it's still a programming error
    errno = EINVAL;
    return 0;
  }
#ifdef ENERGYMON_OSP_USE_POLLING
  return ENERGYMON_OSP_POLL_DELAY_US;
#else
  return OSP_USB_REFRESH_US;
#endif
}

#ifdef ENERGYMON_OSP_USE_POLLING
uint64_t energymon_get_precision_osp_polling(const energymon* em) {
#else
uint64_t energymon_get_precision_osp(const energymon* em) {
#endif
  if (em == NULL) {
    errno = EINVAL;
    return 0;
  }
#ifdef ENERGYMON_OSP_USE_POLLING
  // watts to 3 decimal places (milliwatts) at refresh interval
  return ENERGYMON_OSP_POLL_DELAY_US / 1000;
#else
  // watt-hours to 3 decimal places (milliwatt-hours)
  return UJOULES_PER_WATTHOUR / 1000;
#endif
}

#ifdef ENERGYMON_OSP_USE_POLLING
int energymon_is_exclusive_osp_polling(void) {
#else
int energymon_is_exclusive_osp(void) {
#endif
  return 1;
}

#ifdef ENERGYMON_OSP_USE_POLLING
int energymon_get_osp_polling(energymon* em) {
#else
int energymon_get_osp(energymon* em) {
#endif
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
#ifdef ENERGYMON_OSP_USE_POLLING
  em->finit = &energymon_init_osp_polling;
  em->fread = &energymon_read_total_osp_polling;
  em->ffinish = &energymon_finish_osp_polling;
  em->fsource = &energymon_get_source_osp_polling;
  em->finterval = &energymon_get_interval_osp_polling;
  em->fprecision = &energymon_get_precision_osp_polling;
  em->fexclusive = &energymon_is_exclusive_osp_polling;
#else
  em->finit = &energymon_init_osp;
  em->fread = &energymon_read_total_osp;
  em->ffinish = &energymon_finish_osp;
  em->fsource = &energymon_get_source_osp;
  em->finterval = &energymon_get_interval_osp;
  em->fprecision = &energymon_get_precision_osp;
  em->fexclusive = &energymon_is_exclusive_osp;
#endif
  em->state = NULL;
  return 0;
}
