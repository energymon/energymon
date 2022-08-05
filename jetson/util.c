#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "util.h"

int is_dir(const char* path) {
  DIR* dir = opendir(path);
  if (dir) {
    closedir(dir);
    return 1;
  }
  return errno == ENOENT ? 0 : -1;
}

int read_string(const char* file, char* str, size_t len) {
  int fd;
  ssize_t ret;
  size_t end;
  if ((fd = open(file, O_RDONLY)) < 0) {
    return -1;
  }
  if ((ret = read(fd, str, len)) > 0) {
    // strip trailing new line
    if ((end = strcspn(str, "\n")) < len) {
      str[end] = '\0';
    }
  }
  close(fd);
  return ret < 0 ? -1 : 0;
}

long read_long(const char* file) {
  char data[64];
  int fd;
  int ret;
  if ((fd = open(file, O_RDONLY)) < 0) {
    return -1;
  }
  if (read(fd, data, sizeof(data)) > 0) {
    errno = 0;
    ret = strtol(data, NULL, 0);
    if (!ret && errno) {
      ret = -1;
    }
  } else {
    ret = -1;
  }
  close(fd);
  return ret;
}

int is_i2c_bus_addr_dir(const struct dirent* entry) {
  // format: X-ABCDE
  return (entry->d_type == DT_LNK || entry->d_type == DT_DIR)
         && strlen(entry->d_name) > 2
         && entry->d_name[0] != '.'
         && entry->d_name[1] == '-';
}
