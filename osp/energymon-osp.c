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

#include <hidapi/hidapi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "energymon.h"
#include "energymon-osp.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* impl) {
  return energymon_get_osp(impl);
}
#endif

#define OSP_MAX_STR 65
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

#define UJOULES_PER_WATTHOUR     3600000000.0

typedef struct energymon_osp {
  hid_device* device;
  unsigned char buf[OSP_MAX_STR];

#ifdef ENERGYMON_OSP_USE_POLLING
  unsigned long long osp_total_energy;
  // thread variables
  pthread_t osp_polling_thread;
  int osp_do_polling;
#endif
} energymon_osp;

static inline int em_osp_request_status(energymon_osp* em) {
  em->buf[0] = 0x00;
  memset((void*) &em->buf[2], 0x00, sizeof(em->buf) - 2);
  em->buf[1] = OSP_REQUEST_STATUS;
  if (hid_write(em->device, em->buf, sizeof(em->buf)) == -1 ||
      hid_read(em->device, em->buf, sizeof(em->buf)) == -1) {
    fprintf(stderr,
            "em_osp_request_status: Failed to request/read status\n");
    return -1;
  }
  return 0;
}

static inline int em_osp_request_start_stop(energymon_osp* em, int started) {
  if(started == 0) {
    em->buf[1] = OSP_REQUEST_STARTSTOP;
    if (hid_write(em->device, em->buf, sizeof(em->buf)) == -1) {
      fprintf(stderr, "em_osp_request_start_stop: Request failed\n");
      return -1;
    }
  }

  em->buf[1] = OSP_REQUEST_STARTSTOP;
  if (hid_write(em->device, em->buf, sizeof(em->buf)) == -1) {
    fprintf(stderr, "em_osp_request_start_stop: Request failed\n");
    return -1;
  }

  // let meter reset
  usleep(ENERGYMON_OSP_SLEEP_TIME_US);
  return 0;
}

static inline int em_osp_request_data(energymon_osp* em) {
  em->buf[0] = 0x00;
  memset((void*) &em->buf[2], 0x00, sizeof(em->buf) - 2);
  em->buf[1] = OSP_REQUEST_DATA;
  if (hid_write(em->device, em->buf, sizeof(em->buf)) == -1 ||
      hid_read(em->device, em->buf, sizeof(em->buf)) == -1) {
    fprintf(stderr,"em_osp_request_data: Failed to request data from ODROID "
            "Smart Power\n");
    return -1;
  }
  return 0;
}

#ifdef ENERGYMON_OSP_USE_POLLING
/**
 * pthread function to poll the device at regular intervals
 */
static void* osp_poll_device(void* args) {
  double watts;
  energymon_osp* em = (energymon_osp*) args;
  while(em->osp_do_polling > 0 && em->device != NULL) {
    char w[8] = {'\0'};
    if(em_osp_request_data(em)) {
      fprintf(stderr, "osp_poll_device: Data request failed\n");
    } else if(em->buf[0] == OSP_REQUEST_DATA) {
      strncpy(w, (char*) &em->buf[17], 6);
      watts = atof(w);
      em->osp_total_energy += watts * ENERGYMON_OSP_POLL_DELAY_US;
    } else {
      fprintf(stderr, "osp_poll_device: Did not get data\n");
    }
    // sleep for the polling delay
    usleep(ENERGYMON_OSP_POLL_DELAY_US);
  }
  return (void*) NULL;
}
#endif

int energymon_init_osp(energymon* impl) {
  if (impl == NULL || impl->state != NULL) {
    return -1;
  }

  int started;

  energymon_osp* em = malloc(sizeof(energymon_osp));
  if (em == NULL) {
    return -1;
  }
  impl->state = em;

  em->buf[0] = 0x00;
  memset((void*) &em->buf[2], 0x00, sizeof(em->buf) - 2);

  // initialize
  if(hid_init()) {
    fprintf(stderr,
            "energymon_init_osp: Failed to initialize ODROID Smart Power\n");
    return -1;
  }

  // get the device
  em->device = hid_open(OSP_VENDOR_ID, OSP_PRODUCT_ID, NULL);
  if (em->device == NULL) {
    fprintf(stderr, "energymon_init_osp: Failed to open ODROID Smart Power\n");
    return -1;
  }

  // set nonblocking
  hid_set_nonblocking(em->device, 1);

  // get the status
  if(em_osp_request_status(em)) {
    energymon_finish_osp(impl);
    return -1;
  }

  // TODO: This doesn't seem to be accurate
  started = (em->buf[1] == 0x01) ? 1 : 0;
  if (em_osp_request_start_stop(em, started)) {
    energymon_finish_osp(impl);
    return -1;
  }

  // do an initial couple of reads
  if (em_osp_request_data(em) || em_osp_request_data(em)) {
    fprintf(stderr, "energymon_init_osp: Failed initial write/read of ODROID "
            "Smart Power\n");
    energymon_finish_osp(impl);
    return -1;
  }

#ifdef ENERGYMON_OSP_USE_POLLING
  // start device polling thread
  em->osp_total_energy = 0;
  em->osp_do_polling = 1;
  if (pthread_create(&em->osp_polling_thread, NULL, osp_poll_device, em)) {
    fprintf(stderr,
            "energymon_init_osp: Failed to start ODROID Smart Power thread\n");
    energymon_finish_osp(impl);
    return -1;
  }
#endif

  return 0;
}

unsigned long long energymon_read_total_osp(const energymon* impl) {
  if (impl == NULL || impl->state == NULL) {
    return 0;
  }

  unsigned long long ujoules = 0;
  energymon_osp* em = (energymon_osp*) impl->state;

  if (em->device == NULL) {
    fprintf(stderr, "energymon_read_total_osp: Not initialized!\n");
    return 0;
  }

#ifdef ENERGYMON_OSP_USE_POLLING
  ujoules = em->osp_total_energy;
#else
  char wh[7] = {'\0'};

  if(em_osp_request_data(em)) {
    fprintf(stderr, "energymon_read_total_osp: Data request failed\n");
  } else if(em->buf[0] == OSP_REQUEST_DATA) {
    strncpy(wh, (char*) &em->buf[26], 5);
    ujoules = atof(wh) * UJOULES_PER_WATTHOUR;
  } else {
    fprintf(stderr, "energymon_read_total_osp: Did not get data\n");
  }
#endif

  return ujoules;
}

int energymon_finish_osp(energymon* impl) {
  if (impl == NULL || impl->state == NULL) {
    return -1;
  }

  energymon_osp* em = (energymon_osp*) impl->state;

  if (em->device == NULL) {
    // nothing to do
    return 0;
  }

#ifdef ENERGYMON_OSP_USE_POLLING
  // stop sensors polling thread and cleanup
  em->osp_do_polling = 0;
  if(pthread_join(em->osp_polling_thread, NULL)) {
    fprintf(stderr, "energymon_finish_osp: Error joining ODROID Smart Power "
            "polling thread\n");
  }
#endif

  em->buf[0] = 0x00;
  memset((void*) &em->buf[2], 0x00, sizeof(em->buf) - 2);
#ifdef ENERGYMON_OSP_STOP_ON_FINISH
  em->buf[1] = OSP_REQUEST_STARTSTOP;
  if (hid_write(em->device, em->buf, sizeof(em->buf)) == -1) {
    fprintf(stderr, "energymon_finish_osp: Failed to request start/stop\n");
  }
  usleep(ENERGYMON_OSP_SLEEP_TIME_US);
#endif
  hid_close(em->device);
  em->device = NULL;
  if(hid_exit()) {
    fprintf(stderr, "energymon_finish_osp: Failed to exit\n");
  }
  free(impl->state);
  return 0;
}

char* energymon_get_source_osp(char* buffer, size_t n) {
#ifdef ENERGYMON_OSP_USE_POLLING
  return energymon_strencpy(buffer, "ODROID Smart Power with Polling", n);
#else
  return energymon_strencpy(buffer, "ODROID Smart Power", n);
#endif
}

unsigned long long energymon_get_interval_osp(const energymon* em) {
  return ENERGYMON_OSP_POLL_DELAY_US;
}

int energymon_get_osp(energymon* impl) {
  if (impl == NULL) {
    return -1;
  }
  impl->finit = &energymon_init_osp;
  impl->fread = &energymon_read_total_osp;
  impl->ffinish = &energymon_finish_osp;
  impl->fsource = &energymon_get_source_osp;
  impl->finterval = &energymon_get_interval_osp;
  impl->state = NULL;
  return 0;
}
