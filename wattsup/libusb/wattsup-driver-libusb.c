/**
 * Use libusb to communicate with a Watts Up? Power Meter.
 *
 * @author Connor Imes
 * @date 2017-01-27
 */
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "energymon-util.h"
#include "wattsup-driver.h"

#ifndef ENERGYMON_WATTSUP_INTERFACE_NUMBER
  #define ENERGYMON_WATTSUP_INTERFACE_NUMBER 0
#endif

#ifndef ENERGYMON_WATTSUP_ENDPOINT_WRITE
  #define ENERGYMON_WATTSUP_ENDPOINT_WRITE 0x02
#endif

#ifndef ENERGYMON_WATTSUP_ENDPOINT_READ
  #define ENERGYMON_WATTSUP_ENDPOINT_READ 0x81
#endif

// undocumented environment variable to force setting serial attributes like baud rate
#define WATTSUP_LIBUSB_SET_SERIAL_ATTRIBUTES "WATTSUP_LIBUSB_SET_SERIAL_ATTRIBUTES"

struct energymon_wattsup_ctx {
  libusb_context* ctx;
  libusb_device_handle* handle;
  int reattach_kernel;
  unsigned int timeout_ms;
  int interface;
  unsigned char endpoint_r;
  unsigned char endpoint_w;
};

static int ctx_destroy(energymon_wattsup_ctx* ctx, int claimed) {
  assert(ctx != NULL);
  int rc;
  int ret = 0;
  if (ctx->handle != NULL) {
    // release our claim
    if (claimed && (rc = libusb_release_interface(ctx->handle, ctx->interface)) < 0) {
      fprintf(stderr, "libusb_release_interface: %s\n", libusb_error_name(rc));
      ret |= rc;
    }
    // reattach kernel driver if we detached it
    if (ctx->reattach_kernel && (rc = libusb_attach_kernel_driver(ctx->handle, ctx->interface)) < 0) {
      fprintf(stderr, "libusb_attach_kernel_driver: %s\n", libusb_error_name(rc));
      ret |= rc;
    }
    // close the device handle
    libusb_close(ctx->handle);
  }
  if (ctx->ctx != NULL) {
    // destroy the libusb context
    libusb_exit(ctx->ctx);
  }
  free(ctx);
  return ret;
}

static energymon_wattsup_ctx* init_failed(const char* err, int code, energymon_wattsup_ctx* ctx, int claimed) {
  assert(ctx != NULL);
  int err_save = errno;
  if (code == 0) {
    perror(err);
  } else {
    fprintf(stderr, "%s: %s\n", err, libusb_error_name(code));
  }
  ctx_destroy(ctx, claimed);
  errno = err_save;
  return NULL;
}

energymon_wattsup_ctx* wattsup_connect(const char* dev_file, unsigned int timeout_ms) {
  (void) dev_file;
  energymon_wattsup_ctx* ctx = calloc(1, sizeof(energymon_wattsup_ctx));
  if (ctx == NULL) {
    return NULL;
  }
  int rc;

  // set read/write timeout
  ctx->timeout_ms = timeout_ms;
  // set the interface number and read/write endpoints
  ctx->interface = ENERGYMON_WATTSUP_INTERFACE_NUMBER;
  ctx->endpoint_r = ENERGYMON_WATTSUP_ENDPOINT_READ;
  ctx->endpoint_w = ENERGYMON_WATTSUP_ENDPOINT_WRITE;

  // initialize libusb
  if ((rc = libusb_init(&ctx->ctx)) < 0) {
    return init_failed("libusb_init", rc, ctx, 0);
  }

  /*
   * TODO: docs say "libusb_open_device_with_vid_pid" is a convenience function that shouldn't be used in real apps.
   * It returns the first vid/pid match found, but there could be more than one connected to the system.
   * Instead we should use "libusb_get_device_list" and search for our device there.
   * However, I don't know what we can get from that list other than vid/pid to better identify a WattsUp.
   */
  // find the device and open a handle
  if ((ctx->handle = libusb_open_device_with_vid_pid(ctx->ctx, ENERGYMON_WATTSUP_VENDOR_ID, ENERGYMON_WATTSUP_PRODUCT_ID)) == NULL) {
    return init_failed("libusb_open_device_with_vid_pid", 0, ctx, 0);
  }

  // check if the device is currently in the /dev filesystem (Linux usually mounts it there by default)
  if (libusb_kernel_driver_active(ctx->handle, ctx->interface) == 1) {
    // we must detach it from there so we can use it
    if ((rc = libusb_detach_kernel_driver(ctx->handle, ctx->interface)) != 0) {
      return init_failed("libusb_detach_kernel_driver", rc, ctx, 0);
    }
    // remember to reattach it when we disconnect
    ctx->reattach_kernel = 1;
  }

  // must claim the interface to use it
  if ((rc = libusb_claim_interface(ctx->handle, ctx->interface)) < 0) {
    return init_failed("libusb_claim_interface", rc, ctx, 0);
  }

  // TODO: stil getting some junk characters during reading - see ftdi_read_data(), packet size = 64

  // We don't appear to need to set the following, so we'll only do so if requested
  if (getenv(WATTSUP_LIBUSB_SET_SERIAL_ATTRIBUTES) != NULL) {
    // reverse-engineered these calls and parameters from libftdi
    // reset the device
    static const uint8_t RESET_REQUEST = 0;
    static const uint16_t RESET_VALUE = 0;
    static const uint16_t RESET_INDEX = 0;
    if (libusb_control_transfer(ctx->handle, (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT), RESET_REQUEST,
                                RESET_VALUE, RESET_INDEX,
                                NULL, 0,
                                ctx->timeout_ms) < 0) {
      return init_failed("libusb_control_transfer: reset", rc, ctx, 1);
    }
    // set baud rate
    static const uint8_t SET_BAUD_RATE_REQUEST = 3;
    static const uint16_t BAUD_RATE_CONVERTED = 26;
    static const int BAUD_RATE_INDEX = 0;
    if (libusb_control_transfer(ctx->handle,
                                (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT), SET_BAUD_RATE_REQUEST,
                                BAUD_RATE_CONVERTED, BAUD_RATE_INDEX,
                                NULL, 0,
                                ctx->timeout_ms) < 0) {
      return init_failed("libusb_control_transfer: baud", rc, ctx, 1);
    }
    static const uint8_t SET_DATA_REQUEST = 4;
    static const uint16_t LINE_PROPERTIES_VALUE = 8;
    static const uint16_t LINE_PROPERTIES_INDEX = 8;
    // configure serial line properties
    if (libusb_control_transfer(ctx->handle,
                                (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT), SET_DATA_REQUEST,
                                LINE_PROPERTIES_VALUE, LINE_PROPERTIES_INDEX,
                                NULL, 0,
                                ctx->timeout_ms) < 0) {
      return init_failed("libusb_control_transfer: line properties", rc, ctx, 1);
    }
  }

  return ctx;
}

int wattsup_disconnect(energymon_wattsup_ctx* ctx) {
  assert(ctx != NULL);
  return ctx_destroy(ctx, 1);
}

int wattsup_read(energymon_wattsup_ctx* ctx, char* buf, size_t buflen) {
  assert(ctx != NULL);
  assert(ctx->handle != NULL);
  assert(buf != NULL);
  assert(buflen > 0);
  int actual = 0;
  int rc = libusb_bulk_transfer(ctx->handle, (ctx->endpoint_r | LIBUSB_ENDPOINT_IN), (unsigned char*) buf, buflen, &actual, ctx->timeout_ms);
  if (rc < 0) {
    fprintf(stderr, "wattsup_read:libusb_bulk_transfer: %s\n", libusb_error_name(rc));
  }
  return actual;
}

int wattsup_write(energymon_wattsup_ctx* ctx, const char* buf, size_t buflen) {
  assert(ctx != NULL);
  assert(ctx->handle != NULL);
  assert(buf != NULL);
  assert(buflen > 0);
  assert(buflen <= WU_MAX_MESSAGE_SIZE);
  unsigned char ubuf[WU_MAX_MESSAGE_SIZE];
  int actual = 0;
  memcpy(ubuf, buf, buflen);
  int rc = libusb_bulk_transfer(ctx->handle, (ctx->endpoint_w | LIBUSB_ENDPOINT_OUT), ubuf, buflen, &actual, ctx->timeout_ms);
  if (rc < 0) {
    fprintf(stderr, "wattsup_write:libusb_bulk_transfer: %s\n", libusb_error_name(rc));
  }
  return actual;
}

char* wattsup_get_implementation(char* buf, size_t buflen) {
  return energymon_strencpy(buf, "WattsUp? Power Meter over libusb-1.0", buflen);
}
