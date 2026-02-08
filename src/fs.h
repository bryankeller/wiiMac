//
//  Created by Bryan Keller on 2/7/26.
//

#ifndef fs_h
#define fs_h

#include "types.h"

typedef struct directory_entry {
  char name[256];
  u8 is_directory;
} directory_entry_t;

typedef struct file_metadata {
  char name[256];
  size_t size;
} file_metadata_t;

int fs_fat_read_file(void *ctx, const char *path, size_t size, void *buf);
int fs_fat_get_file_metadata(void *ctx, const char *path, file_metadata_t *file_metadata);
int fs_fat_list_dir(void *ctx, const char *path, directory_entry_t *entries, u32 *entries_count, u32 max_entries);

int fs_hfsp_read_file(void *ctx, const char *path, size_t size, void *buf);
int fs_hfsp_get_file_metadata(void *ctx, const char *path, file_metadata_t *file_metadata);
int fs_hfsp_list_dir(void *ctx, const char *path, directory_entry_t *entries, u32 *entries_count, u32 max_entries);

#endif /* fs_h */
