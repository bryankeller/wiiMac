//
//  Created by Bryan Keller on 2/5/26.
//

#include "driver_loader.h"
#include "bootmii_ppc.h"
#include "string.h"
#include "kernel_allocator.h"
#include "malloc.h"

void load_kexts_from_dir(void *ctx, const char *directory_path, read_file_f read_file, get_file_metadata_f get_file_metadata, list_dir_f list_dir) {
  printf("Loading kexts from %s...\n", directory_path);
  
  directory_entry_t *entries = (directory_entry_t *)malloc(sizeof(directory_entry_t) * 64);
  u32 entries_count;
  list_dir(ctx, directory_path, entries, &entries_count, 64);
  for (u32 i = 0; i < entries_count; i++) {
    if (allocated_drivers_count >= MAX_DRIVERS) {
      printf("Driver limit reached (%d), skipping remaining drivers.\n", MAX_DRIVERS);
    }
    
    directory_entry_t entry = entries[i];
    if (!entry.is_directory) {
      continue;
    }
    
    size_t name_length = strlen(entry.name);
    char bin_name[name_length];
    if (name_length > 5 && entry.name[0] != '.' && strcmp(&entry.name[name_length - 5], ".kext") == 0) {
      strlcpy(bin_name, entry.name, name_length - 4);
    } else if (name_length > 7 && entry.name[0] != '.' && strcmp(&entry.name[name_length - 7], ".bundle") == 0) {
      strlcpy(bin_name, entry.name, name_length - 6);
    }
    
    char kext_path[512];
    strlcpy(kext_path, directory_path, sizeof(kext_path));
    strlcat(kext_path, "/", sizeof(kext_path));
    strlcat(kext_path, entry.name, sizeof(kext_path));
    load_kext_from_dir(ctx, kext_path, bin_name, read_file, get_file_metadata, list_dir);
  }
  
  free(entries);
}

void load_kext_from_dir(void *ctx, const char *directory_path, const char *bin_name, read_file_f read_file, get_file_metadata_f get_file_metadata, list_dir_f list_dir) {
  printf("Loading %s...\n", directory_path);
  
  // First, handle the Info.plist
  char info_plist_path[512];
  strlcpy(info_plist_path, directory_path, sizeof(info_plist_path));
  strlcat(info_plist_path, "/Contents/Info.plist", sizeof(info_plist_path));
  file_metadata_t info_plist_metadata;
  if (get_file_metadata(ctx, info_plist_path, &info_plist_metadata) != 0) {
    printf("Failed to open %s\n", info_plist_path);
    return;
  }
  
  // Then, handle the main binary
  char bin_path[512];
  strlcpy(bin_path, directory_path, sizeof(bin_path));
  strlcat(bin_path, "/Contents/MacOS/", sizeof(bin_path));
  strlcat(bin_path, bin_name, sizeof(bin_path));
  file_metadata_t bin_metadata;
  int bin_metadata_result = get_file_metadata(ctx, bin_path, &bin_metadata);
  
  // Prepare driver_info
  u32 driver_allocation_size = sizeof(driver_info_t) + info_plist_metadata.size + (bin_metadata_result == 0 ? bin_metadata.size : 0);
  driver_info_t *driver_info = (driver_info_t *)alloc_kernel_memory(driver_allocation_size);
  driver_info->info_plist_start = (char *)((u32)driver_info + sizeof(driver_info_t));
  driver_info->info_plist_size = info_plist_metadata.size;
  
  if (bin_metadata_result == 0) {
    driver_info->bin_start = driver_info->info_plist_start + driver_info->info_plist_size;
    driver_info->bin_size = bin_metadata.size;
  } else {
    driver_info->bin_start = 0;
    driver_info->bin_size = 0;
  }
  
  // Load Info.plist and the binary (if present)
  if (read_file(ctx, info_plist_path, info_plist_metadata.size, (void *)driver_info->info_plist_start) != 0) {
    printf("Failed to load %s.\n", info_plist_path);
    return;
  }
  
  if (bin_metadata_result == 0 && read_file(ctx, bin_path, bin_metadata.size, (void *)driver_info->bin_start) != 0) {
    printf("Failed to load %s.\n", bin_path);
    return;
  }
  
  allocated_drivers[allocated_drivers_count] = driver_info;
  allocated_drivers_count += 1;
  
  // Lastly, handle plugin kexts
  char plugins_path[512];
  strlcpy(plugins_path, directory_path, sizeof(plugins_path));
  strlcat(plugins_path, "/Contents/Plugins", sizeof(plugins_path));
  load_kexts_from_dir(ctx, plugins_path, read_file, get_file_metadata, list_dir);
}
