#pragma once

#include "stdint.h"

void far *memcpy(void far *dst, const void far *src, size_t size);

void far *memset(void far *ptr, uint8_t value, size_t size);

uint8_t memcmp(const void far *ptr1, const void far *ptr2, size_t size);
