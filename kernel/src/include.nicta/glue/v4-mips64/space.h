/*********************************************************************
 *                
 * Copyright (C) 2002-2003,   University of New South Wales
 *                
 * File path:     glue/v4-mips64/space.h
 * Description:   MIPS64 specific space implementation.
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
 * $Id: space.h,v 1.19 2004/12/09 00:55:48 cvansch Exp $
 *                
 ********************************************************************/
#ifndef __GLUE__V4_MIPS64__SPACE_H__
#define __GLUE__V4_MIPS64__SPACE_H__

#include <debug.h> /* for UNIMPLMENTED() */

#include INC_API(types.h)

#include INC_API(fpage.h)			/* fpage_t	*/
#include INC_API(thread.h)			/* threadid_t	*/
#include INC_API(queueing.h)
#include INC_GLUE(config.h)
#include INC_ARCH(pgent.h)
#include INC_ARCH(tlb.h)
#include <asid.h>

/* forward declarations - space_t depends on tcb_t and utcb_t */
class tcb_t;
class utcb_t;

/**
 * The address space representation
 */
class space_t {

public:
    /* TCB management */
    bool allocate_tcb(addr_t addr);
    void map_dummy_tcb(addr_t addr);
    utcb_t * allocate_utcb (tcb_t * tcb);

    tcb_t * get_tcb(threadid_t tid);
    tcb_t * get_tcb(void * ptr);

    ringlist_t<space_t> get_spaces_list() {
	return spaces_list;
    }
    tcb_t * get_thread_list() {
	return thread_list;
    }
    void enqueue_spaces();
    void dequeue_spaces();

    /* Address ranges */
    inline bool is_user_area (addr_t addr);
    bool is_user_area (fpage_t fpage);
    bool is_tcb_area (addr_t addr);
    bool is_mappable (addr_t addr);
    bool is_mappable (fpage_t addr);

    /* kip and utcb handling */
    fpage_t get_kip_page_area();
    fpage_t get_utcb_page_area();

    /* reference counting */
    void add_tcb(tcb_t * tcb);
    bool remove_tcb(tcb_t * tcb);

    /* space control */
    word_t space_control (word_t ctrl) { return 0; }

    /* space update hooks */
    static void begin_update (void) {}
    static void end_update (void) {}

    bool is_initialized();

    bool map_fpage(fpage_t snd_fpage, word_t snd_base, space_t * dst_space, fpage_t rcv_fpage, bool grant);
    fpage_t unmap_fpage(fpage_t fpage, bool flush, bool unmap_all);
    void init(fpage_t utcb_area, fpage_t kip_area);
    void free();

    enum access_e {
	read, write, readwrite, execute,
    };
    void handle_pagefault(addr_t addr, addr_t ip, access_e access, bool kernel);
    void map_sigma0(addr_t addr);
    bool sync_kernel_space(addr_t addr);
    utcb_t * map_utcb(utcb_t * utcb);
    utcb_t * utcb_to_kernel_space(utcb_t * utcb);

    /* Methods needed by linear page table walker. */
    pgent_t * pgent (word_t num, word_t cpu = 0);
    bool lookup_mapping (addr_t vaddr, pgent_t ** pg,
			 pgent_t::pgsize_e * size);
    bool readmem (addr_t vaddr, word_t * contents);
    static word_t readmem_phys (addr_t paddr)
	{ return *(word_t*)phys_to_virt((word_t*)paddr); }
    void release_kernel_mapping (addr_t vaddr, addr_t paddr, word_t log2size);

    void init_kernel_mappings(void);
    void add_mapping(addr_t vaddr, addr_t paddr, pgent_t::pgsize_e size, bool writable, bool kernel);

    /* TLB releated methods used by linear page table walker. */
    void flush_tlb (space_t * curspace);
    void flush_tlbent (space_t * curspace, addr_t vaddr, word_t log2size);
    bool does_tlbflush_pay (word_t log2size)
	{ return false; }

    /* MIPS64 specific */
    asid_t *get_asid(void) 
	{ return &asid; }

private:

    union {
	struct {
	    u64_t	maps[(1 << 10) - 8];
	    fpage_t	kip_area;
	    fpage_t	utcb_area;
	    word_t	thread_count;
	    ringlist_t<space_t>	spaces_list;
	    tcb_t *	thread_list;
	    /* MIPS sepecific */
	    asid_t asid;
	};
	u64_t pagedir[(1 << 10)];
    };
};


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
#ifdef CONFIG_DEBUG
    spaces_list_lock.lock();
    ENQUEUE_LIST_TAIL(global_spaces_list, this, spaces_list);
    spaces_list_lock.unlock();
#endif
}

INLINE void space_t::dequeue_spaces()
{
#ifdef CONFIG_DEBUG
    spaces_list_lock.lock();
    DEQUEUE_LIST(global_spaces_list, this, spaces_list);
    spaces_list_lock.unlock();
#endif
}

/**
 * get the KIP area of an address space
 * @returns the KIP area of the address space as an fpage
 */
INLINE fpage_t space_t::get_kip_page_area (void)
{
    return kip_area;
}

/**
 * get the UTCB area of an address space
 * @returns the utcb area of the address space as an fpage
 */
INLINE fpage_t space_t::get_utcb_page_area (void)
{
    return utcb_area;
}

INLINE bool space_t::is_user_area (addr_t addr)
{
    return (addr >= (addr_t) USER_AREA_START && 
	    addr <= (addr_t) USER_AREA_END);
}
        
INLINE bool space_t::is_tcb_area (addr_t addr)
{
    /* carl page table hack */
    addr = (addr_t)((word_t)addr & (~(1UL << (hw_pgshifts[pgent_t::size_max+1]-1))));
    return (addr >= (addr_t) KTCB_AREA_START &&
	    addr <= (addr_t) KTCB_AREA_END);
}

INLINE space_t* get_kernel_space()
{
    extern space_t * kernel_space;
    return kernel_space;
}

INLINE space_t * get_saved_pagetable(void)
{
    word_t  saved_page_table;
    asm (
	"dmfc0	%0, "STR(CP0_CONTEXT)"\n\t"
	"dsra	%0, %0, 32\n\t"
	: "=r" (saved_page_table)
    );

    return (space_t *) saved_page_table;
};


/**
 * translates a global thread ID into a valid tcb pointer
 * @param tid thread ID
 * @returns pointer to the TCB of thread tid
 */
INLINE tcb_t * space_t::get_tcb( threadid_t tid )
{
    return (tcb_t *) (KTCB_AREA_START +
		    ((tid.get_threadno() & (VALID_THREADNO_MASK)) * KTCB_SIZE));
}

/**
 * translates a pointer within a tcb into a valid tcb pointer
 * @param ptr pointer to somewhere in the TCB
 * @returns pointer to the TCB
 */
INLINE tcb_t * space_t::get_tcb (void * ptr)
{
    return (tcb_t *)((word_t)(ptr) & KTCB_MASK);
}

/* Page table access functions */

INLINE pgent_t *space_t::pgent (word_t num, word_t cpu)
{
    return ((pgent_t *)this)->next(this, pgent_t::size_max, num);
}

INLINE void space_t::flush_tlb(space_t * curspace)
{
    flush_tc (this->get_asid()->get());
}

INLINE void space_t::flush_tlbent(space_t * curspace, addr_t vaddr, word_t log2size)
{
    purge_tc (vaddr, log2size, this->get_asid()->get());
}

void init_kernel_space(void);

#endif /* !__GLUE__V4_MIPS64__SPACE_H__ */
