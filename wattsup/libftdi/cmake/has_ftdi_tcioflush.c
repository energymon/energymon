#include <stdlib.h>
#include <ftdi.h>

int main(void) {
  ftdi_tcioflush(NULL);
  return 0;
}
