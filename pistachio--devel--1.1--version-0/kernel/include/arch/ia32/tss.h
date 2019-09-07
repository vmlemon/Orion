/*********************************************************************
 *                
 * Copyright (C) 2002,  Karlsruhe University
 *                
 * File path:     arch/ia32/tss.h
 * Description:   IA32 Task State Segment
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
 * $Id: tss.h,v 1.3 2003/09/24 19:04:27 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __ARCH__IA32__TSS_H__
#define __ARCH__IA32__TSS_H__

#define IA32_IOPERMBITMAP_SIZE 0

class ia32_tss_t 
{
public:
    void setup(u16_t ss0);
    void set_esp0(u32_t esp);
    u32_t get_esp0();

private:
    u32_t	link;
    u32_t	esp0, ss0;
    u32_t	esp1, ss1;
    u32_t	esp2, ss2;
    u32_t	cr3;
    u32_t	eip, eflags;
    u32_t	eax, ecx, edx, ebx, esp, ebp, esi, edi;
    u32_t	es, cs, ss, ds, fs, gs;
    u32_t	ldt;
    u16_t	trace;
    u16_t	iopbm_offset;
    u8_t	io_bitmap[IA32_IOPERMBITMAP_SIZE/8+1];
    u8_t	stopper;
};

INLINE void ia32_tss_t::set_esp0(u32_t esp)
{
    esp0 = esp;
}

INLINE u32_t ia32_tss_t::get_esp0()
{
    return esp0;
}

INLINE void ia32_tss_t::setup(u16_t ss0)
{
    this->ss0 = ss0;
    iopbm_offset = (u16_t)((u32_t)io_bitmap - (u32_t)this);
    stopper = 0xff;
}
    

#endif /* !__ARCH__IA32__TSS_H__ */
