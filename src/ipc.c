/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	Requires mini.

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>
Copyright (C) 2009		John Kelley <wiidev@kelley.ca>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "bootmii_ppc.h"
#include "ipc.h"
#include "string.h"
#include <stdarg.h>

static volatile ipc_request *in_queue;
static volatile ipc_request *out_queue;

static int in_size;
static int out_size;

static int initialized = 0;

static u16 out_head;
static u16 in_tail;

typedef const struct {
	char magic[3];
	char version;
	void *mem2_boundary;
	volatile ipc_request *ipc_in;
	u32 ipc_in_size;
	volatile ipc_request *ipc_out;
	u32 ipc_out_size;
} ipc_infohdr;

static ipc_infohdr *infohdr;

static u32 cur_tag;

#define		HW_REG_BASE			0xd000000

#define		HW_IPC_PPCMSG		(HW_REG_BASE + 0x000) //PPC to ARM
#define		HW_IPC_PPCCTRL		(HW_REG_BASE + 0x004)
#define		HW_IPC_ARMMSG		(HW_REG_BASE + 0x008) //ARM to PPC

#define		IPC_CTRL_SEND		0x01
// Set by peer to acknowledge a message. Write one to clear.
#define		IPC_CTRL_SENT		0x02
// Set by peer to send a message. Write one to clear.
#define		IPC_CTRL_RECV		0x04
// Write one acknowledge a message. Cleared when peer writes one to IPC_CTRL_SENT.
#define		IPC_CTRL_RECVD		0x08
// Enable interrupt when a message is received
#define		IPC_CTRL_INT_RECV	0x10
// Enable interrupt when a sent message is acknowledged
#define		IPC_CTRL_INT_SENT	0x20

static inline u16 peek_outtail(void)
{
	return read32(HW_IPC_ARMMSG) & 0xFFFF;
}
static inline u16 peek_inhead(void)
{
	return read32(HW_IPC_ARMMSG) >> 16;
}

static inline void poke_intail(u16 num)
{
	mask32(HW_IPC_PPCMSG, 0xFFFF, num);
}
static inline void poke_outhead(u16 num)
{
	mask32(HW_IPC_PPCMSG, 0xFFFF0000, num<<16);
}

int ipc_initialize(void)
{

	infohdr = (ipc_infohdr*)(read32(0x13fffffc)|0x80000000);
	sync_before_read((void*)infohdr, sizeof(ipc_infohdr));

	printf("IPC: infoheader at %p %08x\n", infohdr);

	if(memcmp(infohdr->magic, "IPC", 3)) {
		printf("IPC: bad magic on info structure\n",infohdr);
		return -1;
	}
	if(infohdr->version != 1) {
		printf("IPC: unknown IPC version %d\n",infohdr->version);
		return -1;
	}

	in_queue = (void*)(((u32)infohdr->ipc_in)|0x80000000);
	out_queue = (void*)(((u32)infohdr->ipc_out)|0x80000000);

	in_size = infohdr->ipc_in_size;
	out_size = infohdr->ipc_out_size;

	in_tail = read32(HW_IPC_PPCMSG) & 0xffff;
	out_head = read32(HW_IPC_PPCMSG) >> 16;

	printf("IPC: initial in tail: %d, out head: %d\n", in_tail, out_head);

	cur_tag = 1;

	initialized = 1;
	return 0;
}

void ipc_shutdown(void)
{
	if(!initialized)
		return;
	ipc_flush();
	initialized = 0;
}

void ipc_vpost(u32 code, u32 tag, u32 num_args, va_list ap)
{
	int arg = 0;
	int n = 0;

	if(!initialized) {
		printf("IPC: not inited\n");
		return;
	}

	if(peek_inhead() == ((in_tail + 1)&(in_size-1))) {
		printf("IPC: in queue full, spinning\n");
		while(peek_inhead() == ((in_tail + 1)&(in_size-1))) {
			udelay(10);
			if(n++ > 20000) {
				printf("IPC: ARM might be stuck, still waiting for inhead %d != %d\n",
					peek_inhead(), ((in_tail + 1)&(in_size-1)));
				n = 0;
			}
		}
	}
	in_queue[in_tail].code = code;
	in_queue[in_tail].tag = tag;
	while(num_args--) {
		in_queue[in_tail].args[arg++] = va_arg(ap, u32);
	}
	sync_after_write((void*)&in_queue[in_tail], 32);
	in_tail = (in_tail+1)&(in_size-1);
	poke_intail(in_tail);
	write32(HW_IPC_PPCCTRL, IPC_CTRL_SEND);
}

void ipc_post(u32 code, u32 tag, u32 num_args, ...)
{
	va_list ap;

	if(num_args)
		va_start(ap, num_args);

	ipc_vpost(code, tag, num_args, ap);

	if(num_args)
		va_end(ap);
}

void ipc_flush(void)
{
	int n = 0;
	if(!initialized) {
		printf("IPC: not inited\n");
		return;
	}
	while(peek_inhead() != in_tail) {
		udelay(10);
		if(n++ > 20000) {
			printf("IPC: ARM might be stuck, still waiting for inhead %d == intail %d\n",
				peek_inhead(), in_tail);
			n = 0;
		}
	}
}

// last IPC message received, copied because we need to make space in the queue
ipc_request req_recv;

// since we're not using IRQs, we don't use the reception bell at all at the moment
ipc_request *ipc_receive(void)
{
	while(peek_outtail() == out_head);
	sync_before_read((void*)&out_queue[out_head], 32);
	req_recv = out_queue[out_head];
	out_head = (out_head+1)&(out_size-1);
	poke_outhead(out_head);

	return &req_recv;
}

void ipc_process_unhandled(volatile ipc_request *rep)
{
	printf("IPC: Unhandled message: %08x %08x [%08x %08x %08x %08x %08x %08x]\n",
		rep->code, rep->tag, rep->args[0], rep->args[1], rep->args[2], rep->args[3], 
		rep->args[4], rep->args[5]);
}

ipc_request *ipc_receive_tagged(u32 code, u32 tag)
{
	ipc_request *rep;
	rep = ipc_receive();
	while(rep->code != code || rep->tag != tag) {
		ipc_process_unhandled(rep);
		rep = ipc_receive();
	}
	return rep;
}

ipc_request *ipc_exchange(u32 code, u32 num_args, ...)
{
	va_list ap;
	ipc_request *rep;

	if(num_args)
		va_start(ap, num_args);

	ipc_vpost(code, cur_tag, num_args, ap);

	if(num_args)
		va_end(ap);

	rep = ipc_receive_tagged(code, cur_tag);

	cur_tag++;
	return rep;
}

