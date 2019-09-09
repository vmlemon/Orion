/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  University of New South Wales
 *                
 * File path:     glue/v4-mips64/utcb.h
 * Description:   UTCB definition
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
 * $Id: utcb.h,v 1.10 2004/06/04 02:32:31 cvansch Exp $
 *                
 ********************************************************************/
#ifndef __GLUE__V4_MIPS64__UTCB_H__
#define __GLUE__V4_MIPS64__UTCB_H__

#include INC_API(thread.h)
#include INC_API(types.h)

class utcb_t {
public:
    bool allocate(threadid_t tid);
    void free();

public:
    threadid_t		my_global_id;		/* 0 */
    word_t		processor_no;		/* 8 */
    threadid_t		pager;			/* 16 */
    threadid_t		exception_handler;	/* 24 */
    u8_t		preempt_flags;		/* 32 */
    u8_t		cop_flags;		/* 33 */
    u16_t		__reserved0;		/* 34 */
    u32_t		__reserved1;		/* 36 */
    acceptor_t		acceptor;		/* 40 */
    word_t		notify_mask;		/* 48 */
    word_t		notify_bits;		/* 56 */
    word_t		preempt_callback_ip;	/* 64 */
    word_t		preempted_ip;		/* 72 */
    word_t		user_defined_handle;	/* 80 */
    word_t		error_code;		/* 88 */
    threadid_t		intended_receiver;	/* 96 */
    threadid_t		virtual_sender;		/* 104 */
    word_t		__thread_words[2];	/* 112 .. 127 */
    word_t		mr[IPC_NUM_MR];		/* 128 .. 639 */
    word_t		__padding1[48];		/* 640 .. 1023 */
//}__attribute__((packed));
};

#endif /* !__GLUE__V4_MIPS64__UTCB_H__ */
