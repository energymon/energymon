/**
 * Read energy from X86 MSRs (Model-Specific Registers).
 *
 * By default, the MSR on cpu0 is read. To configure other MSRs, set the
 * EM_MSRS environment variable with a comma-delimited list of CPU IDs, e.g.:
 *   export EM_MSRS=0,4,8,12
 *
 * @author Hank Hoffmann
 * @author Connor Imes
 */

#include "energymon.h"
#include "energymon-msr.h"
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

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

typedef struct energymon_msr {
  int msr_count;
  int* msr_fds;
  double* msr_energy_units;
} energymon_msr;

#ifdef EM_DEFAULT
int energymon_get_default(energymon* impl) {
  return energymon_get_msr(impl);
}
#endif

static inline int open_msr(int core) {
  char msr_filename[BUFSIZ];
  int fd;

  sprintf(msr_filename, "/dev/cpu/%d/msr", core);
  fd = open(msr_filename, O_RDONLY);
  if ( fd < 0 ) {
    if ( errno == ENXIO ) {
      fprintf(stderr, "open_msr: No CPU %d\n", core);
    } else if ( errno == EIO ) {
      fprintf(stderr, "open_msr: CPU %d doesn't support MSRs\n", core);
    } else {
      perror("open_msr:open");
      fprintf(stderr, "Trying to open %s\n", msr_filename);
    }
  }
  return fd;
}

static inline long long read_msr(int fd, int which) {
  uint64_t data = 0;
  uint64_t data_size = pread(fd, &data, sizeof data, which);

  if ( data_size != sizeof data ) {
    perror("read_msr:pread");
    return -1;
  }

  return (long long)data;
}

int energymon_init_msr(energymon* impl) {
  if (impl == NULL || impl->state != NULL) {
    return -1;
  }

  int ncores = 0;
  int i;
  long long power_unit_data_ll;
  double power_unit_data;
  // get a delimited list of cores with MSRs to read from
  char* env_cores = getenv(EM_MSR_ENV_VAR);
  char* env_cores_tmp; // need a writable string for strtok function
  if (env_cores == NULL) {
    // default to using core 0
    env_cores = "0";
  }

  // first determine the number of MSRs to be accessed
  env_cores_tmp = strdup(env_cores);
  char* tok = strtok(env_cores_tmp, EM_MSRS_DELIMS);
  while (tok != NULL) {
    ncores++;
    tok = strtok(NULL, EM_MSRS_DELIMS);
  }
  free(env_cores_tmp);
  if (ncores == 0) {
    fprintf(stderr, "em_init: Failed to parse core numbers from "
            "%s environment variable.\n", EM_MSR_ENV_VAR);
    return -1;
  }

  // Now determine which cores' MSRs will be accessed
  int core_ids[ncores];
  env_cores_tmp = strdup(env_cores);
  tok = strtok(env_cores_tmp, EM_MSRS_DELIMS);
  for (i = 0; tok != NULL; tok = strtok(NULL, EM_MSRS_DELIMS), i++) {
    core_ids[i] = atoi(tok);
    // printf("em_init: using MSR for core %d\n", core_ids[i]);
  }
  free(env_cores_tmp);

  energymon_msr* em = malloc(sizeof(energymon_msr));
  if (em == NULL) {
    return -1;
  }
  impl->state = em;

  // allocate shared variables
  em->msr_fds = (int*) malloc(ncores * sizeof(int));
  if (em->msr_fds == NULL) {
    free(impl->state);
    impl->state = NULL;
    return -1;
  }
  em->msr_energy_units = (double*) malloc(ncores * sizeof(double));
  if (em->msr_energy_units == NULL) {
    free(em->msr_fds);
    free(impl->state);
    impl->state = NULL;
    return -1;
  }

  // open the MSR files
  for (i = 0; i < ncores; i++) {
    em->msr_fds[i] = open_msr(core_ids[i]);
    if (em->msr_fds[i] < 0) {
      energymon_finish_msr(impl);
      return -1;
    }
    power_unit_data_ll = read_msr(em->msr_fds[i], MSR_RAPL_POWER_UNIT);
    if (power_unit_data_ll < 0) {
      energymon_finish_msr(impl);
      return -1;
    }
    power_unit_data = (double) ((power_unit_data_ll >> 8) & 0x1f);
    em->msr_energy_units[i] = pow(0.5, power_unit_data);
  }

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
    msr_val = read_msr(em->msr_fds[i], MSR_PKG_ENERGY_STATUS);
    if (msr_val < 0) {
      fprintf(stderr, "energymon_read_total: got bad energy value from MSR\n");
      return 0;
    }
    total += msr_val * em->msr_energy_units[i] * 1000000;
  }
  return total;
}

int energymon_finish_msr(energymon* impl) {
  if (impl == NULL || impl->state == NULL) {
    return -1;
  }

  int ret = 0;
  energymon_msr* em = impl->state;
  if (em->msr_fds != NULL) {
    int i;
    for (i = 0; i < em->msr_count; i++) {
      if (em->msr_fds[i] > 0) {
        ret += close(em->msr_fds[i]);
      }
    }
  }
  free(em->msr_fds);
  em->msr_fds = NULL;
  free(em->msr_energy_units);
  em->msr_energy_units = NULL;
  em->msr_count = 0;
  free(impl->state);
  impl->state = NULL;
  return ret;
}

char* energymon_get_source_msr(char* buffer) {
  if (buffer == NULL) {
    return NULL;
  }
  return strcpy(buffer, "X86 MSR");
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
