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

#ifndef __LIBSAIO_SAIO_INTERNAL_H
#define __LIBSAIO_SAIO_INTERNAL_H

#include "saio_types.h"

/* cache.c */
extern void   CacheInit(CICell ih, long blockSize);
extern long   CacheRead(CICell ih, char *buffer, long long offset,
                        long length, long cache);

/* disk.c */
extern void   diskSeek(BVRef bvr, long long position);
extern int    diskRead(BVRef bvr, long addr, long length);

/* hfs_compare.c */
extern int32_t FastUnicodeCompare(u_int16_t *uniStr1, u_int32_t len1,
							   u_int16_t *uniStr2, u_int32_t len2);
extern void utf_encodestr( const u_int16_t * ucsp, int ucslen,
                u_int8_t * utf8p, u_int32_t bufsize );
extern void utf_decodestr(const u_int8_t *utf8p, u_int16_t *ucsp,
                u_int16_t *ucslen, u_int32_t bufsize );

/* sys.c */
extern long   CreateUUIDString(uint8_t uubytes[], int nbytes, char *uuidStr);

#endif /* !__LIBSAIO_SAIO_INTERNAL_H */
