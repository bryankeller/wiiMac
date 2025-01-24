/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	Requires mini.

Copyright (C) 2009		bLAStY <blasty@bootmii.org>
Copyright (C) 2009		John Kelley <wiidev@kelley.ca>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "bootmii_ppc.h"

#include "video_low.h"
#include "console.h"
#include "string.h"
#include "printf.h"
#include "malloc.h"

#include <stdarg.h>

typedef struct {
        u32 x, y;
        u32 width, height;
        u8 *raw_data;
        u32 *yuv_data;  
        u32 active;
        u8 has_alpha;
} gfx_rect;

#define CONSOLE_WIDTH 500
#define CONSOLE_LINES 10
#define CONSOLE_Y 100

#define CONSOLE_X  58
#define CONSOLE_CHAR_HEIGHT 16
#define CONSOLE_ROW_HEIGHT (CONSOLE_CHAR_HEIGHT + 1) 

static u32 *xfb = NULL;
static int y_add = 0;

u32 *font_yuv[255];

u32 *get_xfb(void) {
	return xfb;
}

static void memcpy32(u32 *dst, u32 *src, u32 count) {
	while(count--) {
		*dst = *src;
		sync_after_write((const void *)dst, 4);

		dst++;
		src++;
	}
}

static void memset32(u32 *dst, u32 setval, u32 count) {
	while(count--) {
		*dst = setval;
		sync_after_write((const void *)dst, 4);

		dst++;
	}
}

u32 pal_idx(int i, u8 *pal, u8 *gfx) { 
	u32 pidx = gfx[i];
	pidx *= 3;

	return (pal[pidx+0] << 16) | (pal[pidx+1] << 8) | (pal[pidx+2]);
}

int make_yuv(u8 r1, u8 g1, u8 b1, u8 r2, u8 g2, u8 b2) {
  int y1, cb1, cr1, y2, cb2, cr2, cb, cr;
 
  y1 = (299 * r1 + 587 * g1 + 114 * b1) / 1000;
  cb1 = (-16874 * r1 - 33126 * g1 + 50000 * b1 + 12800000) / 100000;
  cr1 = (50000 * r1 - 41869 * g1 - 8131 * b1 + 12800000) / 100000;
 
  y2 = (299 * r2 + 587 * g2 + 114 * b2) / 1000;
  cb2 = (-16874 * r2 - 33126 * g2 + 50000 * b2 + 12800000) / 100000;
  cr2 = (50000 * r2 - 41869 * g2 - 8131 * b2 + 12800000) / 100000;
 
  cb = (cb1 + cb2) >> 1;
  cr = (cr1 + cr2) >> 1;
 
  return ((y1 << 24) | (cb << 16) | (y2 << 8) | cr);
}

void fill_rect(int x, int y, int w, int h, u8 r, u8 g, u8 b) {
	u32 *fb = xfb;
	u32 col = make_yuv(r,g,b, r,g,b);

	fb += ((y + y_add) * (640 >> 1));
	fb += (x >> 1);

	for(y = 0; y < h; y++) {
		memset32(fb, col, w >> 1);
		fb += (640 >> 1);
	}
}

void gfx_draw_rect(gfx_rect *n) {
        u32 y;
        gfx_rect *d_rect;
        u32 *fb = xfb;

        d_rect = n;

        fb += ((d_rect->y + y_add) * (640 >> 1));
        fb += (d_rect->x >> 1);

        for(y = 0; y < d_rect->height; y++) {
                memcpy32(fb, d_rect->yuv_data + ((d_rect->width >> 1) * y), d_rect->width >> 1);
                fb += (640 >> 1);
        }
}

void scroll(void) {
	unsigned int y;
	u32 *fb = xfb;

	fb += ((CONSOLE_Y+y_add) * (640 >> 1));
	fb += (CONSOLE_X >> 1);

	fb += (CONSOLE_ROW_HEIGHT * (640 >> 1));

	for (y = 0; y < CONSOLE_LINES*CONSOLE_ROW_HEIGHT; y++) {
		memcpy32(fb - (CONSOLE_ROW_HEIGHT * 320), fb, CONSOLE_WIDTH >> 1);
		fb += 320;
	}
	
	fill_rect(CONSOLE_X, CONSOLE_Y+(CONSOLE_LINES-1)*CONSOLE_ROW_HEIGHT,
		CONSOLE_WIDTH, CONSOLE_ROW_HEIGHT, 0, 0, 0);
}

void print_str_noscroll(int x, int y, char *str) {
	unsigned int i;
	gfx_rect d_char;

	d_char.x = x;

	for (i = 0; i < strlen(str); i++) {
		d_char.width  = 8;
		d_char.height = CONSOLE_CHAR_HEIGHT;
		d_char.y = y;

		if (str[i] == '\n') {
			y += CONSOLE_CHAR_HEIGHT;
			d_char.x = x;
			continue;
		}	

		d_char.yuv_data = font_yuv[ (int)str[i] ];
		gfx_draw_rect(&d_char);
		d_char.x += 10;
	}
}

void print_str(const char *str, size_t len) {
	unsigned int i;
	gfx_rect d_char;

	scroll();
	d_char.width  = 8;
	d_char.height = CONSOLE_CHAR_HEIGHT;
	d_char.y = CONSOLE_Y + ((CONSOLE_LINES - 1) * CONSOLE_ROW_HEIGHT);

	for (i = 0; i < len; i++) {
		d_char.x = CONSOLE_X + (i * 10);
		d_char.yuv_data = font_yuv[(int) str[i]];
		gfx_draw_rect(&d_char);
	}
}

int gfx_printf(const char *fmt, ...)
{
	va_list args;
	char buffer[40];
	int i;

	memset(buffer, 0, sizeof buffer);
	va_start(args, fmt);
	i = vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (i > 0) {
		print_str(buffer, i);
		printf("%s\n", buffer);
	} else
		scroll();

	return i;
}

void font_to_yuv(void) {
	int i, x, y;
	u8 lr,lg,lb, rr,rg,rb;

	for (i = 0; i < 255; i++) {
		font_yuv[i] = (u32*)malloc(8*CONSOLE_CHAR_HEIGHT*2);

		for (y = 0; y < CONSOLE_CHAR_HEIGHT; y++) {
			for (x = 0; x < 8; x+=2) {
				if (((console_font_8x16[(i*CONSOLE_CHAR_HEIGHT)+y] >> (7-x)) & 0x01) == 1) {
					lr = 255; lg = 255; lb = 255;
				} else {
					lr = 0; lg = 0; lb = 0;
				}

				if (((console_font_8x16[(i*CONSOLE_CHAR_HEIGHT)+y] >> (7-(x+1))) & 0x01) == 1) {
					rr = 255; rg = 255; rb = 255;
				} else {
					rr = 0; rg = 0; rb = 0;
				}

				font_yuv[i][(y<<2)+(x>>1)] = make_yuv(lr, lg, lb, rr, rg, rb);
			}
		}
	}
}

void init_fb(int vmode) {
	int i;
	u32 *fb;
	u32 fill_col = make_yuv(0,0,0, 0,0,0);

	font_to_yuv();

	switch(vmode) {
	case VIDEO_640X480_NTSCi_YUV16:
	case VIDEO_640X480_PAL60_YUV16:
	case VIDEO_640X480_NTSCp_YUV16:
		y_add = 0;
		break;

	case VIDEO_640X480_PAL50_YUV16:
		y_add = 48;
		break;
	}

	xfb = memalign(32, 640 * (480 + (y_add*2)) * 2);

	fb  = xfb;
	for (i = 0; i < (480 + (y_add*2)) * 2 * (640 >> 1); i++) {
		*fb = fill_col;
		sync_after_write(fb, 4);
		fb++;
	}
}

