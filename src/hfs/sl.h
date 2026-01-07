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

#ifndef __LIBSAIO_SL_H
#define __LIBSAIO_SL_H

#include "libsaio.h"

#define SWAP_BE16(x)  (x)
#define SWAP_BE32(x)  (x)
#define SWAP_BE64(x)  (x)

// File Permissions and Types
enum {
    kPermOtherExecute  = 1 << 0,
    kPermOtherWrite    = 1 << 1,
    kPermOtherRead     = 1 << 2,
    kPermGroupExecute  = 1 << 3,
    kPermGroupWrite    = 1 << 4,
    kPermGroupRead     = 1 << 5,
    kPermOwnerExecute  = 1 << 6,
    kPermOwnerWrite    = 1 << 7,
    kPermOwnerRead     = 1 << 8,
    kPermMask          = 0x1FF,
    kOwnerNotRoot      = 1 << 9,
    kFileTypeUnknown   = 0x0 << 16,
    kFileTypeFlat      = 0x1 << 16,
    kFileTypeDirectory = 0x2 << 16,
    kFileTypeLink      = 0x3 << 16,
    kFileTypeMask      = 0x3 << 16
};

#define Seek(c, p)     diskSeek(c, p);
#define Read(c, a, l)  diskRead(c, a, l);

extern void * gFSLoadAddress;

#endif /* !__LIBSAIO_SL_H */
