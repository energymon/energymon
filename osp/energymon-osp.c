/**
 * Read energy from an ODROID Smart Power USB device.
 * Uses the HID API.
 * The default implementation just fetches an energy reading when requested.
 * To enable polling of power readings instead, set:
 *   ENERGYMON_OSP_USE_POLLING
 *
 * @author Connor Imes
 * @date 2015-01-27
 */

#include <hidapi.h>
#include <inttypes.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "energymon.h"
#ifdef ENERGYMON_OSP_USE_POLLING
#include "energymon-osp-polling.h"
#include "energymon-time-util.h"
#else
#include "energymon-osp.h"
#endif
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
#define OSP_REQUEST_STARTSTOP   0x80
#define OSP_REQUEST_STATUS      0x81

// how long to sleep for (in microseconds) after certain operations
#ifndef ENERGYMON_OSP_SLEEP_TIME_US
  #define ENERGYMON_OSP_SLEEP_TIME_US 200000
#endif

// sensor polling interval in microseconds
#ifndef ENERGYMON_OSP_POLL_DELAY_US
  // default value determined experimentally
  #define ENERGYMON_OSP_POLL_DELAY_US 200000
#endif

#define UJOULES_PER_WATTHOUR 3600000000.0

// number of retries when bad data is returned from the device
#ifndef ENERGYMON_OSP_RETRIES
  #define ENERGYMON_OSP_RETRIES 1
#endif

typedef struct energymon_osp {
  hid_device* device;
  unsigned char buf[OSP_BUF_SIZE];
#ifdef ENERGYMON_OSP_USE_POLLING
  uint64_t total_uj;
  pthread_t thread;
  int poll_sensors;
#endif
} energymon_osp;

static inline int em_osp_request_status(energymon_osp* em) {
  memset((void*) &em->buf, 0x00, sizeof(em->buf));
  em->buf[1] = OSP_REQUEST_STATUS;
  return hid_write(em->device, em->buf, sizeof(em->buf)) == -1 ||
         hid_read(em->device, em->buf, sizeof(em->buf)) == -1;
}

static inline int em_osp_request_startstop(energymon_osp* em) {
  memset((void*) &em->buf, 0x00, sizeof(em->buf));
  em->buf[1] = OSP_REQUEST_STARTSTOP;
  if (hid_write(em->device, em->buf, sizeof(em->buf)) == -1) {
    return -1;
  }
  return 0;
}

static inline int em_osp_request_data(energymon_osp* em) {
  memset((void*) &em->buf, 0x00, sizeof(em->buf));
  em->buf[1] = OSP_REQUEST_DATA;
  return hid_write(em->device, em->buf, sizeof(em->buf)) == -1 ||
         hid_read(em->device, em->buf, sizeof(em->buf)) == -1;
}

static inline int em_osp_request_data_retry(energymon_osp* em,
                                            unsigned int retries) {
  do {
    // always read twice - the first attempt often returns old data
    em_osp_request_data(em);
    errno = 0;
    if (em_osp_request_data(em)) {
      return -1; // a HID error, we won't try to recover
    }
    if (em->buf[0] == OSP_REQUEST_DATA) {
      return 0; // data was good
    }
  } while (retries--);
  return -1; // no more retries
}

static int energymon_finish_osp_local(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }

  int err_save = 0;
  energymon_osp* state = (energymon_osp*) em->state;

#ifdef ENERGYMON_OSP_USE_POLLING
  if (state->poll_sensors) {
    // stop sensors polling thread and cleanup
    state->poll_sensors = 0;
    err_save = pthread_join(state->thread, NULL);
  }
#endif
#ifdef ENERGYMON_OSP_STOP_ON_FINISH
  if (em_osp_request_startstop(state)) {
    perror("energymon_finish_osp_local: Failed to request start/stop");
    err_save = errno && !err_save ? errno : err_save;
  }
#endif
  errno = 0;
  hid_close(state->device);
  if (errno) {
    perror("energymon_finish_osp_local: hid_close");
    err_save = !err_save ? errno : err_save;
  }
  if (hid_exit()) {
    perror("energymon_finish_osp_local: hid_exit");
    err_save = errno && !err_save ? errno : err_save;
  }
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
  char w[8];
  int64_t exec_us = 0;
  int err_save;
  struct timespec ts;
  if (energymon_clock_gettime(&ts)) {
    // must be that CLOCK_MONOTONIC is not supported
    perror("osp_poll_device");
    return (void*) NULL;
  }
  energymon_sleep_us(ENERGYMON_OSP_POLL_DELAY_US);
  while (state->poll_sensors) {
    w[0] = '\0';
    errno = 0;
    if (em_osp_request_data_retry(state, ENERGYMON_OSP_RETRIES) == 0 &&
        state->buf[0] == OSP_REQUEST_DATA) {
      strncpy(w, (char*) &state->buf[17], 6);
      watts = strtod(w, NULL);
    } else if (!errno) {
      // hidapi not guaranteed to set errno, so we should set it to something
      errno = EIO;
    }
    err_save = errno;
    exec_us = energymon_gettime_us(&ts);
    errno = err_save;
    if (errno) {
      perror("osp_poll_device: Did not get data");
    } else {
      state->total_uj += watts * exec_us;
    }
    // sleep for the polling delay (minus most overhead)
    energymon_sleep_us(2 * ENERGYMON_OSP_POLL_DELAY_US - exec_us);
  }
  return (void*) NULL;
}
#endif

#ifdef ENERGYMON_OSP_USE_POLLING
int energymon_init_osp_polling(energymon* em) {
#else
int energymon_init_osp(energymon* em) {
#endif
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  // initialize
  if (hid_init()) {
    perror("energymon_init_osp: Failed to initialize ODROID Smart Power");
    return -1;
  }

  energymon_osp* state = malloc(sizeof(energymon_osp));
  if (state == NULL) {
    return -1;
  }

  // get the device
  state->device = hid_open(OSP_VENDOR_ID, OSP_PRODUCT_ID, NULL);
  if (state->device == NULL) {
    perror("energymon_init_osp: hid_open");
    if (hid_exit()) {
      perror("energymon_init_osp: hid_exit");
    }
    free(state);
    return -1;
  }

  em->state = state;

  // set nonblocking
  if (hid_set_nonblocking(state->device, 1)) {
    perror("energymon_init_osp: hid_set_nonblocking");
    energymon_finish_osp_local(em);
    return -1;
  }

  // get the status
  if (em_osp_request_status(state)) {
    perror("energymon_init_osp: Failed to request status");
    energymon_finish_osp_local(em);
    return -1;
  }

  // TODO: This doesn't seem to be accurate
  int started = (state->buf[1] == 0x01) ? 1 : 0;
  if (!started && em_osp_request_startstop(state)) {
    perror("energymon_init_osp: Failed to request start/stop");
    energymon_finish_osp_local(em);
    return -1;
  }
  if (em_osp_request_startstop(state)) {
    perror("energymon_init_osp: Failed to request start/stop");
    energymon_finish_osp_local(em);
    return -1;
  }

  // do an initial read
  if (em_osp_request_data_retry(state, ENERGYMON_OSP_RETRIES)) {
    perror("energymon_init_osp: Failed initial r/w of ODROID Smart Power");
    energymon_finish_osp_local(em);
    return -1;
  }

  // let meter reset
  usleep(ENERGYMON_OSP_SLEEP_TIME_US);

#ifdef ENERGYMON_OSP_USE_POLLING
  // start device polling thread
  state->total_uj = 0;
  state->poll_sensors = 1;
  errno = pthread_create(&state->thread, NULL, osp_poll_device, state);
  if (errno) {
    energymon_finish_osp_local(em);
    return -1;
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
  char wh[7] = {'\0'};
  if (em_osp_request_data_retry(state, ENERGYMON_OSP_RETRIES)) {
    perror("energymon_read_total_osp: Data request failed");
    return 0;
  }
  strncpy(wh, (char*) &state->buf[26], 5);
  return atof(wh) * UJOULES_PER_WATTHOUR;
#endif
}

#ifdef ENERGYMON_OSP_USE_POLLING
int energymon_finish_osp_polling(energymon* em) {
#else
int energymon_finish_osp(energymon* em) {
#endif
  return energymon_finish_osp_local(em);
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
  return ENERGYMON_OSP_POLL_DELAY_US;
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
#else
  em->finit = &energymon_init_osp;
  em->fread = &energymon_read_total_osp;
  em->ffinish = &energymon_finish_osp;
  em->fsource = &energymon_get_source_osp;
  em->finterval = &energymon_get_interval_osp;
#endif
  em->state = NULL;
  return 0;
}
