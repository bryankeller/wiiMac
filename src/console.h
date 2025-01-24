/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	Requires mini.

Copyright (C) 2009		bLAStY <blasty@bootmii.org>
Copyright (C) 2009		John Kelley <wiidev@kelley.ca>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

void init_fb(int vmode);
void print_str(const char *str, size_t len);
void print_str_noscroll(int x, int y, char *str);
int console_printf(const char *fmt, ...);
u32 *get_xfb(void);

extern unsigned char console_font_8x16[];

#endif

