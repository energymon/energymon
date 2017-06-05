/**
 * Use the Linux /dev filesystem to communicate with a Watts Up? Power Meter.
 *
 * @author Connor Imes
 * @date 2016-02-08
 */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include "energymon-util.h"
#include "wattsup-driver.h"

struct energymon_wattsup_ctx {
  struct timeval timeout;
  int fd;
};

static int wattsup_open(const char* filename, int* fd) {
  assert(filename != NULL);
  assert(fd != NULL);

  struct stat s;
  char buf[32];
  const char* shortname;

  // Check if device node exists and is writable
  if (stat(filename, &s) < 0) {
    perror(filename);
    return -1;
  }
  if (!S_ISCHR(s.st_mode)) {
    errno = ENOTTY;
    perror("wattsup_open: Not a TTY character device");
    return -1;
  }
  if (access(filename, R_OK | W_OK)) {
    perror(filename);
    return -1;
  }

  // Get shortname by dropping leading "/dev/"
  if (!(shortname = strrchr(filename, '/'))) {
    // shouldn't happen since we've already checked filename
    errno = EINVAL;
    perror(filename);
    return -1;
  }
  shortname++;

  // Check if "/sys/class/tty/<shortname>" exists and is correct type
  snprintf(buf, sizeof(buf), "/sys/class/tty/%s", shortname);
  if (stat(buf, &s) < 0) {
    perror(buf);
    return -1;
  }
  if (!S_ISDIR(s.st_mode)) {
    errno = ENODEV;
    perror("wattsup_open: Not a TTY device");
    return -1;
  }

  // Open the device file
  *fd = open(filename, O_RDWR | O_NONBLOCK);
  if (*fd < 0) {
    perror(filename);
    return -1;
  }
  return 0;
}

static int wattsup_set_serial_attributes(int fd) {
  assert(fd > 0);
  struct termios t;
  // get attributes
  if (tcgetattr(fd, &t)) {
    return -1;
  }
  // set "raw" mode
  cfmakeraw(&t);
  // set input/output baud rate
  cfsetispeed(&t, B115200);
  cfsetospeed(&t, B115200);
  // flush any data received but not read
  tcflush(fd, TCIFLUSH);
  // ignore framing and parity errors (there is no parity bit)
  t.c_iflag |= IGNPAR;
  // Turn off double stop bits (documentation specifies only one is used)
  t.c_cflag &= ~CSTOPB;

  // set the parameters (immediately)
  return tcsetattr(fd, TCSANOW, &t);
}

energymon_wattsup_ctx* wattsup_connect(const char* dev_file, unsigned int timeout_ms) {
  assert(dev_file != NULL);
  int err_save;
  energymon_wattsup_ctx* ctx = malloc(sizeof(energymon_wattsup_ctx));
  if (ctx == NULL) {
    return NULL;
  }

  // set read timeout
  ctx->timeout.tv_sec = timeout_ms / 1000;
  ctx->timeout.tv_usec = timeout_ms % 1000;

  // open the file descriptor and check device properties
  if (wattsup_open(dev_file, &ctx->fd)) {
    perror(dev_file);
    free(ctx);
    return NULL;
  }

  // set serial port configuration
  if (wattsup_set_serial_attributes(ctx->fd)) {
    perror(dev_file);
    err_save = errno;
    close(ctx->fd);
    free(ctx);
    errno = err_save;
    return NULL;
  }

  return ctx;
}

int wattsup_disconnect(energymon_wattsup_ctx* ctx) {
  assert(ctx != NULL);
  int ret = ctx->fd > 0 ? close(ctx->fd) : 0;
  free(ctx);
  return ret;
}

int wattsup_read(energymon_wattsup_ctx* ctx, char* buf, size_t buflen) {
  assert(ctx != NULL);
  assert(ctx->fd > 0);
  assert(buf != NULL);
  assert(buflen > 0);

  int ret = -1;
  fd_set set;
  FD_ZERO(&set);
  FD_SET(ctx->fd, &set);

  switch (select(ctx->fd + 1, &set, NULL, NULL, &ctx->timeout)) {
    case -1:
      // failed
      break;
    case 0:
      // timed out
      errno = ETIME;
      break;
    default:
      ret = read(ctx->fd, buf, buflen);
      if (!ret) {
        // no data was read
        ret = -1;
        errno = ENODATA;
      }
      break;
  }
  return ret;
}

int wattsup_write(energymon_wattsup_ctx* ctx, const char* buf, size_t buflen) {
  assert(ctx != NULL);
  assert(ctx->fd > 0);
  assert(buf != NULL);
  assert(buflen > 0);
  return write(ctx->fd, buf, buflen);
}

char* wattsup_get_implementation(char* buf, size_t buflen) {
  return energymon_strencpy(buf, "WattsUp? Power Meter", buflen);
}
