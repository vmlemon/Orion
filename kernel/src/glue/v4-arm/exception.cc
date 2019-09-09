/*********************************************************************
 *
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *
 * File path:     glue/v4-arm/exception.cc
 * Description:   ARM syscall and page fault handlers
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
 * $Id: exception.cc,v 1.33 2004/12/09 01:13:11 cvansch Exp $
 *
 ********************************************************************/

#include <l4.h>
#include <debug.h>
#include <linear_ptab.h>
#include INC_API(tcb.h)
#include INC_API(syscalls.h)
#include INC_API(kernelinterface.h)
#include INC_ARCH(thread.h)
#include INC_GLUE(exception.h)
#include INC_GLUE(space.h)
#include INC_PLAT(console.h)
#include INC_GLUE(syscalls.h)
#include INC_GLUE(intctrl.h)
#include INC_CPU(cpu.h)
#include INC_PLAT(timer.h)
#include INC_API(schedule.h)
#include <kdb/tracepoints.h>
#ifdef CONFIG_DEBUG
#include <kdb/console.h>
#endif

DECLARE_TRACEPOINT(ARM_PAGE_FAULT);
DECLARE_TRACEPOINT(EXCEPTION_IPC_SYSCALL);
DECLARE_TRACEPOINT(EXCEPTION_IPC_GENERAL);

#if defined(CONFIG_PERF)
static unsigned long pmu_config;
#endif

INLINE void halt_user_thread( void )
{
    tcb_t *current = get_current_tcb();

    current->set_state( thread_state_t::halted );
    current->switch_to_idle();
}

/* As on MIPS */

#define SYSCALL_SAVED_REGISTERS (EXCEPT_IPC_SYS_MR_NUM+1)

static bool send_exception_ipc( word_t exc_no, word_t exc_code, arm_irq_context_t *context );

static bool send_syscall_ipc(arm_irq_context_t *context)
{
    tcb_t *current = get_current_tcb();

    //# HACK
    if (current->get_scheduler().is_nilthread()) {
        printf( "Unable to deliver user exception: no exception handler.\n" );
        return false;
    }

    TRACEPOINT (EXCEPTION_IPC_SYSCALL,
		printf ("EXCEPTION_IPC_SYSCALL: (%p) IP = %p\n",
			current, (word_t)current->get_user_ip()));

    msg_tag_t tag;

    current->set_saved_partner(current->get_partner());
    current->set_saved_state(current->get_state());
 
    // Create the message tag.
    tag.set(0, EXCEPT_IPC_SYS_MR_NUM, EXCEPT_IPC_SYS_LABEL, true, true);
    current->set_tag(tag);

    // Create the message.
    current->set_mr(EXCEPT_IPC_SYS_MR_PC, (word_t)current->get_user_ip()); 
    current->set_mr(EXCEPT_IPC_SYS_MR_R0, context->r0);
    current->set_mr(EXCEPT_IPC_SYS_MR_R1, context->r1);
    current->set_mr(EXCEPT_IPC_SYS_MR_R2, context->r2);
    current->set_mr(EXCEPT_IPC_SYS_MR_R3, context->r3);
    current->set_mr(EXCEPT_IPC_SYS_MR_R4, context->r4);
    current->set_mr(EXCEPT_IPC_SYS_MR_R5, context->r5);
    current->set_mr(EXCEPT_IPC_SYS_MR_R6, context->r6);
    current->set_mr(EXCEPT_IPC_SYS_MR_R7, context->r7);
    current->set_mr(EXCEPT_IPC_SYS_MR_SP, (word_t)current->get_user_sp());
    current->set_mr(EXCEPT_IPC_SYS_MR_LR, context->lr);
    current->set_mr(EXCEPT_IPC_SYS_MR_SYSCALL, *(word_t *)((word_t)current->get_user_ip()-4));
    current->set_mr(EXCEPT_IPC_SYS_MR_FLAGS, current->get_user_flags());
 
    // Deliver the exception IPC.
    //tag = current->do_ipc(current->get_exception_handler(),
    //        current->get_exception_handler());
    tag = current->do_ipc(current->get_scheduler(),
            current->get_scheduler());

    // Alter the user context if necessary.
    if (!tag.is_error()) {
        current->set_user_ip((addr_t)current->get_mr(EXCEPT_IPC_SYS_MR_PC));
        current->set_user_sp((addr_t)current->get_mr(EXCEPT_IPC_SYS_MR_SP));
        current->set_user_flags(current->get_mr(EXCEPT_IPC_SYS_MR_FLAGS));

	context->r0 = current->get_mr(EXCEPT_IPC_SYS_MR_R0);
	context->r1 = current->get_mr(EXCEPT_IPC_SYS_MR_R1);
	context->r2 = current->get_mr(EXCEPT_IPC_SYS_MR_R2);
	context->r3 = current->get_mr(EXCEPT_IPC_SYS_MR_R3);
	context->r4 = current->get_mr(EXCEPT_IPC_SYS_MR_R4);
	context->r5 = current->get_mr(EXCEPT_IPC_SYS_MR_R5);
	context->r6 = current->get_mr(EXCEPT_IPC_SYS_MR_R6);
	context->r7 = current->get_mr(EXCEPT_IPC_SYS_MR_R7);
	context->lr = current->get_mr(EXCEPT_IPC_SYS_MR_LR);
    } else {
        TRACEF("Unable to deliver user exception: IPC error.\n");
    }

    // Clean-up.
    current->set_partner(current->get_saved_partner());
    current->set_saved_partner(NILTHREAD);
    current->set_state(current->get_saved_state());
    current->set_saved_state(thread_state_t::aborted);
    
    return !tag.is_error();
}

extern "C" void syscall_exception(arm_irq_context_t *context)
{  
    if (!send_syscall_ipc(context)) {
        printf(TXT_BRIGHT "--- KD# %s ---\n" TXT_NORMAL, 
                "Unhandled User SYSCALL");  

	if( EXPECT_FALSE(get_kip()->kdebug_entry != NULL) )
	    get_kip()->kdebug_entry(context);

	halt_user_thread();
    }
}

extern "C" void undefined_exception(arm_irq_context_t *context)
{  
	word_t instr;
#ifdef CONFIG_ARM_THUMB_SUPPORT
    if (context->cpsr & CPSR_THUMB_BIT) {
	instr = *(u16_t *)((word_t)get_current_tcb()->get_user_ip());
    } else
#endif
	instr = *(word_t *)((word_t)get_current_tcb()->get_user_ip());

    if (!send_exception_ipc(1, instr, context)) {
	printf(TXT_BRIGHT "--- KD# %s ---\n" TXT_NORMAL, 
		    "Unhandled User Undefined Instruction");  

	if( EXPECT_FALSE(get_kip()->kdebug_entry != NULL) )
	    get_kip()->kdebug_entry(context);

	halt_user_thread();
    }
}

extern "C" void reset_exception(arm_irq_context_t *context)
{  
    printf(TXT_BRIGHT "--- KD# %s ---\n" TXT_NORMAL, 
		"Unhandled Reset Exception");  

    if( EXPECT_FALSE(get_kip()->kdebug_entry != NULL) )
	get_kip()->kdebug_entry(context);

    halt_user_thread();
}

extern "C" void fiq_exception(arm_irq_context_t *context)
{  
    printf(TXT_BRIGHT "--- KD# %s ---\n" TXT_NORMAL, 
		"FIQ Exception");  

    if( EXPECT_FALSE(get_kip()->kdebug_entry != NULL) )
	get_kip()->kdebug_entry(context);

    halt_user_thread();
}

#ifdef CONFIG_DEBUG
static void strcpy(char * dest, const char * src)
{
    while(*src)
	*dest++ = *src++;
    *dest = '\0';
}
#endif

extern "C" void arm_misc_l4_syscall(arm_irq_context_t *context)
{
    switch (context->r12 & 0xff) {
    case L4_TRAP_KIP: 
	{
	    space_t *space = get_current_space();

	    context->r0 = (word_t)space->get_kip_page_area().get_base();
	    context->r1 = get_kip()->api_version;
	    context->r2 = get_kip()->api_flags;
	    context->r3 = get_kip()->kernel_desc_ptr ?
		    *(word_t *)((word_t)get_kip() + get_kip()->kernel_desc_ptr)
		    : 0;

	    return;
	}
#ifdef CONFIG_DEBUG
    case L4_TRAP_KPUTC:
	putc((char)context->r0);
	return;
    case L4_TRAP_KGETC:
	context->r0 = (s32_t)(s8_t)getc(true);
	return;
    case L4_TRAP_KGETC_NB:
	context->r0 = (s32_t)(s8_t)getc(false);
	return;
    case L4_TRAP_KSET_THRD_NAME: 
	{
	    space_t * dummy = NULL;
	    threadid_t tid;
	    u32_t *name;

	    tid.set_raw (context->r0);

	    if (! dummy->get_tcb(tid)->is_activated())
		    return;

	    name = (u32_t*) (dummy->get_tcb(tid)->debug_name);
	    name[0] = context->r1;
	    name[1] = context->r2;
	    name[2] = context->r3;
	    name[3] = context->r4;
	    dummy->get_tcb(tid)->debug_name[15] = '\0';
	    return;
	}
    case L4_TRAP_KDEBUG:
	{
	    space_t *space = get_current_space();
	    char *string = (char *)context->r0;
	    char c;

	    if (space == NULL)
		space = get_kernel_space();

	    printf( TXT_BRIGHT "--- KD# " );
	    word_t user = ((context->cpsr & CPSR_MODE_MASK)
			    == CPSR_USER_MODE);
	    if (user)
		printf( "User: " );

	    while (readmem(space, string, &c) && (c != 0))
	    {
		putc(c);
		string++;
	    }
	    if (c != 0)
		printf("[not mapped]");

	    printf( " ---\n" TXT_NORMAL );
	    kdebug_entry(context);
	    return;
	}
#if defined(CONFIG_PERF) && defined(CONFIG_CPU_ARM_XSCALE)
    case L4_TRAP_PMU_RESET:
	    {
		    unsigned long cfg = pmu_config | 7; /* Reset and enable counters */
		    __asm__  __volatile__ (
					   "   mcr	    p14, 0, %0, c0, c0, 0   \n"
					   :  /* no output */
					   : "r" (cfg)
					   : /* no trash */
					   );
		    return;
	    }
    case L4_TRAP_PMU_STOP:
	    {
		    __asm__  __volatile__ (
					   "   mcr	    p14, 0, %0, c0, c0, 0   \n"
					   :  /* no output */
					   : "r" (pmu_config)
					   : /* no trash */
					   );
		    return;
	    }
    case L4_TRAP_PMU_READ:
	    {
		    unsigned long count = 0;
		    switch(context->r0) {
		    case 0:
			    /* Return cycle counter */
			    __asm__  __volatile__ (
						   "   mrc	    p14, 0, %0, c1, c0, 0   \n"
						   :  "=r" (count)
						   : /* no input */
						   : /* no trash */
						   );
			    break;
		    case 1:
			    /* Return cycle counter */
			    __asm__  __volatile__ (
						   "   mrc	    p14, 0, %0, c2, c0, 0   \n"
						   :  "=r" (count)
						   : /* no input */
						   : /* no trash */
						   );
			    break;
		    case 2:
			    /* Return cycle counter */
			    __asm__  __volatile__ (
						   "   mrc	    p14, 0, %0, c3, c0, 0   \n"
						   :  "=r" (count)
						   : /* no input */
						   : /* no trash */
						   );
			    break;
		    }
		    context->r0 = count;
		    return;
	    }
    case L4_TRAP_PMU_CONFIG:
	    {
		    pmu_config = context->r0;
		    return;
	    }
#endif
#if defined(CONFIG_TRACEPOINTS)
    case L4_TRAP_GETCOUNTER:
	    {
		    context->r0 = tp_list.get(context->r0)->counter[0];
		    return;
	    }
    case L4_TRAP_GETNUMTPS:
	    {
		    context->r0 = tp_list.size();
		    return;
	    }
    case L4_TRAP_GETTPNAME:
	    {
		    strcpy((char*) &get_current_tcb()->get_utcb()->mr[0], 
			   (char*) trace_names[tp_list.get(context->r0)->id]);
		    return;
	    }
    case L4_TRAP_TCCTRL:
	    {
		    if (context->r0 == 0) {
			    /*stop*/
			    tracepoint_count = false;
		    } else {
			    /*reset and go */
			    tracepoint_t * tp;
    
			    tracepoint_count = true;
			    tp_list.reset();
			    while ((tp = tp_list.next ()) != NULL)
				    tp->reset_counter();

		    }
		    return;
	    }
#endif
#endif
    default:
	break;
    }

    /* XXX - should deliver this as a exception IPC */
    printf("Illegal misc syscall\n");
    halt_user_thread();
}

/*
 * ARM page fault handler
 *
 * This function handles ARM pagefaults including domain faults,
 * data and prefetch aborts and alignment faults.
 */

extern "C" void arm_page_fault(word_t fault_status, addr_t fault_addr,
        arm_irq_context_t *context, word_t data_abt)
{
    /* Note, fs = 0 for execute fault */
    word_t fs = fault_status & 0xf;

    TRACEPOINT(ARM_PAGE_FAULT,
	printf("pf @ %p, pc = %p, tcb = %p, fs = %x\n", fault_addr, 
	    (addr_t)context->pc, get_current_tcb(), fs));

    /* See ARM ARM B3-19 - only deal with prefetch aborts, translation,
     * domain and permission data aborts. Alignment and external aborts are
     * not currently recoverable.
     */
    if (fault_status)
    {
	switch(fs) {
	    case 7: case 5: case 15: case 13: case 9: case 11:
		break;
	    case 2:	/* Terminal exception, not recoverable */
	    default:
		if (!send_exception_ipc(0x100 + fs, (word_t)fault_addr, context)) {
		    printf(TXT_BRIGHT "--- KD# %s ---\n" TXT_NORMAL, 
				"Unhandled Memory Abort");  

		    if( EXPECT_FALSE(get_kip()->kdebug_entry != NULL) )
			get_kip()->kdebug_entry(context);

		    halt_user_thread();
		}
		return;
	}
    }

    space_t *space = get_current_tcb()->get_space();
                                                                                
    if (space == NULL)
	space = get_kernel_space();

    bool is_kernel = ((context->cpsr & 0x1f) != 0x10);
    word_t fault_section = (word_t)fault_addr >> PAGE_BITS_1M;
    pgent_t section = *space->pgent(fault_section);

    if (EXPECT_FALSE(is_kernel)) {
	/* Special case when TCB sections are copied in for the first time */
	if (space->is_tcb_area(fault_addr))
	{
	    pgent_t ksection = *get_kernel_space()->pgent(fault_section);
	    if (section != ksection)
	    {
		*space->pgent(fault_section) = ksection;
		return;
	    }
	}

	/* Special case when UTCB sections faulted on */
	if (space->is_utcb_area(fault_addr))
	{
#ifdef CONFIG_ENABLE_FASS
	    pgent_t entry = get_cpd()->pt.pdir[fault_section];

	    // TRACEF("utcb space = %p\n", utcb_space);
	    if (entry.raw && ((entry.raw & 0xf0000000) == 0xf0000000)) {
		space_t * utcb_space = (space_t*)entry.raw;
		arm_fass.activate_other_domain(utcb_space);
		/* NB. We don't add utcb domain's section to dirty set */
		return;
	    }
	    TRACEF("utcb fault, addr = %p, tcb = %p, space = %p\n",
			    fault_addr, get_current_tcb(), space);
	    TRACEF("  entry = %lx (%lx), currdomain = %lx\n",
			    entry.raw, entry.get_domain(), current_domain);
	    TRACEF("  * pc = %p, fs = %x, fdomain = %lx\n", (addr_t)context->pc,
			    fs, (fault_status & 0xf0) >> 4);
	    return;
#else
	    pgent_t ksection = *get_kernel_space()->pgent(fault_section);
	    if (section != ksection)
	    {
		*space->pgent(fault_section) = ksection;
		return;
	    }
#endif
	}
    }

    /* Try lookup the mapping */
    bool is_valid = true;
    bool is_writable = false;

    if (section.is_valid(space, pgent_t::size_1m))
    {
	if (section.is_subtree(space, pgent_t::size_1m)) {
	    pgent_t leaf =
		    *section.subtree(space, pgent_t::size_1m)->next(
				    space, pgent_t::size_min,
				    ((word_t)fault_addr & (PAGE_SIZE_1M-1)) >> ARM_PAGE_BITS);
	    if (leaf.l2.fault.zero == 0)
		is_valid = false;
	    is_writable = leaf.is_writable(space, pgent_t::size_min);
	} else {
	    is_writable = section.is_writable(space, pgent_t::size_1m);
	}
    } else
	is_valid = false;

#ifdef CONFIG_ENABLE_FASS
    /* Does the faulter's space's section match that in the CPD for the fault
     * address? 
     */

    pgent_t spg = section;
    pgent_t cpg = get_cpd()->pt.pdir[fault_section];

    if ( is_valid && (cpg.get_domain() != current_domain) &&
		    space->is_user_area(fault_addr) )
    {
loop:
	bool cpd_section_valid = cpg.is_valid(get_cpd(), pgent_t::size_1m); 
	arm_domain_t cpd_section_domain = cpg.get_domain();

	/* Clean the kernel's UTCB synonym if necessary */
	bool need_flush = (cpd_section_valid &&  
		    TEST_BIT_WORD(domain_dirty, cpd_section_domain));

	/* If there is an existing mapping in the CPD for this section,
	 * remove it from the owner domain's set of sections in the CPD 
	 */
	if (cpd_section_valid)
	    arm_fass.remove_set(cpd_section_domain, fault_section);

	/* Update the faulter domain's set of sections in the CPD */
	arm_fass.add_set(current_domain, fault_section);

	/* Update the CPD */
	spg.set_domain(current_domain);
	get_cpd()->pt.pdir[fault_section] = spg;

	if (need_flush) {
	    bool flush = 1;

	    space_t *old_space = arm_fass.get_space(cpd_section_domain);
	    if (old_space) {
		word_t old_vspace = old_space->get_vspace();

		if (old_vspace && (space->get_vspace() == old_vspace))
		    flush = 0;
	    }

	    arm_fass.clean_all(flush);
	    SET_BIT_WORD(domain_dirty, current_domain);
	} else {
	    // need to remove all entries corresponding to fault section
	    // from TLB. Cheaper to flush than to iterate over section
	    // on SA-1100 (FIXME - check other archs' MMU).
	    arm_cache::tlb_flush();
	}

	return;
    }
#endif

/*printf("v=%x w=%x %p-%p, %p-%p\n", is_valid, is_writable,
		&get_cpd()->pt.pdir[fault_section], get_cpd()->pt.pdir[fault_section],
		space->pgent(fault_section), *space->pgent(fault_section));
*/
    space->handle_pagefault(fault_addr, (addr_t)context->pc,
		    data_abt ? is_valid && !is_writable ?
		    space_t::write : space_t::read : space_t::execute,
		    is_kernel);

#ifdef CONFIG_ENABLE_FASS
    section = *space->pgent(fault_section);
    is_valid = true;
    if (section.is_valid(space, pgent_t::size_1m))
    {
	if (section.is_subtree(space, pgent_t::size_1m)) {
	    pgent_t leaf =
		    *section.subtree(space, pgent_t::size_1m)->next(
				    space, pgent_t::size_min,
				    ((word_t)fault_addr & (PAGE_SIZE_1M-1)) >> ARM_PAGE_BITS);
	    if (leaf.l2.fault.zero == 0)
		is_valid = false;
	}
    } else
	is_valid = false;

    if (is_valid) {
	cpg = get_cpd()->pt.pdir[fault_section];

	if ( (cpg.get_domain() != current_domain) &&
			space->is_domain_area(fault_addr) )
	{
	    spg = section;
	    goto loop;
	}
    }
#endif
    if (fs == 0xf)
	arm_cache::tlb_flush_ent(fault_addr, ARM_PAGE_BITS);
    else if (fs == 0xd)
	arm_cache::tlb_flush();
}

#define GENERIC_SAVED_REGISTERS (EXCEPT_IPC_GEN_MR_NUM+1)

static bool send_exception_ipc( word_t exc_no, word_t exc_code, arm_irq_context_t *context )
{
    tcb_t *current = get_current_tcb();
//    if( current->get_exception_handler().is_nilthread() )
//	return false;
/*#warning HACK*/
    if( current->get_scheduler().is_nilthread() )
	return false;

    TRACEPOINT (EXCEPTION_IPC_GENERAL,
		printf ("EXCEPTION_IPC_GENERAL: (%p) exc_no %d, exc_code %016lx, IP = %p\n",
			current, exc_no, exc_code, (word_t)current->get_user_ip()));

    // Save message registers on the stack
    word_t saved_mr[GENERIC_SAVED_REGISTERS];
    msg_tag_t tag;

    // Save message registers.
    for( int i = 0; i < GENERIC_SAVED_REGISTERS; i++ )
	saved_mr[i] = current->get_mr(i);
    current->set_saved_partner( current->get_partner() );
    current->set_saved_state( current->get_state() );
    current->misc.ipc_copy.saved_acceptor = current->get_acceptor ();

    /* clear acceptor */
    acceptor_t acceptor;
    acceptor.clear();
    current->set_acceptor(acceptor);

    // Create the message tag.
    tag.set( 0, EXCEPT_IPC_GEN_MR_NUM, EXCEPT_IPC_GEN_LABEL, true, true );
    current->set_tag( tag );

    // Create the message.
    current->set_mr( EXCEPT_IPC_GEN_MR_IP, (word_t)current->get_user_ip() );
    current->set_mr( EXCEPT_IPC_GEN_MR_SP, (word_t)current->get_user_sp() );
    current->set_mr( EXCEPT_IPC_GEN_MR_FLAGS, (word_t)current->get_user_flags() );
    current->set_mr( EXCEPT_IPC_GEN_MR_EXCEPTNO, exc_no );
    current->set_mr( EXCEPT_IPC_GEN_MR_ERRORCODE, exc_code );

    // Deliver the exception IPC.
//    tag = current->do_ipc( current->get_exception_handler(),
//	    current->get_exception_handler() );
    tag = current->do_ipc( current->get_scheduler(), current->get_scheduler() );

    // Alter the user context if necessary.
    if( !tag.is_error() )
    {
	current->set_user_ip( (addr_t)current->get_mr(EXCEPT_IPC_GEN_MR_IP) );
	current->set_user_sp( (addr_t)current->get_mr(EXCEPT_IPC_GEN_MR_SP) );
	current->set_user_flags( current->get_mr(EXCEPT_IPC_GEN_MR_FLAGS) );
    }
    else {
	TRACEF( "Unable to deliver user exception: IPC error.\n" );
    }

    // Clean-up.
    for( int i = 0; i < GENERIC_SAVED_REGISTERS; i++ )
	current->set_mr( i, saved_mr[i] );

    current->set_acceptor(current->misc.ipc_copy.saved_acceptor);
    current->set_partner( current->get_saved_partner() );
    current->set_saved_partner( NILTHREAD );
    current->set_state( current->get_saved_state() );
    current->set_saved_state( thread_state_t::aborted );

    return !tag.is_error();
}
