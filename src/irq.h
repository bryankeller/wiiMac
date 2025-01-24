/*
	ppcskel - a Free Software replacement for the Nintendo/BroadOn IOS.
	IRQ support

Copyright (C) 2009		Bernhard Urban <lewurm@gmx.net>
Copyright (C) 2009		Sebastian Falbesoner <sebastian.falbesoner@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#ifndef __IRQ_H__
#define __IRQ_H__

#include "types.h"

#ifdef CAN_HAZ_IRQ
#define IRQ_TIMER	0
#define IRQ_NAND	1
#define IRQ_AES		2
#define IRQ_SHA1	3
#define IRQ_EHCI	4
#define IRQ_OHCI0	5
#define IRQ_OHCI1	6
#define IRQ_SDHC	7
#define IRQ_WIFI	8
#define IRQ_GPIO1B	10
#define IRQ_GPIO1	11
#define IRQ_RESET	17
#define IRQ_PPCIPC	30
#define IRQ_IPC		31

#define IRQF_TIMER	(1<<IRQ_TIMER)
#define IRQF_NAND	(1<<IRQ_NAND)
#define IRQF_AES	(1<<IRQ_AES)
#define IRQF_SDHC	(1<<IRQ_SDHC)
#define IRQF_GPIO1B	(1<<IRQ_GPIO1B)
#define IRQF_GPIO1	(1<<IRQ_GPIO1)
#define IRQF_RESET	(1<<IRQ_RESET)
#define IRQF_IPC	(1<<IRQ_IPC)
#define IRQF_OHCI0 	(1<<IRQ_OHCI0)
#define IRQF_OHCI1 	(1<<IRQ_OHCI1)

#define IRQF_ALL	( \
	IRQF_TIMER|IRQF_NAND|IRQF_GPIO1B|IRQF_GPIO1| \
	IRQF_RESET|IRQF_IPC|IRQF_AES|IRQF_SDHC| \
	IRQF_OHCI0|IRQF_OHCI1 \
	)

/* broadway.h? :o */
/* broadway processor interface registers */
#define BW_PI_IRQFLAG (0x0c003000)
#define BW_PI_IRQMASK (0x0c003004)

// http://hitmen.c02.at/files/yagcd/yagcd/chap5.html#sec5.4
#define BW_PI_IRQ_RESET 	1
#define BW_PI_IRQ_DI 		2
#define BW_PI_IRQ_SI 		3
#define BW_PI_IRQ_EXI 		4
#define BW_PI_IRQ_AI 	 	5
#define BW_PI_IRQ_DSP 	 	6
#define BW_PI_IRQ_MEM 	 	7
#define BW_PI_IRQ_VI 	 	8
#define BW_PI_IRQ_PE_TOKEN 	9
#define BW_PI_IRQ_PE_FINISH 	10
#define BW_PI_IRQ_CP 	 	11
#define BW_PI_IRQ_DEBUG 	12
#define BW_PI_IRQ_HSP 	 	13
#define BW_PI_IRQ_HW 		14 //hollywood pic

/* stolen from libogc - gc/ogc/machine/processor.h */
#define _CPU_ISR_Enable() \
	{ register u32 _val = 0; \
	  __asm__ __volatile__ ( \
		"mfmsr %0\n" \
		"ori %0,%0,0x8000\n" \
		"mtmsr %0" \
		: "=&r" ((_val)) : "0" ((_val)) \
	  ); \
	}

#define _CPU_ISR_Disable( _isr_cookie ) \
  { register u32 _disable_mask = 0; \
	_isr_cookie = 0; \
    __asm__ __volatile__ ( \
	  "mfmsr %0\n" \
	  "rlwinm %1,%0,0,17,15\n" \
	  "mtmsr %1\n" \
	  "extrwi %0,%0,1,16" \
	  : "=&r" ((_isr_cookie)), "=&r" ((_disable_mask)) \
	  : "0" ((_isr_cookie)), "1" ((_disable_mask)) \
	); \
  }

#define _CPU_ISR_Restore( _isr_cookie )  \
  { register u32 _enable_mask = 0; \
	__asm__ __volatile__ ( \
    "    cmpwi %0,0\n" \
	"    beq 1f\n" \
	"    mfmsr %1\n" \
	"    ori %1,%1,0x8000\n" \
	"    mtmsr %1\n" \
	"1:" \
	: "=r"((_isr_cookie)),"=&r" ((_enable_mask)) \
	: "0"((_isr_cookie)),"1" ((_enable_mask)) \
	); \
  }

void irq_initialize(void);
void irq_shutdown(void);

void irq_handler(void);

void irq_bw_enable(u32 irq);
void irq_bw_disable(u32 irq);
void irq_hw_enable(u32 irq);
void irq_hw_disable(u32 irq);

u32 irq_kill(void);
void irq_restore(u32 cookie);

/* TODO: port to ppc 
static inline void irq_wait(void)
{
	u32 data = 0;
	__asm__ volatile ( "mcr\tp15, 0, %0, c7, c0, 4" : : "r" (data) );
}
*/

#endif

#else
// stub functions allow us to avoid sprinkling other code with ifdefs
static inline u32 irq_kill(void) {
	return 0;
}

static inline void irq_restore(u32 cookie) {
	(void)cookie;
}
#endif


