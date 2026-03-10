# gos
Yours truly, Gall Operating System

A from-scratch x86 operating system demonstrating low-level systems programming: a 2-stage real-mode bootloader written in NASM and C, a FAT12 filesystem driver, BIOS interrupt usage, disk I/O, and a minimal kernel — all designed to run on bare x86 hardware or emulators. Built as a learning resource and portfolio project showcasing bootloader development, filesystem implementation, and early-stage kernel design.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for a deep dive into the design, memory map, and boot process.

## Features

- **2-stage bootloader** — Stage 1 fits in the 512-byte MBR; Stage 2 is a full C program loaded by Stage 1
- **Real-mode execution** — Runs entirely in 16-bit x86 real mode using BIOS services
- **FAT12 filesystem support** — Full FAT12 driver: BPB parsing, FAT table traversal, root directory search, cluster chain following, and file reading
- **BIOS interrupts** — Uses INT 10h for video output and INT 13h for disk I/O
- **Disk I/O with retry logic** — LBA-to-CHS conversion, 3-attempt retry on floppy reads
- **Kernel loading** — Stage 2 locates and loads the kernel binary from the FAT12 filesystem
- **Minimal standard library** — Custom implementations of `printf`, `memcpy`, `memset`, string functions, and more for 16-bit real mode

## Tech Stack

`x86 Assembly (NASM)` `C` `Watcom C Compiler` `Make` `QEMU` `Bochs`

## Build

Tested on Linux.

- Install necessary packages (different commands based on your distribution)
  - Fedora:
    - `sudo dnf install nasm gcc make qemu`
  - Arch:
    - `sudo pacman -S --needed nasm gcc make qemu`
  - Ubuntu:
    - `sudo apt install nasm gcc make qemu-system-x86`
- `git clone https://github.com/mikolajlubiak/gos`
- `cd gos`
- `make`

## Usage

- Run the OS image in QEMU:
  - `./run.sh`
- Debug using Bochs:
  - `./debug.sh`
