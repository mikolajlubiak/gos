SRC_DIR := src
BUILD_DIR := build

.PHONY: all floppy_image kernel bootloader clean always


# Floppy image
floppy_image: ${BUILD_DIR}/main_floppy.img

${BUILD_DIR}/main_floppy.img: bootloader kernel
# Initialize the image
	dd if=/dev/zero of=${BUILD_DIR}/main_floppy.img bs=512 count=2880

# Create FAT12 filesystem
	mkfs.fat -F 12 -n "GOS" ${BUILD_DIR}/main_floppy.img

# Write the bootloader
	dd if=${BUILD_DIR}/bootloader.bin of=${BUILD_DIR}/main_floppy.img conv=notrunc

# Copy the kernel
	mcopy -i ${BUILD_DIR}/main_floppy.img ${BUILD_DIR}/kernel.bin "::kernel.bin"


# Bootloader
bootloader: ${BUILD_DIR}/bootloader.bin

${BUILD_DIR}/bootloader.bin: always
	nasm ${SRC_DIR}/bootloader/boot.nasm -f bin -o ${BUILD_DIR}/bootloader.bin


# Kernel
kernel: ${BUILD_DIR}/kernel.bin

${BUILD_DIR}/kernel.bin: always
	nasm ${SRC_DIR}/kernel/kernel.nasm -f bin -o ${BUILD_DIR}/kernel.bin


# Always
alwyas:
	mkdir -p ${BUILD_DIR}

# Clean
clean:
	rm -rf ${BUILD_DIR}/*
