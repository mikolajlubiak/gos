#include "std/stdint.h"
#include "std/stdio.h"

void _cdecl cstart_(uint16_t boot_driver_number) {
  uint8_t i;
  for (i = 0; i < 64; i++) {
    puts("Hello, World! I'm C. ");
    putc((const char)('0' + (i % 10)));
    putc('\n');
  }

  for (;;)
    ;
}
