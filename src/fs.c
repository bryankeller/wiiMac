//
//  Created by Bryan Keller on 2/7/26.
//

#include "fs.h"
#include "ff.h"
#include "string.h"
#include "bootmii_ppc.h"
#include "hfsplus/hfsp_fs.h"

int fs_fat_read_file(void *ctx, const char *path, size_t size, void *buf) {
  (void)ctx;
  FIL file_pointer;
  
  if (f_open(&file_pointer, path, FA_READ) != FR_OK) {
    printf("Failed to open %s\n", path);
    return -1;
  }
  
  u32 bytes_read;
  if (f_read(&file_pointer, buf, size, &bytes_read) != FR_OK) {
    printf("Failed to open %s\n", path);
    return -1;
  }
  f_close(&file_pointer);
  
  return 0;
}

int fs_fat_get_file_metadata(void *ctx, const char *path, file_metadata_t *file_metadata) {
  (void)ctx;
  FILINFO file_info;
  char lfn_buffer[256];
  file_info.lfname = lfn_buffer;
  file_info.lfsize = sizeof(lfn_buffer);
  
  if (f_stat(path, &file_info) != FR_OK) {
    printf("Failed to open %s\n", path);
    return -1;
  }
  
  strlcpy(file_metadata->name, file_info.lfname, sizeof(file_metadata->name));
  file_metadata->size = file_info.fsize;
  
  return 0;
}

int fs_fat_list_dir(void *ctx, const char *path, directory_entry_t *entries, u32 *entries_count, u32 max_entries) {
  (void)ctx;
  DIR directory_pointer;
  FILINFO file_info;
  char lfn_buffer[256];
  file_info.lfname = lfn_buffer;
  file_info.lfsize = sizeof(lfn_buffer);
  
  *entries_count = 0;
  
  if (f_opendir(&directory_pointer, path) != FR_OK) {
    printf("Failed to open directory %s\n", path);
    return -1;
  }
  
  u32 count = 0;
  while (count < max_entries && f_readdir(&directory_pointer, &file_info) == FR_OK && file_info.lfname[0] != 0) {
    strlcpy(entries[count].name, file_info.lfname, sizeof(entries[count].name));
    entries[count].is_directory = (file_info.fattrib & AM_DIR) ? 1 : 0;
    count += 1;
  }
  *entries_count = count;
  
  return 0;
}

int fs_hfsp_read_file(void *ctx, const char *path, size_t size, void *buf) {
  (void)size;
  volume *vol = (volume *)ctx;
  if (hfsp_read_file(vol, path, buf) <= 0) {
    return -1;
  }
  return 0;
}

int fs_hfsp_get_file_metadata(void *ctx, const char *path, file_metadata_t *file_metadata) {
  volume *vol = (volume *)ctx;
  return hfsp_get_file_metadata(vol, path, file_metadata->name, sizeof(file_metadata->name), &file_metadata->size);
}

typedef struct {
  directory_entry_t *entries;
  u32 *entries_count;
  u32 max_entries;
} list_dir_ctx_t;

static void list_dir_callback(const char *name, int is_directory, void *ctx) {
  list_dir_ctx_t *lctx = (list_dir_ctx_t *)ctx;
  if (*lctx->entries_count >= lctx->max_entries) {
    return;
  }
  strlcpy(lctx->entries[*lctx->entries_count].name, name, sizeof(lctx->entries[0].name));
  lctx->entries[*lctx->entries_count].is_directory = is_directory ? 1 : 0;
  (*lctx->entries_count) += 1;
}

int fs_hfsp_list_dir(void *ctx, const char *path, directory_entry_t *entries, u32 *entries_count, u32 max_entries) {
  volume *vol = (volume *)ctx;
  *entries_count = 0;
  list_dir_ctx_t list_dir_ctx = { entries, entries_count, max_entries };
  return hfsp_list_dir(vol, path, list_dir_callback, &list_dir_ctx);
}
