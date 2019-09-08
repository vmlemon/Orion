/*********************************************************************
 *                
 * Copyright (C) 2003-2004,  National ICT Australia (NICTA)
 *                
 * File path:     glue/v4-powerpc64/tcb.h
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
 * $Id: tcb.h,v 1.24 2005/01/18 13:28:57 cvansch Exp $
 *                
 ********************************************************************/
#ifndef __GLUE__V4_POWERPC64__TCB_H__
#define __GLUE__V4_POWERPC64__TCB_H__

#ifndef __API__V4__TCB_H__
#error not for stand-alone inclusion
#endif

#include <debug.h>

#include INC_API(syscalls.h)		/* for sys_ipc */
#include INC_API(space.h)
#include INC_ARCH(msr.h)

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
    ASSERT(NORMAL, start + count <= IPC_NUM_MR);

    asm volatile (
	"   mtctr	%0		\n"	/* Initialize the count register. */
	"   1:				\n"
	"   ldu		%0, 8(%1)	\n"	/* Load from src utcb. */
	"   stdu	%0, 8(%2)	\n"	/* Store to dest utcb. */
	"   bdnz	1b		\n"	/* Decrement ctr and branch if not zero. */
	: /* outputs */
	  "+r" (count)
	: /* inputs */
	  /* Handle pre-increment with -1 offset. */
	  "b" (&this->get_utcb()->mr[start-1]), 
	  "b" (&dest->get_utcb()->mr[start-1])
	: /* clobbers */
	  "ctr"
    );
}

/**
 * allocate the tcb
 * The tcb pointed to by this will be allocated.
 */
INLINE void tcb_t::allocate()
{
/**
 * tcb_t::allocate: allocate memory for TCB
 *
 * Allocate memory for the given TCB.  We do this by generating a
 * write to the TCB area.  If TCB area is not backed by writable
 * memory (i.e., already allocated) the pagefault handler will
 * allocate the memory and map it.
 */

    this->kernel_stack[0] = 0;
}


/**
 * set the address space a TCB belongs to
 * @param space address space the TCB will be associated with
 */
INLINE void tcb_t::set_space(space_t * space)
{
    this->space = space;
    // sometimes it might be desirable to use a pdir cache,
    // like in cases where it's not cheap to derive the page
    // directory from the space
    //this->pdir_cache = (word_t)space->get_pdir();
}


/**
 * set the global thread ID in a TCB
 * @param tid	new thread ID
 */
INLINE void tcb_t::set_global_id(threadid_t tid)
{
    this->myself_global = tid;
    get_utcb()->my_global_id = tid;
}

/**
 * set the cpu in a TCB
 * @param cpu	new cpu number
 */

INLINE void tcb_t::set_cpu(cpuid_t cpu) 
{ 
#ifdef CONFIG_SMP
    this->cpu = cpu;
    get_utcb()->processor_no = cpu;
#endif
}


INLINE void tcb_t::return_from_ipc (void)
{
    register word_t r14 asm("r14");
    extern char _restore_all_ipc[];
    powerpc64_irq_context_t * context =
	(powerpc64_irq_context_t *) get_stack_top () - 1;

    r14 = get_tag ().raw;

    do {
	asm volatile (
	    "mr	    %%r1, %0;"
	    "mtlr   %1;	    "
	    "blr;	    "
	    :: "r" (context), "r" (&_restore_all_ipc),
	       "r" (r14)
	);
    } while (1);
}

/**
 * Short circuit a return path from a user-level interruption or
 * exception.  That is, restore the complete exception context and
 * resume execution at user-level.
 */
INLINE void tcb_t::return_from_user_interruption (void)
{
    extern char _restore_all[];
    powerpc64_irq_context_t * context =
	(powerpc64_irq_context_t *) get_stack_top () - 1;

    do {
	asm volatile (
	    "mr	    %%r1, %0;"
	    "mtlr   %1;	    "
	    "blr;	    "
	    :: "r" (context), "r" (&_restore_all)
	);
    } while (1);
}

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
#if CONFIG_POWERPC64_SLB
    word_t asr = get_kernel_space()->get_vsid_asid(); 
#elif CONFIG_POWERPC64_STAB
    word_t asr = get_kernel_space()->get_seg_table()->get_asr();
    asm volatile (
	"mfsprg	%%r7, %0;	"
	"std	%1, %2(%%r7);	"
	:: "i" (SPRG_LOCAL),
	   "r" (get_kernel_space()->get_vsid_asid()),
	   "i" (LOCAL_VSID_ASID)
	: "r7"
    );
#endif

    TRACEF("(%p), %p\n", tcb, tcb->stack);

    asm volatile (
	"mtsprg	%[tcb_sprg], %[tcb];"
	"mtasr	%[asr];	"
	"mr	%%r1, %[sp];	"
	"ld	%%r13, 32(%%r1);	"	/* Load func in temp1			*/
	"ld	%%r30, 40(%%r1);	"	/* Load arg1 in temp2			*/
	"ld	%%r31, 48(%%r1);	"	/* Load arg2 in temp3			*/
	"addi	%%r1, %%r1, 64;		"	/* Unstack the frame			*/
	"ld	%%r3, 16(%%r1);		"	/* Load the return address		*/
	"isync;				"
	"mtlr	%%r3;			"
	"blr;				"
	::
	[tcb] "r" (tcb),
	[sp] "r" (tcb->stack),
	[asr] "r" (asr),
	[tcb_sprg] "i" (SPRG_TCB)
    );

    while(true) {}
}

/**
 * switches to another tcb thereby switching address spaces if needed
 * @param dest tcb to switch to
 */
INLINE void tcb_t::switch_to(tcb_t * dest)
{
    space_t *space = dest->get_space();
    space_t *currspace = this->get_space();

    if (space == NULL)
	space = get_kernel_space();
    if (currspace == NULL)
	currspace = get_kernel_space();


#if CONFIG_POWERPC64_SLB
    word_t asr = space->get_vsid_asid(); 
#elif CONFIG_POWERPC64_STAB
    word_t asr = space->get_seg_table()->get_asr();
    asm volatile (
	"mfsprg	%%r7, %0;	"
	"std	%1, %2(%%r7);	"
	:: "i" (SPRG_LOCAL),
	   "r" (space->get_vsid_asid()),
	   "i" (LOCAL_VSID_ASID)
	: "r7"
    );
#endif

    asm volatile (
	"stdu	%%r1, -64(%%r1);	"	/* Create switch stack			*/
	"lis	%%r3, 1f@highest;	"	/* Load return address			*/
	"ori	%%r3, %%r3, 1f@higher;	"
	"rldicr	%%r3, %%r3, 32, 31;	"
	"oris	%%r3, %%r3, 1f@h;	"
	"ori	%%r3, %%r3, 1f@l;	"
	"std	%%r3, 16+64(%%r1);		"	/* Save the return address		*/

	"std	%%r1, 0(%[from_stack_save]);	    "	/* Save the stack pointer	*/
	"std	%%r2, 24(%%r1);		"	/* Save TOC in temp0			*/
	"std	%%r13, 32(%%r1);	"	/* Save Thread in temp1			*/
	"std	%%r30, 40(%%r1);	"	/* Save r30 in temp2			*/
	"std	%%r31, 48(%%r1);	"	/* Save r31 in temp3			*/

	"cmpld	cr0, %[from_space], %[dest_space];  "	/* Are the spaces the same?	*/
	"beq+	0f ;			"	/* Skip addr space switch if possible.	*/

#if CONFIG_PLAT_OFPOWER4 || CONFIG_CPU_POWERPC64_PPC970

	/* Optimized Power4 Context Switch
	 * Clear Only the Valid SLB Entries
	 * Preload New Entries
	 */
	"li	%%r13, 16;			"
	"slbmfee    %%r4, %%r13;		"
	"3:;					"
	"andis.	%%r5, %%r4, 0x0800;		"
	"beq-	7f;				"

	"slbmfev    %%r5, %%r13;		"

	"std	%%r4, 568(%[from_space]);	"
	"std	%%r5, 576(%[from_space]);	"

	"rldicl	%%r5, %%r4,36,28;		"
	"rldicr	%%r5, %%r5,28,35;		"
	"slbie	%%r5;				"

	"addi	%%r13, %%r13, 1;		"
	"cmpdi	%%r13, 40;			"
	"slbmfee	%%r4, %%r13;		"
	"addi	%[from_space], %[from_space], 16;	"
	"blt+	3b;				"
	"7:;					"
	"li	%%r4, 0;			"
	"std	%%r4, 568(%[from_space]);	"

	"mtasr	%[asr];			"	/* Setup new ASR			*/

	"li	%%r13, 16;			"
	"mr	%%r3, %[dest_space];		"
	"11:;					"
	"ld	%%r4, 568(%%r3);		"
	"ld	%%r5, 576(%%r3);		"
	"cmpdi	%%r4, 0;			"
	"beq-	10f;				"
	"or	%%r4, %%r4, %%r13;		"
	"slbmte	%%r5, %%r4;			"
	"addi	%%r3, %%r3, 16;			"
	"addi	%%r13, %%r13, 1;		"
	"b	11b;				"
	"10:;					"

	"isync;				"
#elif CONFIG_PLAT_OFPOWER3

	"isync;				"
	"mtasr %[asr];			"
	"slbia;				"	/* Flush the SLB			*/
	"isync;				"

#endif

	"0:				"

	"mtsprg	%[tcb_sprg], %[desttcb];	    "   /* TCB into Supervisor reg	*/
	"mr	%%r1, %[dest_stack];	"	/* Set the new stack			*/
	"ld	%%r2, 24(%%r1);		"	/* Load TOC in temp0			*/
	"ld	%%r13, 32(%%r1);	"	/* Load Thread in temp1			*/
	"ld	%%r30, 40(%%r1);	"	/* Load r30 in temp2			*/
	"ld	%%r31, 48(%%r1);	"	/* Load r31 in temp3			*/
	"addi	%%r1, %%r1, 64;		"	/* Unstack the frame			*/
	"ld	%%r3, 16(1);		"	/* Load the return address		*/
	"mtlr	%%r3;			"
	"blr;				"
	"1:				"
	::
	[desttcb] "r" (dest),
	[dest_stack] "r" (dest->stack),
	[from_stack_save] "b" (&this->stack),
	[asr] "r" (asr),
	[dest_space] "b" (space),
	[from_space] "r" (currspace),
	[tcb_sprg] "i" (SPRG_TCB)
	: /* Trashed */
	"r3", "r4", "r5", "lr",
	"cr0","cr1","cr2","cr3","cr4","cr5","cr6","cr7",
	"memory"
    );
    /* Trash the rest. This allows the compiler to choose which
     * inputs to use in the switch code
     */
    asm volatile (
	"" :::
	"r0", "r6", "r7", "r8", "r9", "r10",
	"r11", "r12", "r14", "r15", "r16", "r17", "r18",
	"r19", "r20", "r21", "r22", "r23", "r24", "r25",
	"r26", "r27", "r28", "r29",
	"ctr", "xer", "memory"
    );
}


INLINE word_t *tcb_t::get_stack_top(void)
{
    /* Simon says : Evil? */
    return (word_t *) ((char *) this + KTCB_SIZE);
}


/**
 * intialize stack for given thread
 */
INLINE void tcb_t::init_stack()
{
    /* Create space for an exception context */
    powerpc64_irq_context_t * context =
	(powerpc64_irq_context_t *) get_stack_top () - 1;

    this->stack = (word_t *) context;	/* Update new stack position */

    /* Clear whole context */
    for (word_t * t = (word_t *) context; t < get_stack_top (); t++)
	*t = 0;

    //TRACEF("[%p] stack = %p\n", this, stack);
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
INLINE void tcb_t::notify (void (*func)())
{
    powerpc64_switch_stack_t *frame = (powerpc64_switch_stack_t *)this->stack;
    register word_t toc asm("r2");

    frame->lr_save = *(word_t *) &powerpc64_do_notify;
    frame--;
    frame->back_chain = (word_t)frame;
    frame->temp0 = toc;
    frame->temp1 = *(word_t *)func;

    this->stack = (word_t *)frame;

    //TRACEF("%p (%p) , %016lx\n", this, frame->temp1, (word_t)stack);
}

/**
 * create stack frame to invoke notify procedure
 * @param func notify procedure to invoke
 * @param arg1 1st argument to notify procedure
 *
 * Create a stack frame in TCB so that next thread switch will invoke
 * the indicated notify procedure.
 */
INLINE void tcb_t::notify (void (*func)(word_t), word_t arg1)
{
    powerpc64_switch_stack_t *frame = (powerpc64_switch_stack_t *)this->stack;
    register word_t toc asm("r2");

    frame->lr_save = *(word_t *) &powerpc64_do_notify;
    frame--;
    frame->back_chain = (word_t)frame;
    frame->temp0 = toc;
    frame->temp1 = *(word_t *)func;
    frame->temp2 = arg1;

    this->stack = (word_t *)frame;

    //TRACEF("%p (%p)(0x%x), %016lx\n", this, frame->temp1, arg1, (word_t)stack);
}

/**
 * create stack frame to invoke notify procedure
 * @param func notify procedure to invoke
 * @param arg1 1st argument to notify procedure
 * @param arg2 2nd argument to notify procedure
 *
 * Create a stack frame in TCB so that next thread switch will invoke
 * the indicated notify procedure.
 */

INLINE void tcb_t::notify (void (*func)(word_t, word_t), word_t arg1, word_t arg2)
{
    powerpc64_switch_stack_t *frame = (powerpc64_switch_stack_t *)this->stack;
    register word_t toc asm("r2");

    frame->lr_save = *(word_t *) &powerpc64_do_notify;
    frame--;
    frame->back_chain = (word_t)frame;
    frame->temp0 = toc;
    frame->temp1 = *(word_t *)func;
    frame->temp2 = arg1;
    frame->temp3 = arg2;

    this->stack = (word_t *)frame;

    //TRACEF("%p (%p)(0x%x,0x%x), %016lx\n", this, frame->temp1, arg1, arg2, (word_t)stack);
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
    powerpc64_irq_context_t * context =
	(powerpc64_irq_context_t *) get_stack_top () - 1;

    return (addr_t) (context)->srr0;
}

/**
 * read the user-level stack pointer
 * @return	the user-level stack pointer
 */
INLINE addr_t tcb_t::get_user_sp()
{
    powerpc64_irq_context_t * context =
	(powerpc64_irq_context_t *) get_stack_top () - 1;

    return (addr_t) (context)->r1;
}


/**
 * set the user-level instruction pointer
 * @param ip	new user-level instruction pointer
 */
INLINE void tcb_t::set_user_ip(addr_t ip)
{
    powerpc64_irq_context_t * context =
	(powerpc64_irq_context_t *) get_stack_top () - 1;

    context->srr0 = (word_t)ip;
}

/**
 * set the user-level stack pointer
 * @param sp	new user-level stack pointer
 */
INLINE void tcb_t::set_user_sp(addr_t sp)
{
    powerpc64_irq_context_t * context =
	(powerpc64_irq_context_t *) get_stack_top () - 1;

    context->r1 = (word_t)sp;
}

INLINE void tcb_t::copy_saved_regs(tcb_t *src)
{
    powerpc64_irq_context_t * to =
	    (powerpc64_irq_context_t *) this->get_stack_top () - 1;
    powerpc64_irq_context_t * from =
	    (powerpc64_irq_context_t *) src->get_stack_top () - 1;

    to->cr = from->cr;
    to->r1 = from->r1;
    to->r2 = from->r2;
    to->r13 = from->r13;
    to->r14 = from->r14;
    to->r15 = from->r15;
    to->r16 = from->r16;
    to->r17 = from->r17;
    to->r18 = from->r18;
    to->r19 = from->r19;
    to->r20 = from->r20;
    to->r21 = from->r21;
    to->r22 = from->r22;
    to->r23 = from->r23;
    to->r24 = from->r24;
    to->r25 = from->r25;
    to->r26 = from->r26;
    to->r27 = from->r27;
    to->r28 = from->r28;
    to->r29 = from->r29;
    to->r30 = from->r30;
    to->r31 = from->r31;
    to->srr1 = from->srr1;
}

INLINE void tcb_t::copy_volatile_regs(tcb_t *src)
{
    powerpc64_irq_context_t * to =
	    (powerpc64_irq_context_t *) this->get_stack_top () - 1;
    powerpc64_irq_context_t * from =
	    (powerpc64_irq_context_t *) src->get_stack_top () - 1;

    to->r0 = from->r0;
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
    to->ctr = from->ctr;
    to->xer = from->xer;
}

INLINE word_t tcb_t::get_utcb_location()
{
    return utcb_location;
}

INLINE void tcb_t::set_utcb_location(word_t location)
{
    powerpc64_irq_context_t * context =
	(powerpc64_irq_context_t *) get_stack_top () - 1;

    //TRACEF( "(%p) utcb -> %p\n", this, utcb_location );

    utcb_location = location;
    context->r13 = utcb_location;
}


/**
 * read the user-level flags (one word)
 * @return	the user-level flags
 */
INLINE word_t tcb_t::get_user_flags (void)
{
    powerpc64_irq_context_t * context =
	(powerpc64_irq_context_t *) get_stack_top () - 1;

    return context->srr1;
}

/**
 * set the user-level flags
 * @param flags	new user-level flags
 */
INLINE void tcb_t::set_user_flags (const word_t flags)
{
    powerpc64_irq_context_t * context =
	(powerpc64_irq_context_t *) get_stack_top () - 1;

    /* Make sure user can't promote themselves etc */
    word_t old_flags = context->srr1;
    context->srr1 = (flags & MSR_USER_MASK) | (~MSR_USER_MASK & old_flags);
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

__attribute__ ((const)) INLINE tcb_t * addr_to_tcb (addr_t addr)
{
    return (tcb_t *) ((word_t) addr & KTCB_MASK);
}

/**
 * Locate current TCB by using current stack pointer and return it.
 */
INLINE tcb_t * get_current_tcb (void)
{
    tcb_t * tcb;
    asm volatile (
	"mfsprg	%[tcb], %[tcb_sprg];"
	: [tcb] "=r" (tcb)
	: [tcb_sprg] "i" (SPRG_TCB)
    );
    return tcb;
};

#if defined(CONFIG_SMP)
INLINE cpuid_t get_current_cpu (void)
{
    return get_idle_tcb ()->get_cpu ();
}
#endif

/**
 * invoke an IPC from within the kernel
 *
 * @param to_tid destination thread id
 * @param from_tid from specifier
 * @param timeout IPC timeout
 * @return IPC message tag (MR0)
 */
INLINE msg_tag_t tcb_t::do_ipc (threadid_t to_tid, threadid_t from_tid,
                                timeout_t timeout)
{
    msg_tag_t tag;
    sys_ipc(to_tid, from_tid, timeout);
    tag.raw = get_mr (0);

    return tag;
}

/**
 * adds a thread to the space
 * @param tcb pointer to thread control block
 */
INLINE void space_t::add_tcb(tcb_t * tcb)
{
    x.thread_count ++;
#ifdef CONFIG_DEBUG
    spaces_list_lock.lock();
    ENQUEUE_LIST_TAIL(x.thread_list, tcb, thread_list);
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
    ASSERT(DEBUG, x.thread_count != 0);
    x.thread_count --;
#ifdef CONFIG_DEBUG
    spaces_list_lock.lock();
    DEQUEUE_LIST(x.thread_list, tcb, thread_list);
    spaces_list_lock.unlock();
#endif
    return (x.thread_count == 0);
}


#endif /* !__GLUE__V4_POWERPC64__TCB_H__ */
