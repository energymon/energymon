#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include "energymon-default.h"

int main(void) {
  energymon em;
  char source[100] = { '\0' };
  uint64_t result;
  uint64_t interval;
  uint64_t precision;
  int exclusive;

  if (energymon_get_default(&em)) {
    perror("energymon_get_default");
    return 1;
  }

  em.fsource(source, sizeof(source));
  printf("Initializing reading from %s\n", source);

  if (em.finit(&em)) {
    perror("finit");
    return 1;
  }

  interval = em.finterval(&em);
  printf("Interval: %"PRIu64" usec\n", interval);
  precision = em.fprecision(&em);
  printf("Precision: %"PRIu64" uJ\n", precision);
  exclusive = em.fexclusive();
  printf("Exclusive: %s\n", exclusive ? "yes" : "no");

  errno = 0;
  result = em.fread(&em);
  if (result == 0 && errno) {
    perror("fread");
    return 1;
  }
  printf("Got reading: %"PRIu64"\n", result);

  if (em.ffinish(&em)) {
    perror("ffinish");
    return 1;
  }
  printf("Finished reading from %s\n", source);

  return 0;
}
