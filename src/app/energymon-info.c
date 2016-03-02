/**
 * Print energymon implementation info.
 *
 * @author Connor Imes
 * @date 2016-03-02
 */
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include "energymon-default.h"

int main(int argc, char** argv) {
  energymon em;
  char buf[256];
  int ret;
  uint64_t reading = 0;

  // silence compiler warnings
  (void) argc;
  (void) argv;

  if (energymon_get_default(&em)) {
    perror("energymon_get_default");
    exit(1);
  }

  em.fsource(buf, sizeof(buf));

  // initialize
  if ((ret = em.finit(&em))) {
    perror("energymon:finit");
  }

  if (!ret) {
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
