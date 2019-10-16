/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  Karlsruhe University
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *                
 * File path:     api/v4/ipcx.cc
 * Description:   Extended transfer of IPC
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
 * $Id: ipcx.cc,v 1.23 2004/11/23 14:32:14 joshua Exp $
 *                
 ********************************************************************/
#include <l4.h>
#include <debug.h>
#include <kdb/tracepoints.h>
#include INC_API(tcb.h)
#include INC_API(ipc.h)
#include INC_API(fpage.h)
#include INC_API(schedule.h)

#define CHECK_MR_IDX(idx, total) if (idx > total) goto message_overflow


DECLARE_TRACEPOINT(IPC_MAPGRANT_ITEM);
DECLARE_TRACEPOINT(IPC_MESSAGE_OVERFLOW);
DECLARE_TRACEPOINT(IPC_EXT_TRANSFER);

msg_tag_t extended_transfer(tcb_t * src, tcb_t * dst, msg_tag_t msgtag)
{
    msg_item_t src_item;
    acceptor_t acceptor;
    word_t total_mrs = msgtag.get_untyped() + msgtag.get_typed();

//    ENABLE_TRACEPOINT (IPC_MESSAGE_OVERFLOW, false);
#undef TRACEF
#define TRACEF(args...)
    
    if (total_mrs > (IPC_NUM_MR-1))
    {
	TRACEF("message exceeds MR's (untyped=%d, typed=%d)\n", 
	    msgtag.get_untyped(), msgtag.get_typed());
//	enter_kdebug("message exceeds MR's");
	goto message_overflow;
    }

    if (msgtag.is_redirected())
    {
	src->copy_mrs(dst, 1 + msgtag.get_untyped(), msgtag.get_typed());
    	return msgtag;
    }

    src->set_state (thread_state_t::locked_running);
    dst->set_state (thread_state_t::locked_waiting);

    src->set_partner (dst->get_global_id ());

    acceptor = dst->get_acceptor();
    
    TRACEPOINT(IPC_EXT_TRANSFER,
	printf("tag=%p, untyped: %d, typed: %d, acceptor: %x\n", 
	   msgtag.raw, msgtag.get_untyped(), msgtag.get_typed(), acceptor));

    for (word_t src_idx = msgtag.get_untyped() + 1; src_idx < total_mrs; )
    {
	src_item = src->get_mr(src_idx);

	if (src_item.is_map_item() || src_item.is_grant_item())
	{
	    /* is the descriptor beyond the valid range? */
	    CHECK_MR_IDX(src_idx + 1, total_mrs);
	    
	    fpage_t snd_fpage, rcv_fpage;

	    rcv_fpage.raw = acceptor.get_rcv_window();
	    snd_fpage.raw = src->get_mr(src_idx + 1);

	    TRACEPOINT(IPC_MAPGRANT_ITEM,
		printf("%s item: snd_base=%p, fpage=%p\n",
		    src_item.is_map_item() ? "map" : "grant",
		    src_item.get_snd_base(), snd_fpage.raw));
	    
	    /* does the receiver accept mappings */
	    if (EXPECT_FALSE( rcv_fpage.is_nil_fpage() ))
		goto message_overflow;

	    src->copy_mrs(dst, src_idx, 2);
	    src_idx += 2;

	    if (EXPECT_FALSE( ! src->get_space()->map_fpage(snd_fpage, 
			    src_item.get_snd_base(), dst->get_space(),
			    rcv_fpage, src_item.is_grant_item())))
		goto message_overflow;
	}
       	else 
	{
	    TRACEF("unknown item type\n");
	    goto message_overflow;
	}
    }

    return msgtag;

message_overflow:

    TRACEPOINT (IPC_MESSAGE_OVERFLOW,
		printf("IPC message overflow (%t->%t)\n",
		       src, dst));

    // Report message overflow error
    dst->set_error_code (IPC_RCV_ERROR (ERR_IPC_MSG_OVERFLOW));
    src->set_error_code (IPC_SND_ERROR (ERR_IPC_MSG_OVERFLOW));
    msgtag.set_error();
    return msgtag;
}
