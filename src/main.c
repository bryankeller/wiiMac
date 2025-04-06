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

static void patch_memory_trap(u32 *offset) {
	offset[0] = 0x7FE00008; // trap
	sync_before_exec(offset, 1);
}

static void patch_memory_nop(u32 *offset) {
	offset[0] = 0x60000000; // ori, 0, 0, 0
	sync_before_exec(offset, 1);
}

static int start_mach_kernel() {
	long msr;

	printf("\n");
	printf("Calling Mach Kernel @ 0x%08x; boot args: 0x%08x, signature: %08x\n", kernel_entry_point, boot_args_address, kMacOSXSignature);

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

	// int vmode = -1;
	// init_fb(vmode);
	// VIDEO_Init(vmode);
	// VIDEO_SetFrameBuffer(get_xfb());
	// VISetupEncoder();

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

	exception_init();

	patch_memory_trap((void*)0x000a0648);
	// patch_memory_trap((void*)0x0008a9f4);

	printf("\n");
	printf("Setting up device tree and boot args...");

	set_up_boot_args();

	printf("\n");
	printf("Device tree:\n");

	print_device_tree((void*)device_tree_start);

	ret = start_mach_kernel();
	if (ret != 0) {
		return -1;
	}

    return 0;
}

