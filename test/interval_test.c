#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "energymon-default.h"

int main(int argc, char** argv) {
  char source[100];
  long usec;
  uint64_t start, end, uj;
  energymon em;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <idle_usec>\n", argv[0]);
    return EINVAL;
  }

  usec = strtol(argv[1], NULL, 0);
  if(usec <= 0) {
    fprintf(stderr, "idle_usec must be > 0\n");
    return EINVAL;
  }

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
  start = em.fread(&em);
  if (start == 0 && errno) {
    perror("fread");
    return 1;
  }
  usleep(usec);
  errno = 0;
  end = em.fread(&em);
  if (end == 0 && errno) {
    perror("fread");
    return 1;
  }
  uj = end - start;

  printf("Total energy: %"PRIu64" uJ\n", uj);
  printf("Average power: %f W\n", (double) uj / (double) usec);
  if (em.ffinish(&em)) {
    perror("ffinish");
    return 1;
  }
  printf("Finished reading from %s\n", source);

  return 0;
}
