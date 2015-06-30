#include <stdio.h>
#include <stdlib.h>
#include "energymon.h"

/**
 * The default implementation does nothing.
 */
int em_impl_get(em_impl* impl) {
  fprintf(stderr, "Default energymon is not implemented. Exiting.\n");
  exit(1);
}
