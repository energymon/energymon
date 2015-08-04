/**
 * Read energy from Intel RAPL via sysfs.
 *
 * @author Connor Imes
 * @date 2015-08-04
 */

#include "energymon.h"
#include "energymon-rapl.h"
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define RAPL_ENERGY_FILE "energy_uj"
#define RAPL_MAX_ENERGY_FILE "max_energy_range_uj"

typedef struct energymon_rapl {
  int fd;
  unsigned long long max_energy_range_uj;
} energymon_rapl;

#ifdef ENERGYMON_DEFAULT
int energymon_get_default(energymon* em) {
  return energymon_get_rapl(em);
}
#endif

static inline int rapl_open_file(unsigned int zone,
                                 int subzone,
                                 const char* file) {
  char filename[BUFSIZ];
  int fd;
  if (subzone >= 0) {
    sprintf(filename, "/sys/class/powercap/intel-rapl:%u/intel-rapl:%u:%d/%s",
            zone, zone, subzone, file);
  } else {
    sprintf(filename, "/sys/class/powercap/intel-rapl:%u/%s", zone, file);
  }
  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    perror("Opening RAPL");
    fprintf(stderr, "Trying to open %s\n", filename);
  }
  return fd;
}

static inline long long rapl_read_value(int fd) {
  unsigned long long val;
  char buf[30];
  if (pread(fd, buf, sizeof(buf), 0) <= 0) {
    perror("rapl_read_value");
    return -1;
  }
  val = strtoull(buf, NULL, 0);
  return val;
}

static inline unsigned long long rapl_read_max_energy(unsigned int zone,
                                                      int subzone) {
  int fd = rapl_open_file(zone, subzone, RAPL_MAX_ENERGY_FILE);
  if (fd < 0) {
    return 0;
  }
  long long ret = rapl_read_value(fd);
  close(fd);
  if (ret < 0) {
    return 0;
  }
  return ret;
}

static inline int rapl_init(energymon_rapl* em) {
  em->fd = rapl_open_file(0, -1, RAPL_ENERGY_FILE);
  em->max_energy_range_uj = rapl_read_max_energy(0, -1);
  if (em->fd < 0) {
    return -1;
  }
  return 0;
}

int energymon_init_rapl(energymon* em) {
  if (em == NULL || em->state != NULL) {
    return -1;
  }
  em->state = malloc(sizeof(energymon_rapl));
  if (em->state == NULL) {
    return -1;
  };
  return rapl_init(em->state);
}

static inline unsigned long long rapl_read_total_energy_uj(energymon_rapl* em) {
  long long val = rapl_read_value(em->fd);
  if (val < 0) {
    return 0;
  }
  return val;
}

unsigned long long energymon_read_total_rapl(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    return 0;
  }
  return rapl_read_total_energy_uj(em->state);
}

static inline int rapl_cleanup(energymon_rapl* em) {
  return close(em->fd);
}

int energymon_finish_rapl(energymon* em) {
  if (em == NULL || em->state == NULL) {
    return -1;
  }
  int ret = rapl_cleanup(em->state);
  free(em->state);
  em->state = NULL;
  return ret;
}

char* energymon_get_source_rapl(char* buffer) {
  if (buffer == NULL) {
    return NULL;
  }
  return strcpy(buffer, "Intel RAPL");
}

unsigned long long energymon_get_interval_rapl(const energymon* em) {
  return 1000;
}

int energymon_get_rapl(energymon* em) {
  if (em == NULL) {
    return -1;
  }
  em->finit = &energymon_init_rapl;
  em->fread = &energymon_read_total_rapl;
  em->ffinish = &energymon_finish_rapl;
  em->fsource = &energymon_get_source_rapl;
  em->finterval = &energymon_get_interval_rapl;
  em->state = NULL;
  return 0;
}
