//
// Created by Bryan Keller on 2/1/26.
//

#include "hfsp_fs.h"
#include "volume.h"
#include "record.h"
#include "blockiter.h"
#include "unicode.h"
#include "../string.h"

int hfsp_mount(volume *vol, UInt32 partition_offset) {
  return volume_open(vol, 0, partition_offset);
}

void hfsp_unmount(volume *vol) {
  volume_close(vol);
}

int hfsp_get_volume_name(volume *vol, char *buf, int buflen) {
  record r;
  if (record_init_cnid(&r, &vol->catalog, HFSP_ROOT_CNID) != 0) {
    return -1;
  }
  if (r.record.type != HFSP_FOLDER_THREAD) {
    return -1;
  }
  unicode_uni2asc(buf, &r.record.u.thread.nodeName, buflen);
  return 0;
}

int hfsp_read_root_file(volume *vol, const char *filename, void *buf, u32 maxsize) {
  record r;
  if (record_init_root(&r, &vol->catalog) != 0) {
    return -1;
  }
  if (record_init_string_parent(&r, &r, (char*)filename) != 0) {
    return -1;
  }
  if (r.record.type != HFSP_FILE) {
    return -1;
  }

  hfsp_cat_file *file = &r.record.u.file;
  u32 filesize = (u32)file->data_fork.total_size;
  if (filesize > maxsize) filesize = maxsize;
  
  u32 blksize = vol->blksize;
  u32 num_blocks = (filesize + blksize - 1) / blksize;  // Round up
  
  if (!volume_readfromfork(vol, buf, &file->data_fork, 0, num_blocks, HFSP_EXTENT_DATA, file->id)) {
    return -1;
  }
  
  return (int)filesize;
}
