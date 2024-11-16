#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct s_BootSector {
  uint8_t BootJumpInstruction[3];
  uint8_t OemIdentifier[8];
  uint16_t BytesPerSector;
  uint8_t SectorsPerCluster;
  uint16_t ReservedSectors;
  uint8_t FatCount;
  uint16_t DirEntryCount;
  uint16_t TotalSectors;
  uint8_t MediaDescriptorType;
  uint16_t SectorsPerFat;
  uint16_t SectorsPerTrack;
  uint16_t Heads;
  uint32_t HiddenSectors;
  uint32_t LargeSectorCount;

  // extended boot record
  uint8_t DriveNumber;
  uint8_t _Reserved;
  uint8_t Signature;
  uint32_t VolumeId;       // serial number, value doesn't matter
  uint8_t VolumeLabel[11]; // 11 bytes, padded with spaces
  uint8_t SystemId[8];
} __attribute__((packed)) BootSector;

typedef struct s_DirectoryEntry {
  uint8_t Name[11];
  uint8_t Attributes;
  uint8_t _Reserved;
  uint8_t CreatedTimeTenths;
  uint16_t CreatedTime;
  uint16_t CreatedDate;
  uint16_t AccessedData;
  uint16_t FirstClusterHigh;
  uint16_t ModifiedTime;
  uint16_t ModifiedDate;
  uint16_t FirstClusterLow;
  uint32_t Size;

} __attribute__((packed)) DirectoryEntry;

uint8_t readBootSector(FILE *const disk, BootSector *boot_sector) {
  const uint8_t count = 1;
  return fread(boot_sector, sizeof(BootSector), count, disk) == count;
}

uint8_t readSectors(FILE *const disk, const BootSector *const boot_sector,
                    const uint32_t lba, const uint32_t count,
                    void *const output_buffer) {
  uint8_t ok = 1;

  ok = ok && (fseek(disk, lba * boot_sector->BytesPerSector, SEEK_SET) == 0);
  ok = ok && (fread(output_buffer, boot_sector->BytesPerSector, count, disk) ==
              count);

  return ok;
}

uint8_t readFat(FILE *const disk, const BootSector *const boot_sector,
                uint8_t **file_allocation_table) {
  *file_allocation_table = (uint8_t *)malloc(boot_sector->SectorsPerFat *
                                             boot_sector->BytesPerSector);

  return readSectors(disk, boot_sector, boot_sector->ReservedSectors,
                     boot_sector->SectorsPerFat, *file_allocation_table);
}

uint8_t readRootDirectory(FILE *const disk, const BootSector *const boot_sector,
                          DirectoryEntry **root_directory,
                          uint32_t *root_directory_end) {
  uint32_t lba = boot_sector->ReservedSectors +
                 boot_sector->SectorsPerFat * boot_sector->FatCount;

  uint32_t size = sizeof(DirectoryEntry) * boot_sector->DirEntryCount;
  uint32_t sectors = (((int32_t)(size + boot_sector->BytesPerSector) - 1) /
                      boot_sector->BytesPerSector);

  *root_directory_end = lba + sectors;

  *root_directory =
      (DirectoryEntry *)malloc(sectors * boot_sector->BytesPerSector);

  return readSectors(disk, boot_sector, lba, sectors, *root_directory);
}

const DirectoryEntry *findFile(const BootSector *const boot_sector,
                               const DirectoryEntry *const root_directory,
                               const char *const filename) {
  for (uint32_t i = 0; i < boot_sector->DirEntryCount; i++) {
    if (memcmp(filename, root_directory[i].Name,
               sizeof(((DirectoryEntry *)0)->Name)) == 0) {
      return &root_directory[i];
    }
  }

  return NULL;
}

uint8_t readFile(FILE *const disk, const BootSector *const boot_sector,
                 const uint8_t *const file_allocation_table,
                 const DirectoryEntry *const file,
                 const uint32_t root_directory_end, void *output_buffer) {
  uint16_t current_cluster = file->FirstClusterLow;
  uint8_t ok = 1;

  do {
    uint32_t lba = root_directory_end +
                   (current_cluster - 2) * boot_sector->SectorsPerCluster;

    ok = ok && readSectors(disk, boot_sector, lba,
                           boot_sector->SectorsPerCluster, output_buffer);

    output_buffer +=
        boot_sector->SectorsPerCluster * boot_sector->BytesPerSector;

    uint32_t fat_index = current_cluster * 3 / 2;
    if (current_cluster % 2 == 0) {
      current_cluster =
          (*(uint16_t *)(file_allocation_table + fat_index)) & 0x0FFF;
    } else {
      current_cluster = (*(uint16_t *)(file_allocation_table + fat_index)) >> 4;
    }

  } while (ok && current_cluster < 0xFF8);

  return ok;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Syntax: %s <disk_image> <file_name>\n", argv[0]);
    return -1;
  }

  FILE *disk = fopen(argv[1], "rb");

  if (!disk) {
    fprintf(stderr, "Could not open disk image %s!\n", argv[1]);
    return -2;
  }

  BootSector boot_sector;

  if (!readBootSector(disk, &boot_sector)) {
    fprintf(stderr, "Could not read boot sector!\n");
    return -3;
  }

  uint8_t *file_allocation_table = NULL;

  if (!readFat(disk, &boot_sector, &file_allocation_table)) {
    fprintf(stderr, "Could not read file allocation table!\n");
    free(file_allocation_table);
    return -4;
  }

  DirectoryEntry *root_directory = NULL;
  uint32_t root_directory_end;

  if (!readRootDirectory(disk, &boot_sector, &root_directory,
                         &root_directory_end)) {
    fprintf(stderr, "Could not read root directory!\n");
    free(file_allocation_table);
    free(root_directory);
    return -5;
  }

  const DirectoryEntry *file = findFile(&boot_sector, root_directory, argv[2]);

  if (!file) {
    fprintf(stderr, "Could not find file %s!\n", argv[2]);
    free(file_allocation_table);
    free(root_directory);
    return -6;
  }

  char *file_contents = malloc(file->Size + boot_sector.BytesPerSector);

  if (!readFile(disk, &boot_sector, file_allocation_table, file,
                root_directory_end, file_contents)) {
    fprintf(stderr, "Could not read file contents!\n");
    free(file_allocation_table);
    free(root_directory);
    free(file_contents);
    return -7;
  }

  printf("%s", file_contents);

  free(file_allocation_table);
  free(root_directory);
  free(file_contents);
  return 0;
}