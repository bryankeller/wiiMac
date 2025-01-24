/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	Requires mini.

Copyright (C) 2008		Segher Boessenkool <segher@kernel.crashing.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "bootmii_ppc.h"

// Timebase frequency is bus frequency / 4.  Ignore roundoff, this
// doesn't have to be very accurate.
#define TICKS_PER_USEC (243/4)

u64 mftb(void)
{
  u32 hi, lo, dum;
  
  asm("0: mftbu %0 ; mftb %1 ; mftbu %2 ; cmplw %0,%2 ; bne 0b" 
      : "=r"(hi), "=r"(lo), "=r"(dum)); 
  return ((u64)hi << 32) | lo;
}

static void __delay(u64 ticks)
{
	u64 start = mftb();

	while (mftb() - start < ticks)
		;
}

void udelay(u32 us)
{
	__delay(TICKS_PER_USEC * (u64)us);
}

