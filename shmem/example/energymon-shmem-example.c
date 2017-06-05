/**
 * This example application starts an energymon and makes its interval and
 * energy data available over shared memory.
 *
 * This is only an example as it doesn't make sense for the shared mem provider
 * to get the default energymon implementation if interested applications are
 * going to do the same. The default implementation should be energymon-shmem
 * with the standalone binary providing energy data for a predetermined data
 * source.
 *
 * @author Connor Imes
 * @date 2016-02-10
 */
#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include "energymon-default.h"
#include "../energymon-shmem.h"

static volatile int running = 1;

static void shandle(int sig) {
  switch (sig) {
    case SIGTERM:
    case SIGINT:
#ifdef SIGQUIT
    case SIGQUIT:
#endif
#ifdef SIGKILL
    case SIGKILL:
#endif
#ifdef SIGHUP
    case SIGHUP:
#endif
      running = 0;
    default:
      break;
  }
}

int main(void) {
  energymon em;
  energymon_shmem* ems;
  struct timespec ts;
  const char* key_proj_id_env;
  const char* key_dir;
  int key_proj_id = ENERGYMON_SHMEM_ID_DEFAULT;
  key_t mem_key;
  int shm_id;

  // register the signal handler
  signal(SIGINT, shandle);

  key_dir = getenv(ENERGYMON_SHMEM_DIR);
  if (key_dir == NULL) {
    key_dir = ENERGYMON_SHMEM_DIR_DEFAULT;
  }
  key_proj_id_env = getenv(ENERGYMON_SHMEM_ID);
  if (key_proj_id_env != NULL) {
    key_proj_id = atoi(key_proj_id_env);
  }

  // get the shared memory
  mem_key = ftok(key_dir, key_proj_id);
  shm_id = shmget(mem_key, sizeof(energymon_shmem), 0644 | IPC_CREAT | IPC_EXCL);
  if (shm_id < 0) {
    perror("shmget");
    return -errno;
  }
  ems = (energymon_shmem*) shmat(shm_id, NULL, 0);
  if (ems == (energymon_shmem*) -1) {
    perror("shmat");
    return -errno;
  }

  // get the local energy monitor
  if (energymon_get_default(&em)) {
    perror("energymon_get_default");
    return -errno;
  }

  // initialize the energy monitor
  if (em.finit(&em)) {
    perror("energymon:finit");
    return -errno;
  }
  
  // store the interval in shared memory
  ems->interval_us = em.finterval(&em);
  ts.tv_sec = ems->interval_us / 1000000;
  ts.tv_nsec = (ems->interval_us % 1000000) * 1000;

  // store the precision in shared memory
  ems->precision_uj = em.fprecision(&em);

  while (running) {
    // update the energy in shared memory
    ems->energy_uj = em.fread(&em);
    nanosleep(&ts, NULL);
  }

  errno = 0;
  // cleanup
  if (em.ffinish(&em)) {
    perror("energymon:ffinish");
  }

  // detach shared memory
  if (shmdt(ems)) {
    perror("shmdt");
  }

  // destroy shared memory
  if (shmctl(shm_id, IPC_RMID, NULL)) {
    perror("shmctl");
  }

  return errno;
}
