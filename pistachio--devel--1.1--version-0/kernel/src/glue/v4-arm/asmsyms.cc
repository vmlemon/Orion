/*********************************************************************
 *                
 * Copyright (C) 2002-2003,  University of New South Wales
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *                
 * File path:     glue/v4-arm/asmsyms.cc
 * Description:   Various asm definitions for arm
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
 * $Id: asmsyms.cc,v 1.1 2004/06/17 04:32:38 cvansch Exp $
 *                
 ********************************************************************/
#include <l4.h>
#include <mkasmsym.h>

#include INC_API(tcb.h)
#include INC_API(schedule.h)
#include INC_GLUE(space.h)
#include INC_GLUE(resources.h)

MKASMSYM( TSTATE_RUNNING, (word_t) thread_state_t::running );
MKASMSYM( TSTATE_WAITING_FOREVER, (word_t) thread_state_t::waiting_forever );
MKASMSYM( TSTATE_WAITING_NOTIFY, (word_t) thread_state_t::waiting_notify );
MKASMSYM( TSTATE_POLLING, (word_t) thread_state_t::polling );
MKASMSYM( TSTATE_ABORTED, (word_t) thread_state_t::aborted );

MKASMSYM( OFS_SPACE_DOMAIN, offsetof(space_t, pt.pd_split.domain) );
MKASMSYM( OFS_SPACE_PID, offsetof(space_t, pt.pd_split.pid_vspace_raw) );

MKASMSYM( OFS_UTCB_ERROR_CODE, offsetof(utcb_t, error_code));
MKASMSYM( OFS_UTCB_INTENDED_RECEIVER, offsetof(utcb_t, intended_receiver));
MKASMSYM( OFS_UTCB_MR0, offsetof(utcb_t, mr));
MKASMSYM( OFS_UTCB_ACCEPTOR, offsetof(utcb_t, acceptor));
MKASMSYM( OFS_UTCB_NOTIFY_BITS, offsetof(utcb_t, notify_bits));
MKASMSYM( OFS_UTCB_NOTIFY_MASK, offsetof(utcb_t, notify_mask));

MKASMSYM( IPC_ERROR_NOT_ACCEPTED, IPC_SND_ERROR(ERR_IPC_NOT_ACCEPTED) );
MKASMSYM( IPC_ERROR_TAG, IPC_MR0_ERROR );

MKASMSYM( KIPC_RESOURCE_BIT, (word_t) (1ul << KIPC));
MKASMSYM( EXCEPTIONFP_RESOURCE_BIT, (word_t) (1ul << EXCEPTIONFP));

#if defined(CONFIG_TRACEBUFFER)
MKASMSYM( TBUF_LOGMASK, (word_t) &((trace_buffer_t *) 0)->log_mask );
MKASMSYM( TBUF_ACTIVEBUF, (word_t) &((trace_buffer_t *) 0)->active_buffer);
MKASMSYM( TBUF_BUFSIZE, (word_t) &((trace_buffer_t *) 0)->buffer_size);
MKASMSYM( TBUF_BUFHEAD0, (word_t) &((trace_buffer_t *) 0)->buffer_head[0]);
MKASMSYM( TBUF_BUFHEAD1, (word_t) &((trace_buffer_t *) 0)->buffer_head[1]);
MKASMSYM( TBUF_BUFOFF0, (word_t) &((trace_buffer_t *) 0)->buffer_offset[0]);
MKASMSYM( TBUF_BUFOFF1, (word_t) &((trace_buffer_t *) 0)->buffer_offset[1]);
#endif
