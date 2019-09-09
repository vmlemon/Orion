/*********************************************************************
 *                
 * Copyright (C) 2002-2003,  Karlsruhe University
 *                
 * File path:     arch/ia32/ptab.h
 * Description:   pagetable management
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
 * $Id: ptab.h,v 1.10 2003/09/24 19:04:27 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __ARCH__IA32__PTAB_H__
#define __ARCH__IA32__PTAB_H__

#include INC_ARCH(ia32.h)

#if !defined(ASSEMBLY)

class pgent_t;

class ia32_pgent_t 
{
public:
    enum pagesize_e {
	size_4k = 0,
	size_4m = 1
    };

    // predicates
    bool is_valid() 
	{ return pg.present == 1; }

    bool is_writable() 
	{ return pg.rw == 1; }

    bool is_accessed()
	{ return pg.accessed == 1; }

    bool is_dirty()
	{ return pg.dirty == 1; }

    bool is_superpage()
	{ return pg.size == 1; }

    bool is_kernel()
	{ return pg.privilege == 0; }

    // retrival
    addr_t get_address()
	{ return (addr_t) (raw & IA32_PAGE_MASK); }

    ia32_pgent_t * get_ptab()
	{ return (ia32_pgent_t*)(raw & IA32_PAGE_MASK); }

    u32_t get_raw()
	{ return raw; }

    // modification
    void clear()
	{ raw = 0; }

    void set_entry(addr_t addr, bool writable, pagesize_e size, u32_t attrib)
	{ 
	    if (size == size_4k)
		raw = ((u32_t)(addr) & IA32_PAGE_MASK);
	    else
		raw = ((u32_t)(addr) & IA32_SUPERPAGE_MASK) | IA32_PAGE_SUPER;

	    raw |= (attrib & IA32_PAGE_FLAGS_MASK) | 
		(writable ? IA32_PAGE_WRITABLE : 0);
	}

    void set_ptab_entry(addr_t addr, u32_t attrib)
	{
	    raw = ((u32_t)(addr) & IA32_PAGE_MASK) | 
		IA32_PAGE_VALID |
		(attrib & IA32_PTAB_FLAGS_MASK);
	}
		
    void copy(const ia32_pgent_t pgent)
	{
	    raw = pgent.raw;
	}

    u32_t get_pdir_idx(addr_t addr)
	{
	    return (u32_t)addr >> IA32_PAGEDIR_BITS;
	}

    u32_t get_ptab_idx(addr_t addr)
	{
	    return (((u32_t)addr) & (~IA32_PAGEDIR_MASK)) >> IA32_PAGE_BITS;
	}

    void set_cacheability (bool cacheable)
	{
	    this->pg.cache_disabled = !cacheable;
	}

    void set_global (bool global)
	{
	    this->pg.global = global;
	}
	      
private:
    union {
	struct {
	    unsigned present		:1;
	    unsigned rw			:1;
	    unsigned privilege		:1;
	    unsigned write_through	:1;

	    unsigned cache_disabled	:1;
	    unsigned accessed		:1;
	    unsigned dirty		:1;
	    unsigned size		:1;

	    unsigned global		:1;
	    unsigned avail		:3;

	    unsigned base		:20;
	} pg;
	u32_t raw;
    };

    friend class pgent_t;
};

#endif /* !ASSEMBLY */

#endif /* !__ARCH__IA32__PTAB_H__ */
