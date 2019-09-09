/*********************************************************************
 *                
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *                
 * File path:     glue/v4-arm/intctrl.cc
 * Description:   Implementation of interrupt control functionality 
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
 * $Id: intctrl.cc,v 1.9 2004/08/12 11:36:36 cvansch Exp $
 *                
 ********************************************************************/

#include <l4.h>
#include INC_API(tcb.h)
#include INC_API(interrupt.h)
#include INC_GLUE(intctrl.h)
#include INC_API(schedule.h)
#include INC_CPU(syscon.h)

#if defined(CONFIG_TRACEBUFFER)
extern long tb_irq;
bool tb_reply_irq();
#endif

void handle_interrupt(word_t irq);

intctrl_t intctrl;
word_t interrupt_handlers[IRQS];

extern "C" void spurious_interrupt_handler(word_t irq,
        arm_irq_context_t * frame)
{
    printf("L4 ARM: Spurious interrupt %d\n", irq);
    enter_kdebug("Spurious interrupt");
}

extern "C" void intctrl_t_handle_irq(word_t irq, arm_irq_context_t * frame)
{   
    get_interrupt_ctrl()->mask(irq);
    handle_interrupt(irq);
}

void intctrl_t::init_cpu()
{
    get_kernel_space()->add_mapping((addr_t)ARM_HIGH_VECTOR_VADDR,
		(addr_t)virt_to_phys(&arm_high_vector),
			pgent_t::size_4k, true, true);

    disable_fiq();

    for (int i = 0; i < IRQS; ++i)
    {
	mask(i);
	register_interrupt_handler(i, intctrl_t_handle_irq);
    }

    word_t control = read_cp15_register(C15_control, C15_CRm_default, C15_OP2_default);
#ifdef CONFIG_DISABLE_ALIGNMENT_EXCEPTIONS
    control &= ~((word_t)C15_CONTROL_A);
#else
    control |= C15_CONTROL_A;
#endif
    write_cp15_register(C15_control, C15_CRm_default, C15_OP2_default, control);

    /* Read control register again - hardware may not support A-bit */
    control = read_cp15_register(C15_control, C15_CRm_default, C15_OP2_default);
    TRACE_INIT("System running with alignment exceptions %s\n",
		    control & C15_CONTROL_A ? "enabled" : "disabled");
}

#ifdef CONFIG_IPC_FASTPATH
extern "C"
void irq_ipc_reply(threadid_t to_tid, threadid_t from_tid)
{
    tcb_t * current = get_current_tcb();
    tcb_t * irq_tcb = get_kernel_space()->get_tcb(to_tid);
    long irq = to_tid.get_irqno();

    // if an interrupt was already pending deliver it
#if defined(CONFIG_TRACEBUFFER)
    if (irq == tb_irq)
    {
	if (tb_reply_irq())
	{
	    // Note, most of thread state is already correct

	    // deliver IPC
	    current->set_tag(msg_tag_t::irq_tag());

	    return;
	}
    }
    else
#endif
    if (EXPECT_FALSE(get_interrupt_ctrl()->unmask(irq)))
    {
	// Note, most of thread state is already correct

	// deliver IPC
	current->set_tag(msg_tag_t::irq_tag());

	return;
    }

    irq_tcb->set_state(thread_state_t::halted);

    current->set_state(thread_state_t::waiting_forever);
    current->set_partner(from_tid);

    get_current_scheduler()->yield(current);
}
#endif

