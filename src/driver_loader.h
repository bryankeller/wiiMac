//
//  Created by Bryan Keller on 2/5/26.
//

#ifndef driver_loader_h
#define driver_loader_h

#include "types.h"
#include "fs.h"

#define MAX_DRIVERS 64

typedef struct driver_info  {
  char *info_plist_start;
  u32 info_plist_size;
  void *bin_start;
  u32 bin_size;
} driver_info_t;

typedef int (*read_file_f)(void *ctx, const char *path, size_t size, void *buf);
typedef int (*get_file_metadata_f)(void *ctx, const char *path, file_metadata_t *file_metadata);
typedef int (*list_dir_f)(void *ctx, const char *path, directory_entry_t *entries, u32 *entries_count, u32 max_entries);

driver_info_t *allocated_drivers[MAX_DRIVERS];
int allocated_drivers_count = 0;

void load_kexts_from_dir(void *ctx, const char *directory_path, read_file_f read_file, get_file_metadata_f get_file_metadata, list_dir_f list_dir);
void load_kext_from_dir(void *ctx, const char *directory_path, const char *bin_name, read_file_f read_file, get_file_metadata_f get_file_metadata, list_dir_f list_dir);

#endif /* driver_loader_h */
