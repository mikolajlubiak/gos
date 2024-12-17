#pragma once

#include "stdint.h"

typedef struct {
  uint8_t id;
  uint16_t cylinders;
  uint16_t sectors;
  uint16_t heads;

} DISK;

// Initialize the disk struct with drive parameters
bool DISK_Initialize(DISK *disk, uint8_t driveNumber);

// Read data from disk
bool DISK_Read(DISK *disk, uint32_t lba, uint8_t sectors_to_read,
               void far *dataOut);

// Convert LBA address to cylinder, sector and head numbers
void DISK_LBA2CHS(DISK *disk, uint32_t lba, uint16_t *cylinderOut,
                  uint16_t *sectorOut, uint16_t *headOut);
