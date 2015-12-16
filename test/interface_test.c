#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include "energymon-default.h"

int main() {
  int ret;
  energymon em;
  char source[100];

  energymon_get_default(&em);

  em.fsource(source, sizeof(source));
  printf("Initializing reading from %s\n", source);
  ret = em.finit(&em);
  if (ret) {
    perror("finit");
  }
  assert(ret == 0);
  uint64_t result = em.fread(&em);
  if (result == 0 && errno) {
    perror("fread");
  }
  printf("Got reading: %"PRIu64"\n", result);
  ret = em.ffinish(&em);
  if (ret) {
    perror("ffinish");
  }
  assert(ret == 0);
  printf("Finished reading from %s\n", source);

  return 0;
}
