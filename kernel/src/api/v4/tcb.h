/*********************************************************************
 *                
 * Copyright (C) 2002, 2003-2004,  Karlsruhe University
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *                
 * File path:     api/v4/tcb.h
 * Description:   V4 TCB
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
 * $Id: tcb.h,v 1.70 2004/12/09 01:03:43 cvansch Exp $
 *                
 ********************************************************************/
#ifndef __API__V4__TCB_H__
#define __API__V4__TCB_H__

#include <debug.h>

#include INC_API(types.h)
#include INC_API(queuestate.h)
#include INC_API(queueing.h)
#include INC_API(threadstate.h)
#include INC_API(space.h)
#include INC_API(resources.h)
#include INC_API(thread.h)
#include INC_API(preempt.h)
#include INC_API(fpage.h)
#include INC_API(ipc.h)

/* implementation specific functions */
#include INC_GLUE(ktcb.h)
#include INC_GLUE(utcb.h)

typedef int prio_t;
class space_t;
class prio_queue_t;

/**
 * tcb_t: kernel thread control block
 */
class tcb_t
{
public:
    enum unwind_reason_e {
	abort		= 1,
    };

    /* public functions */
    bool activate(void (*startup_func)(), threadid_t pager);
    
    bool create_inactive(threadid_t dest, threadid_t scheduler);
    void create_kernel_thread(threadid_t dest, utcb_t * utcb);
    
    void delete_tcb();
    bool migrate_to_space(space_t * space);
    bool migrate_to_processor(cpuid_t processor);
    bool exists() 
	{ return space != NULL; }
    bool is_activated()
	{ return utcb != NULL; }

    void unwind (unwind_reason_e reason);
    
    /* queue manipulations */
    void enqueue_send(tcb_t * tcb);
    void dequeue_send(tcb_t * tcb);
    void enqueue_present();
    void dequeue_present();

    /* thread id access functions */
    void set_global_id(threadid_t tid);
    threadid_t get_global_id();

    bool check_utcb_location (void);
    bool check_utcb_location (word_t location);
    void set_utcb_location (word_t location);
    word_t get_utcb_location();

    void set_error_code (word_t err);
    word_t get_error_code (void);

    /* redirector access functions */
    void set_send_redirector(threadid_t redir);
    threadid_t get_send_redirector();
    void set_recv_redirector(threadid_t redir);
    threadid_t get_recv_redirector();

    /* thread state */
    void set_state(thread_state_t state);
    thread_state_t get_state();
    arch_ktcb_t * get_arch();
    void save_state (void);
    void restore_state (void);

    /* ipc */
    void set_partner(threadid_t tid);
    threadid_t get_partner();
    tcb_t * get_partner_tcb();

    void set_actual_sender(threadid_t tid);
    threadid_t get_virtual_sender();
    void set_intended_receiver(threadid_t tid);
    threadid_t get_intended_receiver();

    msg_tag_t get_tag();
    void set_tag(msg_tag_t tag);
    word_t get_mr(word_t index);
    void set_mr(word_t index, word_t value);
    void copy_mrs(tcb_t * dest, word_t start, word_t count);

    msg_tag_t do_ipc(threadid_t to_tid, threadid_t from_tid);
    void send_pagefault_ipc(addr_t addr, addr_t ip, space_t::access_e access);
    bool send_preemption_ipc(u64_t abstime);
    void return_from_ipc (void);
    void return_from_user_interruption (void);

    /* synchronization */
    void lock() { tcb_lock.lock(); }
    void unlock() { tcb_lock.unlock(); }

    /* thread notification */
    void notify(void (*func)());
    void notify(void (*func)(word_t), word_t arg1);
    void notify(void (*func)(word_t, word_t), word_t arg1, word_t arg2);
    void notify(void (*func)(tcb_t*, word_t), tcb_t * tcb, word_t arg)
	{ notify((void(*)(word_t, word_t))func, (word_t)tcb, arg); }

    /* thread manipulation */
    addr_t get_user_ip();
    addr_t get_user_sp();
    void set_user_ip(addr_t ip);
    void set_user_sp(addr_t sp);
    void copy_user_regs(tcb_t *src);
    void copy_regs_to_mrs(tcb_t *src);

    /* preemption callback signaling */
    addr_t get_preempted_ip();
    void set_preempted_ip(addr_t ip);
    addr_t get_preempt_callback_ip();

    /* thread switching */
    void switch_to(tcb_t * current);
    void switch_to_idle();

    /* space */
    space_t * get_space() { return space; }
    void set_space(space_t * space);

    /* processor */
    cpuid_t get_cpu() 
#if defined(CONFIG_SMP)
	{ return this->cpu; }
#else
	{ return 0; }
#endif
    void set_cpu(cpuid_t cpu);
    bool is_local_cpu();

    /* utcb access functions */
    utcb_t * get_utcb()
        const { return this->utcb; }

public:
    void set_scheduler(const threadid_t tid);
    void set_pager(const threadid_t tid);
    void set_exception_handler(const threadid_t tid);
    void set_user_handle(const word_t handle);
    void set_user_flags(const word_t flags);
    void set_acceptor(const acceptor_t acceptor);

    threadid_t get_pager();
    threadid_t get_scheduler();
    threadid_t get_exception_handler();
    word_t get_user_handle();
    word_t get_user_flags();
    preempt_flags_t get_preempt_flags();
    void set_preempt_flags(preempt_flags_t flags);
    u8_t get_cop_flags();
    word_t * get_reg_stack_bottom (void);
    acceptor_t get_acceptor();

    /* asynchronous notification */
    void clear_notify_bits();
    void add_notify_bits(const word_t bits);
    void sub_notify_bits(const word_t bits);
    void set_notify_mask(const word_t mask);

    word_t get_notify_bits();
    word_t get_notify_mask();

    /* interrupt management */
    void set_irq_handler(const threadid_t tid);
    threadid_t get_irq_handler();
    bool is_interrupt_thread();

public:
    bool allocate();

private:
    void create_tcb(const threadid_t dest);
    bool init(threadid_t dest);
    
    /* stack manipulation */
public:
    word_t * get_stack_top();
    void init_stack();
private:
    void create_startup_stack(void (*func)());
    

    friend void make_offsets(); /* generates OFS_TCB_ stuff */

public:

    typedef union {
	struct {
	    /* IPC copy */
	    word_t		saved_mr[3];
	    acceptor_t		saved_acceptor;
	    word_t		saved_error;
	} ipc_copy;

	struct {
	    /* Exchange registers */
	    word_t		control;
	    word_t		sp;
	    word_t		ip;
	    word_t		flags;
	    threadid_t		pager;
	    word_t		user_handle;
	} exregs;
    } misc_tcb_t;

    threadid_t get_saved_partner (void) { return saved_partner; }
    void set_saved_partner (threadid_t t) { saved_partner = t; }

    thread_state_t get_saved_state (void)
	{ return saved_state; }
    void set_saved_state (thread_state_t s)
	{ saved_state = s; }
    void set_saved_state (thread_state_t::thread_state_e s)
	{ saved_state = (thread_state_t) s; }

    /* do not delete this TCB_START_MARKER */

    // have relatively static values here
    threadid_t		myself_global;

private:
    word_t		utcb_location;
    utcb_t *		utcb;
    
    thread_state_t 	thread_state;
    threadid_t		partner;

    resource_bits_t	resource_bits;
private:
    threadid_t		send_redirector;
    threadid_t		recv_redirector;

    space_t *		space;
public:
    queue_state_t	queue_state;

    /* queues */
    ringlist_t<tcb_t>	ready_list;
    ringlist_t<tcb_t>	send_list;

    tcb_t *		send_head;
    word_t *		stack;

    ringlist_t<tcb_t>	wait_list;
    ringlist_t<tcb_t>	present_list;

    spinlock_t		tcb_lock;

#ifdef CONFIG_SMP
    cpuid_t		cpu;
    ringlist_t<tcb_t>	xcpu_list;
    cpuid_t		xcpu;
    word_t		xcpu_status;
#endif

public:
    /* scheduling */
    word_t		total_quantum;
    word_t		timeslice_length;
    s64_t		current_timeslice;
    prio_t		priority;

private:
    /* pager etc */
    threadid_t		scheduler;

public:
    bitmask_t		flags;
    arch_ktcb_t		arch;

public:
    misc_tcb_t		misc;
    threadid_t		saved_partner;
    thread_state_t	saved_state;
    thread_resources_t	resources;
#ifdef CONFIG_DEBUG
    ringlist_t<tcb_t>	thread_list;
    char                debug_name[16];
#endif

private:
    /* VU: pdir_cache should be architecture-specific!!! */
    word_t		pdir_cache;

    word_t		kernel_stack[0];
    /* do not delete this TCB_END_MARKER */

    /* class friends */
    friend void dump_tcb(tcb_t *);
    friend void handle_ipc_error (void);
    friend class thread_resources_t;
};

/* union to allow allocation of tcb including stack */
union whole_tcb_t {
    u8_t pad[KTCB_SIZE];
} __attribute__((aligned(KTCB_SIZE)));
    

/**********************************************************************
 *
 *               Operations on thread ids and settings
 *
 **********************************************************************/

INLINE threadid_t tcb_t::get_global_id()
{
    return myself_global;
}

INLINE void tcb_t::set_scheduler(const threadid_t tid)
{
    this->scheduler = tid;
}

INLINE threadid_t tcb_t::get_scheduler()
{
    return this->scheduler;
}

INLINE void tcb_t::set_irq_handler(const threadid_t tid)
{
    set_scheduler(tid);
}

INLINE threadid_t tcb_t::get_irq_handler()
{
    return get_scheduler();
}

INLINE void tcb_t::set_send_redirector(const threadid_t redir)
{
    this->send_redirector = redir;
}

INLINE threadid_t tcb_t::get_send_redirector()
{
    return this->send_redirector;
}

INLINE void tcb_t::set_recv_redirector(const threadid_t redir)
{
    this->recv_redirector = redir;
}

INLINE threadid_t tcb_t::get_recv_redirector()
{
    return this->recv_redirector;
}

/**********************************************************************
 *
 *                  Access functions
 *
 **********************************************************************/
INLINE void tcb_t::set_state(thread_state_t state)
{
    this->thread_state = state;
}

INLINE thread_state_t tcb_t::get_state()
{
    return this->thread_state;
}

INLINE void tcb_t::set_partner(threadid_t tid)
{
    this->partner = tid;
}

INLINE threadid_t tcb_t::get_partner()
{
    return this->partner;
}

INLINE tcb_t* tcb_t::get_partner_tcb()
{
    return this->get_space()->get_tcb(partner);
}

/**
 * Get thread ID of a thread's pager
 * @return      thread ID of pager
 */
INLINE threadid_t tcb_t::get_pager()
{
    return get_utcb()->pager;
}

/**
 * Set new pager for a thread
 * @param tid   thread ID of new pager
 */
INLINE void tcb_t::set_pager(const threadid_t tid)
{
    get_utcb()->pager = tid;
}

/**
 * Get thread ID of a thread's exception handler
 * @return      thread ID of exception handler
 */
INLINE threadid_t tcb_t::get_exception_handler()
{
    return get_utcb()->exception_handler;
}

/**
 * Set new exception handler for a thread
 * @param tid   thread ID of new exception handler
 */
INLINE void tcb_t::set_exception_handler(const threadid_t tid)
{
    get_utcb()->exception_handler = tid;
}

/**
 * Get a thread's user-defined handle
 * @return      user-defined handle
 */
INLINE word_t tcb_t::get_user_handle()
{
    return get_utcb()->user_defined_handle;
}

/**
 * Set user-defined handle for a thread
 * @param handle        new value for user-defined handle
 */
INLINE void tcb_t::set_user_handle(const word_t handle)
{
    get_utcb()->user_defined_handle = handle;
}

/**
 * Set the actual sender ID of an IPC
 * @param tid   thread ID of actual sender
 */
INLINE void tcb_t::set_actual_sender (threadid_t tid)
{
    get_utcb()->virtual_sender = tid;
}

/**
 * Get the virtual sender of an IPC
 * @return      thread ID of virtual sender
 */
INLINE threadid_t tcb_t::get_virtual_sender (void)
{
    return get_utcb()->virtual_sender;
}

/**
 * Set the intended receiver of an IPC
 * @return      thread ID of intended receiver
 */
INLINE void tcb_t::set_intended_receiver (threadid_t tid)
{
    get_utcb()->intended_receiver = tid;
}

/**
 * Get the intended receiver of an IPC
 * @return      thread ID of intended receiver
 */
INLINE threadid_t tcb_t::get_intended_receiver (void)
{
    return get_utcb()->intended_receiver;
}

/**
 * Set the IPC error code
 * @param err   new IPC error code
 */
INLINE void tcb_t::set_error_code(word_t err)
{
    get_utcb()->error_code = err;
}

/**
 * Get the IPC error code
 * @return      IPC error code
 */
INLINE word_t tcb_t::get_error_code(void)
{
    return get_utcb()->error_code;
}

/**
 * Get a thread's preemption flags
 * @return      preemption flags
 */
INLINE preempt_flags_t tcb_t::get_preempt_flags (void)
{
    preempt_flags_t flags;
    flags.raw = get_utcb()->preempt_flags;
    return flags;
}

/**
 * Set a thread's preemption flags
 * @param flags new preemption flags
 */
INLINE void tcb_t::set_preempt_flags (preempt_flags_t flags)
{
    get_utcb()->preempt_flags = flags.raw;
}

/**
 * Get a thread's coprocessor flags
 * @return      coprocessor flags
 */
INLINE u8_t tcb_t::get_cop_flags (void)
{
    return get_utcb()->cop_flags;
}

/**
 * clear the notify_bits
 */
INLINE void tcb_t::clear_notify_bits()
{
	get_utcb()->notify_bits = 0;
}

/**
 * add bits to the notify_mask 
 */
INLINE void tcb_t::add_notify_bits(const word_t bits)
{
	get_utcb()->notify_bits |= bits;
}

/**
 * clear bits in the notify_mask 
 */
INLINE void tcb_t::sub_notify_bits(const word_t bits)
{
	get_utcb()->notify_bits &= ~(bits);
}

/**
 * read value of the notify_mask 
 */
INLINE word_t tcb_t::get_notify_bits(void)
{
    return get_utcb()->notify_bits;
}

/**
 * read value of the notify_mask 
 */
INLINE word_t tcb_t::get_notify_mask(void)
{
    return get_utcb()->notify_mask;
}

/**
 * set the value of the notify_mask register
 * @param value value to set
 */
INLINE void tcb_t::set_notify_mask(const word_t mask)
{
    get_utcb()->notify_mask = mask;
}

/**
 * Get message tag
 * @returns message tag
 * The message tag will be read from message register 0
 */
INLINE msg_tag_t tcb_t::get_tag (void)
{
    msg_tag_t tag;
    tag.raw = get_mr(0);
    return tag;
}

/**
 * Set the message tag
 * @param tag   new message tag
 * The message tag will be stored in message register 0
 */
INLINE void tcb_t::set_tag (msg_tag_t tag)
{
    set_mr(0, tag.raw);
}

/**
 * enqueues the tcb into the send queue of tcb
 * @param tcb the thread control block to enqueue
 */
INLINE void tcb_t::enqueue_send(tcb_t * tcb)
{
    ASSERT(NORMAL, !queue_state.is_set(queue_state_t::send) );

    ENQUEUE_LIST_TAIL(tcb->send_head, this, send_list);
    queue_state.set(queue_state_t::send);
}

/**
 * dequeues the tcb from the send queue of tcb
 * @param tcb the thread control block to dequeue from
 */
INLINE void tcb_t::dequeue_send(tcb_t * tcb)
{
    ASSERT(NORMAL, queue_state.is_set(queue_state_t::send) );

    DEQUEUE_LIST(tcb->send_head, this, send_list);
    queue_state.clear(queue_state_t::send);
}

/**
 * enqueue a tcb into the present list
 * the present list primarily exists for debugging reasons, since task 
 * manipulations now happen on a per-thread basis */
#ifdef CONFIG_DEBUG
extern tcb_t * global_present_list;
extern spinlock_t present_list_lock;
#endif

INLINE void tcb_t::enqueue_present()
{
#ifdef CONFIG_DEBUG
    present_list_lock.lock();
    ENQUEUE_LIST_TAIL(global_present_list, this, present_list);
    present_list_lock.unlock();
#endif
}

INLINE void tcb_t::dequeue_present()
{
#ifdef CONFIG_DEBUG
    present_list_lock.lock();
    DEQUEUE_LIST(global_present_list, this, present_list);
    present_list_lock.unlock();
#endif
}

INLINE arch_ktcb_t *tcb_t::get_arch()
{
    return &this->arch;
}

INLINE tcb_t * get_idle_tcb()
{
    extern whole_tcb_t __idle_tcb;
    return (tcb_t*)&__idle_tcb;
}

INLINE void tcb_t::switch_to_idle()
{
    switch_to(get_idle_tcb());
}


/* 
 * include glue header file 
 */
#include INC_GLUE(tcb.h)


/**********************************************************************
 *
 *             global V4 thread management
 *
 **********************************************************************/

INLINE space_t * get_current_space()
{
    return get_current_tcb()->get_space();
}

#ifndef CONFIG_SMP
INLINE cpuid_t get_current_cpu()
{
    return 0;
}

INLINE bool tcb_t::migrate_to_processor(cpuid_t processor)
{
    return processor == 0;
}
#endif

INLINE bool tcb_t::is_local_cpu()
{
    // on non-SMP always evaluates to true
    return (get_current_cpu() == get_cpu());
}


/**
 * creates a root server thread and a fresh space, if the 
 * creation fails the function does not return (assuming that root 
 * servers are functional necessary for the system)
 * 
 * @param dest_tid id of the thread to be created
 * @param scheduler_tid thread id of the scheduler
 * @param pager_tid thread id of the pager
 * @param utcb_area fpage describing the UTCB area
 * @param kip_area fpage describing the kernel interface page area
 * @return the newly created tcb
 */
tcb_t * create_root_server(threadid_t dest_tid, threadid_t scheduler_tid, 
			   threadid_t pager_tid, fpage_t utcb_area, fpage_t kip_area);

/**
 * initializes the root servers
 */
void init_root_servers();

/**
 * initializes the kernel threads
 */
void init_kernel_threads();


#endif /* !__API__V4__TCB_H__ */

