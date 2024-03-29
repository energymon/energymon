/**
 * Calculate system idle power using energymon-default.
 *
 * @author Connor Imes
 * @date 2016-02-15
 */
#define _GNU_SOURCE
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "energymon.h"
#include "energymon-get.h"
#include "energymon-time-util.h"

#ifndef ENERGYMON_UTIL_PREFIX
#define ENERGYMON_UTIL_PREFIX "energymon"
#endif

static const uint64_t DEFAULT_SLEEP_US = 10000000; // 10 seconds
static const int IGNORE_INTERRUPT = 0;

static const char short_options[] = "+h";
static const struct option long_options[] = {
  {"help",      no_argument,       NULL, 'h'},
  {0, 0, 0, 0}
};

__attribute__ ((noreturn))
static void print_usage(int exit_code) {
  fprintf(exit_code ? stderr : stdout,
          "Usage: "ENERGYMON_UTIL_PREFIX"-idle-power [OPTION]... [SECONDS]\n\n"
          "Intended to measure the idle power of the system by doing nothing. In fact, it\n"
          "just measures the average power during the SECONDS specified (%u by default),\n"
          "regardless of whether the system is actually idle.\n\n"
          "Options:\n"
          "  -h, --help               Print this message and exit\n",
          (unsigned int)(DEFAULT_SLEEP_US / 1000000));
  exit(exit_code);
}

int main(int argc, char** argv) {
  energymon em;
  double seconds;
  uint64_t sleep_us = DEFAULT_SLEEP_US;
  uint64_t time_start_ns;
  uint64_t time_end_ns;
  uint64_t energy_start_uj;
  uint64_t energy_end_uj;
  double watts;
  int c;
  while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
    switch (c) {
      case 'h':
        print_usage(0);
        break;
      case '?':
      default:
        print_usage(1);
        break;
    }
  }
  if (optind < argc) {
    if ((seconds = atof(argv[optind])) < 0) {
      fprintf(stderr, "SECONDS value must be > 0, but got: %f\n", seconds);
      exit(1);
    }
    sleep_us = (uint64_t) (seconds * 1000000.0);
  }

  // initialize
  if (energymon_get(&em)) {
    exit(1);
  }
  if (em.finit(&em)) {
    perror("energymon:finit");
    exit(1);
  }

  errno = 0;
  energy_start_uj = em.fread(&em);
  if (!energy_start_uj && errno) {
    perror("energymon:fread");
    em.ffinish(&em);
    exit(1);
  }
  time_start_ns = energymon_gettime_ns();
  if (energymon_sleep_us(sleep_us, &IGNORE_INTERRUPT)) {
    perror("energymon_sleep_us");
    em.ffinish(&em);
    exit(1);
  }
  errno = 0;
  energy_end_uj = em.fread(&em);
  if (!energy_end_uj && errno) {
    perror("energymon:fread");
    em.ffinish(&em);
    exit(1);
  }
  time_end_ns = energymon_gettime_ns();

  watts = (time_end_ns - time_start_ns) > 0
          ? (energy_end_uj - energy_start_uj) * 1000.0 / (time_end_ns - time_start_ns)
          : 0;
  fprintf(stdout, "%f\n", watts);

  // cleanup
  if (em.ffinish(&em)) {
    perror("energymon:ffinish");
    // still return 0 since we completed our primary task
  }

  return 0;
}
