#include "stdio.h"
#include "x86.h"

void putc(const char c) {
  if (c == '\n') {
    x86_Video_WriteCharTeletype(0x0D, 0);
    x86_Video_WriteCharTeletype(0x0A, 0);
  } else {
    x86_Video_WriteCharTeletype(c, 0);
  }
}

void puts(const char *str) {
  while (*str) {
    putc(*str);
    str++;
  }
}
