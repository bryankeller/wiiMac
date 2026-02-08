//
// Created by Bryan Keller on 2/1/26.
//

#ifndef hfsp_fs_h
#define hfsp_fs_h

#include "libhfsp.h"
#include "../types.h"

int hfsp_mount(volume *vol, UInt32 partition_offset);
void hfsp_unmount(volume *vol);

int hfsp_get_volume_name(volume *vol, char *buf, size_t buf_size);
int hfsp_read_file(volume *vol, const char *file_path, void *buf);
int hfsp_get_file_metadata(volume *vol, const char *path, char *name, int name_length, size_t *size);
int hfsp_list_dir(volume *vol, const char *path, void (*callback)(const char *name, int is_dir, void *ctx), void *cb_ctx);

#endif /* hfsp_fs_h */
