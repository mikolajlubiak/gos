#pragma once

#include "disk.h"
#include "stdint.h"

#define SECTOR_SIZE 512
#define MAX_FILE_HANDLES 16
#define ROOT_DIRECTORY_HANDLE -1
#define MAX_PATH_LENGTH 256

#pragma pack(push, 1)

typedef struct {
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
} FAT_BootSector;

typedef struct {
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
} FAT_DirectoryEntry;

#pragma pack(pop)

typedef struct {
  int32_t Handle;
  bool IsDirectory;
  uint32_t Position;
  uint32_t Size;
} FAT_File;

typedef struct {
  FAT_File Public;

  bool Opened;

  uint32_t FirstCluster;
  uint32_t CurrentCluster;
  uint32_t CurrentSector;

  uint8_t Buffer[SECTOR_SIZE];
} FAT_FileData;

typedef struct {
  union {
    FAT_BootSector BootSector;
    uint8_t BootSectorBytes[SECTOR_SIZE];
  } BS;

  FAT_FileData RootDirectory;

  FAT_FileData OpenedFiles[MAX_FILE_HANDLES];

} FAT_Data;

enum FAT_Attributes {
  FAT_ATTRIBUTE_READ_ONLY = 0x01,
  FAT_ATTRIBUTE_HIDDEN = 0x02,
  FAT_ATTRIBUTE_SYSTEM = 0x04,
  FAT_ATTRIBUTE_VOLUME_ID = 0x08,
  FAT_ATTRIBUTE_DIRECTORY = 0x10,
  FAT_ATTRIBUTE_ARCHIVE = 0x20,
  FAT_ATTRIBUTE_LFN = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN |
                      FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

bool FAT_Initialize(DISK *disk);

bool FAT_ReadBootSector(DISK *const disk);

bool FAT_ReadFat(DISK *const disk);

FAT_File far *FAT_Open(DISK *disk, const char *path);

FAT_File far *FAT_OpenEntry(DISK *disk, FAT_DirectoryEntry *entry);

uint32_t FAT_Read(DISK *disk, FAT_File far *file, uint32_t byteCount,
                  void *buffer);

bool FAT_ReadEntry(DISK *disk, FAT_File far *file,
                   FAT_DirectoryEntry *directoryEntry);

void FAT_Close(FAT_File far *file);

bool FAT_FindFile(DISK *disk, FAT_File far *file, const char *name,
                  FAT_DirectoryEntry *entryOut);

uint32_t FAT_ClusterToLba(uint32_t cluster);

uint32_t FAT_NextCluster(uint32_t cluster);
