/**
 * Dummy implementation - doesn't read from any source.
 *
 * @author Connor Imes
 * @date 2014-07-30
 */
#ifndef _EM_DUMMY_H_
#define _EM_DUMMY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "energymon.h"

int em_init_dummy(em_impl* em);

unsigned long long em_read_total_dummy(const em_impl* em);

int em_finish_dummy(em_impl* em);

char* em_get_source_dummy(char* buffer);

unsigned long long em_get_interval_dummy(const em_impl* em);

int em_impl_get_dummy(em_impl* impl);

#ifdef __cplusplus
}
#endif

#endif
