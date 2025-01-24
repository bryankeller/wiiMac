/*
	ppcskel - a Free Software replacement for the Nintendo/BroadOn IOS.
	IRQ support

Copyright (C) 2009		Bernhard Urban <lewurm@gmx.net>
Copyright (C) 2009		Sebastian Falbesoner <sebastian.falbesoner@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "irq.h"
#include "hollywood.h"
#include "ipc.h"
#include "bootmii_ppc.h"
#include "usb/host/host.h"
#include "mini_ipc.h"

void show_frame_no(void);

void irq_initialize(void)
{
	// clear flipper-pic (processor interface)
	write32(BW_PI_IRQMASK, 0);
	write32(BW_PI_IRQFLAG, 0xffffffff);

	// clear hollywood-pic
	write32(HW_PPCIRQMASK, 0);
	write32(HW_PPCIRQFLAG, 0xffffffff);

	/* ??? -- needed?!
	 * in mini they do
	 *
	 * write32(HW_ARMIRQMASK+0x04, 0);
	 * write32(HW_ARMIRQMASK+0x20, 0);
	 *
	 *
	 * may it's here following; on the other 
	 * hand it's already done by mini...
	 *
	 * write32(HW_PPCIRQMASK+0x04+0x08, 0);
	 * write32(HW_PPCIRQMASK+0x20+0x08, 0);
	 */

	_CPU_ISR_Enable()
}

void irq_shutdown(void)
{
	write32(HW_PPCIRQMASK, 0);
	write32(HW_PPCIRQFLAG, 0xffffffff);
	(void) irq_kill();
}

void irq_handler(void)
{
	u32 enabled = read32(BW_PI_IRQMASK);
	u32 flags = read32(BW_PI_IRQFLAG);

	flags = flags & enabled;

	if (flags & (1<<BW_PI_IRQ_RESET)) { 
		write32(BW_PI_IRQFLAG, 1<<BW_PI_IRQ_RESET);
		boot2_run(1,2); //sysmenu
	}

	if (flags & (1<<BW_PI_IRQ_HW)) { //HW-PIC IRQ
		u32 hw_enabled = read32(HW_PPCIRQMASK);
		u32 hw_flags = read32(HW_PPCIRQFLAG);

		//printf("In IRQ handler: 0x%08x 0x%08x 0x%08x\n", hw_enabled, hw_flags, hw_flags & hw_enabled);

		hw_flags = hw_flags & hw_enabled;

		if(hw_flags & IRQF_TIMER) {
			write32(HW_PPCIRQFLAG, IRQF_TIMER);
		}
		if(hw_flags & IRQF_NAND) {
			//		printf("IRQ: NAND\n");
			// hmmm... should be done by mini?
			write32(NAND_CMD, 0x7fffffff); // shut it up
			write32(HW_PPCIRQFLAG, IRQF_NAND);
			//nand_irq();
		}
		if(hw_flags & IRQF_GPIO1B) {
			//		printf("IRQ: GPIO1B\n");
			// hmmm... should be done by mini?
			write32(HW_GPIO1BINTFLAG, 0xFFFFFF); // shut it up
			write32(HW_PPCIRQFLAG, IRQF_GPIO1B);
		}
		if(hw_flags & IRQF_GPIO1) {
			//		printf("IRQ: GPIO1\n");
			// hmmm... should be done by mini?
			write32(HW_GPIO1INTFLAG, 0xFFFFFF); // shut it up
			write32(HW_PPCIRQFLAG, IRQF_GPIO1);
		}
		if(hw_flags & IRQF_RESET) {
			//		printf("IRQ: RESET\n");
			write32(HW_PPCIRQFLAG, IRQF_RESET);
		}
		if(hw_flags & IRQF_IPC) {
			//printf("IRQ: IPC\n");
			//not necessary here?
			//ipc_irq();
			write32(HW_PPCIRQFLAG, IRQF_IPC);
		}
		if(hw_flags & IRQF_AES) {
			//		printf("IRQ: AES\n");
			write32(HW_PPCIRQFLAG, IRQF_AES);
		}
		if (hw_flags & IRQF_SDHC) {
			//		printf("IRQ: SDHC\n");
			write32(HW_PPCIRQFLAG, IRQF_SDHC);
			//sdhc_irq();
		}
		if (hw_flags & IRQF_OHCI0) {
			hcdi_irq(OHCI0_REG_BASE);
			write32(HW_PPCIRQFLAG, IRQF_OHCI0);
		}
		if (hw_flags & IRQF_OHCI1) {
			hcdi_irq(OHCI1_REG_BASE);
			write32(HW_PPCIRQFLAG, IRQF_OHCI1);
		}

		hw_flags &= ~IRQF_ALL;
		if(hw_flags) {
			printf("IRQ: unknown 0x%08x\n", hw_flags);
			write32(HW_PPCIRQFLAG, hw_flags);
		}

		// not necessary here, but "cleaner"?
		write32(BW_PI_IRQFLAG, 1<<BW_PI_IRQ_HW);
	}
}

void irq_bw_enable(u32 irq)
{
	set32(BW_PI_IRQMASK, 1<<irq);
}

void irq_bw_disable(u32 irq) {
	clear32(BW_PI_IRQMASK, 1<<irq);
}

void irq_hw_enable(u32 irq)
{
	set32(HW_PPCIRQMASK, 1<<irq);
}

void irq_hw_disable(u32 irq)
{
	clear32(HW_PPCIRQMASK, 1<<irq);
}

u32 irq_kill() {
	u32 cookie;
	_CPU_ISR_Disable(cookie);
	return cookie;
}

void irq_restore(u32 cookie) {
	_CPU_ISR_Restore(cookie);
	_CPU_ISR_Enable(); //wtf :/
}

