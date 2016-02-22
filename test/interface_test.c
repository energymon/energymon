#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include "energymon-default.h"

int main() {
  energymon em;
  char source[100];
  uint64_t result;

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
