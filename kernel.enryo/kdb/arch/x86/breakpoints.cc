/*********************************************************************
 *                
 * Copyright (C) 2002, 2007-2008,  Karlsruhe University
 *                
 * File path:     kdb/arch/x86/breakpoints.cc
 * Description:   Hardware breakpoints for IA-32
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
 * $Id: breakpoints.cc,v 1.2 2003/09/24 19:05:05 skoglund Exp $
 *                
 ********************************************************************/

#include <debug.h>
#include <kdb/kdb.h>
#include <kdb/input.h>
#include <kdb/tracepoints.h>
#include INC_API(tcb.h)

DECLARE_CMD(cmd_breakpoint, root, 'b', "breakpoint", "set breakpoints");

word_t x86_kdb_dr_mask = 0;

#if defined(CONFIG_TRACEPOINTS)
EXTERN_TRACEPOINT(X86_BREAKPOINT);
bool x86_breakpoint_cpumask;
bool x86_breakpoint_cpumask_kdb;
#endif

void x86_set_kdb_dr(word_t num, x86_breakpoint_type_e type, word_t addr, bool enable, bool kdb)
{
    word_t db7;
    __asm__ __volatile__ ("mov %%db7,%0" : "=r" (db7));
    
    ASSERT(num < 4);
    
    if (enable)
    {
	x86_kdb_dr_mask |= (1 << num);
	db7 |=  (2 << (num * 2)); /* enable */
    }
    else
    {
	x86_kdb_dr_mask &= ~(1 << num);
	db7 &= ~(2 << (num * 2)); /* disable */
    }
    
    db7 &= ~(0x000F0000 << (num * 4));
    db7 |= (type << (num * 4));

    x86_dr_write(num, addr);
    X86_SET_DR(7, db7);

#if defined(CONFIG_TRACEPOINTS)
    cpuid_t cpu = get_current_cpu();
    if (kdb)
	x86_breakpoint_cpumask_kdb |= (1 << cpu);
    else
	x86_breakpoint_cpumask_kdb &= ~(1 << cpu);
    x86_breakpoint_cpumask |= (1 << cpu);
#endif
}

CMD(cmd_breakpoint, cg)
{
    
    /* the breakpoint register to be used */
    word_t db7;
    /* breakpoint address */
    word_t addr = 0;
    
    int num = get_choice("breakpoint [-/?/0..3]: ", "-/?/0/1/2/3", '?');
    switch (num) {
	/* set debug register 0..3 manually */
    case '0'...'3':
	num -= '0';
	break;
	/* reset all debug registers */
    case '-':
	x86_kdb_dr_mask = 0;
	X86_GET_DR(7, db7);
	db7 &= ~(0x00000FF);
	X86_SET_DR(7, db7);
	return CMD_NOQUIT; 
	break;
	
	/* any key dumps debug registers */
    case '?':
    default:
	X86_GET_DR(7, db7);
	printf("\nDR7: %wx\n", db7);
	X86_GET_DR(6, db7);
	printf("DR6: %wx\n", db7); addr=db7;
	X86_GET_DR(3, db7);
	printf("DR3: %wx %c\n", db7, addr & 8 ? '*' : ' ');
	X86_GET_DR(2, db7);
	printf("DR2: %wx %c\n", db7, addr & 4 ? '*' : ' ');
	X86_GET_DR(1, db7);
	printf("DR1: %wx %c\n", db7, addr & 2 ? '*' : ' ');
	X86_GET_DR(0, db7);
	printf("DR0: %wx %c\n", db7, addr & 1 ? '*' : ' ');
	return CMD_NOQUIT; break;
    }
    /* read debug control register */
    X86_GET_DR(7, db7);
    
    char t = get_choice("Type", "Instr/Access/pOrt/Write/-/+", 'i');

    x86_kdb_dr_mask |= (1 << num);

    switch (t)
    {
    case '-':
	x86_kdb_dr_mask &= ~(1 << num);
	db7 &= ~(2 << (num * 2)); /* disable */
	num = -1;
	break;
    case '+':
	db7 |=  (2 << (num * 2)); /* enable */
	num = -1;
	break;
    case 'i': /* instruction execution */
	addr = get_hex("Address");
	db7 &= ~(0x000F0000 << (num * 4));
	db7 |= (0x00000000 << (num * 4));
	db7 |= (2 << (num * 2)); /* enable */
	break;
    case 'w': /* data write */
	addr = get_hex("Address");
	db7 &= ~(0x000F0000 << (num * 4));
	db7 |= (0x00010000 << (num * 4));
	db7 |= (2 << (num * 2)); /* enable */
	break;
    case 'o': /* I/O */
	addr = get_hex("Port");
	db7 &= ~(0x000F0000 << (num * 4));
	db7 |= (0x00020000 << (num * 4));
	db7 |= (2 << (num * 2)); /* enable */
	break;
    case 'a': /* read/write */
	addr = get_hex("Address");
	db7 &= ~(0x000F0000 << (num * 4));
	db7 |= (0x00030000 << (num * 4));
	db7 |= (2 << (num * 2)); /* enable */
	break;
    };
    
    x86_dr_write(num, addr);
    X86_SET_DR(7, db7);
   
#if defined(CONFIG_TRACEPOINTS)
    if (num != -1)
    {
	cpuid_t cpu = get_current_cpu();
	if (get_choice ("Enter KDB", "y/n", 'y') == 'y')
	    x86_breakpoint_cpumask_kdb |= (1 << cpu);
	else
	    x86_breakpoint_cpumask_kdb &= ~(1 << cpu);
	x86_breakpoint_cpumask |= (1 << cpu);
    }
#endif
    
    return CMD_NOQUIT;
}
