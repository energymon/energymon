/**
 * Use the energymon-osp-polling implementation and provide results over shared
 * memory.
 *
 * @author Connor Imes
 * @date 2016-02-18
 */
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include "energymon-osp-polling.h"
#include "../shmem/energymon-shmem.h"

static volatile int running = 1;
static energymon_shmem* ems;
static int shm_id;

void shandle(int sig) {
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

int cleanup_shmem() {
  // detach shared memory
  if (shmdt(ems)) {
    perror("shmdt");
    return -errno;
  }
  // destroy shared memory
  if (shmctl(shm_id, IPC_RMID, NULL)) {
    perror("shmctl");
    return -errno;
  }
  return 0;
}

int main() {
  energymon em;
  struct timespec ts;
  const char* key_proj_id_env;
  const char* key_dir;
  int key_proj_id = ENERGYMON_SHMEM_ID_DEFAULT;
  key_t mem_key;

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
    cleanup_shmem();
    return -errno;
  }

  // get the osp-polling energy monitor
  if (energymon_get_osp_polling(&em)) {
    perror("energymon_get_osp_polling");
    cleanup_shmem();
    return -errno;
  }

  // initialize the energy monitor
  if (em.finit(&em)) {
    perror("energymon:finit");
    cleanup_shmem();
    return -errno;
  }
  
  // store the interval in shared memory
  ems->interval_us = em.finterval(&em);
  ts.tv_sec = ems->interval_us / 1000000;
  ts.tv_nsec = (ems->interval_us % 1000000) * 1000;

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

  cleanup_shmem();

  return errno;
}
