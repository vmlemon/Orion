/*********************************************************************
 *                
 * Copyright (C) 2002, 2004-2003,  Karlsruhe University
 *                
 * File path:     glue/v4-amd64/space.h
 * Description:   
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
 * $Id: space.h,v 1.7 2004/12/09 00:55:46 cvansch Exp $
 * 
 ********************************************************************/
#ifndef __GLUE__V4_AMD64__SPACE_H__
#define __GLUE__V4_AMD64__SPACE_H__

#include INC_API(types.h)	
#include INC_API(fpage.h)	
#include INC_API(thread.h)	
#include INC_API(queueing.h)
#include INC_ARCH(ptab.h)
#include INC_ARCH(pgent.h)      
#include INC_GLUE(config.h)

/* forward declarations - space_t depends on tcb_t and utcb_t */
class tcb_t;
class utcb_t;

/**
 * The address space representation
 */
class space_t {
public:
    enum access_e {
	read		= 0,
	write		= 2,
	readwrite	= -1,
	execute		= 16
    };

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

    bool is_initialized();

    tcb_t * get_tcb(threadid_t tid);
    tcb_t * get_tcb(void *ptr);

    ringlist_t<space_t> get_spaces_list() {
	return x[CONFIG_SMP_MAX_CPUS].spaces_list;
    }
    tcb_t * get_thread_list() {
	return x[CONFIG_SMP_MAX_CPUS].thread_list;
    }
    void enqueue_spaces();
    void dequeue_spaces();

    bool map_fpage(fpage_t snd_fp, word_t base, space_t * t_space, fpage_t rcv_fp, bool grant);
    fpage_t unmap_fpage(fpage_t fpage, bool flush, bool unmap_all);
    utcb_t * allocate_utcb(tcb_t * tcb);
    void init(fpage_t utcb_area, fpage_t kip_area);
    void add_tcb(tcb_t * tcb);
    bool remove_tcb(tcb_t * tcb);
    void allocate_tcb(addr_t addr);
    void free();

    /* space control */
    word_t space_control (word_t ctrl) { return 0; }

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
	{ 
	    ASSERT( (word_t) paddr < (1ULL << 32));
	    return * (word_t *) ( (word_t) paddr + REMAP_32BIT_START); 
	}
    
    void release_kernel_mapping (addr_t vaddr, addr_t paddr, word_t log2size);

    /* TLB releated methods used by linear page table walker. */
    void flush_tlb (space_t * curspace);
    void flush_tlbent (space_t * curspace, addr_t vaddr, word_t log2size);
    bool does_tlbflush_pay (word_t log2size) { return false; };

public:
    /* X86-64 specific functions */
    void init_kernel_mappings();
    amd64_pgent_t *get_pml4(cpuid_t cpu=0);
    word_t get_from_user(addr_t); 
    
    /* generic page table walker */
    void add_mapping(addr_t vaddr, addr_t paddr, pgent_t::pgsize_e size, bool writable, bool kernel, bool global = false, bool cacheable = true);
    void remap_area(addr_t vaddr, addr_t paddr, pgent_t::pgsize_e pgsize, word_t len, bool writable, bool kernel, bool global = false);
    void free_pagetables(fpage_t fpage);

    /* update hooks */
     static void begin_update() { }
     static void end_update() { }  

private:
    union {
	/* 1 per CPU + shadow pgt */
	amd64_pgent_t pml4[512 * (CONFIG_SMP_MAX_CPUS + 1)];
	struct {
	    amd64_pgent_t user[AMD64_PML4_IDX(USER_AREA_END)];
	    /* These are stored in the shadow pagetable */
	    fpage_t kip_area;
	    fpage_t utcb_area;
	    word_t thread_count;
	    amd64_pgent_t kernel_area;
	    ringlist_t<space_t>	spaces_list;
	    tcb_t * thread_list;
         } x [CONFIG_SMP_MAX_CPUS] __attribute__((aligned(AMD64_PTAB_BYTES)));
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
    ENQUEUE_LIST_TAIL(global_spaces_list, this, x[CONFIG_SMP_MAX_CPUS].spaces_list);
    spaces_list_lock.unlock();
#endif
}

INLINE void space_t::dequeue_spaces()
{
#ifdef CONFIG_DEBUG
    spaces_list_lock.lock();
    DEQUEUE_LIST(global_spaces_list, this, x[CONFIG_SMP_MAX_CPUS].spaces_list);
    spaces_list_lock.unlock();
#endif
}

/**
 * get the 
 * @returns the KIP area of the address space as an fpage
 */

INLINE amd64_pgent_t * space_t::get_pml4(cpuid_t cpu)
{
#if !defined(CONFIG_SMP)
    ASSERT(cpu == 0);
#else
    ASSERT(cpu < CONFIG_SMP_MAX_CPUS);
#endif
    return virt_to_phys(&pml4[cpu * 512]);
}

/**
 * get the KIP area of an address space
 * @returns the KIP area of the address space as an fpage
 */
INLINE fpage_t space_t::get_kip_page_area (void)
{
    return x[CONFIG_SMP_MAX_CPUS].kip_area;
}

/**
 * get the UTCB area of an address space
 * @returns the utcb area of the address space as an fpage
 */
INLINE fpage_t space_t::get_utcb_page_area (void)
{
    return x[CONFIG_SMP_MAX_CPUS].utcb_area;
}

INLINE bool space_t::is_user_area (addr_t addr)
{
    return (((word_t) addr | AMD64_SIGN_EXTENSION) >= USER_AREA_START &&
	    ((word_t) addr | AMD64_SIGN_EXTENSION) < USER_AREA_END);
}
        
INLINE bool space_t::is_tcb_area (addr_t addr)
{
    return (((word_t) addr | AMD64_SIGN_EXTENSION) >= KTCB_AREA_START &&
	    ((word_t) addr | AMD64_SIGN_EXTENSION) < KTCB_AREA_END);
}





INLINE space_t* get_kernel_space()
{
    extern space_t * kernel_space;
    return kernel_space;
}


/**
 * translates a global thread ID into a valid tcb pointer
 * @param tid thread ID
 * @returns pointer to the TCB of thread tid
 */
INLINE tcb_t * space_t::get_tcb( threadid_t tid )
{
    return (tcb_t*)((KTCB_AREA_START) + ((tid.get_threadno() & VALID_THREADNO_MASK) * KTCB_SIZE)); 
    
}



INLINE word_t space_t::get_from_user(addr_t addr)
{
    return *(word_t *)(addr);
}

/**
 * translates a pointer within a tcb into a valid tcb pointer
 * @param ptr pointer to somewhere in the TCB
 * @returns pointer to the TCB
 */
INLINE tcb_t * space_t::get_tcb (void * ptr)
{
    return (tcb_t*)((word_t)(ptr) & KTCB_MASK);
}


/**
 * Get PML4 entry
 */

INLINE pgent_t * space_t::pgent (word_t num, word_t cpu)
{
    return ((pgent_t *) phys_to_virt (get_pml4(cpu)))->
	next (this, pgent_t::size_512g, num);
}

/**
 * Flush complete TLB
 */
INLINE void space_t::flush_tlb (space_t * curspace)
{
    if (this == curspace)
	amd64_mmu_t::flush_tlb ();
}

/**
 * Flush a specific TLB entry
 * @param addr  virtual address of TLB entry
 */
INLINE void space_t::flush_tlbent (space_t * curspace, addr_t addr,
				   word_t log2size)
{
    if (this == curspace)
	amd64_mmu_t::flush_tlbent ((u64_t) addr);
}


/**
 * Try to copy a mapping from kernel space into the current space
 * @param addr the address for which the mapping should be copied
 * @return true if something was copied, false otherwise.
 * Synchronization must happen at the highest level, allowing sharing.
 */
INLINE bool space_t::sync_kernel_space(addr_t addr)
{ 
    /* 
     * don't need to sync since TCB pointers are included in the single
     * PML4 kernel area entry
     */
    return false;
}


/**********************************************************************
 *
 *                 global function declarations
 *
 **********************************************************************/

/**
 * converts the current page table into a space_t
 */
INLINE space_t * pml4_to_space(u64_t pml4, cpuid_t cpu = 0)
{
#if defined(CONFIG_SMP)
    return phys_to_virt((space_t*)(pml4 - (cpu * 4096)));
#else
    return phys_to_virt((space_t*)(pml4));
#endif
}

void init_kernel_space();
/**
 * init kernel space
 */
void SECTION(".init.memory") init_kernel_space();



#endif /* !__GLUE__V4_AMD64__SPACE_H__ */
