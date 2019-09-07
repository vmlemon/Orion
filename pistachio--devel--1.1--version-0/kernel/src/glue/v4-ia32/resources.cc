/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  Karlsruhe University
 *                
 * File path:     glue/v4-ia32/resources.cc
 * Description:   thread resource management
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
 * $Id: resources.cc,v 1.16 2004/06/02 08:41:44 sgoetz Exp $
 *                
 ********************************************************************/
#include <l4.h>
#include INC_API(tcb.h)
#include INC_ARCH(fpu.h)
#include INC_GLUE(resource_functions.h)
#include <kdb/tracepoints.h>

//#define FPU_REENABLE
static tcb_t * fpu_owner UNIT("cpulocal");

DECLARE_KMEM_GROUP (kmem_resources);

#ifdef FPU_REENABLE
DECLARE_TRACEPOINT(IA32_FPU_REENABLE);
#endif

void thread_resources_t::save(tcb_t * tcb)
{
    if (tcb->resource_bits.have_resource (FPU))
    {
	ia32_fpu_t::disable();
#ifndef FPU_REENABLE
	tcb->resource_bits -= FPU;
#endif
    }
}

void thread_resources_t::load(tcb_t * tcb)
{
#if defined(CONFIG_IA32_SMALL_SPACES)
    if (tcb->resource_bits.have_resource (IPC_PAGE_TABLE))
    {
	// We may need to reload the page table for our IPC copy
	// communication partner.
	threadid_t ptid = tcb->get_saved_partner ().is_nilthread () ?
	    tcb->get_partner () : tcb->get_saved_partner ();
	tcb_t * partner = tcb->get_space ()->get_tcb (ptid);
	u32_t new_pdir = (u32_t)
	    partner->get_space ()->get_pdir (partner->get_cpu ());

	if (ia32_mmu_t::get_active_pagetable () != new_pdir)
	    ia32_mmu_t::set_active_pagetable (new_pdir);
    }
#endif

#ifdef FPU_REENABLE
    if (tcb->resource_bits.have_resource (FPU))
    {
	ASSERT (DEBUG, fpu_owner == tcb);
	ASSERT (DEBUG, fpu_state != NULL);
	TRACEPOINT(IA32_FPU_REENABLE, 
		   printf("strictly reenabling FPU for %t\n", tcb));
	ia32_fpu_t::enable();
    }
#endif
}

void thread_resources_t::purge(tcb_t * tcb)
{
    if (fpu_owner == tcb)
    {
	ia32_fpu_t::enable();
	ia32_fpu_t::save_state(this->fpu_state);
	fpu_owner = NULL;
	ia32_fpu_t::disable();
#ifdef FPU_REENABLE
	tcb->resource_bits -= FPU;
#endif
    }
}


void thread_resources_t::init(tcb_t * tcb)
{
    tcb->resource_bits.init ();
    fpu_state = NULL;
}


void thread_resources_t::free(tcb_t * tcb)
{
    ASSERT(DEBUG, tcb);
    if (fpu_state)
    {
	kmem.free(kmem_resources, fpu_state, ia32_fpu_t::get_state_size());
	fpu_state = NULL;

	if (fpu_owner == tcb)
	{
	    fpu_owner = NULL;
	    ia32_fpu_t::disable();
	}
    }
}

void thread_resources_t::ia32_no_math_exception(tcb_t * tcb)
{
    ASSERT(NORMAL, &tcb->resources == this);
    ia32_fpu_t::enable();

    // if the current thread owns the fpu already do a quick exit
    if (fpu_owner != tcb)
    {
	if (fpu_owner != NULL)
	{
	    ia32_fpu_t::save_state(fpu_owner->resources.fpu_state);
#ifdef  FPU_REENABLE
	    fpu_owner->resource_bits -= FPU;
#endif
	}
	fpu_owner = tcb;

	if (fpu_state == NULL)
	{
	    fpu_state = kmem.alloc(kmem_resources, ia32_fpu_t::get_state_size());
	    ASSERT(ALWAYS, fpu_state);
	    ia32_fpu_t::init();
	}
	else
	    ia32_fpu_t::load_state(fpu_state);
    }

    tcb->resource_bits += FPU;
}

