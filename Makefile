SRC_DIR := src
BUILD_DIR := build
TOOLS_DIR := tools

CC := gcc
ASM := nasm

.PHONY: all floppy_image kernel bootloader clean always tools_fat

all: floppy_image tools_fat

# Floppy image
floppy_image: ${BUILD_DIR}/main_floppy.img

${BUILD_DIR}/main_floppy.img: bootloader kernel
# Initialize the image
	dd if=/dev/zero of=${BUILD_DIR}/main_floppy.img bs=512 count=2880

# Create FAT12 filesystem
	mkfs.fat -F 12 -n "GOS" ${BUILD_DIR}/main_floppy.img

# Write the bootloader
	dd if=${BUILD_DIR}/stage1.bin of=${BUILD_DIR}/main_floppy.img conv=notrunc

# Copy the stage 2 bootloader
	mcopy -i ${BUILD_DIR}/main_floppy.img ${BUILD_DIR}/stage2.bin "::stage2.bin"

# Copy the kernel
	mcopy -i ${BUILD_DIR}/main_floppy.img ${BUILD_DIR}/kernel.bin "::kernel.bin"


# Bootloader
bootloader: stage1 stage2

# Stage 1 bootloader
stage1: ${BUILD_DIR}/stage1.bin

${BUILD_DIR}/stage1.bin: always
	${MAKE} -C ${SRC_DIR}/bootloader/stage1 BUILD_DIR=${abspath ${BUILD_DIR}}

# Stage 2 bootloader
stage2: ${BUILD_DIR}/stage2.bin

${BUILD_DIR}/stage2.bin: always
	${MAKE} -C ${SRC_DIR}/bootloader/stage2 BUILD_DIR=${abspath ${BUILD_DIR}}

# Kernel
kernel: ${BUILD_DIR}/kernel.bin

${BUILD_DIR}/kernel.bin: always
	${MAKE} -C ${SRC_DIR}/kernel BUILD_DIR=${abspath ${BUILD_DIR}}

#
# Tools
#

# FAT
tools_fat: ${BUILD_DIR}/tools/fat
${BUILD_DIR}/tools/fat: always ${TOOLS_DIR}/fat/fat.c
	mkdir -p ${BUILD_DIR}/tools
	${CC} -g -o ${BUILD_DIR}/tools/fat ${TOOLS_DIR}/fat/fat.c

# Always
alwyas:
	mkdir -p ${BUILD_DIR}

# Clean
clean:
	${MAKE} -C ${SRC_DIR}/bootloader/stage1 BUILD_DIR=${abspath ${BUILD_DIR}} clean
	${MAKE} -C ${SRC_DIR}/bootloader/stage2 BUILD_DIR=${abspath ${BUILD_DIR}} clean
	${MAKE} -C ${SRC_DIR}/kernel BUILD_DIR=${abspath ${BUILD_DIR}} clean
	rm -rf ${BUILD_DIR}/*
