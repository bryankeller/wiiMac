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
 * Copyright (c) 1988 Carnegie-Mellon University
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 *
 */
/*
 * HISTORY
 * Revision 2.3  88/08/08  13:47:07  rvb
 * Allocate buffers dynamically vs statically.
 * Now b[i] and i_fs and i_buf, are allocated dynamically.
 * boot_calloc(size) allocates and zeros a  buffer rounded to a NPG
 * boundary.
 * Generalize boot spec to allow, xx()/mach, xx(n,[a..h])/mach,
 * xx([a..h])/mach, ...
 * Also default "xx" if unspecified and alloc just "/mach",
 * where everything is defaulted
 * Add routine, ptol(), to parse partition letters.
 *
 */
 
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sys.c	7.1 (Berkeley) 6/5/86
 */

#include "libsaio.h"
#include "md5.h"
#include "uuid.h"

/* copied from uuid/namespace.h, just like BootX's fs.c does. */
UUID_DEFINE( kFSUUIDNamespaceSHA1, 0xB3, 0xE2, 0x0F, 0x39, 0xF2, 0x92, 0x11, 0xD6, 0x97, 0xA4, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC );

// filesystem-specific getUUID functions call this shared string generator
long CreateUUIDString(uint8_t uubytes[], int nbytes, char *uuidStr)
{
    unsigned  fmtbase, fmtidx, i;
    uint8_t   uuidfmt[] = { 4, 2, 2, 2, 6 };
    char     *p = uuidStr;
    MD5_CTX   md5c;
    uint8_t   mdresult[16];

    bzero(mdresult, sizeof(mdresult));

    // just like AppleFileSystemDriver
    MD5Init(&md5c);
    MD5Update(&md5c, kFSUUIDNamespaceSHA1, sizeof(kFSUUIDNamespaceSHA1));
    MD5Update(&md5c, uubytes, nbytes);
    MD5Final(mdresult, &md5c);

    // this UUID has been made version 3 style (i.e. via namespace)
    // see "-uuid-urn-" IETF draft (which otherwise copies byte for byte)
    mdresult[6] = 0x30 | ( mdresult[6] & 0x0F );
    mdresult[8] = 0x80 | ( mdresult[8] & 0x3F );


    // generate the text: e.g. 5EB1869F-C4FA-3502-BDEB-3B8ED5D87292
    i = 0; fmtbase = 0;
    for(fmtidx = 0; fmtidx < sizeof(uuidfmt); fmtidx++) {
        for(i=0; i < uuidfmt[fmtidx]; i++) {
            uint8_t byte = mdresult[fmtbase+i];
            char nib;

            nib = byte >> 4;
            *p = nib + '0';  // 0x4 -> '4'
            if(*p > '9')  *p = (nib - 9 + ('A'-1));  // 0xB -> 'B'
            p++;

            nib = byte & 0xf;
            *p = nib + '0';  // 0x4 -> '4'
            if(*p > '9')  *p = (nib - 9 + ('A'-1));  // 0xB -> 'B'
            p++;

        }
        fmtbase += i;
        if(fmtidx < sizeof(uuidfmt)-1)
            *(p++) = '-';
        else
            *p = '\0';
    }

    return 0;
}
