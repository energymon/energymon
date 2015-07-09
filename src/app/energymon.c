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

static volatile int running = 1;

static void print_usage(const char* app) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  %s <output_file> <poll_delay_us>\n", app);
}

void shandle(int dummy) {
  running = 0;
}

static int get_poll_delay(struct timespec* ts, char* us) {
  unsigned long long sleep_us = strtoull(us, NULL, 0);
  if (sleep_us == 0ULL) {
    fprintf(stderr, "Poll delay must be > 0\n");
    return 1;
  }
  if (sleep_us == ULLONG_MAX) {
    perror("Poll delay is too long");
    return 1;
  }
  ts->tv_sec = sleep_us / 1000000;
  ts->tv_nsec = (sleep_us % 1000000) * 1000;
  return 0;
}

int main(int argc, char** argv) {
  em_impl impl;
  struct timespec ts;
  long long energy;
  FILE* fout;
  if (argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  // register the signal handler
  signal(SIGINT, shandle);

  // get the polling interval
  if (get_poll_delay(&ts, argv[2])) {
    return 1;
  }

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
