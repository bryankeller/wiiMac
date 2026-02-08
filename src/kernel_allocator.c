//
//  Created by Bryan Keller on 2/5/26.
//

#include "kernel_allocator.h"

static u32 last_kernel_address = 0;

u32 alloc_kernel_memory(size_t size) {
  u32 address = last_kernel_address;
  
  last_kernel_address += (size + 0xFFF) & ~0xFFF; // round to 4KB pages
  if (last_kernel_address > KERNEL_FILE_LOAD_ADDRESS) {
    return 0;
  }
  return address;
}

u32 top_of_kernel_data() {
  return last_kernel_address;
}
