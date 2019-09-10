/*********************************************************************
 *
 * Copyright (C) 2003-2004,  National ICT Australia (NICTA)
 *                
 * File path:     platform/csb337/plat.cc
 * Description:   
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
 * $Id: plat.cc,v 1.3 2004/08/21 13:31:33 cvansch Exp $
 *                
 ********************************************************************/

#include <l4.h>
#include INC_API(kernelinterface.h)
#include INC_GLUE(space.h)
#include INC_PLAT(console.h)
#include INC_PLAT(timer.h)
#include INC_ARCH(bootdesc.h)

#define rMPLLCON    (*(volatile unsigned *)0x4c000004) //MPLL Control
#define rUPLLCON    (*(volatile unsigned *)0x4c000008) //UPLL Control

/*
 * Initialize the platform specific mappings needed
 * to start the kernel.
 * Add other hardware initialization here as well
 */
extern "C" void SECTION(".init") init_platform(void)
{
    space_t *space = get_kernel_space();

    /* Map the AT91RM9200 system peripherals */
    space->add_mapping((addr_t)IO_AREA0_VADDR, (addr_t)0x50000000, pgent_t::size_4k, true, true, uncached);
    space->add_mapping((addr_t)IO_AREA1_VADDR, (addr_t)0x51000000, pgent_t::size_4k, true, true, uncached);
    space->add_mapping((addr_t)IO_AREA2_VADDR, (addr_t)0x4a000000, pgent_t::size_4k, true, true, uncached);
}

/*
 * Platform memory descriptors
 */
struct arm_bootdesc SECTION(".init.data") platform_memory[] = {
	{ 0x30000000, 0x38000000,	memdesc_t::conventional},
	{ 0x40000000, 0x60000000,	memdesc_t::dedicated },
	{ 0, 0, 0 }
};

extern "C" struct arm_bootdesc* SECTION(".init") init_platform_mem(void)
{
    return (struct arm_bootdesc*)virt_to_phys(&platform_memory);
}

extern "C" void SECTION(".init") init_cpu_mappings(void)
{
}

extern "C" void init_cpu(void)
{
    rMPLLCON = (0x7D<<12) | (1<<4) | 1; /* Main Clock -- 266Mhz */
    rUPLLCON = (0x78<<12) | (2<<4) | 3; /* USB Clock -- set to 48Mhz */
    
    arm_cache::cache_invalidate_d();
    arm_cache::tlb_flush();

    /* Program the Address Remap Register to map Internal SRAM */
    //*((volatile word_t *)(SYS_PADDR + 0xf00)) = 1;

    //  *((volatile word_t *)(SYS_PADDR + 0xf78)) = 0x1100318a;
}
