/**
 * Print energymon implementation info.
 *
 * @author Connor Imes
 * @date 2016-03-02
 */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "energymon-default.h"

int main(void) {
  char buf[256] = { 0 };
  energymon em;
  uint64_t reading = 0;
  int ret;

  if (energymon_get_default(&em)) {
    perror("energymon_get_default");
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
