BUILD_DIR ?= build
ASM ?= nasm

.PHONY: all clean

all: stage1

# Stage 1 bootloader
stage1: ${BUILD_DIR}/stage1.bin

${BUILD_DIR}/stage1.bin:
	${ASM} stage1.asm -f bin -o ${BUILD_DIR}/stage1.bin

# Clean
clean:
	rm ${BUILD_DIR}/stage1.bin
