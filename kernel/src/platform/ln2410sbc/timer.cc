/*********************************************************************
 *
 * Copyright (C) 2003-2004,  National ICT Australia (NICTA)
 *
 * File path:     platform/csb337/timer.cc
 * Description:   Periodic timer handling
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
 * $Id: timer.cc,v 1.2 2004/08/21 13:31:33 cvansch Exp $
 *
 ********************************************************************/

#include <l4.h>
#include <debug.h>
#include INC_API(schedule.h)
#include INC_GLUE(timer.h)
#include INC_GLUE(intctrl.h)
#include INC_PLAT(timer.h)

timer_t timer;
extern "C" void intctrl_t_handle_irq(word_t , arm_irq_context_t *);

struct timer_sn2410x {
    volatile word_t  tcfg0;	/* Config register 0 */
    volatile word_t  tcfg1;	/* Config register 1 */
    volatile word_t  tcon;	/* Control register */

    volatile word_t  tcntb0;	/* Timer 0 count register */
    volatile word_t  tcmpb0;	/* Timer 0 control register */
    volatile word_t  tcnto0;	/* Timer 0 observable count register */

    volatile word_t  tcntb1;	/* Timer 1 count register */
    volatile word_t  tcmpb1;	/* Timer 1 control register */
    volatile word_t  tcnto1;	/* Timer 1 observable count register */

    volatile word_t  tcntb2;	/* Timer 2 count register */
    volatile word_t  tcmpb2;	/* Timer 2 control register */
    volatile word_t  tcnto2;	/* Timer 2 observable count register */

    volatile word_t  tcntb3;	/* Timer 3 count register */
    volatile word_t  tcmpb3;	/* Timer 3 control register */
    volatile word_t  tcnto3;	/* Timer 3 observable count register */

    volatile word_t  tcntb4;	/* Timer 4 count register */
    volatile word_t  tcmpb4;	/* Timer 4 control register */
    volatile word_t  tcnto4;	/* Timer 4 observable count register */
};

static volatile struct timer_sn2410x* st =
	    (struct timer_sn2410x*)TIMER_VADDR;

extern "C" void handle_timer_interrupt(word_t irq, arm_irq_context_t *context)
{
	//printf("handle timer interrupt\n");
    get_current_scheduler()->handle_timer_interrupt();
}

void timer_t::init_global(void)
{
    UNIMPLEMENTED();
}

void timer_t::init_cpu(void)
{
    get_interrupt_ctrl()->register_interrupt_handler(TIMER0_INT, 
						     handle_timer_interrupt);

    st->tcfg0 = 7; /*1/8*/
    st->tcfg1 = (st->tcfg0 & 0xfffffff0)|0x00; /* 1/2 */

    st->tcntb0 = 40625-1; /* 10ms samples */
    st->tcmpb0 = st->tcntb0 - 1; /* Not quite sure what this is.. to do with PWM generation */
    st->tcon = 0x2; /* Manual update */

    get_interrupt_ctrl()->unmask(TIMER0_INT);

    st->tcon = 0x9; /* Start */		// Interval Mode, Inverter Off, Update	
}
