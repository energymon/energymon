/**
 * Utility to attach a WattsUp device to the kernel if it's currently detached.
 *
 * @author Connor Imes
 * @date 2017-01-28
 */
#include <errno.h>
#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>

// vendor and product IDs
#define VID 0x0403
#define PID 0x6001

#define INTERFACE_NUMBER 0

int main(void) {
  libusb_context* ctx;
  libusb_device_handle* handle;
  int rc;

  if ((rc = libusb_init(&ctx)) < 0) {
    fprintf(stderr, "libusb_init: %s\n", libusb_error_name(rc));
    return rc;
  }

  if ((handle = libusb_open_device_with_vid_pid(ctx, VID, PID)) == NULL) {
    perror("libusb_open_device_with_vid_pid");
    return -errno;
  }

  if (libusb_kernel_driver_active(handle, INTERFACE_NUMBER) == 0) {
    if ((rc = libusb_attach_kernel_driver(handle, INTERFACE_NUMBER)) < 0) {
      fprintf(stderr, "libusb_attach_kernel_driver: %s\n", libusb_error_name(rc));
      return rc;
    }
  }

  libusb_close(handle);
  libusb_exit(ctx);

  return 0;
}
