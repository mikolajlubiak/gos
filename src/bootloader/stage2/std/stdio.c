#include "stdio.h"
#include "x86.h"

void putc(char c) { x86_Video_WriteCharTeletype(c, 0); }

void puts(const char *str) {
  while (*str) {
    if (*str == '\n') {
      putc(0x0D);
      putc(0x0A);
    } else {
      putc(*str);
    }

    str++;
  }
}
