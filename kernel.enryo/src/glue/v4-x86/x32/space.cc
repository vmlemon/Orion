/*********************************************************************
 *                
 * Copyright (C) 2002, 2004-2008, 2010,  Karlsruhe University
 *                
 * File path:     glue/v4-x86/x32/space.cc
 * Description:   address space management
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
 * $Id: space.cc,v 1.51 2006/11/18 09:51:24 stoess Exp $
 *                
 ********************************************************************/

#include <debug.h>
#include <kmemory.h>
#include <generic/lib.h>
#include <linear_ptab.h>

#include INC_API(tcb.h)
#include INC_API(smp.h)
#include INC_API(cpu.h)

#include INC_ARCH(mmu.h)
#include INC_ARCH(trapgate.h)
#include INC_ARCH(pgent.h)


#include INC_GLUE(memory.h)
#include INC_GLUE(space.h)
#include INC_API(kernelinterface.h)

EXTERN_TRACEPOINT(DEBUG);

//translation table
struct transTable_t transTable[TRANSLATION_TABLE_ENTRIES];

/**********************************************************************
 *
 *                         space_t implementation
 *
 **********************************************************************/

/**
 * reads a word from a given physical address, uses a remap window and
 * maps a 4MB page for the access
 *
 * @param paddr		physical address to read from
 * @return the value at the given address
 */
word_t space_t::readmem_phys (addr_t paddr)
{
    // get the _real_ pdir, use CR3 for that
    space_t *space = space_t::top_pdir_to_space(x86_mmu_t::get_active_pagetable());
    cpuid_t cpu = get_current_cpu();
    
#if defined(CONFIG_X86_PSE)
    // map physical 4MB page into remap window
    if (!space->data.cpu_ptab[cpu].top_pdir->readmem_area[0].is_valid() ||
	( space->data.cpu_ptab[cpu].top_pdir->readmem_area[0].get_address(x86_pgent_t::size_4m) != 
	  addr_mask(paddr, X86_SUPERPAGE_MASK) ))
    {
	space->data.cpu_ptab[cpu].top_pdir->readmem_area[0].set_entry(addr_mask(paddr, X86_SUPERPAGE_MASK),
					    x86_pgent_t::size_4m, 
					    X86_PAGE_KERNEL | X86_PAGE_VALID);

	// kill potentially stale TLB entry in remap-window
	x86_mmu_t::flush_tlbent (MEMREAD_AREA_START);
#if 0
	printf("readmem_phys %p cpu %p: pdir %p mapped %p @ %p\n", 
	       paddr, space->data.cpu_ptab[cpu].top_pdir, space->data.cpu_ptab[cpu].top_pdir->readmem_area[0].get_raw(), 
	       &space->data.cpu_ptab[cpu].top_pdir->readmem_area);
	
#endif
	
    }
#else /* !CONFIG_X86_PSE */

    pgent_t* pgent = space->pgent( page_table_index(pgent_t::size_max, (addr_t)MEMREAD_AREA_START));

    if (!pgent->is_valid(space, pgent_t::size_max))
	pgent->make_subtree(space, pgent_t::size_max, true);

    pgent = pgent->subtree(space, pgent_t::size_max)->next(
	space, pgent_t::size_4k, page_table_index(pgent_t::size_4k, paddr));

    pgent->set_entry(space, pgent_t::size_4k, paddr, 1, 0, true);
    
    // kill potentially stale TLB entry in remap-window
    x86_mmu_t::flush_tlbent(
	(word_t)addr_offset(addr_mask(paddr, page_mask (pgent_t::size_4m)),
	    MEMREAD_AREA_START));

#endif /* !CONFIG_X86_PSE */
    return *(word_t*)addr_offset(addr_mask(paddr,~X86_SUPERPAGE_MASK), 
				 MEMREAD_AREA_START);

}


/**
 * ACPI memory handling
 */
addr_t acpi_remap(addr_t addr)
{

   
    /* 
     * For now, make sure ACPI mappings are 4M; readmem_phys will map 4M-pages
     * at MEMREAD_AREA_START by directly writing the pagedir entry -- in which
     * case the pagetable created for 4K acpi mappings would be stale.
     */
    ASSERT(ACPI_PGENTSZ == pgent_t::size_4m);
    ASSERT(MEMREAD_AREA_SIZE >= ACPI_PGENTSZ * 2);
    
    addr_t vaddr = (addr_t) MEMREAD_AREA_START;
    addr_t paddr = addr;
    
    get_kernel_space()->add_mapping(
        addr_mask (vaddr, ~page_mask (ACPI_PGENTSZ)),
	addr_mask (paddr, ~page_mask (ACPI_PGENTSZ)),
	ACPI_PGENTSZ, true, true, false, false);

    vaddr = addr_offset(vaddr, page_size(ACPI_PGENTSZ));
    paddr = addr_offset(paddr, page_size(ACPI_PGENTSZ));
    
    get_kernel_space()->add_mapping( 
	addr_mask (vaddr, ~page_mask (ACPI_PGENTSZ)),
	addr_mask (paddr, ~page_mask (ACPI_PGENTSZ)),
	ACPI_PGENTSZ, true, true, false, false);
    
    x86_mmu_t::flush_tlb();
    
        
    return addr_offset(addr_mask(addr, page_mask (ACPI_PGENTSZ)), 
			MEMREAD_AREA_START);
    
}

void acpi_unmap(addr_t addr)
{
    /* empty right now */
}

#if defined(CONFIG_SMP)
void pgent_t::smp_sync(space_t * space, pgsize_e pgsize)
{
    if (pgsize != size_4m) return;
    
    for (cpuid_t cpu = 0; cpu < cpu_t::count; cpu++)
        if (cpu != space->data.reference_ptab && space->data.cpu_ptab[cpu].top_pdir)
        {
            //TRACEF("smp sync %d / %x -> %d / %x\n",
	    //  space->data.reference_ptab, space->pgent(idx(), space->data.reference_ptab),
	    //  cpu, space->pgent(idx(), cpu));
            *space->pgent(idx(), cpu) = *space->pgent(idx());
        }
}

word_t pgent_t::smp_reference_bits(space_t * space, pgsize_e pgsize, addr_t vaddr)
{
    ASSERT(pgsize == size_4m);
    
    word_t rwx = 0;
    
    for (cpuid_t cpu = 0; cpu < cpu_t::count; cpu++)
        if (space->data.cpu_ptab[cpu].top_pdir)
        {
            //TRACEF("smp refbits %d / %x\n",  cpu, space->pgent(idx(), cpu)->raw);
	    if (space->pgent(idx(), cpu)->pgent.is_accessed ()) { rwx |= 5; }
	    if (space->pgent(idx(), cpu)->pgent.is_dirty ()) { rwx |= 6; }
        }
    
    return rwx;

    
}


#endif

/**********************************************************************
 *
 *                    Small address spaces
 *
 **********************************************************************/

#if defined(CONFIG_X86_SMALL_SPACES)

word_t space_t::space_control (word_t ctrl, fpage_t kip_area, fpage_t utcb_area, threadid_t redirector_tid)
{
    // Ignore parameter if 's' bit is not set.
    if ((ctrl & (1 << 31)) == 0)
	return 0;

    word_t old_control = smallid ()->get_raw ();

    smallspace_id_t id;
    id.set_raw (ctrl);

    if (make_small (id))
	// Set 'e' bit if small space operation was successful.
	old_control |= (1 << 31);

    return old_control;
}

bool is_smallspace(space_t *space)
{
    return space->is_small();
}

#else /* !CONFIG_X86_SMALL_SPACES */

word_t space_t::space_control (word_t ctrl, fpage_t kip_area, fpage_t utcb_area, threadid_t redirector_tid)
{
    word_t oldctrl = 0;
    u64_t physaddr;
    int i;

#if defined(CONFIG_X_X86_HVM)
    // Check if 'v' bit is set.
    if (ctrl & (1 << 30))
    {
    if (get_hvm_space ()->activate (this))
        return (1 << 30);
    }
#endif

    if ((ctrl & (1 << 29)) && (sizeof(paddr_t) != sizeof(u32_t))) {
    	for (i = 0; i < TRANSLATION_TABLE_ENTRIES; ++i) {
    		if (transTable[i].size > 0)
    			continue;
        	oldctrl |= 1 << 29;
        	//printf("UTCB: 0x%x, redirect: 0x%x, KIP: 0x%x\n", utcb_area.raw, redirector_tid.get_raw(), kip_area.raw);
        	physaddr = redirector_tid.get_raw();
        	physaddr <<= 32;
        	physaddr |= utcb_area.raw;
        	transTable[i].physaddr = (paddr_t)physaddr;
        	transTable[i].s0addr = kip_area.mem.x.base << 10;
        	transTable[i].size = 1UL << kip_area.mem.x.size;
        	break;
    	}
    }
    return oldctrl;
}

#endif

paddr_t space_t::sigma0_translate(addr_t addr, pgent_t::pgsize_e size)
{
    word_t i;
    paddr_t paddr = (paddr_t)addr;
    for (i = 0; i < TRANSLATION_TABLE_ENTRIES; ++i) {
    	if ((transTable[i].size > 0) && ((word_t)addr >= transTable[i].s0addr) && 
            ((word_t)addr <= transTable[i].s0addr + transTable[i].size - 1)) {
            paddr = (paddr_t) ((u64_t) transTable[i].physaddr + (word_t) addr_offset(addr, - transTable[i].s0addr));
            transTable[i].size = 0;
            break;
    	}
    }
    return paddr;
}



