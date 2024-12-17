#pragma once

#include "stdint.h"

typedef enum e_PrintfState {
  PRINTF_STATE_NORMAL,
  PRINTF_STATE_LENGTH,
  PRINTF_STATE_SPECIFIER,
  PRINTF_STATE_LENGTH_SHORT,
  PRINTF_STATE_LENGTH_LONG,
} PrintfState;

typedef enum e_PrintfLengthState {
  PRINTF_LENGTH_SHORT_SHORT,
  PRINTF_LENGTH_SHORT,
  PRINTF_LENGTH_DEFAULT,
  PRINTF_LENGTH_LONG,
  PRINTF_LENGTH_LONG_LONG,
} PrintfLengthState;

void putc(const char c);

void puts(const char *str);

void sputs(const char *str, size_t length);

void _cdecl printf(const char *fmt, ...);
