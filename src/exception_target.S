/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	Requires mini.
	
Copyright (C) 2008		Segher Boessenkool <segher@kernel.crashing.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#define DBAT2U          540
#define DBAT2L          541

.globl exception_start, exception_end

exception_start:
	# store all interesting regs
	mfcr 0
	stw 0,0x2080(0)
	mfxer 0
	stw 0,0x2084(0)
	mflr 0
	stw 0,0x2088(0)
	mfctr 0
	stw 0,0x208c(0)
	mfsrr0 0
	stw 0,0x2090(0)
	mfsrr1 0
	stw 0,0x2094(0)
	mfdar 0
	stw 0,0x2098(0)
	mfdsisr 0
	stw 0,0x209c(0)

	# ensure that I/O works, since this BAT could have been disabled
	# set DBAT2 for 32MB@0x0C000000,
    # real 0x0C000000 (I/O Base), WIMG=0101, R/W
    isync
    lis 0,0x0C00
    ori 0,0,0x2a
    mtspr DBAT2L,0
    lis 0,0x0C00
    ori 0,0,0x03FF
    mtspr DBAT2U,0
    isync

	# switch on FP, DR, IR
	mfmsr 0
	ori 0,0,0x2030
	mtsrr1 0

	# go to C handler
	lis 0,exception_handler@h
	ori 0,0,exception_handler@l
	mtsrr0 0
	rfi
exception_end:

