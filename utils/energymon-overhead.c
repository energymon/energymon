/**
 * Test the overhead of the energymon-default implementation.
 * Prints out the time in nanoseconds for finit, fread, and ffinish functions.
 *
 * @author Connor Imes
 * @date 2016-02-15
 */
#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "energymon.h"
#include "energymon-get.h"
#include "energymon-time-util.h"

int main(void) {
  char source[64] = { 0 };
  static energymon em;
  uint64_t time_start_ns, time_end_ns;
  uint64_t finit_ns, fread_ns, ffinish_ns;
  uint64_t energy_uj;
  int ret;

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
