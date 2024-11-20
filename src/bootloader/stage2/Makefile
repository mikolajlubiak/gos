BUILD_DIR ?= build

CC16 ?= /usr/bin/watcom/binl64/wcc
CFLAGS16 ?= -d3 -s -wx -ms -zl -zq

ASM ?= nasm
ASMFLAGS ?= -f obj

LD16 ?= /usr/bin/watcom/binl64/wlink

SOURCES_C := ${wildcard *.c std/*.c}
SOURCES_ASM := ${wildcard *.asm std/*.asm}
OBJECTS_C := ${patsubst %.c, ${BUILD_DIR}/stage2/c/%.obj, ${SOURCES_C}}
OBJECTS_ASM := ${patsubst %.asm, ${BUILD_DIR}/stage2/asm/%.obj, ${SOURCES_ASM}}

.PHONY: all stage2 clean always

all: stage2

# Stage 2 bootloader
stage2: ${BUILD_DIR}/stage2.bin

${BUILD_DIR}/stage2.bin: ${OBJECTS_ASM} ${OBJECTS_C}
	${LD16} NAME ${BUILD_DIR}/stage2.bin FILE \{${OBJECTS_ASM} ${OBJECTS_C}\} OPTION MAP=${BUILD_DIR}/stage2.map @linker.lnk

${BUILD_DIR}/stage2/c/%.obj: %.c always
	${CC16} ${CFLAGS16} -fo=$@ $<

${BUILD_DIR}/stage2/asm/%.obj: %.asm always
	${ASM} ${ASMFLAGS} -o $@ $<

# Always
always:
	mkdir -p ${BUILD_DIR}/stage2/c
	mkdir -p ${BUILD_DIR}/stage2/asm
	mkdir -p ${BUILD_DIR}/stage2/c/std
	mkdir -p ${BUILD_DIR}/stage2/asm/std


# Clean
clean:
	rm ${BUILD_DIR}/stage2.bin
