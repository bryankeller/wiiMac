//
//  Created by Bryan Keller on 2/5/26.
//

#ifndef kernel_allocator_h
#define kernel_allocator_h

#include "types.h"

#define KERNEL_FILE_LOAD_ADDRESS (0x00c00000) // @ 12 MB
#define KERNEL_FILE_LOAD_SIZE (0x00800000) // 8 MB

u32 alloc_kernel_memory(size_t size);
u32 top_of_kernel_data();

#endif /* kernel_allocator_h */
