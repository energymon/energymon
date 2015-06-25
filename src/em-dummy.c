/**
 * Dummy em implementation - doesn't read from any source.
 *
 * @author Connor Imes
 * @date 2014-07-30
 */

#include "em-generic.h"
#include "em-dummy.h"
#include <stdlib.h>
#include <inttypes.h>

#ifdef EM_GENERIC
int em_init(void) {
  return em_init_dummy();
}

double em_read_total(int64_t last_hb_time, int64_t curr_hb_time) {
  return em_read_total_dummy(last_hb_time, curr_hb_time);
}

int em_finish(void) {
  return em_finish_dummy();
}

char* em_get_source(void) {
  return em_get_source_dummy();
}

em_impl* em_impl_alloc(void) {
  return em_impl_alloc_dummy();
}
#endif

int em_init_dummy(void) {
  return 0;
}

double em_read_total_dummy(int64_t last_hb_time, int64_t curr_hb_time) {
  return 0.0;
}

int em_finish_dummy(void) {
  return 0;
}

char* em_get_source_dummy(void) {
  return "Dummy Source";
}

em_impl* em_impl_alloc_dummy(void) {
  em_impl* hei = (em_impl*) malloc(sizeof(em_impl));
  hei->finit = &em_init_dummy;
  hei->fread = &em_read_total_dummy;
  hei->ffinish = &em_finish_dummy;
  hei->fsource = &em_get_source_dummy;
  return hei;
}
