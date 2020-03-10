/**
 * Read energy from X86 MSRs (Model-Specific Registers) using the RAPLCap library for MSR portability and discovery.
 *
 * @author Connor Imes
 * @date 2018-05-19
 */
#include <errno.h>
#include <inttypes.h>
#include <raplcap.h>
#include <raplcap-msr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "energymon.h"
#include "energymon-raplcap-msr.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_raplcap_msr(em);
}
#endif

typedef struct raplcap_msr_info {
  double j_last;
  double j_max;
  uint32_t n_overflow;
  int is_active;
} raplcap_msr_info;

typedef struct energymon_raplcap_msr {
  raplcap rc;
  raplcap_zone zone;
  uint32_t n_msrs;
  raplcap_msr_info msrs[];
} energymon_raplcap_msr;

static int get_raplcap_zone(raplcap_zone* zone, const char* env_zone) {
  if (env_zone == NULL || !strcmp(env_zone, "PACKAGE")) {
    *zone = RAPLCAP_ZONE_PACKAGE;
  } else if (!strcmp(env_zone, "CORE")) {
    *zone = RAPLCAP_ZONE_CORE;
  } else if (!strcmp(env_zone, "UNCORE")) {
    *zone = RAPLCAP_ZONE_UNCORE;
  } else if (!strcmp(env_zone, "DRAM")) {
    *zone = RAPLCAP_ZONE_DRAM;
  } else if (!strcmp(env_zone, "PSYS") || !strcmp(env_zone, "PLATFORM")) {
    *zone = RAPLCAP_ZONE_PSYS;
  } else {
    fprintf(stderr, "energymon_init_raplcap_msr: Unknown zone: '%s'\n", env_zone);
    errno = EINVAL;
    return -1;
  }
  return 0;
}

static int get_active_instances(raplcap_msr_info* msrs, uint32_t n_msrs) {
  uint32_t i;
  char* tmp;
  char* tok;
  char* saveptr;
  char* saveptr_parse = NULL;
  const char* env_inst = getenv(ENERGYMON_RAPLCAP_MSR_INSTANCES);
  if (env_inst == NULL) {
    for (i = 0; i < n_msrs; i++) {
      msrs[i].is_active = 1;
    }
  } else {
    if ((tmp = strdup(env_inst)) == NULL) {
      perror("energymon_init_raplcap_msr: strdup");
      return -1;
    }
    tok = strtok_r(tmp, ",", &saveptr);
    while (tok != NULL) {
      i = strtoul(tok, &saveptr_parse, 0);
      if (tok == saveptr_parse) {
        // no conversion was actually made
        fprintf(stderr, "energymon_init_raplcap_msr: Failed to parse env var: "ENERGYMON_RAPLCAP_MSR_INSTANCES"=%s\n",
                env_inst);
        free(tmp);
        errno = EINVAL;
        return -1;
      }
      if (i >= n_msrs) {
        fprintf(stderr, "energymon_init_raplcap_msr: Env var "ENERGYMON_RAPLCAP_MSR_INSTANCES" value out of range "
                "[0, %"PRIu32"): %"PRIu32"\n", n_msrs, i);
        free(tmp);
        errno = ERANGE;
        return -1;
      }
      msrs[i].is_active = 1;
      tok = strtok_r(NULL, ",", &saveptr);
    }
    free(tmp);
  }
  return 0;
}

int energymon_init_raplcap_msr(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  int err_save;
  int supp;
  uint32_t i;
  uint32_t n_msrs = raplcap_get_num_sockets(NULL);
  if (n_msrs == 0) {
    perror("raplcap_get_num_sockets");
    return -1;
  }

  energymon_raplcap_msr* state = calloc(1, sizeof(energymon_raplcap_msr) + (n_msrs * sizeof(raplcap_msr_info)));
  if (state == NULL) {
    return -1;
  }
  state->n_msrs = n_msrs;

  if (get_active_instances(state->msrs, state->n_msrs)) {
    free(state);
    return -1;
  }

  const char* env_zone = getenv(ENERGYMON_RAPLCAP_MSR_ZONE);
  if (get_raplcap_zone(&state->zone, env_zone)) {
    free(state);
    return -1;
  }

  if (raplcap_init(&state->rc)) {
    perror("raplcap_init");
    free(state);
    return -1;
  }

  for (i = 0; i < state->n_msrs; i++) {
    if (!state->msrs[i].is_active) {
      continue;
    }
    // first check if zone is supported
    supp = raplcap_is_zone_supported(&state->rc, i, state->zone);
    if (supp < 0) {
      perror("raplcap_is_zone_supported");
      goto fail;
    }
    if (supp == 0) {
      fprintf(stderr, "energymon_init_raplcap_msr: Unsupported zone: %s\n", env_zone == NULL ? "PACKAGE" : env_zone);
      errno = EINVAL;
      goto fail;
    }
    // Note: max energy is specified in a different MSR than the zone's energy counter,
    // so this call might still work for unsupported zones (which is why we have to check for support first)
    if ((state->msrs[i].j_max = raplcap_get_energy_counter_max(&state->rc, i, state->zone)) < 0) {
      perror("raplcap_get_energy_counter_max");
      goto fail;
    }
  }

  em->state = state;
  return 0;

fail:
  err_save = errno;
  if (raplcap_destroy(&state->rc)) {
    perror("raplcap_destroy");
  }
  free(state);
  errno = err_save;
  return -1;
}

uint64_t energymon_read_total_raplcap_msr(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  uint32_t i;
  double j;
  uint64_t total = 0;
  energymon_raplcap_msr* state = (energymon_raplcap_msr*) em->state;
  for (errno = 0, i = 0; i < state->n_msrs && !errno; i++) {
    if (!state->msrs[i].is_active) {
      continue;
    }
    if ((j = raplcap_get_energy_counter(&state->rc, i, state->zone)) >= 0) {
      if (j < state->msrs[i].j_last) {
        state->msrs[i].n_overflow++;
      }
      state->msrs[i].j_last = j;
      total += (uint64_t) ((j + state->msrs[i].n_overflow * state->msrs[i].j_max) * 1000000.0);
    }
  }
  return errno ? 0 : total;
}

int energymon_finish_raplcap_msr(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }
  int ret;
  energymon_raplcap_msr* state = em->state;
  if ((ret = raplcap_destroy(&state->rc))) {
    perror("raplcap_destroy");
  }
  free(em->state);
  em->state = NULL;
  return ret;
}

char* energymon_get_source_raplcap_msr(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "Intel MSRs via libraplcap-msr", n);
}

uint64_t energymon_get_interval_raplcap_msr(const energymon* em) {
  if (em == NULL) {
    // we don't need to access em, but it's still a programming error
    errno = EINVAL;
    return 0;
  }
  return 1000;
}

uint64_t energymon_get_precision_raplcap_msr(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  energymon_raplcap_msr* state = em->state;
  double j;
  double max = 0;
  uint32_t i;
  for (i = 0; i < state->n_msrs; i++) {
    if (!state->msrs[i].is_active) {
      continue;
    }
    // precision limited by the largest units (they should all be the same though)
    if ((j = raplcap_msr_get_energy_units(&state->rc, 0, state->zone)) > max) {
      max = j;
    }
  }
  return (uint64_t) (max * 1000000);
}

int energymon_is_exclusive_raplcap_msr(void) {
  return 0;
}

int energymon_get_raplcap_msr(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_raplcap_msr;
  em->fread = &energymon_read_total_raplcap_msr;
  em->ffinish = &energymon_finish_raplcap_msr;
  em->fsource = &energymon_get_source_raplcap_msr;
  em->finterval = &energymon_get_interval_raplcap_msr;
  em->fprecision = &energymon_get_precision_raplcap_msr;
  em->fexclusive = &energymon_is_exclusive_raplcap_msr;
  em->state = NULL;
  return 0;
}
