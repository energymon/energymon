/**
 * Common code for reading Cray PM energy files.
 *
 * @author Connor Imes
 * @date 2017-06-28
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "energymon.h"
#include "energymon-cray-pm-common.h"

int energymon_cray_pm_common_init(energymon* em, const char* file) {
  assert(file != NULL);
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }
  energymon_cray_pm_common* state = calloc(1, sizeof(energymon_cray_pm_common));
  if (state == NULL) {
    return -1;
  }
  char buf[64];
  snprintf(buf, sizeof(buf), CRAY_PM_BASE_DIR"/%s", file);
  if ((state->f = fopen(buf, "r")) == NULL) {
    perror(buf);
    free(state);
    return -1;
  }
  em->state = state;
  return 0;
}

uint64_t energymon_cray_pm_common_read_total(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  const energymon_cray_pm_common* state = (energymon_cray_pm_common*) em->state;
  uint64_t joules = 0;
  rewind(state->f);
  if (fscanf(state->f, "%"PRIu64" J", &joules) != 1) {
    return 0;
  }
  return joules * 1000000;
}

int energymon_cray_pm_common_finish(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }
  const energymon_cray_pm_common* state = (energymon_cray_pm_common*) em->state;
  if (state->f != NULL) {
    fclose(state->f);
  }
  free(em->state);
  em->state = NULL;
  return 0;
}

uint64_t energymon_cray_pm_common_get_interval(const energymon* em) {
  if (em == NULL) {
    // we don't need to access em, but it's still a programming error
    errno = EINVAL;
    return 0;
  }
  // default is 10 Hz
  uint64_t us = 100000;
  uint64_t hz = 0;
  FILE* f = fopen(CRAY_PM_BASE_DIR"/raw_scan_hz", "r");
  if (f == NULL) {
    perror(CRAY_PM_BASE_DIR"/raw_scan_hz");
  } else {
    if (fscanf(f, "%"PRIu64, &hz) == 1 && hz > 0) {
      // TODO: What if hz doesn't divide evenly?
      us = 1000000 / hz;
    }
    fclose(f);
  }
  return us;
}

uint64_t energymon_cray_pm_common_get_precision(const energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return 0;
  }
  // 1 J
  return 1000000;
}
