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
/* 
 * Mach Operating System
 * Copyright (c) 1990 Carnegie-Mellon University
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/*
 *          INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *  This software is supplied under the terms of a license  agreement or 
 *  nondisclosure agreement with Intel Corporation and may not be copied 
 *  nor disclosed except in accordance with the terms of that agreement.
 *
 *  Copyright 1988, 1989 Intel Corporation
 */

/*
 * Copyright 1993 NeXT Computer, Inc.
 * All rights reserved.
 */

#include "libsaio.h"
#include "../diskio.h"

#define BPS              512     /* sector size of the device */

static u8 sector_buffer[BPS] __attribute__((aligned(32)));

//==========================================================================
// Handle seek request from filesystem modules.

void diskSeek( BVRef bvr, long long position )
{
      bvr->fs_boff = position / BPS;
      bvr->fs_byteoff = position % BPS;
}

//==========================================================================
// Handle read request from filesystem modules.

int diskRead( BVRef bvr, long addr, long length )
{
  DWORD sector = bvr->fs_boff + bvr->part_boff;
  long byte_offset = bvr->fs_byteoff;
  BYTE *dest = (BYTE *)addr;
  long remaining = length;
  
  printf("diskRead %lu\n", length);
  
  if (byte_offset > 0) {
    if (disk_read(0, sector_buffer, sector, 1) != RES_OK) {
      return -1;
    }
    
    long copy_len = BPS - byte_offset;
    if (copy_len > remaining) {
      copy_len = remaining;
    }
    
    memcpy(dest, sector_buffer + byte_offset, copy_len);
    dest += copy_len;
    remaining -= copy_len;
    sector++;
  }
  
  if (remaining >= BPS) {
    u32 full_sectors = remaining / BPS;
    if (disk_read(0, dest, sector, full_sectors) != RES_OK) {
      return -1;
    }
    dest += full_sectors * BPS;
    remaining -= full_sectors * BPS;
    sector += full_sectors;
  }
  
  if (remaining > 0) {
    if (disk_read(0, sector_buffer, sector, 1) != RES_OK) {
      return -1;
    }
    memcpy(dest, sector_buffer, remaining);
  }
  
  return length;
}
