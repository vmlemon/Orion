/****************************************************************************
 *
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *
 * File path:	glue/v4-arm/space.h
 * Description:	ARM specific space implementation.
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
 * $Id: space.h,v 1.19 2004/12/09 00:55:46 cvansch Exp $
 *
 ***************************************************************************/

#ifndef __GLUE__V4_ARM__SPACE_H
#define __GLUE__V4_ARM__SPACE_H

#include INC_API(fpage.h)			/* fpage_t	*/
#include INC_API(thread.h)			/* threadid_t	*/
#include INC_API(queueing.h)
#include INC_ARCH(fass.h)
#include INC_ARCH(pgent.h)
#include INC_CPU(cache.h)
#include INC_GLUE(config.h)

#define HAVE_ARCH_FREE_SPACE

/* forward declarations - space_t depends on tcb_t and utcb_t */
class tcb_t;
class utcb_t;

class utcb_bitmap_t {
public:
    union {
	struct {
	    BITFIELD2(word_t,
		zero	: 2,
		bits	: 30
	    );
	} x;
	word_t raw;
    };
};

/**
 * The address space representation
 */
class space_t {
public:
    /* TCB management */
    bool allocate_tcb(addr_t addr);
    void map_dummy_tcb(addr_t addr);
    utcb_t * allocate_utcb(tcb_t * tcb);

    tcb_t * get_tcb(threadid_t tid);
    tcb_t * get_tcb(void *ptr);

    ringlist_t<space_t> get_spaces_list() {
	return pt.pd_split.spaces_list;
    }
    tcb_t * get_thread_list() {
	return pt.pd_split.thread_list;
    }
    void enqueue_spaces();
    void dequeue_spaces();

    /* Address ranges */
    bool is_user_area(addr_t);
    bool is_user_area(fpage_t);
    bool is_tcb_area(addr_t addr);
    bool is_utcb_area(addr_t addr);
    bool is_domain_area(addr_t addr);
    bool is_mappable(addr_t addr);
    bool is_mappable(fpage_t);

    /* kip and utcb handling */
    fpage_t get_kip_page_area();
    fpage_t get_utcb_page_area();

    /* reference counting */
    void add_tcb(tcb_t * tcb);
    bool remove_tcb(tcb_t * tcb);

    /* space control */
    word_t space_control (word_t ctrl) { 
#ifdef CONFIG_ENABLE_FASS
        set_pid(ctrl & 0x7f);	/* Only 7 LSBs significant for PID */ 
#endif
	set_vspace(ctrl >> 16);
        return 0; 
    }

    /* space update hooks */
    static void begin_update (void) {}
    static void end_update (void) {}

    bool is_initialized();

    bool map_fpage(fpage_t snd_fp, word_t base, space_t * t_space, 
            fpage_t rcv_fp, bool grant);
    fpage_t unmap_fpage(fpage_t fpage, bool flush, bool unmap_all);
    void init(fpage_t utcb_area, fpage_t kip_area);
    void free();
    void arch_free();

    enum access_e {
	read = 0,
	write = 1,
	readwrite = -1,
	execute = 2
    };

    void handle_pagefault(addr_t addr, addr_t ip, access_e access, bool kernel);
    void map_sigma0(addr_t addr);
    bool sync_kernel_space(addr_t addr);

    /* Methods needed by linear page table walker. */
    pgent_t * pgent (word_t num, word_t cpu = 0);
    bool lookup_mapping (addr_t vaddr, pgent_t ** pg,
			 pgent_t::pgsize_e * size);
    bool readmem (addr_t vaddr, word_t * contents);
    word_t readmem_phys (addr_t paddr);

    void release_kernel_mapping (addr_t vaddr, addr_t paddr, word_t log2size);

    void init_kernel_mappings(void);

    void add_mapping(addr_t vaddr, addr_t paddr, pgent_t::pgsize_e size,
            bool writeable, bool kernel, memattrib_e attrib);

    inline void add_mapping(addr_t vaddr, addr_t paddr, pgent_t::pgsize_e size,
            bool writeable, bool kernel)
    {
	add_mapping(vaddr, paddr, size, writeable, kernel, cached);
    }

    void remove_mapping(addr_t vaddr, pgent_t::pgsize_e size,
	    bool kernel);

    /* TLB releated methods used by linear page table walker. */
    void flush_tlb (space_t * curspace);
    void flush_tlbent (space_t * curspace, addr_t vaddr, word_t log2size);
    inline bool does_tlbflush_pay(word_t log2size)
    { return true; }    

#ifdef CONFIG_ENABLE_FASS
    arm_domain_t get_domain(void);
    void set_domain(arm_domain_t new_domain);
    arm_pid_t get_pid(void); 
    void set_pid(arm_pid_t pid);
#endif

    /* Virtual space for single address space support */
    void set_vspace(word_t vspace)
    {
	this->pt.pd_split.vspace = vspace;
    }
    word_t get_vspace(void)
    {
	return this->pt.pd_split.vspace;
    }

    word_t get_utcb_section() {
	return this->pt.pd_split.utcb_section >> 2;
    }

    void set_utcb_section(word_t section) {
	this->pt.pd_split.utcb_section = section << 2;
    }

    union pt_u {
	/* Pagetable has no shaddow pdir because 1MB user mappings not allowed */
	struct {
	    pgent_t pdir  [1 << ARM_SECTION_BITS];
	};
	struct {
	    pgent_t user_area[USER_AREA_SECTIONS];
	    pgent_t utcb_area[UTCB_AREA_SECTIONS];
	    pgent_t ktcb_area[KTCB_AREA_SECTIONS];
	    pgent_t kernel_area[KERNEL_AREA_SECTIONS];
	    pgent_t uncache_area[UNCACHE_AREA_SECTIONS];

	    /* Should be cache aligned here */

	    /* (ht) This is tricky - potentially a security problem.
	     * All of these variables _must_ have their lower two bits
	     * set to 00, since that will give a fault. 
	     */
	    union {
		struct {
		    word_t domain;
		    union {
			struct {
			    BITFIELD4 (word_t,
				__zero	: 2,
				pid	: 7,
				__fill	: 7,
				vspace	: 16
			    );
			};
			word_t	    pid_vspace_raw;
		    };
		    tcb_t *	    threads;	// tcb & will be word aligned at least
		    word_t	    thread_count;
		    ringlist_t<space_t>	spaces_list;
		    tcb_t *	    thread_list;
		    word_t	    utcb_section;
		};
		pgent_t var_area[VAR_AREA_SECTIONS]; /* 16 entries */
	    };

	    pgent_t io_area[IO_AREA_SECTIONS];
	    union {
		pgent_t misc_area[MISC_AREA_SECTIONS];	/* 15 entries */
		struct {
		    word_t kip_pte;
		    /* Note - keep bottom two bits = 0 */
		    utcb_bitmap_t utcb_bitmap[MISC_AREA_SECTIONS-1];
		};
	    };
	    pgent_t high_int_vector;
        } pd_split;
    } pt;
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
    ENQUEUE_LIST_TAIL(global_spaces_list, this, pt.pd_split.spaces_list);
    spaces_list_lock.unlock();
#endif
}

INLINE void space_t::dequeue_spaces()
{
#ifdef CONFIG_DEBUG
    spaces_list_lock.lock();
    DEQUEUE_LIST(global_spaces_list, this, pt.pd_split.spaces_list);
    spaces_list_lock.unlock();
#endif
}

#ifdef CONFIG_ENABLE_FASS
    INLINE arm_domain_t space_t::get_domain(void)
    {
        return this->pt.pd_split.domain >> 2;
    }

    INLINE void space_t::set_domain(arm_domain_t new_domain)
    {
        this->pt.pd_split.domain = new_domain << 2;
    }

    INLINE arm_pid_t space_t::get_pid(void)
    {
        return this->pt.pd_split.pid;
    }

    INLINE void space_t::set_pid(arm_pid_t new_pid)
    {
        this->pt.pd_split.pid = new_pid;
    }
#endif

INLINE pgent_t *space_t::pgent(word_t num, word_t cpu) 
{
    return (pgent_t*)virt_to_page_table(&(pt.pdir[num]));
}

/**
 * get the KIP area of an address space
 * @returns the KIP area of the address space as an fpage
 */
INLINE fpage_t space_t::get_kip_page_area (void)
{
    fpage_t ret;
    ret.set(USER_KIP_PAGE, KIP_KIP_BITS, 0, 0, 0);
    return ret;
}

/**
 * get the UTCB area of an address space
 * @returns the utcb area of the address space as an fpage
 */
INLINE fpage_t space_t::get_utcb_page_area (void)
{
    fpage_t ret;
    ret.raw = 0;
    return ret;
}

INLINE bool space_t::is_user_area (addr_t addr)
{
    return ((word_t)addr < USER_AREA_END);
}

/* KTCB Area */
INLINE bool space_t::is_tcb_area (addr_t addr)
{
    return ((word_t)addr >= KTCB_AREA_START && (word_t)addr < KTCB_AREA_END);
}

/* UTCB Area */
INLINE bool space_t::is_utcb_area (addr_t addr)
{
    return ((word_t)addr >= UTCB_AREA_START && (word_t)addr < UTCB_AREA_END);
}

/* Domain Fault Area */
INLINE bool space_t::is_domain_area (addr_t addr)
{
    return ((word_t)addr < UTCB_AREA_END);
}


/* May as well return kernel space since mappings should be identical for
 * the kernel area as in the each of the user's address spaces. 
 */

INLINE space_t * get_kernel_space()
{
    extern char * _kernel_space_object;
    return (space_t*)&_kernel_space_object;
}


/**
 * translates a global thread ID into a valid tcb pointer
 * @param tid thread ID
 * @returns pointer to the TCB of thread tid
 */
INLINE tcb_t * space_t::get_tcb( threadid_t tid )
{
    return (tcb_t*)((KTCB_AREA_START) +
		    ((tid.get_threadno() & (VALID_THREADNO_MASK)) * KTCB_SIZE));
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
 * Release mappings that belong to the kernel (UTCB, KIP)
 * @param vaddr		virtual address in the space
 * @param paddr		physical address the mapping refers to
 * @param log2size	log2(size of mapping)
 */
INLINE void space_t::release_kernel_mapping (addr_t vaddr,
		addr_t paddr, word_t log2size)
{
    /* Nothing to do */
}

extern word_t utcb_sections_bitmap[];

INLINE bool space_t::is_initialized()
{
    return (this->pt.pd_split.high_int_vector.raw != 0);
}

#ifdef CONFIG_ENABLE_FASS
INLINE void pgent_t::cpd_sync (space_t * s)
{
    word_t offset = ((word_t)this & 0x3fff);
    pgent_t *cpd_entry = (pgent_t *)((word_t)get_cpd() + offset);
    arm_domain_t n = s->get_domain();

    if (n && (n != INVALID_DOMAIN) && (cpd_entry->get_domain() == n))
    {
	pgent_t temp = {{raw:this->raw}};
	temp.set_domain(n);
	cpd_entry->raw = temp.raw;
    }
}
#endif

void init_kernel_space();

#endif
