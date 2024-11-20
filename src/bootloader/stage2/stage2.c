#include "std/stdint.h"
#include "std/stdio.h"

void _cdecl cstart_(uint16_t boot_driver_number) {
  uint8_t i;
  for (i = 0; i < 16; i++) {
    puts("Hello, World! I'm C.\n");
  }

  for (;;)
    ;
}
