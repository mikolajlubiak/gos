#include "std/stdint.h"
#include "std/stdio.h"

void _cdecl cstart_(uint16_t boot_driver_number) {
  uint16_t i;
  for (i = 0; i < 64; i++) {
    printf("Hello, World! I'm C. %d\n", i);
  }

  for (;;)
    ;
}
