#include "memory.h"

void far *memcpy(void far *dst, const void far *src, size_t size) {
  uint8_t far *u8Dst = (uint8_t far *)dst;
  const uint8_t far *u8Src = (const uint8_t far *)src;

  for (size_t i = 0; i < size; i++) {
    u8Dst[i] = u8Src[i];
  }

  return dst;
}

void far *memset(void far *ptr, uint8_t value, size_t size) {
  uint8_t far *u8Ptr = (uint8_t far *)ptr;

  for (size_t i = 0; i < size; i++) {
    u8Ptr[i] = value;
  }

  return ptr;
}

uint8_t memcmp(const void far *ptr1, const void far *ptr2, size_t size) {
  const uint8_t far *u8Ptr1 = (const uint8_t far *)ptr1;
  const uint8_t far *u8Ptr2 = (const uint8_t far *)ptr2;

  for (size_t i = 0; i < size; i++) {
    if (u8Ptr1[i] != u8Ptr2[i]) {
      return 1;
    }
  }

  return 0;
}
