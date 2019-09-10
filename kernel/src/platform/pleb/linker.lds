/*********************************************************************
 *
 * Copyright (C) 2003-2004,  National ICT Australia (NICTA)
 *
 * File path:     platform/pleb/linker.lds
 * Description:   PLEB Linker Script
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: linker.lds,v 1.9 2004/08/21 12:24:27 cvansch Exp $
 *
 ********************************************************************/

#include <l4.h>
#include INC_PLAT(offsets.h)

ENTRY(_start)
_start_text		= VIRT_ADDR_BASE;

SECTIONS
{
	/* ROM Sections start here */
	_start_rom = _start_text;
	.text _start_text : AT (ADDR(.text) - KERNEL_ROM_OFFSET)
	{
		. = ALIGN(64K);
		*(.base)
		*(.text.startup)
		*(.text)
		*(.gnu.linkonce.*)
	}
	. = ALIGN(4);
	_end_text = .;

	.rodata . : AT (ADDR(.rodata) - KERNEL_ROM_OFFSET)
	{
		*(.rodata)
		*(.rodata.*)
	}
	_end_rom = ALIGN(4);

#ifdef CONFIG_TEXT_IN_ROM
	/* RAM Sections start here */
	. = VIRT_ADDR_RAM;
	. = ALIGN(4k);
	_start_ram = .;
#else
	. = ALIGN(4k);
	_start_ram = _start_rom;
#endif

	.kip .  : AT (ADDR(.kip) - KERNEL_RAM_OFFSET)
	{
#include INC_API(kip.ldi)
		. = ALIGN(2K);
		*(.user.*)
		. = ALIGN(32);
		*(.user_thumb.*)
	}
	. = ALIGN(4K);
	_end_kip = .;

	_memory_descriptors_offset = memory_descriptors - kip;
	_memory_descriptors_size =
		(2K - ((memory_descriptors - kip) & (4K-1))) / 16;

	. = ALIGN(32K);
	_start_data = .;
	.data . :  AT (ADDR(.data) - KERNEL_RAM_OFFSET)
	{
		*(.data.kspace)
		*(.data.traps)
		*(.sdata)
		*(.data)
		*(.data.*)
		*(.bss)
	}

	.kdebug . : AT(ADDR(.kdebug) - KERNEL_RAM_OFFSET)
	{
		*(.kdebug)
		*(.kdebug-bss)
	}

	.sets . : AT(ADDR(.sets) - KERNEL_RAM_OFFSET)
	{
		. = ALIGN(16);
		_start_setlist = .;
		*(.setlist)
		_end_setlist = .;
		. = ALIGN(16);
		_start_sets = .;
		*(SORT(set_*))
		_end_sets = .;
	}
	_end_ram = ALIGN(4);

	. = ALIGN(4K);

	_start_init = .;
	.init . : AT(ADDR(.init) - KERNEL_RAM_OFFSET)
	{
                *(.init.head)
		*(.init)
		*(.init.*)
		*(.roinit)
		*(.init.data)

		. = ALIGN(32);
		_bootstack_bottom = .;
		. = . + 8K;
		_bootstack_top = .;
		_bootstack_top_phys = . - KERNEL_RAM_OFFSET;
	}
	_end_init = ALIGN(4);
	. = ALIGN(4K);

	/* special section that is discarded during linking
	   all unwanted sections should go here */

	/* Required for user code! */
	_end = ALIGN(4K);

	/DISCARD/ :
	{
/*		*(*)            <-- THIS CAUSES SEGFAULT!!! */
		*(.eh_frame)
		*(.note)
		*(.comment)
	}
}
