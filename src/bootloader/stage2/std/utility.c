#include "utility.h"

uint32_t align(uint32_t number, uint32_t alignTo) {
  if (alignTo == 0) {
    return number;
  }

  uint32_t rem = number % alignTo;

  if (rem == 0) {
    return number;
  }

  return number + alignTo - rem;
}

int32_t min(int32_t val1, int32_t val2) {
  if (val1 < val2) {
    return val1;
  }

  return val2;
}

int32_t max(int32_t val1, int32_t val2) {
  if (val1 > val2) {
    return val1;
  }

  return val2;
}
