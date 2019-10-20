/****************************************************************************
 *                
 * Copyright (C) 2002, Karlsruhe University
 *                
 * File path:	arch/powerpc/pvr.h
 * Description:	Constants and functions related to the Processor Version
 * 		Register.
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
 * $Id: pvr.h,v 1.4 2003/12/11 12:47:07 joshua Exp $
 *
 ***************************************************************************/

#ifndef __ARCH__POWERPC__PVR_H__
#define __ARCH__POWERPC__PVR_H__

#ifndef ASSEMBLY

class powerpc_version_t
{
protected:
    enum ppc_pvr_e {
	pvr_psim	= 0,
	pvr_601		= 1,
	pvr_603		= 3,
	pvr_604		= 4,
	pvr_603e	= 6,
	pvr_750		= 8,
	pvr_750FX	= 0x7000,
	pvr_750GX	= 0x7002,
	pvr_604e	= 9,
	pvr_604ev	= 10,
	pvr_7400	= 12,
	pvr_7410	= 0x800C,
	pvr_7450	= 0x8000,
	pvr_7455	= 0x8001,
	    
	/* 64-bit CPUs, for XBox 360, and PS3 */
	pvr_cell_bbe	= 0x0070,
	pvr_xenon	= 0x0071,
    };

	/*
	
	Virtex-II Pro Devices 0x20010820, 0x20010860, 0x200108A0
Virtex-4 Devices 0x20011430, 0x20011470

gentoo@livecd ~ $ cat /proc/cpuinfo
processor       : 0
cpu             : Xenon, altivec supported
clock           : 3192.000000MHz
revision        : 3.0 (pvr 0071 0300)

processor       : 2
cpu             : Xenon, altivec supported
clock           : 3192.000000MHz
revision        : 3.0 (pvr 0071 0300)

processor       : 4
cpu             : Xenon, altivec supported
clock           : 3192.000000MHz
revision        : 3.0 (pvr 0071 0300)

timebase        : 49875000
platform        : Xenon
machine         : CHRP Xenon Game Console


$ cat /proc/cpuinfo
processor       : 0
cpu             : POWER8E (raw), altivec supported
clock           : 2061.000000MHz
revision        : 2.1 (pvr 004b 0201)
...

processor       : 159
cpu             : POWER8E (raw), altivec supported
clock           : 2061.000000MHz
revision        : 2.1 (pvr 004b 0201)

timebase        : 512000000
platform        : PowerNV
model           : 8247-22L
machine         : PowerNV 8247-22L
firmware        : OPAL v3

dpavlin@ps3:~$ cat /proc/cpuinfo 
processor       : 0
cpu             : Cell Broadband Engine, altivec supported
clock           : 3192.000000MHz
revision        : 16.0 (pvr 0070 1000)

processor       : 1
cpu             : Cell Broadband Engine, altivec supported
clock           : 3192.000000MHz
revision        : 16.0 (pvr 0070 1000)

timebase        : 79800000
platform        : PS3
model           : SonyPS3
*/
	    
public:
    static powerpc_version_t read() __attribute__ ((const));

    bool is_psim() { return x.version == powerpc_version_t::pvr_psim; }
    bool is_750()  { return x.version == powerpc_version_t::pvr_750; }
	
    /*Broadway is a derivative of 750
     processor		: 0
    cpu			: 750CL
    clock		: 729.000000MHz
    revision		: 114.0 (pvr 0008 7200)
    bogomips		: 121.50
    timebase		: 60750000
    platform		: wii
    model		: NintendoWii
    vendor		: IBM
    machine		: Nintendo Wii
    Memory		: 317 MB

     */
    bool is_broadway() {
	    if (x.version == 0x0008 && x.revision == 0x7200) {
		    return(x.version == powerpc_version_t::pvr_750);
	    }
    }
		    

protected:
    union
    {
	struct {
	    BITFIELD2( u32_t,
		revision : 16,
		version  : 16
	    );
	} x;
	u32_t raw;
    };
};


INLINE powerpc_version_t powerpc_version_t::read()
{
    powerpc_version_t pvr;
    asm ("mfpvr %0" : "=r" (pvr.raw) );
    return pvr;
}

#endif	/* ASSEMBLY */

#endif	/* __ARCH__POWERPC__PVR_H__ */

