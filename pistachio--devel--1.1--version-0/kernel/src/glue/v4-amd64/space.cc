/****************************************************************************
 *
 * Copyright (C) 2002, Karlsruhe University
 *
 * File path:	glue/v4-tmplarch/space.cc
 * Description:	Template for space_t implementation.
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
 * $Id: space.cc,v 1.9 2004/12/09 01:07:26 cvansch Exp $
 *
 ***************************************************************************/

#include <debug.h>			
#include <linear_ptab.h>
#include <generic/memregion.h>

#include INC_API(space.h)		
#include INC_API(tcb.h)		
#include INC_API(kernelinterface.h)		
#include INC_ARCH(mmu.h)
#include INC_ARCH(trapgate.h)
#include INC_GLUE(memory.h)

#define PGSIZE_KERNEL    (KERNEL_PAGE_SIZE == AMD64_2MPAGE_SIZE) ? pgent_t::size_2m : pgent_t::size_4k
#define PGSIZE_KIP       (pgent_t::size_4k)
#define PGSIZE_KTCB      (pgent_t::size_4k)
EXTERN_KMEM_GROUP(kmem_space);
DECLARE_KMEM_GROUP (kmem_tcb);
DECLARE_KMEM_GROUP (kmem_utcb);

space_t * kernel_space = NULL;
tcb_t * dummy_tcb = NULL;


/**********************************************************************
 *
 *                         Helper functions
 *
 **********************************************************************/

INLINE void align_memregion(mem_region_t & region, word_t size)
{
    region.low = addr_t((word_t)region.low & ~(size - 1));
    region.high = addr_t(((word_t)region.high + size - 1) & ~(size - 1));
}


static tcb_t * get_dummy_tcb()
{
    
    if (!dummy_tcb) 
    { 
 	dummy_tcb = (tcb_t*)kmem.alloc(kmem_tcb, PGSIZE_KTCB); 
 	ASSERT(dummy_tcb); 
 	dummy_tcb = virt_to_phys(dummy_tcb); 
    } 
    return dummy_tcb; 
}


/**********************************************************************
 *
 *                         space_t implementation
 *
 **********************************************************************/

/**
 * initialize a space
 *
 * @param utcb_area	fpage describing location of UTCB area
 * @param kip_area	fpage describing location of KIP
 */
void space_t::init (fpage_t utcb_area, fpage_t kip_area)
{
    ASSERT( (KERNEL_AREA_END - KERNEL_AREA_START) <= AMD64_PML4_SIZE);
    
    *this->pgent(page_table_index(pgent_t::size_512g, (addr_t) KERNEL_AREA_START)) = 	
	*kernel_space->pgent(page_table_index(pgent_t::size_512g, (addr_t) KERNEL_AREA_START));

    
    this->x[CONFIG_SMP_MAX_CPUS].utcb_area = utcb_area;
    this->x[CONFIG_SMP_MAX_CPUS].kip_area = kip_area;


    // map kip read-only to user
    add_mapping(kip_area.get_base(), virt_to_phys((addr_t)get_kip()), PGSIZE_KIP, false, false);

}


/**
 * Release mappings that belong to the kernel (UTCB, KIP)
 * @param vaddr		virtual address in the space
 * @param paddr		physical address the mapping refers to
 * @param log2size	log2(size of mapping)
 */
void space_t::release_kernel_mapping (addr_t vaddr, addr_t paddr,
				      word_t log2size)
{
    // Free up memory used for UTCBs
    if (get_utcb_page_area ().is_addr_in_fpage (vaddr))
      kmem.free (kmem_utcb, phys_to_virt (paddr), 1UL << log2size);

}

/**
 * establish a mapping in sigma0's space
 * @param addr	the fault address in sigma0
 *
 * This function should install a mapping that allows sigma0 to make
 * progress. Sigma0's space is available as this.
 */
void space_t::map_sigma0(addr_t addr)
{
    //TRACEF("%p\n", addr);
    add_mapping(addr, addr, pgent_t::size_2m, true, false);   
}

/**
 * Install a dummy TCB
 * @param addr	address where the dummy TCB should be installed
 *
 * The dummy TCB must be read-only and fail all validity tests.
 */
void space_t::map_dummy_tcb (addr_t addr)
{
    //TRACEF("%p\n", addr);
    //enter_kdebug("map dummy");
    add_mapping(addr, (addr_t)get_dummy_tcb(), PGSIZE_KTCB, false, false);   
}

/* 
 * Allocate a new UTCB
 * @param addr	address where the dummy TCB should be installed
 *
 * precondition: this is a valid utcb location for the space 
 */

utcb_t * space_t::allocate_utcb(tcb_t * tcb)
{

    ASSERT(tcb);
    
    addr_t utcb = (addr_t)tcb->get_utcb_location();
    
    /* walk ptab, to see if a page is already mapped */
    pgent_t::pgsize_e size = pgent_t::size_max;
    pgent_t * pgent = this->pgent(page_table_index(size, utcb));
    
    while (size > pgent_t::size_4k && pgent->is_valid(this, size))
    {
	ASSERT(pgent->is_subtree(this, size));

	pgent = pgent->subtree(this, size);
	size--;
	pgent = pgent->next(this, size, page_table_index(size, utcb));
    }

    /* if pgent is valid a page is mapped, otherwise allocate a new one */
    if (pgent->is_valid(this, size))
	return (utcb_t*)phys_to_virt(addr_offset(pgent->address(this, size),
						 (word_t)utcb & (~AMD64_4KPAGE_MASK)));
    else
    {
	// allocate new UTCB page
	addr_t page = kmem.alloc(kmem_utcb, AMD64_4KPAGE_SIZE);
	ASSERT(page);
	add_mapping((addr_t)utcb, virt_to_phys(page), pgent_t::size_4k, true, false);
	return (utcb_t*)addr_offset(page, (word_t)utcb & (~AMD64_4KPAGE_MASK));
    
    }
}
/**
 * Map memory usable for TCB
 * @param addr address of the TCB that should be made usable
 *
 * This function is called when a TCB should be made usable the first
 * time. Usually, this happens when a) no page is mapped at the TCB
 * address at all, or b) a read-only page is mapped and now a write
 * access to the TCB occured.
 *
 * @see space_t::map_dummy_tcb
 */
void space_t::allocate_tcb(addr_t addr)
{
    addr_t page = kmem.alloc( kmem_tcb, AMD64_4KPAGE_SIZE );
    ASSERT(page);
    
    kernel_space->add_mapping(addr,  virt_to_phys(page), pgent_t::size_4k, true, true, true);
    sync_kernel_space (addr);

    
}

/**
 * Translate a user accessible UTCB address to a kernel accessible one
 * @param utcb	user accessible address of UTCB
 * @returns kernel accessible address of UTCB
 *
 * The returned address must be accessible in the current address
 * space. This is required for checking values in the UTCB of a thread
 * in a different address space.
 */
utcb_t * space_t::utcb_to_kernel_space(utcb_t * utcb)
{
    UNIMPLEMENTED();
    return (utcb_t *) NULL;
}

/* JS: TODO nx bit */
void space_t::add_mapping(addr_t vaddr, addr_t paddr, pgent_t::pgsize_e size, 
			  bool writable, bool kernel, bool global, bool cacheable)
{
    pgent_t::pgsize_e curr_size = pgent_t::size_max;

    pgent_t * pgent = this->pgent(page_table_index(curr_size, vaddr));

    //TRACEF("space=%p, v=%p, p=%08p, s=%d %s %s %s %s\n", this, vaddr, paddr, (long) size, 
    //   (writable ? "w" : "ro"), (kernel  ? "k" : "u"), (global  ? "g" : "ng"), (cacheable  ? "c" : "nc")); 
	 
    /*
     * Sanity checking on page size (must be 4k or 2m)
     */
    if (!is_page_size_valid(size))
    {
	printf("Mapping invalid pagesize (%dKB)\n", page_size(size) >> 10);
	enter_kdebug("invalid page size");
	return;
    }

    /*
     * Walk down hierarchy
     */
    
    while (size < curr_size){
	
	/* Check if already mapped as larger mapping */
	if (pgent->is_valid(this, curr_size)){
	    
	    if (!pgent->is_subtree(this, curr_size))
	    {
		/* check that alignement of virtual and physical page fits */
		ASSERT(addr_mask(vaddr, ~AMD64_2MPAGE_MASK) ==
		       addr_mask(paddr,~AMD64_2MPAGE_MASK));

		if (((addr_t)pgent->address(this, curr_size)) == 
		    addr_mask(paddr, AMD64_2MPAGE_MASK)){
		    printf("2MB mapping @ %p space %p already exists.\n",  vaddr, this);
		    return;

		}

	    }
	}
	else
	{
	    pgent->make_subtree(this, curr_size, kernel);
	}
	curr_size--;
	pgent = pgent->subtree(this, curr_size + 1)->next(this, curr_size, page_table_index(curr_size, vaddr));

    }
     
    pgent->set_entry(this, size, paddr, true, writable, true, kernel);

    // default is cacheable
    if (!cacheable) pgent->set_cacheability (this, curr_size, false);
    
    // default: kernel->global, user->non-global
    if (kernel != global) pgent->set_global(this, curr_size, global);

}

void space_t::remap_area(addr_t vaddr, addr_t paddr, pgent_t::pgsize_e pgsize,
			 word_t len, bool writable, bool kernel, bool global)
{
    //TRACEF("\tremap area %p->%p len=%x, size=%d, %s, %s\n", 
    //    	       vaddr, paddr, len, pgsize,
    //       (writable ? "w" : "ro"), (kernel  ? "k" : "u"));

    word_t page_size = (pgsize == pgent_t::size_4k) ? AMD64_4KPAGE_SIZE : 
	AMD64_2MPAGE_SIZE;

    // length must be page-size aligned
    ASSERT((len & (page_size - 1)) == 0);

    for (word_t offset = 0; offset < len; offset += page_size)
	add_mapping(addr_offset(vaddr, offset), addr_offset(paddr, offset), 
		    pgsize, writable, kernel, global);
}


/**********************************************************************
 *
 *                         System initialization 
 *
 **********************************************************************/

#if (KERNEL_PAGE_SIZE != AMD64_2MPAGE_SIZE) && \
    (KERNEL_PAGE_SIZE != AMD64_4KPAGE_SIZE)
#error invalid kernel page size - please adapt
#endif

/**
 * 
 * initialize kernel mappings
 * 
 * 
 */

void SECTION(".init.memory") space_t::init_kernel_mappings()
{
    
    mem_region_t reg;
    
    /* remap kernel */
    reg.set(start_text_phys, end_text_phys);
    align_memregion(reg, KERNEL_PAGE_SIZE);

    remap_area(phys_to_virt(reg.low), reg.low, PGSIZE_KERNEL, reg.get_size(), 
	       true, true, true);


    /* map init memory */
    reg.set(start_init, end_init);
    align_memregion(reg, AMD64_2MPAGE_SIZE);
    remap_area(reg.low, reg.low, PGSIZE_KERNEL, reg.get_size(), true, true, true);

    /* map low 1MB  for initialization */
    reg.set((addr_t)0, (addr_t)0x00100000);
    align_memregion(reg, AMD64_2MPAGE_SIZE);
    remap_area(reg.low, reg.low, pgent_t::size_2m, reg.get_size(), true, true);

    /* map video mem (use 2MB mapping, don't waste a pagetable) */
    remap_area(phys_to_virt((addr_t)VIDEO_MAPPING), (addr_t)VIDEO_MAPPING,
 		pgent_t::size_2m, AMD64_2MPAGE_SIZE, true, true, true);


    /* map syscalls read-only/exectutable to user  */
    // TODO: nx bit ?
    ASSERT( ((word_t) end_syscalls -  (word_t) start_syscalls) <= KERNEL_PAGE_SIZE);
    remap_area(start_syscalls, virt_to_phys(start_syscalls), 
	       pgent_t::size_2m, AMD64_2MPAGE_SIZE, false, false, true);



    /*  
     * MYUTCB mapping
     * allocate a full page for all myutcb pointers.
     * access must be performed via gs:0, when setting up the gdt
     * each processor gets a full cache line to avoid bouncing 
     * 
     */
    
    EXTERN_KMEM_GROUP(kmem_misc);
    
    remap_area((addr_t)UTCB_MAPPING,	
 		virt_to_phys(kmem.alloc(kmem_misc, AMD64_4KPAGE_SIZE)), 
 		pgent_t::size_4k, AMD64_4KPAGE_SIZE, true, false, true);


    /*  
     * Remap 4GB physical memory (e.g. for readmem) 
     */

    for (word_t p = REMAP_32BIT_START; p < REMAP_32BIT_END; p += AMD64_2MPAGE_SIZE)
	    remap_area((addr_t) p, (addr_t) (p - REMAP_32BIT_START), pgent_t::size_2m, AMD64_2MPAGE_SIZE, true, true, true);
	
}

/**
 * initialize THE kernel space
 * @see get_kernel_space()
 */
void SECTION(".init.memory") init_kernel_space()
{
    ASSERT(!kernel_space);
    
    kernel_space = (space_t *) kmem.alloc(kmem_space, sizeof(space_t));
    kernel_space->init_kernel_mappings();

    //TRACEF("new_pml4=%p\n", (u64_t *) kernel_space->get_pml4(0));

    amd64_mmu_t::set_active_pml4((u64_t)kernel_space->get_pml4(0));
    
    ASSERT(kernel_space);

    kernel_space->enqueue_spaces();
}


/**********************************************************************
 *
 *                    global functions
 *
 **********************************************************************/

/**
 * exc_pagefault: trap gate for x86-64 pagefault handler
 */
AMD64_EXC_WITH_ERRORCODE(exc_pagefault, 0)
{

    u64_t pf = amd64_mmu_t::get_pagefault_address();

    //    TRACE("pagefault @ %p, ip=%p, sp=%p, a=%x, k=%s\n",
    //	  pf, frame.rip, frame.rsp, 
    //    	  (space_t::access_e)(frame.error & (AMD64_PF_RW | AMD64_PF_ID)),
    //  	  (frame.error & (AMD64_PF_US)) ? "false": "true");

    space_t * space = get_current_space();
    
    /* 
     * if the idle thread accesses the tcb area -
     * we will get a pagefault with an invalid space
     * so we use CR3 to figure out the space
     */
    if (EXPECT_FALSE( space == NULL )){
        space = pml4_to_space(amd64_mmu_t::get_active_pml4(),
                              get_current_cpu());
    }
    
    space->handle_pagefault(
        (addr_t)pf,
        (addr_t)frame.rip,
        (space_t::access_e)(frame.error & (AMD64_PF_RW | AMD64_PF_ID)),
        (frame.error & (AMD64_PF_US)) ? false : true);
}
