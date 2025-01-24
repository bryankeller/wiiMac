/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	low-level video support for the BootMii UI

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2009			Haxx Enterprises <bushing@gmail.com>
Copyright (C) 2009		Sven Peter <svenpeter@gmail.com>
# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

Some routines and initialization constants originally came from the
"GAMECUBE LOW LEVEL INFO" document and sourcecode released by Titanik
of Crazy Nation and the GC Linux project.
*/

#ifndef VIDEO_LOW_H
#define VIDEO_LOW_H

#define MEM_VIDEO_BASE               (0xCC002000)           ///< Memory address of Video Interface
#define MEM_VIDEO_BASE_PTR           (u32*)MEM_VIDEO_BASE   ///< Pointer to Video Interface

// 32-bit-wide registers
#define R_VIDEO_VTIMING              (MEM_VIDEO_BASE+0x00)   ///< Vertical timing.
#define R_VIDEO_STATUS1              (MEM_VIDEO_BASE+0x02)   ///< Status? register location.
#define R_VIDEO_PSB_ODD              (MEM_VIDEO_BASE+0x00)   ///< Postblank odd.
#define R_VIDEO_PRB_ODD              (MEM_VIDEO_BASE+0x00)   ///< Preblank odd.
#define R_VIDEO_PSB_EVEN             (MEM_VIDEO_BASE+0x00)   ///< Postblank even.
#define R_VIDEO_PRB_EVEN             (MEM_VIDEO_BASE+0x00)   ///< Preblank even.
#define R_VIDEO_FRAMEBUFFER_1        (MEM_VIDEO_BASE+0x1C)   ///< Framebuffer1 register location.
#define R_VIDEO_FRAMEBUFFER_2        (MEM_VIDEO_BASE+0x24)   ///< Framebuffer2 register location.
// 16-bit-wide registers
#define R_VIDEO_HALFLINE_1           (MEM_VIDEO_BASE+0x2C)   ///< HalfLine1 register location.
#define R_VIDEO_HALFLINE_2           (MEM_VIDEO_BASE+0x2E)   ///< HalfLine2 register location.
#define R_VIDEO_STATUS               (MEM_VIDEO_BASE+0x6C)   ///< VideoStatus register location.
#define R_VIDEO_VISEL                (MEM_VIDEO_BASE+0x6E)   // cable detect

// Constants for VIDEO_Init()
#define VIDEO_640X480_NTSCi_YUV16    (0)
#define VIDEO_640X480_PAL50_YUV16    (1)
#define VIDEO_640X480_PAL60_YUV16    (2)
#define VIDEO_640X480_NTSCp_YUV16    (3)

// Constants for VIDEO_SetFrameBuffer
#define VIDEO_FRAMEBUFFER_1          (1)
#define VIDEO_FRAMEBUFFER_2          (2)
#define VIDEO_FRAMEBUFFER_BOTH       (0)

void VIDEO_Init            (int VideoMode);
void VIDEO_SetFrameBuffer  (void *FrameBufferAddr);
void VIDEO_WaitVSync       (void);
void VIDEO_BlackOut        (void);
void VIDEO_Shutdown        (void);
void VISetupEncoder        (void);

#endif /* VIDEO_H */

