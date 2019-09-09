/*********************************************************************
 *                
 * Copyright (C) 2002, 2004-2003,  Karlsruhe University
 *                
 * File path:     glue/v4-amd64/tcb.h
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
 * $Id: 
 *                
 ********************************************************************/
#ifndef __GLUE__V4_AMD64__TCB_H__
#define __GLUE__V4_AMD64__TCB_H__

#ifndef __API__V4__TCB_H__
#error not for stand-alone inclusion
#endif


#include INC_API(syscalls.h)		/* for sys_ipc */
#include INC_ARCH(tss.h)		/* for amd64_tss_t */
#include INC_GLUE(resource_functions.h)	/* for amd64_tss_t */

INLINE void tcb_t::set_utcb_location(word_t location)
{
    /* utcb address points to mr0 */
    utcb_t * dummy = (utcb_t*)NULL;
    utcb_location = location + ((word_t)&dummy->mr[0]);
}

INLINE word_t tcb_t::get_utcb_location()
{
    utcb_t * dummy = (utcb_t*)NULL;
    return utcb_location - ((word_t)&dummy->mr[0]); 
}

/**
 * sets the CPU
 * @param cpuid
 */
INLINE void tcb_t::set_cpu(cpuid_t cpu)
{
    this->cpu = cpu;

    // only update UTCB if there is one
    if (get_utcb())
        get_utcb()->processor_no = cpu;

    // update the pdir cache on migration
    if (this->space)
        this->pdir_cache = (word_t)space->get_pml4(get_cpu());

}

/**
 * read value of message register
 * @param index number of message register
 */
INLINE word_t tcb_t::get_mr(word_t index)
{
    return get_utcb()->mr[index];
}

/*
 * set the value of a message register
 * @param index number of message register
 * @param value value to set
 */
INLINE void tcb_t::set_mr(word_t index, word_t value)
{
    get_utcb()->mr[index] = value;
}

/**
  * copies a set of message registers from one UTCB to another
  * @param dest destination TCB
  * @param start MR start index
  * @param count number of MRs to be copied
  */
INLINE void tcb_t::copy_mrs(tcb_t * dest, word_t start, word_t count)
{
     ASSERT(start + count <= IPC_NUM_MR);
     ASSERT(count > 0);

     word_t dummy;
     /* use optimized AMD64 copy loop -- uses complete cacheline
        transfers */
     __asm__ __volatile__ (
         "repnz movsq (%%rsi), (%%rdi)\n"
         : /* output */
	 "=S"(dummy), "=D"(dummy), "=c"(dummy)
         : /* input */
         "c"(count), "S"(&get_utcb()->mr[start]),
         "D"(&dest->get_utcb()->mr[start]));
}

/**
 * read value of buffer register
 * @param index number of buffer register
 */
INLINE word_t tcb_t::get_br(word_t index)
{
    return get_utcb()->br[32U-index];
}


/**
 * set the value of a buffer register
 * @param index number of buffer register
 * @param value value to set
 */
INLINE void tcb_t::set_br(word_t index, word_t value)
{
    get_utcb()->br[32U-index] = value;
}


/**
 * allocate the tcb
 * The tcb pointed to by this will be allocated.
 */
INLINE void tcb_t::allocate()
{
    __asm__ __volatile__(
	"orq $0, %0\n"
	:
	: "m"(*this));

}


/**
 * set the address space a TCB belongs to
 * @param space address space the TCB will be associated with
 */
INLINE void tcb_t::set_space(space_t * space)
{ 
    //TRACEF("tcb=%t, space=%p\n", this, space);
    this->space = space;
    this->pdir_cache = (word_t)space->get_pml4();
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


/**
 * Short circuit a return path from an IPC system call.  The error
 * code TCR and message registers are already set properly.  The
 * function only needs to restore the appropriate user context and
 * return execution to the instruction directly following the IPC
 * system call.
 */
INLINE void tcb_t::return_from_ipc (void)
{

    register word_t utcb asm("r12") = get_local_id ().get_raw ();

    asm("movq %0, %%rsp\n"
	"retq\n"
	:
	:
	"r" (&get_stack_top ()[KSTACK_RET_IPC]),
	"r" (utcb),
	"d" (get_tag ().raw)
	);	
    
}


/**
 * Short circuit a return path from a user-level interruption or
 * exception.  That is, restore the complete exception context and
 * resume execution at user-level.
 */
INLINE void tcb_t::return_from_user_interruption (void)
{
    asm("movq %0, %%rsp\n"
	"retq\n"
	:
	: "r"(&get_stack_top()[- sizeof(amd64_exceptionframe_t)/8 - 1]));
    
}


/********************************************************************** 
 *
 *                      thread switch routines
 *
 **********************************************************************/

#ifndef BUILD_TCB_LAYOUT
#include <tcb_layout.h> 
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
	
    asm("movq %0, %%rsp\n"
         "retq\n"
         :
	: "r"(tcb->stack));
    
     while (true)
         /* do nothing */;
}
/**
 * switches to another tcb thereby switching address spaces if needed
 * @param dest tcb to switch to
 */

INLINE void tcb_t::switch_to(tcb_t * dest)
{
    extern amd64_tss_t tss;
    word_t dummy;
    
    ASSERT(dest->stack);
    ASSERT(dest != this);
    ASSERT(get_cpu() == dest->get_cpu());

    if ( EXPECT_FALSE(this->resource_bits))
	resources.save(this);
	
    /* modify stack in tss */
    tss.set_rsp0((u64_t)dest->get_stack_top());

#if 0
    TRACEF("\ncurr=%t (sp=%p, pdc=%p, spc=%p)\ndest=%t (sp=%p, pdc=%p, spc=%p)\n",
	   this, this->stack, this->pdir_cache, this->space,
	   dest, dest->stack, dest->pdir_cache, dest->space);
    //enter_kdebug("hmm");
#endif

    TBUF_RECORD_EVENT("switch %t => %t", (word_t)this, (word_t)dest);

#ifdef CONFIG_SMP
    set_active_space(get_cpu(), dest->space);
#endif
    __asm__ __volatile__ (
	"/* switch_to_thread */			\n\t"
	"pushq	%%rbp				\n\t"	/* save rbp			*/
	
	"pushq	$3f				\n\t"	/* store return address		*/
	
	"movq	%%rsp, %c[stack](%[stcb])	\n\t"	/* switch stacks		*/
	"movq	%c[stack](%[dtcb]), %%rsp	\n\t"
	
	"cmpq	%[spdir], %[dpdir]		\n\t"	/* same pdir_cache?		*/
	"je	2f				\n\t"

	"cmpq	$0, %c[space](%[dtcb])		\n\t"	/* kernel thread (space==NULL)?	*/
	"jnz	1f				\n\t"	
	"movq	%[spdir], %c[pdir](%[dtcb])	\n\t"	/* yes: update dest->pdir_cache */
	"jmp	2f				\n\t"

	"1:					\n\t"
	"movq	%[dpdir], %%cr3			\n\t"	/* no:  reload pagedir		*/
	"2:					\n\t"
	"popq	%%rdx				\n\t"	/* load (new) return address	*/
	"movq   %[utcb], %%gs:0		        \n\t"   /* update current UTCB		*/
	"jmpq	*%%rdx				\n\t"	/* jump to new return address 	*/

	"3:					\n\t"
	"movq   %[dtcb], %[stcb]	        \n\t"   /* restore this			 */
	"popq	%%rbp				\n\t"	/* restore rbp			*/
	"/* switch_to_thread */			\n\t"
	: /* output */
	  "=a" (dummy)						/* %0 RAX */
	: /* input */
	  [dtcb]	"D" (dest),				/* %1 RDI */
	  [stcb]	"S" (this),				/* %2 RSI */
	  [stack]	"i" (OFS_TCB_STACK),			/* %3 IMM */
	  [space]	"i" (OFS_TCB_SPACE),			/* %4 IMM */
	  [pdir]	"i" (OFS_TCB_PDIR_CACHE),		/* %5 IMM */
	  [dpdir]	"0" (dest->pdir_cache),			/* %6 RAX */
	  [spdir]	"c" (this->pdir_cache),			/* %7 RCX */
	  [utcb]	"b" (dest->get_local_id().get_raw())	/* %8 RBX */

	: /* clobber - trash global registers */ 
	  "memory", "rdx", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
	);
	
    if ( EXPECT_FALSE(this->resource_bits) )
	resources.load(this);
}

#endif



INLINE word_t * tcb_t::get_stack_top()
{
    
     return (word_t*)addr_offset(this, KTCB_SIZE);
}

/**
 * intialize stack for given thread
 */
INLINE void tcb_t::init_stack()
{ 
    stack = get_stack_top();
}




/**********************************************************************
 *
 *                        Notification functions
 *
 **********************************************************************/

/* notify prologue pops the arguments in their registers */
extern "C" void notify_prologue(void);

/**
 * create stack frame to invoke notify procedure
 * @param func notify procedure to invoke
 *
 * Create a stack frame in TCB so that next thread switch will invoke
 * the indicated notify procedure.
 */
INLINE void tcb_t::notify (void (*func)())
{
   *(--stack) = (word_t)func;   
   stack-=2;
   *(--stack) = (word_t)notify_prologue;
   

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
    *(--stack) = (word_t)func;   
    stack--;
    *(--stack) = arg1;
    *(--stack) = (word_t)notify_prologue;
    
}

/**
 * create stack frame to invoke notify procedure
 * @param func notify procedure to invoke
 * @param arg1 1st argument to notify procedure
 * @param arg2 2st argument to notify procedure
 *
 * Create a stack frame in TCB so that next thread switch will invoke
 * the indicated notify procedure.
 */
INLINE void tcb_t::notify (void (*func)(word_t, word_t), word_t arg1, word_t arg2)
{
    *(--stack) = (word_t)func;   
    *(--stack) = arg2;
    *(--stack) = arg1;
    *(--stack) = (word_t)notify_prologue;
    
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
    return (addr_t)get_stack_top()[KSTACK_UIP];
}

/**
 * read the user-level stack pointer
 * @return	the user-level stack pointer
 */
INLINE addr_t tcb_t::get_user_sp()
{
    return (addr_t)get_stack_top()[KSTACK_USP];
}

/**
 * read the user-level flags (one word)
 * @return	the user-level flags
 */
INLINE word_t tcb_t::get_user_flags (void)
{
    return get_stack_top()[KSTACK_UFLAGS];
}

/**
 * set the user-level instruction pointer
 * @param ip	new user-level instruction pointer
 */
INLINE void tcb_t::set_user_ip(addr_t ip)
{
    get_stack_top()[KSTACK_UIP] = (u64_t)ip;
}

/**
 * set the user-level stack pointer
 * @param sp	new user-level stack pointer
 */
INLINE void tcb_t::set_user_sp(addr_t sp)
{
    get_stack_top()[KSTACK_USP] = (u64_t)sp;
}



/**
 * set the user-level flags
 * @param flags	new user-level flags
 */
INLINE void tcb_t::set_user_flags (const word_t flags)
{
    get_stack_top()[KSTACK_UFLAGS] = (get_user_flags() & (~AMD64_USER_FLAGMASK)) | (flags & AMD64_USER_FLAGMASK);
}

/**********************************************************************
 *
 *                        global tcb functions
 *
 **********************************************************************/

INLINE tcb_t * addr_to_tcb (addr_t addr)
{
    return (tcb_t *) ((word_t) addr & KTCB_MASK);
}

INLINE tcb_t * get_current_tcb()
{
    addr_t stack;
    asm ("leaq -8(%%rsp), %0" :"=r" (stack));
    return addr_to_tcb (stack);
    
}

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
    
    sys_ipc (to_tid, from_tid, 0, 0, 0, 0, timeout);
    tag.raw = get_mr (0);

    return tag;
}


/**
 * adds a thread to the space
 * @param tcb pointer to thread control block
 */
INLINE void space_t::add_tcb(tcb_t * tcb)
{
    x[CONFIG_SMP_MAX_CPUS].thread_count++;
#ifdef CONFIG_DEBUG
    spaces_list_lock.lock();
    ENQUEUE_LIST_TAIL(x[CONFIG_SMP_MAX_CPUS].thread_list, tcb, thread_list);
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
    x[CONFIG_SMP_MAX_CPUS].thread_count--;
#ifdef CONFIG_DEBUG
    spaces_list_lock.lock();
    DEQUEUE_LIST(x[CONFIG_SMP_MAX_CPUS].thread_list, tcb, thread_list);
    spaces_list_lock.unlock();
#endif
    return (x[CONFIG_SMP_MAX_CPUS].thread_count == 0);
}


#endif /* !__GLUE__V4_AMD64__TCB_H__ */
