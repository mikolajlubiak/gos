# Boot Process

This document is a step-by-step walkthrough of everything that happens from the moment the machine is powered on to when the kernel is running. It covers register state at each transition, BIOS interrupts used, and how Stage 1 locates Stage 2 using the FAT12 filesystem.

See [ARCHITECTURE.md](ARCHITECTURE.md) for the memory map and overall component overview.

---

## Table of Contents

1. [Overview](#overview)
2. [Stage 0 — BIOS](#stage-0--bios)
3. [Stage 1 — MBR Bootloader](#stage-1--mbr-bootloader)
   - [Startup & Segment Initialisation](#startup--segment-initialisation)
   - [Drive Geometry Query](#drive-geometry-query)
   - [Reading the FAT Root Directory](#reading-the-fat-root-directory)
   - [Searching for Stage 2](#searching-for-stage-2)
   - [Loading the FAT Table](#loading-the-fat-table)
   - [Following the Cluster Chain](#following-the-cluster-chain)
   - [Jumping to Stage 2](#jumping-to-stage-2)
4. [Stage 2 — Entry Stub](#stage-2--entry-stub)
5. [Stage 2 — C Bootloader](#stage-2--c-bootloader)
   - [Disk Initialisation](#disk-initialisation)
   - [FAT12 Initialisation](#fat12-initialisation)
   - [Reading Files](#reading-files)
6. [Kernel](#kernel)
7. [BIOS Interrupts Reference](#bios-interrupts-reference)
8. [Register State at Each Transition](#register-state-at-each-transition)

---

## Overview

```
Power On
  |
  v  POST + hardware init
BIOS
  |
  | Loads MBR (sector 0) to 0x0000:0x7C00
  | Checks boot signature 0xAA55
  v
Stage 1  (0x0000:0x7C00, 512 bytes, NASM)
  |  1. Init segments & stack
  |  2. Query drive geometry       [INT 13h / AH=08h]
  |  3. Read root directory        [INT 13h / AH=02h]
  |  4. Search for STAGE2  BIN
  |  5. Read FAT table             [INT 13h / AH=02h]
  |  6. Follow cluster chain, load Stage 2 to 0x2000:0x0000
  v
Stage 2 entry  (0x2000:0x0000, NASM stub)
  |  Set up stack, push DL (boot drive), call _cstart_
  v
Stage 2 C  (0x2000:xxxx, Watcom C)
  |  1. DISK_Initialize            [INT 13h / AH=08h]
  |  2. FAT_Initialize             [INT 13h / AH=02h]
  |  3. Print root directory
  |  4. Read & print test.txt
  v
(Future) Jump to kernel
  v
Kernel  (kernel.nasm)
     Prints "Hello, world!"
     HLT
```

---

## Stage 0 — BIOS

When the CPU resets it begins executing at physical address `0xFFFF0` (the reset vector). The BIOS:

1. Runs the Power-On Self Test (POST) — tests memory, peripherals, etc.
2. Enumerates boot devices in priority order.
3. Reads the first sector (512 bytes) of the selected boot device into memory at `0x0000:0x7C00`.
4. Verifies that the last two bytes of the sector are `0x55` and `0xAA` (the boot signature).
5. If valid, transfers control with a far jump or `retf` to `0x0000:0x7C00`.

At entry to Stage 1, the BIOS places the boot drive number in `DL` (e.g. `0x00` for floppy A:).

---

## Stage 1 — MBR Bootloader

**Source:** `src/bootloader/stage1/stage1.asm`
**Load address:** `0x0000:0x7C00`
**Size:** exactly 512 bytes

### Startup & Segment Initialisation

```asm
org 0x7C00
bits 16

main:
    xor ax, ax
    mov ds, ax          ; DS = 0
    mov es, ax          ; ES = 0
    mov ss, ax          ; SS = 0
    mov sp, 0x7C00      ; Stack grows down from 0x7C00

    ; Normalise CS:IP — some BIOSes jump to 07C0:0000 instead of 0000:7C00
    push es
    push word .after
    retf                ; Far return to 0000:0x7C00

.after:
    mov [ebr_drive_number], dl  ; Save BIOS boot drive number
```

After this, all segment registers are 0 and the stack is below the bootloader.

The FAT12 BIOS Parameter Block (BPB) is embedded at offset 3 in the sector, right after the 3-byte JMP instruction. It is populated at assembly time with the correct floppy geometry values.

### Drive Geometry Query

```asm
    push es
    mov ah, 08h         ; INT 13h / Get Drive Parameters
    int 13h
    jc floppy_error
    pop es

    and cl, 0x3F        ; CL[0:5] = max sector number
    xor ch, ch
    mov [bdb_sectors_per_track], cx

    inc dh              ; DH = max head index → head count = DH + 1
    mov [bdb_head_count], dh
```

**INT 13h / AH=08h** returns the actual drive geometry. Using the real geometry (rather than the BPB values alone) ensures correct LBA→CHS conversion on any machine.

### Reading the FAT Root Directory

The root directory starts right after the reserved sectors and the two FAT copies:

```
Root directory LBA = reserved_sectors + fat_count × sectors_per_fat
                   = 1 + 2 × 9 = 19
Root directory size (sectors) = ceil(dir_entries × 32 / bytes_per_sector)
                              = ceil(224 × 32 / 512) = 14
```

Stage 1 calls `disk_read` to load these 14 sectors into the in-memory `buffer` area (located just past the boot signature at `0x7E00`).

### Searching for Stage 2

The root directory contains up to 224 32-byte entries. Each entry's first 11 bytes hold the filename in **8.3 padded uppercase** format (no dot separator; spaces as padding).

Stage 1 compares each entry to `"STAGE2  BIN"` (11 bytes, note double space) using `REPE CMPSB`:

```asm
file_stage2_bin: db "STAGE2  BIN"

.stage2_search:
    mov si, file_stage2_bin
    mov cx, 11
    push di
    repe cmpsb          ; Compare 11 bytes
    pop di
    je .stage2_found    ; Jump if all 11 bytes matched
    add di, 32          ; Next directory entry
    inc bx
    cmp bx, [bdb_dir_entries]
    jl .stage2_search
    jmp stage2_not_found_error
```

When found, the first cluster number is read from bytes 26–27 of the directory entry:

```asm
.stage2_found:
    mov ax, [di + 26]           ; FirstClusterLow
    mov [stage2_cluster], ax
```

### Loading the FAT Table

Before following the cluster chain, Stage 1 needs the FAT table. It reads `sectors_per_fat` (9) sectors starting from sector 1 into the same scratch buffer:

```asm
    mov ax, [bdb_reserved_sectors]   ; ax = 1
    mov bx, buffer
    mov cl, [bdb_sectors_per_fat]    ; cl = 9
    mov dl, [ebr_drive_number]
    call disk_read
```

### Following the Cluster Chain

With the FAT in memory, Stage 1 loads Stage 2 cluster by cluster to `0x2000:0x0000`.

FAT12 uses 12-bit entries packed into a byte array. For a given cluster N:
- If N is even: `next = FAT[N×3/2] & 0x0FFF`
- If N is odd:  `next = FAT[N×3/2] >> 4`

The cluster-to-LBA conversion adds 31 (the data region offset for this image, cluster 2 maps to sector 33, so cluster N maps to sector N+31):

```asm
.load_stage2_loop:
    mov ax, [stage2_cluster]
    add ax, 31              ; Convert cluster to LBA (data starts at cluster 2 = sector 33)
    mov cl, 1               ; Read 1 sector
    mov dl, [ebr_drive_number]
    call disk_read
    add bx, [bdb_bytes_per_sector]   ; Advance write pointer by 512

    ; Compute next cluster from FAT
    mov ax, [stage2_cluster]
    mov cx, 3
    mul cx
    mov cx, 2
    div cx                  ; AX = byte offset into FAT, DX = remainder (0=even, 1=odd)
    mov si, buffer
    add si, ax
    mov ax, [ds:si]         ; Read 16 bits (12-bit entry spans at most 2 bytes)

    or dx, dx
    jz .even
.odd:
    shr ax, 4               ; Odd cluster: upper 12 bits
    jmp .next_cluster_after
.even:
    and ax, 0x0FFF          ; Even cluster: lower 12 bits

.next_cluster_after:
    cmp ax, 0x0FF8
    jae .read_finish        ; 0xFF8-0xFFF = end of chain
    mov [stage2_cluster], ax
    jmp .load_stage2_loop
```

### Jumping to Stage 2

After loading all clusters, Stage 1 sets `DS` and `ES` to `0x2000` and performs a far jump:

```asm
    mov ax, STAGE2_LOAD_SEGMENT   ; 0x2000
    mov ds, ax
    mov es, ax
    jmp STAGE2_LOAD_SEGMENT:STAGE2_LOAD_OFFSET   ; Far jump to 0x2000:0x0000
```

### Error Handling

All error paths (`floppy_error`, `stage2_not_found_error`) print a message via `INT 10h` and call `wait_key_and_reboot`, which waits for a keypress (`INT 16h`) and then jumps to the BIOS reset vector (`0xFFFF:0x0000`).

---

## Stage 2 — Entry Stub

**Source:** `src/bootloader/stage2/stage2.asm`
**Section:** `_ENTRY` (placed first by the linker)

```asm
bits 16
section _ENTRY class=code

extern _cstart_
global entry

entry:
    cli                 ; Disable interrupts while setting up stack
    mov ax, ds
    mov ss, ax          ; SS = DS (both already set to 0x2000 by Stage 1)
    mov sp, 0           ; SP = 0 (stack wraps to top of the 64KB segment)
    mov bp, sp
    sti                 ; Re-enable interrupts

    xor dh, dh
    push dx             ; Push boot drive number (DL preserved from Stage 1)
    call _cstart_       ; Call C entry point

    cli
    hlt
```

The linker script (`linker.lnk`) ensures `_ENTRY` is the very first section in the binary, so `entry` is at offset 0 and executes immediately on the far jump from Stage 1.

---

## Stage 2 — C Bootloader

**Source:** `src/bootloader/stage2/stage2.c`
**Compiler:** Watcom C (`wcc`), 16-bit small model

### Disk Initialisation

```c
DISK disk;
if (!DISK_Initialize(&disk, bootDrive)) {
    puts("Disk init error\n");
    goto end;
}
```

`DISK_Initialize` calls `x86_Disk_GetDriveParams` (which wraps INT 13h/AH=08h) to get the drive geometry and stores it in the `DISK` struct.

### FAT12 Initialisation

```c
if (!FAT_Initialize(&disk)) {
    puts("FAT init error\n");
    goto end;
}
```

`FAT_Initialize` performs three operations:
1. `FAT_ReadBootSector` — reads sector 0 into `FAT_Data.BS` at `MEMORY_FAT_ADDR`
2. `FAT_ReadFat` — reads the FAT table into the buffer immediately following the boot sector
3. Opens the root directory as a pseudo file handle (`ROOT_DIRECTORY_HANDLE = -1`)

All data is stored at the fixed physical address `0x00000500` (MEMORY_FAT_ADDR, expressed as segment:offset `0x0050:0x0000`).

### Reading Files

Stage 2 demonstrates two operations:

**1. List root directory:**
```c
FAT_File far *fd = FAT_Open(&disk, "/");
FAT_DirectoryEntry entry;
uint8_t i = 0;
while (FAT_ReadEntry(&disk, fd, &entry) && i++ < 3) {
    sputs(entry.Name, 11);
    putc('\n');
}
FAT_Close(fd);
```

**2. Read a file:**
```c
fd = FAT_Open(&disk, "test.txt");
char buffer[512];
uint32_t read;
while ((read = FAT_Read(&disk, fd, sizeof(buffer), buffer))) {
    for (uint32_t i = 0; i < read; i++)
        putc(buffer[i]);
}
FAT_Close(fd);
```

`FAT_Open` searches the root directory for the given filename, converts it to 8.3 uppercase format, and returns a `FAT_File far *` handle. `FAT_Read` reads data cluster by cluster, following the FAT chain and calling `DISK_Read` for each new sector.

---

## Kernel

**Source:** `src/kernel/kernel.nasm`

The kernel is a minimal NASM binary assembled at `org 0x0`. It prints a hello message using INT 10h and halts:

```asm
org 0x0
bits 16

start:
    mov si, msg_hello
    call puts
halt:
    cli
    hlt

msg_hello: db "Hello, world! It's me, the KERNEL!", 0x0D, 0x0A, 0
```

The kernel is built into `build/kernel.bin` and written to the FAT12 image as `kernel.bin`. Loading the kernel from Stage 2 is the planned next step in development.

---

## BIOS Interrupts Reference

| Interrupt | AH   | Function                        | Used in                  |
|-----------|------|---------------------------------|--------------------------|
| INT 10h   | 0Eh  | Write character in TTY mode     | Stage 1 `puts`, Stage 2 `x86_Video_WriteCharTeletype` |
| INT 13h   | 00h  | Reset disk controller           | Stage 1 `disk_reset`, Stage 2 `x86_Disk_Reset`        |
| INT 13h   | 02h  | Read sectors from disk          | Stage 1 `disk_read`, Stage 2 `x86_Disk_Read`          |
| INT 13h   | 08h  | Get drive parameters            | Stage 1 `main`, Stage 2 `x86_Disk_GetDriveParams`     |
| INT 16h   | 00h  | Wait for keypress               | Stage 1 `wait_key_and_reboot`                         |

---

## Register State at Each Transition

### BIOS → Stage 1

| Register | Value                  | Notes                        |
|----------|------------------------|------------------------------|
| `CS`     | `0x0000`               | Set by BIOS                  |
| `IP`     | `0x7C00`               | Entry point                  |
| `DL`     | Boot drive number      | `0x00` = floppy A:           |
| `DS/ES/SS` | Undefined            | Stage 1 initialises these    |
| `SP`     | Undefined              | Stage 1 sets to `0x7C00`     |

### Stage 1 → Stage 2

| Register | Value                  | Notes                              |
|----------|------------------------|------------------------------------|
| `CS`     | `0x2000`               | Set by `STAGE2_LOAD_SEGMENT`       |
| `IP`     | `0x0000`               | `STAGE2_LOAD_OFFSET`               |
| `DS`     | `0x2000`               | Set by Stage 1 before far jump     |
| `ES`     | `0x2000`               | Set by Stage 1 before far jump     |
| `DL`     | Boot drive number      | Preserved from BIOS, used by Stage 2 entry |

### Stage 2 ASM entry → `_cstart_`

| Register/Stack | Value               | Notes                               |
|----------------|---------------------|-------------------------------------|
| `SS`           | `0x2000`            | Same as `DS`                        |
| `SP`           | `0x0000`            | Top of 64KB segment (wraps)         |
| Stack `[SP+2]` | `bootDrive` (uint16) | `DX` pushed, `DH` zeroed so `DL` = drive |
