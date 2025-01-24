/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	Requires mini.

Copyright (C) 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __FAT_H__
#define __FAT_H__

#include "ff.h"
#include "diskio.h"

u32 fat_mount(void);
u32 fat_umount(void);
u32 fat_clust2sect(u32 clust);

#endif

