/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  Karlsruhe University
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *                
 * File path:     api/v4/schedule.cc
 * Description:   Scheduling functions
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
 * $Id: schedule.cc,v 1.56 2004/06/02 08:41:43 sgoetz Exp $
 *                
 ********************************************************************/
#include <l4.h>
#include <debug.h>
#include INC_API(tcb.h)
#include INC_API(schedule.h)
#include INC_API(interrupt.h)
#include INC_API(queueing.h)
#include INC_API(syscalls.h)
#include INC_API(smp.h)
#include INC_API(kernelinterface.h)
#include INC_GLUE(syscalls.h)
#include INC_GLUE(config.h)
#include INC_ARCH(special.h)

#include <kdb/tracepoints.h>

/* global idle thread, we allocate a utcb to make accessing MRs etc easier */
whole_tcb_t __idle_tcb UNIT("cpulocal") __attribute__((aligned(sizeof(whole_tcb_t))));
utcb_t	    __idle_utcb UNIT("cpulocal") __attribute__((aligned(sizeof(utcb_t))));

/* global scheduler object */
scheduler_t scheduler UNIT("cpulocal") __attribute__((aligned(32)));
volatile u64_t scheduler_t::current_time = 0;

#define QUANTUM_EXPIRED (~0UL)

DECLARE_TRACEPOINT(SYSCALL_THREAD_SWITCH);
DECLARE_TRACEPOINT(SYSCALL_SCHEDULE);
DECLARE_TRACEPOINT(TIMESLICE_EXPIRED);
DECLARE_TRACEPOINT(TOTAL_QUANTUM_EXPIRED);
DECLARE_TRACEPOINT(PREEMPTION_FAULT);
DECLARE_TRACEPOINT(PREEMPTION_SIGNALED);


/**
 * sends preemption IPC to the scheduler thread that the total quantum
 * has expired */
void scheduler_t::total_quantum_expired(tcb_t * tcb)
{
    if ( EXPECT_FALSE(tcb->get_preempt_flags().is_signaled()) )
    {
	TRACEPOINT(PREEMPTION_SIGNALED, 
		printf("preemption signalled for %t\n", tcb));

	tcb->set_preempted_ip( tcb->get_user_ip() );
	tcb->set_user_ip( tcb->get_preempt_callback_ip() );
    }

    TRACEPOINT(TOTAL_QUANTUM_EXPIRED, 
	       printf("total quantum expired for %t\n", tcb));
    tcb->send_preemption_ipc(current_time);
}


/**
 * find_next_thread: selects the next tcb to be dispatched
 *
 * returns the selected tcb, if no runnable thread is available, 
 * the idle tcb is returned.
 */
tcb_t * scheduler_t::find_next_thread(prio_queue_t * prio_queue)
{
    ASSERT(DEBUG, prio_queue);

    if (prio_queue->index_bitmap) {
	word_t top_word = msb(prio_queue->index_bitmap);
	word_t offset = BITS_WORD * top_word;

	for (long i = top_word; i >= 0; i--)
	{
	    word_t bitmap = prio_queue->prio_bitmap[i];

	    if (bitmap == 0)
		goto update;

	    do {
		word_t bit = msb(bitmap);
		word_t prio = bit + offset;
		tcb_t *tcb = prio_queue->get(prio);

		if (tcb) {
		    ASSERT(NORMAL, tcb->queue_state.is_set(queue_state_t::ready));

		    if ( EXPECT_TRUE(tcb->get_state().is_runnable() /*&& tcb->current_timeslice > 0 */) )
		    {
found:
			prio_queue->prio_bitmap[i] = bitmap;
			//TRACE("find_next: returns %t\n", tcb);
			return tcb;
		    }
		    else {
			/*if (tcb->get_state().is_runnable())
			    TRACEF("dequeueing runnable thread %t without timeslice\n", tcb);*/
			/* dequeue non-runnable thread */
			tcb = prio_queue->dequeue_fast(prio, tcb);
			if (tcb)
			    goto found;
		    }
		}
		/* If we get here, there are nomore runnable threads at this
		 * priority. Remove bit from prio_bitmap
		 */
		bitmap &= ~(1ul << bit);
	    } while (bitmap);
	    /* Update the bitmap before looking for more bits */
	    prio_queue->prio_bitmap[i] = 0;
	    prio_queue->index_bitmap &= ~(1ul<<i);
	    
update:
	    offset -= BITS_WORD;
	}
    }

    /* if we can't find a schedulable thread - switch to idle */
    //TRACE("find_next: returns idle\n");
    return get_idle_tcb();
}

/**
 * selects the next runnable thread and activates it.
 * @return true if a runnable thread was found, false otherwise
 */
bool scheduler_t::schedule(tcb_t * current)
{
    tcb_t * tcb = find_next_thread (&root_prio_queue);
    
    ASSERT(DEBUG, current);
    ASSERT(NORMAL, tcb);
    
    // do not switch to ourself
    if (tcb == current)
	return false;

    if (current != get_idle_tcb())
	enqueue_ready(current);

    current->switch_to(tcb);

    return true;
}

/**
 * activate a thread, if it is highest priority ready thread then
 * do so immediately, else put it on the run queue
 */
void scheduler_t::switch_highest(tcb_t *current, tcb_t * tcb)
{
    tcb_t * max = find_next_thread (&root_prio_queue);

    ASSERT(DEBUG, tcb);
    ASSERT(NORMAL, max);

    if (check_dispatch_thread (max, tcb) )
    {
	current->switch_to(tcb);
    }
    else
    {
	tcb->set_state( thread_state_t::running );
	enqueue_ready(tcb);
	current->switch_to(max);
    }
}


/**
 * Ensure current's state is not runnble() before this call!
 */
void scheduler_t::yield(tcb_t * current)
{
    tcb_t * tcb = find_next_thread (&root_prio_queue);

    ASSERT(NORMAL, tcb);
    ASSERT(NORMAL, tcb != current);

    current->switch_to(tcb);
}


/**
 * selects the next runnable thread in a round-robin fashion
 */
void scheduler_t::end_of_timeslice (tcb_t * tcb)
{
    spin(74, get_current_cpu());
    ASSERT(DEBUG, tcb);
    ASSERT(DEBUG, tcb != get_idle_tcb()); // the idler never yields

    prio_queue_t * prio_queue = get_prio_queue ( tcb );
    ASSERT(DEBUG, prio_queue);
    
    /* make sure we are in the run queue, put the TCB in front to
     * maintain the RR order */
    enqueue_ready_head (tcb);

    /* now schedule RR */
    prio_queue->set(get_priority (tcb), tcb->ready_list.next);

    // renew timeslice
    tcb->current_timeslice = prio_queue->current_timeslice + tcb->timeslice_length;

    /* now give the prio queue the newly scheduled thread's timeslice;
     * if we are the only thread in this prio we are going to be
     * rescheduled */
    prio_queue->current_timeslice = prio_queue->get(get_priority (tcb))->current_timeslice;
}


void scheduler_t::handle_timer_interrupt()
{
    spin(77, get_current_cpu());
    kdebug_check_breakin();

    if (get_current_cpu() == 0)
    {
	/* update the time global time*/
	current_time += get_timer_tick_length();
    }

#if defined(CONFIG_SMP)
    process_xcpu_mailbox();
#endif

    tcb_t * current = get_current_tcb();

    /* the idle thread schedules itself so no point to do it here.
     * Furthermore, it should not be preempted on end of timeslice etc.
     */
    if (current == get_idle_tcb())
	return;

    /* Check for not infinite timeslice and expired */
    if ( EXPECT_TRUE( current->timeslice_length != 0 ) &&
	 EXPECT_FALSE( (get_prio_queue(current)->current_timeslice -=
			 get_timer_tick_length()) <= 0 ) )
    {
	// We have end-of-timeslice.
	TRACEPOINT(TIMESLICE_EXPIRED, 
		    printf("timeslice expired for %t\n", current));

	end_of_timeslice ( current );
    }
    else
    {
	if ( EXPECT_FALSE(current->get_preempt_flags().is_signaled()) )
	{
	    TRACEPOINT(PREEMPTION_SIGNALED, 
		    printf("preemption signalled for %t\n", current));

	    current->set_preempted_ip( current->get_user_ip() );
	    current->set_user_ip( current->get_preempt_callback_ip() );
	}

	return;
    }

    /* time slice expired */
    if (EXPECT_FALSE (current->total_quantum))
    {
	/* we have a total quantum - so do some book-keeping */
	if (current->total_quantum == QUANTUM_EXPIRED)
	{
	    /* VU: must be revised. If a thread has an expired time quantum
	     * and is activated with switch_to his timeslice will expire
	     * and the event will be raised multiple times */
	    total_quantum_expired (current);
	}
	else if (current->total_quantum <= current->timeslice_length)
	{
	    /* we are getting close... */
	    current->current_timeslice += current->total_quantum;
	    current->total_quantum = QUANTUM_EXPIRED;
	}
	else
	{
	    // account this time slice
	    current->total_quantum -= current->timeslice_length;
	}
    }

    /* schedule the next thread */
    enqueue_ready(current);

    if ( schedule(current) )
    {
	if ( EXPECT_FALSE(current->get_preempt_flags().is_signaled()) )
	{
	    TRACEPOINT(PREEMPTION_SIGNALED, 
		    printf("preemption signalled for %t\n", current));

	    current->set_preempted_ip( current->get_user_ip() );
	    current->set_user_ip( current->get_preempt_callback_ip() );
	}
    }
}

static void init_all_threads(void)
{
    init_interrupt_threads();
    init_kernel_threads();
    init_root_servers();

#if defined(CONFIG_KDB_ON_STARTUP)
    enter_kdebug ("System started (press 'g' to continue)");
#endif
}

/**
 * the idle thread checks for runnable threads in the run queue
 * and performs a thread switch if possible. Otherwise, it 
 * invokes a system sleep function (which should normally result in
 * a processor halt)
 */
static void idle_thread()
{
    TRACE_INIT("idle thread started on CPU %d\n", get_current_cpu());

    while(1)
    {
	if (!get_current_scheduler()->schedule(get_idle_tcb()))
	{
	    spin(78, get_current_cpu());
	    processor_sleep();
	}
    }
}

SYS_THREAD_SWITCH (threadid_t dest)
{
    /* Make sure we are in the ready queue to 
     * find at least ourself and ensure that the thread 
     * is rescheduled */
    tcb_t * current = get_current_tcb();
    scheduler_t * scheduler = get_current_scheduler();

    TRACEPOINT( SYSCALL_THREAD_SWITCH,
		printf("SYS_THREAD_SWITCH current=%t, dest=%t\n",
		       current, TID(dest)) );

    /* explicit timeslice donation */
    if (!dest.is_nilthread())
    {
	tcb_t * dest_tcb = get_current_space()->get_tcb(dest);

	if ( dest_tcb == current )
	    return_thread_switch();

	if ( dest_tcb->get_state().is_runnable() &&
	     dest_tcb->myself_global == dest &&
	     dest_tcb->is_local_cpu() )
	{
	    scheduler->enqueue_ready(current);
	    current->switch_to(dest_tcb);
	    return_thread_switch();
	}
    }

    scheduler->enqueue_ready(current);

    /* eat up timeslice - we get a fresh one */
    scheduler->get_prio_queue(current)->current_timeslice = 0;
    scheduler->end_of_timeslice (current);
    scheduler->schedule (current);
    return_thread_switch();
}


/* local part of schedule */
bool do_schedule(tcb_t * tcb, word_t ts_len, word_t total_quantum, word_t prio)
{
    ASSERT(DEBUG, tcb->get_cpu() == get_current_cpu());

    scheduler_t * scheduler = get_current_scheduler();

    if ( (prio != (~0UL)) && ((prio_t)prio != scheduler->get_priority(tcb)) )
    {
	scheduler->dequeue_ready(tcb);
	scheduler->set_priority(tcb, (prio_t) (prio & 0xff));
	scheduler->enqueue_ready(tcb);
    }

    if ( total_quantum != (~0UL) )
    {
	if ( total_quantum == 0 )
	{
	    tcb->total_quantum = QUANTUM_EXPIRED;
	} else
	{
	    scheduler->set_total_quantum (tcb, total_quantum);
	}
	scheduler->enqueue_ready (tcb);
    }

    if ( ts_len != (~0UL) )
    {
	scheduler->set_timeslice_length (tcb, ts_len);
    }

    return true;
}

#ifdef CONFIG_SMP
static void do_xcpu_schedule(cpu_mb_entry_t * entry)
{
    tcb_t * tcb = entry->tcb;

#ifdef CONFIG_IS_64BIT
    // meanwhile migrated? 
    if (tcb->get_cpu() != get_current_cpu())
	xcpu_request (tcb->get_cpu(), do_xcpu_schedule, tcb, 
		      entry->param[0], entry->param[1],
		      entry->param[2], entry->param[3]);
    else
	do_schedule (tcb, entry->param[0], entry->param[1],
		entry->param[2], entry->param[3]);
#else
    // meanwhile migrated? 
    if (tcb->get_cpu() != get_current_cpu())
	xcpu_request (tcb->get_cpu(), do_xcpu_schedule, tcb, 
		      entry->param[0], entry->param[1],
		      entry->param[2], entry->param[3],
		      entry->param[4], entry->param[5]);
    else
	do_schedule (tcb, entry->param[0], entry->param[1],
		entry->param[2] + (u64_t)entry->param[3] >> 32,
		entry->param[4] + (u64_t)entry->param[5] >> 32);
#endif
}
#endif

SYS_SCHEDULE (threadid_t dest_tid, word_t ts_len, word_t total_quantum,
	word_t processor_control, word_t prio )
{
    tcb_t * current = get_current_tcb();
    
    TRACEPOINT(SYSCALL_SCHEDULE, 
	printf("SYS_SCHEDULE: curr=%t, dest=%t, quantum=%x, "
	       "timeslice=%x, proc_ctrl=%x, prio=%x\n",
	       current, TID(dest_tid), total_quantum, ts_len,
	       processor_control, prio));

    tcb_t * dest_tcb = get_current_space()->get_tcb(dest_tid);
    
    // make sure the thread id is valid
    if (dest_tcb->get_global_id() != dest_tid)
    {
	current->set_error_code (EINVALID_THREAD);
	return_schedule(0, 0, 0);
    }

    // are we in the same address space as the scheduler of the thread?
    tcb_t * sched_tcb = get_current_space()->get_tcb(dest_tcb->get_scheduler());
    if (sched_tcb->get_global_id() != dest_tcb->get_scheduler() ||
	sched_tcb->get_space() != get_current_space())
    {
	current->set_error_code (ENO_PRIVILEGE);
	return_schedule(0, 0, 0);
    }

    /* #warning FIXME: Check sys_schedule parameters */

    if ( dest_tcb->is_local_cpu() )
    {
	do_schedule ( dest_tcb, ts_len, total_quantum, prio );
	
	if ( processor_control != ~0UL ) 
	    dest_tcb->migrate_to_processor (processor_control);
    }
#ifdef CONFIG_SMP
    else
    {
#ifdef CONFIG_IS_64BIT
	xcpu_request(dest_tcb->get_cpu(), do_xcpu_schedule, 
		     dest_tcb, prio, preemption_control, quantum, timeslice);
#else
	xcpu_request(dest_tcb->get_cpu(), do_xcpu_schedule, 
		     dest_tcb, prio, preemption_control, (u32_t)quantum, (u32_t)(quantum >> 32),
		     (u32_t)timeslice, (u32_t)(timeslice >> 32));
#endif

	if ( processor_control != ~0UL ) 
	    dest_tcb->migrate_to_processor (processor_control);
    }
#endif

    thread_state_t state = dest_tcb->get_state();

    return_schedule(state == thread_state_t::aborted	? 1 : 
		    state.is_halted()			? 2 :
		    state.is_running()			? 3 :
		    state.is_polling()			? 4 :
		    state.is_sending()			? 5 :
		    state.is_waiting()			? 6 :
		    state.is_receiving()		? 7 :
		    ({ TRACEF("invalid state (%x)\n", (word_t)state); 0; }),
		    dest_tcb->current_timeslice,
		    dest_tcb->total_quantum);
}

/**
 * Enqueues a TCB into the ready list
 *
 * @param tcb       thread control block to enqueue
 * @param head      Enqueue TCB at the head of the list when true
 */
void scheduler_t::enqueue_ready(tcb_t * tcb)
{
    ASSERT(DEBUG, tcb);
    ASSERT(DEBUG, tcb->get_cpu() == get_current_cpu());
    get_prio_queue(tcb)->enqueue(tcb);
}

void scheduler_t::enqueue_ready_head(tcb_t * tcb)
{
    ASSERT(DEBUG, tcb);
    ASSERT(DEBUG, tcb->get_cpu() == get_current_cpu());
    get_prio_queue(tcb)->enqueue_head(tcb);
}

/**********************************************************************
 *
 *                     Initialization
 *
 **********************************************************************/

void SECTION(".init") scheduler_t::start(cpuid_t cpuid)
{
    TRACE_INIT ("Switching to idle thread (CPU %d)\n", cpuid);
    get_idle_tcb()->set_cpu(cpuid);

    initial_switch_to(get_idle_tcb());
}

void SECTION(".init") scheduler_t::init( bool bootcpu )
{
    TRACE_INIT ("Initializing threading\n");

    root_prio_queue.init();

    get_idle_tcb()->create_kernel_thread(NILTHREAD, &__idle_utcb);
    /* set idle-magic */
    get_idle_tcb()->myself_global.set_raw((word_t)0x1d1e1d1e1d1e1d1eULL);
    get_idle_tcb()->notify(idle_thread);
    if( bootcpu )
    	get_idle_tcb()->notify(init_all_threads);
    return;
}
