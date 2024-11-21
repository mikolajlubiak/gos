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

int *printf_format_numbers(int *argp, PrintfLengthState length, bool sign,
                           uint16_t radix);

void _cdecl printf(const char *fmt, ...) {
  int *argp = (int *)&fmt;
  PrintfState state = PRINTF_STATE_NORMAL;
  PrintfLengthState length = PRINTF_LENGTH_DEFAULT;
  uint16_t radix = 10;
  bool sign = false;

  argp++;

  while (*fmt) {
    switch (state) {
    case PRINTF_STATE_NORMAL:
      switch (*fmt) {
      case '%':
        state = PRINTF_STATE_LENGTH;
        break;

      default:
        putc(*fmt);
        break;
      }

      fmt++;
      break;

    case PRINTF_STATE_LENGTH:
      switch (*fmt) {
      case 'h':
        length = PRINTF_LENGTH_SHORT;
        state = PRINTF_STATE_LENGTH_SHORT;
        fmt++;
        break;
      case 'l':
        length = PRINTF_LENGTH_LONG;
        state = PRINTF_STATE_LENGTH_LONG;
        fmt++;
        break;

      default:
        break;
      }
      break;

    case PRINTF_STATE_LENGTH_SHORT:
      switch (*fmt) {
      case 'h':
        length = PRINTF_LENGTH_SHORT_SHORT;
        fmt++;
        break;

      default:
        break;
      }

      state = PRINTF_STATE_SPECIFIER;
      break;

    case PRINTF_STATE_LENGTH_LONG:
      switch (*fmt) {
      case 'l':
        length = PRINTF_LENGTH_LONG_LONG;
        fmt++;
        break;

      default:
        break;
      }

      state = PRINTF_STATE_SPECIFIER;
      break;

    case PRINTF_STATE_SPECIFIER:
      switch (*fmt) {
      case 'c':
        putc((const char)*argp);
        argp++;
        break;

      case 's':
        puts(*(const char **)argp);
        argp++;
        break;

      case '%':
        putc('%');
        break;

      case 'd':
      case 'i':
        radix = 10;
        sign = true;
        argp = printf_format_numbers(argp, length, sign, radix);
        break;

      case 'u':
        radix = 10;
        sign = false;
        argp = printf_format_numbers(argp, length, sign, radix);
        break;

      case 'X':
      case 'x':
      case 'p':
        radix = 16;
        sign = false;
        argp = printf_format_numbers(argp, length, sign, radix);
        break;

      case 'o':
        radix = 8;
        sign = false;
        argp = printf_format_numbers(argp, length, sign, radix);
        break;

      default:
        break;
      }

      state = PRINTF_STATE_NORMAL;
      length = PRINTF_LENGTH_DEFAULT;
      radix = 10;
      sign = false;
      fmt++;
      break;

    default:
      break;
    }
  }
}

int *printf_format_numbers(int *argp, PrintfLengthState length, bool sign,
                           uint16_t radix) {
  const char HEX_CHARS[] = "0123456789abcdef";

  char buffer[32];
  uint64_t number;
  bool negative = false;
  uint16_t pos = 0;

  switch (length) {
  case PRINTF_LENGTH_SHORT_SHORT:
  case PRINTF_LENGTH_SHORT:
  case PRINTF_LENGTH_DEFAULT:
    if (sign) {
      int16_t n = *argp;

      if (n < 0) {
        negative = true;
      }

      number = (uint64_t)n;
    }

    argp++;
    break;

  case PRINTF_LENGTH_LONG:
    if (sign) {
      int32_t n = *argp;

      if (n < 0) {
        negative = true;
      }

      number = (uint64_t)n;
    }

    argp += 2;
    break;

  case PRINTF_LENGTH_LONG_LONG:
    if (sign) {
      int64_t n = *argp;

      if (n < 0) {
        negative = true;
      }

      number = (uint64_t)n;
    }

    argp += 4;
    break;

  default:
    break;
  }

  do {
    uint32_t rem;
    x86_div64_32(number, radix, &number, &rem);
    buffer[pos++] = HEX_CHARS[rem];
  } while (number > 0);

  if (sign && negative) {
    buffer[pos++] = '-';
  }

  while (pos > 0) {
    putc(buffer[pos--]);
  }

  return argp;
}
