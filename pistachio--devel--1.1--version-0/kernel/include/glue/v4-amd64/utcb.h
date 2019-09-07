/*********************************************************************
 *                
 * Copyright (C) 2002-2003,  Karlsruhe University
 *                
 * File path:     glue/v4-amd64/utcb.h
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
 * $Id: utcb.h,v 1.4 2004/06/02 09:25:37 stoess Exp $
 *                
 ********************************************************************/

class utcb_t
{
public:
    /* do not delete this TCB_START_MARKER */

    word_t              padding0[16];           /* -512 .. -385 */
    word_t              br[IPC_NUM_BR];         /* -384 .. -121  */
    threadid_t          my_global_id;           /* -120          */
    word_t              processor_no;           /* -112          */
    word_t              user_defined_handle;    /* -104          */
    threadid_t          pager;                  /* - 96          */
    threadid_t          exception_handler;      /* - 88          */
    u8_t                preempt_flags;          /* - 80          */
    u8_t                cop_flags;
    u16_t               reserved0[3];
    word_t              error_code;             /* - 72           */
    timeout_t           xfer_timeout;           /* - 64           */
    threadid_t          intended_receiver;      /* - 56           */
    threadid_t          virtual_sender;         /* - 48           */
    word_t              reserved1[5];           /* - 40 ..  -1    */
    word_t              mr[IPC_NUM_MR];         /*    0 .. 511    */
   
    /* do not delete this TCB_END_MARKER */
public:
    bool allocate(threadid_t tid);
    void free();

} ;    


