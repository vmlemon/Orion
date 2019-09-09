/*********************************************************************
 *                
 * Copyright (C) 2003-2004,  Karlsruhe University
 *                
 * File path:     kdb/platform/pc99/intctrl.cc
 * Description:   IO-APIC analysis
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
 * $Id: intctrl.cc,v 1.4 2004/03/01 17:13:28 stoess Exp $
 *                
 ********************************************************************/

#include <l4.h>
#include <debug.h>
#include <kdb/kdb.h>
#include <kdb/input.h>
#include INC_GLUE(intctrl.h)

DECLARE_CMD (cmd_apic, arch, 'I', "intctrl", "dump interrupt controller");

CMD(cmd_apic, cg)
{
    intctrl_t * ctrl = get_interrupt_ctrl();
    printf("\nInterrupt controller dump (%d IRQs)\n", 
	   ctrl->get_number_irqs());

#ifdef CONFIG_IOAPIC
    for (unsigned idx = 0; idx < NUM_REDIR_ENTRIES; idx++)
    {
	if (!ctrl->redir[idx].is_valid())
	    continue;

	printf("IRQ %2d: IOAPIC %d, Line %2d: ", idx, 
	       ctrl->redir[idx].id, ctrl->redir[idx].line);
	
	printf("vec %d, %s, %s, %s, %s\n",
	       ctrl->redir[idx].entry.vector,
	       ctrl->redir[idx].entry.dest_mode ? "virt" : "phys",
	       ctrl->redir[idx].entry.polarity ? " low" : "high",
	       ctrl->redir[idx].entry.trigger_mode ? "level" : " edge",
	       ctrl->redir[idx].entry.mask ? "masked" : "unmasked");

	ioapic_redir_t redir = ctrl->get_ioapic(ctrl->redir[idx].id)->
	    get_redir_entry(ctrl->redir[idx].line);
	
	if ( (*((u32_t *)&redir + 1) != *(((u32_t*)&ctrl->redir[idx].entry) + 1)) ||
	     (*((u32_t *)&redir) !=  *((u32_t*)&ctrl->redir[idx].entry)))
	    printf(" redir entries mismatch %8x%8x != %8x%8x\n",
		   *((u32_t *)&redir + 1), *((u32_t *)&redir),
		   *(((u32_t*)&ctrl->redir[idx].entry) + 1), *((u32_t*)&ctrl->redir[idx].entry));
    }
#endif

    return CMD_NOQUIT;
}


