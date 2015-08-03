/**
 * Dummy implementation - doesn't read from any source.
 *
 * @author Connor Imes
 * @date 2014-07-30
 */

#include "energymon.h"
#include "em-dummy.h"
#include <stdlib.h>
#include <string.h>

typedef struct energymon_dummy {
  unsigned long long energy;
} energymon_dummy;

#ifdef EM_DEFAULT
int energymon_get_default(energymon* impl) {
  return energymon_get_dummy(impl);
}
#endif

int energymon_init_dummy(energymon* impl) {
  if (impl == NULL || impl->state != NULL) {
    return -1;
  }

  impl->state = malloc(sizeof(energymon_dummy));
  if (impl->state == NULL) {
    return -1;
  }
  ((energymon_dummy*) impl->state)->energy = 0;
  return 0;
}

unsigned long long energymon_read_total_dummy(const energymon* impl) {
  if (impl == NULL || impl->state == NULL) {
    return 0;
  }
  return ((energymon_dummy*) impl->state)->energy;
}

int energymon_finish_dummy(energymon* impl) {
  if (impl == NULL || impl->state == NULL) {
    return -1;
  }
  free(impl->state);
  return 0;
}

char* energymon_get_source_dummy(char* buffer) {
  if (buffer == NULL) {
    return NULL;
  }
  return strcpy(buffer, "Dummy Source");
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
