#include "disk.h"
#include "stdio.h"
#include "x86.h"

bool DiskInitialize(Disk *disk, uint8_t driveNumber) {
  disk->id = driveNumber;
  uint8_t diskType;

  if (!x86_Disk_GetDriveParams(disk->id, &diskType, &disk->cylinders,
                               &disk->heads, &disk->sectors)) {
    return false;
  }

  return true;
}

bool DiskRead(Disk *disk, uint32_t lba, uint8_t sectors_to_read,
              uint8_t far *dataOut) {
  uint16_t cylinder, sector, head;

  DiskLBA2CHS(disk, lba, &cylinder, &sector, &head);

  const int tries = 3;

  for (uint8_t i = 0; i < tries; i++) {
    if (x86_Disk_Read(disk->id, cylinder, sector, head, sectors_to_read,
                      dataOut)) {
      return true;
    }

    if (!x86_Disk_Reset(disk->id)) {
      printf("Failed to reset the disk %hhu", disk->id);
    }
  }

  printf(
      "Tried to read from the disk %hhu times and failed. Disk %hhu, LBA %u, "
      "cylinder %hu, sector %hu, head %hu, sectors to read count "
      "%hhu",
      tries, disk->id, lba, cylinder, sector, head, sectors_to_read);

  return false;
}

void DiskLBA2CHS(Disk *disk, uint32_t lba, uint16_t *cylinderOut,
                 uint16_t *sectorOut, uint16_t *headOut) {
  *sectorOut = lba % disk->sectors + 1;
  *cylinderOut = (lba / disk->sectors) / disk->heads;
  *headOut = (lba / disk->sectors) % disk->heads;
}
