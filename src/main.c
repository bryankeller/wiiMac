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
#include "macho.h"
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
#include "sha1.h"
#include "hollywood.h"
#include "apm.h"

#define MINIMUM_MINI_VERSION 0x00010003
#define kMacOSXSignature 0x4D4F5358

static bool mini_version_too_old = true;
static u32 mini_version = 0;
static DSTATUS disk_stat = -1;
static int selected_partition = -1;
static bool loading_kernel = false;
static bool kernel_load_failed = false;
static bool loaded_kernel = false;
static int autoboot_ms = -1;
static int autoboot_ms_threshold = 10000;

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

static int start_mach_kernel() {
  long msr;
  
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

static void print_instructions() {
  console_println("");
  console_println("Insert an SD card formatted as Apple Partition Map");
  console_println("containing an HFS+ formatted Mac OS X boot partition.");
  clear_remainder_reset_y();
}

static void redraw_screen() {
  console_println("wiiMac - A Mac OS X bootloader for the Nintendo Wii");
  console_println("(c) 2025 Bryan Keller - @blk19_");
  
  console_println("");
  
  if (mini_version_too_old) {
    console_println("The MINI version (%d.%0d) is too old. Required: %d.%0d.", (mini_version >> 16), (mini_version & 0xFFFF), (MINIMUM_MINI_VERSION >> 16), (MINIMUM_MINI_VERSION & 0xFFFF));
    console_println("Run the latest HackMii installer to resolve this issue.");
    clear_remainder_reset_y();
    return;
  }

  if (is_verbose_boot) {
    console_println("Verbose boot enabled.");
  } else {
    console_println("");
  }
  
  console_println("");
  
  if (disk_stat & STA_NODISK) {
    console_println("No SD card detected.");
    print_instructions();
    return;
  }
  
  if (disk_stat & STA_NOINIT) {
    console_println("Failed to read SD card.");
    print_instructions();
    return;
  }
  
  if (found_partitions_count <= 0) {
    console_println("No partitions found.");
    print_instructions();
    return;
  }
  
  console_println("Partitions:");
  for (int i = 0; i < found_partitions_count; i++) {
    int partition_index = i + 1;

    apm_entry_t partition = found_partitions[i];
    long long partitionOffset = (long long)partition.startingSector * 512;
//    SetBasePosition(partitionOffset);
//    HFSInitPartition();
//    u_char *partitionName = HFSGetPartitionName();
//    
//    console_println("%c %d: %s     %s", partition_index == partition_number ? '*' : ' ', partition_index, partition.type, partitionName);
  }
  
  if (autoboot_ms != -1) {
    console_println("");
    console_println("Booting from disk0s%d in %ds", partition_number, (autoboot_ms_threshold - autoboot_ms) / 1000);
  }
  
  if (loading_kernel) {
    console_println("");
    console_println("Loading mach_kernel from root of disk0s%d...", partition_number);
  }
  
  if (kernel_load_failed) {
    console_println("");
    console_println("Failed to load mach_kernel from root of disk0s%d.", partition_number);
  }
  
  if (loaded_kernel) {
    console_println("");
    console_println("Calling kernel...");
  }
  
  clear_remainder_reset_y();
}

int main(void) {
	// Cannot gecko_init without delay
	udelay(1000000);
	gecko_init();

	exception_init();
	irq_initialize();

	ipc_initialize();
	ipc_slowping();

	int vmode = VIDEO_640X480_NTSCp_YUV16;
	init_fb(vmode);
	VIDEO_Init(vmode);
	VIDEO_SetFrameBuffer(get_xfb());
	VISetupEncoder();
  
  VI_DumpRegisters();
  
  mini_version = ipc_getvers();
  mini_version_too_old = mini_version < MINIMUM_MINI_VERSION;
  redraw_screen();
  while (mini_version_too_old) { }
  
  while (!loaded_kernel) {
    DSTATUS prev_disk_stat = disk_stat;
    disk_stat = disk_initialize(0);
    if (disk_stat != prev_disk_stat) {
      autoboot_ms = -1;
      selected_partition = -1;
      loading_kernel = false;
      kernel_load_failed = false;
      
      find_partitions();
      for (int i = 0; i < found_partitions_count; i++) {
        apm_entry_t partition = found_partitions[i];
        if (selected_partition == -1 && (strcmp(partition.type, "Apple_HFS") == 0 || strcmp(partition.type, "Apple_HFSX") == 0)) {
          selected_partition = i;
        }
      }
      
      if (selected_partition != -1) {
        autoboot_ms = 0;
      }
    }
    
    
    u16 input = input_read();
    bool load_kernel = false;
    switch (input) {
      case GPIO_POWER: {
        autoboot_ms = -1;
        if (selected_partition != -1 && found_partitions_count > 0) {
          loading_kernel = false;
          kernel_load_failed = false;
          selected_partition = (selected_partition + 1) % found_partitions_count;
        }
        break;
      }
        
      case GPIO_RESET: {
        load_kernel = true;
        break;
      }
        
      case GPIO_EJECT:
        is_verbose_boot = !is_verbose_boot;
        break;
    }
    
    partition_number = selected_partition + 1;
    
    if (autoboot_ms >= autoboot_ms_threshold || load_kernel) {
      autoboot_ms = -1;
      loading_kernel = true;
      redraw_screen();
      apm_entry_t partition = found_partitions[selected_partition];
      long long partitionOffset = (long long)partition.startingSector * 512;
//      SetBasePosition(partitionOffset);
      printf("Load mach_kernel from offset: %lu\n", partitionOffset);
//      if (HFSLoadFile("\\mach_kernel") > 0) {
//        printf("Loaded mach_kernel\n");
//        // Zero all memory leading up to the file load address
//        memset((void*)0x2FF, 0, kLoadAddr - 0x2FF);
//        if (load_mach_kernel() == 0) {
//          printf("Loaded mach_kernel\n");
//          loaded_kernel = true;
//        }
//      }
      
      if (!loaded_kernel) {
        printf("Failed to load mach_kernel\n");
        loading_kernel = false;
        kernel_load_failed = true;
      }
    }
    
    redraw_screen();
    udelay(100000);
    
    if (autoboot_ms != -1) {
      autoboot_ms += 100;
    }
  }

  printf("\n");
	printf("Setting up device tree and boot args...");

	set_up_boot_args();

	printf("\n");
	printf("Device tree:\n");
	print_device_tree((void*)device_tree_start);
  
  printf("\n");
  printf("Top of kernel data: %08x\n", ((boot_args_t*)boot_args_address)->topOfKernelData);
  
  printf("\n");
  printf("Calling Mach Kernel @ 0x%08x; boot args: 0x%08x\n", kernel_entry_point, boot_args_address);
  
  udelay(1000000);
  draw_screen_for_graphical_boot();

	if (start_mach_kernel() != 0) {
		return -1;
	}

  return 0;
}
