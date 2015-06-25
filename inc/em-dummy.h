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

#include "em-generic.h"
#include <inttypes.h>

int em_init_dummy(void);

double em_read_total_dummy(int64_t last_time, int64_t curr_time);

int em_finish_dummy(void);

char* em_get_source_dummy(void);

int em_impl_get_dummy(em_impl* impl);

#ifdef __cplusplus
}
#endif

#endif
