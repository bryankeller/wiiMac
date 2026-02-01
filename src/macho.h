//
// Created by Bryan Keller on 2/20/25.
//

#ifndef MACHO_H
#define MACHO_H

#define kLoadAddr       (0x00c00000) // @ 12 MB
#define kLoadSize       (0x00800000)

#define kHeaderAddr (0x00500000) // @ 5 MB
#define kSymtabAddr (0x00600000) // @ 6 MB

u32 kernel_entry_point;

u32 kernel_header_size;

u32 kernel_text_start;
u32 kernel_text_size;

u32 kernel_data_start;
u32 kernel_data_size;

u32 kernel_symtab_size;

int load_mach_kernel();

#endif //MACHO_H
