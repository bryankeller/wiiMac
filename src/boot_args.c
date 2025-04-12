//
// Created by Bryan Keller on 4/5/25.
//

#include "boot_args.h"
#include "console.h"
#include "device_tree.h"
#include "printf.h"

void set_up_boot_args() {
    boot_args_t *boot_args = (boot_args_t*)boot_args_address;

    boot_args->Revision = 1;

    boot_args->Version = 1;

    sprintf(boot_args->CommandLine, "-v debug=0x8\0");

    for (int i = 0; i < 26; i++) {
        boot_args->PhysicalDRAM[i].base = 0;
        boot_args->PhysicalDRAM[i].size = 0;
    }

    // MEM1 24MB @0x00000000
    boot_args->PhysicalDRAM[0].base = 0x00000000;
    boot_args->PhysicalDRAM[0].size = 24 * 1024 * 1024;

    // MEM2 64MB @0x10000000
    boot_args->PhysicalDRAM[1].base = 0x10000000;
    boot_args->PhysicalDRAM[1].size = 64 * 1024 * 1024;

    boot_args->Video.v_baseAddr = 0;
    boot_args->Video.v_display = 0;
    boot_args->Video.v_rowBytes = 0;
    boot_args->Video.v_width = 0;
    boot_args->Video.v_height = 0;
    boot_args->Video.v_depth = 0;

//    boot_args->Video.v_baseAddr = (u32)get_xfb();
//    boot_args->Video.v_display = 0;
//    boot_args->Video.v_rowBytes = 4 * 640;
//    boot_args->Video.v_width = 640;
//    boot_args->Video.v_height = 480;
//    boot_args->Video.v_depth = 32;

    boot_args->machineType = 0;

    build_device_tree();
    boot_args->deviceTreeP = (u8*)device_tree_start;
    boot_args->deviceTreeLength = device_tree_end - device_tree_start;

    boot_args->topOfKernelData = (u32)device_tree_end;
}