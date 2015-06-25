/**
 * Energy reading for an ODROID with INA231 power sensors.
 *
 * @author Connor Imes
 * @date 2014-06-30
 */
#ifndef _EM_ODROID_H_
#define _EM_ODROID_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "em-generic.h"
#include <inttypes.h>

int em_init_odroid(void);

double em_read_total_odroid(void);

int em_finish_odroid(void);

char* em_get_source_odroid(char* buffer);

int em_impl_get_odroid(em_impl* impl);

#ifdef __cplusplus
}
#endif

#endif
