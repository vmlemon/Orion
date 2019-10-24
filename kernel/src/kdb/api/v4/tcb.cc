/*********************************************************************
 *
 * Copyright (C) 2002-2004,  Karlsruhe University
 *
 * File path:     kdb/api/v4/tcb.cc
 * Description:   tcb dumping
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
 * $Id: tcb.cc,v 1.42 2004/12/09 01:26:20 cvansch Exp $
 *
 ********************************************************************/
#include <l4.h>
#include <debug.h>
#include <kdb/kdb.h>
#include <kdb/cmd.h>
#include <kdb/input.h>
#include INC_API(tcb.h)
#include INC_API(schedule.h)

#if defined(CONFIG_IS_64BIT)
#define __PADSTRING__ "        "
#else
#define __PADSTRING__ ""
#endif


DECLARE_CMD(cmd_show_tcb, root, 't', "showtcb",  "show thread control block");
DECLARE_CMD(cmd_show_tcbext, root, 'T', "showtcbext", "shows thread control block (extended)");

static inline msg_tag_t SECTION(SEC_KDEBUG) get_msgtag(tcb_t* tcb)
{
    msg_tag_t tag;
    tag.raw = tcb->get_mr(0);
    return tag;
}

void SECTION(SEC_KDEBUG) dump_tcb(tcb_t * tcb)
{
    printf("=== %s == TCB: %p == ID: %p = %p/%p == PRIO: 0x%02x",
	   tcb->debug_name[0] ? tcb->debug_name : "<noname>",
	   tcb, tcb->get_global_id().get_raw(),
	   tcb->utcb_location, tcb->get_utcb(),
	   get_current_scheduler()->get_priority(tcb));
#if !defined(CONFIG_SMP)
    printf(" ===\n");
#else
    printf(" CPU: %d ===\n", tcb->get_cpu());
#endif
    printf("UIP: %p   queues: %c%c%c%c%s      wait : %wt:%-wt   space: %p\n",
	   tcb->get_user_ip(),
	   tcb->queue_state.is_set(queue_state_t::ready )	? 'R' : 'r',
	   tcb->queue_state.is_set(queue_state_t::send)		? 'S' : 's',
	   tcb->queue_state.is_set(queue_state_t::wakeup)	? 'W' : 'w',
	   tcb->queue_state.is_set(queue_state_t::late_wakeup)	? 'L' : 'l',
	   __PADSTRING__,
	   tcb->wait_list.next, tcb->wait_list.prev,
	   tcb->get_space());
    printf("USP: %p   tstate: %ws  ready: %w.wt:%-w.wt   pdir : %p\n",
	   tcb->get_user_sp(),
	   tcb->get_state() == thread_state_t::running		? "RUNNING" :
	   tcb->get_state() == thread_state_t::polling		? "POLLING" :
	   tcb->get_state() == thread_state_t::waiting_forever	? "WAIT_FE" :
	   tcb->get_state() == thread_state_t::waiting_notify	? "WAIT_NF" :
	   tcb->get_state() == thread_state_t::locked_running	? "LCKRUNN" :
	   tcb->get_state() == thread_state_t::locked_waiting	? "LCKWAIT" :
	   tcb->get_state() == thread_state_t::halted		? "HALTED " :
	   tcb->get_state() == thread_state_t::aborted		? "ABORTED" :
								  "???????",
	   tcb->ready_list.next, tcb->ready_list.prev,
	   tcb->pdir_cache);
    printf("KSP: %p   sndhd : %-w.wt  send : %w.wt:%-w.wt   pager: %.wt\n",
	   tcb->stack, tcb->send_head, tcb->send_list.next, tcb->send_list.prev,
	   TID(tcb->get_utcb() ? tcb->get_pager() : threadid_t::nilthread()));

    printf("total quant:    0x%x us, ts length  :       0x%x us, curr ts: 0x%m us\n",
	   tcb->total_quantum, tcb->timeslice_length,
	   tcb->current_timeslice);
    printf("resources: %p [", (word_t) tcb->resource_bits);
    tcb->resources.dump (tcb);
#if defined(CONFIG_ARCH_ARM) && defined(CONFIG_ENABLE_FASS)
    printf("]");
    printf(", ARM [PID: %d, vspace: %d, domain: %d",
		    tcb->get_space() ? tcb->get_space()->get_pid() : 0,
		    tcb->get_space() ? tcb->get_space()->get_vspace() : 0,
		    tcb->get_space()->get_domain() );
#endif
    printf("]\n");

    printf("scheduler: %.wt  send redirector: %.wt  recv redirector: %.wt\n",
		    TID(tcb->scheduler), TID(tcb->send_redirector),
		    TID(tcb->recv_redirector));
    printf("  partner: %.wt    saved partner: %.wt      saved state: %s\n",
	   TID(tcb->get_partner()), TID(tcb->get_saved_partner ()),
	   tcb->get_saved_state ().string ());
}


void SECTION (SEC_KDEBUG) dump_utcb (tcb_t * tcb)
{
    printf ("\nuser handle:       %p  "
	    "cop flags:      %02x%s  "
	    "preempt flags:     %02x [%c]\n"
	    "exception handler: %t  "
	    "virtual sender: %t  "
	    "intended receiver: %t\n",
	    tcb->get_user_handle (), tcb->get_cop_flags (),
	    sizeof (word_t) == 8 ? "              " : "      ",
	    tcb->get_preempt_flags ().raw,
	    tcb->get_preempt_flags ().is_signaled() ? 's' : '~',
	    TID (tcb->get_exception_handler ()),
	    TID (tcb->get_virtual_sender ()),
	    TID (tcb->get_intended_receiver ()));
    printf("incomming notify bits: %p  "
	    "notify mask:         %p\n",
	    tcb->get_notify_bits(), tcb->get_notify_mask());
    printf("last preempted_ip:     %p  "
	    "preempt_callback_ip: %p\n",
	    tcb->get_preempted_ip(), tcb->get_preempt_callback_ip());
}


/**
 * Dumps a message and buffer registers of a thread in human readable form
 * @param tcb	pointer to thread control block
 */
static void SECTION(SEC_KDEBUG) dump_message_registers(tcb_t * tcb)
{
    for (int i = 0; i < IPC_NUM_MR; i++)
    {
	if (!(i % 8)) printf("\nmr(%02d):", i);
	printf(" %p", tcb->get_mr(i));
    }

    printf("\nMessage Tag: %d untyped, %d typed, label = %x, flags = %c%c%c%c\n",
	get_msgtag(tcb).get_untyped(), get_msgtag(tcb).get_typed(),
	get_msgtag(tcb).send.label,
	get_msgtag(tcb).is_error() ? 'E' : '-',
	get_msgtag(tcb).is_xcpu() ? 'X' : '-',
	get_msgtag(tcb).is_redirected() ? 'r' : '-',
	get_msgtag(tcb).is_propagated() ? 'p' : '-'
    );

    for (word_t i = 0; i < get_msgtag(tcb).get_typed();)
    {
	int offset = get_msgtag(tcb).get_untyped() + 1;
	msg_item_t item;

	item = tcb->get_mr(offset + i);
	if (item.is_map_item() || item.is_grant_item())
	{
	    fpage_t fpage ((fpage_t) {{ raw: tcb->get_mr(offset + i + 1)}} );
	    printf("%s item: snd base=%p, fpage=%p (addr=%p, sz=%x), %c%c%c\n",
		item.is_map_item() ? "map" : "grant",
		item.get_snd_base(),
		fpage.raw, fpage.get_base(), fpage.get_size(),
		fpage.x.write	? 'W' : 'w',
		fpage.x.read	? 'R' : 'r',
		fpage.x.execute	? 'X' : 'x');
	    i+=2;
	}
	else
	{
	    printf("unknown item type (%p)\n", item.raw);
	    i++;
	}
    }
}

static void SECTION(SEC_KDEBUG) dump_ipc_registers(tcb_t * tcb)
{
    acceptor_t acc;
    fpage_t fpage;

    acc = tcb->get_acceptor();
    fpage.raw = acc.raw;
    fpage.raw &= ~0xf; // mask out lowermost bits.

    printf("\nAcceptor: %p (%c)", acc.raw, acc.accept_notify() ? 'A' : 'a');
    printf("  Error code: %d\n", tcb->get_error_code());
    printf("  fpage :");
    if (fpage.is_nil_fpage())
	printf(" (NIL-FPAGE)\n");
    else if (fpage.is_complete_fpage())
	printf(" (COMPLETE-FPAGE)\n");
    else
	printf("  fpage=%p (addr=%p, sz=%p)\n",
	    fpage.raw, fpage.get_base(), fpage.get_size());
}

tcb_t SECTION(SEC_KDEBUG) * kdb_get_tcb()
{
    return get_thread("tcb/tid/name");
}

CMD(cmd_show_tcb, cg)
{
    tcb_t * tcb = kdb_get_tcb();
    if (tcb)
	dump_tcb(tcb);
    return CMD_NOQUIT;
}

CMD(cmd_show_tcbext, cg)
{
    tcb_t * tcb = kdb_get_tcb();
    if (tcb)
    {
	dump_tcb(tcb);
	if (tcb->get_utcb())
	{
	    dump_utcb(tcb);
	    dump_message_registers(tcb);
	    dump_ipc_registers(tcb);
	}
	else
	    printf("no valid UTCB\n");
    }
    return CMD_NOQUIT;
}
