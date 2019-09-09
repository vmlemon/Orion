/*********************************************************************
 *
 * Copyright (C) 2003-2004,  National ICT Australia (NICTA)
 *
 * File path:     platform/csb337/irq.cc
 * Description:   Cogent-CSB337 interrupt demultiplexing 
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
 * $Id: irq.cc,v 1.1 2004/08/12 11:17:37 cvansch Exp $
 *
 ********************************************************************/

#include <l4.h>
#include <debug.h>
#include <linear_ptab.h>
#include INC_API(tcb.h)
#include INC_API(syscalls.h)
#include INC_API(kernelinterface.h)
#include INC_ARCH(thread.h)
#include INC_PLAT(console.h)
#include INC_GLUE(syscalls.h)
#include INC_GLUE(intctrl.h)
#include INC_CPU(cpu.h)

extern "C" void spurious_interrupt_handler(word_t , arm_irq_context_t *);

extern "C" void arm_irq(arm_irq_context_t *context)
{
    int i = LN2410_INT_OFFSET;
    word_t sub;
    int irq = -1;
    //printf("irq: %d\n", i);
    switch(i) {
    case 31:
	    /* Look up submsk top bits */
	    sub = LN2410_INT_SUBSRCPND & (~LN2410_INT_SUBMASK);
	    if (sub & 0x400) {
		    irq = 33;
		    LN2410_INT_SUBMASK |= 0x400;
		    LN2410_INT_SUBSRCPND = 0x400;
	    } else if (sub & 0x200) {
		    irq = 32;
		    LN2410_INT_SUBMASK |= 0x200;
		    LN2410_INT_SUBSRCPND = 0x200;
	    } else {
		    printf("##########: Spurious int31\n");
		    irq = 31; /* COnfused here */
	    }
	    break;
    case 10:
	    /* Don't mask timer irq */
	    irq = i;
	    break;
    default:
	    irq = i;
	    LN2410_INT_MASK |= (1 << i); /* Mask it */
    }
    void (*irq_handler)(int, arm_irq_context_t *) =
	    (void (*)(int, arm_irq_context_t *))interrupt_handlers[irq];


    LN2410_INT_SRCPND = 1 << i; /* Ack it */
    LN2410_INT_PND = 1 << i; /* Ack it again */

    irq_handler(irq, context); 
}

void intctrl_t::init_arch()
{
    printf("LN2410_INT_MASK\n");

    LN2410_INT_MOD = 0; /* Set all to IRQ mode (not fiq mode) */
    LN2410_INT_SRCPND = LN2410_INT_SRCPND; /* Ack src pending */
    LN2410_INT_PND = LN2410_INT_PND; /* Ack int pending */
    LN2410_INT_SUBMASK = 0xffff;
    LN2410_INT_SUBSRCPND = 0;
    LN2410_INT_MASK = 0x7fffffff;  /* Mask off most all interrupts, except
				    those that are dealt with by pants */
    printf("LN2410_INT_MASK: %lx\n", LN2410_INT_MASK);
}
