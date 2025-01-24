ifeq ($(strip $(DEVKITAMATEUR)),)
$(error "Set DEVKITAMATEUR in your environment.")
endif

PREFIX = $(DEVKITAMATEUR)/bin/powerpc-none-elf-

CFLAGS = -mcpu=750 -m32 -mhard-float -mno-eabi -mno-sdata
CFLAGS += -ffreestanding -ffunction-sections
CFLAGS += -Wall -Wextra -O0 -pipe
ASFLAGS =
LDFLAGS = -mcpu=750 -m32 -n -z muldefs -nostartfiles -nodefaultlibs -Wl,-gc-sections

