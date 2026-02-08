//
// Created by Bryan Keller on 2/1/26.
//

#include "hfsp_fs.h"
#include "volume.h"
#include "record.h"
#include "blockiter.h"
#include "unicode.h"
#include "../string.h"
#include "../bootmii_ppc.h"

static int hfsp_resolve_path(volume *vol, const char *file_path, record *result_record) {
  record r;
  char component[256];
  const char *start, *end;
  
  if (!file_path || !*file_path) {
    return -1;
  }
  
  // Skip leading slashes
  start = file_path;
  while (*start == '/') {
    start++;
  }
  
  // Start at root
  if (record_init_root(&r, &vol->catalog) != 0) {
    return -1;
  }
  
  // Process each path component
  while (*start) {
    end = strchr(start, '/');
    if (!end) {
      end = start + strlen(start);
    }
    
    // Copy component (with length check)
    size_t length = end - start;
    if (length == 0) {
      start = end + 1;
      continue; // Skip empty components (e.g., "//")
    }
    if (length >= sizeof(component)) {
      return -1; // Component too long
    }
    memcpy(component, start, length);
    component[length] = '\0';
    
    // Look up this component
    if (record_init_string_parent(&r, &r, component) != 0) {
      return -1;
    }
    
    // Move past this component and any trailing slashes
    start = end;
    while (*start == '/') {
      start++;
    }
    
    // If start is not null, we're not at the end of the null-terminated path.
    // If that's the case, and the current record type is a file, something is wrong.
    if (*start && r.record.type != HFSP_FOLDER) {
      return -1;
    }
  }
  
  *result_record = r;
  return 0;
}

int hfsp_mount(volume *vol, UInt32 partition_offset) {
  return volume_open(vol, 0, partition_offset);
}

void hfsp_unmount(volume *vol) {
  volume_close(vol);
}

int hfsp_get_volume_name(volume *vol, char *buf, size_t buf_size) {
  record r;
  if (record_init_cnid(&r, &vol->catalog, HFSP_ROOT_CNID) != 0) {
    return -1;
  }
  if (r.record.type != HFSP_FOLDER_THREAD) {
    return -1;
  }
  unicode_uni2asc(buf, &r.record.u.thread.nodeName, buf_size);
  return 0;
}

int hfsp_read_file(volume *vol, const char *file_path, void *buf) {
  record r;
  if (hfsp_resolve_path(vol, file_path, &r) != 0) {
    return -1;
  }
  
  if (r.record.type != HFSP_FILE) {
    return -1;
  }

  hfsp_cat_file *file = &r.record.u.file;
  u32 filesize = (u32)file->data_fork.total_size;
  
  u32 blksize = vol->blksize;
  u32 num_blocks = (filesize + blksize - 1) / blksize; // Round up
  
  if (!volume_readfromfork(vol, buf, &file->data_fork, 0, num_blocks, HFSP_EXTENT_DATA, file->id)) {
    return -1;
  }
  
  return (int)filesize;
}

int hfsp_get_file_metadata(volume *vol, const char *path, char *name, int name_length, size_t *size) {
  record r;
  if (hfsp_resolve_path(vol, path, &r) != 0) {
    return -1;
  }
  if (r.record.type != HFSP_FILE) {
    return -1;
  }
  
  unicode_uni2asc(name, &r.key.name, name_length);
  *size = (size_t)r.record.u.file.data_fork.total_size;
  return 0;
}

int hfsp_list_dir(volume *vol, const char *path, void (*callback)(const char *name, int is_directory, void *ctx), void *cb_ctx) {
  record r, child;
  char name[256];
  
  if (hfsp_resolve_path(vol, path, &r) != 0) {
    return -1;
  }
  if (r.record.type != HFSP_FOLDER) {
    return -1;
  }
  
  if (record_init_parent(&child, &r) != 0) {
    return 0; // Empty directory
  }
  
  u32 parent_cnid = r.record.u.folder.id;
  do {
    if (child.key.parent_cnid != parent_cnid) {
      break; // Moved past this directory
    }
    if (child.record.type == HFSP_FILE || child.record.type == HFSP_FOLDER) {
      unicode_uni2asc(name, &child.key.name, sizeof(name));
      callback(name, child.record.type == HFSP_FOLDER, cb_ctx);
    }
  } while (record_next(&child) == 0);
  
  return 0;
}
