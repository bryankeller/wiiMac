/*
        BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
        Requires mini.

Copyright (C) 2008, 2009        Haxx Enterprises <bushing@gmail.com>
Copyright (C) 2009              Andre Heider "dhewg" <dhewg@wiibrew.org>
Copyright (C) 2008, 2009        Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009        Sven Peter <svenpeter@gmail.com>
Copyright (C) 2009              John Kelley <wiidev@kelley.ca>
Copyright (C) 2025              Bryan Keller <kellerbryan19@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "boot_args.h"
#include "bootmii_ppc.h"
#include "device_tree.h"
#include "string.h"
#include "ipc.h"
#include "macho_loader.h"
#include "mini_ipc.h"
#include "nandfs.h"
#include "fat.h"
#include "malloc.h"
#include "diskio.h"
#include "printf.h"
#include "video_low.h"
#include "input.h"
#include "console.h"
#include "irq.h"
#include "usb/core/core.h"
#include "usb/drivers/class/hid.h"
#include "sha1.h"
#include "hollywood.h"

#define kMacOSXSignature 0x4D4F5358

static char ascii(char s) {
	if(s < 0x20) return '.';
	if(s > 0x7E) return '.';
	return s;
}

void hexdump(void *d, int len) {
	u8 *data;
	int i, off;
	data = (u8*)d;
	for (off=0; off<len; off += 16) {

		// Print address
		void *addr = (void *)((u32)data + off);
		printf("0x%08x  ", addr);

		// Print hex interpretation
		for (i=0; i<16; i++) {
            if ((i+off)>=len) {
                printf("   ");
            } else {
                printf("%02x ", data[off+i]);
            }
        }

		printf(" ");

		// Print ASCII interpretation
		for(i=0; i<16; i++) {
			if((i+off)>=len) {
				printf(" ");
			} else {
				printf("%c", ascii(data[off+i]));
			}
		}
		printf("\n");
	}
}

static void patch_memory_trap(u32 *src) {
	src[0] = 0x7FE00008; // trap
	sync_before_exec(src, 1 * 4);
}

static void patch_memory_syscall(u32 *src) {
	src[0] = 0x44000002; // sc
	sync_before_exec(src, 1 * 4);
}

static void patch_memory_nop(u32 *src) {
	src[0] = 0x60000000; // ori, 0, 0, 0
	sync_before_exec(src, 1 * 4);
}

static void patch_exception_handler(u32 *vector) {
	// Re-enable IO BAT
	vector[0] = 0x4C00012C;
	vector[1] = 0x3C000C00;
	vector[2] = 0x6000002A;
	vector[3] = 0x7C1D83A6;
	vector[4] = 0x3C000C00;
	vector[5] = 0x600003FF;
	vector[6] = 0x7C1C83A6;
	vector[7] = 0x4C00012C;

	// Update MSR
	vector[8] = 0x7C0000A6;
	vector[9] = 0x60002030;
	vector[10] = 0x7C000124;

	// Blink drive light
	vector[11] = 0x3CA00D80;
	vector[12] = 0x60A500C0;
	vector[13] = 0x80850000;
	vector[14] = 0x7C0004AC;
	vector[15] = 0x68840020;
	vector[16] = 0x90850000;
	vector[17] = 0x7C0004A6;

	// Infinite loop
	vector[18] = 0x48000000;

	sync_before_exec(vector, 19 * 4);
}

static void patch_cnputc() {
	u32 *src = (u32*)0x000e2e4c;//0x000a473c;  (vanilla)
	u32 dst = (u32)(void*)gecko_putc;
	u32 offset = (dst - (u32)src) & 0x03FFFFFC;

	src[0] = 0x48000000 | offset; // b

	sync_before_exec(src, 1 * 4);
}

static void patch_ppc_init_io_bat_setup() {
	u32 *src = (u32*)0x000c6590; //0x0008a984; (vanilla)

	// Enable IO BAT
	src[0] = 0x4C00012C;
	src[1] = 0x3C000C00;
	src[2] = 0x6000002A;
	src[3] = 0x7C1D83A6;
	src[4] = 0x3C000C00;
	src[5] = 0x600003FF;
	src[6] = 0x7C1C83A6;
	src[7] = 0x4C00012C;

	// ori 0, 0, 0
	src[8] = 0x60000000;
	src[9] = 0x60000000;
	src[10] = 0x60000000;
	src[11] = 0x60000000;
	src[12] = 0x60000000;
	src[13] = 0x60000000;
	src[14] = 0x60000000;
	src[15] = 0x60000000;
	src[16] = 0x60000000;
	src[17] = 0x60000000;
	src[18] = 0x60000000;
	src[19] = 0x60000000;
	src[20] = 0x60000000;
	src[21] = 0x60000000;
	src[22] = 0x60000000;
	src[23] = 0x60000000;
	src[24] = 0x60000000;
	src[25] = 0x60000000;
	src[26] = 0x60000000;
	// src[27] = 0x60000000;
	// src[28] = 0x60000000;
	// src[29] = 0x60000000;
	// src[30] = 0x60000000;

	sync_before_exec(src, 27 * 4);
}

static int start_mach_kernel() {
	long msr;

	printf("\n");
	printf("Calling Mach Kernel @ 0x%08x; boot args: 0x%08x, signature: %08x\n\n", kernel_entry_point, boot_args_address, kMacOSXSignature);

	msr = 0x00001000;
	__asm__ volatile("mtmsr %0" : : "r" (msr));
	__asm__ volatile("isync");

	// Make sure everything get sync'd up.
	__asm__ volatile("isync");
	__asm__ volatile("sync");
	__asm__ volatile("eieio");

	(*(void (*)())kernel_entry_point)(boot_args_address, kMacOSXSignature);

	return -1;
}

int main(void) {
	// Cannot gecko_init without delay
	udelay(1000000);
	gecko_init();

	exception_init();

	irq_initialize();
	irq_bw_enable(BW_PI_IRQ_RESET);
	irq_bw_enable(BW_PI_IRQ_HW); //hollywood pic

	ipc_initialize();
	ipc_slowping();

	int vmode = -1;
	init_fb(vmode);
	VIDEO_Init(vmode);
	VIDEO_SetFrameBuffer(get_xfb());
	VISetupEncoder();

	printf("\n");
	printf("wiiMac - A Mac OS X bootloader for the Nintendo Wii\n");
	printf("(c) 2025 Bryan Keller - @blk19_\n");

	int ret;

	printf("\n");
	ret = load_mach_kernel("/mk");
	if (ret != 0) {
		return -1;
	}

	printf("Loaded Mach Kernel\n");

	printf("\n");
	printf("Setting up device tree and boot args...");

	set_up_boot_args();

	printf("\n");
	printf("Device tree:\n");
	print_device_tree((void*)device_tree_start);

	patch_ppc_init_io_bat_setup();
	patch_cnputc();
	// patch_memory_trap((void*)0x0004bdac);

	// patch_exception_handler((void*)0x00000100);
	// patch_exception_handler((void*)0x00000200);
	patch_exception_handler((void*)0x00000300);
	// patch_exception_handler((void*)0x00000400);
	// patch_exception_handler((void*)0x00000500);
	// patch_exception_handler((void*)0x00000600);
	// patch_exception_handler((void*)0x00000700);
	// patch_exception_handler((void*)0x00000800);
	// patch_exception_handler((void*)0x00000900);
	// patch_exception_handler((void*)0x00000A00);
	// patch_exception_handler((void*)0x00000B00);
	// patch_exception_handler((void*)0x00000C00);
	// patch_exception_handler((void*)0x00000D00);
	// patch_exception_handler((void*)0x00000E00);
	// patch_exception_handler((void*)0x00000F00);

	ret = start_mach_kernel();
	if (ret != 0) {
		return -1;
	}

    return 0;
}

