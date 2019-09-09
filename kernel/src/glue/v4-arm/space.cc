/****************************************************************************
 *
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *
 * File path:	glue/v4-arm/space.cc
 * Description:	ARM space_t implementation.
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
 * $Id: space.cc,v 1.23 2004/12/09 01:09:42 cvansch Exp $
 *
 ***************************************************************************/

#include <l4.h>
#include <debug.h>			/* for UNIMPLEMENTED	*/
#include <linear_ptab.h>
#include INC_API(space.h)		/* space_t		*/
#include INC_API(tcb.h)
#include INC_ARCH(pgent.h)
#include INC_API(kernelinterface.h)
#include INC_GLUE(memory.h)
#include INC_GLUE(config.h)
#include INC_ARCH(fass.h)
#include INC_CPU(syscon.h)
#include INC_ARCH(special.h)

FEATURESTRING ("virtualspaceids");

DECLARE_KMEM_GROUP(kmem_utcb);
DECLARE_KMEM_GROUP(kmem_tcb);
EXTERN_KMEM_GROUP (kmem_space);

void * dummy_tcb_phys = NULL;

word_t utcb_sections_bitmap[UTCB_AREA_SECTIONS/BITS_WORD];

/* The kernel space is statically defined beause it is needed
 * before the virtual memory has been setup or the kernel
 * memory allocator.
 */
char UNIT("kspace") _kernel_space_object[sizeof(space_t)] __attribute__((aligned(ARM_L1_SIZE)));

#ifdef CONFIG_ENABLE_FASS
space_t * const cpd = (space_t *)(
	(word_t)(&_kernel_space_object) - VIRT_ADDR_BASE + PGTABLE_ADDR_BASE);
#endif

void SECTION(".init") init_kernel_space()
{
    TRACE_INIT("Initializing kernel space @ %p...\n\r", get_kernel_space());
    void * dummy;

    get_kernel_space()->init_kernel_mappings();

    dummy = (tcb_t *)kmem.alloc(kmem_tcb, KTCB_PAGESIZE);
    arm_cache::cache_flush_d_ent(dummy, KTCB_PAGESIZE);
    dummy_tcb_phys = virt_to_ram(dummy);
 
#ifdef CONFIG_ENABLE_FASS
    arm_fass.init();
#endif 
    get_kernel_space()->enqueue_spaces();

    /* Set all bits to '1' - marked as free */
    for (word_t i = 0; i < UTCB_AREA_SECTIONS/BITS_WORD; i++)
	utcb_sections_bitmap[i] = ~(0ul);
}

/**
 * initialize THE kernel space
 * @see get_kernel_space()
 */
void SECTION(".init") space_t::init_kernel_mappings()
{
    /* Kernel space's mappings already setup in initial root page table */
}

/**
 * initialize a space
 *
 * @param utcb_area	fpage describing location of UTCB area
 * @param kip_area	fpage describing location of KIP
 */
void space_t::init (fpage_t utcb_area, fpage_t kip_area)
{
    word_t i, offset = USER_AREA_SECTIONS + UTCB_AREA_SECTIONS;
    pgent_t *pg_to = pgent(offset);
    pgent_t *pg_from = get_kernel_space()->pgent(offset);

    for (i=0; i < (KTCB_AREA_SECTIONS); i++)
	*pg_to++ = *pg_from++;
    for (i=0; i < (KERNEL_AREA_SECTIONS); i++)
	*pg_to++ = *pg_from++;
    for (i=0; i < (UNCACHE_AREA_SECTIONS); i++)
	*pg_to++ = *pg_from++;

    pg_to += VAR_AREA_SECTIONS;
    pg_from += VAR_AREA_SECTIONS;

    for (i=0; i < (IO_AREA_SECTIONS); i++)
	*pg_to++ = *pg_from++;
    for (i=0; i < (MISC_AREA_SECTIONS); i++)
	*pg_to++ = *pg_from++;

    *pg_to = *pg_from;	/* high_int_vector */

#ifdef CONFIG_ENABLE_FASS
    set_domain(INVALID_DOMAIN);
#endif 

    /* Ignore user provided kip/user area */

    /* Find a section in the UTCB_AREA for this address space */
    for (word_t i = 0; i < UTCB_AREA_SECTIONS/BITS_WORD; i++) {
	/* At least one entry is available */
	if (utcb_sections_bitmap[i]) {
	    word_t bit = msb(utcb_sections_bitmap[i]);
	    /* Allocate this section */
	    utcb_sections_bitmap[i] &= ~(1ul << bit);
	    set_utcb_section(i * BITS_WORD + bit);

#ifdef CONFIG_ENABLE_FASS
	    word_t section = ( UTCB_AREA_START/ARM_SECTION_SIZE +
			    (get_utcb_section()) );
	    /* We have invalid domain, so store the space_t pointer in the
	     * section */
	    pgent_t entry;
	    entry.raw = (word_t)this;
	    cpd->pt.pdir[section] = entry;
#endif
	    return;
	}
    }
    TRACEF("No more UTCB sections\n");
    enter_kdebug("No more UTCB sections");
}

/**
 * Clean up a Space
 */
void space_t::arch_free()
{
    word_t section = get_utcb_section() / BITS_WORD;
    word_t bit = get_utcb_section() % BITS_WORD;
    utcb_sections_bitmap[section] |= (1ul << bit);
    // TRACEF("freed utcb_section %d for %p\n", get_utcb_section(), this);

    section = ( UTCB_AREA_START/ARM_SECTION_SIZE + (get_utcb_section()) );

#ifdef CONFIG_ENABLE_FASS
    pgent_t entry, *pg;
    entry.raw = 0;
    /* We need to clear the space_t pointer in the cpd section */
    cpd->pt.pdir[section] = entry;

    pg = &this->pt.pdir[section];
    if ((*pg).raw) {
	pg->remove_subtree (this, pgent_t::size_1m, true);
    }
#else
    /* We need to remove the utcb level1 mapping from the kernel page table */
    pgent_t * pg = get_kernel_space()->pgent(section);

    if ((*pg).raw) {
	pg->remove_subtree (get_kernel_space(), pgent_t::size_1m, true);
    }
#endif
}

/**
 * Allocate a UTCB
 * @param tcb   Owner of the utcb
 */
utcb_t * space_t::allocate_utcb(tcb_t * tcb) 
{
    ASSERT (DEBUG, tcb);

    word_t pos = 0;
    for (word_t i = 0; i < (MISC_AREA_SECTIONS-1); i++) {
	/* Any free UTCBs here? */
	utcb_bitmap_t bitmap = this->pt.pd_split.utcb_bitmap[i];
	if (bitmap.raw != (-4ul)) {
	    for (word_t bit = 0; bit < 30; bit++) {
		if ((bitmap.x.bits & (1ul<<bit)) == 0)
		{
		    /* Allocate this utcb position */
		    this->pt.pd_split.utcb_bitmap[i].x.bits |= (1ul<<bit);
		    pos = i * 30 + bit;
		    goto found;
		}
	    }
	    ASSERT(ALWAYS, 0);
            enter_kdebug("error in bitmap");
	}
    }
    ASSERT (DEBUG, !"no free utcbs");
    return (utcb_t *)0;

found:
    utcb_t * utcb = (utcb_t*)( UTCB_AREA_START +
		    (get_utcb_section() * ARM_SECTION_SIZE) +
		    (pos * UTCB_SIZE) );

    /* Try lookup the UTCB page for this utcb */
    word_t section = (word_t)utcb >> PAGE_BITS_1M;
#ifdef CONFIG_ENABLE_FASS
    space_t * space = this;
#else
    space_t * space = get_kernel_space();
#endif
    pgent_t level1 = *space->pgent(section);
    bool is_valid = true;

    if (level1.is_valid(space, pgent_t::size_1m))
    {
	if (level1.is_subtree(space, pgent_t::size_1m)) {
	    pgent_t leaf =
		    *level1.subtree(space, pgent_t::size_1m)->next(
					space, UTCB_AREA_PGSIZE,
					((word_t)utcb & (PAGE_SIZE_1M-1)) >> ARM_PAGE_BITS
					);
	    if (leaf.l2.fault.zero == 0)
		is_valid = false;
	} else {
	    ASSERT(ALWAYS, 0);
            enter_kdebug("1MB page in UTCB area");
	    return (utcb_t *)0;
	}
    } else {
	is_valid = false;
    }

    /* No UTCB page present for this utcb, alloc and map a new one */
    if (!is_valid) {
	addr_t page = kmem.alloc (kmem_utcb, UTCB_AREA_PAGESIZE);
	if (page == NULL)
	    return NULL;
	arm_cache::cache_flush_d_ent(page, UTCB_AREA_PAGEBITS);

	space->add_mapping((addr_t)addr_align(utcb, UTCB_AREA_PAGESIZE),
			virt_to_ram(page),
			UTCB_AREA_PGSIZE, true, false);
    }
//TRACEF("Using UTCB %ld (%p) for tcb %p  (%s)\n", pos, utcb, tcb,
//		is_valid ? "valid map" : "new map");
    tcb->set_utcb_location((word_t)utcb);
    return utcb;
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
    add_mapping(addr, addr, pgent_t::size_4k, true, false);
}

/**
 * Try to copy a mapping from kernel space into the current space
 * @param addr the address for which the mapping should be copied
 * @return true if something was copied, false otherwise.
 * Synchronization must happen at the highest level, allowing sharing.
 */
bool space_t::sync_kernel_space(addr_t addr)
{
    /* We set everything up at initialisation time */
    return false;
}

/**
 * Install a dummy TCB
 * @param addr	address where the dummy TCB should be installed
 *
 * The dummy TCB must be read-only and fail all validity tests.
 */
void space_t::map_dummy_tcb (addr_t addr)
{
    get_kernel_space()->add_mapping(addr_align(addr, KTCB_PAGESIZE),
		    dummy_tcb_phys, KTCB_PGSIZE, false, true);
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
bool space_t::allocate_tcb(addr_t addr)
{
    pgent_t *pg;
    pgent_t::pgsize_e pgsize;

    if (! get_kernel_space()->lookup_mapping(addr, &pg, &pgsize) ||
          (pgsize == KTCB_PGSIZE &&
           pg->address(get_kernel_space(), pgsize) == dummy_tcb_phys))
    {
	/* We need to remove the stale dummy page entries */
	arm_cache::cache_flush_d_ent(addr, KTCB_PAGEBITS);

	addr_t page = kmem.alloc(kmem_tcb, KTCB_PAGESIZE);
        if (page == NULL)
	    return false;

	arm_cache::cache_flush_d_ent(page, KTCB_PAGEBITS);

	get_kernel_space()->add_mapping(addr_align(addr, KTCB_PAGESIZE),
		virt_to_ram(page), KTCB_PGSIZE, true, true);

	arm_cache::tlb_flush_ent(addr, KTCB_PAGEBITS);
    }
    return true;
}

/**
 * reads a word from a given physical address, uses a remap window and
 * maps a 4MB page for the access
 *
 * @param paddr		physical address to read from
 * @return the value at the given address
 */
word_t space_t::readmem_phys (addr_t paddr)
{
    /* XXX this needs to be optimized */
    word_t data;

    space_t *space = (space_t *)read_cp15_register(C15_ttbase,
				C15_CRm_default, C15_OP2_default);
    space = phys_to_virt(space);

    arm_cache::cache_flush(); /* Only really used in kdb afaict */

    space->add_mapping((addr_t) PHYSMAPPING_VADDR,
		    (addr_t)((word_t)paddr & (~(PAGE_SIZE_1M-1))),
		    pgent_t::size_1m, false, true, uncached );

    data = *(word_t*)(PHYSMAPPING_VADDR + ((word_t)paddr & (PAGE_SIZE_1M-1)));

    space->remove_mapping((addr_t) PHYSMAPPING_VADDR, pgent_t::size_1m, true);

    return data;
}

void space_t::add_mapping(addr_t vaddr, addr_t paddr, pgent_t::pgsize_e size,
                bool writeable, bool kernel, memattrib_e attrib)
{
    pgent_t::pgsize_e pgsize = pgent_t::size_max;
    pgent_t *pg = this->pgent(page_table_index(pgsize, vaddr));

    //TRACEF("vaddr = %p paddr = %p %d\n", vaddr, paddr, size);

    /*
     * Lookup mapping
     */
    while (1) {
	if ( pgsize == size )
	    break;

	// Create subtree
	if ( !pg->is_valid( this, pgsize ) )
	    pg->make_subtree( this, pgsize, kernel );

	pg = pg->subtree( this, pgsize )->next
	    ( this, pgsize-1, page_table_index( pgsize-1, vaddr ) );
	pgsize--;
    }

    pg->set_entry(this, pgsize, paddr, true, writeable, true, kernel, attrib);
}

void space_t::remove_mapping(addr_t vaddr, pgent_t::pgsize_e size, bool kernel)
{
    pgent_t::pgsize_e pgsize = pgent_t::size_max;
    pgent_t *pg = this->pgent(page_table_index(pgsize, vaddr));

    while (pgsize > size)
    {
        if (pg->is_valid(this, pgsize))
        {
            if (!pg->is_subtree(this, pgsize))
            {
                ASSERT(ALWAYS, 0);
                enter_kdebug("larger mapping exists");
                return;
            }
        }
        else {
	    ASSERT(ALWAYS, 0);
            enter_kdebug("mapping not present");
        }

        pg = pg->subtree(this, pgsize)->next(this, pgsize-1, 
                page_table_index(pgsize-1, vaddr));

        pgsize--;
    }

    pg->clear(this, pgsize, kernel);

    flush_tlbent(this, vaddr, page_shift(size));
}

space_t * allocate_space()
{
    space_t * space = (space_t*)kmem.alloc(kmem_space, sizeof(space_t));
    if (!space)
	return NULL;
 
    /* kmem.alloc zeros out the page, in cached memory. Since we'll be using
     * this for uncached accesses, need to flush this out now.
     */
    arm_cache::cache_flush_d_ent(space, ARM_L1_SIZE_BITS);

    space->enqueue_spaces();

    return space;
}

void free_space(space_t * space)
{
    ASSERT(DEBUG, space);

#ifdef CONFIG_ENABLE_FASS
    if (space->get_domain() != INVALID_DOMAIN)
        arm_fass.set_space(space->get_domain(), NULL);
#endif
    space->dequeue_spaces();

    kmem.free(kmem_space, (addr_t)space, sizeof(space_t));
}

void space_t::flush_tlb(space_t *curspace)
{
    arm_cache::cache_flush();
    arm_cache::tlb_flush();
}

void space_t::flush_tlbent(space_t *curspace, addr_t vaddr, word_t log2size)
{
    vaddr = addr_align(vaddr, 1 << log2size);
    arm_cache::cache_flush_ent(vaddr, log2size);
    arm_cache::tlb_flush_ent(vaddr, log2size);
}
