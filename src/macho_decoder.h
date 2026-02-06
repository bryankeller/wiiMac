//
// Created by Bryan Keller on 2/20/25.
//

#ifndef MACHO_H
#define MACHO_H

u32 kernel_entry_point;

u32 kernel_header_start;
u32 kernel_header_size;

u32 kernel_text_start;
u32 kernel_text_size;

u32 kernel_data_start;
u32 kernel_data_size;

u32 kernel_symtab_start;
u32 kernel_symtab_size;

int decode_mach_kernel();

#endif //MACHO_H
