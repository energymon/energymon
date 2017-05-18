/**
 * Read energy from a shared memory location.
 *
 * @author Connor Imes
 * @date 2016-02-10
 */
#ifndef _ENERGYMON_SHMEM_H_
#define _ENERGYMON_SHMEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "energymon.h"

#define ENERGYMON_SHMEM_DIR "ENERGYMON_SHMEM_DIR"
#define ENERGYMON_SHMEM_DIR_DEFAULT "."
#define ENERGYMON_SHMEM_ID "ENERGYMON_SHMEM_ID"
#define ENERGYMON_SHMEM_ID_DEFAULT 1

typedef struct energymon_shmem {
  volatile uint64_t interval_us;
  volatile uint64_t precision_uj;
  volatile uint64_t energy_uj;
} energymon_shmem;

int energymon_init_shmem(energymon* em);

uint64_t energymon_read_total_shmem(const energymon* em);

int energymon_finish_shmem(energymon* em);

char* energymon_get_source_shmem(char* buffer, size_t n);

uint64_t energymon_get_interval_shmem(const energymon* em);

uint64_t energymon_get_precision_shmem(const energymon* em);

int energymon_is_exclusive_shmem(void);

int energymon_get_shmem(energymon* em);

#ifdef __cplusplus
}
#endif

#endif
