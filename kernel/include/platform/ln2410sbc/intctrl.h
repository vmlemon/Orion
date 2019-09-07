/*********************************************************************
 *                
 * Copyright (C) 2003-2004,  National ICT Australia (NICTA)
 *                
 * File path:     platform/csb337/intctrl.h
 * Description:   
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
 * $Id: intctrl.h,v 1.1 2004/08/12 10:58:53 cvansch Exp $
 *                
 ********************************************************************/
#ifndef __PLATFORM__CSB337__INTCTRL_H__
#define __PLATFORM__CSB337__INTCTRL_H__

/* Atmel AT91RM9200 Advanced Interrupt Controller (AIC) */
#define IRQS	34

extern word_t arm_high_vector; 

#define LN2410_INT_SRCPND (*(volatile word_t *)(INTERRUPT_VADDR + 0x00))   /* Source pending register */
#define LN2410_INT_MOD (*(volatile word_t *)(INTERRUPT_VADDR + 0x04))   /* Mode register */
#define LN2410_INT_MASK (*(volatile word_t *)(INTERRUPT_VADDR + 0x08))   /* Mask register */
#define LN2410_INT_PND (*(volatile word_t *)(INTERRUPT_VADDR + 0x10))   /* Pending register */
#define LN2410_INT_OFFSET (*(volatile word_t *)(INTERRUPT_VADDR + 0x14))   /* Pending register */

#define LN2410_INT_SUBSRCPND (*(volatile word_t *)(INTERRUPT_VADDR + 0x18))   /* subsrc register */
#define LN2410_INT_SUBMASK (*(volatile word_t *)(INTERRUPT_VADDR + 0x1c))   /* submask register */


extern word_t interrupt_handlers[IRQS];

class intctrl_t : public generic_intctrl_t {

public:
    void init_arch();
    void init_cpu();

    word_t get_number_irqs(void)
    {
	printf("get_number_irqs called\n");
	return IRQS;
    }

    void register_interrupt_handler (word_t vector, void (*handler)(word_t,
	    arm_irq_context_t *))
    {
        ASSERT(DEBUG, vector < IRQS);
        interrupt_handlers[vector] = (word_t) handler;
        printf("interrupt vector[%d] = %p\n", vector, 
	       interrupt_handlers[vector]);
    }

    static inline void mask(word_t irq)
    {
	ASSERT(DEBUG, irq < IRQS);
	switch (irq) {
	case 33:
		LN2410_INT_SUBMASK |= 0x400;
		break;
	case 32:
		LN2410_INT_SUBMASK |= 0x200;
		break;
	case 31:
		break;
	default:
		LN2410_INT_MASK |= (1 << irq);
	}
	//printf("Mask: %d -- %lx\n", irq, LN2410_INT_MASK);

    }

    static inline bool unmask(word_t irq)
    {
	ASSERT(DEBUG, irq < IRQS);
	//printf("Unmask: %d -- %lx\n", irq);

	switch (irq) {
	case 33:
		LN2410_INT_SUBMASK &= ~0x400;
		break;
	case 32:
		LN2410_INT_SUBMASK &= ~0x200;
		break;
	case 31:
		break;
	default:
		//printf("Unmask: %d\n", irq);
		LN2410_INT_MASK &= ~(1 << irq);
	}

	//printf("Unmask: %d -- %lx\n", irq, LN2410_INT_MASK);
	return false;
    }

    static inline void disable(word_t irq)
    {  
	mask(irq);
    }

    static inline bool enable(word_t irq)
    {
	return unmask(irq);
    }

    void disable_fiq(void)
    {
    }

    bool is_irq_available(int irq)
    { 
	/* IRQ0 - Fast Interrupt not available */
	return (irq >= 1) && (irq < IRQS);  
    }

    void set_cpu(word_t irq, word_t cpu) {}
}; 

#endif /*__PLATFORM__CSB337__INTCTRL_H__ */
