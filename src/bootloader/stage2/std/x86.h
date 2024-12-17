#pragma once

#include "stdint.h"

void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor,
                         uint64_t *quotient_out, uint32_t *remainder_out);

void _cdecl x86_Video_WriteCharTeletype(char character, uint8_t page);

// Resets the disk
bool _cdecl x86_Disk_Reset(uint8_t drive);

// Read data from disk
bool _cdecl x86_Disk_Read(uint8_t drive, uint16_t cylinder, uint16_t sector,
                          uint16_t head, uint8_t count, void far *dataOut);

// Get drive parameters
bool _cdecl x86_Disk_GetDriveParams(uint8_t drive, uint8_t *driveTypeOut,
                                    uint16_t *cylindersOut, uint16_t *sectorOut,
                                    uint16_t *headOut);
