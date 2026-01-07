/*
 * Copyright (c) 1999-2003 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 2.0 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* Useful types. */

#ifndef __LIBSAIO_SAIO_TYPES_H
#define __LIBSAIO_SAIO_TYPES_H

typedef struct FinderInfo {
    unsigned char data[16];
} FinderInfo;

struct         BootVolume;
typedef struct BootVolume * BVRef;
typedef struct BootVolume * CICell;

typedef long (*FSInit)(CICell ih);
typedef long (*FSLoadFile)(CICell ih, char * filePath);
typedef long (*FSReadFile)(CICell ih, char *filePath, void *base, uint64_t offset, uint64_t length);
typedef long (*FSGetFileBlock)(CICell ih, char *filePath, unsigned long long *firstBlock);
typedef long (*FSGetDirEntry)(CICell ih, char * dirPath, long * dirIndex,
                              char ** name, long * flags, long * time,
                              FinderInfo * finderInfo, long * infoValid);
typedef long (* FSGetUUID)(CICell ih, char *uuidStr);
typedef void (*BVGetDescription)(CICell ih, char * str, long strMaxLen);
// Can be just pointed to free or a special free function
typedef void (*BVFree)(CICell ih);

#define BVSTRLEN 32

struct BootVolume {
  BVRef            next;            /* list linkage pointer */
  int              biosdev;         /* BIOS device number */
  int              type;            /* device type (floppy, hd, network) */
  unsigned int     flags;           /* attribute flags */
  BVGetDescription description;     /* BVGetDescription function */
  int              part_no;         /* partition number (1 based) */
  unsigned int     part_boff;       /* partition block offset */
  unsigned int     part_type;       /* partition type */
  unsigned int     fs_boff;         /* 1st block # of next read */
  unsigned int     fs_byteoff;      /* Byte offset for read within block */
  FSLoadFile       fs_loadfile;     /* FSLoadFile function */
  FSReadFile       fs_readfile;     /* FSReadFile function */
  FSGetDirEntry    fs_getdirentry;  /* FSGetDirEntry function */
  FSGetFileBlock   fs_getfileblock; /* FSGetFileBlock function */
  FSGetUUID        fs_getuuid;      /* FSGetUUID function */
  unsigned int     bps;             /* bytes per sector for this device */
  char             name[BVSTRLEN];  /* (name of partition) */
  char             type_name[BVSTRLEN]; /* (type of partition, eg. Apple_HFS) */
  BVFree           bv_free;         /* BVFree function */
  uint32_t         modTime;
  char             label[BVSTRLEN]; /* partition volume label */
  char             altlabel[BVSTRLEN]; /* partition volume label */
  bool             filtered;        /* newFilteredBVChain() will set to TRUE */
  bool             visible;         /* will shown in the device list */
};

#endif /* !__LIBSAIO_SAIO_TYPES_H */
