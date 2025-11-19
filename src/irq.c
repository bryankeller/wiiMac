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
	u32 bw_mask = read32(BW_PI_IRQMASK);
	u32 bw_flags = read32(BW_PI_IRQFLAG);
  u32 bw_pending = bw_flags & bw_mask;
  
  int bw_irq = 0;
  while (bw_pending) {
    if (bw_pending & 0x1) {
      printf("BW IRQ: %d\n", bw_irq);
      switch (bw_irq) {
        case BW_PI_IRQ_RESET:
          break;
          
        case BW_PI_IRQ_DI:
          break;
          
        case BW_PI_IRQ_SI:
          break;
          
        case BW_PI_IRQ_EXI:
          break;
          
        case BW_PI_IRQ_AI:
          break;
          
        case BW_PI_IRQ_DSP:
          break;
          
        case BW_PI_IRQ_MEM:
          break;
          
        case BW_PI_IRQ_VI:
          write32(0x0c002030, read32(0x0c002030) & ~(1 << 31));
          break;
          
        case BW_PI_IRQ_PE_TOKEN:
          break;
          
        case BW_PI_IRQ_PE_FINISH:
          break;
          
        case BW_PI_IRQ_CP:
          break;
          
        case BW_PI_IRQ_DEBUG:
          break;
          
        case BW_PI_IRQ_HSP:
          break;
          
        case BW_PI_IRQ_HW:
          hollywood_irq_handler();
          break;
          
        default:
          break;
      }
      
      // Ack by writing 1
      write32(BW_PI_IRQFLAG, 1 << bw_irq);
    }
    
    bw_pending = bw_pending >> 1;
    bw_irq += 1;
  }
}

void hollywood_irq_handler()
{
  u32 hw_mask = read32(HW_PPCIRQMASK);
  u32 hw_flags = read32(HW_PPCIRQFLAG);
  u32 hw_pending = hw_flags & hw_mask;
  
  int hw_irq = 0;
  while (hw_pending) {
    if (hw_pending & 0x1) {
      printf("HW IRQ: %d\n", hw_irq);
      switch (hw_irq) {
        case IRQ_TIMER:
          break;
          
        case IRQ_NAND:
          break;
          
        case IRQ_AES:
          break;
          
        case IRQ_SHA1:
          break;
          
        case IRQ_EHCI:
          break;
          
        case IRQ_OHCI0:
          hcdi_irq(OHCI0_REG_BASE);
          break;
          
        case IRQ_OHCI1:
          break;
          
        case IRQ_SDHC:
          break;
          
        case IRQ_WIFI:
          break;
          
        case IRQ_GPIO1B:
          break;
          
        case IRQ_GPIO1:
          break;
          
        case IRQ_RESET:
          break;
          
        case IRQ_PPCIPC:
          break;
          
        case IRQ_IPC:
          break;
          
        default:
          break;
      }
      
      // Ack by writing 1
      write32(HW_PPCIRQFLAG, 1 << hw_irq);
    }
    
    hw_pending = hw_pending >> 1;
    hw_irq += 1;
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
  
  // Mask and clear Starlet interrupt
  u32 hw_mask = read32(HW_ARMIRQMASK);
  hw_mask &= ~(1<<irq);
  write32(HW_ARMIRQMASK, hw_mask);
  write32(HW_ARMIRQFLAG, 1<<irq);
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
//	_CPU_ISR_Enable(); //wtf :/
}

