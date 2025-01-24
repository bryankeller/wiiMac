/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	Requires mini.

Copyright (C) 2008, 2009	Haxx Enterprises <bushing@gmail.com>
Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>
Copyright (C) 2009		John Kelley <wiidev@kelley.ca>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __IPC_H__
#define __IPC_H__

/* TODO: It would be nice to somehow link this header file with mini/ipc.h.
   Until then, if you do make any changes here, you MUST make them here
   (or vice-versa).  See warnings in mini/ipc.h. --bushing */

#define IPC_SYS_PING	0x01000000
#define IPC_SYS_SLWPING	0x00000000
#define IPC_SYS_JUMP	0x00000001
#define IPC_SYS_GETVERS	0x00000002
#define IPC_SYS_GETGITS 0x00000003

#define IPC_SYS_WRITE32	0x01000100
#define IPC_SYS_WRITE16	0x01000101
#define IPC_SYS_WRITE8	0x01000102
#define IPC_SYS_READ32	0x01000103
#define IPC_SYS_READ16	0x01000104
#define IPC_SYS_READ8	0x01000105
#define IPC_SYS_SET32	0x01000106
#define IPC_SYS_SET16	0x01000107
#define IPC_SYS_SET8	0x01000108
#define IPC_SYS_CLEAR32	0x01000109
#define IPC_SYS_CLEAR16	0x0100010a
#define IPC_SYS_CLEAR8	0x0100010b
#define IPC_SYS_MASK32	0x0100010c
#define IPC_SYS_MASK16	0x0100010d
#define IPC_SYS_MASK8	0x0100010e

#define	IPC_NAND_RESET	0x00010000
#define IPC_NAND_GETID	0x00010001
#define IPC_NAND_READ	0x00010002
#define IPC_NAND_WRITE	0x00010003
#define IPC_NAND_ERASE	0x00010004
#define IPC_NAND_STATUS	0x00010005
//#define IPC_NAND_USER0 0x00018000
//#define IPC_NAND_USER1 0x00018001
// etc.

#define IPC_SDMMC_ACK	0x00070000
#define IPC_SDMMC_READ	0x00070001
#define IPC_SDMMC_WRITE 0x00070002
#define IPC_SDMMC_STATE 0x00070003
#define IPC_SDMMC_SIZE	0x00070004

#define IPC_SDHC_DISCOVER 0x00020000

#define IPC_KEYS_GETOTP	0x00030000
#define IPC_KEYS_GETEEP	0x00030001

#define IPC_AES_RESET	0x00040000
#define IPC_AES_SETIV	0x00040001
#define	IPC_AES_SETKEY	0x00040002
#define	IPC_AES_DECRYPT	0x00040003

#define IPC_BOOT2_RUN	0x00050000
#define IPC_BOOT2_TMD	0x00050001

#define IPC_PPC_BOOT	0x00060000

typedef struct {
	union {
		struct {
			u8 flags;
			u8 device;
			u16 req;
		};
		u32 code;
	};
	u32 tag;
	u32 args[6];
} ipc_request;

extern void *mem2_boundary;

int ipc_initialize(void);
void ipc_shutdown(void);

void ipc_post(u32 code, u32 tag, u32 num_args, ...);

void ipc_flush(void);

ipc_request *ipc_receive(void);
ipc_request *ipc_receive_tagged(u32 code, u32 tag);

ipc_request *ipc_exchange(u32 code, u32 num_args, ...);

static inline void ipc_sys_write32(u32 addr, u32 x)
{
	ipc_post(IPC_SYS_WRITE32, 0, 2, addr, x);
}
static inline void ipc_sys_write16(u32 addr, u16 x)
{
	ipc_post(IPC_SYS_WRITE16, 0, 2, addr, x);
}
static inline void ipc_sys_write8(u32 addr, u8 x)
{
	ipc_post(IPC_SYS_WRITE8, 0, 2, addr, x);
}

static inline u32 ipc_sys_read32(u32 addr)
{
	return ipc_exchange(IPC_SYS_READ32, 1, addr)->args[0];
}
static inline u16 ipc_sys_read16(u32 addr)
{
	return ipc_exchange(IPC_SYS_READ16, 1, addr)->args[0];
}
static inline u8 ipc_sys_read8(u32 addr)
{
	return ipc_exchange(IPC_SYS_READ8, 1, addr)->args[0];
}

static inline void ipc_sys_set32(u32 addr, u32 set)
{
	ipc_post(IPC_SYS_SET32, 0, 2, addr, set);
}
static inline void ipc_sys_set16(u32 addr, u16 set)
{
	ipc_post(IPC_SYS_SET16, 0, 2, addr, set);
}
static inline void ipc_sys_set8(u32 addr, u8 set)
{
	ipc_post(IPC_SYS_SET8, 0, 2, addr, set);
}

static inline void ipc_sys_clear32(u32 addr, u32 clear)
{
	ipc_post(IPC_SYS_CLEAR32, 0, 2, addr, clear);
}
static inline void ipc_sys_clear16(u32 addr, u16 clear)
{
	ipc_post(IPC_SYS_CLEAR16, 0, 2, addr, clear);
}
static inline void ipc_sys_clear8(u32 addr, u8 clear)
{
	ipc_post(IPC_SYS_CLEAR8, 0, 2, addr, clear);
}

static inline void ipc_sys_mask32(u32 addr, u32 clear, u32 set)
{
	ipc_post(IPC_SYS_MASK32, 0, 3, addr, clear, set);
}
static inline void ipc_sys_mask16(u32 addr, u16 clear, u32 set)
{
	ipc_post(IPC_SYS_MASK16, 0, 3, addr, clear, set);
}
static inline void ipc_sys_mask8(u32 addr, u8 clear, u32 set)
{
	ipc_post(IPC_SYS_MASK8, 0, 3, addr, clear, set);
}

static inline void ipc_ping(void)
{
	ipc_exchange(IPC_SYS_PING, 0);
}

static inline void ipc_slowping(void)
{
	ipc_exchange(IPC_SYS_SLWPING, 0);
}

static inline u32 ipc_getvers(void)
{
	return ipc_exchange(IPC_SYS_GETVERS, 0)->args[0];
}

#endif

