/*********************************************************************
 *                
 * Copyright (C) 2002-2003,  Karlsruhe University
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *                
 * File path:     api/v4/ipc.h
 * Description:   IPC declarations
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
 * $Id: ipc.h,v 1.22 2003/09/24 19:04:24 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __API__V4__IPC_H__
#define __API__V4__IPC_H__

#include INC_API(fpage.h)
#include <debug.h>

class tcb_t;


/**
 * Error codes
 */
#define ERR_IPC_NOPARTNER		(1)
#define ERR_IPC_NON_EXISTING		(2)
#define ERR_IPC_CANCELED		(3)
#define ERR_IPC_MSG_OVERFLOW		(4)
#define ERR_IPC_NOT_ACCEPTED		(5)
#define ERR_IPC_ABORTED			(7)

/**
 * Error encoding
 */
#define IPC_SND_ERROR(err)		((err << 1) | 0)
#define IPC_RCV_ERROR(err)		((err << 1) | 1)

/**
 * MR0 values
 */
#define IPC_MR0_PROPAGATED		(1 << 12)
#define IPC_MR0_REDIRECTED		(1 << 13)
#define IPC_MR0_XCPU			(1 << 14)
#define IPC_MR0_ERROR			(1 << 15)
#define IPC_MR0_PAGEFAULT		(-2UL << 4)


class msg_tag_t 
{
public:
    word_t get_label() { return send.label; }
    word_t get_typed() { return send.typed; }
    word_t get_untyped() { return send.untyped; }

    void clear_flags() { raw &= ~(0xf << 12);}
    void clear_receive_flags() { raw &= ~(0xe << 12); }

    void set(word_t typed, word_t untyped, word_t label,
	    bool send_blocks, bool recv_blocks)
	{
	    this->raw = 0;
	    this->send.typed = typed;
	    this->send.untyped = untyped;
	    this->send.label = label;
	    this->send.rcvblock = recv_blocks;
	    this->send.sndblock = send_blocks;
	}

    bool recv_blocks() { return send.rcvblock; }
    void set_recv_blocks(bool val = true) { send.rcvblock = val; }

    bool send_blocks() { return send.sndblock; }
    void set_send_blocks(bool val = true) { send.sndblock = val; }

    bool is_notify() { return send.notify; }

    bool is_error() { return recv.error; }
    void set_error() { recv.error = 1; }

    bool is_redirected() { return recv.redirected; }
    void set_redirected() { recv.redirected = 1; }

    bool is_propagated() { return send.propagated; }
    void set_propagated(bool val = true) { recv.propagated = val; }

    bool is_xcpu() { return recv.xcpu; }
    void set_xcpu() { recv.xcpu = 1; }

    static msg_tag_t nil_tag() 
	{ 
	    msg_tag_t tag; 
	    tag.raw = 0; 
	    return tag;
	}

    static msg_tag_t error_tag() 
	{ 
	    msg_tag_t tag; 
	    tag.raw = 0; 
	    tag.set_error(); 
	    return tag;
	}

    static msg_tag_t tag(word_t typed, word_t untyped, word_t label,
	    bool send_blocks, bool recv_blocks)
	{
	    msg_tag_t tag;
	    tag.set(typed, untyped, label, send_blocks, recv_blocks);
	    return tag;
	}

    static msg_tag_t irq_tag()
	{
	    /* this is an output parameter */
	    return tag(0, 0, -1UL << 4, false, false);
	}

    static msg_tag_t notify_tag()
	{
	    return tag (0, 1, 0, false, false);
	}

    static msg_tag_t preemption_tag()
	{
	    return tag (0, 0, -3UL << 4, true, true);
	}

    static msg_tag_t pagefault_tag(bool read, bool write, bool exec)
	{
	    return tag (0, 2, (-2UL << 4) | 
			(read  ? 1 << 2 : 0) | 
			(write ? 1 << 1 : 0) | 
			(exec  ? 1 << 0 : 0),
			true, true);
	}
public:
    union {
	word_t raw;
	struct {
	    BITFIELD7(word_t,
		      untyped		: 6,
		      typed		: 6,
		      propagated	: 1,
		      notify		: 1,
		      rcvblock		: 1,
		      sndblock		: 1,
		      label		: BITS_WORD - 16);
	} send;
	struct {
	    BITFIELD7(word_t,
		      untyped		: 6,
		      typed		: 6,
		      propagated	: 1,
		      redirected	: 1,
		      xcpu	       	: 1,
		      error		: 1,
		      label		: BITS_WORD - 16);
	} recv;
    };
};

INLINE msg_tag_t msgtag (word_t rawtag)
{
    msg_tag_t t;
    t.raw = rawtag;
    return t;
}

class msg_item_t
{
public:
    inline bool is_map_item() 
	{ return x.type == 8; }

    inline bool is_grant_item() 
	{ return x.type == 10; }

    inline word_t get_snd_base()
	{ return raw & (~0x3ff); }

    inline void operator = (word_t raw) 
	{ this->raw = raw; }
public:
    union {
	word_t raw;
	struct {
	    BITFIELD4(word_t,
		      type		: 4,
		      num_ptrs		: 5,
		      continuation	: 1,
		      length		: BITS_WORD - 10);
	} x;
    };
};

class acceptor_t
{
public:
    inline void clear()
	{ this->raw = 0; }

    inline void operator = (word_t raw) 
	{ this->raw = raw; }

    inline bool accept_notify()
	{ return x.notify; }

    inline word_t get_rcv_window()
	{ return x.rcv_window << 4; }

    inline void set_rcv_window(fpage_t fpage)
	{ x.rcv_window = (fpage.raw >> 4); };

    static inline acceptor_t untyped_words()
	{
	    acceptor_t x;
	    x.raw = 0;
	    return x;
	}
public:
    union {
	word_t raw;
	struct {
	    BITFIELD4(word_t,
		      _res1		: 1,
		      notify		: 1,
		      _res2		: 2,
		      rcv_window	: BITS_WORD - 4);
	} x;
    };
};


/**
 * Implements all non-untyped transfers
 *
 * @param src		Source tcb the message is sent from
 * @param dst		Destination tcb the message is sent to
 * @param msgtag	Message tag of the message being transferred
 *
 * @returns the message tag
 */
msg_tag_t extended_transfer(tcb_t * src, tcb_t * dst, msg_tag_t msgtag);

#endif /* !__API__V4__IPC_H__ */
