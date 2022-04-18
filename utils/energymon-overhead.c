/**
 * Test the overhead of the energymon-default implementation.
 * Prints out the time in nanoseconds for finit, fread, and ffinish functions.
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

static const char short_options[] = "+h";
static const struct option long_options[] = {
  {"help",      no_argument,       NULL, 'h'},
  {0, 0, 0, 0}
};

__attribute__ ((noreturn))
static void print_usage(int exit_code) {
  fprintf(exit_code ? stderr : stdout,
          "Usage: "ENERGYMON_UTIL_PREFIX"-overhead [OPTION]...\n\n"
          "Measure the overhead of the init, read, and finish functions. Results are in\n"
          "nanoseconds.\n\n"
          "Note that overhead readings can only be as precise as the system clock supports.\n\n"
          "Options:\n"
          "  -h, --help               Print this message and exit\n");
  exit(exit_code);
}

int main(int argc, char** argv) {
  char source[64] = { 0 };
  static energymon em;
  uint64_t time_start_ns, time_end_ns;
  uint64_t finit_ns, fread_ns, ffinish_ns;
  uint64_t energy_uj;
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

  // no need to profile getting the energymon
  if (energymon_get(&em)) {
    exit(1);
  }

  if (em.fsource(source, sizeof(source)) == NULL) {
    perror("energymon:fsource");
    exit(1);
  }

  // init
  time_start_ns = energymon_gettime_ns();
  ret = em.finit(&em);
  time_end_ns = energymon_gettime_ns();
  finit_ns = time_end_ns - time_start_ns;
  if (ret) {
    perror("energmon:finit");
    exit(1);
  }

  // read
  errno = 0;
  time_start_ns = energymon_gettime_ns();
  energy_uj = em.fread(&em);
  time_end_ns = energymon_gettime_ns();
  fread_ns = time_end_ns - time_start_ns;
  if (!energy_uj && errno) {
    perror("energymon:fread");
    em.ffinish(&em);
    exit(1);
  }

  // finish
  time_start_ns = energymon_gettime_ns();
  ret = em.ffinish(&em);
  time_end_ns = energymon_gettime_ns();
  ffinish_ns = time_end_ns - time_start_ns;
  if (ret) {
    perror("energmon:ffinish");
    exit(1);
  }

  fprintf(stdout, "%s\nfinit: %"PRIu64"\nfread: %"PRIu64"\nffinish: %"PRIu64"\n",
                  source, finit_ns, fread_ns, ffinish_ns);

  return 0;
}
