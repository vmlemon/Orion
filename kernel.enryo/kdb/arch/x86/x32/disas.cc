/*********************************************************************
 *                
 * Copyright (C) 2002-2003, 2006-2009,  Karlsruhe University
 *                
 * File path:     kdb/arch/x86/x32/disas.cc
 * Description:   Disassembler wrapper for IA-32
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
 * $Id: disas.cc,v 1.7 2006/05/24 09:31:58 stoess Exp $
 *                
 ********************************************************************/

#include <debug.h>
#include <kdb/kdb.h>
#include <kdb/input.h>
#include INC_API(tcb.h)
#include INC_ARCH(trapgate.h)
#include INC_GLUE(space.h)

extern "C" int disas(addr_t pc);
extern "C" int disas16(addr_t pc);

extern space_t *current_disas_space;

DECLARE_CMD(cmd_disas, root, 'U', "disas", "disassemble");

CMD(cmd_disas, cg)
{
    debug_param_t * param = (debug_param_t*)kdb.kdb_param;
    x86_exceptionframe_t* f = param->frame;

    char c;
    u32_t pc;
restart:
    
    if ((pc = get_hex("IP", f->eip)) == ABORT_MAGIC)
	return CMD_NOQUIT;

    current_disas_space = get_space ("Space");
    if (!current_disas_space) current_disas_space = get_kernel_space();

    printf("Key strokes: [space]=next instruction, u=new IP, q=quit\n");
    do {
	printf("%x: ", pc);
	pc += disas((addr_t) pc);
	printf("\n");
	c = get_choice(NULL, " /u/q", ' ');
    } while ((c != 'q') && (c != 'u'));
    if (c == 'u')
	goto restart;

    return CMD_NOQUIT;
}

#if defined(CONFIG_X_X86_HVM)
DECLARE_CMD(cmd_disas_hvm, arch, 'U', "disas", "disassemble HVM");

CMD(cmd_disas_hvm, cg)
{
    debug_param_t * param = (debug_param_t*)kdb.kdb_param;
    x86_exceptionframe_t* f = param->frame;
    tcb_t *tcb;
    
    char c;
    u32_t pc;
    bool real_mode;
restart:
    
    if (current_disas_space && current_disas_space->is_hvm_space())
    {
	tcb = current_disas_space->get_hvm_space()->get_tcb_list();
	ASSERT(tcb);
	pc = (u32_t) tcb->get_user_ip();
	real_mode = tcb->get_user_flags() & X86_FLAGS_VM;
	
	if (real_mode)
	    pc += tcb->arch.get_ctrlxfer_reg(ctrlxfer_item_t::id_csregs, 1);
    }
	    
    if ((pc = get_hex("IP", f->eip)) == ABORT_MAGIC)
	return CMD_NOQUIT;

    current_disas_space = get_space ("Space");
    if (!current_disas_space) current_disas_space = get_kernel_space();

    if (!current_disas_space->is_hvm_space())
	return CMD_NOQUIT;

    if (! current_disas_space->get_hvm_space()->lookup_gphys_addr ((addr_t) pc, (addr_t *) &pc))
	return CMD_NOQUIT;

    tcb = current_disas_space->get_hvm_space()->get_tcb_list();
    ASSERT(tcb);
    real_mode = tcb->get_user_flags() & X86_FLAGS_VM;
    
    
    printf("Key strokes: [space]=next instruction, u=new IP, q=quit\n");
    
    do {
	printf("[%s-bit]: %x: ", (real_mode ? "16" : "32"), pc);
	pc += (real_mode ? disas16((addr_t) pc) : disas((addr_t) pc));
	printf("\n");
	c = get_choice(NULL, " /u/q", ' ');
    } while ((c != 'q') && (c != 'u'));
    if (c == 'u')
	goto restart;

    return CMD_NOQUIT;
}

#endif
