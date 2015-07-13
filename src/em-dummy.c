/**
 * Dummy em implementation - doesn't read from any source.
 *
 * @author Connor Imes
 * @date 2014-07-30
 */

#include "energymon.h"
#include "em-dummy.h"
#include <stdlib.h>
#include <string.h>

typedef struct em_dummy {
  long long energy;
} em_dummy;

#ifdef EM_DEFAULT
int em_impl_get(em_impl* impl) {
  return em_impl_get_dummy(impl);
}
#endif

int em_init_dummy(em_impl* impl) {
  if (impl == NULL || impl->state != NULL) {
    return -1;
  }

  impl->state = malloc(sizeof(em_dummy));
  if (impl->state == NULL) {
    return -1;
  }
  ((em_dummy*) impl->state)->energy = 0;
  return 0;
}

long long em_read_total_dummy(const em_impl* impl) {
  if (impl == NULL || impl->state == NULL) {
    return -1;
  }
  return ((em_dummy*) impl->state)->energy;
}

int em_finish_dummy(em_impl* impl) {
  if (impl == NULL || impl->state == NULL) {
    return -1;
  }
  free(impl->state);
  return 0;
}

char* em_get_source_dummy(char* buffer) {
  if (buffer == NULL) {
    return NULL;
  }
  return strcpy(buffer, "Dummy Source");
}

unsigned long long em_get_interval_dummy(const em_impl* em) {
  return 1;
}

int em_impl_get_dummy(em_impl* impl) {
  if (impl == NULL) {
    return -1;
  }
  impl->finit = &em_init_dummy;
  impl->fread = &em_read_total_dummy;
  impl->ffinish = &em_finish_dummy;
  impl->fsource = &em_get_source_dummy;
  impl->finterval = &em_get_interval_dummy;
  impl->state = NULL;
  return 0;
}
