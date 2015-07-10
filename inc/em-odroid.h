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

#include "energymon.h"

int em_init_odroid(em_impl* em);

long long em_read_total_odroid(em_impl* em);

int em_finish_odroid(em_impl* em);

char* em_get_source_odroid(char* buffer);

unsigned long long em_get_interval_odroid(const em_impl* em);

int em_impl_get_odroid(em_impl* impl);

#ifdef __cplusplus
}
#endif

#endif
