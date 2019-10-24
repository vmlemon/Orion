/*********************************************************************
 *                
 * Copyright (C) 2003,  National ICT Australia (NICTA)
 *                
 * File path:     kdb/arch/arm/frame.cc
 * Description:   Exception frame dumping
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
 * $Id: frame.cc,v 1.3 2004/12/09 01:24:41 cvansch Exp $
 *                
 ********************************************************************/
#include <l4.h>
#include <debug.h>
#include <kdb/cmd.h>
#include <kdb/kdb.h>
#include <kdb/input.h>

#include INC_ARCH(thread.h)
#include INC_API(tcb.h)

#ifdef CONFIG_CPU_ARM_XSCALE

/**
 * cmd_find_copproc: dump coprocessor registers
 */
DECLARE_CMD (cmd_dump_coproc, arch, 'p', "coprocessor",
	     "dump coprocessor registers");

CMD (cmd_dump_coproc, cg)
{
    word_t clkcfg;
    __asm__  __volatile__ (
	"   mrc	    p14, 0, %0, c6, c0, 0   \n"
	:"=r" (clkcfg)
    );

    printf("ClkConfig: %x\n", clkcfg);
    printf("This command currently not impemented.\n");
    return CMD_NOQUIT;
}

#endif
