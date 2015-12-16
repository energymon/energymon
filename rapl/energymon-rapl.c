/**
 * Read energy from Intel RAPL via sysfs.
 *
 * @author Connor Imes
 * @date 2015-08-04
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
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
  int energy_fd;
#ifdef ENERGYMON_RAPL_OVERFLOW
  uint64_t max_energy_range_uj;
  uint64_t energy_last;
  unsigned int energy_overflow_count;
#endif
} rapl_zone;

typedef struct energymon_rapl {
  int count;
  rapl_zone zones[];
} energymon_rapl;

/**
 * Count the number of RAPL zones (does not include subzones).
 * Returns 0 on error (check errno) or if no RAPL zones found.
 */
static inline unsigned int rapl_zone_count() {
  unsigned int count = 0;
  int err_save;
  struct dirent* entry;
  char buf[24];
  DIR* dir = opendir(RAPL_BASE_DIR);
  for (errno = 0; dir != NULL && (entry = readdir(dir)) != NULL;) {
    snprintf(buf, sizeof(buf), "intel-rapl:%u", count);
    count += strncmp(entry->d_name, buf, strlen(buf)) ? 0 : 1;
  }
  err_save = errno; // from opendir or readdir
  if (closedir(dir)) {
    perror(RAPL_BASE_DIR);
  }
  errno = err_save;
  return errno ? 0 : count;
}

/**
 * Returns 0 on error (check errno), otherwise the max energy.
 */
static inline uint64_t rapl_read_max_energy(unsigned int zone) {
  uint64_t ret = 0;
  int err_save;
  char buf[96];
  char data[30];
  snprintf(buf, sizeof(buf), RAPL_BASE_DIR"/intel-rapl:%u/%s",
           zone, RAPL_MAX_ENERGY_FILE);
  int fd = open(buf, O_RDONLY);
  if (fd > 0) {
    if (pread(fd, data, sizeof(data), 0) > 0) {
      ret = strtoull(data, NULL, 0);
    }
    err_save = errno;
    if (close(fd)) {
      perror(buf);
    }
    errno = err_save;
  }
  if (errno) {
    perror(buf);
  }
  return ret;
}

static inline int rapl_cleanup(const energymon_rapl* state) {
  int err_save = 0;
  unsigned int i;
  for (i = 0; i < state->count; i++) {
    if (state->zones[i].energy_fd > 0 && close(state->zones[i].energy_fd)) {
      err_save = errno;
    }
  }
  errno = err_save;
  return errno ? -1 : 0;
}

static inline int rapl_init(energymon_rapl* state, unsigned int count) {
  unsigned int i;
  int err_save;
  char buf[96];
  state->count = count;
  for (i = 0; i < state->count; i++) {
    snprintf(buf, sizeof(buf), RAPL_BASE_DIR"/intel-rapl:%u/%s",
             i, RAPL_ENERGY_FILE);
    state->zones[i].energy_fd = open(buf, O_RDONLY);
    if (state->zones[i].energy_fd <= 0) {
      perror(buf);
      err_save = errno;
      rapl_cleanup(state);
      errno = err_save;
      return -1;
    }
#ifdef ENERGYMON_RAPL_OVERFLOW
    state->zones[i].max_energy_range_uj = rapl_read_max_energy(zone);
#endif
  }
  return 0;
}

int energymon_init_rapl(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  unsigned int count = rapl_zone_count();
  if (count == 0) {
    if (errno) {
      perror(RAPL_BASE_DIR);
    } else {
      fprintf(stderr, "energymon_init_rapl: No RAPL zones found!\n");
    }
    return -1;
  }

  size_t size = sizeof(energymon_rapl) + count * sizeof(rapl_zone);
  energymon_rapl* state = calloc(1, size);
  if (state == NULL) {
    return -1;
  }

  if (rapl_init(state, count)) {
    free(state);
    return -1;
  }

  em->state = state;
  return 0;
}

/**
 * Returns 0 on error (check errno), otherwise the zone's energy value.
 */
static inline uint64_t rapl_zone_read(const rapl_zone* z) {
  uint64_t val = 0;
  char buf[30];
  if (pread(z->energy_fd, buf, sizeof(buf), 0) > 0) {
    val = strtoull(buf, NULL, 0);
  }
  if (errno) { // from pread or strtoull
    return 0;
  }
#ifdef ENERGYMON_RAPL_OVERFLOW
  // attempt to detect overflow of counter
  if (val < z->energy_last) {
    z->energy_overflow_count++;
  }
  z->energy_last = val;
  val += z->energy_overflow_count * z->max_energy_range_uj;
#endif
  return val;
}

/**
 * Returns 0 on error (check errno), otherwise the total energy across zones.
 */
static inline uint64_t rapl_read_total_energy_uj(const energymon_rapl* em) {
  uint64_t val = 0;
  uint64_t total = 0;
  unsigned int i;
  for (i = 0; i < em->count; i++) {
    val = rapl_zone_read(&em->zones[i]);
    if (val == 0 && errno) {
      return 0;
    }
    total += val;
  }
  return total;
}

uint64_t energymon_read_total_rapl(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  return rapl_read_total_energy_uj(em->state);
}

int energymon_finish_rapl(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }
  int ret = rapl_cleanup((energymon_rapl*) em->state);
  free(em->state);
  em->state = NULL;
  return ret;
}

char* energymon_get_source_rapl(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "Intel RAPL", n);
}

uint64_t energymon_get_interval_rapl(const energymon* em) {
  if (em == NULL) {
    // we don't need to access em, but it's still a programming error
    errno = EINVAL;
    return 0;
  }
  return 1000;
}

int energymon_get_rapl(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
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
