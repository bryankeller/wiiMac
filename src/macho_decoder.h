//
// Created by Bryan Keller on 2/20/25.
//

#ifndef MACHO_H
#define MACHO_H

#define MAX_MACHO_MEMORY_MAP_ENTRIES 16

typedef struct macho_memory_map_entry  {
  char name[32];
  u32 start;
  u32 size;
} macho_memory_map_entry_t;

macho_memory_map_entry_t macho_memory_map_entries[MAX_MACHO_MEMORY_MAP_ENTRIES];
int macho_memory_map_entries_count = 0;

u32 kernel_entry_point;

int decode_mach_kernel();

#endif //MACHO_H
