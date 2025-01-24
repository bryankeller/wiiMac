/*
	BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
	Requires mini.

Copyright (C) 2009			Andre Heider "dhewg" <dhewg@wiibrew.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

OUTPUT_FORMAT("elf32-powerpc")
OUTPUT_ARCH(powerpc:common)

ENTRY(_realmode_vector)

PHDRS {
	realmode	PT_LOAD FLAGS(5);
	app			PT_LOAD FLAGS(7);
}

SECTIONS {
	. = 0x00003400;

	.realmode : { *(.realmode) } :realmode = 0

	. = 0x80004000;

	.start : AT(ADDR(.start) & 0x3fffffff) { crt0.o(*) } :app = 0

	.text : { *(.text) *(.text.*) . = ALIGN(32); }

	.data : { *(.data) *(.data.*) . = ALIGN(32); }
	.sdata : { *(.sdata) *(.sdata.*) . = ALIGN(32); }
	.rodata : { *(.rodata) *(.rodata.*) . = ALIGN(32); }
	.stack : {
		_stack_top = .;
		. += 32768;
		_stack_bot = .;
	}

	. = ALIGN(32);

	__bss_start = .;
	.bss : { *(.bss) }
	.sbss : { *(.sbss) }
	__bss_end = .;

	. = ALIGN(0x10000);

	_sbrk_start = .;
	_sbrk_end = 0x816ffff0;
}

