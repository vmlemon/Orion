/*********************************************************************
 *                
 * Copyright (C) 2002, 2004-2003,  Karlsruhe University
 *                
 * File path:     glue/v4-amd64/user.cc
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
#include <debug.h>
#include INC_API(user.h)
#include INC_API(kernelinterface.h)
#include INC_API(tcb.h)
#include INC_API(ipc.h)
#include INC_API(syscalls.h)
#include INC_ARCH(trapgate.h)

extern "C" void user_nop() {
    __asm__ (
	"	syscall			\n"
	"	.global user_nop_entry	\n"
	"user_nop_entry:		\n"
	);
}

#define OFS_USER_UTCB_MYGLOBAL  (OFS_UTCB_MY_GLOBAL_ID - OFS_UTCB_MR)
#define OFS_USER_UTCB_PROC      (OFS_UTCB_PROCESSOR_NO - OFS_UTCB_MR)
extern "C" void user_ipc() 
{
    __asm__ (
	"	.global user_lipc		\n"
	"user_lipc:				\n"
	"       test    $0x3f,%%si		\n" // check if to=local/nilthread
        "       je      3f			\n" // jmp if so
	"1:     test    $0x3f,%%dl	        \n" // check if from=local/nilthread/anylocal
        "       je      4f			\n" // jmp if so   
	"2:     popq    %%rdi			\n" // save UIP in rdi (clobber unneeded UTBC)
	"	syscall				\n"
	"	.global user_ipc_entry		\n"
	"user_ipc_entry:			\n"
	"3:     testq   %%rsi, %%rsi		\n" // check if to=nilthread
	"       je      1b              	\n" // jmp if so
	"       movq    %c0(%%rsi),%%rsi	\n" // load global id of to
	"       test    $0x3f, %%dl     	\n" // check if from=local
	"       jne     2b              	\n" // jmp if not
	"4:     testq   %%rdx, %%rdx    	\n" // check if from=nilthread
	"       je      2b              	\n" // jmp if so
	"       movq    %%rdx, %%r9     	\n" // check if from=anylocal
	"       addq    %c1, %%r9	      	\n" 
	"       jz      2b			\n" // jmp if so
	"       movq    %c0(%%rdx),%%rdx	\n" // load global fromspec
	"       jmp     2b			\n" // jmp if so
	:
	: "i"(OFS_USER_UTCB_MYGLOBAL), "i"(1 << L4_LOCAL_ID_ZERO_BITS)
	);
}



extern "C" void user_exchange_registers()
{
    __asm__ __volatile__ ("syscall\n");
}


extern "C" void user_thread_switch()
{
    __asm__ __volatile__ ("syscall\n");
}

extern "C" void user_schedule()
{
    __asm__ __volatile__ ("syscall\n");
}

extern "C" void user_unmap()
{
    __asm__ __volatile__ (
	"movq	 %r9,  (%rdi)			\n"	// save mr0
	"movq	%rax, 8(%rdi)			\n"	// save mr1
	"movq	%rbx, 16(%rdi)			\n"	// save mr2
	"movq	%r10, 24(%rdi)			\n"	// save mr3
	"movq	%r12, 32(%rdi)			\n"	// save mr4
	"movq	%r13, 40(%rdi)			\n"	// save mr5
	"movq	%r14, 48(%rdi)			\n"	// save mr6
	"movq	%r15, 56(%rdi)			\n"	// save mr7
	"syscall\n"		
	"movq      %rax, %rdi			\n"	// restore UTCB
	"movq	  (%rdi),  %r9			\n"	// restore mr0
	"movq	 8(%rdi), %rax			\n"	// restore mr1
	"movq	16(%rdi), %rbx   		\n"	// restore mr2
	"movq	24(%rdi), %r10   		\n"	// restore mr3
	"movq	32(%rdi), %r12   		\n"	// restore mr4
	"movq	40(%rdi), %r13   		\n"	// restore mr5
	"movq	48(%rdi), %r14   		\n"	// restore mr6
	"movq	56(%rdi), %r15   		\n"	// restore mr7
	);
}

extern "C" void user_thread_control()
{
    __asm__ __volatile__ ("syscall\n");
}

extern "C" void user_space_control()
{
    __asm__ __volatile__ ("syscall\n");
}

extern "C" void user_processor_control()
{
    __asm__ __volatile__ ("syscall\n");
}

extern "C" void user_memory_control()
{
    __asm__ __volatile__ ("syscall\n");
}

