/**
 * Dummy em implementation - doesn't read from any source.
 *
 * @author Connor Imes
 * @date 2014-07-30
 */

#include "em-generic.h"
#include "em-dummy.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef EM_GENERIC
int em_init(void) {
  return em_init_dummy();
}

double em_read_total(int64_t last_time, int64_t curr_time) {
  return em_read_total_dummy(last_time, curr_time);
}

int em_finish(void) {
  return em_finish_dummy();
}

char* em_get_source(char* buffer) {
  return em_get_source_dummy(buffer);
}

int em_impl_get(em_impl* impl) {
  return em_impl_get_dummy(impl);
}
#endif

int em_init_dummy(void) {
  return 0;
}

double em_read_total_dummy(int64_t last_time, int64_t curr_time) {
  return 0.0;
}

int em_finish_dummy(void) {
  return 0;
}

char* em_get_source_dummy(char* buffer) {
  return strcpy(buffer, "Dummy Source");
}

int em_impl_get_dummy(em_impl* impl) {
  if (impl != NULL) {
      impl->finit = &em_init_dummy;
      impl->fread = &em_read_total_dummy;
      impl->ffinish = &em_finish_dummy;
      impl->fsource = &em_get_source_dummy;
      return 0;
  }
  return 1;
}
