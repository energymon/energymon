/**
 * Read energy from Intel RAPL via sysfs.
 *
 * @author Connor Imes
 * @date 2015-08-04
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "energymon.h"
#include "energymon-rapl.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_rapl(em);
}
#endif

#define RAPL_BASE_DIR "/sys/class/powercap"
#define RAPL_ENERGY_FILE "energy_uj"
#define RAPL_MAX_ENERGY_FILE "max_energy_range_uj"

// Add power polling support for zones that don't support energy readings?

typedef struct rapl_zone {
  int energy_supported;
  int energy_fd;
#ifdef ENERGYMON_RAPL_OVERFLOW
  unsigned long long max_energy_range_uj;
  unsigned long long energy_last;
  unsigned int energy_overflow_count;
#endif
} rapl_zone;

typedef struct energymon_rapl {
  int count;
  rapl_zone* zones;
} energymon_rapl;

static inline int rapl_is_energy_supported(unsigned int zone) {
  struct dirent* entry;
  char buf[64];
  snprintf(buf, sizeof(buf), RAPL_BASE_DIR"/intel-rapl:%u", zone);
  DIR* dir = opendir(buf);
  int ret = 0;
  if (dir != NULL) {
    entry = readdir(dir);
    while (entry != NULL) {
      if (!strncmp(entry->d_name, RAPL_ENERGY_FILE, strlen(RAPL_ENERGY_FILE))) {
        ret = 1;
        break;
      }
      entry = readdir(dir);
    }
    closedir(dir);
  }
  return ret;
}

/**
 * Count the number of zones - does not include subzones.
 */
static inline unsigned int rapl_get_count() {
  unsigned int count = 0;
  struct dirent* entry;
  DIR* dir = opendir(RAPL_BASE_DIR);
  char buf[24];

  if (dir != NULL) {
    snprintf(buf, sizeof(buf), "intel-rapl:%u", count);
    entry = readdir(dir);
    while (entry != NULL) {
      if (!strncmp(entry->d_name, buf, strlen(buf))) {
        count++;
        break;
      }
      entry = readdir(dir);
    }
    closedir(dir);
  }

  return count;
}

static inline int rapl_open_file(unsigned int zone,
                                 int subzone,
                                 const char* file) {
  char filename[96];
  if (subzone >= 0) {
    sprintf(filename, RAPL_BASE_DIR"/intel-rapl:%u/intel-rapl:%u:%d/%s",
            zone, zone, subzone, file);
  } else {
    sprintf(filename, "/sys/class/powercap/intel-rapl:%u/%s", zone, file);
  }
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "rapl_open_file:open: %s: %s\n", filename, strerror(errno));
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

#ifdef ENERGYMON_RAPL_OVERFLOW
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
#endif

static inline int rapl_cleanup(energymon_rapl* em) {
  if (em == NULL) {
    return -1;
  }

  int ret = 0;
  unsigned int i = 0;
  if (em->zones != NULL) {
    for (i = 0; i < em->count; i++) {
      if (em->zones[i].energy_supported && em->zones[i].energy_fd >= 0) {
        ret += close(em->zones[i].energy_fd);
      }
    }
  }
  free(em->zones);
  em->zones = NULL;
  em->count = 0;
  return ret;
}

static inline int rapl_zone_init(rapl_zone* z, unsigned int zone, int subzone) {
  z->energy_supported = rapl_is_energy_supported(zone);
  if (z->energy_supported) {
#ifdef ENERGYMON_RAPL_OVERFLOW
    z->max_energy_range_uj = rapl_read_max_energy(zone, subzone);
#endif
    z->energy_fd = rapl_open_file(zone, subzone, RAPL_ENERGY_FILE);
    if (z->energy_fd < 0) {
      return -1;
    }
  } else {
#ifdef ENERGYMON_RAPL_OVERFLOW
    z->max_energy_range_uj = 0;
#endif
    z->energy_fd = -1;
  }
  return 0;
}

static inline int rapl_init(energymon_rapl* em) {
  unsigned int i;

  em->zones = NULL;
  em->count = rapl_get_count();
  if (em->count == 0) {
    fprintf(stderr, "rapl_init: No RAPL zones found!\n");
    return -1;
  }

  em->zones = calloc(em->count, sizeof(rapl_zone));
  if (em->zones == NULL) {
    em->count = 0;
    return -1;
  }

  for (i = 0; i < em->count; i++) {
    if (rapl_zone_init(&em->zones[i], i, -1)) {
      rapl_cleanup(em);
      return -1;
    }
  }
  return 0;
}

int energymon_init_rapl(energymon* em) {
  int ret;
  if (em == NULL || em->state != NULL) {
    return -1;
  }
  em->state = malloc(sizeof(energymon_rapl));
  if (em->state == NULL) {
    return -1;
  };
  ret = rapl_init(em->state);
  if (ret) {
    energymon_finish_rapl(em);
  }
  return ret;
}

static inline long long rapl_zone_read(rapl_zone* z) {
  long long val = 0;
  if (z->energy_supported) {
    val = rapl_read_value(z->energy_fd);
    if (val < 0) {
      return -1;
    }
#ifdef ENERGYMON_RAPL_OVERFLOW
    // attempt to detect overflow of counter
    if (val < z->energy_last) {
      z->energy_overflow_count++;
    }
    z->energy_last = val;
    val += z->energy_overflow_count * z->max_energy_range_uj;
#endif
  }
  return val;
}

static inline unsigned long long rapl_read_total_energy_uj(energymon_rapl* em) {
  long long val;
  unsigned long long total = 0;
  unsigned int i;
  for (i = 0; i < em->count; i++) {
    val = rapl_zone_read(&em->zones[i]);
    if (val < 0) {
      return 0;
    }
    total += val;
  }
  return total;
}

unsigned long long energymon_read_total_rapl(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    return 0;
  }
  return rapl_read_total_energy_uj(em->state);
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

char* energymon_get_source_rapl(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "Intel RAPL", n);
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
