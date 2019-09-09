/*********************************************************************
 *                
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *                
 * File path:     glue/v4-arm/thread.cc
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
 * $Id: thread.cc,v 1.4 2004/06/04 06:32:02 cvansch Exp $
 *                
 ********************************************************************/

#include <l4.h>
#include <debug.h>
#include <kdb/tracepoints.h>
#include INC_API(tcb.h)
#include INC_ARCH(thread.h)

DECLARE_TRACEPOINT(FASTPATH_ASYNC_IPC);

extern "C" void arm_initial_to_user(void);

/**
 * Setup TCB to execute a function when switched to
 * @param func pointer to function
 *
 * The old stack state of the TCB does not matter.
 */
void tcb_t::create_startup_stack (void (*func)())
{
    init_stack();

    notify(arm_initial_to_user);
    notify(func);

    arm_irq_context_t *context = (arm_irq_context_t *)get_stack_top() - 1;

    /* set user mode */
    context->cpsr = CPSR_USER_MODE;
}

/**
 * switches to another tcb thereby switching address spaces if needed
 * Note: current_tcb may not be in dest->space, be careful
 * @param dest tcb to switch to
 */
void tcb_t::switch_to(tcb_t * dest)
{
    if (EXPECT_FALSE(resource_bits))
        resources.save(this);

#ifdef CONFIG_ENABLE_FASS
    if (EXPECT_TRUE(dest->get_space() != get_space())) 
        arm_fass.activate_domain(dest->get_space());

    // USER_UTCB_REF = dest->get_utcb_location();

    __asm__ __volatile__ (
	"   mov	    r12,    #0xff000000		\n"
	"   str	    %2,	    [r12, #0xff0]	\n" /* USER_UTCB_REF = dest->get_utcb_location() */
	"   adr	    r12,    1f			\n"
	"   stmfd   sp!,    {r4, r5, r11, r12}	\n"
	"   str	    sp,	    [%0]		\n"
	"   mov	    sp,	    %1			\n"
	"   ldmfd   sp!,    {r4, r5, r11, pc}	\n"
	"1:					\n"
	:
	: "r" (&stack), "r" (dest->stack), "r" (dest->get_utcb_location())
	: "r12", "memory"
    );
    __asm__ __volatile__ ("" ::: "r0","r1","r2","r3","r6","r7" );
    __asm__ __volatile__ ("" ::: "r8","r9","r10","r11","memory" );

#else
    addr_t new_pt;
    space_t *dest_space = dest->get_space();
    space_t *from_space = this->get_space();

    if (EXPECT_FALSE(dest_space != from_space))
    {
        if (dest_space == NULL) 
            dest_space = get_kernel_space();

        new_pt = virt_to_ram(dest_space);

	arm_cache::cache_flush();
	arm_cache::tlb_flush();
    } else {
	new_pt = NULL;
    }

    __asm__ __volatile__ (
	"   adr	    r12,    1f			\n"
	"   stmfd   sp!,    {r4, r5, r11, r12}	\n"
	"   str	    sp,	    [%0]		\n"
	"   mov	    sp,	    %1			\n"
	"   cmp	    %2,	    #0			\n"
	"   mcrne   p15, 0, %2, c2, c0, 0	\n" /* Set new page table */
	"   str	    %4,	    [%3, #0xff0]	\n" /* USER_UTCB_REF = dest->get_utcb_location() */
	"   ldmfd   sp!,    {r4, r5, r11, pc}	\n"
	"1:					\n"
	:
	: "r" (&stack), "r" (dest->stack), "r" (new_pt),
	  "r" (USER_KIP_PAGE), "r" (dest->get_utcb_location())
	: "r12", "memory"
    );
    __asm__ __volatile__ ("" ::: "r0","r1","r2","r3","r6","r7" );
    __asm__ __volatile__ ("" ::: "r8","r9","r10","r11","memory" );
#endif

    if (EXPECT_FALSE(resource_bits))
        resources.load(this);
}

#include INC_API(schedule.h)

#ifdef CONFIG_IPC_FASTPATH
extern "C"
void async_fp_helper(tcb_t * to_tcb, tcb_t * current)
{
    word_t link;
    {
	register word_t r_current   asm("r9") = (word_t)current;
	register word_t r_to_tcb    asm("r2") = (word_t)to_tcb;
	register word_t r_from	    asm("lr") = 0;
	/* to_tcb	= r2 */
	/* current	= r9 */
	__asm__ __volatile__ (
	    "   .global async_fp_helper_asm		\n"
	    "   .balign	32				\n"
	    "async_fp_helper_asm:			\n"
	    "	sub	sp,	sp,	#4		\n"
	: "+r" (r_to_tcb), "+r" (r_current), "+r" (r_from)
	);

	to_tcb = (tcb_t*)r_to_tcb;
	current = (tcb_t*)r_current;

	link = r_from;
    }

    scheduler_t * scheduler = get_current_scheduler();

    TRACEPOINT_TB (FASTPATH_ASYNC_IPC, ,
		   "async_fp %t => %t",
		   TID(current->get_global_id()),
		   TID(to_tcb->get_global_id()));

    if (EXPECT_TRUE(scheduler->check_dispatch_thread (current, to_tcb) ))
    {
	scheduler->enqueue_ready(current);

#ifdef CONFIG_ENABLE_FASS
	if (EXPECT_TRUE(to_tcb->get_space() != current->get_space())) 
	    arm_fass.activate_domain(to_tcb->get_space());

	__asm__ __volatile__ (
	    "   mov	    r12,    #0xff000000		\n"
	    "   str	    %2,	    [r12, #0xff0]	\n" //	USER_UTCB_REF = dest->get_utcb_location()
	    "   adr	    r12,    1f			\n"
	    "   stmfd	    sp!,    {r4, r5, r11, r12}	\n"
	    "   str	    sp,	    [%0]		\n"
	    "   mov	    sp,	    %1			\n"
	    "   ldmfd	    sp!,    {r4, r5, r11, pc}	\n"
	    "1:						\n"
	    :
	    : "r" (&current->stack), "r" (to_tcb->stack), "r" (to_tcb->get_utcb_location())
	    : "r12", "memory"
	);
	__asm__ __volatile__ ("" ::: "r0","r1","r2","r3","r6","r7" );
	__asm__ __volatile__ ("" ::: "r8","r9","r10","r11","memory" );
#else
	current->switch_to(to_tcb);
#endif
    }
    else
    {
	scheduler->enqueue_ready(to_tcb);
    }

    {
	__asm__ __volatile__ (
	    "	add	sp,	sp,	#4		\n"
	    "   mov	pc,	%0			\n"
	    :: "r" (link)
	);
    }
    while (1);
}
#endif
