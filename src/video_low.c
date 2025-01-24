/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	low-level video support for the BootMii UI

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2009			Haxx Enterprises <bushing@gmail.com>
Copyright (c) 2009		Sven Peter <svenpeter@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

Some routines and initialization constants originally came from the
"GAMECUBE LOW LEVEL INFO" document and sourcecode released by Titanik
of Crazy Nation and the GC Linux project.
*/

#include "bootmii_ppc.h"
#include "video_low.h"
#include "string.h"

#ifdef VI_DEBUG
#define  VI_debug(f, arg...) printf("VI: " f, ##arg);
#else
#define  VI_debug(f, arg...) while(0)
#endif

// hardcoded VI init states
static const u16 VIDEO_Mode640X480NtsciYUV16[64] = {
  0x0F06, 0x0001, 0x4769, 0x01AD, 0x02EA, 0x5140, 0x0003, 0x0018,
  0x0002, 0x0019, 0x410C, 0x410C, 0x40ED, 0x40ED, 0x0043, 0x5A4E,
  0x0000, 0x0000, 0x0043, 0x5A4E, 0x0000, 0x0000, 0x0000, 0x0000,
  0x1107, 0x01AE, 0x1001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
  0x0000, 0x0000, 0x0000, 0x0000, 0x2850, 0x0100, 0x1AE7, 0x71F0,
  0x0DB4, 0xA574, 0x00C1, 0x188E, 0xC4C0, 0xCBE2, 0xFCEC, 0xDECF,
  0x1313, 0x0F08, 0x0008, 0x0C0F, 0x00FF, 0x0000, 0x0000, 0x0000,
  0x0280, 0x0000, 0x0000, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF};

static const u16 VIDEO_Mode640X480Pal50YUV16[64] = {
  0x11F5, 0x0101, 0x4B6A, 0x01B0, 0x02F8, 0x5640, 0x0001, 0x0023,
  0x0000, 0x0024, 0x4D2B, 0x4D6D, 0x4D8A, 0x4D4C, 0x0043, 0x5A4E,
  0x0000, 0x0000, 0x0043, 0x5A4E, 0x0000, 0x0000, 0x013C, 0x0144,
  0x1139, 0x01B1, 0x1001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
  0x0000, 0x0000, 0x0000, 0x0000, 0x2850, 0x0100, 0x1AE7, 0x71F0,
  0x0DB4, 0xA574, 0x00C1, 0x188E, 0xC4C0, 0xCBE2, 0xFCEC, 0xDECF,
  0x1313, 0x0F08, 0x0008, 0x0C0F, 0x00FF, 0x0000, 0x0000, 0x0000,
  0x0280, 0x0000, 0x0000, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF};

static const u16 VIDEO_Mode640X480Pal60YUV16[64] = {
  0x0F06, 0x0001, 0x4769, 0x01AD, 0x02EA, 0x5140, 0x0003, 0x0018,
  0x0002, 0x0019, 0x410C, 0x410C, 0x40ED, 0x40ED, 0x0043, 0x5A4E,
  0x0000, 0x0000, 0x0043, 0x5A4E, 0x0000, 0x0000, 0x0005, 0x0176,
  0x1107, 0x01AE, 0x1001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
  0x0000, 0x0000, 0x0000, 0x0000, 0x2850, 0x0100, 0x1AE7, 0x71F0,
  0x0DB4, 0xA574, 0x00C1, 0x188E, 0xC4C0, 0xCBE2, 0xFCEC, 0xDECF,
  0x1313, 0x0F08, 0x0008, 0x0C0F, 0x00FF, 0x0000, 0x0000, 0x0000,
  0x0280, 0x0000, 0x0000, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF};

static const u16 VIDEO_Mode640X480NtscpYUV16[64] = {
  0x1E0C, 0x0005, 0x4769, 0x01AD, 0x02EA, 0x5140, 0x0006, 0x0030,
  0x0006, 0x0030, 0x81D8, 0x81D8, 0x81D8, 0x81D8, 0x0015, 0x77A0,
  0x0000, 0x0000, 0x0015, 0x77A0, 0x0000, 0x0000, 0x022A, 0x01D6,
  0x120E, 0x0001, 0x1001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
  0x0000, 0x0000, 0x0000, 0x0000, 0x2828, 0x0100, 0x1AE7, 0x71F0,
  0x0DB4, 0xA574, 0x00C1, 0x188E, 0xC4C0, 0xCBE2, 0xFCEC, 0xDECF,
  0x1313, 0x0F08, 0x0008, 0x0C0F, 0x00FF, 0x0000, 0x0001, 0x0001,
  0x0280, 0x807A, 0x019C, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF};

static int video_mode;

void VIDEO_Init(int VideoMode)
{
	u32 Counter=0;
	const u16 *video_initstate=NULL;

	VI_debug("Resetting VI...\n");
	write16(R_VIDEO_STATUS1, 2);
	udelay(2);
	write16(R_VIDEO_STATUS1, 0);
	VI_debug("VI reset...\n");

	switch(VideoMode)
	{
	case VIDEO_640X480_NTSCi_YUV16:
		video_initstate = VIDEO_Mode640X480NtsciYUV16;
		break;

	case VIDEO_640X480_PAL50_YUV16:
		video_initstate = VIDEO_Mode640X480Pal50YUV16;
		break;

	case VIDEO_640X480_PAL60_YUV16:
		video_initstate = VIDEO_Mode640X480Pal60YUV16;
		break;

	case VIDEO_640X480_NTSCp_YUV16:
		video_initstate = VIDEO_Mode640X480NtscpYUV16;
		break;

	/* Use NTSC as default */
	default:
		VideoMode = VIDEO_640X480_NTSCi_YUV16;
		video_initstate = VIDEO_Mode640X480NtsciYUV16;
		break;
	}
	
	VI_debug("Configuring VI...\n");
	for(Counter=0; Counter<64; Counter++)
	{
		if(Counter==1)
			write16(MEM_VIDEO_BASE + 2*Counter, video_initstate[Counter] & 0xFFFE);
		else
			write16(MEM_VIDEO_BASE + 2*Counter, video_initstate[Counter]);
	}

	video_mode = VideoMode;

	write16(R_VIDEO_STATUS1, video_initstate[1]);
#ifdef VI_DEBUG
	VI_debug("VI dump:\n");
	for(Counter=0; Counter<32; Counter++)
		printf("%02x: %04x %04x,\n", Counter*4, read16(MEM_VIDEO_BASE + Counter*4), read16(MEM_VIDEO_BASE + Counter*4+2));

	printf("---\n");
#endif
}

void VIDEO_SetFrameBuffer(void *FrameBufferAddr)
{
	u32 fb = virt_to_phys(FrameBufferAddr);

	write32(R_VIDEO_FRAMEBUFFER_1, (fb >> 5) | 0x10000000);
	if(video_mode != VIDEO_640X480_NTSCp_YUV16)
		fb += 2 * 640; // 640 pixels == 1 line
	write32(R_VIDEO_FRAMEBUFFER_2, (fb >> 5) | 0x10000000);
}

void VIDEO_WaitVSync(void)
{
	while(read16(R_VIDEO_HALFLINE_1) >= 200);
	while(read16(R_VIDEO_HALFLINE_1) <  200);
}

/* black out video (not reversible!) */
void VIDEO_BlackOut(void)
{
	VIDEO_WaitVSync();

	int active = read32(R_VIDEO_VTIMING) >> 4;

	write32(R_VIDEO_PRB_ODD, read32(R_VIDEO_PRB_ODD) + ((active<<1)-2));
	write32(R_VIDEO_PRB_EVEN, read32(R_VIDEO_PRB_EVEN) + ((active<<1)-2));
	write32(R_VIDEO_PSB_ODD, read32(R_VIDEO_PSB_ODD) + 2);
	write32(R_VIDEO_PSB_EVEN, read32(R_VIDEO_PSB_EVEN) + 2);

	mask32(R_VIDEO_VTIMING, 0xfffffff0, 0);
}

//static vu16* const _viReg = (u16*)0xCC002000;

void VIDEO_Shutdown(void)
{
	VIDEO_BlackOut();
	write16(R_VIDEO_STATUS1, 0);
}

#define		HW_REG_BASE		0xd800000

// PPC side of GPIO1 (Starlet can access this too)
// Output state
#define		HW_GPIO1BOUT		(HW_REG_BASE + 0x0c0)
// Direction (1=output)
#define		HW_GPIO1BDIR		(HW_REG_BASE + 0x0c4)
// Input state
#define		HW_GPIO1BIN			(HW_REG_BASE + 0x0c8)

#define SLAVE_AVE 0xe0

static inline void aveSetDirection(u32 dir)
{
	u32 val = (read32(HW_GPIO1BDIR)&~0x8000)|0x4000;
	if(dir) val |= 0x8000;
	write32(HW_GPIO1BDIR, val);
}

static inline void aveSetSCL(u32 scl)
{
	u32 val = read32(HW_GPIO1BOUT)&~0x4000;
	if(scl) val |= 0x4000;
	write32(HW_GPIO1BOUT, val);
}

static inline void aveSetSDA(u32 sda)
{
	u32 val = read32(HW_GPIO1BOUT)&~0x8000;
	if(sda) val |= 0x8000;
	write32(HW_GPIO1BOUT, val);
}

static inline u32 aveGetSDA()
{
	if(read32(HW_GPIO1BIN)&0x8000)
		return 1;
	else
		return 0;
}

static u32 __sendSlaveAddress(u8 addr)
{
	u32 i;

	aveSetSDA(0);
	udelay(2);

	aveSetSCL(0);
	for(i=0;i<8;i++) {
		if(addr&0x80) aveSetSDA(1);
		else aveSetSDA(0);
		udelay(2);

		aveSetSCL(1);
		udelay(2);

		aveSetSCL(0);
		addr <<= 1;
	}

	aveSetDirection(0);
	udelay(2);

	aveSetSCL(1);
	udelay(2);

	if(aveGetSDA()!=0) {
		VI_debug("No ACK\n");
		return 0;
	}

	aveSetSDA(0);
	aveSetDirection(1);
	aveSetSCL(0);

	return 1;
}

static u32 __VISendI2CData(u8 addr,void *val,u32 len)
{
	u8 c;
	u32 i,j;
	u32 ret;

	VI_debug("I2C[%02x]:",addr);
	for(i=0;i<len;i++)
		VI_debug(" %02x", ((u8*)val)[i]);
	VI_debug("\n");

	aveSetDirection(1);
	aveSetSCL(1);

	aveSetSDA(1);
	udelay(4);

	ret = __sendSlaveAddress(addr);
	if(ret==0) {
		return 0;
	}

	aveSetDirection(1);
	for(i=0;i<len;i++) {
		c = ((u8*)val)[i];
		for(j=0;j<8;j++) {
			if(c&0x80) aveSetSDA(1);
			else aveSetSDA(0);
			udelay(2);

			aveSetSCL(1);
			udelay(2);
			aveSetSCL(0);

			c <<= 1;
		}
		aveSetDirection(0);
		udelay(2);
		aveSetSCL(1);
		udelay(2);

		if(aveGetSDA()!=0) {
			VI_debug("No ACK\n");
			return 0;
		}

		aveSetSDA(0);
		aveSetDirection(1);
		aveSetSCL(0);
	}

	aveSetDirection(1);
	aveSetSDA(0);
	udelay(2);
	aveSetSDA(1);

	return 1;
}

static void __VIWriteI2CRegister8(u8 reg, u8 data)
{
	u8 buf[2];
	buf[0] = reg;
	buf[1] = data;
	__VISendI2CData(SLAVE_AVE,buf,2);
	udelay(2);
}

static void __VIWriteI2CRegister16(u8 reg, u16 data)
{
	u8 buf[3];
	buf[0] = reg;
	buf[1] = data >> 8;
	buf[2] = data & 0xFF;
	__VISendI2CData(SLAVE_AVE,buf,3);
	udelay(2);
}

static void __VIWriteI2CRegister32(u8 reg, u32 data)
{
	u8 buf[5];
	buf[0] = reg;
	buf[1] = data >> 24;
	buf[2] = (data >> 16) & 0xFF;
	buf[3] = (data >> 8) & 0xFF;
	buf[4] = data & 0xFF;
	__VISendI2CData(SLAVE_AVE,buf,5);
	udelay(2);
}

static void __VIWriteI2CRegisterBuf(u8 reg, int size, u8 *data)
{
	u8 buf[0x100];
	buf[0] = reg;
	memcpy(&buf[1], data, size);
	__VISendI2CData(SLAVE_AVE,buf,size+1);
	udelay(2);
}

static void __VISetYUVSEL(u8 dtvstatus)
{
	int vdacFlagRegion;
	switch(video_mode) {
	case VIDEO_640X480_NTSCi_YUV16:
	case VIDEO_640X480_NTSCp_YUV16:
	default:
		vdacFlagRegion = 0;
		break;
	case VIDEO_640X480_PAL50_YUV16:
	case VIDEO_640X480_PAL60_YUV16:
		vdacFlagRegion = 2;
		break;
	}
	__VIWriteI2CRegister8(0x01, (dtvstatus<<5) | (vdacFlagRegion&0x1f));
}

static void __VISetFilterEURGB60(u8 enable)
{
	__VIWriteI2CRegister8(0x6e, enable);
}

void VISetupEncoder(void)
{
	u8 macrobuf[0x1a];

	u8 gamma[0x21] = {
		0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00,
		0x10, 0x00, 0x10, 0x00, 0x10, 0x20, 0x40, 0x60,
		0x80, 0xa0, 0xeb, 0x10, 0x00, 0x20, 0x00, 0x40,
		0x00, 0x60, 0x00, 0x80, 0x00, 0xa0, 0x00, 0xeb,
		0x00
	};

	u8 dtv;

	//tv = VIDEO_GetCurrentTvMode();
	dtv = read16(R_VIDEO_VISEL) & 1;
	//oldDtvStatus = dtv;

	// SetRevolutionModeSimple

	VI_debug("DTV status: %d\n", dtv);

	memset(macrobuf, 0, 0x1a);

	__VIWriteI2CRegister8(0x6a, 1);
	__VIWriteI2CRegister8(0x65, 1);
	__VISetYUVSEL(dtv);
	__VIWriteI2CRegister8(0x00, 0);
	__VIWriteI2CRegister16(0x71, 0x8e8e);
	__VIWriteI2CRegister8(0x02, 7);
	__VIWriteI2CRegister16(0x05, 0x0000);
	__VIWriteI2CRegister16(0x08, 0x0000);
	__VIWriteI2CRegister32(0x7A, 0x00000000);

	// Macrovision crap
	__VIWriteI2CRegisterBuf(0x40, sizeof(macrobuf), macrobuf);

	// Sometimes 1 in RGB mode? (reg 1 == 3)
	__VIWriteI2CRegister8(0x0A, 0);

	__VIWriteI2CRegister8(0x03, 1);

	__VIWriteI2CRegisterBuf(0x10, sizeof(gamma), gamma);

	__VIWriteI2CRegister8(0x04, 1);
	__VIWriteI2CRegister32(0x7A, 0x00000000);
	__VIWriteI2CRegister16(0x08, 0x0000);
	__VIWriteI2CRegister8(0x03, 1);

	//if(tv==VI_EURGB60) __VISetFilterEURGB60(1);
	//else
	__VISetFilterEURGB60(0);

	//oldTvStatus = tv;
}

