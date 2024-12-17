#include "string.h"

#include "stdint.h"

const char *strchr(const char *str, char chr) {
  if (str == NULL) {
    return NULL;
  }

  while (*str) {
    if (*str == chr) {
      return str;
    }
    str++;
  }

  return NULL;
}

char *strcpy(char *dst, const char *src) {
  char *orgDst = dst;

  if (dst == NULL) {
    return NULL;
  }

  if (src == NULL) {
    *dst = '\0';
    return dst;
  }

  while (*src) {
    *dst = *src;
    dst++;
    src++;
  }

  *dst = '\0';

  return orgDst;
}

size_t strlen(const char *str) {
  size_t len = 0;

  while (*str) {
    len++;
    str++;
  }

  return len;
}
