/*********************************************************************
 *                
 * Copyright (C) 2005,  National ICT Australia
 *                
 * File path:     arch/arm/special.h
 * Description:   ARM-specific special optimisations
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
 * $Id: $
 *                
 ********************************************************************/
#ifndef __ARCH__ARM__SPECIAL_H__
#define __ARCH__ARM__SPECIAL_H__

/*
 * Find Most Significant bit.
 * NB - check for w == 0 must be done before calling msb()
 * */

#ifdef CONFIG_ARM_V5

inline word_t msb(word_t w)
{
    word_t zeros;
    __asm__ __volatile__ (
	"   clz	    %0, %1	\n"
	: "=r" (zeros)
	: "r" (w)
    );
    return 31-zeros;
}

#else

inline word_t msb(word_t w)
{
    word_t test, pos;

    pos = 31;
    test = 1 << 31;

    while (!(w & test)) {
	w <<= 1;
	pos--;
    }
    return pos;
}

#endif

#endif
