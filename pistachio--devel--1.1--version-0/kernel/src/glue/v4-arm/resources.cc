/*********************************************************************
 *
 * Copyright (C) 2004-2005,  National ICT Australia (NICTA)
 *
 * File path:     glue/v4-arm/resource.cc
 * Description:   ARM copy area resource
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
 * $Id: resources.cc,v 1.4 2004/06/09 09:52:50 htuch Exp $
 *
 ********************************************************************/

#include <l4.h>
#include INC_API(tcb.h)
#include INC_GLUE(resource_functions.h)
#include INC_API(space.h)
#include <debug.h>

void thread_resources_t::purge(tcb_t * tcb)
{
    UNIMPLEMENTED();
}

void thread_resources_t::init(tcb_t * tcb)
{
    ASSERT(DEBUG, tcb);

    tcb->resource_bits.init ();
}

void thread_resources_t::free(tcb_t * tcb)
{
    ASSERT(DEBUG, tcb);
    /* We have to free the thread's UTCB */
    space_t * space = tcb->get_space();

    /* Can't free UTCBs of kernel space */
    ASSERT(NORMAL, space);

    /* No UTCB had been allocated yet? */
    if (!tcb->is_activated())
	return;

    utcb_t * utcb = tcb->get_utcb();

    word_t offset, utcb_num, index, bit;

    offset = (word_t)utcb - UTCB_AREA_START -
	    (space->get_utcb_section() * ARM_SECTION_SIZE);
    utcb_num = offset >> UTCB_BITS;

    index = utcb_num / 30;
    bit = utcb_num % 30;
    /* Free this utcb */
    space->pt.pd_split.utcb_bitmap[index].x.bits &= ~(1ul<<bit);

    /* Search to see if all UTCBs in this page are now free */
    offset = (offset & (~(UTCB_AREA_PAGESIZE-1)));
    utcb_num = offset >> UTCB_BITS;

    index = utcb_num / 30;
    bit = utcb_num % 30;

    for (word_t search = offset; search < (offset + UTCB_AREA_PAGESIZE); search += UTCB_SIZE)
    {
	if (space->pt.pd_split.utcb_bitmap[index].x.bits & (1ul<<bit))
	    return;

	if (++bit == 30) {
	    bit = 0;
	    index ++;
	}
    }

    pgent_t::pgsize_e pgsize;
    pgent_t * pg;

#ifdef CONFIG_ENABLE_FASS
    space_t * mapspace = space;
#else
    space_t * mapspace = get_kernel_space();
#endif

    if (!mapspace->lookup_mapping ((addr_t) utcb, &pg, &pgsize)) {
	ASSERT(ALWAYS, 0);
	enter_kdebug("UTCB mapping not found");
    }

    addr_t kaddr = phys_to_virt(pg->address(mapspace, pgsize));

    /* No more UTCBs in page, free it */
    mapspace->remove_mapping((addr_t)utcb, UTCB_AREA_PGSIZE, false);
    kmem.free(kmem_utcb, kaddr, UTCB_AREA_PAGESIZE);
}

