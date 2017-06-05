/**
 * Use libftdi to communicate with a Watts Up? Power Meter.
 *
 * @author Connor Imes
 * @date 2017-01-27
 */
#include <assert.h>
#include <errno.h>
#include <ftdi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "energymon-util.h"
#include "wattsup-driver.h"

// undocumented environment variable to force setting serial attributes like baud rate
#define WATTSUP_LIBFTDI_SET_SERIAL_ATTRIBUTES "WATTSUP_LIBFTDI_SET_SERIAL_ATTRIBUTES"

#ifndef ENERGYMON_WATTSUP_BAUD_RATE
  #define ENERGYMON_WATTSUP_BAUD_RATE 115200
#endif

struct energymon_wattsup_ctx {
  struct ftdi_context* ctx;
};

static int ctx_destroy(energymon_wattsup_ctx* ctx, int opened) {
  assert(ctx != NULL);
  int ret = 0;
  if (ctx->ctx != NULL) {
    if (opened && (ret = ftdi_usb_close(ctx->ctx)) < 0) {
      fprintf(stderr, "ftdi_usb_close: %s\n", ftdi_get_error_string(ctx->ctx));
    }
    ftdi_free(ctx->ctx);
  }
  free(ctx);
  return ret;
}

static energymon_wattsup_ctx* init_failed(const char* err, energymon_wattsup_ctx* ctx, int opened) {
  assert(ctx != NULL);
  int err_save = errno;
  if (errno) {
    perror(err);
  }
  fprintf(stderr, "%s: %s\n", err, ftdi_get_error_string(ctx->ctx));
  ctx_destroy(ctx, opened);
  errno = err_save;
  return NULL;
}

energymon_wattsup_ctx* wattsup_connect(const char* dev_file, unsigned int timeout_ms) {
  (void) dev_file;
  energymon_wattsup_ctx* ctx = malloc(sizeof(energymon_wattsup_ctx));
  if (ctx == NULL) {
    return NULL;
  }

  // create a ftdi context
  if ((ctx->ctx = ftdi_new()) == NULL) {
    return init_failed("ftdi_new", ctx, 0);
  }

  // TODO: No API to set read/write timeouts?
  (void) timeout_ms;
  // set read/write timeout
  // ctx->ctx->usb_read_timeout = timeout_ms;
  // ctx->ctx->usb_write_timeout = timeout_ms;

  // find the device and open it
  if (ftdi_usb_open(ctx->ctx, ENERGYMON_WATTSUP_VENDOR_ID, ENERGYMON_WATTSUP_PRODUCT_ID) < 0) {
    return init_failed("ftdi_usb_open", ctx, 0);
  }

  // We don't appear to need to set the following, so we'll only do so if requested
  if (getenv(WATTSUP_LIBFTDI_SET_SERIAL_ATTRIBUTES) != NULL) {
    // configure baud rate
    if (ftdi_set_baudrate(ctx->ctx, ENERGYMON_WATTSUP_BAUD_RATE) < 0) {
      return init_failed("ftdi_set_baudrate", ctx, 1);
    }
    // ftdi example "serial_test" also does this for writing to this same vid/pid
    if (ftdi_set_line_property(ctx->ctx, BITS_8, STOP_BIT_1, NONE)) {
      return init_failed("ftdi_set_line_property", ctx, 1);
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
  assert(ctx->ctx != NULL);
  assert(buf != NULL);
  assert(buflen > 0);
  int rc = ftdi_read_data(ctx->ctx, (unsigned char*) buf, buflen);
  if (rc < 0) {
    fprintf(stderr, "ftdi_read_data: %s\n", ftdi_get_error_string(ctx->ctx));
  }
  return rc;
}

int wattsup_write(energymon_wattsup_ctx* ctx, const char* buf, size_t buflen) {
  assert(ctx != NULL);
  assert(ctx->ctx != NULL);
  assert(buf != NULL);
  assert(buflen > 0);
  assert(buflen <= WU_MAX_MESSAGE_SIZE);
  unsigned char ubuf[WU_MAX_MESSAGE_SIZE];
  memcpy(ubuf, buf, buflen);
  // The normal ftdi.h specifies the buf param as const, but it's really not under the hood when calling libusb.
  // Some systems deploy a header that removes the const qualifer, causing a warning with -Wdiscarded-qualifiers.
  // We'll have to copy off the buffer like in the libusb implementation.
  int rc = ftdi_write_data(ctx->ctx, ubuf, buflen);
  if (rc < 0) {
    fprintf(stderr, "ftdi_write_data: %s\n", ftdi_get_error_string(ctx->ctx));
  }
  return rc;
}

char* wattsup_get_implementation(char* buf, size_t buflen) {
  return energymon_strencpy(buf, "WattsUp? Power Meter over libftdi", buflen);
}
