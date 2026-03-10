# gos
Yours truly, Gall Operating System

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
