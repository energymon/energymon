#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "energymon-default.h"

int main(int argc, char** argv) {
  char source[100];
  long usec;
  unsigned long long start, end, uj;
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <idle_usec>\n", argv[0]);
    return 1;
  }
  usec = strtol(argv[1], NULL, 0);
  assert(usec > 0);

  energymon impl;
  energymon_get_default(&impl);

  impl.fsource(source, sizeof(source));
  printf("Initializing reading from %s\n", source);
  assert(impl.finit(&impl) == 0);

  start = impl.fread(&impl);
  usleep(usec);
  end = impl.fread(&impl);
  uj = end - start;

  printf("Total energy: %lld uJ\n", uj);
  printf("Average power: %f W\n", (double) uj / (double) usec);
  assert(impl.ffinish(&impl) == 0);
  printf("Finished reading from %s\n", source);

  return 0;
}
