/*********************************************************************
 *                
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *                
 * File path:     glue/v4-arm/tcb.h
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
 * $Id: tcb.h,v 1.37 2004/12/09 00:56:57 cvansch Exp $
 *                
 ********************************************************************/
#ifndef __GLUE__V4_ARM__TCB_H__
#define __GLUE__V4_ARM__TCB_H__

#ifndef __API__V4__TCB_H__
#error not for stand-alone inclusion
#endif

#include INC_API(syscalls.h)		/* for sys_ipc */
#include INC_ARCH(thread.h)
#include INC_ARCH(fass.h)
#include INC_ARCH(fass_inline.h)
#include INC_CPU(cache.h)
#include INC_GLUE(resource_functions.h)


INLINE word_t *tcb_t::get_stack_top(void)
{
    word_t *st = (word_t *) ((char *)this + KTCB_SIZE);

    return st;
}

/**
 * Locate current TCB by using current stack pointer and return it.
 */
INLINE tcb_t * get_current_tcb (void)
{
    register word_t stack_var asm("sp");

    return (tcb_t *) (stack_var & KTCB_MASK);
}


/**
 * read value of message register
 * @param index number of message register
 */
INLINE word_t tcb_t::get_mr(word_t index)
{
    return get_utcb()->mr[index];
}

/**
 * set the value of a message register
 * @param index number of message register
 * @param value value to set
 */
INLINE void tcb_t::set_mr(word_t index, word_t value)
{
    get_utcb()->mr[index] = value;
}

/**
 * read value of the acceptor
 */
INLINE acceptor_t tcb_t::get_acceptor(void)
{
    return get_utcb()->acceptor;
}

/**
 * set the value of the acceptor register
 * @param value value to set
 */
INLINE void tcb_t::set_acceptor(const acceptor_t value)
{
    get_utcb()->acceptor = value;
}

/**
 * copies a set of message registers from one UTCB to another
 * @param dest destination TCB
 * @param start MR start index
 * @param count number of MRs to be copied
 */
INLINE void tcb_t::copy_mrs(tcb_t * dest, word_t start, word_t count)
{
    word_t *dest_mr = &dest->get_utcb()->mr[start];
    word_t *src_mr = &get_utcb()->mr[start];
    word_t temp;

    count = count >= IPC_NUM_MR ? (IPC_NUM_MR-1) : count;

    /* This will always copy at least 1 register,
     * assuming IPCs with 0 MRs are rare.
     */
    __asm__ __volatile__ (
	"1:				\n"
	"ldr	%[tmp],	[%[src]], #4	\n"
	"subs	%[num],	%[num],	#1	\n"
	"str	%[tmp],	[%[dst]], #4	\n"
	"bpl	1b			\n"
	: [tmp] "=r" (temp),
	  [src] "+r" (src_mr),
	  [dst] "+r" (dest_mr),
	  [num]	"+r" (count)
    );
}

/**
 * allocate the tcb
 * The tcb pointed to by this will be allocated.
 */
INLINE bool tcb_t::allocate()
{
    return get_kernel_space()->allocate_tcb(this);
}


/**
 * set the address space a TCB belongs to
 * @param space address space the TCB will be associated with
 */
INLINE void tcb_t::set_space(space_t * space)
{
    this->space = space;
}

/**
 * set the global thread ID in a TCB
 * @param tid	new thread ID
 */
INLINE void tcb_t::set_global_id(threadid_t tid)
{
    myself_global = tid;
    get_utcb()->my_global_id = tid;
}

/* 
 * Return back to user_land when an IPC is aborted
 * We short circuit the restoration of any saved registers/state
 */
INLINE void tcb_t::return_from_ipc (void)
{
    char * context =
	    (char *) get_stack_top () - ARM_IPC_STACK_SIZE; 
    extern void * vector_ipc_syscall_return;
    word_t ret = (word_t)&vector_ipc_syscall_return;

    do {
	__asm__ __volatile__ (
	    "mov	sp,	%0		\n"
	    "mov	pc,	%1		\n"
	    : : "r" (context), "r" (ret)
	);
    } while (1);
}


/**
 * Short circuit a return path from a user-level interruption or
 * exception.  That is, restore the complete exception context and
 * resume execution at user-level.
 */
INLINE void tcb_t::return_from_user_interruption(void)
{
    arm_irq_context_t * context =
	    (arm_irq_context_t *) get_stack_top () - 1;
    extern void * vector_arm_abort_return;
    word_t ret = (word_t)&vector_arm_abort_return;

#ifdef CONFIG_IPC_FASTPATH
    tcb_t * current = get_current_tcb();
    current->resources.clear_kernel_ipc( current );
    current->resources.clear_except_fp( current );
#endif
    do {
	__asm__ __volatile__ (
	    "mov	sp,	%0		\n"
	    "mov	pc,	%1		\n"
	    : : "r" (context), "r" (ret)
	);
    } while (1);
    // NOT REACHED
}


INLINE void tcb_t::set_cpu(unsigned short foo) {}

/********************************************************************** 
 *
 *                      thread switch routines
 *
 **********************************************************************/

/**
 * switch to initial thread
 * @param tcb TCB of initial thread
 *
 * Initializes context of initial thread and switches to it.  The
 * context (e.g., instruction pointer) has been generated by inserting
 * a notify procedure context on the stack.  We simply restore this
 * context.
 */

INLINE void NORETURN initial_switch_to (tcb_t * tcb)
{
#ifdef CONFIG_ENABLE_FASS
    addr_t new_pt = page_table_to_phys(get_cpd());

    /* Load CPD with kernel's PD */
    for (int i = 0; i < (1 << ARM_SECTION_BITS); ++i)
        get_cpd()->pt.pdir[i] = get_kernel_space()->pt.pdir[i];

    arm_fass.activate_domain(tcb->get_space());
#else
    addr_t new_pt = tcb->get_space();

    if (new_pt == NULL)
        new_pt = get_kernel_space();

    new_pt = virt_to_phys(new_pt);
#endif

    arm_cache::cache_flush();

    USER_UTCB_REF = tcb->get_utcb_location();

    __asm__ __volatile__ (
        "    mcr     p15, 0, r0, c7, c10, 4  \n" /* drain write buffer */   
        "    mcr     p15, 0, %0, c2, c0, 0   \n" /* install new PT */      
        "    mcr     p15, 0, r0, c8, c7, 0   \n" /* flush TLB */            
        "                                    \n"               
        "    mov     sp,     %1              \n" /* load new stack ptr */   
        "                                    \n"               
        "    ldmfd   sp!,    {r4,r5,r11,lr}  \n" /* load notify context */
        "                                    \n"               
        "    mov     pc,     lr              \n" /* load new PC */  
        : 
        : "r" (new_pt), "r" (tcb->stack)
        : "r0", "memory");

    ASSERT(ALWAYS, !"We shouldn't get here!");
    while(true) {}
}


/**
 * intialize stack for given thread
 */
INLINE void tcb_t::init_stack()
{
    /* Create space for an exception context */
    arm_irq_context_t *context = (arm_irq_context_t *)get_stack_top() - 1;

    stack = (word_t *)context; /* Update new stack position */

    /* clear whole context */
    for (word_t *t = (word_t *)context; t < get_stack_top(); t++)
	*t = 0;
}



/**********************************************************************
 *
 *                        notification functions
 *
 **********************************************************************/


/**
 * create stack frame to invoke notify procedure
 * @param func notify procedure to invoke
 *
 * Create a stack frame in TCB so that next thread switch will invoke
 * the indicated notify procedure.
 */
extern "C" void arm_return_from_notify0(void);
INLINE void tcb_t::notify (void (*func)())
{
    arm_switch_stack_t *arm_switch = (arm_switch_stack_t *)stack;
    arm_switch--;
    stack = (word_t*) arm_switch;
    arm_switch->r4 = (word_t)func;
    arm_switch->lr = (word_t)arm_return_from_notify0; 
}


    
/**
 * create stack frame to invoke notify procedure
 * @param func notify procedure to invoke
 * @param arg1 1st argument to notify procedure
 *
 * Create a stack frame in TCB so that next thread switch will invoke
 * the indicated notify procedure.
 */
extern "C" void arm_return_from_notify1(void);
INLINE void tcb_t::notify (void (*func)(word_t), word_t arg1)
{
    arm_switch_stack_t *arm_switch = (arm_switch_stack_t *)stack;
    arm_switch--;
    stack = (word_t*) arm_switch;

    arm_switch->r4 = (word_t) func;
    arm_switch->r5 = arg1;
    arm_switch->lr = (word_t)arm_return_from_notify1; 
}

extern "C" void arm_return_from_notify2(void);
INLINE void tcb_t::notify (void (*func)(word_t, word_t), word_t arg1, 
        word_t arg2)
{
    arm_switch_stack_t *arm_switch = (arm_switch_stack_t *)stack;
    arm_switch--;
    stack = (word_t*) arm_switch;

    arm_switch->r4 = (word_t) func;
    arm_switch->r5 = arg1;
    arm_switch->r11 = arg2;
    arm_switch->lr = (word_t)arm_return_from_notify2; 
}

/**********************************************************************
 * 
 *            access functions for ex-regs'able registers
 *
 **********************************************************************/

/**
 * read the user-level instruction pointer
 * @return	the user-level stack pointer
 */
INLINE addr_t tcb_t::get_user_ip()
{
    arm_irq_context_t * context =
        (arm_irq_context_t *) get_stack_top () - 1;

    return (addr_t) (context)->pc;
}

/**
 * read the user-level stack pointer
 * @return	the user-level stack pointer
 */
INLINE addr_t tcb_t::get_user_sp()
{
    arm_irq_context_t * context =
        (arm_irq_context_t *) get_stack_top () - 1;

    return (addr_t) (context)->sp;
}

/**
 * set the user-level instruction pointer
 * @param ip	new user-level instruction pointer
 */
INLINE void tcb_t::set_user_ip(addr_t ip)
{
    arm_irq_context_t *context = (arm_irq_context_t *)get_stack_top() -1;

    context->pc = (word_t)ip;
#ifdef CONFIG_ARM_THUMB_SUPPORT
    /* CPU has thumb support, fix cpsr if needed */
    if ((word_t)ip & 1)
	context->cpsr |= CPSR_THUMB_BIT;
    else
	context->cpsr &= ~CPSR_THUMB_BIT;
#endif
}

/**
 * set the user-level stack pointer
 * @param sp	new user-level stack pointer
 */
INLINE void tcb_t::set_user_sp(addr_t sp)
{
    arm_irq_context_t *context = (arm_irq_context_t *)get_stack_top() -1;

    context->sp = (word_t)sp;
}

INLINE void tcb_t::copy_user_regs(tcb_t *src)
{
    arm_irq_context_t * to = (arm_irq_context_t *) this->get_stack_top () - 1;
    arm_irq_context_t * from = (arm_irq_context_t *) src->get_stack_top () - 1;

    to->r0 = from->r0;
    to->r1 = from->r1;
    to->r2 = from->r2;
    to->r3 = from->r3;
    to->r4 = from->r4;
    to->r5 = from->r5;
    to->r6 = from->r6;
    to->r7 = from->r7;
    to->r8 = from->r8;
    to->r9 = from->r9;
    to->r10 = from->r10;
    to->r11 = from->r11;
    to->r12 = from->r12;
    to->lr = from->lr;
    to->cpsr = from->cpsr;
}

INLINE void tcb_t::copy_regs_to_mrs(tcb_t *src)
{
    arm_irq_context_t * from = (arm_irq_context_t *) src->get_stack_top () - 1;

    set_mr (0, from->r0);
    set_mr (1, from->r1);
    set_mr (2, from->r2);
    set_mr (3, from->r3);
    set_mr (4, from->r4);
    set_mr (5, from->r5);
    set_mr (6, from->r6);
    set_mr (7, from->r7);
    set_mr (8, from->r8);
    set_mr (9, from->r9);
    set_mr (10, from->r10);
    set_mr (11, from->r11);
    set_mr (12, from->r12);
    set_mr (13, from->sp);
    set_mr (14, from->lr);
    set_mr (15, from->pc);
    set_mr (16, from->cpsr);
}

INLINE word_t tcb_t::get_utcb_location()
{
    return (word_t)this->utcb;
}

INLINE void tcb_t::set_utcb_location(word_t location)
{
    utcb_location = location;
}


/**
 * read the user-level flags (one word)
 * @return	the user-level flags
 */
INLINE word_t tcb_t::get_user_flags (void)
{
    arm_irq_context_t * context =
        (arm_irq_context_t *) get_stack_top () - 1;

    return (word_t) (context)->cpsr & ARM_USER_FLAGS_MASK;
}

/**
 * set the user-level flags
 * @param flags	new user-level flags
 */
INLINE void tcb_t::set_user_flags (const word_t flags)
{
    arm_irq_context_t *context = (arm_irq_context_t *)get_stack_top() -1;

    context->cpsr = (context->cpsr & ~ARM_USER_FLAGS_MASK) |
            ((word_t)flags & ARM_USER_FLAGS_MASK);
}

/**********************************************************************
 *
 *                       preemption callback function
 *
 **********************************************************************/

/**
 * set the address where preemption occured
 */
INLINE void tcb_t::set_preempted_ip(addr_t ip)
{
#ifdef CONFIG_ARM_THUMB_SUPPORT
    arm_irq_context_t *context = (arm_irq_context_t *)get_stack_top() -1;

    /* CPU has thumb support, fix ip if needed */
    if (context->cpsr & CPSR_THUMB_BIT)
	ip = (addr_t)((word_t)ip | 1);
#endif
    get_utcb()->preempted_ip = (word_t)ip;
}

INLINE addr_t tcb_t::get_preempted_ip()
{
    return (addr_t)get_utcb()->preempted_ip;
}

/**
 * get the preemption callback ip
 */
INLINE addr_t tcb_t::get_preempt_callback_ip()
{
    return (addr_t)get_utcb()->preempt_callback_ip;
}

/**********************************************************************
 *
 *                        global tcb functions
 *
 **********************************************************************/

INLINE tcb_t * addr_to_tcb(addr_t addr) 
{
    return (tcb_t *) ((word_t) addr & KTCB_MASK);
}



/**
 * invoke an IPC from within the kernel
 *
 * @param to_tid destination thread id
 * @param from_tid from specifier
 * @return IPC message tag (MR0)
 */
INLINE msg_tag_t tcb_t::do_ipc (threadid_t to_tid, threadid_t from_tid)
{
    msg_tag_t tag;

#ifdef CONFIG_IPC_FASTPATH
    tcb_t *current = get_current_tcb();
    // For fast path, we need to indicate that we are doing ipc from the kernel
    current->resources.set_kernel_ipc( current );
#endif

    sys_ipc (to_tid, from_tid);
    tag.raw = get_mr (0);

#ifdef CONFIG_IPC_FASTPATH
    current->resources.clear_kernel_ipc( current );
    current->resources.clear_except_fp( current );
#endif

    return tag;
}

/**
 * adds a thread to the space
 * @param tcb pointer to thread control block
 */

#define THR_COUNT_SHIFT 2
 
INLINE void space_t::add_tcb(tcb_t * tcb)
{
    pt.pd_split.thread_count+= (1<<THR_COUNT_SHIFT);
#ifdef CONFIG_DEBUG
    spaces_list_lock.lock();
    ENQUEUE_LIST_TAIL(pt.pd_split.thread_list, tcb, thread_list);
    spaces_list_lock.unlock();
#endif
}

/**
 * removes a thread from a space
 * @param tcb_t thread control block
 * @return true if it was the last thread
 */
INLINE bool space_t::remove_tcb(tcb_t * tcb)
{
    pt.pd_split.thread_count-= (1<<THR_COUNT_SHIFT);
#ifdef CONFIG_DEBUG
    spaces_list_lock.lock();
    DEQUEUE_LIST(pt.pd_split.thread_list, tcb, thread_list);
    spaces_list_lock.unlock();
#endif
    return (pt.pd_split.thread_count == 0);
}


#endif /* !__GLUE__V4_ARM__TCB_H__ */
