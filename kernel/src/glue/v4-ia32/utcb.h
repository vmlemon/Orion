/*********************************************************************
 *                
 * Copyright (C) 2002-2003,  Karlsruhe University
 *                
 * File path:     glue/v4-ia32/utcb.h
 * Description:   UTCB for IA32
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
 * $Id: utcb.h,v 1.13 2003/09/24 19:04:36 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __GLUE__V4_IA32__UTCB_H__
#define __GLUE__V4_IA32__UTCB_H__

class utcb_t
{
public:
    /* do not delete this TCB_START_MARKER */

    word_t		padding0[16];		/* -256 .. -196 */
    word_t		padding1[32];		/* -192 .. -68	*/
    threadid_t		my_global_id;		/* -64		*/
    word_t		notify_mask;		/* -60		*/
    word_t		notify_bits;		/* -56		*/
    acceptor_t		acceptor;		/* -52		*/
    word_t		processor_no;		/* -48		*/
    word_t		user_defined_handle;	/* -44		*/
    threadid_t		pager;			/* -40		*/
    threadid_t		exception_handler;	/* -36		*/
    u8_t		preempt_flags;		/* -32		*/
    u8_t		cop_flags;
    u16_t		reserved0;
    word_t		error_code;		/* -28		*/
    threadid_t		intended_receiver;	/* -24		*/
    threadid_t		virtual_sender;		/* -20		*/
    word_t		preempt_callback_ip;	/* -16		*/
    word_t		preempted_ip;		/* -12		*/
    word_t		reserved1[2];		/* -8 .. -4	*/
    word_t		mr[IPC_NUM_MR];		/* 0 .. 252	*/

    /* do not delete this TCB_END_MARKER */
};

#endif /* !__GLUE__V4_IA32__UTCB_H__ */
