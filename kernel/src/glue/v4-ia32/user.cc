/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  Karlsruhe University
 *                
 * File path:     glue/v4-ia32/user.cc
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
 * $Id: user.cc,v 1.29 2004/09/15 17:53:47 jdoll Exp $
 *                
 ********************************************************************/
#include <l4.h>
#include <debug.h>
#include <asmsyms.h>
#include <tcb_layout.h>
#include INC_API(user.h)
#include INC_API(kernelinterface.h)
#include INC_API(tcb.h)
#include INC_API(ipc.h)
#include INC_API(syscalls.h)

#include INC_GLUE(timer.h)

#include INC_ARCH(trapgate.h)

#define OFS_USER_UTCB_MYGLOBAL	(OFS_UTCB_MY_GLOBAL_ID - OFS_UTCB_MR)
#define OFS_USER_UTCB_PROC	(OFS_UTCB_PROCESSOR_NO - OFS_UTCB_MR)

#define SYS_IPC_NUM			0x30
#define SYS_SYSCALL_NUM			0x31


#define SYSCALL_STUB(name)		\
    extern "C" void entry_##name ();	\
    extern "C" void user_##name ()
    

#define SYSCALL_LABEL(name)		\
    "	.global entry_"#name"	\n"	\
    "entry_"#name":		\n"	\
    "	int	%0		\n"


extern "C" void user_ipc() 
{
    __asm__ (
	"	popl	%%ebx		\n"	// get return address
#if defined(CONFIG_IA32_SMALL_SPACES) && defined(CONFIG_IA32_SYSENTER)
	"	pushl	%1		\n"
	"	pushl	%%ebx		\n"
#endif
#if defined (CONFIG_IA32_SYSENTER)
	"	movl	%%esp, %%ebp	\n"	// store return stack pointer
	"	sysenter		\n"
#else
	"	int	 %0		\n"
#endif
	: 
	:
	"i"(SYS_IPC_NUM),
	"i"(IA32_UCS));
}

extern "C" void user_lipc() 
{
    __asm__ (
	"	popl	%%ebx		\n"	// get return address
#if defined(CONFIG_IA32_SMALL_SPACES) && defined(CONFIG_IA32_SYSENTER)
	"	pushl	%1		\n"
	"	pushl	%%ebx		\n"
#endif
#if defined (CONFIG_IA32_SYSENTER)
	"	movl	%%esp, %%ebp	\n"	// store return stack pointer
	"	sysenter		\n"
#else
	"	int	 %0		\n"
#endif
	: 
	:
	"i"(SYS_IPC_NUM),
	"i"(IA32_UCS));
}

SYSCALL_STUB(exchange_registers)
{
    __asm__ __volatile__ (
	"	test	$0x3f,%%al		\n"
	"	jne	1f			\n"
	"	movl	-60(%%eax),%%eax	\n"
	"1:					\n"
	SYSCALL_LABEL (exchange_registers)
	:
	: "i"(SYS_SYSCALL_NUM));

}

SYSCALL_STUB(thread_switch)
{
    __asm__ __volatile__ (
	"	test	%%eax,%%eax		\n"
	"	je	1f			\n"
	"	test	$0x3f,%%al		\n"
	"	jne	1f			\n"
	"	movl	-60(%%eax),%%eax	\n"
	"1:					\n"
	SYSCALL_LABEL (thread_switch)
	:
	: "i"(SYS_SYSCALL_NUM));
}

SYSCALL_STUB(schedule)
{
    __asm__ __volatile__ (
	"	test	$0x3f,%%al		\n"
	"	jne	1f			\n"
	"	movl	-60(%%eax),%%eax	\n"
	"1:					\n"
	SYSCALL_LABEL (schedule)
	:
	: "i"(SYS_SYSCALL_NUM));
}

SYSCALL_STUB(unmap)
{
    __asm__ __volatile__ (
	"	movl	%%esi, (%%edi)	\n"	// store MR0 into UTCB
	SYSCALL_LABEL (unmap)
	"	movl	(%%edi), %%esi	\n"	// load MR0 from UTCB
	:
	: "i"(SYS_SYSCALL_NUM));
}

SYSCALL_STUB(thread_control)
{
    __asm__ __volatile__ (
	"	test	%%ecx,%%ecx		\n"
	"	je	1f			\n"
	"	test	$0x3f,%%cl		\n"
	"	jne	1f			\n"
	"	movl	-60(%%ecx),%%ecx	\n"
	"1:	test	%%edx,%%edx		\n"
	"	je	1f			\n"
	"	test	$0x3f,%%dl		\n"
	"	jne	1f			\n"
	"	movl	-60(%%edx),%%edx	\n"
	"1:	test	%%esi,%%esi		\n"
	"	je	1f			\n"
	"	test	$0x3f,%%esi		\n"
	"	jne	1f			\n"
	"	movl	-60(%%esi),%%esi	\n"
	"1:					\n"
	SYSCALL_LABEL (thread_control)
	:
	: "i"(SYS_SYSCALL_NUM));

}

SYSCALL_STUB(space_control)
{
    __asm__ __volatile__(
	SYSCALL_LABEL (space_control)
	:
	: "i"(SYS_SYSCALL_NUM));
}

SYSCALL_STUB(processor_control)
{
    __asm__ __volatile__(
	SYSCALL_LABEL (processor_control)
	:
	: "i"(SYS_SYSCALL_NUM));
}

SYSCALL_STUB(memory_control)
{
    __asm__ __volatile__(
	"	movl	%%esi, (%%edi)	\n"	// store MR0 into UTCB
	SYSCALL_LABEL (memory_control)
	:
	: "i"(SYS_SYSCALL_NUM));
}

#define IS_SYSCALL(x) ((frame->eip - (u32_t)get_current_space()->get_kip_page_area().get_base()) == ((u32_t)(&entry_##x) - (u32_t)get_kip() + 2))

IA32_EXC_NO_ERRORCODE(exc_user_syscall, 0)
{
    /* syscalls are dispatched by IP */
    if (IS_SYSCALL(exchange_registers))
    {
	sys_exchange_registers(threadid(frame->eax),
			       frame->ecx, 
			       frame->edx, frame->esi,
			       frame->edi, frame->ebx,
			       threadid(frame->ebp), 
			       frame);
	return;
    } 

    else if (IS_SYSCALL(thread_switch))
    {
	sys_thread_switch(threadid(frame->eax));
    }

    else if (IS_SYSCALL(unmap))
    {
	sys_unmap(frame->eax);
    }

    else if ( IS_SYSCALL(schedule) )
    {
	sys_schedule(threadid(frame->eax), 
		     frame->ecx, frame->edx, 
		     frame->esi, frame->edi,
		     frame);
    }

    else if (IS_SYSCALL(thread_control))
    {
	sys_thread_control(threadid(frame->eax), // dest
			   threadid(frame->esi), // space
			   threadid(frame->edx), // sched
			   threadid(frame->ecx), // pager
			   threadid(frame->edi), // sendRedirector
			   threadid(frame->ebx), // recvRedirector
			   frame->ebp, frame);
    }

    else if (IS_SYSCALL(space_control))
    {
	sys_space_control(threadid(frame->eax),		  // dest
			  frame->ecx,			  // control
			  (fpage_t){{ raw: frame->edx }}, // kip
			  (fpage_t){{ raw: frame->esi }}, // utcb
			  frame);
    }

    else if ( IS_SYSCALL(memory_control) )
    {
    }

    else
	printf("unknown syscall\n");
    return;
}
