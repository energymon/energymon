/**
 * Polls an energymon at the provided interval and writes the output to the
 * file specified.
 *
 * @author Connor Imes
 * @date 2015-07-09
 */
#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "energymon-default.h"
#include "energymon-time-util.h"

// set a minimum sleep time between polls
#ifndef ENERGYMON_MIN_INTERVAL_US
  #define ENERGYMON_MIN_INTERVAL_US 100000
#endif

static volatile int running = 1;

static void print_usage(const char* app) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  %s <output_file>\n", app);
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

int main(int argc, char** argv) {
  energymon em;
  uint64_t us;
  uint64_t energy;
  FILE* fout;
  int ret = 0;
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  // register the signal handler
  signal(SIGINT, shandle);

  // initialize the energy monitor
  if (energymon_get_default(&em) || em.finit(&em)) {
    return 1;
  }

  // open the output file
  fout = fopen(argv[1], "w+");
  if (fout == NULL) {
    perror("Failed to open output file");
    em.ffinish(&em);
    return 1;
  }

  // get the update interval
  us = em.finterval(&em);
  if (us < ENERGYMON_MIN_INTERVAL_US) {
    us = ENERGYMON_MIN_INTERVAL_US;
  }

  // update file at regular intervals
  while (running) {
    energy = em.fread(&em);
    rewind(fout);
    if (fprintf(fout, "%"PRIu64"\n", energy) < 0) {
      ret = 1;
      perror("Writing to output file");
      break;
    }
    fflush(fout);
    energymon_sleep_us(us, &running);
  }

  // cleanup
  if (fclose(fout)) {
    ret = 1;
  }
  if (em.ffinish(&em)) {
    ret = 1;
  }

  return ret;
}
