#include "disk.h"
#include "stdio.h"
#include "x86.h"

bool DISK_Initialize(DISK *disk, uint8_t driveNumber) {
  uint8_t driveType;
  uint16_t cylinders, sectors, heads;

  if (!x86_Disk_GetDriveParams(disk->id, &driveType, &cylinders, &sectors,
                               &heads)) {
    puts("Failed to get disk parameters");
    return false;
  }

  disk->id = driveNumber;
  disk->cylinders = cylinders + 1;
  disk->heads = heads + 1;
  disk->sectors = sectors;

  return true;
}

bool DISK_Read(DISK *disk, uint32_t lba, uint8_t sectors_to_read,
               void far *dataOut) {
  uint16_t cylinder, sector, head;

  DISK_LBA2CHS(disk, lba, &cylinder, &sector, &head);

  const uint8_t tries = 3;

  for (uint8_t i = 0; i < tries; i++) {
    if (x86_Disk_Read(disk->id, cylinder, sector, head, sectors_to_read,
                      dataOut)) {
      return true;
    }

    if (!x86_Disk_Reset(disk->id)) {
      printf("Failed to reset the disk %hhu", disk->id);
    }
  }

  printf("Tried to read from the disk %hu times and failed. DISK %hu, LBA %lu, "
         "cylinder %u, sector %u, head %u, sectors to read count "
         "%hu\n",
         tries, disk->id, lba, cylinder, sector, head, sectors_to_read);

  return false;
}

void DISK_LBA2CHS(DISK *disk, uint32_t lba, uint16_t *cylinderOut,
                  uint16_t *sectorOut, uint16_t *headOut) {
  *sectorOut = lba % disk->sectors + 1;
  *cylinderOut = (lba / disk->sectors) / disk->heads;
  *headOut = (lba / disk->sectors) % disk->heads;
}
