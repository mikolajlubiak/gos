#include "std/stdint.h"
#include "std/stdio.h"

void _cdecl cstart_(uint16_t boot_driver_number) {
  puts("Hello, World! I'm C.");

  for (;;)
    ;
}
