/*********************************************************************
 *                
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *                
 * File path:     arch/arm/asm.h
 * Description:   Assembler macros etc. 
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
 * $Id: asm.h,v 1.4 2004/12/02 00:15:07 cvansch Exp $
 *                
 ********************************************************************/

#ifndef __ARCH__ARM__ASM_H__
#define __ARCH__ARM__ASM_H__

#define BEGIN_PROC(name)			\
    .global name; 				\
    .type   name,function;			\
    .align;					\
name:

#define END_PROC(name)				\
.fend_##name:					\
    .size   name,.fend_##name - name;

/* Functions to generate symbols in the output file
 * with correct relocated address for debugging
 */
#define	TRAPS_BEGIN_MARKER			\
    .balign 4096;				\
    .section .data.traps;			\
    __vector_vaddr:

#define VECTOR_WORD(name)			\
    .equ    vector_##name, (name - __vector_vaddr + 0xffff0000);	\
    .global vector_##name;			\
    .type   vector_##name,object;		\
    .size   vector_##name,4;			\
name:

#define BEGIN_PROC_TRAPS(name)			\
    .global name; 				\
    .type   name,function;			\
    .equ    vector_##name, (name - __vector_vaddr + 0xffff0000);	\
    .global vector_##name;			\
    .type   vector_##name,function;		\
    .align;					\
name:

#define END_PROC_TRAPS(name)			\
.fend_##name:					\
    .size   name,.fend_##name - name;		\
    .equ    .fend_vector_##name, (.fend_##name - __vector_vaddr + 0xffff0000);	\
    .size   vector_##name,.fend_vector_##name - vector_##name;

#define BEGIN_PROC_KIP(name)			\
    .global name; 				\
    .type   name,function;			\
    .align;					\
name:

#define END_PROC_KIP(name)			\
.fend_##name:					\
    .size   name,.fend_##name - name;

#define CHECK_ARG(a, b)				\
	"   .ifnc " a ", " b "	\n"		\
	"   .err		\n"		\
	"   .endif		\n"		\

#endif /* __ARCH__ARM__ASM_H__ */
