/**
 * Provide energymon readings over shared memory.
 *
 * @author Connor Imes
 * @date 2016-02-18
 */
#define _GNU_SOURCE
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include "energymon.h"
#include "energymon-get.h"
#include "energymon-shmem.h"

#ifndef ENERGYMON_UTIL_PREFIX
#error Must set ENERGYMON_UTIL_PREFIX
#endif

static volatile int running = 1;
static energymon_shmem* ems;
static const char* key_dir = NULL;
static int key_proj_id = -1;
static int shm_id;

static const char short_options[] = "hd:i:";
static const struct option long_options[] = {
  {"help",      no_argument,       NULL, 'h'},
  {"dir",       required_argument, NULL, 'd'},
  {"id",        required_argument, NULL, 'i'},
  {0, 0, 0, 0}
};

static void print_usage(int exit_code) {
  fprintf(exit_code ? stderr : stdout,
          "Usage: "ENERGYMON_UTIL_PREFIX"-shmem-provider [OPTION]...\n\n"
          "Provide EnergyMon readings over shared memory, e.g., for use by\n"
          "libenergymon-shmem.\n\n"
          "Both the provider and the consumer must agree on the path and id used to create\n"
          "the IPC shared memory key, as required by ftok(3).\n\n"
          "If the consumer is libenergymon-shmem, the shared memory provider must be\n"
          "running before the energymon context is initialized. To specify path and id\n"
          "values for libenergymon-shmem, set the ENERGYMON_SHMEM_DIR and\n"
          "ENERGYMON_SHMEM_ID environment variables, respectively.\n\n"
          "Options:\n"
          "  -h, --help               Print this message and exit\n"
          "  -d, --dir=PATH           The shared memory path (default = \"%s\")\n"
          "  -i, --id=ID              The shared memory identifier (default = %d)\n"
          "                           ID must be in range [1, 255]\n",
          ENERGYMON_SHMEM_DIR_DEFAULT, ENERGYMON_SHMEM_ID_DEFAULT);
  exit(exit_code);
}

static void enforce_key_proj_id(void) {
  if (key_proj_id <= 0 || key_proj_id > 255) {
    print_usage(EINVAL);
  }
}

static void parse_args(int argc, char** argv) {
  const char* key_proj_id_env;
  int c;
  while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
    switch (c) {
      case 'h':
        print_usage(0);
        break;
      case 'd':
        key_dir = optarg;
        break;
      case 'i':
        key_proj_id = atoi(optarg);
        enforce_key_proj_id();
        break;
      case '?':
      default:
        print_usage(EINVAL);
        break;
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
      enforce_key_proj_id();
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

  // get the energy monitor
  if (energymon_get(&em)) {
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
  ts.tv_sec = (time_t) (ems->interval_us / (uint64_t) 1000000);
  ts.tv_nsec = (long) ((ems->interval_us % (uint64_t) 1000000) * (uint64_t) 1000);

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
