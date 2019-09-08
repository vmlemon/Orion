/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  Karlsruhe University
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *                
 * File path:     api/v4/ipc.cc
 * Description:   generic IPC path
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
 * $Id: ipc.cc,v 1.64 2004/06/02 08:41:42 sgoetz Exp $
 *                
 ********************************************************************/
#include <l4.h>
#include <debug.h>
#include <kdb/tracepoints.h>

#define DO_TRACE_IPC(x...) do { printf("CPU%d: ", get_current_cpu()); printf(x); } while(0)
#define TRACE_IPC(x...)
#define TRACE_XIPC(x...)
#define TRACE_NOTIFY(x...)

//#define TRACE_IPC	DO_TRACE_IPC
//#define TRACE_XIPC	DO_TRACE_IPC
//#define TRACE_NOTIFY	DO_TRACE_IPC

#include INC_API(tcb.h)
#include INC_API(schedule.h)
#include INC_API(ipc.h)
#include INC_API(interrupt.h)
#include INC_API(syscalls.h)
#include INC_API(smp.h)
#include INC_GLUE(syscalls.h)


DECLARE_TRACEPOINT(SYSCALL_IPC);
DECLARE_TRACEPOINT(IPC_TRANSFER);

#ifdef CONFIG_SMP
DECLARE_TRACEPOINT(XCPU_IPC_SEND);
DECLARE_TRACEPOINT(XCPU_IPC_SEND_DONE);
DECLARE_TRACEPOINT(XCPU_IPC_RECEIVE);
#endif


INLINE bool transfer_message(tcb_t * src, tcb_t * dst,
		bool redirected, threadid_t intended_receiver)
{
    TRACEPOINT (IPC_TRANSFER, 
		printf("IPC transfer message: src=%t, dst=%t\n", src, dst));

    msg_tag_t tag = src->get_tag();
    
    // clear all flags except propagation
    tag.clear_receive_flags();

    // propagation IPC
    if (EXPECT_FALSE( tag.is_propagated() ))
    {
	// now rewrite sender etc.
	dst->set_actual_sender( src->get_global_id() );
    }

    // redireceted IPC
    if (EXPECT_FALSE( redirected ))
    {
	tag.set_redirected();
	dst->set_intended_receiver(intended_receiver);
    }

    /* Architecture must make sure excess message
     * are ignored */
    if (EXPECT_TRUE(tag.get_untyped()))
	src->copy_mrs(dst, 1, tag.get_untyped());

    if (EXPECT_TRUE( !tag.get_typed() ))
    {
	dst->set_tag(tag);
	return true;
    }
    else
    {
	tag = extended_transfer(src, dst, tag);
	dst->set_tag(tag);
	return (!tag.is_error());
    }
}

INLINE void setup_notify_return(tcb_t *tcb)
{
    word_t mask = tcb->get_notify_mask();

    tcb->set_tag(msg_tag_t::notify_tag());
    tcb->set_mr(1, tcb->get_notify_bits() & mask);

    tcb->sub_notify_bits(mask);
}

#ifdef CONFIG_SMP

/**********************************************************************
 *                          SMP handlers
 **********************************************************************/

// handler functions to kick remote transfers
static void do_xcpu_receive(cpu_mb_entry_t * entry)
{
    tcb_t * from_tcb = entry->tcb;
    tcb_t * to_tcb = (tcb_t*)entry->param[0];

    TRACE_XIPC("%s from_tcb: %t, \n", __func__, from_tcb);

    // did the sender migrate meanwhile?
    if (!from_tcb->is_local_cpu())
	UNIMPLEMENTED();
    
    // still waiting for the IPC?
    if ( (from_tcb->get_state().is_polling() && (from_tcb->get_partner() == to_tcb->get_global_id()) ) ||
	 (to_tcb->get_state().is_locked_waiting() && (to_tcb->get_partner() == from_tcb->get_global_id())) )
    {
	// everything is fine -- now kick the thread
	from_tcb->set_state(thread_state_t::locked_running);
	get_current_scheduler()->enqueue_ready(from_tcb);
    }
    else
	UNIMPLEMENTED();
}

// reply function
static void do_xcpu_send_reply(cpu_mb_entry_t * entry)
{
    // the send operation can start now
    tcb_t * from_tcb = (tcb_t*)entry->tcb;
    TRACE_XIPC("%s to_tcb: %t (%x), from_tcb: %t\n",
	       __func__, entry->tcb, (word_t) entry->tcb->get_state(),
	       from_tcb);

    // we can let the thread run
    if (!from_tcb->is_local_cpu())
	UNIMPLEMENTED();

    // store status of XCPU request in TCB
    from_tcb->xcpu_status = entry->param[0];

    // and re-activate the thread
    from_tcb->set_state(thread_state_t::locked_running);
    get_current_scheduler()->enqueue_ready(from_tcb);
}

static void do_xcpu_send(cpu_mb_entry_t * entry)
{
    tcb_t * to_tcb = entry->tcb;
    tcb_t * from_tcb = (tcb_t*)entry->param[0];
    threadid_t sender_id; 
    sender_id.set_raw(entry->param[1]);

    ASSERT(NORMAL, to_tcb);
    ASSERT(NORMAL, from_tcb);
    
    TRACE_XIPC("%s to_tcb: %t (%x), from_tcb: %t\n",
	       __func__, to_tcb, (word_t)to_tcb->get_state(), from_tcb);

    // did the receiver migrate meanwhile?
    if (!to_tcb->is_local_cpu())
    {
	xcpu_request(from_tcb->get_cpu(), do_xcpu_send_reply, from_tcb, 1);
	return;
    }

    if ( to_tcb->get_state().is_waiting() &&
	 ( to_tcb->get_partner() == sender_id || 
	   to_tcb->get_partner().is_anythread() ))
    {
	// ok, still waiting --> everything is fine
	to_tcb->set_state(thread_state_t::locked_waiting);
	to_tcb->set_partner(sender_id);

	// now let the other thread run again
	xcpu_request(from_tcb->get_cpu(), do_xcpu_send_reply, from_tcb, 0);
    }
    else if (to_tcb->get_state().is_locked_waiting() &&
	     to_tcb->get_partner() == sender_id)
    {
	// ok, we are locked_waiting -- means we already issued
	// a request packet (do_xcpu_receive) -- so don't bother
	TRACE_XIPC("%t is locked_waiting for %t\n", to_tcb, TID(sender_id));
    }
    else
    {
	TRACE_XIPC("%s (not waiting anymore) to_tcb: %t (%x), from_tcb: %t\n",
		   __func__, to_tcb, (word_t)to_tcb->get_state(), from_tcb);
	xcpu_request(from_tcb->get_cpu(), do_xcpu_send_reply, from_tcb, 1);
    }
}

static void do_xcpu_send_done(cpu_mb_entry_t * entry)
{
    tcb_t * to_tcb = entry->tcb;
    threadid_t sender_id; 
    sender_id.set_raw(entry->param[0]);
    
    TRACE_XIPC("%s to_tcb: %t\n", __func__, to_tcb);

    // did the receiver migrate meanwhile?
    if (!to_tcb->is_local_cpu())
	UNIMPLEMENTED();

    if ( to_tcb->get_state().is_locked_waiting() && 
	 to_tcb->get_partner() == sender_id )
    {
	msg_tag_t tag = to_tcb->get_tag();
	tag.set_xcpu();
	to_tcb->set_tag(tag);
	to_tcb->set_state(thread_state_t::running);
	get_current_scheduler()->enqueue_ready(to_tcb);
    }
    else
    {
	TRACEF("tcb=%t, state=%s, partner=%t, %t\n", 
	       TID(to_tcb->get_global_id()), to_tcb->get_state().string(),
	       TID(to_tcb->get_partner()), TID(sender_id));
	UNIMPLEMENTED();
    }
}

#endif /* CONFIG_SMP */

/**********************************************************************
 *
 *                          IPC syscall
 *
 **********************************************************************/

SYS_IPC (threadid_t to_tid, threadid_t from_tid)
{
    tcb_t * to_tcb = NULL;
    tcb_t * from_tcb;
    tcb_t * current = get_current_tcb();
    scheduler_t * scheduler = get_current_scheduler();
    bool recv_blocks;

    TRACEPOINT_TB (SYSCALL_IPC,
		   msg_tag_t tag = current->get_tag ();
		   printf ("SYS_IPC: current: %t, to_tid: %t, "
			   "from_tid: %t, "
			   "tag: 0x%x (label=0x%x, %c%c%c%c, u=%d, t=%d)\n",
			   TID(current->get_global_id()), TID(to_tid),
			   TID(from_tid), tag.raw, tag.get_label (),
			   tag.send_blocks() ? 'S' : 's',
			   tag.recv_blocks() ? 'R' : 'r',
			   tag.is_notify() ? 'A' : 'a',
			   tag.is_propagated() ? 'P' : 'p',
			   tag.get_untyped (), tag.get_typed ()
			   ),
		   "sys_ipc %t => %t (recvfrom=%t)",
		   TID(current->get_global_id()),
		   TID(to_tid), TID(from_tid));

    /* --- send phase --------------------------------------------------- */
#if defined(CONFIG_SMP)
 send_path:
#endif

    if (! EXPECT_FALSE( to_tid.is_nilthread() ))
    {
	to_tcb = current->get_space()->get_tcb(to_tid);
	msg_tag_t tag = current->get_tag ();

	recv_blocks = tag.recv_blocks();

	/* --- check for asynchronous notification -------------------------- */
	if (EXPECT_FALSE( tag.is_notify() ))
	{
	    TRACE_NOTIFY("notify send phase curr=%t, to=%t\n", current, to_tcb);

	    if (EXPECT_FALSE( to_tcb->get_global_id() != to_tid ))
	    {
		/* specified thread id invalid */
		TRACE_NOTIFY("invalid send tid, wanted %t, but have %t\n",
			to_tid.get_raw(), to_tcb );

		current->set_error_code(IPC_SND_ERROR(ERR_IPC_NON_EXISTING));
		current->set_tag(msg_tag_t::error_tag());
		return_ipc(NILTHREAD);
	    }

	    acceptor_t acceptor = to_tcb->get_acceptor();

	    if ( EXPECT_TRUE(
			acceptor.accept_notify() &&
			current->get_send_redirector().is_anythread() &&
			to_tcb->get_recv_redirector().is_anythread() ))
	    {
		to_tcb->add_notify_bits(current->get_mr(1));

		/* check if we need to wakeup the destination */
		if (EXPECT_TRUE( (to_tcb->get_notify_bits() & to_tcb->get_notify_mask()) &&
			    ( to_tcb->get_state().is_waiting_notify() ||
			      ( to_tcb->get_partner().is_anythread() &&
				to_tcb->get_state().is_waiting() ) ) ))
		{
		    TRACE_NOTIFY("notify wakeup (to=%t)\n", to_tcb);
		    to_tcb->set_partner(NILTHREAD);
		}
		else
		{
		    /* Do no wakeup destination */
		    to_tcb = NULL;
		}
	    }
	    else
	    {
		/* destination thread not accepting notifications */
		current->set_error_code(IPC_SND_ERROR(ERR_IPC_NOT_ACCEPTED));
		current->set_tag(msg_tag_t::error_tag());
		return_ipc(NILTHREAD);
	    }
	    goto receive_phase;
	}

    	threadid_t intended_receiver = to_tid;

	TRACE_IPC("send phase curr=%t, to=%t\n", current, to_tcb);

	if (EXPECT_FALSE( to_tcb->get_global_id() != to_tid ))
	{
	    if ( !current->get_send_redirector().is_anythread() )
	    {
		threadid_t redir_tid = current->get_send_redirector();
		ASSERT(DEBUG, !redir_tid.is_nilthread());
		tcb_t *redir_tcb = current->get_space()->get_tcb(redir_tid);

		/* Any send should be redirected, even if to_tid is invalid */
		if ( redir_tcb->get_global_id() == redir_tid )
		{
		    to_tid = redir_tid;
		    to_tcb = redir_tcb;
		    goto continue_send_redirected;
		}
	    }
	    /* specified thread id invalid */
	    TRACE_IPC("invalid send tid, wanted %t, but have %t\n",
		      to_tid.get_raw(), to_tcb );
	    current->set_error_code(IPC_SND_ERROR(ERR_IPC_NON_EXISTING));
	    current->set_tag(msg_tag_t::error_tag());
	    return_ipc(NILTHREAD);
	}

	if (EXPECT_FALSE( !current->get_send_redirector().is_anythread() ))
	{
	    threadid_t redir_tid = current->get_send_redirector();
	    ASSERT(DEBUG, !redir_tid.is_nilthread());
	    tcb_t *redir_tcb = current->get_space()->get_tcb(redir_tid);

	    if ( redir_tcb->get_global_id() != redir_tid )
	    {
		/* specified thread id invalid */
		TRACE_IPC("invalid redirector tid, wanted %t, but have %t\n",
			to_tid.get_raw(), to_tcb );
		current->set_error_code(IPC_SND_ERROR(ERR_IPC_NON_EXISTING));
		current->set_tag(msg_tag_t::error_tag());
		return_ipc(NILTHREAD);
	    }

	    /* send to same space as current or send redirector is allowed */
	    if (( redir_tcb->get_space() != to_tcb->get_space() ) &&
		    ( current->get_space() != to_tcb->get_space() ))
	    {
		TRACE_IPC("send redirected from %t to %t\n", TID(to_tid), TID(redir_tid));

		to_tid = redir_tid;
		to_tcb = redir_tcb;
	    }
	}
continue_send_redirected:

	while (EXPECT_FALSE( !to_tcb->get_recv_redirector().is_anythread() ))
	{
	    threadid_t redir_tid = to_tcb->get_recv_redirector();
	    ASSERT(DEBUG, !redir_tid.is_nilthread());
	    tcb_t *redir_tcb = current->get_space()->get_tcb(redir_tid);

	    if ( redir_tcb->get_global_id() != redir_tid )
	    {
		/* specified thread id invalid */
		TRACE_IPC("invalid redirector tid, wanted %t, but have %t\n",
			to_tid.get_raw(), to_tcb );
		current->set_error_code(IPC_SND_ERROR(ERR_IPC_NON_EXISTING));
		current->set_tag(msg_tag_t::error_tag());
		return_ipc(NILTHREAD);
	    }

	    /* recv from same space as dest or recieve redirector allowed */
	    if (( redir_tcb->get_space() != current->get_space() ) &&
		    ( to_tcb->get_space() != current->get_space() ))
	    {
		TRACE_IPC("recv redirected from %t to %t\n", TID(to_tid), TID(redir_tid));

		to_tid = redir_tid;
		to_tcb = redir_tcb;
	    } else {
		break;
	    }
	}

	threadid_t sender_id = current->get_global_id();

	if (EXPECT_FALSE( tag.is_propagated() ))
	{
	    bool allow = false;
	    // propagation only allowed within same address space or valid send redirector
	    tcb_t * virt_sender = current->get_space()->get_tcb(current->get_virtual_sender());

	    if (current->get_virtual_sender() == virt_sender->get_global_id())
	    {
		if ((current->get_space() == virt_sender->get_space() ||
			    current->get_space() == to_tcb->get_space()))
		{
		    allow = true;
		}
		else
		{
		    /* check the redirector chain */
		    while (( !virt_sender->get_send_redirector().is_anythread() ))
		    {
			threadid_t redir_tid = virt_sender->get_send_redirector();
			ASSERT(DEBUG, !redir_tid.is_nilthread());
			tcb_t *redir_tcb = current->get_space()->get_tcb(redir_tid);

			if ( redir_tcb->get_global_id() != redir_tid )
			    break;

			if ( redir_tcb == current )
			{
			    allow = true;
			    break;
			}

			/* stop searching the chain if we reach the dest tcb */
			if ( redir_tcb == to_tcb )
			    break;

			virt_sender = redir_tcb;
		    }
		}
	    }

	    if (allow)
	    {
		sender_id = current->get_virtual_sender();
	    }
	    else
	    {
		tag.set_propagated(false);
		current->set_tag(tag);
	    }
	}

#ifdef CONFIG_SMP
	/* VU: set the thread state before actually checking 
	 * the partner. Allows for concurrent checks on SMP */
	current->set_partner(to_tid);
	current->set_state(thread_state_t::polling);
#endif

	// not waiting || (not waiting for me && not waiting for any && not waiting for anylocal)
	// optimized for receive and wait any
	if (EXPECT_FALSE(
		    !to_tcb->get_state().is_waiting() ||
		    ( to_tcb->get_partner() != sender_id &&
		      !to_tcb->get_partner().is_anythread() &&
		      !( to_tcb->get_partner().is_anylocalthread() && 
			  to_tcb->get_space() == current->get_space() ) ) ))
	{
	    TRACE_IPC ("dest not ready (%t, is_wt=%d)\n", 
		    to_tcb, to_tcb->get_state().is_waiting());
	    //enter_kdebug("blocking send");

	    /* thread is not receiving */
	    if (EXPECT_FALSE( !tag.send_blocks() ))
	    {
		current->set_tag(msg_tag_t::error_tag());
		current->set_error_code(IPC_SND_ERROR(ERR_IPC_NOPARTNER));
		return_ipc(NILTHREAD);
	    }
	    to_tcb->lock();
	    current->enqueue_send(to_tcb);
	    to_tcb->unlock();
	    current->set_partner(to_tid);
	    current->set_state(thread_state_t::polling);
	    scheduler->yield(current);

	    // got re-activated -- start IPC now
	    to_tcb->lock();	
	    current->dequeue_send(to_tcb); 
	    to_tcb->unlock();
	}
#ifdef CONFIG_SMP
	else if ( !to_tcb->is_local_cpu() )
	{
	    TRACEPOINT(XCPU_IPC_SEND, 
		    printf("XCPU IPC send %t (%d) -> %t (%d)\n",
			current, current->get_cpu(),
			to_tcb, to_tcb->get_cpu()));

	    // receiver seems to be waiting -- try to send
	    xcpu_request( to_tcb->get_cpu(), do_xcpu_send, 
		    to_tcb, (word_t)current, sender_id.get_raw());

	    // at this stage we are already polling...
	    current->switch_to_idle();

	    // re-activated?
	    TRACE_XIPC("%t got re-activated after waiting for XCPU send\n", current);

	    // something happened -- retry sending
	    if (current->xcpu_status != 0)
	    {
#if 0
		// zero-timeout XCPU don't retry -- should be more generic
		// use absolute timeout on start of first round and check when
		// coming back
		if ( timeout.get_snd().is_zero() )
		{
		    current->set_state(thread_state_t::running);
		    current->set_tag(msg_tag_t::error_tag());
		    current->set_error_code(IPC_SND_ERROR(ERR_IPC_TIMEOUT));
		    return_ipc(NILTHREAD);
		}
#endif
		goto send_path;
	    }
	    // ... fall through and perform send
	}
#endif

	// The partner must be told who the IPC originated from.
	to_tcb->set_partner(sender_id);

	if (EXPECT_FALSE(!transfer_message(current, to_tcb,
			to_tid != intended_receiver, intended_receiver) ))
	{
	    /* error on transfer - activate the partner and return */
	    current->set_tag(to_tcb->get_tag());
	    if (EXPECT_TRUE( to_tcb->is_local_cpu() ))
	    {
		to_tcb->set_state(thread_state_t::running);
		scheduler->enqueue_ready(to_tcb);
	    }
#ifdef CONFIG_SMP
	    else
		UNIMPLEMENTED();
#endif
	    return_ipc(to_tid);
	}

#if defined(CONFIG_SMP)
	if (EXPECT_FALSE( !to_tcb->is_local_cpu()) )
	{
	    /* VU: kick receiver and forget about him
	     * we have to transmit the sender id since it is
	     * going to change soon!!! */
	    TRACEPOINT(XCPU_IPC_SEND_DONE,
		    printf("XCPU IPC send done %t (%d) -> %t (%d)\n",
			current, current->get_cpu(),
			to_tcb, to_tcb->get_cpu()));

	    TRACE_XIPC("%t (%x), %t\n", to_tcb, (word_t)to_tcb->get_state(), current);
	    xcpu_request( to_tcb->get_cpu(), do_xcpu_send_done, 
		    to_tcb, sender_id.get_raw());

	    // make sure we are running before potentially exiting to user
	    current->set_state(thread_state_t::running);
	    to_tcb = NULL;
	}
#endif
    }
    else
    {
	recv_blocks = current->get_tag().recv_blocks();
	current->set_tag(msgtag(0));
    }

    /* --- send finished ------------------------------------------------ */
receive_phase:

    if (EXPECT_FALSE( from_tid.is_nilthread() ))
    {
	/* this case is entered on:
	 *   - send-only case
	 *   - both descriptors set to nil id 
	 * in the SMP case to_tcb is always NULL! */
	if (to_tcb != NULL)
	{
	    ASSERT(DEBUG, to_tcb->is_local_cpu());

	    to_tcb->set_state(thread_state_t::running);
	    current->set_state(thread_state_t::running);

	    if (scheduler->check_dispatch_thread (current, to_tcb) )
	    {
		scheduler->enqueue_ready(current);
		current->switch_to(to_tcb);
	    }
	    else
	    {
		scheduler->enqueue_ready(to_tcb);
	    }
	}

	current->set_tag(msg_tag_t::nil_tag());
	return_ipc(from_tid);
    }
    /* --- receive phase ------------------------------------------------ */
    else /* ! from_tid.is_nilthread() */
    {
#ifndef CONFIG_SMP
	/* special case : from_tid == to_tid */
	if ( (from_tid == to_tid) && recv_blocks )
	{
	    TRACE_IPC("special case: from_tid=%t, to_tid=%t\n", TID(from_tid), TID(to_tid));
	    current->set_partner(from_tid);
	    current->set_state(thread_state_t::waiting_forever);
	    current->switch_to(to_tcb);
	    current->set_state(thread_state_t::running);
	    return_ipc(current->get_partner());
	}
#endif
#ifdef CONFIG_SMP
	/* VU: set thread state early to catch races */
	current->set_partner(from_tid);
	current->set_state(thread_state_t::waiting_forever);
#endif
	acceptor_t acceptor;
	acceptor.clear();

	/* VU: optimize for common case -- any, closed, anylocal */
	if (from_tid.is_anythread())
	{
	    from_tcb = current->send_head;
	    /* only accept notify bits if waiting from anythread */
	    acceptor = current->get_acceptor();
	}
	else if (from_tid.is_notifywait())
	{
	    /* Wait on asynch notify only */

	    /* check if we have any pending notify bits */
	    if (EXPECT_TRUE( current->get_notify_bits() & current->get_notify_mask() ))
	    {
		TRACE_NOTIFY("notify recieve (curr=%t)\n", current);
		setup_notify_return(current);
		return_ipc(NILTHREAD);
	    }

	    TRACE_NOTIFY("notify blocking receive (curr=%t)\n", current);

	    acceptor = current->get_acceptor();

	    /* current thread not accepting notifications? */
	    if ( !acceptor.accept_notify() )
	    {
		current->set_error_code(IPC_RCV_ERROR(ERR_IPC_NOT_ACCEPTED));
		current->set_tag(msg_tag_t::error_tag());
		return_ipc(NILTHREAD);
	    }

	    /* no pending bits and non-blocking */
	    if (EXPECT_FALSE( !recv_blocks ))
	    {
		/* prepare the IPC error */
		current->set_error_code(IPC_RCV_ERROR(ERR_IPC_NOPARTNER));
		current->set_tag(msg_tag_t::error_tag());
		ON_CONFIG_SMP(current->set_state(thread_state_t::running));
		return_ipc(NILTHREAD);
	    }
	    else
		current->set_state(thread_state_t::waiting_notify);

	    scheduler_t * scheduler = get_current_scheduler();

	    if (EXPECT_TRUE(to_tcb != NULL))
		scheduler->switch_highest(current, to_tcb);
	    else
		scheduler->yield(current);

	    TRACE_NOTIFY("  got wakeup (curr=%t)\n", current);
	    setup_notify_return(current);
	    return_ipc(NILTHREAD);
	}
	else if (EXPECT_TRUE( !from_tid.is_anylocalthread() ))
	{
	    /* closed wait */
	    ASSERT(DEBUG, from_tid.is_global());
	    from_tcb = current->get_space()->get_tcb(from_tid);

	    TRACE_IPC("closed wait from %t, current=%t, current_space=%p\n",
		      from_tcb, current, current->get_space());

	    if (EXPECT_FALSE( from_tcb->get_global_id() != from_tid ))
	    {
		/* wrong receiver id */
		TRACE_IPC("invalid from tid, wanted %t, but have %t\n",
			  from_tid.get_raw(), from_tcb );
		current->set_tag(msg_tag_t::error_tag());
		current->set_error_code(IPC_RCV_ERROR(ERR_IPC_NON_EXISTING));
		ON_CONFIG_SMP(current->set_state(thread_state_t::running));
		return_ipc(NILTHREAD);
	    }
	}
	else
	{   /* anylocal */
	    /* #warning wait_local incorrect */
	    from_tcb = current->send_head;
	}

	TRACE_IPC("receive phase curr=%t, from=%t\n", current, from_tcb);

	/*
	 * no partner || partner is not polling || 
	 * partner doesn't poll on me
	 */
	if( EXPECT_TRUE ( (from_tcb == NULL) || 
			  (!from_tcb->get_state().is_polling()) ||
			  (from_tcb->get_partner() != current->get_global_id()) ) )
	{
	    /* If no partner is ready to send, check if we have any pending notify bits */
	    if ( acceptor.accept_notify() && (current->get_notify_bits() & current->get_notify_mask()) )
	    {
		TRACE_NOTIFY("notify recieve (curr=%t)\n", current);
		setup_notify_return(current);
		return_ipc(NILTHREAD);
	    }

	    TRACE_IPC("blocking receive (curr=%t, from=%t)\n", current, from_tcb);

	    /* partner is not trying to send to me */
	    if (EXPECT_FALSE( !recv_blocks ))
	    {
		/* prepare the IPC error */
		current->set_error_code(IPC_RCV_ERROR(ERR_IPC_NOPARTNER));

		current->set_tag(msg_tag_t::error_tag());
		current->set_state(thread_state_t::running);
		return_ipc(NILTHREAD);
	    }
	    else
		current->set_state(thread_state_t::waiting_forever);
	    	    
	    /* VU: should we convert to a global id here??? */
	    current->set_partner(from_tid);

#ifdef CONFIG_SMP
	    if (EXPECT_FALSE( to_tcb && !to_tcb->is_local_cpu() ))
	    {
		xcpu_request( to_tcb->get_cpu(), do_xcpu_send_done,
				to_tcb, (word_t)current);
		to_tcb = NULL;
	    }
#endif
	    if (EXPECT_TRUE(to_tcb != NULL))
		scheduler->switch_highest(current, to_tcb);
	    else
		scheduler->yield(current);

	    if ( current->get_partner().is_nilthread() && (!current->get_tag().is_error()) )
	    {
		TRACE_IPC("Asynch notify wakeup\n");
		setup_notify_return(current);
		return_ipc(NILTHREAD);
	    }

	    TRACE_IPC("%t received msg\n", current);
	}
	else
	{
	    TRACE_IPC("perform receive from %t\n", from_tcb);
	    //enter_kdebug("do receive");

	    // both threads on the same CPU?
	    if (EXPECT_TRUE( from_tcb->is_local_cpu() ))
	    {
		/* partner is ready to send */
		from_tcb->set_state(thread_state_t::locked_running);
		current->set_state(thread_state_t::locked_waiting);

		/* Switch to waiting partner.  */
		if ( to_tcb != NULL)
		{
		    // Preserve priorities when choosing threads to activate,
		    // not on the critical path.
		    if ( scheduler->check_dispatch_thread ( from_tcb, to_tcb ) )
		    {
			scheduler->enqueue_ready(from_tcb);
			current->switch_to(to_tcb);
		    }
		    else
		    {
			to_tcb->set_state( thread_state_t::running );
			scheduler->enqueue_ready(to_tcb);
			current->switch_to(from_tcb);
		    }
		}
		else
		    current->switch_to(from_tcb);
	    }
#ifdef CONFIG_SMP
	    else {
		TRACEPOINT(XCPU_IPC_RECEIVE, 
			   printf("XCPU IPC receive curr=%t (%d) -> from=%t (%d)\n",
				  current, current->get_cpu(),
				  from_tcb, from_tcb->get_cpu()));

		TRACE_XIPC("XCPU receive from %t, to_tcb=%t\n", from_tcb, to_tcb);
		current->set_state(thread_state_t::locked_waiting);
		xcpu_request(from_tcb->get_cpu(), do_xcpu_receive, from_tcb, (word_t)current);

		if (!to_tcb) to_tcb = get_idle_tcb();

		current->switch_to(to_tcb);
		TRACE_XIPC("XCPU receive done (from=%t, curr=%t)\n", from_tcb, current);
	    }
#endif
	}
	current->set_state(thread_state_t::running);
	return_ipc(current->get_partner());
    }

    // this case should never happen
    enter_kdebug("ipc fall-through - why?");
    spin_forever();
}
