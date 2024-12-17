#include "fat.h"

#include "ctype.h"
#include "memory.h"
#include "memory_defines.h"
#include "stdio.h"
#include "string.h"
#include "utility.h"

static FAT_Data far *g_Data;
static uint8_t far *g_Fat = NULL;
static uint32_t g_DataSectionLba;

bool FAT_Initialize(DISK *const disk) {
  // Read boot sector
  g_Data = (FAT_Data far *)MEMORY_FAT_ADDR;

  if (!FAT_ReadBootSector(disk)) {
    puts("[FAT]: Boot sector read failed!\n");
    return false;
  }

  // Read FAT
  g_Fat = (uint8_t far *)g_Data + sizeof(FAT_Data);

  uint32_t fatSize = g_Data->BS.BootSector.BytesPerSector *
                     g_Data->BS.BootSector.SectorsPerFat;

  if (sizeof(FAT_Data) + fatSize >= MEMORY_FAT_SIZE) {
    printf("[FAT]: Not enough memory to read FAT! Required %lu, provided %u\n",
           sizeof(FAT_Data) + fatSize, MEMORY_FAT_SIZE);
    return false;
  }

  if (!FAT_ReadFat(disk)) {
    puts("[FAT]: FAT read failed\n");
    return false;
  }

  // Read root directory
  uint32_t rootSize =
      sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;

  uint32_t rootLba =
      g_Data->BS.BootSector.ReservedSectors +
      g_Data->BS.BootSector.SectorsPerFat * g_Data->BS.BootSector.FatCount;

  // Open root directory
  g_Data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
  g_Data->RootDirectory.Public.IsDirectory = true;
  g_Data->RootDirectory.Public.Position = 0;
  g_Data->RootDirectory.Public.Size = rootSize;
  g_Data->RootDirectory.Opened = true;
  g_Data->RootDirectory.FirstCluster = rootLba;
  g_Data->RootDirectory.CurrentCluster = rootLba;
  g_Data->RootDirectory.CurrentSector = 0;

  if (!DISK_Read(disk, rootLba, 1, g_Data->RootDirectory.Buffer)) {
    puts("[FAT]: Root directory read failed\n");
    return false;
  }

  uint32_t rootSectors = (rootSize + g_Data->BS.BootSector.BytesPerSector - 1) /
                         g_Data->BS.BootSector.BytesPerSector;

  g_DataSectionLba = rootLba + rootSectors;

  // Reset opened files
  for (size_t i = 0; i < MAX_FILE_HANDLES; i++) {
    g_Data->OpenedFiles[i].Opened = false;
  }

  return true;
}

bool FAT_ReadBootSector(DISK *const disk) {
  return DISK_Read(disk, 0, 1, g_Data->BS.BootSectorBytes);
}

bool FAT_ReadFat(DISK *const disk) {
  return DISK_Read(disk, g_Data->BS.BootSector.ReservedSectors,
                   g_Data->BS.BootSector.SectorsPerFat, g_Fat);
}

FAT_File far *FAT_Open(DISK *disk, const char *path) {
  char name[MAX_PATH_LENGTH];

  if (path[0] == '/') {
    path++;
  }

  FAT_File far *current = &g_Data->RootDirectory.Public;

  while (*path) {
    // Extract next file name from path
    bool isLast = false;

    const char *delim = strchr(path, '/');

    if (delim != NULL) {
      memcpy(name, path, delim - path);
      name[delim - path + 1] = '\0';

      path = delim + 1;
    } else {
      size_t len = strlen(path);

      memcpy(name, path, len);
      name[len + 1] = '\0';

      path += len;
      isLast = true;
    }

    // Find directory entry in current directory
    FAT_DirectoryEntry entry;
    if (FAT_FindFile(disk, current, name, &entry)) {
      FAT_Close(current);

      if (!isLast && entry.Attributes & FAT_ATTRIBUTE_DIRECTORY == 0) {
        printf("[FAT]: %s is not a directory\n", name);
        return NULL;
      }

      // Open new directory entry
      current = FAT_OpenEntry(disk, &entry);
    } else {
      FAT_Close(current);
      printf("[FAT]: %s not found\n", name);
      return NULL;
    }
  }

  return current;
}

FAT_File far *FAT_OpenEntry(DISK *disk, FAT_DirectoryEntry *entry) {
  // Find empty handle
  int32_t handle = -1;

  for (size_t i = 0; i < MAX_FILE_HANDLES && handle < 0; i++) {
    if (!g_Data->OpenedFiles[i].Opened) {
      handle = i;
    }
  }

  if (handle < 0) {
    puts("[FAT]: Out of file handles\n");
    return false;
  }

  FAT_FileData far *fd = &g_Data->OpenedFiles[handle];
  fd->Public.Handle = handle;
  fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
  fd->Public.Position = 0;
  fd->Public.Size = entry->Size;
  fd->FirstCluster =
      entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
  fd->CurrentCluster = fd->FirstCluster;
  fd->CurrentSector = 0;

  if (!DISK_Read(disk, FAT_ClusterToLba(fd->CurrentCluster), 1, fd->Buffer)) {
    puts("[FAT]: Read error\n");
    return false;
  }

  fd->Opened = true;
  return &fd->Public;
}

uint32_t FAT_Read(DISK *disk, FAT_File far *file, uint32_t byteCount,
                  void *buffer) {
  // Get file data
  FAT_FileData far *fd;
  if (file->Handle == ROOT_DIRECTORY_HANDLE) {
    fd = &g_Data->RootDirectory;
  } else {
    fd = &g_Data->OpenedFiles[file->Handle];
  }

  uint8_t *u8Buffer = (uint8_t *)buffer;

  if (!fd->Public.IsDirectory) {
    byteCount = min(byteCount, fd->Public.Size - fd->Public.Position);
  }

  while (byteCount > 0) {
    uint32_t leftInBuffer = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
    uint32_t take = min(byteCount, leftInBuffer);

    memcpy(u8Buffer, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);

    u8Buffer += take;
    fd->Public.Position += take;
    byteCount -= take;

    if (leftInBuffer == take) {
      // Root directory special case
      if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE) {
        fd->CurrentCluster++;

        // Read next sector
        if (!DISK_Read(disk, fd->CurrentCluster, 1, fd->Buffer)) {
          puts("[FAT]: Read error!\n");
          break;
        }
      } else {
        // Calculate next cluster and sector to read
        if (++fd->CurrentSector >= g_Data->BS.BootSector.SectorsPerCluster) {
          fd->CurrentSector = 0;
          fd->CurrentCluster = FAT_NextCluster(fd->CurrentCluster);
        }

        // Next cluster out of bounds
        // Unreachable
        if (fd->CurrentCluster >= 0xFF8) {
          fd->Public.Size = fd->Public.Position;
          puts("[FAT]: Read error! Invalid next cluster!\n");
          break;
        }

        // Read next sector
        if (!DISK_Read(disk,
                       FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSector,
                       1, fd->Buffer)) {
          puts("[FAT]: Read error!\n");
          break;
        }
      }
    }
  }

  return u8Buffer - (uint8_t *)buffer;
}

bool FAT_ReadEntry(DISK *disk, FAT_File far *file,
                   FAT_DirectoryEntry *directoryEntry) {
  return FAT_Read(disk, file, sizeof(FAT_DirectoryEntry), directoryEntry) ==
         sizeof(FAT_DirectoryEntry);
}

void FAT_Close(FAT_File far *file) {
  if (file->Handle == ROOT_DIRECTORY_HANDLE) {
    file->Position = 0;
    g_Data->RootDirectory.CurrentCluster = g_Data->RootDirectory.FirstCluster;
  } else {
    g_Data->OpenedFiles[file->Handle].Opened = false;
  }
}

bool FAT_FindFile(DISK *disk, FAT_File far *file, const char *name,
                  FAT_DirectoryEntry *entryOut) {
  char fatName[12];
  FAT_DirectoryEntry entry;

  memset(fatName, ' ', sizeof(fatName));
  fatName[11] = '\0';

  const char *ext = strchr(name, '.');

  if (ext == NULL) {
    ext = name + 11;
  }

  for (uint8_t i = 0; i < 8 && name + i < ext; i++) {
    fatName[i] = toUpper(name[i]);
  }

  if (ext != NULL) {
    for (uint8_t i = 0; i < 3 && ext[i + 1]; i++) {
      fatName[i + 8] = toUpper(ext[i + 1]);
    }
  }

  while (FAT_ReadEntry(disk, file, &entry)) {
    if (memcmp(fatName, entry.Name, 11) == 0) {
      *entryOut = entry;
      return true;
    }
  }

  return false;
}

uint32_t FAT_ClusterToLba(uint32_t cluster) {
  return g_DataSectionLba +
         (cluster - 2) * g_Data->BS.BootSector.SectorsPerCluster;
}

uint32_t FAT_NextCluster(uint32_t cluster) {
  uint32_t fatIndex = cluster * 3 / 2;

  if (cluster % 2 == 0) {
    return (*(uint16_t *)(g_Fat + fatIndex)) & 0x0FFF;
  } else {
    return (*(uint16_t *)(g_Fat + fatIndex)) >> 4;
  }
}
