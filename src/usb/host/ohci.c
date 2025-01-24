/*
       ppcskel - a Free Software replacement for the Nintendo/BroadOn bootloader.
       ohci hardware support

Copyright (C) 2009     Bernhard Urban <lewurm@gmx.net>
Copyright (C) 2009     Sebastian Falbesoner <sebastian.falbesoner@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "../../bootmii_ppc.h"
#include "../../hollywood.h"
#include "../../irq.h"
#include "../../string.h"
#include "../../malloc.h"
#include "ohci.h"
#include "host.h"
#include "../usbspec/usb11spec.h"

/* activate control_quirk */
#define _USE_C_Q

/* macro for accessing u32 variables that need to be in little endian byte order;
 *
 * whenever you read or write from an u32 field that the ohci host controller
 * will read or write from too, use this macro for access!
 */
#define LE(dword) (u32)( (((dword) & 0xFF000000) >> 24) | \
			   (((dword) & 0x00FF0000) >> 8)  | \
			   (((dword) & 0x0000FF00) << 8)  | \
			   (((dword) & 0x000000FF) << 24) )

static struct general_td *allocate_general_td();
static void dbg_op_state(u32 reg);
static void configure_ports(u8 from_init, u32 reg);
static struct usb_device *setup_port(u32 ohci, u32 reg, u8 pport, u8 from_init);
static void set_target_hcca(u32 reg);

static struct ohci_hcca hcca_oh0;
static struct ohci_hcca hcca_oh1;
struct ohci_hcca *hcca;

static struct general_td *allocate_general_td()
{
	struct general_td *td;
	td = (struct general_td *)memalign(16, sizeof(struct general_td));
	memset(td, 0, sizeof(struct general_td));
	td->flags = LE(0);
	td->nexttd = LE(0);
	td->cbp = td->be = LE(0);
	return td;
}


static void dbg_op_state(u32 reg) 
{
	switch (read32(reg+OHCI_HC_CONTROL) & OHCI_CTRL_HCFS) {
		case OHCI_USB_SUSPEND:
			printf("ohci-- OHCI_USB_SUSPEND\n");
			break;
		case OHCI_USB_RESET:
			printf("ohci-- OHCI_USB_RESET\n");
			break;
		case OHCI_USB_OPER:
			printf("ohci-- OHCI_USB_OPER\n");
			break;
		case OHCI_USB_RESUME:
			printf("ohci-- OHCI_USB_RESUME\n");
			break;
	}
}

#ifdef _DU_OHCI_F_HALT
static void dbg_td_flag(u32 flag)
{
	printf("**************** dbg_td_flag: 0x%08X ***************\n", flag);
	printf("CC: %X\tshould be 0, see page 32 (ohci spec)\n", (flag>>28)&0xf);
	printf("EC: %X\tsee page 20 (ohci spec)\n", (flag>>26)&3);
	printf(" T: %X\n", (flag>>24)&3);
	printf("DI: %X\n", (flag>>21)&7);
	printf("DP: %X\n", (flag>>19)&3);
	printf(" R: %X\n", (flag>>18)&1);
	printf("********************************************************\n");
}
#endif

static void general_td_fill(struct general_td *dest, const struct usb_transfer_descriptor *src)
{
	if(src->actlen) {
		dest->cbp = LE(virt_to_phys(src->buffer));
		dest->be = LE(LE(dest->cbp) + src->actlen - 1);
		/* save virtual address here */
		dest->bufaddr = (u32) src->buffer;
	}
	else {
		dest->cbp = dest->be = LE(0);
		dest->bufaddr = 0;
	}

	dest->buflen = src->actlen;

	dest->flags &= LE(~OHCI_TD_DIRECTION_PID_MASK);
	switch(src->pid) {
		case USB_PID_SETUP:
#ifdef _DU_OHCI_Q
			printf("pid_setup\n");
#endif
			dest->flags |= LE(OHCI_TD_DIRECTION_PID_SETUP);
			dest->flags |= LE(OHCI_TD_TOGGLE_0);
			dest->flags |= LE(OHCI_TD_BUFFER_ROUNDING);
			break;
		case USB_PID_OUT:
#ifdef _DU_OHCI_Q
			printf("pid_out\n");
#endif
			dest->flags |= LE(OHCI_TD_DIRECTION_PID_OUT);
			dest->flags |= LE(OHCI_TD_BUFFER_ROUNDING);

			dest->flags |= src->togl ? LE(OHCI_TD_TOGGLE_1) : LE(OHCI_TD_TOGGLE_0);
			break;
		case USB_PID_IN:
#ifdef _DU_OHCI_Q
			printf("pid_in\n");
#endif
			dest->flags |= LE(OHCI_TD_DIRECTION_PID_IN);
			if(src->maxp > src->actlen) {
				dest->flags |= LE(OHCI_TD_BUFFER_ROUNDING);
#ifdef _DU_OHCI_Q
				printf("round buffer!\n");
#endif
			}
			dest->flags |= src->togl ? LE(OHCI_TD_TOGGLE_1) : LE(OHCI_TD_TOGGLE_0);
			break;
	}
	dest->flags |= LE(OHCI_TD_SET_DELAY_INTERRUPT(7));
}

#ifdef _DU_OHCI_F_HALT
static void dump_address(void *addr, u32 size, const char* str)
{
	printf("%s hexdump (%d) @ 0x%08X:\n", str, size, addr);
	hexdump(addr, size);
}
#endif

static struct endpoint_descriptor _edhead;
struct endpoint_descriptor *edhead = 0;
void hcdi_fire(u32 reg)
{
#ifdef _DU_OHCI_F
	printf("<^>  <^>  <^> hcdi_fire(start)\n");
#endif

	if(edhead == 0)
		return;

	u8 itmp;
	switch(edhead->type) {
		case USB_CTRL:
#ifdef _USE_C_Q
			/* quirk... 11ms seems to be a minimum :O */
			udelay(11000);
#endif
			write32(reg+OHCI_HC_CTRL_HEAD_ED, virt_to_phys(edhead));
		break;

		case USB_INTR:
			//udelay(11000);
			set_target_hcca(reg);
			sync_before_read(hcca, sizeof(struct ohci_hcca));
			for(itmp = 0; itmp < NUM_INITS; itmp++) {
				hcca->int_table[itmp] = LE(virt_to_phys(edhead));
			}
			sync_after_write(hcca, sizeof(struct ohci_hcca));
		break;

		case USB_BULK:
			write32(reg+OHCI_HC_BULK_HEAD_ED, virt_to_phys(edhead));
		break;

		case USB_ISOC:
		break;
	}

	/* sync it all */
	sync_after_write(edhead, sizeof(struct endpoint_descriptor));
#ifdef _DU_OHCI_F
	dump_address(edhead, sizeof(struct endpoint_descriptor), "edhead(before)");
#endif

	struct general_td *x = phys_to_virt(LE(edhead->headp) & OHCI_ENDPOINT_HEAD_MASK);
	while(virt_to_phys(x)) {
		sync_after_write(x, sizeof(struct general_td));
#ifdef _DU_OHCI_F
		dump_address(x, sizeof(struct general_td), "x(before)");
#endif

		if(x->buflen > 0) {
			sync_after_write((void*) phys_to_virt(LE(x->cbp)), x->buflen);
#ifdef _DU_OHCI_F
			dump_address((void*) phys_to_virt(LE(x->cbp)), x->buflen, "x->cbp(before)");
#endif
		}
		x = phys_to_virt(LE(x->nexttd));
	}

	/* start transfer */
	switch(edhead->type) {
		case USB_CTRL:
			/* trigger control list */
			set32(reg+OHCI_HC_CONTROL, OHCI_CTRL_CLE);
			write32(reg+OHCI_HC_COMMAND_STATUS, OHCI_CLF);
			break;

		case USB_INTR:
			/* trigger periodic list */
			set32(reg+OHCI_HC_CONTROL, OHCI_CTRL_PLE);
			break;

		case USB_BULK:
			/* trigger bulk list */
			set32(reg+OHCI_HC_CONTROL, OHCI_CTRL_BLE);
			write32(reg+OHCI_HC_COMMAND_STATUS, OHCI_BLF);
			break;

		case USB_ISOC:
			break;
	}

	struct general_td *n=0, *prev = 0, *next = 0;
	/* poll until edhead->headp is null */
	do {
		sync_before_read(edhead, sizeof(struct endpoint_descriptor));
#ifdef _DU_OHCI_F
		printf("edhead->headp: 0x%08X\n", LE(edhead->headp));
#endif

		/* if halted, debug output plz. will break the transfer */
		if((LE(edhead->headp) & OHCI_ENDPOINT_HALTED)) {
			n = phys_to_virt(LE(edhead->headp)&~0xf);
			prev = phys_to_virt((u32)prev);
#ifdef _DU_OHCI_F_HALT
			printf("halted!\n");
#endif

			sync_before_read((void*) n, sizeof(struct general_td));
#ifdef _DU_OHCI_F_HALT
			printf("n: 0x%08X\n", n);
			dump_address(n, sizeof(struct general_td), "n(after)");
#endif
			if(n->buflen > 0) {
				sync_before_read((void*) n->bufaddr, n->buflen);
#ifdef _DU_OHCI_F_HALT
				dump_address((void*) n->bufaddr, n->buflen, "n->bufaddr(after)");
#endif
			}
#ifdef _DU_OHCI_F_HALT
			dbg_td_flag(LE(n->flags));
#endif

			sync_before_read((void*) prev, sizeof(struct general_td));
#ifdef _DU_OHCI_F_HALT
			printf("prev: 0x%08X\n", prev);
			dump_address(prev, sizeof(struct general_td), "prev(after)");
#endif
			if(prev->buflen >0) {
				sync_before_read((void*) prev->bufaddr, prev->buflen);
#ifdef _DU_OHCI_F_HALT
				dump_address((void*) prev->bufaddr, prev->buflen, "prev->bufaddr(after)");
#endif
			}
#ifdef _DU_OHCI_F_HALT
			dbg_td_flag(LE(prev->flags));
			printf("halted end!\n");
#endif
			goto out;
		}
		prev = (struct general_td*) (LE(edhead->headp)&~0xf);
	} while(LE(edhead->headp)&~0xf);

	n = phys_to_virt(read32(reg+OHCI_HC_DONE_HEAD) & ~1);
#ifdef _DU_OHCI_F
	printf("hc_done_head: 0x%08X\n", read32(reg+OHCI_HC_DONE_HEAD));
#endif

	prev = 0; next = 0;
	/* reverse done queue */
	while(virt_to_phys(n) && edhead->tdcount) {
		sync_before_read((void*) n, sizeof(struct general_td));
#ifdef _DU_OHCI_F
		printf("n: 0x%08X\n", n);
		printf("next: 0x%08X\n", next);
		printf("prev: 0x%08X\n", prev);
#endif

		next = n;
		n = (struct general_td*) phys_to_virt(LE(n->nexttd));
		next->nexttd = (u32) prev;
		prev = next;

		edhead->tdcount--;
	}

	n = next;
	prev = 0;
	while(virt_to_phys(n)) {
#ifdef _DU_OHCI_F
		dump_address(n, sizeof(struct general_td), "n(after)");
#endif
		if(n->buflen > 0) {
			sync_before_read((void*) n->bufaddr, n->buflen);
#ifdef _DU_OHCI_F
			dump_address((void*) n->bufaddr, n->buflen, "n->bufaddr(after)");
#endif
		}
#ifdef _DU_OHCI_F
		dbg_td_flag(LE(n->flags));
#endif
		prev = n;
		n = (struct general_td*) n->nexttd;
		free(prev);
	}

out:
	set_target_hcca(reg);
	sync_before_read(hcca, sizeof(struct ohci_hcca));

	u8 jtmp;
	switch(edhead->type) {
		case USB_CTRL:
			write32(reg+OHCI_HC_CONTROL, read32(reg+OHCI_HC_CONTROL)&~OHCI_CTRL_CLE);
			break;

		case USB_INTR:
			write32(reg+OHCI_HC_CONTROL, read32(reg+OHCI_HC_CONTROL)&~OHCI_CTRL_PLE);
			for(jtmp = 0; jtmp < NUM_INITS; jtmp++) {
				hcca->int_table[jtmp] = 0;
			}
			break;

		case USB_BULK:
			write32(reg+OHCI_HC_CONTROL, read32(reg+OHCI_HC_CONTROL)&~OHCI_CTRL_BLE);
			break;

		case USB_ISOC:
			break;
	}

	hcca->done_head = 0;
	sync_after_write(hcca, sizeof(struct ohci_hcca));

	edhead = 0;

#ifdef _DU_OHCI_F
	printf("<^>  <^>  <^> hcdi_fire(end)\n");
#endif
}

/**
 * Enqueue a transfer descriptor.
 */
u8 hcdi_enqueue(const struct usb_transfer_descriptor *td, u32 reg) {
#ifdef _DU_OHCI_Q
	printf("*()*()*()*()*()*()*() hcdi_enqueue(start)\n");
#endif
	if(!edhead) {
		edhead = &_edhead;
		memset(edhead, 0, sizeof(struct endpoint_descriptor));
		edhead->flags = LE(OHCI_ENDPOINT_GENERAL_FORMAT);
		edhead->headp = edhead->tailp = edhead->nexted = LE(0);
		if(td->fullspeed) {
			edhead->flags |= LE(OHCI_ENDPOINT_FULL_SPEED);
		} else {
			edhead->flags |= LE(OHCI_ENDPOINT_LOW_SPEED);
		}
		edhead->flags |= LE(OHCI_ENDPOINT_SET_DEVICE_ADDRESS(td->devaddress) |
				OHCI_ENDPOINT_SET_ENDPOINT_NUMBER(td->endpoint) |
				OHCI_ENDPOINT_SET_MAX_PACKET_SIZE(td->maxp));
		edhead->tdcount = 0;
		edhead->type = td->type;
	}

	struct general_td *tdhw = allocate_general_td();
	general_td_fill(tdhw, td);
	edhead->tdcount ++;

	if(!edhead->headp) {
		/* first transfer */
		edhead->headp = LE(virt_to_phys((void*) ((u32)tdhw & OHCI_ENDPOINT_HEAD_MASK)));
	}
	else {
		/* headp in endpoint already exists
		 * => go to list end
		 */
		struct general_td *n = (struct general_td*) phys_to_virt(LE(edhead->headp) & OHCI_ENDPOINT_HEAD_MASK);
		while(LE(n->nexttd)) {
			n = phys_to_virt(LE(n->nexttd));
		}
		n->nexttd = LE(virt_to_phys((void*) ((u32)tdhw & OHCI_ENDPOINT_HEAD_MASK)));
#ifdef _DU_OHCI_Q
		printf("n: 0x%08X\n", n);
		printf("n->nexttd: 0x%08X\n", phys_to_virt(LE(n->nexttd)));
#endif
	}

#ifdef _DU_OHCI_Q
	printf("*()*()*()*()*()*()*() hcdi_enqueue(end)\n");
#endif
	return 0;
}


/**
 * Remove an transfer descriptor from transfer queue.
 */
u8 hcdi_dequeue(struct usb_transfer_descriptor *td, u32 reg) {
	return 0;
}

void hcdi_init(u32 reg)
{
	printf("ohci-- init\n");
	dbg_op_state(reg);

	/* disable hc interrupts */
	set32(reg+OHCI_HC_INT_DISABLE, OHCI_INTR_MIE);

	/* save fmInterval and calculate FSMPS */
#define FSMP(fi) (0x7fff & ((6 * ((fi) - 210)) / 7))
#define FI 0x2edf /* 12000 bits per frame (-1) */
	u32 fmint = read32(reg+OHCI_HC_FM_INTERVAL) & 0x3fff;
	if(fmint != FI)
		printf("ohci-- fminterval delta: %d\n", fmint - FI);
	fmint |= FSMP (fmint) << 16;

	/* enable interrupts of both usb host controllers */
	set32(EHCI_CTL, EHCI_CTL_OH0INTE | EHCI_CTL_OH1INTE | 0xe0000);

	/* reset HC */
	write32(reg+OHCI_HC_COMMAND_STATUS, OHCI_HCR);

	/* wait max. 30us */
	u32 ts = 30;
	while ((read32(reg+OHCI_HC_COMMAND_STATUS) & OHCI_HCR) != 0) {
		 if(--ts == 0) {
			printf("ohci-- FAILED");
			return;
		 }
		 udelay(1);
	}

	/* disable interrupts; 2ms timelimit here! 
	   now we're in the SUSPEND state ... must go OPERATIONAL
	   within 2msec else HC enters RESUME */

	u32 cookie = irq_kill();

	/* Tell the controller where the control and bulk lists are
	 * The lists are empty now. */
	write32(reg+OHCI_HC_CTRL_HEAD_ED, 0);
	write32(reg+OHCI_HC_BULK_HEAD_ED, 0);

	/* set hcca adress */
	set_target_hcca(reg);
	sync_after_write(hcca, 256);
	write32(reg+OHCI_HC_HCCA, virt_to_phys(hcca));

	/* set periodicstart */
#define FIT (1<<31)
	u32 fmInterval = read32(reg+OHCI_HC_FM_INTERVAL) &0x3fff;
	u32 fit = read32(reg+OHCI_HC_FM_INTERVAL) & FIT;

	write32(reg+OHCI_HC_FM_INTERVAL, fmint | (fit ^ FIT));
	write32(reg+OHCI_HC_PERIODIC_START, ((9*fmInterval)/10)&0x3fff);

	/* testing bla */
	if ((read32(reg+OHCI_HC_FM_INTERVAL) & 0x3fff0000) == 0 || !read32(reg+OHCI_HC_PERIODIC_START)) {
		printf("ohci-- w00t, fail!! see ohci-hcd.c:669\n");
	}
	
	/* start HC operations */
	write32(reg+OHCI_HC_CONTROL, OHCI_CONTROL_INIT | OHCI_USB_OPER);

	/* wake on ConnectStatusChange, matching external hubs */
	write32(reg+OHCI_HC_RH_STATUS, /*RH_HS_DRWE |*/ RH_HS_LPSC);

	/* Choose the interrupts we care about now, others later on demand */
	write32(reg+OHCI_HC_INT_STATUS, ~0);
	write32(reg+OHCI_HC_INT_ENABLE, OHCI_INTR_INIT);

	//wtf?
	wait_ms ((read32(reg+OHCI_HC_RH_DESCRIPTOR_A) >> 23) & 0x1fe);

	configure_ports((u8)1, reg);
	irq_restore(cookie);

	dbg_op_state(reg);
}

static struct usb_device *connected[2] = {NULL, NULL};
static void configure_ports(u8 from_init, u32 reg)
{
#ifdef _DU_OHCI_RH
	printf("=== Roothub @ %s ===\n", reg == OHCI0_REG_BASE ? "OHCI0" : "OHCI1");
	printf("OHCI_HC_RH_DESCRIPTOR_A:\t0x%08X\n", read32(reg+OHCI_HC_RH_DESCRIPTOR_A));
	printf("OHCI_HC_RH_DESCRIPTOR_B:\t0x%08X\n", read32(reg+OHCI_HC_RH_DESCRIPTOR_B));
	printf("OHCI_HC_RH_STATUS:\t\t0x%08X\n", read32(reg+OHCI_HC_RH_STATUS));
	printf("OHCI_HC_RH_PORT_STATUS_1:\t0x%08X\n", read32(reg+OHCI_HC_RH_PORT_STATUS_1));
	printf("OHCI_HC_RH_PORT_STATUS_2:\t0x%08X\n", read32(reg+OHCI_HC_RH_PORT_STATUS_2));
#endif

	struct usb_device *dtmp;
	if(!(dtmp = setup_port(reg, reg+OHCI_HC_RH_PORT_STATUS_1, 0, from_init))) {
		if(connected[0]) {
			usb_remove_device(connected[0]);
			connected[0] = NULL;
		}
	} else {
		connected[0] = dtmp;
	}

	if(!(dtmp = setup_port(reg, reg+OHCI_HC_RH_PORT_STATUS_2, 1, from_init))) {
		if(connected[1]) {
			usb_remove_device(connected[1]);
			connected[1] = NULL;
		}
	} else {
		connected[1] = dtmp;
	}

#ifdef _DU_OHCI_RH
	printf("configure_ports done\n");
#endif
}

static struct usb_device *setup_port(u32 ohci, u32 reg, u8 pport, u8 from_init)
{
	u32 port = read32(reg);
	if((port & RH_PS_CCS) && ((port & RH_PS_CSC) || from_init)) {
		write32(reg, RH_PS_CSC);

		wait_ms(120);

		/* clear CSC flag, set PES and start port reset (PRS) */
		write32(reg, RH_PS_PES);
		while(!(read32(reg) & RH_PS_PES)) {
#ifdef _DU_OHCI_RH
			printf("fu\n");
#endif
			return NULL;
		}

		write32(reg, RH_PS_PRS);

		/* spin until port reset is complete */
		while(!(read32(reg) & RH_PS_PRSC)); // hint: it may stuck here
#ifdef _DU_OHCI_RH
		printf("loop done\n");
#endif

		/* returns usb_device struct */
		return usb_add_device((read32(reg) & RH_PS_LSDA) >> 8, ohci);
	}
	if(port & RH_PS_CCS) {
		return connected[pport];
	}
	return NULL;
}

void hcdi_irq(u32 reg)
{
	/* read interrupt status */
	u32 flags = read32(reg+OHCI_HC_INT_STATUS);

	/* when all bits are set to 1 some problem occured */
	if (flags == 0xffffffff) {
		printf("ohci-- Houston, we have a serious problem! :(\n");
		return;
	}

	/* only care about interrupts that are enabled */
	flags &= read32(reg+OHCI_HC_INT_ENABLE);

	/* nothing to do? */
	if (flags == 0) {
		printf("OHCI Interrupt occured: but not for you! WTF?!\n");
		return;
	}

	printf("OHCI Interrupt occured: ");
	/* UnrecoverableError */
	if (flags & OHCI_INTR_UE) {
		printf("UnrecoverableError\n");
		/* TODO: well, I don't know... nothing,
		 *       because it won't happen anyway? ;-) */
	}

	/* RootHubStatusChange */
	if (flags & OHCI_INTR_RHSC) {
		printf("RootHubStatusChange\n");
		/* TODO: set some next_statechange variable... */
		configure_ports(0, reg);
		write32(reg+OHCI_HC_INT_STATUS, OHCI_INTR_RD | OHCI_INTR_RHSC);
	}
	/* ResumeDetected */
	else if (flags & OHCI_INTR_RD) {
		printf("ResumeDetected\n");
		write32(reg+OHCI_HC_INT_STATUS, OHCI_INTR_RD);
		/* TODO: figure out what the linux kernel does here... */
	}

	/* WritebackDoneHead */
	if (flags & OHCI_INTR_WDH) {
		printf("WritebackDoneHead\n");
		/* basically the linux irq handler reverse TDs to their urbs
		 * and set done_head to null.
		 * since we are polling atm, just should do the latter task.
		 * however, this won't work for now (i don't know why...)
		 * TODO!
		 */
#if 0
		sync_before_read(&hcca_oh0, 256);
		hcca_oh0.done_head = 0;
		sync_after_write(&hcca_oh0, 256);
#endif
	}

	/* TODO: handle any pending URB/ED unlinks... */

#define HC_IS_RUNNING() 1 /* dirty, i know... just a temporary solution */
	if (HC_IS_RUNNING()) {
		write32(reg+OHCI_HC_INT_STATUS, flags);
		write32(reg+OHCI_HC_INT_ENABLE, OHCI_INTR_MIE);
	}
}

/* Before you access the HCCA structure in any way, call this to set the pointer correctly! */
static void set_target_hcca(u32 reg)
{
	switch(reg) {
	case OHCI0_REG_BASE: hcca = &hcca_oh0; break;
	case OHCI1_REG_BASE: hcca = &hcca_oh1; break;
	}
}

void show_frame_no(u32 reg)
{
	set_target_hcca(reg);
	sync_before_read(hcca, 256);
	printf("***** frame_no: %d *****\n", LE(hcca->frame_no));
}
