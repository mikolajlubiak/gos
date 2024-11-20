BUILD_DIR ?= build
ASM ?= nasm

.PHONY: all clean

all: kernel

# Kernel
kernel: ${BUILD_DIR}/kernel.bin

${BUILD_DIR}/kernel.bin:
	${ASM} kernel.nasm -f bin -o ${BUILD_DIR}/kernel.bin

# Clean
clean:
	rm ${BUILD_DIR}/kernel.bin
