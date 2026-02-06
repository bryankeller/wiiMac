//
//  Created by Bryan Keller on 2/5/26.
//

#ifndef kernel_allocator_h
#define kernel_allocator_h

#include "types.h"

#define kernel_file_load_address (0x00c00000) // @ 12 MB
#define kernel_file_load_size (0x00800000) // 8 MB

u32 alloc_kernel_memory(size_t size);
u32 top_of_kernel_data();

#endif /* kernel_allocator_h */
