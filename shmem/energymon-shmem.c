/**
 * Read energy from a shared memory location.
 *
 * @author Connor Imes
 * @date 2016-02-10
 */

#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "energymon.h"
#include "energymon-shmem.h"
#include "energymon-util.h"

#ifdef ENERGYMON_DEFAULT
#include "energymon-default.h"
int energymon_get_default(energymon* em) {
  return energymon_get_shmem(em);
}
#endif

int energymon_init_shmem(energymon* em) {
  if (em == NULL || em->state != NULL) {
    errno = EINVAL;
    return -1;
  }

  const char* key_dir;
  int key_proj_id = ENERGYMON_SHMEM_ID_DEFAULT;
  const char* key_proj_id_env;
  key_t mem_key;
  int shm_id;
  energymon_shmem* ems;

  // get desired configuration from environment
  key_dir = getenv(ENERGYMON_SHMEM_DIR);
  if (key_dir == NULL) {
    key_dir = ENERGYMON_SHMEM_DIR_DEFAULT;
  }
  key_proj_id_env = getenv(ENERGYMON_SHMEM_ID);
  if (key_proj_id_env != NULL) {
    key_proj_id = atoi(key_proj_id_env);
  }

  // attach to shared memory
  mem_key = ftok(key_dir, key_proj_id);
  shm_id = shmget(mem_key, sizeof(energymon_shmem), 0444);
  if (shm_id < 0) {
    // among other reasons, fails if nobody is providing this shared memory
    return -1;
  }
  ems = (energymon_shmem*) shmat(shm_id, NULL, SHM_RDONLY);
  if (ems == (energymon_shmem*) -1) {
    return -1;
  }

  em->state = ems;
  return 0;
}

uint64_t energymon_read_total_shmem(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  return ((energymon_shmem*) em->state)->energy_uj;
}

int energymon_finish_shmem(energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return -1;
  }
  energymon_shmem* state = (energymon_shmem*) em->state;
  em->state = NULL;
  // detach from shared memory
  return shmdt(state);
}

char* energymon_get_source_shmem(char* buffer, size_t n) {
  return energymon_strencpy(buffer, "Shared Memory", n);
}

uint64_t energymon_get_interval_shmem(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  return ((energymon_shmem*) em->state)->interval_us;
}

uint64_t energymon_get_precision_shmem(const energymon* em) {
  if (em == NULL || em->state == NULL) {
    errno = EINVAL;
    return 0;
  }
  return ((energymon_shmem*) em->state)->precision_uj;
}

int energymon_is_exclusive_shmem(void) {
  return 0;
}

int energymon_get_shmem(energymon* em) {
  if (em == NULL) {
    errno = EINVAL;
    return -1;
  }
  em->finit = &energymon_init_shmem;
  em->fread = &energymon_read_total_shmem;
  em->ffinish = &energymon_finish_shmem;
  em->fsource = &energymon_get_source_shmem;
  em->finterval = &energymon_get_interval_shmem;
  em->fprecision = &energymon_get_precision_shmem;
  em->fexclusive = &energymon_is_exclusive_shmem;
  em->state = NULL;
  return 0;
}
