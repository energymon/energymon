/**
 * Read energy from X86 MSRs (Model-Specific Registers).
 *
 * By default, the MSR on cpu0 is read. To configure other MSRs, set the
 * ENERGYMON_MSRS environment variable with a comma-delimited list of CPU IDs,
 * e.g.:
 *   export ENERGYMON_MSRS=0,4,8,12
 *
 * @author Hank Hoffmann
 * @author Connor Imes
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "energymon.h"
#include "energymon-msr.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* impl) {
  return energymon_get_msr(impl);
}
#endif

#define MSR_RAPL_POWER_UNIT		0x606

/*
 * Platform specific RAPL Domains.
 * Note that PP1 RAPL Domain is supported on 062A only
 * And DRAM RAPL Domain is supported on 062D only
 */
/* Package RAPL Domain */
#define MSR_PKG_RAPL_POWER_LIMIT	0x610
#define MSR_PKG_ENERGY_STATUS		0x611
#define MSR_PKG_PERF_STATUS		0x613
#define MSR_PKG_POWER_INFO		0x614

/* PP0 RAPL Domain */
#define MSR_PP0_POWER_LIMIT		0x638
#define MSR_PP0_ENERGY_STATUS		0x639
#define MSR_PP0_POLICY			0x63A
#define MSR_PP0_PERF_STATUS		0x63B

/* PP1 RAPL Domain, may reflect to uncore devices */
#define MSR_PP1_POWER_LIMIT		0x640
#define MSR_PP1_ENERGY_STATUS		0x641
#define MSR_PP1_POLICY			0x642

/* DRAM RAPL Domain */
#define MSR_DRAM_POWER_LIMIT		0x618
#define MSR_DRAM_ENERGY_STATUS		0x619
#define MSR_DRAM_PERF_STATUS		0x61B
#define MSR_DRAM_POWER_INFO		0x61C

/* RAPL UNIT BITMASK */
#define POWER_UNIT_OFFSET	0
#define POWER_UNIT_MASK		0x0F

#define ENERGY_UNIT_OFFSET	0x08
#define ENERGY_UNIT_MASK	0x1F00

#define TIME_UNIT_OFFSET	0x10
#define TIME_UNIT_MASK		0xF000

typedef struct msr_info {
  int fd;
  double energy_units;
} msr_info;

typedef struct energymon_msr {
  int msr_count;
  msr_info* msrs;
} energymon_msr;

static inline int open_msr(unsigned int core) {
  char filename[24];
  sprintf(filename, "/dev/cpu/%u/msr", core);
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "open_msr:open: %s: %s\n", filename, strerror(errno));
  }
  return fd;
}

static inline long long read_msr(int fd, int which) {
  uint64_t data = 0;
  if (pread(fd, &data, sizeof(data), which) != sizeof(data)) {
    perror("read_msr:pread");
    return -1;
  }
  return (long long) data;
}

static inline unsigned int msr_count(char* env_cores) {
  unsigned int ncores = 0;
  char* saveptr;
  char* env_cores_tmp = strdup(env_cores);
  if (env_cores_tmp == NULL) {
    return 0;
  }
  char* tok = strtok_r(env_cores_tmp, ENERGYMON_MSRS_DELIMS, &saveptr);
  while (tok) {
    ncores++;
    tok = strtok_r(NULL, ENERGYMON_MSRS_DELIMS, &saveptr);
  }
  free(env_cores_tmp);
  return ncores;
}

static inline int msr_core_ids(char* env_cores,
                               unsigned int* core_ids,
                               unsigned int ncores) {
  unsigned int i;
  char* saveptr;
  char* env_cores_tmp = strdup(env_cores);
  if (env_cores_tmp == NULL) {
    return -1;
  }
  char* tok = strtok_r(env_cores_tmp, ENERGYMON_MSRS_DELIMS, &saveptr);
  for (i = 0; tok != NULL && i < ncores; i++) {
    core_ids[i] = strtoul(tok, NULL, 0);
    tok = strtok_r(NULL, ENERGYMON_MSRS_DELIMS, &saveptr);
  }
  free(env_cores_tmp);
  return 0;
}

static inline int msr_info_init(msr_info* m, unsigned int id) {
  m->fd = open_msr(id);
  if (m->fd < 0) {
    return -1;
  }
  long long power_unit_data_ll = read_msr(m->fd, MSR_RAPL_POWER_UNIT);
  if (power_unit_data_ll < 0) {
    return -1;
  }
  double power_unit_data = (double) ((power_unit_data_ll >> 8) & 0x1f);
  m->energy_units = pow(0.5, power_unit_data);
  return 0;
}

int energymon_init_msr(energymon* impl) {
  if (impl == NULL || impl->state != NULL) {
    return -1;
  }

  unsigned int i;
  unsigned int ncores;
  unsigned int* core_ids;
  // get a delimited list of cores with MSRs to read from
  char* env_cores = getenv(ENERGYMON_MSR_ENV_VAR);
  if (env_cores == NULL) {
    // default to using core 0
    ncores = 1;
    core_ids = malloc(sizeof(unsigned int));
    if (core_ids == NULL) {
      return -1;
    }
    core_ids[0] = 0;
  } else {
    ncores = msr_count(env_cores);
    if (ncores == 0) {
      fprintf(stderr, "energymon_init_msr: Failed to parse core numbers from "
              ENERGYMON_MSR_ENV_VAR " environment variable.\n");
      return -1;
    }
    // Now determine which cores' MSRs will be accessed
    core_ids = malloc(ncores * sizeof(unsigned int));
    if (core_ids == NULL) {
      return -1;
    }
    if(msr_core_ids(env_cores, core_ids, ncores)) {
      free(core_ids);
      return -1;
    }
  }

  energymon_msr* em = malloc(sizeof(energymon_msr));
  if (em == NULL) {
    free(core_ids);
    return -1;
  }
  impl->state = em;

  // allocate shared variables
  em->msrs = (msr_info*) malloc(ncores * sizeof(msr_info));
  if (em->msrs == NULL) {
    free(impl->state);
    free(core_ids);
    impl->state = NULL;
    return -1;
  }

  // open the MSR files
  for (i = 0; i < ncores; i++) {
    if(msr_info_init(&em->msrs[i], core_ids[i])) {
      energymon_finish_msr(impl);
      free(core_ids);
      return -1;
    }
  }
  free(core_ids);

  em->msr_count = ncores;
  return 0;
}

unsigned long long energymon_read_total_msr(const energymon* impl) {
  if (impl == NULL || impl->state == NULL) {
    return 0;
  }

  int i;
  long long msr_val;
  unsigned long long total = 0;
  energymon_msr* em = impl->state;
  for (i = 0; i < em->msr_count; i++) {
    msr_val = read_msr(em->msrs[i].fd, MSR_PKG_ENERGY_STATUS);
    if (msr_val < 0) {
      fprintf(stderr,
              "energymon_read_total_msr: got bad energy value from MSR\n");
      return 0;
    }
    total += msr_val * em->msrs[i].energy_units * 1000000;
  }
  return total;
}

int energymon_finish_msr(energymon* impl) {
  if (impl == NULL || impl->state == NULL) {
    return -1;
  }

  int ret = 0;
  energymon_msr* em = impl->state;
  if (em->msrs != NULL) {
    int i;
    for (i = 0; i < em->msr_count; i++) {
      if (em->msrs[i].fd >= 0) {
        ret += close(em->msrs[i].fd);
      }
    }
  }
  free(em->msrs);
  em->msrs = NULL;
  em->msr_count = 0;
  free(impl->state);
  impl->state = NULL;
  return ret;
}

char* energymon_get_source_msr(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "X86 MSR", n);
}

unsigned long long energymon_get_interval_msr(const energymon* em) {
  return 1000;
}

int energymon_get_msr(energymon* impl) {
  if (impl == NULL) {
    return -1;
  }
  impl->finit = &energymon_init_msr;
  impl->fread = &energymon_read_total_msr;
  impl->ffinish = &energymon_finish_msr;
  impl->fsource = &energymon_get_source_msr;
  impl->finterval = &energymon_get_interval_msr;
  impl->state = NULL;
  return 0;
}
