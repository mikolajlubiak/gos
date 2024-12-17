#include "std/disk.h"
#include "std/fat.h"
#include "std/stdint.h"
#include "std/stdio.h"

void _cdecl cstart_(uint16_t bootDrive) {
  DISK disk;

  if (!DISK_Initialize(&disk, bootDrive)) {
    puts("Disk init error\n");
    goto end;
  }

  if (!FAT_Initialize(&disk)) {
    puts("FAT init error\n");
    goto end;
  }

  // List root directory contents
  FAT_File far *fd = FAT_Open(&disk, "/");
  FAT_DirectoryEntry entry;
  uint8_t i = 0;

  while (FAT_ReadEntry(&disk, fd, &entry) && i++ < 3) {
    sputs(entry.Name, 11);
    putc('\n');
  }

  FAT_Close(fd);

  // Read test.txt
  char buffer[512];
  uint32_t read;
  fd = FAT_Open(&disk, "test.txt");

  while ((read = FAT_Read(&disk, fd, sizeof(buffer), buffer))) {
    for (uint32_t i = 0; i < read; i++) {
      putc(buffer[i]);
    }
  }

  FAT_Close(fd);

end:
  while (true) {
    continue;
  }
}
