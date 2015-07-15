/**
 * Read energy from X86 MSRs (Model-Specific Registers).
 *
 * By default, the MSR on cpu0 is read. To configure other MSRs, set the
 * EM_MSRS environment variable with a comma-delimited list of CPU IDs, e.g.:
 *   export EM_MSRS=0,4,8,12
 *
 * @author Hank Hoffmann
 * @author Connor Imes
 */
#ifndef _EM_MSR_H_
#define _EM_MSR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "energymon.h"

/* Environment variable for specifying the MSRs to use */
#define EM_MSR_ENV_VAR "EM_MSRS"
#define EM_MSRS_DELIMS ", :;|"

int em_init_msr(em_impl* impl);

unsigned long long em_read_total_msr(const em_impl* impl);

int em_finish_msr(em_impl* impl);

char* em_get_source_msr(char* buffer);

unsigned long long em_get_interval_msr(const em_impl* em);

int em_impl_get_msr(em_impl* impl);

#ifdef __cplusplus
}
#endif

#endif
