/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  Karlsruhe University
 *                
 * File path:     glue/v4-ia32/space.h
 * Description:   ia32-specific space implementation
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
 * $Id: space.h,v 1.48 2004/12/09 00:55:47 cvansch Exp $
 *                
 ********************************************************************/
#ifndef __GLUE__V4_IA32__SPACE_H__
#define __GLUE__V4_IA32__SPACE_H__

#include INC_API(types.h)

#include <debug.h>
#include INC_API(fpage.h)
#include INC_API(thread.h)
#include INC_API(queueing.h)
#include INC_ARCH(ptab.h)
#include INC_ARCH(pgent.h)
#include INC_GLUE(config.h)

#if defined(CONFIG_IA32_SMALL_SPACES)
#include INC_GLUE(smallspaces.h)
#include INC_ARCH(segdesc.h)
#endif


class utcb_t;
class tcb_t;
class pgent_t;

class space_t
{
public:
    enum access_e {
	read		= 0,
	write		= 2,
	readwrite	= -1,
	execute		= 0
    };

    void init(fpage_t utcb_area, fpage_t kip_area);
    void free();
    bool sync_kernel_space(addr_t addr);
    void handle_pagefault(addr_t addr, addr_t ip, access_e access, bool kernel);
    bool is_initialized();

    ringlist_t<space_t> get_spaces_list() {
	return x[CONFIG_SMP_MAX_CPUS].u.shadow.spaces_list;
    }
    tcb_t * get_thread_list() {
	return x[CONFIG_SMP_MAX_CPUS].u.shadow.thread_list;
    }
    void enqueue_spaces();
    void dequeue_spaces();

    /* mapping */
    void map_sigma0(addr_t addr);
    bool map_fpage(fpage_t snd_fp, word_t base, 
	space_t * t_space, fpage_t rcv_fp, bool grant);
    fpage_t unmap_fpage(fpage_t fpage, bool flush, bool unmap_all);

    
    /* tcb management */
    bool allocate_tcb(addr_t addr);
    void map_dummy_tcb(addr_t addr);
    utcb_t *  allocate_utcb(tcb_t * tcb);

    tcb_t * get_tcb(threadid_t tid);
    tcb_t * get_tcb(void * ptr);

    /* address ranges */
    bool is_user_area(addr_t addr);
    bool is_user_area(fpage_t fpage);
    bool is_tcb_area(addr_t addr);
    bool is_mappable(addr_t addr);
    bool is_mappable(fpage_t fpage);

    /* kip and utcb handling */
    fpage_t get_kip_page_area();
    fpage_t get_utcb_page_area();

    /* reference counting */
    void add_tcb(tcb_t * tcb);
    bool remove_tcb(tcb_t * tcb);

    /* space control */
    word_t space_control (word_t ctrl);

#if defined(CONFIG_IA32_SMALL_SPACES)
    bool make_small (smallspace_id_t id);
    void make_large (void);
    bool is_small (void);
    bool is_smallspace_area (addr_t addr);
    bool sync_smallspace (addr_t addr);
    word_t smallspace_offset (void);
    word_t smallspace_size (void);
#   define HAVE_ARCH_FREE_SPACE
    void arch_free (void);
#endif

public:
    /* ia-32 specific functions */
    void init_kernel_mappings();
    void init_cpu_mappings(cpuid_t cpu);
    ia32_pgent_t * get_pdir(cpuid_t cpu = 0);
    bool lookup_mapping (addr_t vaddr, pgent_t ** r_pg,
			 pgent_t::pgsize_e * r_size);
    bool readmem (addr_t vaddr, word_t * contents);
    static word_t readmem_phys (addr_t paddr);

    void release_kernel_mapping (addr_t vaddr, addr_t paddr, word_t log2size);

    void flush_tlb (space_t * curspace);
    void flush_tlbent (space_t * curspace, addr_t vaddr, word_t log2size);
    bool does_tlbflush_pay (word_t log2size)
	{ return log2size >= 32; }

    /* user space access */
    u8_t get_from_user(addr_t);

    /* update hooks */
    static void begin_update() { }
    static void end_update();

    /* generic page table walker */
    pgent_t * pgent (word_t num, word_t cpu = 0);
    void add_mapping(addr_t vaddr, addr_t paddr, pgent_t::pgsize_e size, bool writable, bool kernel, bool global, bool cacheable = true);
    void remap_area(addr_t vaddr, addr_t paddr, pgent_t::pgsize_e pgsize, word_t len, bool writable, bool kernel, bool global);

private:
    union {
	ia32_pgent_t pdir[1024 * (CONFIG_SMP_MAX_CPUS + 1)];
	struct {
	    ia32_pgent_t user[USER_AREA_END >> IA32_PAGEDIR_BITS];
	    ia32_pgent_t small[SMALLSPACE_AREA_SIZE >> IA32_PAGEDIR_BITS];
	    ia32_pgent_t unused_area[UNUSED_AREA_COUNT][UNUSED_AREA_SIZE >> IA32_PAGEDIR_BITS];
	    ia32_pgent_t readmem_area;

	    // Abuse some pagedir enries.  Make sure that either the
	    // valid bit (bit 0) is never set, or that the shadow page
	    // table entry is used.
	    union {
		struct {
#if defined(CONFIG_IA32_SMALL_SPACES)
		    space_t		*prev;
		    space_t		*next;
#endif
		} real;

		struct {
		    fpage_t		kip_area;
		    fpage_t		utcb_area;
		    word_t		thread_count;
		    ringlist_t<space_t>	spaces_list;
		    tcb_t *		thread_list;
#if defined(CONFIG_IA32_SMALL_SPACES)
		    smallspace_id_t	smallid;
		    ia32_segdesc_t	segdesc;
#endif
		} shadow;
	    } u;

	    // afterwards, we have MYUTCB, APIC, TCBs, Kernel code
	    word_t __pad [1024 - (PAGEDIR_STUFF_END >> IA32_PAGEDIR_BITS)];
	} x [CONFIG_SMP_MAX_CPUS + 1] __attribute__((aligned(IA32_PAGE_SIZE)));
    };

public:

    /*
     * Access functions for abused pdir entries.
     */

    void set_kip_area (fpage_t f)
	{ x[CONFIG_SMP_MAX_CPUS].u.shadow.kip_area = f; }

    fpage_t get_kip_area (void)
	{ return x[CONFIG_SMP_MAX_CPUS].u.shadow.kip_area; }

    void set_utcb_area (fpage_t f)
	{ x[CONFIG_SMP_MAX_CPUS].u.shadow.utcb_area = f; }

    fpage_t get_utcb_area (void)
	{ return x[CONFIG_SMP_MAX_CPUS].u.shadow.utcb_area; }

    void set_thread_count (word_t c)
	{ x[CONFIG_SMP_MAX_CPUS].u.shadow.thread_count = c; }

    word_t get_thread_count (void)
	{ return x[CONFIG_SMP_MAX_CPUS].u.shadow.thread_count; }

#if defined(CONFIG_IA32_SMALL_SPACES)
    smallspace_id_t * smallid (void)
	{ return &x[CONFIG_SMP_MAX_CPUS].u.shadow.smallid; }
    
    ia32_segdesc_t * segdesc (void)
	{ return &x[CONFIG_SMP_MAX_CPUS].u.shadow.segdesc; }

    space_t * get_prev (void) { return x[0].u.real.prev; }
    space_t * get_next (void) { return x[0].u.real.next; }
    void set_prev (space_t * p) { x[0].u.real.prev = p; }
    void set_next (space_t * n) { x[0].u.real.next = n; }

    void dequeue_polluted (void);
    void enqueue_polluted (void);
#endif
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
    ENQUEUE_LIST_TAIL(global_spaces_list, this, x[CONFIG_SMP_MAX_CPUS].u.shadow.spaces_list);
    spaces_list_lock.unlock();
#endif
}

INLINE void space_t::dequeue_spaces()
{
#ifdef CONFIG_DEBUG
    spaces_list_lock.lock();
    DEQUEUE_LIST(global_spaces_list, this, x[CONFIG_SMP_MAX_CPUS].u.shadow.spaces_list);
    spaces_list_lock.unlock();
#endif
}

/**********************************************************************
 *
 *                      inline functions
 *
 **********************************************************************/

INLINE ia32_pgent_t * space_t::get_pdir(cpuid_t cpu)
{
#if !defined(CONFIG_SMP)
    ASSERT(DEBUG, cpu == 0);
#else
    ASSERT(DEBUG, cpu < CONFIG_SMP_MAX_CPUS);
#endif
    return virt_to_phys(&pdir[cpu * 1024]);
}

#if defined(CONFIG_IA32_SMALL_SPACES)
INLINE bool space_t::is_small (void)
{
    return smallid ()->is_small ();
}

INLINE bool space_t::is_smallspace_area (addr_t addr)
{
    return (addr >= (addr_t) SMALLSPACE_AREA_START && 
	    addr < (addr_t) SMALLSPACE_AREA_END);
}

INLINE word_t space_t::smallspace_offset (void)
{
    return smallid ()->offset () + SMALLSPACE_AREA_START;
}

INLINE word_t space_t::smallspace_size (void)
{
    return smallid ()->size ();
}
#endif /* CONFIG_IA32_SMALL_SPACES */

INLINE bool space_t::is_user_area(addr_t addr)
{
    return (addr >= (addr_t)USER_AREA_START && 
	    addr < (addr_t)USER_AREA_END);
}
	
INLINE bool space_t::is_tcb_area(addr_t addr)
{
    return (addr >= (addr_t)KTCB_AREA_START &&
	    addr < (addr_t)KTCB_AREA_END);
}

INLINE fpage_t space_t::get_kip_page_area()
{
    return get_kip_area ();
}

INLINE fpage_t space_t::get_utcb_page_area()
{
    return get_utcb_area ();
}

INLINE u8_t space_t::get_from_user(addr_t addr)
{
#if defined(CONFIG_IA32_SMALL_SPACES)
    if (EXPECT_FALSE (is_small ()))
	return *(u8_t *) (smallspace_offset () + (word_t) addr);
#endif
    return *(u8_t *) (addr);
}

/**
 * translates a threadid into a tcb pointer
 * @param tid thread id of the thread
 * @return pointer to thread control block
 */
INLINE tcb_t * space_t::get_tcb(threadid_t tid)
{
    return (tcb_t*)((KTCB_AREA_START) + ((tid.get_raw() >> THREADNO_SHIFT) & (THREADNO_MASK << KTCB_BITS)));
}

INLINE tcb_t * space_t::get_tcb(void * ptr)
{
   return (tcb_t*)((word_t)(ptr) & KTCB_MASK);
}

INLINE pgent_t * space_t::pgent (word_t num, word_t cpu)
{
    return ((pgent_t *) phys_to_virt (get_pdir (cpu)))->
	next (this, pgent_t::size_4m, num);
}

#if defined(CONFIG_IA32_SMALL_SPACES)
#define IS_SPACE_SMALL(s)	(s)->is_small ()
#else
#define IS_SPACE_SMALL(s)	false
#endif

#if defined(CONFIG_IA32_SMALL_SPACES_GLOBAL)
#define IS_SPACE_GLOBAL(s)	(s)->is_small ()
#else
#define IS_SPACE_GLOBAL(s)	false
#endif

#ifndef CONFIG_SMP
/**
 * Flush complete TLB
 */
INLINE void space_t::flush_tlb (space_t * curspace)
{
    if (this == curspace || IS_SPACE_SMALL (this))
	ia32_mmu_t::flush_tlb (IS_SPACE_GLOBAL (this));
}

/**
 * Flush a specific TLB entry
 * @param addr	virtual address of TLB entry
 */
INLINE void space_t::flush_tlbent (space_t * curspace, addr_t addr,
				   word_t log2size)
{
    if (this == curspace || IS_SPACE_SMALL (this))
	ia32_mmu_t::flush_tlbent ((u32_t) addr);
}

/**
 * Update functions are empty in non-SMP case
 */
inline void space_t::end_update() { }

#else

INLINE void set_active_space (cpuid_t cpu, space_t * space)
{
    /* glue/space.cc */
    extern struct {
	space_t * space;
	char __pad [CACHE_LINE_SIZE - sizeof(space_t*)];
    } active_cpu_space[CONFIG_SMP_MAX_CPUS];
    
    if (space)
	active_cpu_space[cpu].space = space;
}
#endif /* !CONFIG_SMP */

/**********************************************************************
 *
 *                 global function declarations
 *
 **********************************************************************/

INLINE space_t* get_kernel_space()
{
    extern space_t * kernel_space;
    return kernel_space;
}

/**
 * converts the current page table into a space_t
 */
INLINE space_t * ptab_to_space(u32_t ptab, cpuid_t cpu = 0)
{
#if defined(CONFIG_SMP)
    return phys_to_virt((space_t*)(ptab - (cpu * IA32_PAGE_SIZE)));
#else
    return phys_to_virt((space_t*)(ptab));
#endif
}

INLINE void reload_user_segregs (void)
{
    asm volatile (
	"	movl %0, %%es	\n"
#if !defined(CONFIG_TRACEBUFFER)
	"	movl %0, %%fs	\n"
#endif
	"	movl %1, %%gs	\n"
	:
	: "r" (IA32_UDS), "r" (IA32_UTCB));
}

void init_kernel_space();

#endif /* !__GLUE__V4_IA32__SPACE_H__ */
