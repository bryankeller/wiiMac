/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	Requires mini.

Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __NANDFS_H__
#define __NANDFS_H__

#define	NANDFS_NAME_LEN	12

#define	NANDFS_SEEK_SET	0
#define	NANDFS_SEEK_CUR	1
#define	NANDFS_SEEK_END	2

struct nandfs_fp {
	s16 first_cluster;
	s32 cur_cluster;
	u32 size;
	u32 offset;
};

s32 nandfs_initialize(void);
u32 nandfs_get_usage(void);

s32 nandfs_open(struct nandfs_fp *fp, const char *path);
s32 nandfs_read(void *ptr, u32 size, u32 nmemb, struct nandfs_fp *fp);
s32 nandfs_seek(struct nandfs_fp *fp, s32 offset, u32 whence);

#endif

