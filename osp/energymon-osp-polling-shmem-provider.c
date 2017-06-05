/**
 * Use the energymon-osp-polling implementation and provide results over shared
 * memory.
 *
 * @author Connor Imes
 * @date 2016-02-18
 */
#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include "energymon-osp-polling.h"
#include "../shmem/energymon-shmem.h"

static volatile int running = 1;
static energymon_shmem* ems;
static const char* key_dir = NULL;
static int key_proj_id = -1;
static int shm_id;

static void print_usage(const char* name, int exit_code) {
  printf("Usage: %s [OPTIONS]\n", name);
  printf("  -d --dir      The shared memory directory\n");
  printf("                default = \"%s\"\n", ENERGYMON_SHMEM_DIR_DEFAULT);
  printf("  -i --id       The shared memory identifier\n");
  printf("                default = %d\n", ENERGYMON_SHMEM_ID_DEFAULT);
  printf("  -h, --help    Print this message\n");
  printf("\n");
  exit(exit_code);
}

static void parse_args(int argc, char** argv) {
  int i;
  const char* key_proj_id_env;
  for (i = 1; i < argc; i++) {
    if (!strcmp("-h", argv[i]) || !strcmp("--help", argv[i])) {
      print_usage(argv[0], 0);
    } else if (!strcmp("-d", argv[i]) || !strcmp("--dir", argv[i])) {
      if (++i == argc) {
        print_usage(argv[0], EINVAL);
      }
      key_dir = argv[i];
    } else if (!strcmp("-i", argv[i]) || !strcmp("--id", argv[i])) {
      if (++i == argc) {
        print_usage(argv[0], EINVAL);
      }
      key_proj_id = atoi(argv[i]);
      if (key_proj_id < 0) {
        print_usage(argv[0], EINVAL);
      }
    } else {
      print_usage(argv[0], EINVAL);
    }
  }
  if (key_dir == NULL) {
    key_dir = getenv(ENERGYMON_SHMEM_DIR);
    if (key_dir == NULL) {
      key_dir = ENERGYMON_SHMEM_DIR_DEFAULT;
    }
  }
  if (key_proj_id < 0) {
    key_proj_id_env = getenv(ENERGYMON_SHMEM_ID);
    if (key_proj_id_env == NULL) {
      key_proj_id = ENERGYMON_SHMEM_ID_DEFAULT;
    } else {
      key_proj_id = atoi(key_proj_id_env);
    }
  }
}

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

static int cleanup_shmem(void) {
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

int main(int argc, char** argv) {
  energymon em;
  struct timespec ts;
  key_t mem_key;

  // register the signal handler
  signal(SIGINT, shandle);

  parse_args(argc, argv);

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

  cleanup_shmem();

  return errno;
}
