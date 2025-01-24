/*
       ppcskel - a Free Software replacement for the Nintendo/BroadOn bootloader.
       ohci hardware support

Copyright (C) 2009     Bernhard Urban <lewurm@gmx.net>
Copyright (C) 2009     Sebastian Falbesoner <sebastian.falbesoner@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __OHCI_H__
#define __OHCI_H__

#include "../../types.h"

/* stolen from drivers/usb/host/ohci.h (linux-kernel) :) */

/* OHCI CONTROL AND STATUS REGISTER MASKS */

/*
 * HcControl (control) register masks
 */
#define OHCI_CTRL_CBSR (3 << 0)        /* control/bulk service ratio */
#define OHCI_CTRL_PLE  (1 << 2)        /* periodic list enable */
#define OHCI_CTRL_IE   (1 << 3)        /* isochronous enable */
#define OHCI_CTRL_CLE  (1 << 4)        /* control list enable */
#define OHCI_CTRL_BLE  (1 << 5)        /* bulk list enable */
#define OHCI_CTRL_HCFS (3 << 6)        /* host controller functional state */
#define OHCI_CTRL_IR   (1 << 8)        /* interrupt routing */
#define OHCI_CTRL_RWC  (1 << 9)        /* remote wakeup connected */
#define OHCI_CTRL_RWE  (1 << 10)       /* remote wakeup enable */

/* pre-shifted values for HCFS */
#define OHCI_USB_RESET (0 << 6)
#define OHCI_USB_RESUME        (1 << 6)
#define OHCI_USB_OPER  (2 << 6)
#define OHCI_USB_SUSPEND       (3 << 6)

/*
 * HcCommandStatus (cmdstatus) register masks
 */
#define OHCI_HCR       (1 << 0)        /* host controller reset */
#define OHCI_CLF       (1 << 1)        /* control list filled */
#define OHCI_BLF       (1 << 2)        /* bulk list filled */
#define OHCI_OCR       (1 << 3)        /* ownership change request */
#define OHCI_SOC       (3 << 16)       /* scheduling overrun count */

/*
 * masks used with interrupt registers:
 * HcInterruptStatus (intrstatus)
 * HcInterruptEnable (intrenable)
 * HcInterruptDisable (intrdisable)
 */
#define OHCI_INTR_SO   (1 << 0)        /* scheduling overrun */
#define OHCI_INTR_WDH  (1 << 1)        /* writeback of done_head */
#define OHCI_INTR_SF   (1 << 2)        /* start frame */
#define OHCI_INTR_RD   (1 << 3)        /* resume detect */
#define OHCI_INTR_UE   (1 << 4)        /* unrecoverable error */
#define OHCI_INTR_FNO  (1 << 5)        /* frame number overflow */
#define OHCI_INTR_RHSC (1 << 6)        /* root hub status change */
#define OHCI_INTR_OC   (1 << 30)       /* ownership change */
#define OHCI_INTR_MIE  (1 << 31)       /* master interrupt enable */

/* For initializing controller (mask in an HCFS mode too) */
#define OHCI_CONTROL_INIT      (3 << 0)
#define        OHCI_INTR_INIT \
               (OHCI_INTR_MIE | OHCI_INTR_RHSC | OHCI_INTR_UE)

/* OHCI ROOT HUB REGISTER MASKS */

/* roothub.portstatus [i] bits */
#define RH_PS_CCS            0x00000001                /* current connect status */
#define RH_PS_PES            0x00000002                /* port enable status*/
#define RH_PS_PSS            0x00000004                /* port suspend status */
#define RH_PS_POCI           0x00000008                /* port over current indicator */
#define RH_PS_PRS            0x00000010                /* port reset status */
#define RH_PS_PPS            0x00000100                /* port power status */
#define RH_PS_LSDA           0x00000200                /* low speed device attached */
#define RH_PS_CSC            0x00010000                /* connect status change */
#define RH_PS_PESC           0x00020000                /* port enable status change */
#define RH_PS_PSSC           0x00040000                /* port suspend status change */
#define RH_PS_OCIC           0x00080000                /* over current indicator change */
#define RH_PS_PRSC           0x00100000                /* port reset status change */

/* roothub.status bits */
#define RH_HS_LPS           0x00000001         /* local power status */
#define RH_HS_OCI           0x00000002         /* over current indicator */
#define RH_HS_DRWE          0x00008000         /* device remote wakeup enable */
#define RH_HS_LPSC          0x00010000         /* local power status change */
#define RH_HS_OCIC          0x00020000         /* over current indicator change */
#define RH_HS_CRWE          0x80000000         /* clear remote wakeup enable */

/* roothub.b masks */
#define RH_B_DR                0x0000ffff              /* device removable flags */
#define RH_B_PPCM      0xffff0000              /* port power control mask */

/* roothub.a masks */
#define        RH_A_NDP        (0xff << 0)             /* number of downstream ports */
#define        RH_A_PSM        (1 << 8)                /* power switching mode */
#define        RH_A_NPS        (1 << 9)                /* no power switching */
#define        RH_A_DT         (1 << 10)               /* device type (mbz) */
#define        RH_A_OCPM       (1 << 11)               /* over current protection mode */
#define        RH_A_NOCP       (1 << 12)               /* no over current protection */
#define        RH_A_POTPGT     (0xff << 24)            /* power on to power good time */

struct ohci_hcca {
#define NUM_INITS 32
	u32 int_table[NUM_INITS]; /* periodic schedule */
	/*
	 * OHCI defines u16 frame_no, followed by u16 zero pad.
	 * Since some processors can't do 16 bit bus accesses,
	 * portable access must be a 32 bits wide.
	 */
	u32 frame_no;			/* current frame number */
	u32 done_head;		/* info returned for an interrupt */
	u8 reserved_for_hc [116];
	u8 what [4];		   /* spec only identifies 252 bytes :) */
} ALIGNED(256);

struct endpoint_descriptor {
	/* required by HC */
	u32 flags;
	u32 tailp;
	u32 headp;
	u32 nexted;

	/* required by software */
	u32 tdcount;
	u8 type;
} ALIGNED(16);

#define	OHCI_ENDPOINT_ADDRESS_MASK				0x0000007f
#define	OHCI_ENDPOINT_GET_DEVICE_ADDRESS(s)		((s) & 0x7f)
#define	OHCI_ENDPOINT_SET_DEVICE_ADDRESS(s)		(s)
#define	OHCI_ENDPOINT_GET_ENDPOINT_NUMBER(s)	(((s) >> 7) & 0xf)
#define	OHCI_ENDPOINT_SET_ENDPOINT_NUMBER(s)	((s) << 7)
#define	OHCI_ENDPOINT_DIRECTION_MASK			0x00001800
#define	OHCI_ENDPOINT_DIRECTION_DESCRIPTOR		0x00000000
#define	OHCI_ENDPOINT_DIRECTION_OUT				0x00000800
#define	OHCI_ENDPOINT_DIRECTION_IN				0x00001000
#define	OHCI_ENDPOINT_LOW_SPEED					0x00002000
#define	OHCI_ENDPOINT_FULL_SPEED				0x00000000
#define	OHCI_ENDPOINT_SKIP						0x00004000
#define	OHCI_ENDPOINT_GENERAL_FORMAT			0x00000000
#define	OHCI_ENDPOINT_ISOCHRONOUS_FORMAT		0x00008000
#define	OHCI_ENDPOINT_MAX_PACKET_SIZE_MASK		(0x7ff << 16)
#define	OHCI_ENDPOINT_GET_MAX_PACKET_SIZE(s)	(((s) >> 16) & 0x07ff)
#define	OHCI_ENDPOINT_SET_MAX_PACKET_SIZE(s)	((s) << 16)
#define	OHCI_ENDPOINT_HALTED					0x00000001
#define	OHCI_ENDPOINT_TOGGLE_CARRY				0x00000002
#define	OHCI_ENDPOINT_HEAD_MASK					0xfffffffc


struct general_td {
	/* required by HC */
	u32 flags;
	u32 cbp;
	u32 nexttd;
	u32 be;

	/* required by software */
	u32 bufaddr;
	u32 buflen;
	u32 pad1;
	u32 pad2;
} ALIGNED(16);

#define	OHCI_TD_BUFFER_ROUNDING			0x00040000
#define	OHCI_TD_DIRECTION_PID_MASK		0x00180000
#define	OHCI_TD_DIRECTION_PID_SETUP		0x00000000
#define	OHCI_TD_DIRECTION_PID_OUT		0x00080000
#define	OHCI_TD_DIRECTION_PID_IN		0x00100000
#define	OHCI_TD_GET_DELAY_INTERRUPT(x)	(((x) >> 21) & 7)
#define	OHCI_TD_SET_DELAY_INTERRUPT(x)	((x) << 21)
#define	OHCI_TD_INTERRUPT_MASK			0x00e00000
#define	OHCI_TD_TOGGLE_CARRY			0x00000000
#define	OHCI_TD_TOGGLE_0				0x02000000
#define	OHCI_TD_TOGGLE_1				0x03000000
#define	OHCI_TD_TOGGLE_MASK				0x03000000
#define	OHCI_TD_GET_ERROR_COUNT(x)		(((x) >> 26) & 3)
#define	OHCI_TD_GET_CONDITION_CODE(x)	((x) >> 28)
#define	OHCI_TD_SET_CONDITION_CODE(x)	((x) << 28)
#define	OHCI_TD_CONDITION_CODE_MASK		0xf0000000

#define OHCI_TD_INTERRUPT_IMMEDIATE			0x00
#define OHCI_TD_INTERRUPT_NONE				0x07

#define OHCI_TD_CONDITION_NO_ERROR			0x00
#define OHCI_TD_CONDITION_CRC_ERROR			0x01
#define OHCI_TD_CONDITION_BIT_STUFFING		0x02
#define OHCI_TD_CONDITION_TOGGLE_MISMATCH	0x03
#define OHCI_TD_CONDITION_STALL				0x04
#define OHCI_TD_CONDITION_NO_RESPONSE		0x05
#define OHCI_TD_CONDITION_PID_CHECK_FAILURE	0x06
#define OHCI_TD_CONDITION_UNEXPECTED_PID	0x07
#define OHCI_TD_CONDITION_DATA_OVERRUN		0x08
#define OHCI_TD_CONDITION_DATA_UNDERRUN		0x09
#define OHCI_TD_CONDITION_BUFFER_OVERRUN	0x0c
#define OHCI_TD_CONDITION_BUFFER_UNDERRUN	0x0d
#define OHCI_TD_CONDITION_NOT_ACCESSED		0x0f

#endif

