# GOS Architecture

This document describes the architecture of the Gall Operating System (GOS): how the components are structured, how they interact, and the key design decisions behind the implementation.

See [BOOT_PROCESS.md](BOOT_PROCESS.md) for a detailed walkthrough of every step from power-on to kernel execution.

---

## Table of Contents

1. [Overview](#overview)
2. [Boot Flow Diagram](#boot-flow-diagram)
3. [Memory Map](#memory-map)
4. [Component Descriptions](#component-descriptions)
5. [FAT12 Filesystem Implementation](#fat12-filesystem-implementation)
6. [Key Design Decisions](#key-design-decisions)

---

## Overview

GOS is a minimal x86 operating system that demonstrates bootloader development, FAT12 filesystem parsing, and early kernel initialisation — all in 16-bit real mode. The system is delivered as a 1.44 MB FAT12 floppy image that can run directly in QEMU or Bochs.

```
+------------------+
|  1.44MB Floppy   |
|  FAT12 Image     |
|                  |
|  Sector 0        |  <-- Stage 1 bootloader (MBR, 512 bytes)
|  Sectors 1-9     |  <-- File Allocation Table (2 copies × 9 sectors)
|  Sectors 10-23   |  <-- Root directory (224 entries × 32 bytes)
|  Sectors 24+     |  <-- Data area: stage2.bin, kernel.bin, test.txt
+------------------+
```

---

## Boot Flow Diagram

```
Power On
    |
    v
+-------------------+
|       BIOS        |
|  POST & hardware  |
|  initialisation   |
+-------------------+
    |
    | Loads sector 0 (MBR) to 0x0000:0x7C00
    | Verifies boot signature 0xAA55
    | Transfers control via far jump
    v
+-------------------+
|  Stage 1          |  0x0000:0x7C00  (512 bytes, NASM)
|  stage1.asm       |
|                   |
|  1. Init segments |
|     & stack       |
|  2. Query drive   |
|     geometry via  |
|     INT 13h/08h   |
|  3. Load FAT root |
|     directory     |
|  4. Search for    |
|     STAGE2.BIN    |
|  5. Load FAT into |
|     buffer        |
|  6. Follow cluster|
|     chain, load   |
|     Stage 2 to    |
|     0x2000:0x0000 |
|  7. Far-jump to   |
|     Stage 2       |
+-------------------+
    |
    | Far jump to 0x2000:0x0000
    v
+-------------------+
|  Stage 2 entry    |  0x2000:0x0000  (NASM stub)
|  stage2.asm       |
|                   |
|  1. Disable ints  |
|  2. Set up stack  |
|  3. Push boot     |
|     drive number  |
|  4. Call cstart_  |
+-------------------+
    |
    | Call _cstart_(bootDrive)
    v
+-------------------+
|  Stage 2 C code   |  0x2000:xxxx    (C, Watcom compiler)
|  stage2.c         |
|                   |
|  1. Init DISK     |
|     driver        |
|  2. Init FAT12    |
|     driver        |
|  3. List root dir |
|  4. Read test.txt |
|  5. (Future: find |
|     and jump to   |
|     kernel.bin)   |
+-------------------+
    |
    | (Planned) Load kernel.bin and jump
    v
+-------------------+
|  Kernel           |  (to be determined)
|  kernel.nasm      |
|                   |
|  Prints "Hello,   |
|  world! It's me,  |
+-------------------+
```

---

## Memory Map

The following layout is used at runtime during the boot sequence. All addresses are physical (real-mode, 20-bit).

```
Physical Address    Size        Contents
------------------  ----------  ------------------------------------------
0x00000000          1 KB        Interrupt Vector Table (IVT)
0x00000400          256 B       BIOS Data Area (BDA)
0x00000500          64 KB       FAT driver data (MEMORY_FAT_ADDR)
                                - Boot sector copy
                                - FAT table cache
                                - Root directory entries
                                - Open file handle table (16 handles)
0x00007C00          512 B       Stage 1 bootloader (loaded by BIOS)
                                Stack grows DOWN from 0x7C00
0x00007E00          ~120 KB     Scratch buffer used by Stage 1
                                (disk read buffer, FAT cache)
0x00020000          64 KB       Stage 2 (loaded by Stage 1)
                                STAGE2_LOAD_SEGMENT = 0x2000
                                STAGE2_LOAD_OFFSET  = 0x0000
0x00030000          320 KB      Free memory (available for kernel)
0x00080000          128 KB      Extended BIOS Data Area (EBDA)
0x000A0000          160 KB      Video memory (VGA)
0x000C8000          224 KB      BIOS ROM
```

Key constants (defined in `src/bootloader/stage1/stage1.asm`):

```asm
STAGE2_LOAD_SEGMENT  equ 0x2000   ; Segment where Stage 2 is loaded
STAGE2_LOAD_OFFSET   equ 0        ; Offset within that segment
; Physical address = segment × 16 + offset = 0x20000
```

Key constants (defined in `src/bootloader/stage2/std/memory_defines.h`):

```c
#define MEMORY_FAT_ADDR  ((void far *)0x00500000)  // segment:offset notation
#define MEMORY_FAT_SIZE  0x00010000                // 64 KB for FAT data
```

---

## Component Descriptions

### `src/bootloader/stage1/stage1.asm`

The **Stage 1 bootloader** is exactly 512 bytes. It is assembled with NASM to a flat binary and written into sector 0 of the floppy image. The last two bytes are the boot signature `0xAA55` required by the BIOS.

Responsibilities:
- Set up segment registers (`DS`, `ES`, `SS`) and stack pointer
- Ensure execution continues at the correct physical address (handles both `0000:7C00` and `07C0:0000` BIOS conventions)
- Query drive geometry using BIOS INT 13h/AH=08h
- Read the FAT12 root directory into a scratch buffer
- Search the root directory for `STAGE2  BIN` (8.3 filename format)
- Load the FAT table into the scratch buffer
- Follow the cluster chain and load each Stage 2 cluster to `0x2000:0x0000`
- Perform a far jump to Stage 2

Key routines:
| Routine         | Description                                        |
|-----------------|----------------------------------------------------|
| `puts`          | Print null-terminated string via INT 10h/AH=0Eh   |
| `lba_to_chs`    | Convert Logical Block Address to Cylinder/Head/Sector |
| `disk_read`     | Read sectors from disk using INT 13h with 3 retries |
| `disk_reset`    | Reset floppy controller using INT 13h/AH=00h       |

---

### `src/bootloader/stage2/stage2.asm`

A minimal 16-bit assembly **entry stub** for Stage 2. Because Stage 2 is written in C and compiled with the Watcom C compiler, a small assembly stub is needed to:
1. Disable interrupts (`cli`)
2. Set up a valid stack frame
3. Pass the boot drive number (from `DL`) to the C function `_cstart_`

The linker places this `_ENTRY` section first in the binary so it runs immediately on entry.

---

### `src/bootloader/stage2/stage2.c`

The **Stage 2 C main file**. Compiled with Watcom's 16-bit C compiler (`wcc`) targeting the small memory model. It:
1. Initialises the `DISK` abstraction using `DISK_Initialize()`
2. Initialises the FAT12 driver using `FAT_Initialize()`
3. Opens the root directory and prints the first 3 entries
4. Opens `test.txt` and streams its contents to the screen
5. Loops forever (kernel loading is the intended next step)

---

### `src/bootloader/stage2/std/`

A minimal **standard library** implemented from scratch for 16-bit real mode. No libc is available in this environment.

| File           | Contents                                                    |
|----------------|-------------------------------------------------------------|
| `stdint.h`     | `uint8_t` … `uint64_t`, `bool`, `size_t`, `NULL`           |
| `stdio.c/h`    | `putc`, `puts`, `sputs`, full `printf` with format states   |
| `string.c/h`   | `strchr`, `strcpy`, `strlen`                                |
| `memory.c/h`   | `memcpy`, `memset`, `memcmp` (using `far` pointers)         |
| `ctype.c/h`    | `isLower`, `isUpper`, `toUpper`, `toLower`                  |
| `utility.c/h`  | `align`, `min`, `max`                                       |
| `disk.c/h`     | `DISK` struct, `DISK_Initialize`, `DISK_Read`, `DISK_LBA2CHS` |
| `fat.c/h`      | Full FAT12 driver (see [FAT12 section](#fat12-filesystem-implementation)) |
| `x86.asm`      | BIOS call wrappers callable from C (INT 10h, INT 13h)       |
| `x86.h`        | C declarations for the assembly functions                   |
| `memory_defines.h` | Physical memory layout constants                        |

---

### `src/kernel/kernel.nasm`

A minimal **kernel stub** assembled as a flat binary. Currently it:
- Prints `"Hello, world! Its me, the KERNEL!"` using INT 10h
- Halts the CPU

This is the foundation for future kernel development. It is copied into the FAT12 image but is not yet loaded by Stage 2.

---

### `tools/fat/fat.c`

A host-side **FAT12 image inspector** tool. Run on the development machine to verify that files were written correctly to the floppy image. Parses the boot sector, reads the FAT and root directory, and extracts a named file.

Usage:
```
./fat <disk_image> <filename>
```

---

## FAT12 Filesystem Implementation

FAT12 (File Allocation Table, 12-bit entries) is the filesystem used on the 1.44 MB floppy image. The driver lives in `src/bootloader/stage2/std/fat.c`.

### Disk Layout

```
+-------------------+---+---+---+-----+-----+------------------+
| Boot Sector / BPB | F | F | F | Root| Root|   Data Region    |
| (Stage 1 code)    | A | A | A | Dir | Dir |  (clusters 2+)   |
| Sector 0          | T | T | T |     |     |                  |
|                   | 1 | 1 | 2 |     |     |  stage2.bin      |
|                   |   |   |   |     |     |  kernel.bin      |
|                   |   |   |   |     |     |  test.txt        |
+-------------------+---+---+---+-----+-----+------------------+
  Sector:     0       1       9   10     23    24
              |<-- 9 sectors -->|<-- 14 s -->|
              FAT 1 (sectors 1-9)
                                FAT 2 (sectors 10-18) [backup]
```

Actual layout for a 1.44 MB floppy (`bdb_` values in Stage 1):

| Parameter                | Value  |
|--------------------------|--------|
| Bytes per sector         | 512    |
| Sectors per cluster      | 1      |
| Reserved sectors         | 1      |
| FAT copies               | 2      |
| Root directory entries   | 224    |
| Total sectors            | 2880   |
| Sectors per FAT          | 9      |
| Sectors per track        | 18     |
| Heads                    | 2      |
| Media descriptor         | 0xF0   |

### BIOS Parameter Block (BPB)

The BPB is embedded in the first 62 bytes of sector 0, right after the 3-byte jump instruction. The `FAT_BootSector` struct (`fat.h`) maps directly onto it.

### FAT Table

Each entry in the FAT is 12 bits. For cluster N:
- Entries at even positions: `value = FAT[N*3/2] & 0x0FFF`
- Entries at odd positions:  `value = FAT[N*3/2] >> 4`

Special values:

| Range          | Meaning                          |
|----------------|----------------------------------|
| 0x000          | Free cluster                     |
| 0x001          | Reserved                         |
| 0x002–0xFEF    | Next cluster in chain            |
| 0xFF0–0xFF6    | Reserved                         |
| 0xFF7          | Bad cluster                      |
| 0xFF8–0xFFF    | End of cluster chain (EOF)       |

### Root Directory

Immediately follows the FAT copies. Contains up to 224 fixed-size (32-byte) entries. Each `FAT_DirectoryEntry` holds:
- `Name[11]` — 8.3 filename, uppercase, space-padded
- `Attributes` — file attributes (read-only, hidden, system, directory, archive)
- `FirstClusterLow` — starting cluster number (12-bit in FAT12)
- `Size` — file size in bytes

### Data Region

Data starts at cluster 2. The `FAT_ClusterToLba()` function converts a cluster number to an LBA sector:

```
LBA = data_start + (cluster - 2) * sectors_per_cluster
data_start = reserved_sectors + (fat_count * sectors_per_fat)
           + ceil(dir_entries * 32 / bytes_per_sector)
           = 1 + (2 × 9) + ceil(224 × 32 / 512)
           = 1 + 18 + 14
           = 33    (but Stage 1 uses +31 because data region starts at sector 33
                    with cluster 2, so: LBA = cluster + 31)
```

### Driver API

```c
// Initialise the FAT driver — reads boot sector, FAT, and root directory
bool FAT_Initialize(DISK *disk);

// Open a file or directory by path (e.g. "/" or "test.txt")
FAT_File far *FAT_Open(DISK *disk, const char *path);

// Read up to byteCount bytes from an open file
uint32_t FAT_Read(DISK *disk, FAT_File far *file, uint32_t byteCount, void *buffer);

// Read the next directory entry from an open directory handle
bool FAT_ReadEntry(DISK *disk, FAT_File far *file, FAT_DirectoryEntry *directoryEntry);

// Close a file handle
void FAT_Close(FAT_File far *file);
```

The driver allocates its working memory at the fixed address `MEMORY_FAT_ADDR` (0x00500000 in segment:offset notation = physical 0x00000500). This includes the boot sector copy, the full FAT table cache, and a table of up to 16 open file handles.

---

## Key Design Decisions

### 1. Two-stage bootloader

The MBR is limited to 512 bytes — not enough for a full FAT12 driver. Stage 1 is kept minimal: it only needs to find and load Stage 2. Stage 2 has no size constraint and can be a full C program.

### 2. Watcom C Compiler for Stage 2

Stage 2 is compiled with the **Open Watcom C compiler** (`wcc`) rather than GCC because:
- It targets the 16-bit small memory model (`-ms`) directly, producing correct real-mode code without a GCC cross-compile toolchain
- It supports the `far` pointer qualifier needed for segment:offset memory addressing
- The `_cdecl` calling convention makes C↔ASM interop straightforward

### 3. FAT data at fixed physical address 0x00000500

The FAT driver (`fat.c`) allocates its working buffers at a hard-coded physical address (`MEMORY_FAT_ADDR = 0x00500000` in segment:offset = 0x00000500 physical). This region is above the BIOS Data Area and below Stage 2, making it safe in real mode without a dynamic allocator.

### 4. LBA addressing internally, CHS for BIOS calls

All internal disk logic uses Logical Block Addresses (LBA). The `DISK_LBA2CHS` / `lba_to_chs` functions convert to Cylinder/Head/Sector (CHS) format just before the INT 13h BIOS call, keeping the rest of the code geometry-independent.

### 5. Far pointers for cross-segment access

Real mode uses 20-bit physical addresses via `segment:offset` pairs. The FAT driver uses `void far *` and `FAT_File far *` to correctly address memory across segment boundaries (e.g. accessing FAT data at 0x500 while Stage 2 is mapped at 0x20000).

### 6. 3-retry logic on disk reads

Floppy drives are mechanically unreliable. Both Stage 1 (`disk_read`) and Stage 2 (`DISK_Read`) retry failed reads up to 3 times, resetting the controller between attempts using INT 13h/AH=00h.
