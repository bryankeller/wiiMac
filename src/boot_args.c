//
// Created by Bryan Keller on 4/5/25.
//

#include "boot_args.h"
#include "console.h"
#include "device_tree.h"
#include "string.h"
#include "kernel_allocator.h"

void set_up_boot_args() {
  boot_args_address = alloc_kernel_memory(BOOT_ARGS_SIZE);
  boot_args_t *boot_args = (boot_args_t*)boot_args_address;
  
  boot_args->Revision = 1;
  
  boot_args->Version = 1;
  
  strlcpy(boot_args->CommandLine, boot_args_command_line, 256);
  
  for (int i = 0; i < 26; i++) {
    boot_args->PhysicalDRAM[i].base = 0;
    boot_args->PhysicalDRAM[i].size = 0;
  }
  
  // MEM1 22 MB @0x00000000
  boot_args->PhysicalDRAM[0].base = 0x00000000;
  boot_args->PhysicalDRAM[0].size = 22 * 1024 * 1024; // 22 MB MEM1 (upper 2 MB used for frame buffers (real YUV and RGB shadow))
  
  // MEM2 56 MB @0x10000000
  boot_args->PhysicalDRAM[1].base = 0x10000000;
  boot_args->PhysicalDRAM[1].size = 56 * 1024 * 1024; // 56 MB MEM2 (upper 7 + 1 MB used by DMA allocator + MINI)
  
  boot_args->Video.v_baseAddr = (u32)get_xfb();
  boot_args->Video.v_display = 0; // change this to 1 when not verbose boot
  boot_args->Video.v_rowBytes = 2 * 640;
  boot_args->Video.v_width = 640;
  boot_args->Video.v_height = 480;
  boot_args->Video.v_depth = 16;
  
  boot_args->machineType = 0;
  
  build_device_tree();
  boot_args->deviceTreeP = (u8*)device_tree_start;
  boot_args->deviceTreeLength = device_tree_end - device_tree_start;
  
  boot_args->topOfKernelData = (u32)device_tree_end;
}
