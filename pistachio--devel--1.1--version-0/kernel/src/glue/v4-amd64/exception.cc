/*********************************************************************
 *                
 * Copyright (C) 2002, 2004-2003,  Karlsruhe University
 *                
 * File path:     glue/v4-amd64/exception.cc
 * Description:   exception handling
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
 * $Id: 
 *                
 ********************************************************************/

#include <debug.h>
#include <kdb/tracepoints.h>
#include INC_ARCH(traps.h)
#include INC_ARCH(trapgate.h)
#include INC_GLUE(traphandler.h)
#include INC_API(tcb.h)
#include INC_API(space.h)
#include INC_API(kernelinterface.h)

DECLARE_TRACEPOINT (AMD64_GP);
DECLARE_TRACEPOINT (AMD64_UD);
DECLARE_TRACEPOINT (AMD64_NOMATH);


static bool __attribute__((__noinline__)) send_exception_ipc(amd64_exceptionframe_t &frame, word_t exception_number )
{
    tcb_t * current = get_current_tcb();
    if (current->get_exception_handler().is_nilthread())
 	return false;

    /* setup exception IPC */
    word_t saved_mr[21];
    msg_tag_t tag;

    // save message registers 
    for (int i = 0; i < 20; i++)
 	saved_mr[i] = current->get_mr(i);

    word_t saved_br0 = current->get_br(0);

    current->saved_partner = current->get_partner();

    tag.set(0, 20, -5 << 4);
    current->set_mr(0,  tag.raw);
    current->set_mr(1,  frame.rip);
    current->set_mr(2,  frame.rbx);
    current->set_mr(3, frame.r10);
    current->set_mr(4,  frame.r12);
    current->set_mr(5,  frame.r13);
    current->set_mr(6,  frame.r14);
    current->set_mr(7,  frame.r15);
    current->set_mr(8, frame.rax);
    current->set_mr(9, frame.rcx);
    current->set_mr(10, frame.rdx);
    current->set_mr(11, frame.rsi);
    current->set_mr(12, frame.rdi);
    current->set_mr(13, frame.rbp);
    current->set_mr(14, frame.r8);
    current->set_mr(15, frame.r9);
    current->set_mr(16,  frame.r11);
    current->set_mr(17, frame.rsp);
    current->set_mr(18, frame.rflags);
    current->set_mr(19,  exception_number);
    current->set_mr(20,  frame.error);

    tag = current->do_ipc(current->get_exception_handler(), 
			  current->get_exception_handler(), 
			  timeout_t::never());

    if (!tag.is_error())
    {
	frame.rip = current->get_mr(1);
	frame.rbx = current->get_mr(2);
	frame.r10 = current->get_mr(3);
	frame.r12 = current->get_mr(4);
	frame.r13 = current->get_mr(5);
	frame.r14 = current->get_mr(6);
	frame.r15 = current->get_mr(7);
	frame.rax = current->get_mr(8);
	frame.rcx = current->get_mr(9);
	frame.rdx = current->get_mr(10);
	frame.rsi = current->get_mr(11);
	frame.rdi = current->get_mr(12);
	frame.rbp = current->get_mr(13);
	frame.r8 = current->get_mr(14);
	frame.r9 = current->get_mr(15);
	frame.r11 = current->get_mr(16);
	frame.rsp = current->get_mr(17);
	current->set_user_flags(current->get_mr(18));
    }
    else
    {
 	enter_kdebug("Exception delivery error");
    }

    for (int i = 0; i < 20; i++)
 	current->set_mr(i, saved_mr[i]);
    
    current->set_br(0, saved_br0);
    current->set_partner(current->saved_partner);
    current->saved_partner = NILTHREAD;

    return !tag.is_error();
}

AMD64_EXC_NO_ERRORCODE(exc_catch_diverr, -1)
{
    printf("Divide by Zero Exception caught, dumping frame:\n");
    printf("\tRAX %16x", frame.rax);
    printf("\t R8 %16x\n", frame.r8 );
    printf("\tRCX %16x", frame.rcx);
    printf("\t R9 %16x\n", frame.r9 );
    printf("\tRDX %16x", frame.rdx);
    printf("\tR10 %16x\n", frame.r10);
    printf("\tRBX %16x", frame.rbx);
    printf("\tR11 %16x\n", frame.r11);
    printf("\tRSP %16x", frame.rsp);
    printf("\tR12 %16x\n", frame.r12);
    printf("\tRBP %16x", frame.rbp);
    printf("\tR13 %16x\n", frame.r13);
    printf("\tRSI %16x", frame.rsi);
    printf("\tR14 %16x\n", frame.r14);
    printf("\tRDI %16x", frame.rdi);
    printf("\tR15 %16x\n", frame.r15);
    printf("\tRIP %16x\n", frame.rip);
    printf("\tERR %16x", frame.error);
    printf("\tRFL %16x\n", frame.rflags);
    printf("\t CS %16x", frame.cs);
    printf("\t SS %16x\n", frame.ss);
    
    enter_kdebug("Exception");
}				  
AMD64_EXC_NO_ERRORCODE(exc_catch_overflow, -1)
{
    printf("Overflow Exception caught, dumping frame:\n");
    printf("\tRAX %16x", frame.rax);
    printf("\t R8 %16x\n", frame.r8 );
    printf("\tRCX %16x", frame.rcx);
    printf("\t R9 %16x\n", frame.r9 );
    printf("\tRDX %16x", frame.rdx);
    printf("\tR10 %16x\n", frame.r10);
    printf("\tRBX %16x", frame.rbx);
    printf("\tR11 %16x\n", frame.r11);
    printf("\tRSP %16x", frame.rsp);
    printf("\tR12 %16x\n", frame.r12);
    printf("\tRBP %16x", frame.rbp);
    printf("\tR13 %16x\n", frame.r13);
    printf("\tRSI %16x", frame.rsi);
    printf("\tR14 %16x\n", frame.r14);
    printf("\tRDI %16x", frame.rdi);
    printf("\tR15 %16x\n", frame.r15);
    printf("\tRIP %16x\n", frame.rip);
    printf("\tERR %16x", frame.error);
    printf("\tRFL %16x\n", frame.rflags);
    printf("\t CS %16x", frame.cs);
    printf("\t SS %16x\n", frame.ss);
    
    enter_kdebug("Exception");
}				  
AMD64_EXC_NO_ERRORCODE(exc_catch_boundrange, -1)
{
    printf("Bound Range Exception caught, dumping frame:\n");
    printf("\tRAX %16x", frame.rax);
    printf("\t R8 %16x\n", frame.r8 );
    printf("\tRCX %16x", frame.rcx);
    printf("\t R9 %16x\n", frame.r9 );
    printf("\tRDX %16x", frame.rdx);
    printf("\tR10 %16x\n", frame.r10);
    printf("\tRBX %16x", frame.rbx);
    printf("\tR11 %16x\n", frame.r11);
    printf("\tRSP %16x", frame.rsp);
    printf("\tR12 %16x\n", frame.r12);
    printf("\tRBP %16x", frame.rbp);
    printf("\tR13 %16x\n", frame.r13);
    printf("\tRSI %16x", frame.rsi);
    printf("\tR14 %16x\n", frame.r14);
    printf("\tRDI %16x", frame.rdi);
    printf("\tR15 %16x\n", frame.r15);
    printf("\tRIP %16x\n", frame.rip);
    printf("\tERR %16x", frame.error);
    printf("\tRFL %16x\n", frame.rflags);
    printf("\t CS %16x", frame.cs);
    printf("\t SS %16x\n", frame.ss);
    
    enter_kdebug("Exception");
}				  
AMD64_EXC_NO_ERRORCODE(exc_catch_doublefault, -1)
{
    printf("Doublefault Exception caught, dumping frame:\n");
    printf("\tRAX %16x", frame.rax);
    printf("\t R8 %16x\n", frame.r8 );
    printf("\tRCX %16x", frame.rcx);
    printf("\t R9 %16x\n", frame.r9 );
    printf("\tRDX %16x", frame.rdx);
    printf("\tR10 %16x\n", frame.r10);
    printf("\tRBX %16x", frame.rbx);
    printf("\tR11 %16x\n", frame.r11);
    printf("\tRSP %16x", frame.rsp);
    printf("\tR12 %16x\n", frame.r12);
    printf("\tRBP %16x", frame.rbp);
    printf("\tR13 %16x\n", frame.r13);
    printf("\tRSI %16x", frame.rsi);
    printf("\tR14 %16x\n", frame.r14);
    printf("\tRDI %16x", frame.rdi);
    printf("\tR15 %16x\n", frame.r15);
    printf("\tRIP %16x\n", frame.rip);
    printf("\tERR %16x", frame.error);
    printf("\tRFL %16x\n", frame.rflags);
    printf("\t CS %16x", frame.cs);
    printf("\t SS %16x\n", frame.ss);
    
    enter_kdebug("Exception");
}				  
AMD64_EXC_NO_ERRORCODE(exc_catch_overrun, -1)
{
    printf("Overrun Exception caught, dumping frame:\n");
    printf("\tRAX %16x", frame.rax);
    printf("\t R8 %16x\n", frame.r8 );
    printf("\tRCX %16x", frame.rcx);
    printf("\t R9 %16x\n", frame.r9 );
    printf("\tRDX %16x", frame.rdx);
    printf("\tR10 %16x\n", frame.r10);
    printf("\tRBX %16x", frame.rbx);
    printf("\tR11 %16x\n", frame.r11);
    printf("\tRSP %16x", frame.rsp);
    printf("\tR12 %16x\n", frame.r12);
    printf("\tRBP %16x", frame.rbp);
    printf("\tR13 %16x\n", frame.r13);
    printf("\tRSI %16x", frame.rsi);
    printf("\tR14 %16x\n", frame.r14);
    printf("\tRDI %16x", frame.rdi);
    printf("\tR15 %16x\n", frame.r15);
    printf("\tRIP %16x\n", frame.rip);
    printf("\tERR %16x", frame.error);
    printf("\tRFL %16x\n", frame.rflags);
    printf("\t CS %16x", frame.cs);
    printf("\t SS %16x\n", frame.ss);
    
    enter_kdebug("Exception");
}				  
AMD64_EXC_NO_ERRORCODE(exc_catch_invtss, -1)
{
    printf("Invalid TSS Exception caught, dumping frame:\n");
    printf("\tRAX %16x", frame.rax);
    printf("\t R8 %16x\n", frame.r8 );
    printf("\tRCX %16x", frame.rcx);
    printf("\t R9 %16x\n", frame.r9 );
    printf("\tRDX %16x", frame.rdx);
    printf("\tR10 %16x\n", frame.r10);
    printf("\tRBX %16x", frame.rbx);
    printf("\tR11 %16x\n", frame.r11);
    printf("\tRSP %16x", frame.rsp);
    printf("\tR12 %16x\n", frame.r12);
    printf("\tRBP %16x", frame.rbp);
    printf("\tR13 %16x\n", frame.r13);
    printf("\tRSI %16x", frame.rsi);
    printf("\tR14 %16x\n", frame.r14);
    printf("\tRDI %16x", frame.rdi);
    printf("\tR15 %16x\n", frame.r15);
    printf("\tRIP %16x\n", frame.rip);
    printf("\tERR %16x", frame.error);
    printf("\tRFL %16x\n", frame.rflags);
    printf("\t CS %16x", frame.cs);
    printf("\t SS %16x\n", frame.ss);
    
    enter_kdebug("Exception");
}				  
AMD64_EXC_NO_ERRORCODE(exc_catch_segnotpr, -1)
{
    printf("Segment not present Exception caught, dumping frame:\n");
    printf("\tRAX %16x", frame.rax);
    printf("\t R8 %16x\n", frame.r8 );
    printf("\tRCX %16x", frame.rcx);
    printf("\t R9 %16x\n", frame.r9 );
    printf("\tRDX %16x", frame.rdx);
    printf("\tR10 %16x\n", frame.r10);
    printf("\tRBX %16x", frame.rbx);
    printf("\tR11 %16x\n", frame.r11);
    printf("\tRSP %16x", frame.rsp);
    printf("\tR12 %16x\n", frame.r12);
    printf("\tRBP %16x", frame.rbp);
    printf("\tR13 %16x\n", frame.r13);
    printf("\tRSI %16x", frame.rsi);
    printf("\tR14 %16x\n", frame.r14);
    printf("\tRDI %16x", frame.rdi);
    printf("\tR15 %16x\n", frame.r15);
    printf("\tRIP %16x\n", frame.rip);
    printf("\tERR %16x", frame.error);
    printf("\tRFL %16x\n", frame.rflags);
    printf("\t CS %16x", frame.cs);
    printf("\t SS %16x\n", frame.ss);
    
    enter_kdebug("Exception");
}				  
AMD64_EXC_NO_ERRORCODE(exc_catch_ss_fault, -1)
{
    printf("Stack Segment fault Exception caught, dumping frame:\n");
    printf("\tRAX %16x", frame.rax);
    printf("\t R8 %16x\n", frame.r8 );
    printf("\tRCX %16x", frame.rcx);
    printf("\t R9 %16x\n", frame.r9 );
    printf("\tRDX %16x", frame.rdx);
    printf("\tR10 %16x\n", frame.r10);
    printf("\tRBX %16x", frame.rbx);
    printf("\tR11 %16x\n", frame.r11);
    printf("\tRSP %16x", frame.rsp);
    printf("\tR12 %16x\n", frame.r12);
    printf("\tRBP %16x", frame.rbp);
    printf("\tR13 %16x\n", frame.r13);
    printf("\tRSI %16x", frame.rsi);
    printf("\tR14 %16x\n", frame.r14);
    printf("\tRDI %16x", frame.rdi);
    printf("\tR15 %16x\n", frame.r15);
    printf("\tRIP %16x\n", frame.rip);
    printf("\tERR %16x", frame.error);
    printf("\tRFL %16x\n", frame.rflags);
    printf("\t CS %16x", frame.cs);
    printf("\t SS %16x\n", frame.ss);
    
    enter_kdebug("Exception");
}				  
AMD64_EXC_NO_ERRORCODE(exc_catch_ac, -1)
{
    printf("Alignment Check Exception caught, dumping frame:\n");
    printf("\tRAX %16x", frame.rax);
    printf("\t R8 %16x\n", frame.r8 );
    printf("\tRCX %16x", frame.rcx);
    printf("\t R9 %16x\n", frame.r9 );
    printf("\tRDX %16x", frame.rdx);
    printf("\tR10 %16x\n", frame.r10);
    printf("\tRBX %16x", frame.rbx);
    printf("\tR11 %16x\n", frame.r11);
    printf("\tRSP %16x", frame.rsp);
    printf("\tR12 %16x\n", frame.r12);
    printf("\tRBP %16x", frame.rbp);
    printf("\tR13 %16x\n", frame.r13);
    printf("\tRSI %16x", frame.rsi);
    printf("\tR14 %16x\n", frame.r14);
    printf("\tRDI %16x", frame.rdi);
    printf("\tR15 %16x\n", frame.r15);
    printf("\tRIP %16x\n", frame.rip);
    printf("\tERR %16x", frame.error);
    printf("\tRFL %16x\n", frame.rflags);
    printf("\t CS %16x", frame.cs);
    printf("\t SS %16x\n", frame.ss);
    
    enter_kdebug("Exception");
}				  
AMD64_EXC_NO_ERRORCODE(exc_catch_mc, -1)
{
    printf("Machine Check Exception caught, dumping frame:\n");
    printf("\tRAX %16x", frame.rax);
    printf("\t R8 %16x\n", frame.r8 );
    printf("\tRCX %16x", frame.rcx);
    printf("\t R9 %16x\n", frame.r9 );
    printf("\tRDX %16x", frame.rdx);
    printf("\tR10 %16x\n", frame.r10);
    printf("\tRBX %16x", frame.rbx);
    printf("\tR11 %16x\n", frame.r11);
    printf("\tRSP %16x", frame.rsp);
    printf("\tR12 %16x\n", frame.r12);
    printf("\tRBP %16x", frame.rbp);
    printf("\tR13 %16x\n", frame.r13);
    printf("\tRSI %16x", frame.rsi);
    printf("\tR14 %16x\n", frame.r14);
    printf("\tRDI %16x", frame.rdi);
    printf("\tR15 %16x\n", frame.r15);
    printf("\tRIP %16x\n", frame.rip);
    printf("\tERR %16x", frame.error);
    printf("\tRFL %16x\n", frame.rflags);
    printf("\t CS %16x", frame.cs);
    printf("\t SS %16x\n", frame.ss);
    
    enter_kdebug("Exception");
}				  

AMD64_EXC_NO_ERRORCODE(exc_invalid_opcode, AMD64_EXC_INVALIDOPCODE)
{
    tcb_t * current = get_current_tcb();
    space_t * space = current->get_space();
    addr_t addr = (addr_t) frame.rip;

    TRACEPOINT_TB (AMD64_UD,
 		   printf ("%t: invalid opcode at IP %p\n", current, addr),
		   "amd64_ud at %x (current=%x)", (u64_t)addr, (u64_t)current);

    /* instruction emulation */
    switch( (u8_t) space->get_from_user(addr))
    {
    case 0xf0: /* lock prefix */
 	if ( (u8_t) space->get_from_user(addr_offset(addr, 1)) == 0x90)
 	{
 	    /* lock; nop */
	    // TRACEF("lock;nop: kip is at %p\n", (u64_t)space->get_kip_page_area().get_base());
 	    frame.rax = (u64_t)space->get_kip_page_area().get_base();
 	    frame.rcx = get_kip()->api_version;
 	    frame.rdx = get_kip()->api_flags;
 	    frame.rsi = get_kip()->get_kernel_descriptor()->kernel_id.get_raw();
 	    frame.rip+= 2;
 	    return;
 	}
      
    default:
	printf ("%p: invalid opcode at IP %p\n", current, addr);
 	enter_kdebug("invalid opcode");
 	while(1);
    }
}

AMD64_EXC_WITH_ERRORCODE(exc_gp, AMD64_EXC_GENERAL_PROTECTION)
{
    TRACEPOINT_TB (AMD64_GP,
 		   printf ("general protection fault @ %p, error: %x\n", 
 			   frame.rip, frame.error),
		   "amd64_gp at %x (error=%d)",
		   frame.rip, frame.error);

    word_t ds = 0 , es = 0, fs = 0, gs = 0;
    
    printf("GP exception caught, dumping frame:\n");
    printf("\tRAX %16x", frame.rax);
    printf("\t R8 %16x\n", frame.r8 );
    printf("\tRCX %16x", frame.rcx);
    printf("\t R9 %16x\n", frame.r9 );
    printf("\tRDX %16x", frame.rdx);
    printf("\tR10 %16x\n", frame.r10);
    printf("\tRBX %16x", frame.rbx);
    printf("\tR11 %16x\n", frame.r11);
    printf("\tRSP %16x", frame.rsp);
    printf("\tR12 %16x\n", frame.r12);
    printf("\tRBP %16x", frame.rbp);
    printf("\tR13 %16x\n", frame.r13);
    printf("\tRSI %16x", frame.rsi);
    printf("\tR14 %16x\n", frame.r14);
    printf("\tRDI %16x", frame.rdi);
    printf("\tR15 %16x\n", frame.r15);
    printf("\tRIP %16x\n", frame.rip);
    printf("\tERR %16x", frame.error);
    printf("\tRFL %16x\n", frame.rflags);
    printf("\t CS %16x", frame.cs);
    printf("\t SS %16x\n", frame.ss);

    __asm__ (
        "mov %%ds, %0   \n" 
        "mov %%es, %1	\n" 
        "mov %%fs, %2   \n" 
        "mov %%gs, %3   \n" 
        :
        : "r"(ds), "r"(es), "r"(fs),"r"(gs)
        );
    printf("\t DS %16x", ds);
    printf("\t ES %16x\n", es);
    printf("\t FS %16x", fs);
    printf("\t GS %16x\n", gs);

    if (send_exception_ipc(frame, AMD64_EXC_GENERAL_PROTECTION))
	return;

    enter_kdebug("#GP");
}

AMD64_EXC_NO_ERRORCODE(exc_nomath_coproc, AMD64_EXC_NOMATH_COPROC)
{
    tcb_t * current = get_current_tcb();

    TRACEPOINT(AMD64_NOMATH, 
	       printf("AMD64_NOMATH %t @ %p\n", current, frame.rip));

    current->resources.amd64_no_math_exception(current);
}

AMD64_EXC_NO_ERRORCODE(exc_fpu_fault, AMD64_EXC_FPU_FAULT)
{

    send_exception_ipc(frame, AMD64_EXC_FPU_FAULT);
}

AMD64_EXC_NO_ERRORCODE(exc_simd_fault, AMD64_EXC_SIMD_FAULT)
{
    send_exception_ipc(frame, AMD64_EXC_SIMD_FAULT);
}



word_t exc_catch_all[IDT_SIZE];
extern "C" void exc_catch_common_handler(amd64_exceptionframe_t frame){

    word_t irq  = (frame.error - 5 - (word_t) exc_catch_all) / 8;
    printf("Invalid jump to IDT Entry No %d - Bogus Interrupt?\n", irq);
    //enter_kdebug("Bogus Interrupt");
}

void exc_catch_common_wrapper() 					
{							
    __asm__ (						
        ".section .data.amd64.exc_common	\n"
        ".global exc_catch_common		\n"
	"\t.type exc_catch_common,@function	\n"
	"exc_catch_common:			\n"
        "pushq %%rax				\n"
	"pushq %%rcx				\n"
	"pushq %%rbx				\n"
	"pushq %%rdx				\n"
	"pushq %%rbp				\n"
    	"pushq %%rsi				\n"
    	"pushq %%rdi				\n"
    	"pushq %%r8				\n"
    	"pushq %%r9				\n"
    	"pushq %%r10				\n"
    	"pushq %%r11				\n"
    	"pushq %%r12				\n" 
    	"pushq %%r13				\n"
    	"pushq %%r14				\n" 
    	"pushq %%r15				\n"
	"pushq %0			    	\n"
	"call exc_catch_common_handler		\n"		
	"addq  $8, %%rsp			\n"		
    	"popq  %%r15				\n"		
    	"popq  %%r14				\n"		
    	"popq  %%r13				\n"		
    	"popq  %%r12				\n"		
    	"popq  %%r11				\n"		
    	"popq  %%r10				\n"		
    	"popq  %%r9				\n"		
    	"popq  %%r8				\n"		
    	"popq  %%rdi				\n"		
    	"popq  %%rsi				\n"		
	"popq  %%rbp				\n"		
	"popq  %%rdx				\n"		
	"popq  %%rbx				\n"		
	"popq  %%rcx				\n"		
        "popq  %%rax				\n"		
	"addq  $8, %%rsp			\n"		
	"iretq					\n"		
	".previous				\n"
	:						
	: "i"(0)					
	);						
}							
