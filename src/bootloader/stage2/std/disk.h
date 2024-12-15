#pragma once

#include "stdint.h"

typedef struct s_Disk {
  uint8_t id;
  uint16_t cylinders;
  uint16_t sectors;
  uint16_t heads;

} Disk;

// Initialize the disk struct with drive parameters
bool DiskInitialize(Disk *disk, uint8_t driveNumber);

// Read data from disk
bool DiskRead(Disk *disk, uint32_t lba, uint8_t sectors, uint8_t far *dataOut);

// Convert LBA address to cylinder, sector and head numbers
void DiskLBA2CHS(Disk *disk, uint32_t lba, uint16_t *cylinderOut,
                 uint16_t *sectorOut, uint16_t *headOut);
