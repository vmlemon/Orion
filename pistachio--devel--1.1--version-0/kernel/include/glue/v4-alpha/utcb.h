/*********************************************************************
 *                
 * Copyright (C) 2002-2003,  University of New South Wales
 *                
 * File path:     glue/v4-alpha/utcb.h
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
 * $Id: utcb.h,v 1.4 2003/09/24 19:04:34 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __GLUE__V4_ALPHA__UTCB_H__
#define __GLUE__V4_ALPHA__UTCB_H__

/* On ev6, the L1 cache size is 64 bytes, on ev4 and ev5m size is 32 bytes */

class utcb_t
{
public:
    bool allocate(threadid_t tid);
    void free() {this->my_global_id = NILTHREAD;}

public:
    threadid_t		my_global_id;		/* 0 */
    word_t		processor_no;		/* 8 */
    word_t		user_defined_handle;	/* 16 */
    threadid_t		pager;			/* 24 */
    threadid_t		exception_handler;	/* 32 */
    u8_t		preempt_flags;		/* 40 */
    u8_t		cop_flags;              /* 41 */
    u16_t		__reserved0;           
    u32_t               __reserved1;
    acceptor_t          acceptor;    		/* 48 */
    word_t              notify_mask;		/* 56 */
    word_t		error_code;		/* 64 */
    threadid_t		intended_receiver;	/* 72 */
    threadid_t		virtual_sender;		/* 80 */
    word_t		preempt_callback_ip;	/* 88 */
    word_t		preempted_ip;		/* 96 */
    word_t              __padding0[3];          /* 104 .. 127 */
    word_t		mr[IPC_NUM_MR];		/* 128 .. 639 */
    word_t              __padding1[48];         /* 640 .. 1023 */
} __attribute__((packed));

/**
 * try to allocate a UTCB and mark it as used
 * @param tid the global ID of the associated thread
 * @return true on success, false if UTCB is already allocated
 */
INLINE bool utcb_t::allocate( threadid_t tid )
{
#if defined(CONFIG_SMP)
#warning utcb_t::allocate not SMP safe
#endif
    if(this->my_global_id.is_nilthread()) {
    	this->my_global_id.set_raw(tid.get_raw());
	return true;
    } else
	return false;
}

#endif /* !__GLUE__V4_ALPHA__UTCB_H__ */
