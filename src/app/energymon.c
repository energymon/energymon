/**
 * Polls an energymon at the provided interval and writes the output to the
 * file specified.
 *
 * @author Connor Imes
 * @date 2015-07-09
 */
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "energymon.h"

// set a minimum sleep time between polls
#ifndef ENERGYMON_MIN_INTERVAL_US
  #define ENERGYMON_MIN_INTERVAL_US 1000
#endif

static volatile int running = 1;

static void print_usage(const char* app) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  %s <output_file>\n", app);
}

void shandle(int dummy) {
  running = 0;
}

int main(int argc, char** argv) {
  em_impl impl;
  unsigned long long us;
  struct timespec ts;
  long long energy;
  FILE* fout;
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  // register the signal handler
  signal(SIGINT, shandle);

  // open the output file
  fout = fopen(argv[1], "wb");
  if (fout == NULL) {
    perror("Failed to open output file");
    return 1;
  }

  // initialize the energy monitor
  if (em_impl_get(&impl) || impl.finit(&impl)) {
    fclose(fout);
    return 1;
  }

  // get the update interval
  us = impl.finterval(&impl);
  if (us < ENERGYMON_MIN_INTERVAL_US) {
    us = ENERGYMON_MIN_INTERVAL_US;
  }
  ts.tv_sec = us / 1000000;
  ts.tv_nsec = (us % 1000000) * 1000;

  // update file at regular intervals
  while (running) {
    energy = impl.fread(&impl);
    rewind(fout);
    if(fprintf(fout, "%lld\n", energy) < 0) {
      perror("Writing to output file");
      break;
    }
    fflush(fout);
    nanosleep(&ts, NULL);
  }

  // cleanup
  fclose(fout);
  if (impl.ffinish(&impl)) {
    return 1;
  }

  return 0;
}
