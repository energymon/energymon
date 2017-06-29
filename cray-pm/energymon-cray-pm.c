/**
 * Read energy from Cray PM files specified by an environment variable.
 *
 * @author Connor Imes
 * @date 2017-06-28
 */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "energymon.h"
#include "energymon-cray-pm.h"
#include "energymon-cray-pm-common.h"
#include "energymon-cray-pm-energy.h"
#include "energymon-cray-pm-accel_energy.h"
#include "energymon-cray-pm-cpu_energy.h"
#include "energymon-cray-pm-memory_energy.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT_CRAY_PM
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_cray_pm(em);
}
#endif

typedef enum energymon_cray_pm_file {
  FILE_ENERGY,
  FILE_ACCEL_ENERGY,
  FILE_CPU_ENERGY,
  FILE_MEMORY_ENERGY,
  FILE_COUNT
} energymon_cray_pm_file;

typedef struct energymon_cray_pm {
  energymon file[FILE_COUNT];
  int has_file[FILE_COUNT];
  FILE* f_freshness;
} energymon_cray_pm;

static int cray_pm_open_files(energymon_cray_pm* state) {
  int ret = 0;
  energymon* em;
  char* saveptr;
  char* tmp;
  const char* tok;
  const char* env_files = getenv(ENERGYMON_CRAY_PM_COUNTERS_ENV_VAR);
  if (env_files == NULL) {
    fprintf(stderr, "Must set environment variable %s\n", ENERGYMON_CRAY_PM_COUNTERS_ENV_VAR);
    return -1;
  } else {
    tmp = strdup(env_files);
    if (tmp == NULL) {
      return -1;
    }
    tok = strtok_r(tmp, ",", &saveptr);
    while (tok != NULL) {
      if (strncmp(tok, ENERGYMON_CRAY_PM_COUNTER_ENERGY, sizeof(ENERGYMON_CRAY_PM_COUNTER_ENERGY)) == 0) {
        if (!state->has_file[FILE_ENERGY]) {
          em = &state->file[FILE_ENERGY];
          energymon_get_cray_pm_energy(em);
          if (em->finit(em)) {
            ret = -1;
            break;
          }
          state->has_file[FILE_ENERGY] = 1;
        }
      } else if (strncmp(tok, ENERGYMON_CRAY_PM_COUNTER_ACCEL_ENERGY, sizeof(ENERGYMON_CRAY_PM_COUNTER_ACCEL_ENERGY)) == 0) {
        if (!state->has_file[FILE_ACCEL_ENERGY]) {
          em = &state->file[FILE_ACCEL_ENERGY];
          energymon_get_cray_pm_accel_energy(em);
          if (em->finit(em)) {
            ret = -1;
            break;
          }
          state->has_file[FILE_ACCEL_ENERGY] = 1;
        }
      } else if (strncmp(tok, ENERGYMON_CRAY_PM_COUNTER_CPU_ENERGY, sizeof(ENERGYMON_CRAY_PM_COUNTER_CPU_ENERGY)) == 0) {
        if (!state->has_file[FILE_CPU_ENERGY]) {
          em = &state->file[FILE_CPU_ENERGY];
          energymon_get_cray_pm_cpu_energy(em);
          if (em->finit(em)) {
            ret = -1;
            break;
          }
          state->has_file[FILE_CPU_ENERGY] = 1;
        }
      } else if (strncmp(tok, ENERGYMON_CRAY_PM_COUNTER_MEMORY_ENERGY, sizeof(ENERGYMON_CRAY_PM_COUNTER_MEMORY_ENERGY)) == 0) {
        if (!state->has_file[FILE_MEMORY_ENERGY]) {
          em = &state->file[FILE_MEMORY_ENERGY];
          energymon_get_cray_pm_memory_energy(em);
          if (em->finit(em)) {
            ret = -1;
            break;
          }
          state->has_file[FILE_MEMORY_ENERGY] = 1;
        }
      } else {
        // unknown token
        fprintf(stderr, "cray_pm_open_files: Unknown token in environment variable %s: %s\n",
                ENERGYMON_CRAY_PM_COUNTERS_ENV_VAR, tok);
        ret = -1;
        errno = EINVAL;
        break;
      }
      tok = strtok_r(NULL, ",", &saveptr);
    }
    free(tmp);
  }
  return ret;
}

int energymon_init_cray_pm(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }
  int err_save;
  energymon_cray_pm* state = calloc(1, sizeof(energymon_cray_pm));
  if (state == NULL) {
    return -1;
  }
  if ((state->f_freshness = fopen(CRAY_PM_BASE_DIR"/freshness", "r")) == NULL) {
    perror(CRAY_PM_BASE_DIR"/freshness");
    free(state);
    return -1;
  }
  em->state = state;
  if (cray_pm_open_files(state)) {
    err_save = errno;
    energymon_finish_cray_pm(em);
    errno = err_save;
    return -1;
  }
  return 0;
}

uint64_t energymon_read_total_cray_pm(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  unsigned int i;
  uint64_t joules;
  uint64_t tmp;
  uint64_t fresh_start = 1;
  uint64_t fresh_end = 0;
  const energymon_cray_pm* state = (energymon_cray_pm*) em->state;
  if (state->f_freshness == NULL) {
    errno = EINVAL;
    return 0;
  }
  // don't let the counters update in the middle of reading - check freshness
  while (fresh_start != fresh_end) {
    joules = 0;
    rewind(state->f_freshness);
    if (fscanf(state->f_freshness, "%"PRIu64, &fresh_start) != 1) {
      return 0;
    }
    for (i = 0; i < FILE_COUNT; i++) {
      if (state->has_file[i]) {
        errno = 0;
        tmp = state->file[i].fread(&state->file[i]);
        if (tmp == 0 && errno) {
          return 0;
        }
        joules += tmp;
      }
    }
    rewind(state->f_freshness);
    if (fscanf(state->f_freshness, "%"PRIu64, &fresh_end) != 1) {
      return 0;
    }
  }
  return joules * 1000000;
}

int energymon_finish_cray_pm(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }
  unsigned int i;
  int err_save = 0;
  energymon_cray_pm* state = (energymon_cray_pm*) em->state;
  for (i = 0; i < FILE_COUNT; i++) {
    if (state->has_file[i]) {
      if (state->file[i].ffinish(&state->file[i]) && !err_save) {
        err_save = errno;
      }
    }
  }
  if (state->f_freshness != NULL) {
    if (fclose(state->f_freshness) && !err_save) {
      err_save = errno;
    }
  }
  free(em->state);
  em->state = NULL;
  if (err_save) {
    errno = err_save;
    return -1;
  }
  return 0;
}

char* energymon_get_source_cray_pm(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "Cray Power Monitoring files", n);
}

uint64_t energymon_get_interval_cray_pm(const energymon* em) {
  return energymon_cray_pm_common_get_interval(em);
}

uint64_t energymon_get_precision_cray_pm(const energymon* em) {
  return energymon_cray_pm_common_get_precision(em);
}

int energymon_is_exclusive_cray_pm(void) {
  return 0;
}

int energymon_get_cray_pm(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_cray_pm;
  em->fread = &energymon_read_total_cray_pm;
  em->ffinish = &energymon_finish_cray_pm;
  em->fsource = &energymon_get_source_cray_pm;
  em->finterval = &energymon_get_interval_cray_pm;
  em->fprecision = &energymon_get_precision_cray_pm;
  em->fexclusive = &energymon_is_exclusive_cray_pm;
  em->state = NULL;
  return 0;
}
