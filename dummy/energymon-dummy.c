/**
 * Dummy implementation - doesn't read from any source.
 *
 * @author Connor Imes
 * @date 2014-07-30
 */

#include <string.h>
#include "energymon.h"
#include "energymon-dummy.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* impl) {
  return energymon_get_dummy(impl);
}
#endif

int energymon_init_dummy(energymon* impl) {
  return 0;
}

unsigned long long energymon_read_total_dummy(const energymon* impl) {
  return 0;
}

int energymon_finish_dummy(energymon* impl) {
  return 0;
}

char* energymon_get_source_dummy(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "Dummy Source", n);
}

unsigned long long energymon_get_interval_dummy(const energymon* em) {
  return 1;
}

int energymon_get_dummy(energymon* impl) {
  if (impl == NULL) {
    return -1;
  }
  impl->finit = &energymon_init_dummy;
  impl->fread = &energymon_read_total_dummy;
  impl->ffinish = &energymon_finish_dummy;
  impl->fsource = &energymon_get_source_dummy;
  impl->finterval = &energymon_get_interval_dummy;
  impl->state = NULL;
  return 0;
}
