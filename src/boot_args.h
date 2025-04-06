//
// Created by Bryan Keller on 3/30/25.
//

#ifndef BOOT_ARGS_H
#define BOOT_ARGS_H

#include "types.h"

#define boot_args_address (0x00500000) // @ 5 MB

/*
 * Video information..
 */

struct Boot_Video {
    u32	v_baseAddr;	                /* Base address of video memory */
    u32	v_display;	                /* Display Code (if Applicable */
    u32	v_rowBytes;	                /* Number of bytes per pixel row */
    u32	v_width;	                /* Width */
    u32	v_height;	                /* Height */
    u32	v_depth;	                /* Pixel Depth */
};

typedef struct Boot_Video	Boot_Video;

/* DRAM Bank definitions - describes physical memory layout.
 */

struct DRAMBank {
    u32	base;		                /* physical base of DRAM bank */
    u32	size;		                /* size of bank */
};
typedef struct DRAMBank DRAMBank;

typedef struct boot_args {
    u16	Revision;	                /* Revision of boot_args structure */
    u16	Version;	                /* Version of boot_args structure */
    char CommandLine[256];	        /* Passed in command line */
    DRAMBank PhysicalDRAM[26];	    /* base and range pairs for the 26 DRAM banks */
    Boot_Video Video;		        /* Video Information */
    u32	machineType;	            /* Machine Type (gestalt) */
    void *deviceTreeP;	            /* Base of flattened device tree */
    u32	deviceTreeLength;           /* Length of flattened tree */
    u32	topOfKernelData;            /* Highest address used in kernel data area */
} boot_args_t;

void set_up_boot_args();

#endif //BOOT_ARGS_H
