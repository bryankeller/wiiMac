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

#include "bootmii_ppc.h"
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

void patch_memory_trap(u32 *offset) {
	u32 r24 = (u32)offset;
	offset[0] = 0x3F000000 | ((r24 >> 16) & 0xFFFF); // lis 24 r24h
	offset[1] = 0x63180000 | (r24 & 0xFFFF); // ori 24, 24, r24l
	offset[2] = 0x7FE00008; // trap
	sync_before_exec(offset, 4);
}

int main(void) {
	// Cannot gecko_init without delay
	udelay(1000000);
	gecko_init();

	printf("\nwiiMac\n");
	printf("(c) 2025 Bryan Keller - @blk19_\n\n");

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

	int ret;

	ret = load_mach_kernel("/mk");
	if (ret != 0) {
		return -1;
	}

	exception_init();
	patch_memory_trap((void *)0x0027b7f0);


	ret = start_mach_kernel();
	if (ret != 0) {
		return -1;
	}

    return 0;
}

