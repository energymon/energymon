#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "energymon-default.h"

int main(int argc, char** argv) {
  char source[100];
  long usec;
  uint64_t start, end, uj;
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <idle_usec>\n", argv[0]);
    return 1;
  }
  usec = strtol(argv[1], NULL, 0);
  assert(usec > 0);

  energymon em;
  energymon_get_default(&em);

  em.fsource(source, sizeof(source));
  printf("Initializing reading from %s\n", source);
  assert(em.finit(&em) == 0);

  start = em.fread(&em);
  usleep(usec);
  end = em.fread(&em);
  uj = end - start;

  printf("Total energy: %"PRIu64" uJ\n", uj);
  printf("Average power: %f W\n", (double) uj / (double) usec);
  assert(em.ffinish(&em) == 0);
  printf("Finished reading from %s\n", source);

  return 0;
}
