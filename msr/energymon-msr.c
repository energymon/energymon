/**
 * Read energy from X86 MSRs (Model-Specific Registers).
 *
 * By default, the MSR on cpu0 is read. To configure other MSRs, set the
 * ENERGYMON_MSRS environment variable with a comma-delimited list of CPU IDs,
 * e.g.:
 *   export ENERGYMON_MSRS=0,4,8,12
 *
 * @author Connor Imes
 * @author Hank Hoffmann
 */

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "energymon.h"
#include "energymon-msr.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_msr(em);
}
#endif

#define MSR_RAPL_POWER_UNIT		0x606

/* Package RAPL Domain */
#define MSR_PKG_ENERGY_STATUS		0x611

/* PP0 RAPL Domain */
#define MSR_PP0_ENERGY_STATUS		0x639

/* PP1 RAPL Domain, may reflect to uncore devices */
#define MSR_PP1_ENERGY_STATUS		0x641

/* DRAM RAPL Domain */
#define MSR_DRAM_ENERGY_STATUS		0x619

typedef struct msr_info {
  int fd;
  unsigned int n_overflow;
  uint64_t energy_last;
  double energy_units;
} msr_info;

typedef struct energymon_msr {
  unsigned int msr_count;
  msr_info msrs[];
} energymon_msr;

/**
 * env_cores is consumed by strtok_r, so cannot be reused after this function.
 */
static inline unsigned int count_msrs(char* env_cores) {
  unsigned int ncores;
  char* saveptr;
  char* tok = strtok_r(env_cores, ENERGYMON_MSRS_DELIMS, &saveptr);
  for (ncores = 0; tok; ncores++) {
    tok = strtok_r(NULL, ENERGYMON_MSRS_DELIMS, &saveptr);
  }
  return ncores;
}

/**
 * env_cores is consumed by strtok_r, so cannot be reused after this function.
 * Returns the errno (if any)
 */
static inline int msr_info_init(msr_info* m, unsigned int n, char* env_cores) {
  uint64_t msr_val;
  unsigned int i;
  unsigned int energy_status_units;
  char filename[32];
  char* saveptr;
  const char* tok = env_cores == NULL ? "0" :
    strtok_r(env_cores, ENERGYMON_MSRS_DELIMS, &saveptr);
  for (i = 0; tok && i < n; i++) {
    m[i].n_overflow = 0;
    m[i].energy_last = 0;
    // first try msr_safe file
    snprintf(filename, sizeof(filename), "/dev/cpu/%s/msr_safe", tok);
    if ((m[i].fd = open(filename, O_RDONLY)) <= 0) {
      // fall back on regular msr file
      snprintf(filename, sizeof(filename), "/dev/cpu/%s/msr", tok);
      if ((m[i].fd = open(filename, O_RDONLY)) <= 0) {
        perror(filename);
        return errno;
      }
    }
    if (pread(m[i].fd, &msr_val, sizeof(msr_val), MSR_RAPL_POWER_UNIT) < 0) {
      perror(filename);
      return errno;
    }

    // Energy related information (in Joules) is based on the multiplier,
    // 1/2^ESU; where ESU is an unsigned integer represented by bits 12:8.
    energy_status_units = ((msr_val >> 8) & 0x1f);
    // At 5 bits only, 0 <= energy_status_units < 32, so bit shift instead,
    // no need to use "pow" and require linking to math library
    // m[i].energy_units = pow(0.5, energy_status_units);
    m[i].energy_units = 1.0 / (1 << energy_status_units);
    tok = env_cores == NULL ? NULL :
      strtok_r(NULL, ENERGYMON_MSRS_DELIMS, &saveptr);
  }
  return 0;
}

int energymon_init_msr(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  unsigned int ncores = 1;
  char* tmp = NULL;
  // get a delimited list of cores with MSRs to read from
  const char* env_cores = getenv(ENERGYMON_MSR_ENV_VAR);
  if (env_cores != NULL) {
    if ((tmp = strdup(env_cores)) == NULL) {
      return -1;
    }
    ncores = count_msrs(tmp);
    free(tmp);
    if (ncores == 0) {
      errno = EINVAL;
      perror("Parsing number of cores from " ENERGYMON_MSR_ENV_VAR " env var");
      return -1;
    }
    if ((tmp = strdup(env_cores)) == NULL) {
      return -1;
    }
  }

  size_t size = sizeof(energymon_msr) + ncores * sizeof(msr_info);
  energymon_msr* state = calloc(1, size);
  if (state == NULL) {
    free(tmp);
    return -1;
  }
  state->msr_count = ncores;

  // open the MSR files
  em->state = state;
  int save_err = msr_info_init(state->msrs, ncores, tmp);
  free(tmp);
  if (save_err) {
    energymon_finish_msr(em);
    errno = save_err;
    return -1;
  }

  return 0;
}

uint64_t energymon_read_total_msr(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }

  unsigned int i;
  uint64_t msr_val;
  uint64_t total = 0;
  energymon_msr* state = (energymon_msr*) em->state;
  for (errno = 0, i = 0; i < state->msr_count && !errno; i++) {
    if (pread(state->msrs[i].fd, &msr_val, sizeof(uint64_t),
              MSR_PKG_ENERGY_STATUS) == sizeof(uint64_t)) {
      // bits 31:0 hold the energy consumption counter, ignore upper 32 bits
      msr_val &= 0xFFFFFFFF;
      // overflows at 32 bits
      if (msr_val < state->msrs[i].energy_last) {
        state->msrs[i].n_overflow++;
      }
      state->msrs[i].energy_last = msr_val;
      total += (msr_val + state->msrs[i].n_overflow * (uint64_t) UINT32_MAX)
               * state->msrs[i].energy_units * 1000000;
    }
  }
  return errno ? 0 : total;
}

int energymon_finish_msr(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }

  int err_save = 0;
  unsigned int i;
  energymon_msr* state = em->state;
  for (i = 0; i < state->msr_count; i++) {
    if (state->msrs[i].fd > 0 && close(state->msrs[i].fd)) {
      err_save = errno;
    }
  }
  free(em->state);
  em->state = NULL;
  errno = err_save;
  return errno ? -1 : 0;
}

char* energymon_get_source_msr(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "X86 MSR", n);
}

uint64_t energymon_get_interval_msr(const energymon* em) {
  if (em == NULL) {
    // we don't need to access em, but it's still a programming error
    errno = EINVAL;
    return 0;
  }
  return 1000;
}

uint64_t energymon_get_precision_msr(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  energymon_msr* state = (energymon_msr*) em->state;
  uint64_t prec;
  unsigned int i;
  if (state->msr_count == 0) {
    prec = 1;
  } else {
    prec = UINT64_MAX;
    for (i = 0; i < state->msr_count; i++) {
      if (state->msrs[i].energy_units < prec) {
        // 61 uJ by default
        prec = (uint64_t) (state->msrs[i].energy_units * 1000000);
      }
    }
  }
  return prec ? prec : 1;
}

int energymon_is_exclusive_msr(void) {
  return 0;
}

int energymon_get_msr(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_msr;
  em->fread = &energymon_read_total_msr;
  em->ffinish = &energymon_finish_msr;
  em->fsource = &energymon_get_source_msr;
  em->finterval = &energymon_get_interval_msr;
  em->fprecision = &energymon_get_precision_msr;
  em->fexclusive = &energymon_is_exclusive_msr;
  em->state = NULL;
  return 0;
}
