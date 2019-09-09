/*********************************************************************
 *
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *
 * File path:     glue/v4-arm/user_thumb.S
 * Description:   Kernel entry points for thumb mode syscalls
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

	.force_thumb

#define SYSCALL_UNIMPLEMENTED(name)		\
.section    .user_thumb.##name;			\
BEGIN_PROC_KIP(user_thumb_##name);		\
        mov   pc, lr;				\
END_PROC_KIP(user_thumb_##name)

#define SYSCALL_IPC(name)				\
.section	.user_thumb.##name;		\
BEGIN_PROC_KIP(user_thumb_##name);		\
	mov	ip,	sp;			\
	mov	r2,	#(~(0xffffff00 + SYSCALL_##name));	\
	mvn	r2,	r2;			\
	mov	sp,	r2;			\
	swi	0x14;				\
END_PROC_KIP(user_thumb_##name)

#define SYSCALL(name)				\
.section	.user_thumb.##name;		\
BEGIN_PROC_KIP(user_thumb_##name);		\
	mov	ip,	sp;			\
	mov	r7,	#(~(0xffffff00 + SYSCALL_##name));	\
	mvn	r7,	r7;			\
	mov	sp,	r7;			\
	swi	0x14;				\
END_PROC_KIP(user_thumb_##name)

#define SYSCALL_MRS(name)			\
.section	.user_thumb.##name;		\
BEGIN_PROC_KIP(user_thumb_##name);		\
	ldr	r2,	=0xff000ff0;		\
	ldr	r2,	[r2];			\
	add	r2,	r2,	#64;		\
	push	{r2, lr};			\
	stmia	r2!,	{r3-r7};		\
	mov	r3,	r8;			\
	str	r3,	[r2];			\
	ldr	r2,	=(0xffffff00 + SYSCALL_##name);	\
	mov	ip,	sp;			\
	mov	sp,	r2;			\
	mov	lr,	pc;			\
	swi	0x14;				\
	pop	{r2};				\
	ldmia	r2!,	{r3-r7};		\
	ldr	r2,	[r2];			\
	mov	r8,	r2;			\
	pop	{pc};				\
END_PROC_KIP(user_thumb_##name)

SYSCALL_IPC(ipc)
SYSCALL_MRS(lipc)
SYSCALL(thread_switch)
SYSCALL(space_control)
SYSCALL_MRS(unmap)
SYSCALL(exchange_registers)
SYSCALL(thread_control)
SYSCALL(schedule)
SYSCALL(memory_control)

/* ProcessorControl currently unimplemented */

SYSCALL_UNIMPLEMENTED(processor_control)

