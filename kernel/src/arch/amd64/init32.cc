/*********************************************************************
 *                
 * Copyright (C) 2002, 2004-2003,  Karlsruhe University
 *                
 * File path:     arch/amd64/init32.cc
 * Description:   Switch to 64bit long mode
 *                This file is compiled as 32bit Code
 *
 * 
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
 * $Id: init32.cc,v 1.4 2004/05/31 14:01:16 stoess Exp $
 *                
 ********************************************************************/

#include <init.h>

#include INC_ARCH(cpu.h)
#include INC_ARCH(mmu.h)
#include INC_ARCH(pagebits.h)
#include INC_ARCH(segdesc.h)
#include INC_ARCH(descreg.h)
#include INC_GLUE(offsets.h)

/* ugly ... */
#define KERNEL_OFFSET_TYPED	AMD64_64_BIT_TYPED(KERNEL_OFFSET)

/* Init Pagemap - We use 2MByt Page Translation, (only :-) 3 levels */
u64_t init_pdir[512] __attribute__((aligned(4096))) SECTION(".init.data");
u64_t init_pdp[512] __attribute__((aligned(4096))) SECTION(".init.data");
u64_t init_pml4[512] __attribute__((aligned(4096))) SECTION(".init.data");

#define INIT32_PDIR_ENTRIES	256
#define INIT32_PDIR_ATTRIBS	(AMD64_PAGE_VALID | AMD64_PAGE_WRITABLE | AMD64_2MPAGE_SUPER)
#define INIT32_PTAB_ATTRIBS	(AMD64_PAGE_VALID | AMD64_PAGE_WRITABLE)

/* There we go... */
extern "C" void SECTION(SEC_INIT) startup_system();

/* Temporary GDT (INVALID, CS), temporary CS */
amd64_segdesc_t init32_gdt[2] SECTION(".init.data");
#define INIT32_CS		0x08

/* Temporary debug messaging system  */
#define INIT32_DEBUG_SCREEN	(0xb8000)
INLINE void init32_spin(int pos = 0)
{

    while(1)
    {
	((u16_t *) INIT32_DEBUG_SCREEN)[pos] += 1;
    }
}

/**
 * 
 * Initializes paged 64bit mode
 * 
 * Precondition: paged or non paged protected mode
 */

extern "C" void SECTION(".init.init32") init_paging() 
{

    /* 
     * Test if long mode is available
     * as long as no real console is available:
     * spin(1) is error, spin(0) is success  
     */
    
    
    if (!amd64_mmu_t::has_long_mode())
	init32_spin(1);


    for (int i=0; i<512; i++){
	init_pdp[i] = init_pdp[i] = init_pdir[i] = 0;
    }
    
    /* Create a pagetable hierarchy */
    
    /* ugly...
     * otherwise we override our own pdir entries or we make entries
     * outside the pdir
     */
    
    if  (AMD64_PDIR_IDX(KERNEL_OFFSET_TYPED) != 0 && 
	 (AMD64_PDIR_IDX(KERNEL_OFFSET_TYPED) < INIT32_PDIR_ENTRIES ||
	  AMD64_PDIR_IDX(KERNEL_OFFSET_TYPED) + INIT32_PDIR_ENTRIES > 512ULL))
	init32_spin(1);
    
    for (int i=0; i< INIT32_PDIR_ENTRIES; i++){
    	/* the pdir (used twice!) maps 1 GByte */
	init_pdir[i] = init_pdir[i + AMD64_PDIR_IDX(KERNEL_OFFSET_TYPED)] =
	    ((i << AMD64_2MPAGE_BITS) | (INIT32_PDIR_ATTRIBS & AMD64_2MPAGE_FLAGS_MASK));
    }

    /* 
     * the pdp maps 512 GByte
     * we use it twice for high and and low mapping, since it's only a 
     * preliminary pagetable hierarchy
     */
   
    
    init_pdp[0] = init_pdp[AMD64_PDP_IDX(KERNEL_OFFSET_TYPED) ] =\
	(u64_t) ( ( (u32_t) (init_pdir)) | INIT32_PTAB_ATTRIBS);

    
    /* 
     * the pml4 needs 2 entries: 
     * one for the first 512 GByte, one for the last 512 GByte
     */
   
    init_pml4[0] = init_pml4[AMD64_PML4_IDX(KERNEL_OFFSET_TYPED) ] =\
	(u64_t) ( ( (u32_t) (init_pdp)) | INIT32_PTAB_ATTRIBS);


    /* Disable Paging (Vol. 2, 14.6.1) */
    amd64_mmu_t::disable_paging();
	 
    /* Enable PAE mode - required before  long mode */
    amd64_mmu_t::enable_pae_mode();
     
    /* Enable long mode (not active unless paging is enabled) */
    amd64_mmu_t::enable_long_mode();
	 
    /* Set pagemap base pointer (CR3) */
    amd64_mmu_t::set_active_pml4((u64_t) ((u32_t)init_pml4));
	 
    /* Enable paged mode */
    amd64_mmu_t::enable_paging();
	 
    /* Success ?  */
    if (!(amd64_mmu_t::long_mode_active()))
	init32_spin(1);
	 
    /* Set up temporary GDT (true long mode needs 64bit Code Segment) */
    init32_gdt[0].set_seg((u32_t)0, amd64_segdesc_t::inv, 0, amd64_segdesc_t::m_comp);
    init32_gdt[1].set_seg((u32_t)0, amd64_segdesc_t::code, 0, amd64_segdesc_t::m_long);
	 
    /* Install temporary GDT */
    amd64_descreg_t::setdescreg(amd64_descreg_t::gdtr, (u64_t) (u32_t) init32_gdt, sizeof(init32_gdt));
	 
    /* 
     * Load new (64-bit) code segment, 
     * use it as data segment, too 
     * and leave this type-casting hell... 
     */

    asm("	mov  %0, %%ds		\n\t"	
	"	pushw %1		\n\t"
	"	call 1f			\n\t"
	"1:	andl %2, (%%esp)	\n\t"
	"	addl %3, (%%esp) 	\n\t"
	"	ljmp *(%%esp)		\n\t"
	: /* No Output */ 
	: 
	"r" (INIT32_CS),
	"i" (INIT32_CS),
	"i" (~(1024 -1)),
	"i" (1024)
	);
    
    init32_spin(1);
}
