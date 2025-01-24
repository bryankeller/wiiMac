/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	Requires mini.

Copyright (C) 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>
Copyright (C) 2009		John Kelley <wiidev@kelley.ca>
Copyright (C) 2009		bLAStY <blasty@bootmii.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __INPUT_H__
#define __INPUT_H__

#define PAD_LEFT  0x0001
#define PAD_RIGHT 0x0002
#define PAD_DOWN  0x0004
#define PAD_UP    0x0008
#define PAD_Z     0x0010
#define PAD_RS    0x0020
#define PAD_LS    0x0040
//    unused      0x0080
#define PAD_A     0x0100
#define PAD_B     0x0200
#define PAD_X     0x0400
#define PAD_Y     0x0800
#define PAD_START 0x1000
#define PAD_ANY   0x1F7F

#define GPIO_POWER	PAD_RIGHT
#define GPIO_RESET	PAD_A
#define GPIO_EJECT	PAD_START

typedef struct {
        u16 btns_held;
        u16 btns_up;
        u16 btns_down;
		s8 x, y, cx, cy;
		u8 l, r;
} GC_Pad;

void input_init(void);

u16 pad_read(GC_Pad *pad, int chan);
u16 gpio_read(void);

u16 input_read(void);
u16 input_wait(void);

#endif

