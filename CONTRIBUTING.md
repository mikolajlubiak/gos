# Contributing to GOS

Thank you for your interest in contributing to the Gall Operating System. This guide covers everything you need to set up the development environment, understand the build system, and debug effectively.

---

## Table of Contents

1. [Development Environment Setup](#development-environment-setup)
2. [Build System Walkthrough](#build-system-walkthrough)
3. [Running the OS](#running-the-os)
4. [Debugging with QEMU and Bochs](#debugging-with-qemu-and-bochs)
5. [Code Style and Conventions](#code-style-and-conventions)
6. [Project Structure](#project-structure)

---

## Development Environment Setup

### Required Tools

| Tool               | Purpose                               | Version tested |
|--------------------|---------------------------------------|----------------|
| **NASM**           | Assembles Stage 1, Stage 2 entry, kernel | 2.15+         |
| **Open Watcom**    | 16-bit C compiler for Stage 2         | 2.0            |
| **GCC**            | Compiles the host-side FAT tool       | Any modern     |
| **GNU Make**       | Build orchestration                   | 3.81+          |
| **QEMU**           | x86 emulator for running the image    | 6.0+           |
| **Bochs**          | x86 emulator with built-in debugger   | 2.7+           |
| **mtools**         | `mcopy` for writing files to FAT image | 4.0+          |

### Installation

**Fedora / RHEL:**
```bash
sudo dnf install nasm gcc make qemu-system-x86 bochs mtools
```
For Open Watcom, download from [https://github.com/open-watcom/open-watcom-v2/releases](https://github.com/open-watcom/open-watcom-v2/releases) and install to `/usr/watcom` (or update `WCC` paths in `src/bootloader/stage2/Makefile`).

**Arch Linux:**
```bash
sudo pacman -S --needed nasm gcc make qemu mtools bochs
```

**Ubuntu / Debian:**
```bash
sudo apt install nasm gcc make qemu-system-x86 bochs bochs-sdl mtools
```

### Verifying the Toolchain

```bash
nasm --version          # NASM version 2.x
wcc --version           # Open Watcom C Compiler x.x
qemu-system-i386 --version
bochs --version
mcopy --version
```

---

## Build System Walkthrough

The root `Makefile` orchestrates the entire build. Here is a breakdown of each target:

### Root Makefile (`Makefile`)

```
make                 Build everything and produce the floppy image
make floppy_image    Build and assemble the FAT12 floppy image
make bootloader      Build Stage 1 and Stage 2 only
make kernel          Build the kernel only
make tools_fat       Build the host-side FAT inspection tool
make clean           Remove all build artefacts
```

**Full build sequence:**

1. `make bootloader` → builds `build/stage1.bin` and `build/stage2.bin`
2. `make kernel` → builds `build/kernel.bin`
3. `make floppy_image`:
   - Creates a zeroed 1.44 MB image (`dd`)
   - Formats it as FAT12 (`mformat`)
   - Writes Stage 1 into the MBR sector (`dd` with `conv=notrunc`)
   - Copies `stage2.bin`, `kernel.bin`, `test.txt` into the FAT filesystem (`mcopy`)

### Stage 1 Makefile (`src/bootloader/stage1/Makefile`)

```bash
nasm -f bin src/bootloader/stage1/stage1.asm -o build/stage1.bin
```

`-f bin` produces a flat binary with no headers — exactly what the MBR requires.

### Stage 2 Makefile (`src/bootloader/stage2/Makefile`)

Stage 2 is a mixed C + assembly project. The Watcom C compiler targets 16-bit real mode:

```bash
# Compile each C file to Watcom object format
wcc -d3 -s -wx -ms -zl -zq -za99 -fo=<output.o> <source.c>

# Assemble x86.asm to Watcom object format
nasm -f obj src/bootloader/stage2/std/x86.asm -o build/x86.asm.o

# Link everything with the Watcom linker
wlink @src/bootloader/stage2/linker.lnk
```

Key compiler flags:
| Flag  | Meaning                                                    |
|-------|------------------------------------------------------------|
| `-d3` | Full debug information                                     |
| `-s`  | Disable stack overflow checks (not available in real mode) |
| `-wx` | Maximum warnings                                           |
| `-ms` | Small memory model (one code segment, one data segment)    |
| `-zl` | Remove default library references                          |
| `-zq` | Quiet mode                                                 |
| `-za99` | Enable C99 syntax                                       |

The linker script (`src/bootloader/stage2/linker.lnk`) produces a flat binary, places `_ENTRY` first, and allocates a 512-byte stack.

### Kernel Makefile (`src/kernel/Makefile`)

```bash
nasm -f bin src/kernel/kernel.nasm -o build/kernel.bin
```

---

## Running the OS

### QEMU

```bash
./run.sh
# Equivalent to:
qemu-system-i386 -fda build/main_floppy.img
```

You should see Stage 2 print the first three root directory entries and the contents of `test.txt` in the QEMU window.

### Bochs

```bash
./debug.sh
# Equivalent to:
bochs-debugger -f bochs_config -q
```

This starts Bochs with the GUI debugger enabled. See [Debugging with Bochs](#debugging-with-bochs) below.

---

## Debugging with QEMU and Bochs

### Debugging with QEMU + GDB

Start QEMU with a GDB stub:

```bash
qemu-system-i386 -fda build/main_floppy.img -s -S
```

`-s` opens a GDB server on port 1234; `-S` pauses execution at startup.

In a second terminal:

```bash
gdb
(gdb) target remote :1234
(gdb) set architecture i8086
(gdb) break *0x7c00        # Break at Stage 1 entry
(gdb) continue
(gdb) layout asm           # Show disassembly
(gdb) info registers       # Dump registers
```

Useful GDB commands for real mode:
```
x/10i $cs*16+$eip    # Disassemble at current CS:IP
x/4xb 0x7c00         # Inspect memory at Stage 1 entry
x/4xb 0x20000        # Inspect memory at Stage 2 load address
```

### Debugging with Bochs

The `bochs_config` file configures Bochs to use SDL2 with the built-in GUI debugger.

When Bochs starts, it opens a debugger window. Useful commands:

```
b 0x7c00          # Breakpoint at Stage 1 entry
b 0x20000         # Breakpoint at Stage 2 entry
c                 # Continue execution
s                 # Step one instruction
n                 # Step over (next)
r                 # Show registers
u 0x7c00 0x7c10   # Disassemble addresses 0x7c00 to 0x7c10
xp /4 0x500       # Examine physical memory at 0x500
print-stack       # Show stack contents
info break        # List breakpoints
```

To set a breakpoint on Stage 2's `_cstart_` function, first find its address:
```bash
# Check the Watcom map file or use objdump on stage2.o
```

### Debugging with Bochs — `bochs_config` options

Key settings in `bochs_config`:

```
megs: 128                           # 128 MB RAM
floppya: 1_44=build/main_floppy.img, status=inserted
boot: floppy
display_library: sdl2, options="gui_debug"   # Enable GUI debugger
```

---

## Code Style and Conventions

### Assembly (NASM)

- Use `bits 16` at the top of every file targeting real mode
- Label names: `snake_case` for regular labels, `.local_label` for local scope
- Comment every non-obvious instruction; use block comments above routines
- Preserve all modified registers that callers expect to be unchanged
- Document function parameters and return values in the comment block above the function

Example:
```asm
;   Read sectors from disk
;   Parameters:
;       ax: LBA address
;       cl: number of sectors to read
;       dl: drive number
;       es:bx: destination buffer
;
disk_read:
    push ax
    ...
    ret
```

### C (Stage 2 / tools)

- Follow C99 style; use the custom `stdint.h` types (`uint8_t`, `uint16_t`, etc.)
- Naming conventions:
  - Types/structs: `PascalCase` (e.g. `FAT_BootSector`, `DISK`)
  - Functions: `MODULE_FunctionName` (e.g. `FAT_Initialize`, `DISK_Read`)
  - Local variables: `camelCase`
  - Constants/macros: `UPPER_SNAKE_CASE`
- Use `far` pointers where cross-segment access is required
- Annotate calling convention explicitly: `_cdecl` on all public C functions called from assembly

### Makefiles

- Keep each sub-module's Makefile self-contained with explicit source and output paths
- Use `$(BUILD_DIR)` variables for output directories
- Add a `clean` target to every Makefile

### Commit Messages

- Use the imperative mood: `Add FAT_ReadFat error handling`, not `Added...`
- Reference the component in the subject: `stage1:`, `stage2:`, `kernel:`, `docs:`
- Keep subject lines under 72 characters

---

## Project Structure

```
gos/
├── Makefile                    Root build orchestrator
├── run.sh                      Run image in QEMU
├── debug.sh                    Run image in Bochs with debugger
├── bochs_config                Bochs configuration file
├── test.txt                    Test file copied into FAT image
├── docs/
│   ├── ARCHITECTURE.md         System architecture overview
│   └── BOOT_PROCESS.md         Detailed boot process walkthrough
├── src/
│   ├── bootloader/
│   │   ├── stage1/
│   │   │   ├── stage1.asm      512-byte MBR bootloader (NASM)
│   │   │   └── Makefile
│   │   └── stage2/
│   │       ├── stage2.asm      Assembly entry stub
│   │       ├── stage2.c        C bootloader main
│   │       ├── linker.lnk      Watcom linker script
│   │       ├── Makefile
│   │       └── std/
│   │           ├── stdint.h    Integer type definitions
│   │           ├── stdio.c/h   printf, putc, puts
│   │           ├── string.c/h  strchr, strcpy, strlen
│   │           ├── memory.c/h  memcpy, memset, memcmp
│   │           ├── ctype.c/h   isLower, isUpper, toUpper, toLower
│   │           ├── utility.c/h align, min, max
│   │           ├── disk.c/h    Disk abstraction (BIOS INT 13h)
│   │           ├── fat.c/h     FAT12 filesystem driver
│   │           ├── x86.asm     BIOS call implementations
│   │           ├── x86.h       C declarations for x86.asm
│   │           └── memory_defines.h  Physical memory layout
│   └── kernel/
│       ├── kernel.nasm         Minimal kernel
│       └── Makefile
└── tools/
    └── fat/
        └── fat.c               Host-side FAT12 image inspector
```
