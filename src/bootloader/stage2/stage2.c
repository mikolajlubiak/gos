#include "std/stdint.h"
#include "std/stdio.h"

void _cdecl cstart_(uint16_t boot_driver_number) {
  for (int16_t i = -64; i < 0; i++) {
    printf("Hello, World! I'm C. %d\n", i);
  }

  puts("Hello world from C!\n");
  printf("Formatted %% %c %s\n", 'a', "string");
  printf("Formatted %d %i %x %p %o %hd %hi %hhu %hhd\n", 1234, -5678, 0xdead,
         0xbeef, 012345, (int16_t)27, (int16_t)-42, (uint8_t)20, (int8_t)-10);
  printf("Formatted %ld %lx %lld %llx\n", (int32_t)-100000000,
         (uint32_t)0xdeadbeef, (int64_t)10200300400,
         (uint64_t)0xdeadbeeffeebdaed);

  while (true)
    ;
}
