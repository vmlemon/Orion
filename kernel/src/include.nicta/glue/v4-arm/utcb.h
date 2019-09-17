/*********************************************************************
 *                
 * Copyright (C) 2003,  National ICT Australia (NICTA)
 *                
 * File path:     glue/v4-arm/utcb.h
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
 * $Id: utcb.h,v 1.3 2004/06/04 02:26:52 cvansch Exp $
 *                
 ********************************************************************/
#ifndef __GLUE__V4_ARM__UTCB_H__
#define __GLUE__V4_ARM__UTCB_H__

#include INC_API(thread.h)
#include INC_API(types.h)

class utcb_t {
public:
    bool allocate(threadid_t tid);
    void free();

public:
    threadid_t		my_global_id;		/* 0 */
    word_t		user_defined_handle;	/* 4 */
    threadid_t		pager;			/* 8 */
    threadid_t		exception_handler;	/* 12 */
    struct {
	    BITFIELD3( word_t,
		      preempt_flags : 8,	/* 16 */
		      cop_flags	    : 8,	/* 17 */
		      __reserved0   : 16 )	/* 18 */
    };
    acceptor_t		acceptor;		/* 20 */
    word_t		notify_mask;		/* 24 */
    word_t		notify_bits;		/* 28 */
    word_t		processor_no;		/* 32 */
    word_t		error_code;		/* 36 */
    threadid_t		intended_receiver;	/* 40 */
    threadid_t		virtual_sender;		/* 44 */
    word_t		preempt_callback_ip;	/* 48 */
    word_t		preempted_ip;		/* 52 */
    /* Reserved for future kernel use */
    word_t		kernel_reserved[2];	/* 56 .. 63 */
    
    word_t		mr[IPC_NUM_MR];		/* 64 .. 191 */

    /* Reserved for future platform specific use */
    word_t		platform_reserved[6];	/* 192 .. 215 */
    /* Reserved for future user specific use */
    word_t		user_reserved[10];	/* 216 .. 255 */
};

#endif /* !__GLUE__V4_ARM__UTCB_H__ */
