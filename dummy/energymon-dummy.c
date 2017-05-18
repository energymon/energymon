/**
 * Dummy implementation - doesn't read from any source.
 *
 * @author Connor Imes
 * @date 2014-07-30
 */

#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include "energymon.h"
#include "energymon-dummy.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_dummy(em);
}
#endif

int energymon_init_dummy(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  return 0;
}

uint64_t energymon_read_total_dummy(const energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return 0;
  }
  return 0;
}

int energymon_finish_dummy(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  return 0;
}

char* energymon_get_source_dummy(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "Dummy Source", n);
}

uint64_t energymon_get_interval_dummy(const energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return 0;
  }
  return 1;
}

uint64_t energymon_get_precision_dummy(const energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return 0;
  }
  return 1;
}

int energymon_is_exclusive_dummy(void) {
  return 0;
}

int energymon_get_dummy(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_dummy;
  em->fread = &energymon_read_total_dummy;
  em->ffinish = &energymon_finish_dummy;
  em->fsource = &energymon_get_source_dummy;
  em->finterval = &energymon_get_interval_dummy;
  em->fprecision = &energymon_get_precision_dummy;
  em->fexclusive = &energymon_is_exclusive_dummy;
  em->state = NULL;
  return 0;
}
