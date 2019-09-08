/*********************************************************************
 *                
 * Copyright (C) 2002-2003,  Karlsruhe University
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *                
 * File path:     api/v4/schedule.h
 * Description:   scheduling declarations
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
 * $Id: schedule.h,v 1.22 2003/10/28 10:29:42 joshua Exp $
 *                
 ********************************************************************/

#ifndef __API__V4__SCHEDULE_H__
#define __API__V4__SCHEDULE_H__

#include INC_API(tcb.h)
#include INC_GLUE(schedule.h)
#include <kdb/tracepoints.h>

#define BITMAP_WORDS	( (MAX_PRIO + BITS_WORD) / BITS_WORD )

class prio_queue_t 
{
public:
    void enqueue( tcb_t * tcb )
	{
	    ASSERT(DEBUG, tcb != get_idle_tcb());
    
	    if (tcb->queue_state.is_set(queue_state_t::ready))
		return;
    
	    ASSERT(DEBUG, tcb->priority >= 0 && tcb->priority <= MAX_PRIO);

	    ENQUEUE_LIST_TAIL (prio_queue[tcb->priority], tcb, ready_list);
    
	    tcb->queue_state.set(queue_state_t::ready);

	    prio_t prio = tcb->priority;
	    index_bitmap |= (1ul << (prio / BITS_WORD));
	    prio_bitmap[prio / BITS_WORD] |= 1ul << (prio % BITS_WORD);
	}

    void enqueue_head( tcb_t * tcb )
	{
	    ASSERT(DEBUG, tcb != get_idle_tcb());
    
	    if (tcb->queue_state.is_set(queue_state_t::ready))
		return;
    
	    ASSERT(DEBUG, tcb->priority >= 0 && tcb->priority <= MAX_PRIO);

	    ENQUEUE_LIST_HEAD (prio_queue[tcb->priority], tcb, ready_list);
    
	    tcb->queue_state.set(queue_state_t::ready);

	    prio_t prio = tcb->priority;
	    index_bitmap |= (1ul << (prio / BITS_WORD));
	    prio_bitmap[prio / BITS_WORD] |= 1ul << (prio % BITS_WORD);
	}

    void dequeue(tcb_t * tcb)
	{
	    ASSERT(DEBUG, tcb);
	    if (!tcb->queue_state.is_set(queue_state_t::ready))
		return;
	    ASSERT(DEBUG, tcb->priority >= 0 && tcb->priority <= MAX_PRIO);
	    DEQUEUE_LIST(prio_queue[tcb->priority], tcb, ready_list);
	    tcb->queue_state.clear(queue_state_t::ready);
	}

    tcb_t * dequeue_fast(prio_t prio, tcb_t * tcb)
	{
	    ASSERT(DEBUG, tcb);
	    tcb_t *head;
	    tcb->queue_state.clear(queue_state_t::ready);

	    if (tcb->ready_list.next == tcb)
	    {
		head = NULL;
	    }
	    else
	    {
		head = tcb;
		do {
		    head = head->ready_list.next;
		    if (head == tcb) {
			head = NULL;
			goto fast_out;
		    }
		    if (!head->get_state().is_runnable()) {
			head->queue_state.clear(queue_state_t::ready);
		    } else
			break;
		} while (1);

		tcb_t * prev = tcb->ready_list.prev;
		prev->ready_list.next = head;
		head->ready_list.prev = prev;
	    }
fast_out:
	    prio_queue[prio] = head;
	    return head;
	}

    void set(prio_t prio, tcb_t * tcb)
	{
	    ASSERT(DEBUG, tcb);
	    ASSERT(NORMAL, prio >= 0 && prio <= MAX_PRIO);
	    this->prio_queue[prio] = tcb;
	}

    tcb_t * get(prio_t prio)
	{
	    ASSERT(NORMAL, prio >= 0 && prio <= MAX_PRIO);
	    return prio_queue[prio];
	}

    void init ()
	{
	    word_t i;
	    /* prio queues */
	    for(i = 0; i < MAX_PRIO; i++)
		prio_queue[i] = (tcb_t *)NULL;
	    index_bitmap = 0;
	    for(i = 0; i < BITMAP_WORDS; i++)
		prio_bitmap[i] = 0;
	}

private:
    tcb_t * prio_queue[MAX_PRIO + 1];
public:
    word_t index_bitmap;
    word_t prio_bitmap[BITMAP_WORDS];
    tcb_t * timeslice_tcb;
    s64_t current_timeslice;
};

class scheduler_t
{
public:
    /**
     * initializes the scheduler, must be called before init
     */
    void init( bool bootcpu = true );

    /**
     * starts the scheduling, does not return
     * @param cpu processor the scheduler starts on
     */
    void start(cpuid_t cpu = 0);

    /**
     * schedule a runnable thread
     * @param current the current thread control block
     * @return true if a runnable thread was found, false otherwise
     */
    bool schedule(tcb_t * current);

    /**
     * Marks end of timeslice
     * @param tcb       TCB of thread whose timeslice ends
     */
    void end_of_timeslice (tcb_t * tcb);

    /**
     * dispatches a thread 
     * @param tcb the thread control block of the thread to be dispatched
     */
    void dispatch_thread(tcb_t * tcb);

    void yield(tcb_t * current);

    /**
     * activate a thread, if it is highest priority ready thread then
     * do so immediately, else put it on the run queue
     */
    void scheduler_t::switch_highest(tcb_t *current, tcb_t * tcb);

    /**
     * hierarchical scheduling prio queue 
     */
    prio_queue_t * get_prio_queue(tcb_t * tcb)
	{
	    ASSERT(DEBUG, tcb);
	    return &this->root_prio_queue;
	}

    /**
     * delay preemption
     * @param current   current TCB
     * @param tcb       destination TCB
     * @return true if preemption was delayed, otherwise false
     */
    bool delay_preemption ( tcb_t * current, tcb_t * tcb );

    /**
     * scheduler-specific dispatch decision
     * @param current current thread control block
     * @param tcb tcb to be dispatched
     * @return true if tcb can be dispatched, false if a 
     * scheduling decission has to be made
     */
    bool check_dispatch_thread(tcb_t * current, tcb_t * tcb)
	{
	    ASSERT(DEBUG, tcb);
	    ASSERT(DEBUG, current);
	    return (get_priority(current) < get_priority(tcb)); 
	}

    /**
     * scheduler preemption accounting remaining timeslice etc.
     */
    void preempt_thread(tcb_t * current, tcb_t * dest)
	{
	    ASSERT(DEBUG, dest);
	    ASSERT(DEBUG, current);
	    ASSERT(DEBUG, current->get_cpu() == dest->get_cpu());
	    ASSERT(DEBUG, current != get_idle_tcb()); // don't "preempt" idle

	    /* re-account the remaining timeslice */
	    current->current_timeslice = 
		get_prio_queue(current)->current_timeslice;
	    enqueue_ready_head(current);

	    /* now switch to timeslice of dest */
	    get_prio_queue(dest)->current_timeslice = dest->current_timeslice;
	}

    /**
     * Enqueues a TCB into the ready list
     *
     * @param tcb       thread control block to enqueue
     * @param head      Enqueue TCB at the head of the list when true
     */
    void enqueue_ready(tcb_t * tcb);
    void enqueue_ready_head(tcb_t * tcb);

    /**
     * dequeues tcb from the ready list (if the thread is in)
     * @param tcb thread control block
     */
    void dequeue_ready(tcb_t * tcb)
    {
	ASSERT(DEBUG, tcb);
	get_prio_queue(tcb)->dequeue(tcb);
    }

    /**
     * sets the total time quantum of the thread
     */
    void set_total_quantum(tcb_t * tcb, word_t quantum)
    {
	ASSERT(DEBUG, tcb);

	tcb->total_quantum = quantum;
	// give a fresh timeslice according to the spec
	tcb->current_timeslice = tcb->timeslice_length;
    }

    /**
     * sets the timeslice length of a thread 
     * @param tcb thread control block of the thread
     * @param timeslice timeslice length (must be a time period)
     */
    void set_timeslice_length(tcb_t * tcb, word_t timeslice)
    {
	ASSERT(DEBUG, tcb);
	tcb->current_timeslice = tcb->timeslice_length = timeslice;
    }

    /**
     * sets the priority of a thread
     * @param tcb thread control block
     * @param prio priority of thread
     */
    void set_priority(tcb_t * tcb, prio_t prio)
    {
	ASSERT(DEBUG, prio >= 0 && prio <= MAX_PRIO);
	ASSERT(NORMAL, !tcb->queue_state.is_set(queue_state_t::ready));
	tcb->priority = prio;
    }

    /**
     * delivers the current priority of a thread
     * @param tcb thread control block of the thread
     * @return the priority 
     */
    static prio_t get_priority(tcb_t * tcb)
    { 
	ASSERT(DEBUG, tcb);
	return tcb->priority; 
    }

    /**
     * initialize the scheduling parameters of a TCB
     * @param tcb		pointer to TCB
     * @param prio		priority of the thread
     * @param total_quantum	initial total quantum
     * @param timeslice_length	length of time slice
     */
    void init_tcb(tcb_t * tcb, prio_t prio = DEFAULT_PRIORITY, 
	word_t total_quantum = DEFAULT_TOTAL_QUANTUM,
	word_t timeslice_length = DEFAULT_TIMESLICE_LENGTH)
    {
	set_timeslice_length(tcb, timeslice_length);
	set_total_quantum(tcb, total_quantum);
	set_priority(tcb, prio);
    }
    
    /**
     * handles the timer interrupt event, walks the wait lists 
     * and makes a scheduling decission
     */
    void handle_timer_interrupt();

    /**
     * delivers the current time relative to the system 
     * startup in microseconds
     * @return absolute time 
     */
    u64_t get_current_time()
    {
	return current_time;
    }

    /**
     * function is called when the total quantum of a thread expires 
     * @param tcb thread control block of the thread whose quantum expired
     */
    void total_quantum_expired(tcb_t * tcb);

private:
    /**
     * searches the run queue for the next runnable thread
     * @return next thread to be scheduled
     */
    tcb_t * find_next_thread(prio_queue_t * prio_queue);

private:
    prio_queue_t root_prio_queue;
    static volatile u64_t current_time;
};





/* global function declarations */

/**
 * @return the current scheduler 
 * the default implementation features exactly one scheduler at a time.
 */
INLINE scheduler_t * get_current_scheduler()
{
    extern scheduler_t scheduler;
    return &scheduler;
}

/* Get the current l4 scheduler time */
INLINE u64_t get_current_time()
{
    return get_current_scheduler()->get_current_time();
}

#endif /*__API__V4__SCHEDULE_H__*/

