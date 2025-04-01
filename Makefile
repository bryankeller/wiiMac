include .env

include broadway.mk

DEFINES = -DLACKS_SYS_TYPES_H -DLACKS_ERRNO_H -DLACKS_STDLIB_H -DLACKS_STRING_H -DLACKS_STRINGS_H -DLACKS_UNISTD_H -DCAN_HAZ_IRQ
LDSCRIPT = mini.ld
LIBS = -lgcc

TARGET = bin/wiiMac.elf

OBJS = bin/realmode.o bin/crt0.o bin/main.o bin/string.o bin/sync.o bin/time.o bin/printf.o bin/input.o \
	bin/exception.o bin/exception_target.o bin/malloc.o bin/gecko.o bin/video_low.o \
	bin/ipc.o bin/mini_ipc.o bin/nandfs.o bin/ff.o bin/diskio.o bin/fat.o bin/font.o bin/console.o \
	bin/irq.o bin/sha1.o bin/macho_loader.o

include src/usb/Makefile

include common.mk

install:
	$(DEVKITAMATEUR)/bin/bootmii -p $(TARGET)

.PHONY: install
