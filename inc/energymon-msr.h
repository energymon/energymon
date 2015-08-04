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
#ifndef _ENERGYMON_MSR_H_
#define _ENERGYMON_MSR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "energymon.h"

/* Environment variable for specifying the MSRs to use */
#define EM_MSR_ENV_VAR "EM_MSRS"
#define EM_MSRS_DELIMS ", :;|"

int energymon_init_msr(energymon* impl);

unsigned long long energymon_read_total_msr(const energymon* impl);

int energymon_finish_msr(energymon* impl);

char* energymon_get_source_msr(char* buffer);

unsigned long long energymon_get_interval_msr(const energymon* em);

int energymon_get_msr(energymon* impl);

#ifdef __cplusplus
}
#endif

#endif
