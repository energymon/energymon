/**
 * Read energy from an ODROID Smart Power USB device.
 * Uses the HID API.
 * The default implementation just fetches an energy reading when requested.
 * To enable polling of power readings instead, set:
 *   EM_ODROID_SMART_POWER_USE_POLLING
 *
 * @author Connor Imes
 * @date 2015-01-27
 */

#include "energymon.h"
#include "em-odroid-smart-power.h"
#include <hidapi/hidapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define OSP_MAX_STR 65
#define OSP_VENDOR_ID           0x04d8
#define OSP_PRODUCT_ID          0x003f
#define OSP_REQUEST_DATA        0x37
#define OSP_REQUEST_STARTSTOP   0x80
#define OSP_REQUEST_STATUS      0x81

// how long to sleep for (in microseconds) after certain operations
#ifndef EM_ODROID_SMART_POWER_SLEEP_TIME_US
  #define EM_ODROID_SMART_POWER_SLEEP_TIME_US 200000
#endif

// sensor polling interval in microseconds
#ifndef EM_ODROID_SMART_POWER_POLL_DELAY_US
  // default value determined experimentally
  #define EM_ODROID_SMART_POWER_POLL_DELAY_US 200000
#endif

#define UJOULES_PER_WATTHOUR     3600000000

typedef struct em_osp {
  hid_device* device;
  unsigned char buf[OSP_MAX_STR];

#ifdef EM_ODROID_SMART_POWER_USE_POLLING
  long long osp_total_energy;
  // thread variables
  pthread_t osp_polling_thread;
  int osp_do_polling;
#endif
} em_osp;

#ifdef EM_DEFAULT
int em_impl_get(em_impl* impl) {
  return em_impl_get_osp(impl);
}
#endif

static inline int em_osp_request_status(em_osp* em) {
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

static inline int em_osp_request_start_stop(em_osp* em, int started) {
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
  usleep(EM_ODROID_SMART_POWER_SLEEP_TIME_US);
  return 0;
}

static inline int em_osp_request_data(em_osp* em) {
  em->buf[0] = 0x00;
  memset((void*) &em->buf[2], 0x00, sizeof(em->buf) - 2);
  em->buf[1] = OSP_REQUEST_DATA;
  if (hid_write(em->device, em->buf, sizeof(em->buf)) == -1 ||
      hid_read(em->device, em->buf, sizeof(em->buf)) == -1) {
    fprintf(stderr, "Failed to request data from ODROID Smart Power\n");
    return -1;
  }
  return 0;
}

#ifdef EM_ODROID_SMART_POWER_USE_POLLING
/**
 * pthread function to poll the device at regular intervals
 */
static void* osp_poll_device(void* args) {
  double watts;
  em_osp* em = (em_osp*) args;
  while(em->osp_do_polling > 0 && em->device != NULL) {
    char w[8] = {'\0'};
    if(em_osp_request_data(em)) {
      fprintf(stderr, "osp_poll_device: Data request failed\n");
    } else if(em->buf[0] == OSP_REQUEST_DATA) {
      strncpy(w, (char*) &em->buf[17], 6);
      watts = atof(w);
      em->osp_total_energy += watts * EM_ODROID_SMART_POWER_POLL_DELAY_US;
    } else {
      fprintf(stderr, "osp_poll_device: Did not get data\n");
    }
    // sleep for the polling delay
    usleep(EM_ODROID_SMART_POWER_POLL_DELAY_US);
  }
  return (void*) NULL;
}
#endif

int em_init_osp(em_impl* impl) {
  if (impl == NULL || impl->state != NULL) {
    return -1;
  }

  int started;

  em_osp* em = malloc(sizeof(em_osp));
  if (em == NULL) {
    return -1;
  }
  impl->state = em;

  em->buf[0] = 0x00;
  memset((void*) &em->buf[2], 0x00, sizeof(em->buf) - 2);

  // initialize
  if(hid_init()) {
    fprintf(stderr, "Failed to initialize ODROID Smart Power\n");
    return -1;
  }

  // get the device
  em->device = hid_open(OSP_VENDOR_ID, OSP_PRODUCT_ID, NULL);
  if (em->device == NULL) {
    fprintf(stderr, "Failed to open ODROID Smart Power\n");
    return -1;
  }

  // set nonblocking
  hid_set_nonblocking(em->device, 1);

  // get the status
  if(em_osp_request_status(em)) {
    em_finish_osp(impl);
    return -1;
  }

  // TODO: This doesn't seem to be accurate
  started = (em->buf[1] == 0x01) ? 1 : 0;
  if (em_osp_request_start_stop(em, started)) {
    em_finish_osp(impl);
    return -1;
  }

  // do an initial couple of reads
  if (em_osp_request_data(em) || em_osp_request_data(em)) {
    fprintf(stderr, "Failed initial write/read of ODROID Smart Power\n");
    em_finish_osp(impl);
    return -1;
  }

#ifdef EM_ODROID_SMART_POWER_USE_POLLING
  // start device polling thread
  em->osp_total_energy = 0;
  em->osp_do_polling = 1;
  if (pthread_create(&em->osp_polling_thread, NULL, osp_poll_device, em)) {
    fprintf(stderr, "Failed to start ODROID Smart Power thread.\n");
    em_finish_osp(impl);
    return -1;
  }
#endif

  return 0;
}

long long em_read_total_osp(const em_impl* impl) {
  if (impl == NULL || impl->state == NULL) {
    return -1;
  }

  long long ujoules = -1;
  em_osp* em = (em_osp*) impl->state;

  if (em->device == NULL) {
    fprintf(stderr, "em_read_total_osp: Not initialized!\n");
    return -1;
  }

#ifdef EM_ODROID_SMART_POWER_USE_POLLING
  ujoules = em->osp_total_energy;
#else
  char wh[7] = {'\0'};

  if(em_osp_request_data(em)) {
    fprintf(stderr, "em_read_total_osp: Data request failed\n");
  } else if(em->buf[0] == OSP_REQUEST_DATA) {
    strncpy(wh, (char*) &em->buf[26], 5);
    ujoules = atof(wh) * UJOULES_PER_WATTHOUR;
    // printf("em_read_total_osp: %s Watt-Hours = %lld uJoules\n", wh, ujoules);
  } else {
    fprintf(stderr, "em_read_total_osp: Did not get data\n");
  }
#endif

  return ujoules;
}

int em_finish_osp(em_impl* impl) {
  if (impl == NULL || impl->state == NULL) {
    return -1;
  }

  em_osp* em = (em_osp*) impl->state;

  if (em->device == NULL) {
    // nothing to do
    return 0;
  }

#ifdef EM_ODROID_SMART_POWER_USE_POLLING
  // stop sensors polling thread and cleanup
  em->osp_do_polling = 0;
  if(pthread_join(em->osp_polling_thread, NULL)) {
    fprintf(stderr, "Error joining ODROID Smart Power polling thread.\n");
  }
#endif

  em->buf[0] = 0x00;
  memset((void*) &em->buf[2], 0x00, sizeof(em->buf) - 2);
#ifdef EM_ODROID_SMART_POWER_STOP_ON_FINISH
  buf[1] = OSP_REQUEST_STARTSTOP;
  if (hid_write(em->device, em->buf, sizeof(em->buf)) == -1) {
    fprintf(stderr, "em_finish_osp: Failed to request start/stop\n");
  }
  usleep(EM_ODROID_SMART_POWER_SLEEP_TIME_US);
#endif
  hid_close(em->device);
  em->device = NULL;
  if(hid_exit()) {
    fprintf(stderr, "em_finish_osp: Failed to exit\n");
  }
  free(impl->state);
  return 0;
}

char* em_get_source_osp(char* buffer) {
  if (buffer == NULL) {
    return NULL;
  }
#ifdef EM_ODROID_SMART_POWER_USE_POLLING
  return strcpy(buffer, "ODROID Smart Power with Polling");
#else
  return strcpy(buffer, "ODROID Smart Power");
#endif
}

unsigned long long em_get_interval_osp(const em_impl* em) {
  return EM_ODROID_SMART_POWER_POLL_DELAY_US;
}

int em_impl_get_osp(em_impl* impl) {
  if (impl == NULL) {
    return -1;
  }
  impl->finit = &em_init_osp;
  impl->fread = &em_read_total_osp;
  impl->ffinish = &em_finish_osp;
  impl->fsource = &em_get_source_osp;
  impl->finterval = &em_get_interval_osp;
  impl->state = NULL;
  return 0;
}
