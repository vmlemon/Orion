/*********************************************************************
 *                
 * Copyright (C) 2002, 2004-2003,  Karlsruhe University
 *                
 * File path:     arch/amd64/pgent.h
 * Description:   Generic page table manipluation for X86-64
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
 * $Id: 
 *                
 ********************************************************************/
#ifndef __ARCH__AMD64__PGENT_H__
#define __ARCH__AMD64__PGENT_H__

#include <kmemory.h>
#include <debug.h>
#include INC_GLUE(config.h)
#include INC_GLUE(hwspace.h)
#include INC_ARCH(ptab.h)
#include INC_ARCH(mmu.h)

#define HW_PGSHIFTS             { 12, 21, 30, 39, 48, 64 }
#define HW_VALID_PGSIZES        ((1 << 12) | (1 << 21))
#define MDB_BUFLIST_SIZES       { {24}, {16}, {4096}, {0} }
#define MDB_PGSHIFTS            { 12, 21, 30, 39, 48 }
#define MDB_NUM_PGSIZES         (4)

EXTERN_KMEM_GROUP(kmem_pgtab);

class mapnode_t;
class space_t;

class pgent_t
{
public:
    union {
	amd64_pgent_t    pgent;
	u64_t            raw;
    };

    enum pgsize_e {
	size_4k   = 0,
	size_2m   = 1,
	size_1g   = 2,
	size_512g = 3,
	size_max = size_512g
    };
private:

    /* Shadow pagetable */
#if !defined(CONFIG_SMP)
    u64_t get_linknode (pgsize_e pgsize)
    { return *(u64_t *) ((word_t) this + AMD64_PTAB_BYTES); }
    
    void set_linknode (pgsize_e pgsize, u64_t val)
    { *(u64_t *) ((word_t) this + AMD64_PTAB_BYTES) = val; }

    void sync (space_t * s, pgsize_e pgsize) { }
#else
#error X86-64 does not support SMP!
#endif
      
public:
    // Predicates
    bool is_valid (space_t * s, pgsize_e pgsize)
    { return pgent.is_valid (); }
    
    bool is_writable (space_t * s, pgsize_e pgsize)
    { return pgent.is_writable (); }
    
    bool is_readable (space_t * s, pgsize_e pgsize)
    { return pgent.is_valid(); }
    
    bool is_executable (space_t * s, pgsize_e pgsize)
    { return pgent.is_executable(); }

    bool is_subtree (space_t * s, pgsize_e pgsize)
    { return !(pgsize == size_4k || (pgsize == size_2m && pgent.is_superpage())); }

    bool is_kernel (space_t * s, pgsize_e pgsize)
    { return pgent.is_kernel(); }

    // Retrieval

    addr_t address (space_t * s, pgsize_e pgsize)
    { return pgent.get_address (); }

    pgent_t * subtree (space_t * s, pgsize_e pgsize)
    { return (pgent_t *) phys_to_virt (pgent.get_ptab ()); }

 
    mapnode_t * mapnode (space_t * s, pgsize_e pgsize, addr_t vaddr) 
    { return (mapnode_t *) (get_linknode (pgsize) ^ (u64_t) vaddr); }
	
    addr_t vaddr (space_t * s, pgsize_e pgsize, mapnode_t * map) 
    { return (addr_t) (get_linknode(pgsize) ^ (u64_t) map); }

    word_t reference_bits (space_t * s, pgsize_e pgsize, addr_t vaddr)
    {
	word_t rwx = 0;
	if (pgent.is_accessed ()) { rwx = 5; }
	if (pgent.is_dirty ()) { rwx |= 6; }
	return rwx;
    }    

    // Modification

    void clear (space_t * s, pgsize_e pgsize, bool kernel, addr_t vaddr)
    {
	pgent.clear ();
	sync (s, pgsize);
	    
	if (! kernel)
	    set_linknode (pgsize, 0);
    }
    
    void flush (space_t * s, pgsize_e pgsize, bool kernel, addr_t vaddr)
    {
    }
    
    void make_subtree (space_t * s, pgsize_e pgsize, bool kernel)
    {
	pgent.set_ptab_entry
	    (virt_to_phys
	     (kmem.alloc (kmem_pgtab, kernel ? AMD64_PTAB_BYTES :
			  AMD64_PTAB_BYTES * 2)),
	     AMD64_PAGE_USER|AMD64_PAGE_WRITABLE);
	    
	sync(s, pgsize);
    }


    void remove_subtree (space_t * s, pgsize_e pgsize, bool kernel)
    {
	addr_t ptab = pgent.get_ptab ();
	pgent.clear();
	sync(s, pgsize);
	kmem.free (kmem_pgtab, phys_to_virt (ptab),
		   kernel ? AMD64_PTAB_BYTES : AMD64_PTAB_BYTES * 2);
    }

    void set_entry (space_t * s, pgsize_e pgsize,
		    addr_t paddr, bool readable, 
		    bool writable, bool executable, bool kernel)
    {
	pgent.set_entry (paddr, writable,
			 (amd64_pgent_t::pagesize_e) pgsize,
			 (kernel ? (AMD64_PAGE_KERNEL | AMD64_PAGE_GLOBAL) 
			  : AMD64_PAGE_USER) |
			 AMD64_PAGE_VALID);
	sync(s, pgsize);
	    
    }
    
    void set_entry (space_t * s, pgsize_e pgsize, pgent_t pgent)
    {
	this->pgent = pgent.pgent;
	sync(s, pgsize);
    }
    
    void set_cacheability (space_t * s, pgsize_e pgsize, bool cacheable)
    {
	this->pgent.set_cacheability(cacheable);
	sync(s, pgsize);
    }
    void set_global (space_t * s, pgsize_e pgsize, bool global)
    {
	pgent.set_global (global);
	sync (s, pgsize);
    }
    
    void revoke_rights (space_t * s, pgsize_e pgsize, word_t rwx)
    { if (rwx & 2) raw &= ~AMD64_PAGE_WRITABLE; sync(s, pgsize); }

    void update_rights (space_t * s, pgsize_e pgsize, word_t rwx)
    { if (rwx & 2) raw |= AMD64_PAGE_WRITABLE; sync(s, pgsize); }

    void reset_reference_bits (space_t * s, pgsize_e pgsize)
    { raw &= ~(AMD64_PAGE_ACCESSED | AMD64_PAGE_DIRTY); sync(s, pgsize); }

    void update_reference_bits (space_t * s, pgsize_e pgsize, word_t rwx)
	{ raw |= ((rwx >> 1) & 0x3) << 5; }

    void set_linknode (space_t * s, pgsize_e pgsize,
		       mapnode_t * map, addr_t vaddr)
    { set_linknode (pgsize, (u64_t) map ^ (u64_t) vaddr); }

    // Movement

    pgent_t * next (space_t * s, pgsize_e pgsize, word_t num)
    { return this + num; }

    // Debug

    void dump_misc (space_t * s, pgsize_e pgsize)
    {
    }
};



#endif /* !__ARCH__AMD64__PGENT_H__ */
