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
#include "macho_decoder.h"
#include "mini_ipc.h"
#include "fat.h"
#include "malloc.h"
#include "diskio.h"
#include "printf.h"
#include "video_low.h"
#include "input.h"
#include "console.h"
#include "irq.h"
#include "hollywood.h"
#include "apm.h"
#include "driver_loader.h"
#include "kernel_allocator.h"
#include "fs.h"
#include "hfsplus/hfsp_fs.h"

#define MINIMUM_MINI_VERSION 0x00010003
#define MAC_OS_X_SIGNATURE 0x4D4F5358
#define MAX_BOOT_ARGS_COMMAND_LINE_ENTRIES 5

static bool mini_version_too_old = true;
static u32 mini_version = 0;

static DSTATUS disk_stat = -1;
static int apm_fat_partition_index = -1;
static int selected_boot_partition_index = -1;
static char partition_names[64][255];

static int vmode = VIDEO_640X480_NTSCp_YUV16;
char boot_args_command_line_entries[MAX_BOOT_ARGS_COMMAND_LINE_ENTRIES][256] = {
  "", // from config file
  "-v Force800x600=1",
  "-v",
  "-v -s",
  "-v io=0xffff",
};
static int boot_args_index = 0;

static bool loading_kernel = false;
static bool kernel_load_failed = false;
static bool loaded_kernel = false;

static bool loading_kexts = false;
static bool loaded_kexts = false;

static int autoboot_ms = -1;
static int autoboot_ms_threshold = 10000;

static void print_instructions() {
  console_println("");
  console_println("Insert an SD card formatted as Apple Partition Map");
  console_println("containing an HFS+ formatted Mac OS X boot partition");
  clear_remainder_reset_y();
}

static void redraw_screen() {
  console_println("wiiMac - A Mac OS X bootloader for the Nintendo Wii");
  console_println("(c) 2025 Bryan Keller - @blk19_");
  
  console_println("");
  
  if (mini_version_too_old) {
    console_println("The MINI version (%d.%0d) is too old. Required: %d.%0d", (mini_version >> 16), (mini_version & 0xFFFF), (MINIMUM_MINI_VERSION >> 16), (MINIMUM_MINI_VERSION & 0xFFFF));
    console_println("Run the latest HackMii installer to resolve this issue");
    clear_remainder_reset_y();
    return;
  }
  
  console_println("");
  
  if (disk_stat & STA_NODISK) {
    console_println("No SD card detected");
    print_instructions();
    return;
  }
  
  if (disk_stat & STA_NOINIT) {
    console_println("Failed to read SD card");
    print_instructions();
    return;
  }
  
  if (apm_found_partitions_count <= 0) {
    console_println("No partitions found");
    print_instructions();
    return;
  }
  
  console_println("Bootable partitions:");
  for (int i = 0; i < apm_found_partitions_count; i++) {
    apm_entry_t partition = apm_found_partitions[i];
    if (!is_bootable(partition)) {
      continue;
    }
    
    int current_partition_number = i + 1;
    console_println("%c %s     %s", current_partition_number == partition_number ? '*' : ' ', partition.type, partition_names[i]);
  }
  
  console_println("");
  if (boot_args_index == 0) {
    console_println("Boot args: %s", boot_args_command_line);
  } else {
    console_println("Boot args (override %d): %s", boot_args_index, boot_args_command_line);
  }
  
  if (autoboot_ms != -1) {
    console_println("");
    console_println("Booting from %s in %ds", partition_names[partition_number - 1], (autoboot_ms_threshold - autoboot_ms) / 1000);
  }
  
  if (loading_kernel) {
    console_println("");
    console_println("Loading kernel...");
  }
  
  if (kernel_load_failed) {
    console_println("Failed to load kernel");
  }
  
  if (loading_kexts) {
    console_println("");
    console_println("Loading kernel extensions...");
  }
  
  if (loaded_kernel && loaded_kexts) {
    console_println("");
    console_println("Calling kernel...");
  }
  
  clear_remainder_reset_y();
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
  
  (*(void (*)())kernel_entry_point)(boot_args_address, MAC_OS_X_SIGNATURE);
  
  return -1;
}

static void update_boot_args_command_line() {
  // We need to make sure we don't exceed 256 characters total
  char truncated[240];
  strlcpy(truncated, boot_args_command_line_entries[boot_args_index], sizeof(truncated));
  sprintf(boot_args_command_line, "rd=disk0s%d %s", partition_number, truncated);
}

static void configure_video() {
  init_fb(vmode);
  VIDEO_Init(vmode);
  VIDEO_SetFrameBuffer(get_xfb());
  VISetupEncoder();
}

static void process_config_key_value(const char* key, const char *value) {
  if (strcmp(key, "video-mode") == 0) {
    printf("Video mode: %s\n", value);
    int new_vmode;
    if (strcmp(value, "ntscp") == 0) {
      new_vmode = VIDEO_640X480_NTSCp_YUV16;
    } else if (strcmp(value, "ntsci") == 0) {
      new_vmode = VIDEO_640X480_NTSCi_YUV16;
    } else if (strcmp(value, "pal60") == 0) {
      new_vmode = VIDEO_640X480_PAL60_YUV16;
    } else if (strcmp(value, "pal50") == 0) {
      new_vmode = VIDEO_640X480_PAL50_YUV16;
    } else {
      new_vmode = VIDEO_640X480_NTSCp_YUV16;
      printf("Invalid video mode. Defaulting to ntscp.\n");
    }
    
    if (new_vmode != vmode) {
      vmode = new_vmode;
      configure_video();
    }
  } else if (strcmp(key, "boot-args") == 0) {
    if (strlen(value) < 240) {
      printf("Boot args: %s\n", value);
      strlcpy(boot_args_command_line_entries[0], value, sizeof(boot_args_command_line_entries[0]));
    } else {
      printf("boot-args value is too long. Keep it under 240 characters.");
    }
  } else {
    printf("Unrecgonized config key\n.");
  }
}

static void process_config_file(FIL file_pointer) {
  size_t size = file_pointer.fsize;
  if (size > 2048) {
    printf("/wiiMac/config.txt is too large. Keep it under 2KB.\n");
    return;
  }

  char contents[size + 1];
  u32 bytes_read;
  if (f_read(&file_pointer, contents, size, &bytes_read) != FR_OK || bytes_read != size) {
    printf("Failed to read /wiiMac/config.txt\n");
    return;
  }
  contents[size] = '\0';
  
  char *position = contents;
  char *end = position + size;
  while (position < end) {
    // If we encounter the start of a comment or a new line character
    if (*position == '#' || *position == '\n' || *position == '\r') {
      // Increment the pointer to the next new line
      size_t skip = strcspn(position, "\n\r");
      if (position + skip >= end) {
        // Reached end of file, break
        break;
      }
      position += skip + 1;
    } else {
      // Handle config line
      
      // Get key up to '='
      int key_length = strcspn(position, "=");
      if (position + key_length >= end) {
        // Reached end of file without finding '=', skip to end
        break;
      }
      char key[key_length + 1];
      strlcpy(key, position, key_length + 1);
      position += key_length + 1;
      
      // Get value after '='
      // strcspn will stop at #, \n, \r, or \0 (end of string)
      int value_length = strcspn(position, "#\n\r");
      if (position + value_length > end) {
        printf("Invalid config file\n");
        return;
      }
      char value[value_length + 1];
      strlcpy(value, position, value_length + 1);
      position += value_length;
      
      process_config_key_value(str_trim_spaces(key), str_trim_spaces(value));
    }
  }
}

static void handle_disk_change() {
  // If we have an MBR, try to read the config file from the first FAT partition
  FIL file_pointer;
  if (fat_mount(0) == FR_OK) {
    printf("Reading config file at /wiiMac/config.txt...\n");
    if (f_open(&file_pointer, "/wiiMac/config.txt", FA_READ) == FR_OK) {
      process_config_file(file_pointer);
      f_close(&file_pointer);
    } else {
      printf("Failed to open config file at /wiiMac/config.txt\n");
    }
    
    fat_umount();
  }
  
  apm_find_partitions();
  if (apm_found_partitions_count > 0) {
    // Find boot-candidate HFS+ partitions
    for (int i = 0; i < apm_found_partitions_count; i++) {
      apm_entry_t partition = apm_found_partitions[i];
      
      if (apm_fat_partition_index == -1 && (strcmp(partition.type, "DOS_FAT_32") == 0 || strcmp(partition.type, "DOS_FAT_16") == 0 || strcmp(partition.type, "Apple_UNIX_SVR2") == 0)) {
        apm_fat_partition_index = i;
      }
      
      volume vol;
      if (hfsp_mount(&vol, partition.startingSector) != 0) {
        continue;
      }
      hfsp_get_volume_name(&vol, partition_names[i], 255);
      hfsp_unmount(&vol);
      if (selected_boot_partition_index == -1 && is_bootable(partition)) {
        selected_boot_partition_index = i;
      }
    }
    
    if (selected_boot_partition_index != -1) {
      autoboot_ms = 0;
    }
  }
}

static void decode_kernel() {
  // Zero all memory leading up to the file load address
  irq_shutdown();
  memset((void*)0x2FF, 0, KERNEL_FILE_LOAD_ADDRESS - 0x2FF);
  if (decode_mach_kernel() == 0) {
    loaded_kernel = true;
  }
}

static void load_kernel_from_fat() {
  FIL file_pointer;
  
  printf("Loading /wiiMac/mach_kernel...\n");
  
  if (f_open(&file_pointer, "/wiiMac/mach_kernel", FA_READ) != FR_OK) {
    printf("Failed to load /wiiMac/mach_kernel\n");
    return;
  }
  
  if (file_pointer.fsize > KERNEL_FILE_LOAD_SIZE) {
    printf("Failed to load /wiiMac/mach_kernel, kernel file is too big.\n");
    return;
  }
  
  u32 bytes_read;
  if (f_read(&file_pointer, (void *)KERNEL_FILE_LOAD_ADDRESS, file_pointer.fsize, &bytes_read) != FR_OK) {
    printf("Failed to load /wiiMac/mach_kernel\n");
    return;
  }
  
  f_close(&file_pointer);
    
  printf("Loaded /wiiMac/mach_kernel\n");
  decode_kernel();
}

static void load_kernel_from_hfs() {
  volume vol;
  apm_entry_t partition = apm_found_partitions[selected_boot_partition_index];
  
  printf("Loading /mach_kernel...\n");

  if (hfsp_mount(&vol, partition.startingSector) != 0) {
    printf("Failed to mount HFS+ partition.\n");
    return;
  }
  
  if (hfsp_read_file(&vol, "/mach_kernel", (void *)KERNEL_FILE_LOAD_ADDRESS) == 0) {
    printf("Failed to load /mach_kernel\n");
    return;
  }

  printf("Loaded /mach_kernel\n");
  hfsp_unmount(&vol);
  decode_kernel();
}

int main(void) {
	// Cannot gecko_init without delay
	udelay(1000000);
	gecko_init();

	exception_init();
	irq_initialize();

	ipc_initialize();
	ipc_slowping();
  
  input_init();

  configure_video();
  
//  VI_DumpRegisters();
  
  mini_version = ipc_getvers();
  mini_version_too_old = mini_version < MINIMUM_MINI_VERSION;
  redraw_screen();
  while (mini_version_too_old) { }
  
  while (!loaded_kernel) {
    DSTATUS prev_disk_stat = disk_stat;
    disk_stat = disk_initialize(0);
    if (disk_stat != prev_disk_stat) {
      autoboot_ms = -1;
      selected_boot_partition_index = -1;
      apm_fat_partition_index = -1;
      loading_kernel = false;
      kernel_load_failed = false;
      memset(partition_names, 0, 64*255);
      
      handle_disk_change();
    }
    
    u16 input = input_read();
    bool load_kernel = false;
    switch (input) {
      case GPIO_POWER: {
        autoboot_ms = -1;
        if (selected_boot_partition_index != -1 && apm_found_partitions_count > 0) {
          loading_kernel = false;
          kernel_load_failed = false;
          for (int i = 0; i < apm_found_partitions_count; i++) {
            selected_boot_partition_index = (selected_boot_partition_index + 1) % apm_found_partitions_count;
            apm_entry_t partition = apm_found_partitions[selected_boot_partition_index];
            if (is_bootable(partition)) {
              break;
            }
          }
        }
        break;
      }
        
      case GPIO_RESET: {
        load_kernel = true;
        break;
      }
        
      case GPIO_EJECT: {
        autoboot_ms = -1;
        boot_args_index += 1;
        if (boot_args_index >= MAX_BOOT_ARGS_COMMAND_LINE_ENTRIES) {
          boot_args_index = 0;
        }
        break;
      }
    }
    
    partition_number = selected_boot_partition_index + 1;
    
    update_boot_args_command_line();
    
    if (autoboot_ms >= autoboot_ms_threshold || load_kernel) {
      autoboot_ms = -1;
      loading_kernel = true;
      redraw_screen();
      
      // Try to load override kernel from FAT partition first (/wiiMac/mach_kernel)
      if (apm_fat_partition_index >= 0 && apm_fat_partition_index < apm_found_partitions_count) {
        apm_entry_t partition = apm_found_partitions[apm_fat_partition_index];
        if (fat_mount(partition.startingSector) == FR_OK) {
          load_kernel_from_fat();
        } else {
          printf("Failed to mount FAT partition.\n");
        }
      }
      
      // Otherwise, load kernel from HFS+ partition (/mach_kernel)
      if (!loaded_kernel) {
        load_kernel_from_hfs();
      }
      
      fat_umount();

      if (!loaded_kernel) {
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
  
  loading_kexts = true;
  redraw_screen();
  
  volume vol;
  if (hfsp_mount(&vol, apm_found_partitions[selected_boot_partition_index].startingSector) == 0) {
    // Load /S/L/E/System.kext (and its Plugins) from the boot partition.
    // This seems to be a dependency for other kexts, so it needs to be loaded.
    // Specifically, the IOKit.kext plugin seems important.
    load_kext_from_dir(&vol, "/System/Library/Extensions/System.kext", "", fs_hfsp_read_file, fs_hfsp_get_file_metadata, fs_hfsp_list_dir);
    
    // Load these drivers if they exist
    load_kext_from_dir(&vol, "/System/Library/Extensions/AppleOnboardDisplay.kext", "AppleOnboardDisplay", fs_hfsp_read_file, fs_hfsp_get_file_metadata, fs_hfsp_list_dir);
    load_kext_from_dir(&vol, "/System/Library/Extensions/IOAudioFamily.kext", "IOAudioFamily", fs_hfsp_read_file, fs_hfsp_get_file_metadata, fs_hfsp_list_dir);
    load_kext_from_dir(&vol, "/System/Library/Extensions/IOStorageFamily.kext", "IOStorageFamily", fs_hfsp_read_file, fs_hfsp_get_file_metadata, fs_hfsp_list_dir);
    load_kext_from_dir(&vol, "/System/Library/Extensions/IOGraphicsFamily.kext", "IOGraphicsFamily", fs_hfsp_read_file, fs_hfsp_get_file_metadata, fs_hfsp_list_dir);
    load_kext_from_dir(&vol, "/System/Library/Extensions/IOPCIFamily.kext", "IOPCIFamily", fs_hfsp_read_file, fs_hfsp_get_file_metadata, fs_hfsp_list_dir);
    load_kext_from_dir(&vol, "/System/Library/Extensions/IOHIDFamily.kext", "IOHIDFamily", fs_hfsp_read_file, fs_hfsp_get_file_metadata, fs_hfsp_list_dir);

    hfsp_unmount(&vol);
  }
  
  // Try to load kexts from FAT partition
  if (apm_fat_partition_index >= 0 && apm_fat_partition_index < apm_found_partitions_count) {
    apm_entry_t partition = apm_found_partitions[apm_fat_partition_index];
    if (fat_mount(partition.startingSector) == FR_OK) {
      printf("\n");
      load_kexts_from_dir(NULL, "/wiiMac", fs_fat_read_file, fs_fat_get_file_metadata, fs_fat_list_dir);
      fat_umount();
    }
  }
  
  loaded_kexts = true;
  redraw_screen();

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
