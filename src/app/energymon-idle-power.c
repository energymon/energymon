/**
 * Calculate system idle power using energymon-default.
 *
 * @author Connor Imes
 * @date 2016-02-15
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "energymon-default.h"
#include "energymon-time-util.h"

static const int DEFAULT_SLEEP_US = 10000000; // 10 seconds
static const int IGNORE_INTERRUPT = 0;

/**
 * The only argument allowed is an integer value for the number of seconds to
 * sleep for.
 */
int main(int argc, char** argv) {
  int sleep_us = DEFAULT_SLEEP_US;
  uint64_t time_start_ns;
  uint64_t time_end_ns;
  uint64_t energy_start_uj;
  uint64_t energy_end_uj;
  double watts;
  energymon em;

  if (argc > 1) {
    sleep_us = atoi(argv[1]) * 1000000;
    if (sleep_us <= 0) {
      fprintf(stderr, "Sleep time must be > 0 seconds.\n");
      exit(1);
    }
  }

  if (energymon_get_default(&em)) {
    perror("energymon_get_default");
    exit(1);
  }

  // initialize
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

  watts = (energy_end_uj - energy_start_uj) * 1000.0 / (time_end_ns - time_start_ns);
  fprintf(stdout, "%f\n", watts);

  // cleanup
  if (em.ffinish(&em)) {
    perror("energymon:ffinish");
    // still return 0 since we completed our primary task
  }

  return 0;
}
