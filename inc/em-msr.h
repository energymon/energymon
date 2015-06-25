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

#include "em-generic.h"
#include <inttypes.h>

/* Environment variable for specifying the MSRs to use */
#define EM_MSR_ENV_VAR "EM_MSRS"
#define EM_MSRS_DELIMS ", :;|"

int em_init_msr(void);

double em_read_total_msr(int64_t last_hb_time, int64_t curr_hb_time);

int em_finish_msr(void);

char* em_get_source_msr(void);

em_impl* em_impl_alloc_msr(void);

#ifdef __cplusplus
}
#endif

#endif
