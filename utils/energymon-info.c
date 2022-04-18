/**
 * Print energymon implementation info.
 *
 * @author Connor Imes
 * @date 2016-03-02
 */
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "energymon.h"
#include "energymon-get.h"

#ifndef ENERGYMON_UTIL_PREFIX
#define ENERGYMON_UTIL_PREFIX "energymon"
#endif

static const char short_options[] = "+h";
static const struct option long_options[] = {
  {"help",      no_argument,       NULL, 'h'},
  {0, 0, 0, 0}
};

__attribute__ ((noreturn))
static void print_usage(int exit_code) {
  fprintf(exit_code ? stderr : stdout,
          "Usage: "ENERGYMON_UTIL_PREFIX"-info [OPTION]...\n\n"
          "Prints information from the EnergyMon interface functions, including source\n"
          "name, exclusivity, refresh interval, energy reading precision, and a current\n"
          "energy value.\n\n"
          "Even if the EnergyMon implementation fails to initialize, the program will\n"
          "attempt to read from as many functions as possible.\n\n"
          "Options:\n"
          "  -h, --help               Print this message and exit\n");
  exit(exit_code);
}

int main(int argc, char** argv) {
  char buf[256] = { 0 };
  energymon em;
  uint64_t reading = 0;
  int ret;
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

  if (energymon_get(&em)) {
    exit(1);
  }

  em.fsource(buf, sizeof(buf));

  // initialize
  if ((ret = em.finit(&em))) {
    // don't quit, we can still try other functions (except fread)
    perror("energymon:finit");
  } else {
    errno = 0;
    reading = em.fread(&em);
    if (!reading && errno) {
      perror("energymon:fread");
    }
  }

  printf("source: %s\n", buf);
  printf("exclusive: %s\n", em.fexclusive() ? "true" : "false");
  printf("interval (usec): %"PRIu64"\n", em.finterval(&em));
  printf("precision (uJ): %"PRIu64"\n", em.fprecision(&em));
  printf("reading (uJ): %"PRIu64 "\n", reading);
  
  // cleanup
  if (!ret && em.ffinish(&em)) {
    perror("energymon:ffinish");
  }

  return ret;
}
