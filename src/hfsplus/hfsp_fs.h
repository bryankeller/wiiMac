//
// Created by Bryan Keller on 2/1/26.
//

#ifndef hfsp_fs_h
#define hfsp_fs_h

#include "libhfsp.h"

int hfsp_mount(volume *vol, UInt32 partition_offset);
void hfsp_unmount(volume *vol);

int hfsp_get_volume_name(volume *vol, char *buf, int buflen);
int hfsp_read_root_file(volume *vol, const char *filename, void *buf, u32 maxsize);

#endif /* hfsp_fs_h */
