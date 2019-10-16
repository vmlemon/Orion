/*********************************************************************
 *
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *
 * File path:     glue/v4-arm/user.S
 * Description:   Kernel entry points for syscalls
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
 * $Id: user.S,v 1.19 2004/12/01 23:57:01 cvansch Exp $
 *
 ********************************************************************/

#include <l4.h>
#include INC_ARCH(asm.h)
#include INC_ARCH(thread.h)
#include INC_GLUE(syscalls.h)

#include <asmsyms.h>

#define SYSCALL_UNIMPLEMENTED(name) \
.section    .user.##name;			\
BEGIN_PROC_KIP(user_##name);			\
        mov   pc, lr;				\
END_PROC_KIP(user_##name)

#define SYSCALL(name)				\
.section	.user.##name;			\
BEGIN_PROC_KIP(user_##name);			\
	mov	ip,	sp;			\
	mov	sp,	#(0xffffff00 + SYSCALL_##name);	\
	swi	0x141414;			\
END_PROC_KIP(user_##name)

#define SYSCALL_MRS(name, last_reg)			\
.section	.user.##name;			\
BEGIN_PROC_KIP(user_##name);			\
	mov	r2,	#0xff000000;		\
	sub	ip,	sp,	#8;		\
	ldr	r2,	[r2, #0xff0];		\
	add	r2,	r2,	#OFS_UTCB_MR0;	\
	stmia	r2,	{r3-##last_reg};	\
	str	lr,	[sp, #-4];		\
	str	r2,	[sp, #-8];		\
	mov	sp,	#(0xffffff00 + SYSCALL_##name);	\
	mov	lr,	pc;			\
	swi	0x141414;			\
	ldr	ip,	[sp],	#4;		\
	ldr	lr,	[sp],	#4;		\
	ldmia	ip,	{r3-##last_reg};	\
	mov	pc,	lr;			\
END_PROC_KIP(user_##name)

SYSCALL(ipc)
SYSCALL(thread_switch)
SYSCALL(schedule)
SYSCALL(memory_control)
SYSCALL(exchange_registers)
SYSCALL(thread_control)
SYSCALL(space_control)
SYSCALL_MRS(unmap, r8)
SYSCALL_MRS(lipc, r8)

/* ProcessorControl currently unimplemented */

SYSCALL_UNIMPLEMENTED(processor_control)

	.equ	kip_code,	0xff000000
	.type	kip_code,	function
	.size	kip_code,	0x1000
