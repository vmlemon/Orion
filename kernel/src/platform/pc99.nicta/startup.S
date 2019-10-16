/*********************************************************************
 *                
 * Copyright (C) 2002, 2004-2003,  Karlsruhe University
 *                
 * File path:     platform/pc99/startup.S
 * Description:   startup file for pc99 platform
 *		  initializes segments, invokes init_paging and
 *		  startup_system
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
 * $Id: startup.S,v 1.14 2004/05/31 14:19:52 stoess Exp $
 *                
 ********************************************************************/

	.globl	_start
	.globl	start
	.section .init.startup32

start:	
_start:
	cli	/* disable int's */
	cld	/* clear direction flag */
	mov	%ds, %ax
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	mov	%ax, %ss

	/* load temporary stack pointer */
	lea	_mini_stack-4, %esp
	/* set up paging */
	call	init_paging
	nop

	/* still running in 1:1 mapped area
	   jump to the top level init function */ 
	jmp	startup_system


        .align  16
	.space	1024
_mini_stack:
	.section .mb_header
	.align	4
_mb_header:
        .long   0x1BADB002			/* magic */
        .long   0x00000000			/* flags */
        .long   - 0x00000000 - 0x1BADB002	/* check */

	
