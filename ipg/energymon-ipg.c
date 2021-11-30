/**
 * Intel Power Gadget implementation.
 *
 * @author Connor Imes
 * @date 2021-11-29
 */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <PowerGadgetLib.h>
#include "energymon.h"
#include "energymon-ipg.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_ipg(em);
}
#endif

// Environment variable to not call IPG static lifecycle functions.
// Keeping this as an undocumented feature for now.
#define ENERGYMON_IPG_SKIP_LIFECYCLE "ENERGYMON_IPG_SKIP_LIFECYCLE"
// Environment variable to specify whether to use the PMU.
// Keeping this as an undocumented feature for now.
#define ENERGYMON_IPG_USE_PMU "ENERGYMON_IPG_USE_PMU"

typedef enum ipg_zone {
  IPG_ZONE_PACKAGE,
  IPG_ZONE_IA,
  // Note that there's no UNCORE zone
  IPG_ZONE_DRAM,
  IPG_ZONE_PLATFORM,
} ipg_zone;

typedef struct ipg_pkg {
  PGSampleID sampleID;
  double j_sum;
} ipg_pkg;

typedef struct energymon_ipg {
  ipg_zone zone;
  int n_pkgs;
  ipg_pkg pkgs[];
} energymon_ipg;

static int get_zone(ipg_zone* zone, const char* env_zone) {
  if (env_zone == NULL || !strcmp(env_zone, "PACKAGE")) {
    *zone = IPG_ZONE_PACKAGE;
  } else if (!strcmp(env_zone, "CORE") || !strcmp(env_zone, "IA")) {
    *zone = IPG_ZONE_IA;
  } else if (!strcmp(env_zone, "DRAM")) {
    *zone = IPG_ZONE_DRAM;
  } else if (!strcmp(env_zone, "PSYS") || !strcmp(env_zone, "PLATFORM")) {
    *zone = IPG_ZONE_PLATFORM;
  } else {
    fprintf(stderr, "energymon_init_ipg: Unknown zone: '%s'\n", env_zone);
    errno = EINVAL;
    return -1;
  }
  return 0;
}

static int get_use_pmu(const char* env_use_pmu) {
  // returns -1 if variable not set, 0 if set to something false, 1 otherwise
  if (env_use_pmu == NULL) {
    return -1;
  }
  if (!strcmp(env_use_pmu, "0") || !strcmp(env_use_pmu, "false") ||
      !strcmp(env_use_pmu, "False") || !strcmp(env_use_pmu, "FALSE")) {
    return 0;
  }
  return 1;
}

static void enforce_errno(int fallback) {
  if (!errno) {
    errno = fallback;
  }
}

static int ipg_maybe_init(void) {
  if (!getenv(ENERGYMON_IPG_SKIP_LIFECYCLE)) {
    if (!PG_Initialize()) {
      perror("PG_Initialize");
      return -1;
    }
  }
  return 0;
}

static int ipg_maybe_shutdown(void) {
  if (!getenv(ENERGYMON_IPG_SKIP_LIFECYCLE)) {
    if (!PG_Shutdown()) {
      perror("PG_Shutdown");
      return -1;
    }
  }
  return 0;
}

static int ipg_is_zone_available(int pkg, ipg_zone zone) {
  bool result;
  bool available = false;
  switch (zone) {
    case IPG_ZONE_PACKAGE:
      // always supported
      result = true;
      available = true;
      break;
    case IPG_ZONE_IA:
      if (!(result = PG_IsIAEnergyAvailable(pkg, &available))) {
        perror("PG_IsIAEnergyAvailable");
      }
      break;
    case IPG_ZONE_DRAM:
      if (!(result = PG_IsDRAMEnergyAvailable(pkg, &available))) {
        perror("PG_IsDRAMEnergyAvailable");
      }
      break;
    case IPG_ZONE_PLATFORM:
      if (!(result = PG_IsPlatformEnergyAvailable(pkg, &available))) {
#ifdef __APPLE__
        // https://community.intel.com/t5/Software-Tuning-Performance/Power-Gadget-os-OSX-PowerGadgetLib-h-PG/td-p/1340398
        // "macOS version has no capability to read if platform energy is available or not, so it always returns false"
        fprintf(stderr, "energymon_init_ipg: Platform zone detection not supported on macOS\n");
        result = true;
#else
        perror("PG_IsPlatformEnergyAvailable");
#endif
      }
      break;
    default:
      // should never occur
      result = false;
      available = false;
      errno = ENOSYS;
      break;
  }
  return result ? (available ? 1 : 0) : -1;
}

static double ipg_get_zone_energy_j(ipg_zone zone, PGSampleID oldID, PGSampleID newID) {
  bool result;
  double w;
  double j = 0;
  switch (zone) {
    case IPG_ZONE_PACKAGE:
      if (!(result = PGSample_GetPackagePower(oldID, newID, &w, &j))) {
        perror("PGSample_GetPackagePower");
      }
      break;
    case IPG_ZONE_IA:
      if (!(result = PGSample_GetIAPower(oldID, newID, &w, &j))) {
        perror("PGSample_GetIAPower");
      }
      break;
    case IPG_ZONE_DRAM:
      if (!(result = PGSample_GetDRAMPower(oldID, newID, &w, &j))) {
        perror("PGSample_GetDRAMPower");
      }
      break;
    case IPG_ZONE_PLATFORM:
      if (!(result = PGSample_GetPlatformPower(oldID, newID, &w, &j))) {
        perror("PGSample_GetPlatformPower");
      }
      break;
    default:
      // should never occur
      result = false;
      errno = ENOSYS;
      break;
  }
  return result ? j : -1;
}

static void cleanup_samples(energymon_ipg* state, int n) {
  int pkg;
  for (pkg = 0; pkg < n; pkg++) {
    if (!PGSample_Release(state->pkgs[pkg].sampleID)) {
      // log error and continue with cleanup
      perror("PGSample_Release");
    }
  }
}

int energymon_init_ipg(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  ipg_zone zone = IPG_ZONE_PACKAGE;
  int n_pkgs = 0;
  int pkg;
  int available;
  int err_save;
  int use_pmu = get_use_pmu(getenv(ENERGYMON_IPG_USE_PMU));
  if (get_zone(&zone, getenv(ENERGYMON_IPG_ZONE))) {
    return -1;
  }

  errno = 0;
  if (ipg_maybe_init()) {
    enforce_errno(ENODEV);
    return -1;
  }
  if (!PG_GetNumPackages(&n_pkgs)) {
    perror("PG_GetNumPackages");
    enforce_errno(ENODEV);
    goto fail_post_init;
  }
  if (n_pkgs <= 0) {
    fprintf(stderr, "energymon_init_ipg: got invalid package count: %d\n", n_pkgs);
    enforce_errno(ENODEV);
    goto fail_post_init;
  }

  energymon_ipg* state = calloc(1, sizeof(energymon_ipg) + (n_pkgs * sizeof(ipg_pkg)));
  if (state == NULL) {
    goto fail_post_init;
  }
  state->n_pkgs = n_pkgs;
  state->zone = zone;

  // read initial samples
  for (pkg = 0; pkg < n_pkgs; pkg++) {
    if (use_pmu >= 0 && !PG_UsePMU(pkg, use_pmu ? true : false)) {
      perror("PG_UsePMU");
      enforce_errno(EAGAIN);
      goto fail_sampling;
    }
    available = ipg_is_zone_available(pkg, state->zone);
    if (available < 0) {
      enforce_errno(EAGAIN);
      goto fail_sampling;
    } else if (!available) {
      fprintf(stderr, "energymon_init_ipg: zone not available\n");
      enforce_errno(ENODEV);
      goto fail_sampling;
    }
    if (!PG_ReadSample(pkg, &state->pkgs[pkg].sampleID)) {
      perror("PG_ReadSample");
      enforce_errno(EAGAIN);
      goto fail_sampling;
    }
  }
  em->state = state;
  return 0;

fail_sampling:
  err_save = errno;
  cleanup_samples(state, pkg);
  free(state);
  errno = err_save;
fail_post_init:
  err_save = errno;
  ipg_maybe_shutdown();
  errno = err_save;
  return -1;
}

uint64_t energymon_read_total_ipg(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  energymon_ipg* state = (energymon_ipg*) em->state;
  PGSampleID newID;
  double j_total = 0;
  double j;
  int pkg;
  int err_save;
  for (pkg = 0; pkg < state->n_pkgs; pkg++) {
    errno = 0;
    if (!PG_ReadSample(pkg, &newID)) {
      perror("PG_ReadSample");
      enforce_errno(EAGAIN);
      return 0;
    }
    if ((j = ipg_get_zone_energy_j(state->zone, state->pkgs[pkg].sampleID, newID)) < 0) {
      // on error, keep the old sample ID and discard the new one
      // previous packages will have up-to-date info, we just won't be able to report them now
      enforce_errno(EAGAIN);
      err_save = errno;
      if (!PGSample_Release(newID)) {
        perror("PGSample_Release");
      }
      errno = err_save;
      return 0;
    }
    state->pkgs[pkg].j_sum += j;
    j_total += state->pkgs[pkg].j_sum;
    // clean up the old sample
    if (!PGSample_Release(state->pkgs[pkg].sampleID)) {
      // log and continue since it doesn't affect our readings
      perror("PGSample_Release");
    }
    state->pkgs[pkg].sampleID = newID;
  }
  return j_total * 1000000;
}

int energymon_finish_ipg(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }
  energymon_ipg* state = (energymon_ipg*) em->state;
  int pkg;
  int ret = 0;
  errno = 0;
  for (pkg = 0; pkg < state->n_pkgs; pkg++) {
    if (!PGSample_Release(state->pkgs[pkg].sampleID)) {
      // log error and continue
      perror("PGSample_Release");
      enforce_errno(EAGAIN);
      ret = -1;
    }
  }
  free(em->state);
  em->state = NULL;
  if (ipg_maybe_shutdown()) {
    enforce_errno(EAGAIN);
    ret = -1;
  }
  return ret;
}

char* energymon_get_source_ipg(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "Intel Power Gadget", n);
}

uint64_t energymon_get_interval_ipg(const energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return 0;
  }
  return 1000;
}

uint64_t energymon_get_precision_ipg(const energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return 0;
  }
  // no way to know without reading MSR that specifies energy units
  return 0;
}

int energymon_is_exclusive_ipg(void) {
  // Should be false if we're not using the PMU, but even then PG_Shutdown appears to cause other
  // software using the PMU (like IPG) to start failing to read samples.
  // Running multiple energymon instances without PMU works, but that config option is undocumented.
  // Even if we default to not using the PMU, PG_Shutdown behavior still affects SW using the PMU.
  return 1;
}

int energymon_get_ipg(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_ipg;
  em->fread = &energymon_read_total_ipg;
  em->ffinish = &energymon_finish_ipg;
  em->fsource = &energymon_get_source_ipg;
  em->finterval = &energymon_get_interval_ipg;
  em->fprecision = &energymon_get_precision_ipg;
  em->fexclusive = &energymon_is_exclusive_ipg;
  em->state = NULL;
  return 0;
}
