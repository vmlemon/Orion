/*********************************************************************
 *
 * Copyright (C) 2004-2005,  National ICT Australia (NICTA)
 *
 * File path:     glue/v4-arm/resource_functions.h
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
 * $Id: resource_functions.h,v 1.9 2004/09/30 08:58:13 cvansch Exp $
 *
 ********************************************************************/


#ifndef __GLUE__V4_ARM__RESOURCE_FUNCTIONS_H__
#define __GLUE__V4_ARM__RESOURCE_FUNCTIONS_H__

#include INC_GLUE(resources.h)
#include INC_API(tcb.h)

/**
 * Mark the thread as being in a kernel IPC
 *
 * @param tcb			current TCB
 */
INLINE void thread_resources_t::set_kernel_ipc (tcb_t * tcb)
{
    tcb->resource_bits += KIPC;
}


/**
 * Clear the kernel IPC bit
 *
 * @param tcb			current TCB
 */
INLINE void thread_resources_t::clear_kernel_ipc (tcb_t * tcb)
{
    tcb->resource_bits -= KIPC;
}


/**
 * Clear the exception fastpath bit
 *
 * @param tcb			current TCB
 */
INLINE void thread_resources_t::clear_except_fp(tcb_t * tcb)
{
    tcb->resource_bits -= EXCEPTIONFP;
}


INLINE void thread_resources_t::save(tcb_t * tcb)
{
}

INLINE void thread_resources_t::load(tcb_t * tcb)
{
}

INLINE void thread_resources_t::dump(tcb_t * tcb)
{
    printf("%c%c", tcb->resource_bits.have_resource(EXCEPTIONFP) ? 'E' : 'e',
		    tcb->resource_bits.have_resource(KIPC) ? 'K' : 'k');
}

#endif /* __GLUE__V4_ARM__RESOURCE_FUNCTIONS_H__ */
