/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	Requires mini.
	
Copyright (C) 2008		Segher Boessenkool <segher@kernel.crashing.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

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

	# switch on FP, DR, IR
	mfmsr 0
	ori 0,0,0x2030
	mtsrr1 0

	# go to C handler
	lis 0,exception_dispatch@h
	ori 0,0,exception_dispatch@l
	mtsrr0 0
	rfi

exception_dispatch:
	# r3 contains the exception vector value
	bl exception_handler

	# restore CR/XER/LR/CTR
	lwz 0,0x2080(0)
	mtcr 0
	lwz 0,0x2084(0)
	mtxer 0
	lwz 0,0x2088(0)
	mtlr 0
	lwz 0,0x208c(0)
	mtctr 0

	# restore SRR0/SRR1 (resume address and MSR)
	lwz 0,0x2090(0)
	mtsrr0 0
	lwz 0,0x2094(0)
	mtsrr1 0

	# restore all GPRs (r0..r31) and return from interrupt
	lwz 0,0x2000(0)		# r0
	lwz 1,0x2004(0)		# r1
	lwz 2,0x2008(0)		# r2
	lmw 3,0x200c(0)		# r3..r31
	rfi

exception_end:
