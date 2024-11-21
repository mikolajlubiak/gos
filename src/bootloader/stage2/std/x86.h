#pragma once

#include "stdint.h"

void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor,
                         uint64_t *quotient_out, uint32_t *remainder_out);

void _cdecl x86_Video_WriteCharTeletype(char character, uint8_t page);
