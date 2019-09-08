/****************************************************************************
 *
 * Copyright (C) 2002, Karlsruhe University
 *
 * File path:	glue/v4-tmplarch/space.h
 * Description:	Template class definition for space_t.
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
 * $Id: space.h,v 1.16 2004/12/09 02:35:21 cvansch Exp $
 *
 ***************************************************************************/

#include INC_API(fpage.h)			/* fpage_t	*/
#include INC_API(thread.h)			/* threadid_t	*/
#include INC_API(queueing.h)

#include INC_GLUE(config.h)

/* forward declarations - space_t depends on tcb_t and utcb_t */
class tcb_t;
class utcb_t;

/**
 * The address space representation
 */
class space_t {
public:
    /**
     * Test whether an address is in a mappable page, that is, no
     * kernel pages, not the KIP and not within the UTCB area.
     * @param addr address to test
     * Returns true if address is in a mappable page.
     */
    bool is_mappable(addr_t addr);

    /**
     * test whether an fpage is mappable
     */
    /* Address ranges */
    bool is_mappable(fpage_t);
    bool is_user_area(addr_t);
    bool is_user_area(fpage_t);
    bool is_tcb_area(addr_t addr);

    /* kip and utcb handling */
    fpage_t get_kip_page_area();
    fpage_t get_utcb_page_area();

    /* space control */
    word_t space_control (word_t ctrl) { return 0; }

    bool is_initialized();

    tcb_t * get_tcb(threadid_t tid);
    tcb_t * get_tcb(void *ptr);

    ringlist_t<space_t> get_spaces_list() {
#warning PORTME
	UNIMPLEMENTED();
	return NULL;
    }
    tcb_t * get_thread_list() {
#warning PORTME
	UNIMPLEMENTED();
	return NULL;
    }
    void enqueue_spaces();
    void dequeue_spaces();

    bool map_fpage(fpage_t snd_fp, word_t base, space_t * t_space, fpage_t rcv_fp, bool grant);
    fpage_t unmap_fpage(fpage_t fpage, bool flush, bool unmap_all);
    bool allocate_utcb(tcb_t * tcb, utcb_t * &kern_utcb, utcb_t * &user_utcb);
    void init(fpage_t utcb_area, fpage_t kip_area);
    void add_tcb(tcb_t * tcb);
    bool remove_tcb(tcb_t * tcb);
    void allocate_tcb(addr_t addr);
    void free();

    enum access_e {
	read, write, readwrite, execute,
    };
    void handle_pagefault(addr_t addr, addr_t ip, access_e access, bool kernel);
    void map_sigma0(addr_t addr);
    bool sync_kernel_space(addr_t addr);
    utcb_t * map_utcb(utcb_t * utcb);
    void map_dummy_tcb(addr_t addr);
    utcb_t * utcb_to_kernel_space(utcb_t * utcb);

    /* Methods needed by linear page table walker. */
    pgent_t * pgent (word_t num, word_t cpu = 0);
    bool lookup_mapping (addr_t vaddr, pgent_t ** pg,
			 pgent_t::pgsize_e * size);
    bool readmem (addr_t vaddr, word_t * contents);
    static word_t readmem_phys (addr_t paddr)
	{ return *phys_to_virt((word_t*)paddr); }
    void release_kernel_mapping (addr_t vaddr, addr_t paddr, word_t log2size);

    /* TLB releated methods used by linear page table walker. */
    void flush_tlb (space_t * curspace);
    void flush_tlbent (space_t * curspace, addr_t vaddr, word_t log2size);
    bool does_tlbflush_pay (word_t log2size);

    /* Update hooks to allow for efficient XCPU TLB updates and
     * invalidations, invoked by the linear ptab walker */
    static void begin_update();
    static void end_update();
};


/**
 * get the KIP area of an address space
 * @returns the KIP area of the address space as an fpage
 */
INLINE fpage_t space_t::get_kip_page_area (void)
{
#warning PORTME
    UNIMPLEMENTED();
    return fpage_t::nilpage();
    //return kip_area;
}

/**
 * enqueue a spaces into the spaces list
 * the present list primarily exists for debugging reasons
 */
#ifdef CONFIG_DEBUG
extern space_t * global_spaces_list;
extern spinlock_t spaces_list_lock;
#endif

INLINE void space_t::enqueue_spaces()
{
    UNIMPLEMENTED();
#ifdef CONFIG_DEBUG
//    spaces_list_lock.lock();
//    ENQUEUE_LIST_TAIL(global_spaces_list, this, spaces_list);
//    spaces_list_lock.unlock();
#endif
}

INLINE void space_t::dequeue_spaces()
{
    UNIMPLEMENTED();
#ifdef CONFIG_DEBUG
//    spaces_list_lock.lock();
//    DEQUEUE_LIST(global_spaces_list, this, spaces_list);
//    spaces_list_lock.unlock();
#endif
}

/**
 * get the UTCB area of an address space
 * @returns the utcb area of the address space as an fpage
 */
INLINE fpage_t space_t::get_utcb_page_area (void)
{
#warning PORTME
    UNIMPLEMENTED();
    return fpage_t::nilpage();
    //return utcb_area;
}

INLINE bool space_t::is_user_area (addr_t addr)
{
#warning PORTME
    UNIMPLEMENTED();
    return false;
}
        
INLINE bool space_t::is_tcb_area (addr_t addr)
{
#warning PORTME
    UNIMPLEMENTED();
    return false;
}



#warning PORTME
INLINE space_t* get_kernel_space()
{
    UNIMPLEMENTED();
    return NULL;

    //extern space_t * kernel_space;
    //return kernel_space;
}


/**
 * translates a global thread ID into a valid tcb pointer
 * @param tid thread ID
 * @returns pointer to the TCB of thread tid
 */
INLINE tcb_t * space_t::get_tcb( threadid_t tid )
{
#warning PORTME
    UNIMPLEMENTED();

    return NULL;
}

/**
 * translates a pointer within a tcb into a valid tcb pointer
 * @param ptr pointer to somewhere in the TCB
 * @returns pointer to the TCB
 */
INLINE tcb_t * space_t::get_tcb (void * ptr)
{
#warning PORTME
    UNIMPLEMENTED();

    return NULL;
}


