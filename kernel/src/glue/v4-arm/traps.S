/*********************************************************************
 *
 * Copyright (C) 2003-2004,  National ICT Australia (NICTA)
 *
 * File path:      glue/v4-arm/traps.S
 * Description:    Exception vectors
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
 * $Id: traps.S,v 1.28 2004/12/01 23:58:27 cvansch Exp $
 *
 ********************************************************************/

#include <l4.h>
#include INC_ARCH(thread.h)
#include INC_GLUE(syscalls.h)
#include INC_ARCH(asm.h)
#include INC_ARCH(fass.h)

#include <tcb_layout.h>
#include <asmsyms.h>

TRAPS_BEGIN_MARKER  /* Do not remove */

/* Relative branches, loads and stores to locations outside this 4K page are
 * broken, as this is remapped to the high interrupt vector 0xFFFF0000
 */
BEGIN_PROC_TRAPS(arm_high_vector)
	b	arm_reset_exception
	b	arm_undefined_inst_exception
	b	arm_swi_syscall
	b	arm_prefetch_abort_exception
	b	arm_data_abort_exception
	nop
	b	arm_irq_exception
	b	arm_fiq_exception
END_PROC_TRAPS(arm_high_vector)

BEGIN_PROC_TRAPS(arm_reset_exception)
	/* Save r14, SPSR */
	str	lr,	tmp_r14
	mrs	lr,	spsr
	str	lr,	tmp_spsr

	/* Enter supervisor mode, IRQ/FIQ disabled */
	msr	cpsr_c,	#0x000000d3

	/* since SAVE_ALL_INT only does user's banked lr */
	str	lr,	[sp, #(-PT_SIZE + PT_KLR)]

	ldr	lr,	tmp_r14

	SAVE_ALL_INT_TMP_LINKED_DACR	/* Macro sets r12 */

	ldr	r1,	=reset_exception
	mov	r0,	sp

	SET_KERNEL_DACR_LINKED	    /* Macro uses register ip/r12 */

	/* Call C function reset_exception(arm_irq_context_t *) */
	adr	lr,	arm_common_return
	mov	pc,	r1
END_PROC_TRAPS(arm_reset_exception)

BEGIN_PROC_TRAPS(arm_undefined_inst_exception)
	/* Save R14, SPSR */
	sub	lr,	lr,	#4
	str	lr,	tmp_r14
	mrs	lr,	spsr
	str	lr,	tmp_spsr

	/* Enter supervisor mode, IRQ/FIQ disabled */
	msr	cpsr_c,	#0x000000d3

	/* since SAVE_ALL_INT only does user's banked lr */
	str	lr,	[sp, #(-PT_SIZE + PT_KLR)]

	ldr	lr,	tmp_r14

	SAVE_ALL_INT_TMP_LINKED_DACR	/* Macro sets r12 */
	
	ldr	r1,	=undefined_exception
	mov	r0,	sp

	SET_KERNEL_DACR_LINKED	    /* Macro uses register ip/r12 */

	/* Call C function undefined_exception(arm_irq_context_t *) */
	adr	lr,	arm_common_return
	mov	pc,	r1
END_PROC_TRAPS(arm_undefined_inst_exception)

BEGIN_PROC_TRAPS(arm_fiq_exception)
	/* Save R14, SPSR */
	sub	lr,	lr,	#4
	str	lr,	tmp_r14
	mrs	lr,	spsr
	str	lr,	tmp_spsr

	/* Enter supervisor mode, IRQ/FIQ disabled */
	msr	cpsr_c,	#0x000000d3

	/* since SAVE_ALL_INT only does user's banked lr */
	str	lr,	[sp, #(-PT_SIZE + PT_KLR)]

	ldr	lr,	tmp_r14

	SAVE_ALL_INT_TMP_LINKED_DACR	/* Macro sets r12 */

	ldr	r1,	=fiq_exception
	mov	r0,	sp

	SET_KERNEL_DACR_LINKED	    /* Macro uses register ip/r12 */

	/* Call C function fiq_exception(arm_irq_context_t *) */
	adr	lr,	arm_common_return
	mov	pc,	r1
END_PROC_TRAPS(arm_fiq_exception)

	.balign 32
BEGIN_PROC_TRAPS(arm_common_return)
	SET_USER_DACR
	RESTORE_ALL
	movs	pc,	lr
END_PROC_TRAPS(arm_common_return)

/* ==========================================================================================*/
	.balign 32
BEGIN_PROC_TRAPS(arm_swi_syscall)
	str	lr,	tmp_r14			/* Save user PC	(for exception swi) */
	stmdb	sp,	{sp, lr}^		/* Save user LR, SP [syscall no]*/
	nop

	ldr	lr,	[sp, #-8]		/* Get user's SP [syscall no]	*/
	// XXX 2 bubbles (xscale) here!!!

	cmp	lr,	#(0xffffff00 + SYSCALL_ipc);	/* this a IPC syscall?	*/
	bne	check_other_syscalls

	/* ---- SYS_IPC starts here ---- */
	sub	sp,	sp,	#ARM_IPC_STACK_SIZE
	str	r12,	[sp, #0]	/* Save r12 to SP (user SP was in r12) */
	mrs	r12,	spsr		/* Get user CPSR    */
	str	r12,	[sp, #4]	/* Save CPSR	    */

#ifdef CONFIG_IPC_FASTPATH

#define	to_tid		r0
#define	from_tid	r1
#define	mr0		r3
#define	mr1		r4
#define	mr2		r5
#define	mr3		r6
#define	mr4		r7
#define	mr5		r8

#define	to_tcb		r2
#define	current		r9
#define	tmp1		r10

#define	intended	r11
#define	tmp3		r12
#define	tmp6		lr

#if ((KTCB_AREA_START) != 0xe0000000)
#error KTCB_AREA moved
#endif
#if ((USER_KIP_PAGE) != 0xff000000)
#error KIP_AREA moved
#endif

	/***** Fast path IPC *****/

	/* Check for typed messages and flags	bits 13..6 = 0	   TEST1 */
	mov	tmp1,	mr0, LSL #18				/* TEST1 */

	/* Look for a nil to-tid. */				/* TEST0 */
	cmp	to_tid, #0					/* TEST0 - NB no banked registers */

	mov	intended,	to_tid				/* Save intended thread id */

	/* Check that the receive blocks || asynch notify	   TEST3 */
	tstne	mr0,	#(0x6<<12)				/* TEST3 */

	/* Calculate to_tcb (destination tcb)			   CALC1 */
	mov	to_tcb,	to_tid, LSL #(32 - VALID_THREADNO_BITS - L4_GLOBAL_VERSION_BITS)	/* CALC1 */
	beq	ipc_slowpath					/* TEST0/3 - (in xscale shift input delay slot) */

	mov	to_tcb, to_tcb, LSR #(32 - VALID_THREADNO_BITS)	/* CALC1 */
	movs	tmp1,	tmp1, LSR #24				/* TEST1 - (in xscale shift input delay slot) */

	mov	to_tcb, to_tcb, LSL #KTCB_BITS			/* CALC1 */
	add	to_tcb,	to_tcb, #0xe0000000			/* CALC1 */

	/* Calculate current tcb				   CALC2 */
	mov	current, sp, LSR #KTCB_BITS			/* CALC2 */
	ldr	tmp6,	[to_tcb, #OFS_TCB_MYSELF_GLOBAL]	/* TEST4 - (in xscale shift input delay slot) */

	mov	current, current, LSL #KTCB_BITS		/* CALC2 */

	bne	check_async_ipc					/* TEST1 */

	/* !current->get_send_redirector().is_anythread() */	/* TEST 1B */
	ldr	tmp1,	[current, #OFS_TCB_SEND_REDIRECTOR]	/* TEST 1B */

	/* Check to_tcb->get_global_id == to_tid		   TEST4 */
	cmp	tmp6,	to_tid					/* TEST4 */
	bne	check_send_redir_to_invalid			/* TEST4 */

	cmp	tmp1,	#-1					/* TEST 1B */
	bne	check_send_redir_tcb				/* TEST 1B */

continue_send_check_recv_redir:
	/* !to_tcb->get_recv_redirector().is_anythread()*/
	ldr	tmp1,	[to_tcb, #OFS_TCB_RECV_REDIRECTOR]	/* TEST 1C */
	// XXX 2 bubbles (xscale) here!!!
	cmp	tmp1,	#-1					/* TEST 1C */
	bne	check_recv_redir_tcb				/* TEST 1C */

continue_send:
	ldr	tmp3,	[to_tcb, #OFS_TCB_RESOURCE_BITS]	/* TEST9 */
	ldr	tmp6,	[current, #OFS_TCB_RESOURCE_BITS]	/* TEST10 */

#ifdef CONFIG_ENABLE_FASS
	ldr	tmp1,	kernel_access				/* DACR */
#endif
	/* Check if any resource bits are set (except KIPC_RESOURCE_BIT/EXCEPTIONFP_RESOURCE_BIT in to_tcb)   TEST9 | TEST10 */
	bic	tmp3,	tmp3, 	#KIPC_RESOURCE_BIT|EXCEPTIONFP_RESOURCE_BIT	/* TEST9 | TEST10 */
	orrs	tmp3,	tmp3,	tmp6				/* TEST9 | TEST10 */
//	bne	ipc_slowpath					/* TEST9 | TEST10 */

	//SET_KERNEL_DACR
#ifdef CONFIG_ENABLE_FASS
	mcr     p15, 0, tmp1, c3, c0				/* DACR */
	mov	tmp1,	#0					/* DACR */
	mcr	p15, 0, tmp1, c13, c0				/* DACR */
#endif

	ldreq	tmp1,	[to_tcb, #OFS_TCB_THREAD_STATE]		/* TEST5 */

	ldreq	tmp6,	[to_tcb, #OFS_TCB_SPACE]		/* TEST11 */
	/* Get the acceptor */

	/* Check partner (to_tcb) is waiting			   TEST5 */
	cmpeq	tmp1,	#-1					/* TEST5 */
//	bne	ipc_slowpath					/* TEST5/TEST9/TEST10 */

	ldreq	tmp1,	[to_tcb, #OFS_TCB_PARTNER]		/* TEST6 */
	ldreq	tmp3,	[current, #OFS_TCB_MYSELF_GLOBAL]	/* TEST6 */

	bne	ipc_slowpath					/* TEST5/TEST9/TEST10 */

	/* tcb->get_partner().is_anythread()			   TEST6 */
	cmp	tmp1,	#-1					/* TEST6 */
	/* tcb->get_partner() == current->get_global_id()	   TEST6 */
	cmpne	tmp1,	tmp3					/* TEST6 */
	bne	ipc_slowpath					/* TEST6 */

// XXX - move this check up to improve receive only-ipc times
	/* Check if IPC is a Call				   TEST12 */
	cmp	to_tid,	from_tid				/* TEST12 */
	ldrne	tmp1,	[current, #OFS_TCB_SEND_HEAD]		/* TEST8 */

	beq	continue_ipc					/* TEST12 */
	
	/* Require send_head to be empty			   TEST8 */
	cmp	tmp1,	#0					/* TEST8 */
	bne	ipc_slowpath					/* TEST8 */

	/* Check that receive phase blocks			   TEST7 */
	cmp	from_tid,   #-1					/* TEST7 */
	bne	check_other_tcb					/* TEST7 */

continue_ipc:

	/* Check if to_tcb->space == NULL			   TEST11 */
	cmp	tmp6,	#0					/* TEST11 */

#ifdef	CONFIG_ENABLE_FASS
	/* Check destination has a domain */
	add	tmp6,	tmp6,	#OFS_SPACE_DOMAIN
	ldrne	tmp6,	[tmp6]

	beq	check_irq_reply					/* TEST11 */
	mov	tmp1,	#-1

	mov	tmp6,	tmp6,	LSR #2
	cmp	tmp6,	#INVALID_DOMAIN
	beq	ipc_slowpath
#else
	/* Check for address space switch */
	ldr	tmp3,	[current, #OFS_TCB_SPACE]

	beq	check_irq_reply					/* TEST11 */
	mov	tmp1,	#-1

	cmp	tmp6,	tmp3
	bne	ipc_slowpath
#endif

#ifdef CONFIG_TRACEBUFFER
	ldr	tmp3,	=trace_buffer
	ldr	tmp3,	[tmp3]
	ldr	tmp1,	[tmp3, #TBUF_LOGMASK]
	tst	tmp1,	#(1<<3)			/* IPC major_id = 3 */
	beq	end_ipc_trace			/* not traceing this major no */
	
	ldr	tmp1,	[tmp3, #TBUF_ACTIVEBUF]
	tst	tmp1,	#0x80000000
	beq	do_ipc_trace			/* an active buffer */

end_ipc_trace:
	mov	tmp1,	#-1
#endif
	/* Point of no return */

	/* Set thread state to waiting				   STORE1 */
	str	tmp1,	[current, #OFS_TCB_THREAD_STATE]	/* STORE1  tmp1 = -1 */
	/* Set partner						   STORE2/3 */
	str	from_tid,	[current, #OFS_TCB_PARTNER]	/* STORE2 */
#if 0
	ldr	tmp3,	[current, #OFS_TCB_MYSELF_GLOBAL]	/* STORE3 */
	str	tmp3,	[to_tcb, #OFS_TCB_PARTNER]		/* STORE3 */
#endif

	/* redirected IPC */
	cmp	intended,   to_tid
	strne	intended,   [to_tcb, #OFS_UTCB_INTENDED_RECEIVER]

	/* Clean up mr0 (clear receive flags) */
	and	mr0,	mr0,	#(~(0xe << 12))
	orrne	mr0,	mr0,	#(0x2<<12)			/* tag.set_redirected() */

	/* Use copy loop if more than 6 message registers	   TEST2 */
	and	tmp1,	mr0,	#(IPC_NUM_MR-1)			/* TEST2 */
	subs	tmp1,	tmp1,	#6				/* TEST2 */
	bpl	do_ipc_copy					/* TEST2 */

#define	tmp2		r11	    /* only use after last use of intended! */
#define	tmp4		r0	    /* only use after last to_tid use! */
#define	tmp5		r1	    /* only use after last from_tid use! */

fast_path_switch_to:
#ifdef CONFIG_ENABLE_FASS
	/* ACTIVATE NEW DOMAIN */
	/* current_domain = target */
	ldr	tmp4,	arm_current_ptr
	ldr	tmp5,	[to_tcb, #OFS_TCB_UTCB]	/* Get to_tcb UTCB Pointer */
	mov	tmp1,	#1
	str	tmp6,	[tmp4]

	/* SET_BIT_WORD(domain_dirty, target) */
	ldr	tmp4,	arm_domain_ptr
	mov	tmp1,	tmp1,	LSL tmp6

	ldr	tmp3,	[tmp4]

	/* Create switch stack	*/
	sub	sp,	sp,	#(4*4)
	/* Set fast path return address */
	adr	tmp2,	fast_path_recover

	orr	tmp3,	tmp3,	tmp1

	ldr	tmp1,	[to_tcb, #OFS_TCB_SPACE]
	str	tmp3,	[tmp4]

	/* current_pid = get_pid */
	ldr	tmp4,	arm_pid_ptr
	add	tmp1,	tmp1,	#OFS_SPACE_DOMAIN
	ldr	tmp3,	[tmp1,	#(OFS_SPACE_PID - OFS_SPACE_DOMAIN)]

	str	tmp2,	[sp, #12]		/* Save return address */
	mov	tmp1,	#0xff000000		/* USER_KIP_PAGE */

	mov	tmp2,	tmp3,	LSL #23
	str	tmp2,	[tmp4]
3:
#else
	ldr	tmp5,	[to_tcb, #OFS_TCB_UTCB]	/* Get to_tcb UTCB Pointer */
	/* Set fast path return address */
	adr	tmp2,	fast_path_recover
	mov	tmp1,	#0xff000000		/* USER_KIP_PAGE */
	/* Create switch stack	*/
	sub	sp,	sp,	#(4*4)
	str	tmp2,	[sp, #12]		/* Save return address */
#endif
	/* do not use tmp2 here, contains current pid (FASS) */

	/* Get resource bits -- test for KIPC */
	ldr	tmp3,	[to_tcb, #OFS_TCB_RESOURCE_BITS]

	/* Set new UTCB XXX - if we fault after this, (before switch) is this bad? */
	str	tmp5,	[tmp1, #0xff0]		/* UTCB ref */

	/* Save stack		*/
	str	sp,	[current, #OFS_TCB_STACK]

	tst	tmp3,	#KIPC_RESOURCE_BIT|EXCEPTIONFP_RESOURCE_BIT
	bne	ipc_complete_switch_to		/* UTCB in tmp5 */

	/* Set destination thread to running */
	mov	tmp1,	#TSTATE_RUNNING
	str	tmp1,	[to_tcb, #OFS_TCB_THREAD_STATE]

	/* Load new stack	*/
	add	sp,	to_tcb,	#KTCB_SIZE

#undef tmp4
	/* Load result		*/
	ldr	r0,	[current, #OFS_TCB_MYSELF_GLOBAL]

ipc_return_user:
	/* trashes ip/r12 and lr */
//	SET_USER_DACR
// dacr = (0x00000001 | (1 << (2 * current_domain)))
#ifdef CONFIG_ENABLE_FASS             
	/* tmp2 (r11) here, contains current pid (FASS) */
	ldr	tmp3,	arm_current_ptr

	/* restore the user's banked SP, LR, CPSR */
	sub	tmp1,	sp,	#ARM_IPC_STACK_SIZE
	ldr	tmp5,	[tmp1, #4]  /* Get user CPSR	*/

	ldr	tmp3,	[tmp3]

	ldmia	tmp1,	{sp}^	    /* Load user sp	*/

	msr	spsr,	tmp5	    /* Set user CPSR	*/

	mov	tmp3,	tmp3, lsl #1
	mov	tmp5,	#1
	orr	tmp5,	tmp5, tmp5, lsl tmp3

	ldr	lr,	[tmp1, #8]  /* Get user LR	- (in xscale shift delay slot) */

	mcr	p15,	0, tmp5, c3, c0
	mcr	p15,	0, tmp2, c13, c0
#else
	/* restore the user's banked SP, LR, CPSR */
	sub	tmp1,	sp,	#ARM_IPC_STACK_SIZE
	ldr	tmp2,	[tmp1, #4]  /* Get user CPSR	*/
	ldmia	tmp1,	{sp}^	    /* Load user sp	*/
	nop
	ldr	lr,	[tmp1, #8]  /* Get user LR	*/
	msr	spsr,	tmp2	    /* Set user CPSR	*/
#endif

	movs	pc,	lr

#define	tmp4		r0	    /* must be same as above #define tmp4 ... */
#ifdef CONFIG_TRACEBUFFER
do_ipc_trace:
	/* tmp3 = trace_buffer, tmp1 = buffer no */
	str	r3,	[sp, #-4]
	str	r4,	[sp, #-8]
	tst	tmp1,	#1

	ldreq	r3,	[tmp3, #TBUF_BUFHEAD0]
	ldrne	r3,	[tmp3, #TBUF_BUFHEAD1]
	ldr	r4,	[tmp3, #TBUF_BUFSIZE]
	add	r3,	r3,	#(6*4)

	/* Check if enough space in buffer */
	subs	r4,	r3,	r4
	bpl	slow_ipc_trace

	/* Update buffer head */
	tst	tmp1,	#1
	streq	r3,	[tmp3, #TBUF_BUFHEAD0]
	strne	r3,	[tmp3, #TBUF_BUFHEAD1]
	ldreq	r4,	[tmp3, #TBUF_BUFOFF0]
	ldrne	r4,	[tmp3, #TBUF_BUFOFF1]
	
	/* Get buffer offset into tmp3 */
	sub	r3,	r3,	#(6*4)
	add	r3,	r3,	r4
	add	tmp3,	tmp3,	r3

	/* Write trace entry */
	ldr	r4,	=0x00630c50
	ldr	tmp1,	time_ptr
	ldr	r3,	[current, #OFS_TCB_MYSELF_GLOBAL]

	str	r4,	[tmp3, #8]
	ldr	r4,	[tmp1, #0]
	str	to_tid,	[tmp3, #16]
	str	from_tid,	[tmp3, #20]
	str	r4,	[tmp3, #0]
	ldr	r4,	[tmp1, #4]
	str	r3,	[tmp3, #12]
	str	r4,	[tmp3, #4]
	
	ldr	r3,	[sp, #-4]
	ldr	r4,	[sp, #-8]
	b	end_ipc_trace
slow_ipc_trace:
	ldr	r3,	[sp, #-4]
	ldr	r4,	[sp, #-8]
	b	ipc_slowpath
#endif

do_ipc_copy:
	/* destination utcb */
	/* Non-fass only gets here for Intra address space IPC */
	ldr	tmp3,	[to_tcb, #OFS_TCB_UTCB]

	/* current utcb */
	ldr	tmp2,	[current, #OFS_TCB_UTCB]

	/* tmp1 = num to copy - 1
	 * tmp2 = from utcb
	 * tmp3 = to utcb	    */
	add	tmp3,	tmp3,	#88
	add	tmp2,	tmp2,	#88

copy_loop:
	ldr	tmp4,	[tmp2],	#4
	subs	tmp1,	tmp1,	#1
	str	tmp4,	[tmp3],	#4
	bpl	copy_loop

	b	fast_path_switch_to

ipc_complete_switch_to:
#ifdef CONFIG_EXCEPTION_FASTPATH
	/* tmp3 = resource bits */
	tst	tmp3,	#EXCEPTIONFP_RESOURCE_BIT
	movne	r0,	r8		/* MR5(r0) is in r8 */
	addne	r8,	tmp5,	#OFS_UTCB_MR0 + 24
	addne	sp,	to_tcb,	#KTCB_SIZE
	bne	fast_reply_exception	/* Pointer to MR5 in r8 */
#endif

	/* Load stack */
	ldr	sp,	[to_tcb, #OFS_TCB_STACK]

	/* Return to a kernel "C" switch_to */
	/* UTCB is in tmp4 (r2) */
	add	r12,	tmp5,	#OFS_UTCB_MR0
	stmia	r12,	{r3-r8}
	ldmfd	sp!,	{r4, r5, r11, pc}

check_other_tcb:
	/* use tmp1, tmp3, tmp6 only */

	/* from_tid == 0?					   TEST13 */
	cmp	from_tid,   #0					/* TEST13 */
	beq	ipc_slowpath					/* TEST13 */

	/* Calculate from_tcb					   CALC4 */
	mov	tmp1,	from_tid, LSL #(32 - VALID_THREADNO_BITS - L4_GLOBAL_VERSION_BITS)	/* CALC4 */
	mov	tmp1,	tmp1,	LSR #(32 - VALID_THREADNO_BITS)	/* CALC4 */
	mov	tmp1,	tmp1,	LSL #KTCB_BITS			/* CALC4 */
	add	tmp1,	tmp1,	#0xe0000000			/* CALC4 */

	/* Check global ID */
	ldr	tmp3,	[tmp1, #OFS_TCB_MYSELF_GLOBAL]
	ldr	tmp6,	[tmp1, #OFS_TCB_THREAD_STATE]

	cmp	tmp3,	from_tid
	bne	ipc_slowpath

	/*
	 * Check if the thread is polling us --- if so, go to slow path
	 */

	/* is_polling() */	
	cmp	tmp6,	#TSTATE_POLLING
	bne	continue_ipc	    /* from_tcb isn't polling */

	b	ipc_slowpath

fast_path_recover:
	mov	tmp1,	sp,	LSR #KTCB_BITS
	mov	current,tmp1,	LSL #KTCB_BITS
	add	sp,	current,#KTCB_SIZE
	sub	sp,	sp,	#ARM_IPC_STACK_SIZE

	/* Set the state to running */
	mov	tmp1,	#TSTATE_RUNNING
	str	tmp1,	[current, #OFS_TCB_THREAD_STATE]

	/* Load result */
	ldr	r0,	[current, #OFS_TCB_PARTNER]

	b	ipc_syscall_return

	/* tmp1 = send_redirector */
check_send_redir_tcb:
	/* Calculate (destination redirected tcb)		   CALC 1B */
	mov	tmp3,	tmp1, LSL #(32 - VALID_THREADNO_BITS - L4_GLOBAL_VERSION_BITS)	/* CALC 1A */
	mov	tmp3,	tmp3, LSR #(32 - VALID_THREADNO_BITS)	/* CALC 1B */

	mov	tmp3,	tmp3, LSL #KTCB_BITS			/* CALC 1B */
	add	tmp3,	tmp3, #0xe0000000			/* CALC 1B */

	/* Check (redirected)->get_global_id == (redirected tid)   TEST 2B */
	ldr	tmp6,	[tmp3, #OFS_TCB_MYSELF_GLOBAL]		/* TEST 2B */
	cmp	tmp6,	tmp1					/* TEST 2B */
	bne	ipc_slowpath					/* TEST 2B */

	/* send to same space as current or send redirector is allowed */
	ldr	tmp1,	[to_tcb, #OFS_TCB_SPACE]		/* TEST 3B */
	ldr	tmp6,	[tmp3, #OFS_TCB_SPACE]			/* TEST 3B */
	cmp	tmp1,	tmp6					/* TEST 3B */
	beq	continue_send_check_recv_redir			/* TEST 3B */
	ldr	tmp6,	[current, #OFS_TCB_SPACE]		/* TEST 3B */
	cmp	tmp1,	tmp6					/* TEST 3B */
	beq	continue_send_check_recv_redir			/* TEST 3B */

	/* send redirected */
	ldr	to_tid,	[current, #OFS_TCB_SEND_REDIRECTOR]
	mov	to_tcb,	tmp3

	b	continue_send_check_recv_redir

check_recv_redir_tcb:
	/* Calculate (recv redirected tcb)			   CALC 1C */
	mov	tmp3,	tmp1, LSL #(32 - VALID_THREADNO_BITS - L4_GLOBAL_VERSION_BITS)	/* CALC 1A */
	mov	tmp3,	tmp3, LSR #(32 - VALID_THREADNO_BITS)	/* CALC 1C */

	mov	tmp3,	tmp3, LSL #KTCB_BITS			/* CALC 1C */
	add	tmp3,	tmp3, #0xe0000000			/* CALC 1C */

	/* Check (redirected)->get_global_id == (redirected tid)   TEST 2C */
	ldr	tmp6,	[tmp3, #OFS_TCB_MYSELF_GLOBAL]		/* TEST 2C */
	cmp	tmp6,	tmp1					/* TEST 2C */
	bne	ipc_slowpath					/* TEST 2C */

	/* recv from same space as dest or recieve redirector allowed */
	ldr	tmp1,	[tmp3, #OFS_TCB_SPACE]			/* TEST 3C */
	ldr	tmp6,	[current, #OFS_TCB_SPACE]		/* TEST 3C */
	cmp	tmp1,	tmp6					/* TEST 3C */
	beq	continue_send					/* TEST 3C */
	ldr	tmp6,	[to_tcb, #OFS_TCB_SPACE]		/* TEST 3C */
	cmp	tmp1,	tmp6					/* TEST 3C */
	beq	continue_send					/* TEST 3C */

	/* recv redirected */
	ldr	to_tid,	[current, #OFS_TCB_SEND_REDIRECTOR]
	mov	to_tcb,	tmp3
	/* recheck new destination thread's recv redirector */
	b	continue_send_check_recv_redir

	/* tmp1 = (mr0 >> 6) & 0xff */
check_async_ipc:
	cmp	tmp6,	to_tid				/* Check to_tcb->get_global_id == to_tid */
	cmpeq	tmp1,	#0x80				/* Check notify bit in tag */

	/* Send and receive redirectors must be "ANYTHREAD"	   TEST A5 */
	ldr	tmp3,	[current, #OFS_TCB_SEND_REDIRECTOR]	/* TEST A5 */
	ldr	tmp6,	[to_tcb, #OFS_TCB_RECV_REDIRECTOR]	/* TEST A5 */

	/* from_tid != niltread ?				   TEST A0 */
	cmpeq	from_tid,   #0					/* TEST A0 */
	cmpeq	tmp3,	#-1					/* TEST A5 */
	cmpeq	tmp6,	#-1					/* TEST A5 */

	bne	ipc_slowpath					/* above tests */

	/* Get to_tcb->acceptor ( to_tcb->get_br(0) )		   TEST A1 */
	ldr	tmp6,	[to_tcb, #OFS_TCB_UTCB]			/* TEST A1 */

	/* mr2, mr3, mr4 - can be used as temp from here */

	SET_KERNEL_DACR	    /* Macro uses register ip/r12/tmp3 */

	ldr	mr3,	[tmp6, #OFS_UTCB_ACCEPTOR]		/* TEST A1 */
	ldr	mr4,	[tmp6, #OFS_UTCB_NOTIFY_MASK]		/* TEST A2 */
	ldr	tmp3,	[tmp6, #OFS_UTCB_NOTIFY_BITS]		/* CALC A1 */

	tst	mr3,	#2					/* TEST A1 */
	beq	async_no_acceptor				/* TEST A1 - UTCB must be in tmp6 here*/

	/* OR in the bits */
	orr	tmp3,	tmp3,	mr1				/* CALC A1 */

	/* to_tcb->get_notify_bits() & to_tcb->get_br(1)	   TEST A2 */
	ands	tmp1,	mr4,	tmp3				/* TEST A2 */

	ldrne	mr2,	[to_tcb, #OFS_TCB_THREAD_STATE]		/* TEST A3 */
	str	tmp3,	[tmp6, #OFS_UTCB_NOTIFY_BITS]		/* CALC A1 */

	beq	async_no_trigger				/* TEST A2 */

	/* Check is_waiting()					   TEST A3 */
	cmp	mr2,	#TSTATE_WAITING_NOTIFY			/* TEST A3 */
	beq	1f

	ldr	mr3,	[to_tcb, #OFS_TCB_PARTNER]		/* TEST A4 */
	cmp	mr2,	#TSTATE_WAITING_FOREVER			/* TEST A3 */

	/* Check to_tcb->partner->is_anythread			   TEST A4 */
	cmpeq	mr3,	#-1					/* TEST A4 */
	bne	async_no_trigger				/* TEST A4 */

1:
	mov	tmp1,	#0					/* OP A1 */

	/* to_tcb->set_partner(NILTHREAD)			   OP A1 */
	str	tmp1,	[to_tcb, #OFS_TCB_PARTNER]		/* OP A1 */

	/* set to_tcb and current states to running */
	mov	tmp3,	#TSTATE_RUNNING
	str	tmp3,	[to_tcb, #OFS_TCB_THREAD_STATE]
	str	tmp3,	[current, #OFS_TCB_THREAD_STATE]

	/* to_tcb	= r2 */
	/* current	= r9 */

	ldr	tmp1,	=async_fp_helper_asm
	mov	lr,	pc
	mov	pc,	tmp1
	/* async_fp_helper_asm returns here */
async_no_trigger:
	ldr	tmp2,	arm_pid_ptr
	add	sp,	sp,	#ARM_IPC_STACK_SIZE
	mov	mr0,	#0			/* clear error status */
2:
	ldr	tmp2,	[tmp2]
	b	ipc_return_user

async_no_acceptor:
	ldr	tmp6,	[current, #OFS_TCB_UTCB]
	add	sp,	sp,	#ARM_IPC_STACK_SIZE
	/* No acceptor, set error */
	mov	tmp3,	#IPC_ERROR_NOT_ACCEPTED
	str	tmp3,	[tmp6, #OFS_UTCB_ERROR_CODE]
	mov	mr0,	#IPC_ERROR_TAG
	ldr	tmp2,	arm_pid_ptr
	b	2b

	/* Any send should be redirected, even if to_tid is invalid */
check_send_redir_to_invalid:	    /* can safely use tmp1, tmp3, tmp6 */

	/* !current->get_send_redirector().is_anythread() */	/* TEST 1A */
	ldr	to_tid,	[current, #OFS_TCB_SEND_REDIRECTOR]	/* TEST 1A */

	/* Calculate to_tcb (destination redirected tcb)	   CALC 1A */
	mov	to_tcb,	to_tid, LSL #(32 - VALID_THREADNO_BITS - L4_GLOBAL_VERSION_BITS)	/* CALC 1A */
	mov	to_tcb,	to_tcb, LSR #(32 - VALID_THREADNO_BITS)	/* CALC 1A */

	cmp	to_tid,	#-1					/* TEST 1A */
	beq	ipc_slowpath					/* TEST 1A */

	mov	to_tcb,	to_tcb, LSL #KTCB_BITS			/* CALC 1A */
	add	to_tcb,	to_tcb, #0xe0000000			/* CALC 1A */

	/* Check to_tcb(redirected)->get_global_id == to_tid(redirected)    TEST 2A */
	ldr	tmp1,	[to_tcb, #OFS_TCB_MYSELF_GLOBAL]	/* TEST 2A */
	cmp	tmp1,	to_tid					/* TEST 2A */
	beq	continue_send_check_recv_redir			/* TEST 2A */

	b	ipc_slowpath

check_irq_reply:
	/* Check if dest is an interrupt thread */
	mov	tmp1,	to_tid,	LSR #L4_GLOBAL_VERSION_BITS
#if defined(CONFIG_PLAT_PLEB) || defined(CONFIG_PLAT_PLEB2) || defined(CONFIG_PLAT_IXDP425)\
    || defined(CONFIG_PLAT_CSB337)
#define IRQS   32
#elif defined(CONFIG_PLAT_INNOVATOR)
#define IRQS   64
#elif defined(CONFIG_PLAT_LN2410SBC)
#define IRQS   32 /* FIXME! */
#else
#error fix this
#endif
	cmp	tmp1,	#IRQS

	adrlt	lr,	fast_path_recover

	SET_KERNEL_DACR	    /* Macro uses register ip/r12 */

	ldrlt	pc,	=irq_ipc_reply
	/* fall through */
#undef	to_tid
#undef	from_tid
#undef	timeouts
#undef	mr0
#undef	mr1
#undef	mr2
#undef	mr3
#undef	mr4

#undef	to_tcb
#undef	current
#undef	tmp1

#undef	intended
#undef	tmp2
#undef	tmp3
#undef	tmp4
#undef	tmp5
#undef	tmp6

#endif	/* CONFIG_IPC_FASTPATH */

ipc_slowpath:
	/* Save message registers */
	ldr	ip,	kernel_access	// part of SET_KERNEL_DACR
	mov	lr,	#0xff000000
	ldr	lr,	[lr, #0xff0]
	mcr	p15, 0, ip, c3, c0

	mov	ip,	#0
	add	lr,	lr,	#OFS_UTCB_MR0
	stmia	lr,	{r3-r8}

	// SET_KERNEL_DACR	    /* Macro uses register ip/r12 */
	mcr	p15, 0, ip, c13, c0

	adr	lr,	ipc_syscall_return
	ldr	pc,	arm_syscall_vectors + 0	    /* sys_ipc */

check_other_syscalls:
	/* User SP less than 0xffffff00? - SWI exception */
        bcc     arm_swi_exception

	/* svc_sp should point to current thread's kernel stack in the KTCB */

	/* Test to see if it is a syscall */
	and	lr,	lr,	#0x000000ff
	cmp	lr,	#SYSCALL_limit
	bhi	arm_l4_kdebug_syscall

arm_std_syscall:
	/* It is a syscall, so save the user's banked SP as well as
	 * CPSR
	 */
	sub	sp,	sp,	#ARM_IPC_STACK_SIZE
	str	r12,	[sp, #0]	/* Save r12 to SP (user SP was in r12) */
	mrs	r12,	spsr		/* Get user CPSR    */
	str	r12,	[sp, #4]	/* Save CPSR	    */

	SET_KERNEL_DACR	    /* Macro uses register ip/r12 */

	/* Calling registers:
	 *   r0, r1, r2, r3, r4, r5, r6, r7 : arguments 1 - 8
	 * Retuned registers:
	 *   r0, r1, r2, r3, r4, r5, r6     : returned 1 - 7
	 */
	mov	r12,	lr
	adr	lr,	syscall_return
	ldr	pc,	[pc, r12]
	nop

arm_syscall_vectors:
	.word	sys_ipc
	.word	sys_thread_switch
	.word	vector_sys_thread_control_exargs
	.word	vector_sys_exchange_registers_exargs
	.word	vector_sys_schedule_exargs 
	.word	sys_unmap 
	.word	sys_space_control
	.word	syscall_return /* sys_processor_control */
	.word	vector_sys_memory_control_exargs 
	.word   syscall_return /* unused */
	.word   sys_ipc /* lipc */
END_PROC_TRAPS(arm_swi_syscall)

#define SYS_EXARGS(name, lastreg, numregs)	\
BEGIN_PROC_TRAPS(name##_exargs)			\
	ldr	r12,	=name##;		\
	stmdb	sp!,	{r4##lastreg};		\
	mov	lr,	pc;			\
	mov	pc,	r12;			\
						\
	add	sp,	sp,     #(numregs * 4);	\
	b	syscall_return;			\
END_PROC_TRAPS(name##_exargs)

SYS_EXARGS(sys_thread_control,-r6, 3)
SYS_EXARGS(sys_exchange_registers,-r7, 4)
SYS_EXARGS(sys_schedule, , 1)
SYS_EXARGS(sys_memory_control, , 1)


BEGIN_PROC_TRAPS(arm_swi_exception)
	sub	sp,	sp,	#4
	stmdb	sp,	{r0-r14}^		/* Save user r0..r14	*/
	ldr	lr,	tmp_r14			/* Get user PC		*/
	sub	sp,	sp,	#(PT_SIZE-4)	/* Push arm_irq_context_t on stack  */
	mrs	r11,	spsr			/* Get user CPSR	*/
	str	lr,	[sp, #PT_PC]		/* Save the user's PC	*/
	str	r11,	[sp, #PT_CPSR]		/* Save CPSR		*/
	
#ifdef CONFIG_EXCEPTION_FASTPATH
#include    INC_GLUE(exception.h)
	/***** Fast path Exception IPC *****/

	/* Registers R4-R7 - remain in place
	 * Register R0->R8
	 * Registers R1-R3,LR - must go into MR6-MR9
	 */
#define	current	r9
#define	to_tcb	r10
#define	tmp1	r11
#define	tmp2	r8
#define	tmp3	r12

	/* Calculate current tcb				   CALC_E1 */
	mov	current, sp, LSR #KTCB_BITS			/* CALC_E1 */
	mov	current, current, LSL #KTCB_BITS		/* CALC_E1 */

	/* Get exception_handler tcb				   CALC_E2 */
	ldr	tmp1,	[current, #OFS_TCB_SCHEDULER]		/* CALC_E2 */
	mov	to_tcb,	tmp1, LSL #(32 - VALID_THREADNO_BITS - L4_GLOBAL_VERSION_BITS)	/* CALC_E2 */
	mov	to_tcb, to_tcb, LSR #(32 - VALID_THREADNO_BITS)	/* CALC_E2 */
	mov	to_tcb, to_tcb, LSL #KTCB_BITS			/* CALC_E2 */
	add	to_tcb,	to_tcb, #0xe0000000			/* CALC_E2 */

	ldr	tmp2,	[to_tcb, #OFS_TCB_MYSELF_GLOBAL]	/* TEST_E1 */
	ldr	tmp3,	[to_tcb, #OFS_TCB_RESOURCE_BITS]	/* TEST_E2 */
	/* Check to_tcb->get_global_id == to_tid		   TEST_E1 */
	cmp	tmp2,	tmp1					/* TEST_E1 */
	//bne	exception_slowpath				/* TEST_E1 */

	/* Check if any resource bits are set (except KIPC_RESOURCE_BIT in to_tcb)   TEST9 | TEST10 */
	ldreq	tmp2,	[current, #OFS_TCB_RESOURCE_BITS]	/* TEST_E3 */
	biceq	tmp3,	tmp3, 	#KIPC_RESOURCE_BIT		/* TEST_E2 | TEST_E3 */
	ldreq	tmp1,	[to_tcb, #OFS_TCB_THREAD_STATE]		/* TEST_E4 */
	orreqs	tmp3,	tmp3,	tmp2				/* TEST_E2 | TEST_E3 */
	//bne	exception_slowpath				/* TEST_E1 | TEST_E2 | TEST_E3 */

	ldreq	tmp3,	[to_tcb, #OFS_TCB_PARTNER]		/* TEST_E5 */
	ldreq	tmp2,	[current, #OFS_TCB_MYSELF_GLOBAL]	/* TEST_E5 */

	/* Check partner (to_tcb) is waiting			   TEST_E4 */
	cmp	tmp1,	#-1					/* TEST_E4 */

	bne	exception_slowpath				/* TEST_E1 | TEST_E2 | TEST_E3 | TEST_E4 */

	ldr	tmp1,	[current, #OFS_TCB_SEND_HEAD]		/* TEST_E6 */
	/* tcb->get_partner().is_anythread()			   TEST_E5 */
	cmp	tmp3,	#-1					/* TEST_E5 */
	/* tcb->get_partner() == current->get_global_id()	   TEST_E5 */
	cmpne	tmp3,	tmp2					/* TEST_E5 */
	ldreq	tmp2,	[to_tcb, #OFS_TCB_SPACE]		/* TEST_E7 */
	//bne	exception_slowpath				/* TEST_E5 */

	/* Require send_head to be empty			   TEST_E6 */
	cmpeq	tmp1,	#0					/* TEST_E6 */
	//bne	exception_slowpath				/* TEST_E6 */

	/* Check if to_tcb->space == NULL			   TEST_E7 */
	cmpeq	tmp2,	#0					/* TEST_E7 */
	beq	exception_slowpath				/* TEST_E7 */

exception_do_ipc:
#ifdef	CONFIG_ENABLE_FASS
	/* Check destination has a domain */
	add	tmp2,	tmp2,	#OFS_SPACE_DOMAIN
	ldr	tmp2,	[tmp2]

	SET_KERNEL_DACR	    /* Macro uses register ip/r12/tmp3 */

	mov	tmp2,	tmp2,	LSR #2
	cmp	tmp2,	#INVALID_DOMAIN
	beq	exception_slowpath
	/* domain in tmp2 */
#else
	/* Check for address space switch */
	ldr	tmp3,	[current, #OFS_TCB_SPACE]
	cmp	tmp2,	tmp3
	bne	exception_slowpath
#endif

#ifdef CONFIG_TRACEBUFFER
	ldr	tmp3,	=trace_buffer
	ldr	tmp3,	[tmp3]
	ldr	tmp1,	[tmp3, #TBUF_LOGMASK]
	tst	tmp1,	#(1<<3)			/* IPC major_id = 3 */
	beq	end_excep_trace			/* not tracing this major no */
	
	ldr	tmp1,	[tmp3, #TBUF_ACTIVEBUF]
	tst	tmp1,	#0x80000000
	beq	do_excep_trace			/* an active buffer */

end_excep_trace:
#endif
	/* Point of no return */
	mov	tmp1,	#-1
	ldr	tmp3,	[current, #OFS_TCB_SCHEDULER]		/* STORE_E2 */
	/* Set thread state to waiting				   STORE_E1 */
	str	tmp1,	[current, #OFS_TCB_THREAD_STATE]	/* STORE_E1  tmp1 = -1 */
	mov	tmp1,	#TSTATE_RUNNING
	/* Set partner						   STORE_E2/E3 */
	str	tmp3,	[current, #OFS_TCB_PARTNER]		/* STORE_E2 */

	ldr	tmp3,	[to_tcb, #OFS_TCB_UTCB]
	/* Set thread saved-state to running			   STORE_E4 */
	str	tmp1,	[current, #OFS_TCB_SAVED_STATE]		/* STORE_E4 */
#if 0
	ldr	tmp1,	[current, #OFS_TCB_MYSELF_GLOBAL]	/* STORE_E3 */
	str	tmp1,	[to_tcb, #OFS_TCB_PARTNER]		/* STORE_E3 */
#endif

	/* destination utcb */
	/* Non-fass only gets here for Intra address space IPC */

	add	tmp3,	tmp3,	#OFS_UTCB_MR0 + 24	/* Save to UTCB */
	stmia	tmp3,	{r1-r3, lr}			/* Save to UTCB r1-r3,pc */

#define	tag	r3
#define tmp4	r1
#define tmp5	r2
	/* Setup Exception-IPC tag */
	mov	tag,	#(EXCEPT_IPC_SYS_TAG_HI)		/* SETUP_E1 */

	ldr	tmp4,	[sp,	#PT_LR]
	ldr	tmp5,	[sp,	#PT_SP]

	mov	tag,	tag,	LSL #20				/* SETUP_E1 */

	str	tmp4,	[tmp3,	#20]			/* Save to UTCB user LR		*/
	str	tmp5,	[tmp3,	#16]			/* Save to UTCB user SP		*/

	mrs	tmp1,	spsr

	ldr	tmp5,	[lr,	#-4]			/* Read swi instruction		*/

	orr	tag,	tag,	#EXCEPT_IPC_SYS_TAG_LO		/* SETUP_E1 */

	str	tmp1,	[tmp3,	#28]			/* Save to UTCB user CPSR	*/
	str	tmp5,	[tmp3,	#24]			/* Save to UTCB, SYSCALL	*/

exception_switch_to:
#ifdef CONFIG_ENABLE_FASS
	/* domain in tmp2 */
	/* ACTIVATE NEW DOMAIN */
	/* current_domain = target */
	ldr	tmp4,	arm_current_ptr
	mov	tmp1,	#1
	str	tmp2,	[tmp4]

	/* SET_BIT_WORD(domain_dirty, target) */
	ldr	tmp4,	arm_domain_ptr
	mov	tmp1,	tmp1,	LSL tmp2
	ldr	tmp3,	[tmp4]

	/* Set new UTCB XXX - if we fault after this, (before switch) is this bad? */
	ldr	tmp5,	[to_tcb, #OFS_TCB_UTCB]
	mov	tmp2,	#0xff000000	/* USER_KIP_PAGE */

	orr	tmp3,	tmp3,	tmp1
	str	tmp3,	[tmp4]

	/* current_pid = get_pid */
	ldr	tmp1,	[to_tcb, #OFS_TCB_SPACE]
	ldr	tmp4,	arm_pid_ptr
	/* Create switch stack */
	sub	sp,	sp,	#(4*4)

	add	tmp1,	tmp1,	#OFS_SPACE_DOMAIN
	ldr	tmp3,	[tmp1,	#(OFS_SPACE_PID - OFS_SPACE_DOMAIN)]

	str	tmp5,	[tmp2, #0xff0]		/* UTCB ref */
	/* Set fast path return address */
	adr	tmp1,	exception_fastpath_recover

	mov	tmp3,	tmp3,	LSL #23
	str	tmp3,	[tmp4]
#else
	/* Set new UTCB XXX - if we fault after this, (before switch) is this bad? */
	ldr	tmp5,	[to_tcb, #OFS_TCB_UTCB]
	mov	tmp1,	#0xff000000		/* USER_KIP_PAGE */
	str	tmp5,	[tmp1, #0xff0]		/* UTCB ref */

	/* Set fast path return address */
	adr	tmp1,	exception_fastpath_recover

	/* Create switch stack */
	sub	sp,	sp,	#(4*4)
#endif

	/* Get resource bits -- test for KIPC */
	ldr	tmp2,	[to_tcb, #OFS_TCB_RESOURCE_BITS]

	str	tmp1,	[sp, #12]		/* Save return address */

	/* Set resource bit -- EXCEPTIONFP for fastpath replies */
	mov	tmp3,	#EXCEPTIONFP_RESOURCE_BIT
	str	tmp3,	[current, #OFS_TCB_RESOURCE_BITS]

	/* Save stack */
	str	sp,	[current, #OFS_TCB_STACK]

	tst	tmp2,	#KIPC_RESOURCE_BIT

	mov	r8,	r0			/* Put r0 in r8/tmp2 (MR5) */
	bne	ipc_complete_switch_to		/* UTCB in tmp5 */

	/* Set destination thread to running */
	mov	tmp1,	#TSTATE_RUNNING
	str	tmp1,	[to_tcb, #OFS_TCB_THREAD_STATE]

	/* Load new stack */
	add	sp,	to_tcb,	#KTCB_SIZE

	/* place pid_ptr in r11 for ipc_return_user */
	ldr	r11,	arm_pid_ptr

	/* Load result (should be cached from before XXX) */
	ldr	r0,	[current, #OFS_TCB_MYSELF_GLOBAL]

	ldr	r11,	[r11]
	b	ipc_return_user

#ifdef CONFIG_TRACEBUFFER
do_excep_trace:
	/* tmp3 = trace_buffer, tmp1 = buffer no */
	str	r3,	[sp, #-4]
	str	r4,	[sp, #-8]
	tst	tmp1,	#1

	ldreq	r3,	[tmp3, #TBUF_BUFHEAD0]
	ldrne	r3,	[tmp3, #TBUF_BUFHEAD1]
	ldr	r4,	[tmp3, #TBUF_BUFSIZE]
	add	r3,	r3,	#(5*4)

	/* Check if enough space in buffer */
	subs	r4,	r3,	r4
	bpl	slow_excep_trace

	/* Update buffer head */
	tst	tmp1,	#1
	streq	r3,	[tmp3, #TBUF_BUFHEAD0]
	strne	r3,	[tmp3, #TBUF_BUFHEAD1]
	ldreq	r4,	[tmp3, #TBUF_BUFOFF0]
	ldrne	r4,	[tmp3, #TBUF_BUFOFF1]
	
	/* Get buffer offset */
	sub	r3,	r3,	#(5*4)
	add	r3,	r3,	r4
	add	r3,	r3,	tmp3

	/* Write trace entry */
	ldr	r4,	=0x00620a51
	ldr	tmp1,	time_ptr
	ldr	tmp3,	[current, #OFS_TCB_MYSELF_GLOBAL]

	str	r4,	[r3, #8]
	ldr	r4,	[tmp1, #0]
	str	tmp3,	[r3, #12]
	ldr	tmp3,	[to_tcb, #OFS_TCB_MYSELF_GLOBAL]
	str	r4,	[r3, #0]
	ldr	r4,	[tmp1, #4]
	str	tmp3,	[r3, #16]
	str	r4,	[r3, #4]
	
	ldr	r3,	[sp, #-4]
	ldr	r4,	[sp, #-8]
	b	end_excep_trace
slow_excep_trace:
	ldr	r3,	[sp, #-4]
	ldr	r4,	[sp, #-8]
	b	exception_slowpath
#endif

#undef tag
#undef to_tcb
#undef current
#undef tmp3
#undef tmp4
#undef tmp5

/* define current such that it matches to_tcb in ipc_fastpath */
#define current	    r2
exception_fastpath_recover:
	mov	tmp1,	sp,	LSR #KTCB_BITS
	mov	current,tmp1,	LSL #KTCB_BITS
	add	sp,	current,#KTCB_SIZE

	/* Restore USER registers from message */
	// current = r9
	// tmp1 = r11, tmp2 = r8
	mov	tmp2,	#0xff000000
	ldr	tmp2,	[tmp2, #0xff0]
	add	tmp2,	tmp2,	#OFS_UTCB_MR0 + 4
	ldmia	tmp2!,	{r4-r7}			/* Load R4-R7		*/
	ldmia	tmp2!,	{r0}			/* Load R0		*/
fast_reply_exception:
	/* trashes ip/r12 and lr */
	SET_USER_DACR

	/* Set the state to running */
	mov	tmp1,	#TSTATE_RUNNING
	str	tmp1,	[current, #OFS_TCB_THREAD_STATE]
	/* Set the saved-state to aborted */
	mov	tmp1,	#TSTATE_ABORTED
	str	tmp1,	[current, #OFS_TCB_SAVED_STATE]

	/* Clear resource bit -- EXCEPTIONFP */
	mov	tmp1,	#0
	str	tmp1,	[current, #OFS_TCB_RESOURCE_BITS]

#undef current
	ldmia	tmp2!,	{r1-r3, lr}		/* Load R1-R3 + user PC */

	ldmia	tmp2,	{sp, lr}^		/* Load user SP, LR	*/
	ldr	tmp1,	[tmp2,	#12]		/* Load user CPSR (accesses no banked registers)	*/
	sub	sp,	sp,	#12
	msr	spsr,	tmp1			/* Set user CPSR */

#undef tmp1
#undef tmp2
	ldmdb	sp,	{r8-r12}		/* Load user R8, R9, R10, R11, R12 */
	add	sp,	sp,	#12

	movs	pc,	lr

#endif	/* EXCEPTION_FASTPATH */
exception_slowpath:

	SET_KERNEL_DACR	    /* Macro uses register ip/r12 */

	/* Call C function syscall_exception(arm_irq_context_t *) */
	ldr	r1,	=syscall_exception
	mov	r0,	sp
	adr	lr,	arm_common_return2
	mov	pc,	r1
END_PROC_TRAPS(arm_swi_exception)

	.balign 32
arm_domain_ptr:
	.word	domain_dirty
arm_current_ptr:
	.word	current_domain
arm_pid_ptr:
	.word	current_pid
#ifdef CONFIG_ENABLE_FASS
arm_fass_ptr:
	.word	arm_fass
#endif

	.balign 32
BEGIN_PROC_TRAPS(ipc_syscall_return)
	SET_USER_DACR

	mov	r12,	#0xff000000
	ldr	r12,	[r12, #0xff0]
	// XXX 2 bubbles (xscale) here!!!
	add	r12,	r12,	#OFS_UTCB_MR0
	ldmia	r12,	{r3-r8}

	/* Fall through */
END_PROC_TRAPS(ipc_syscall_return)

BEGIN_PROC_TRAPS(common_syscall_return)
	/* restore the user's banked SP, LR, CPSR */
	ldr	r12,	[sp, #4]    /* Get user CPSR	*/
	ldmia	sp,	{sp}^
	nop
	ldr	lr,	[sp, #8]    /* Get user LR	*/
	msr	spsr,	r12
	add	sp,	sp,	#ARM_IPC_STACK_SIZE

	movs	pc,	lr
END_PROC_TRAPS(common_syscall_return)

BEGIN_PROC_TRAPS(syscall_return)
	SET_USER_DACR
	b	common_syscall_return
END_PROC_TRAPS(syscall_return)

	.balign 32
BEGIN_PROC_TRAPS(arm_common_return2)
	SET_USER_DACR
	RESTORE_ALL
	movs	pc,	lr
END_PROC_TRAPS(arm_common_return2)

BEGIN_PROC_TRAPS(arm_l4_kdebug_syscall)
	sub	sp,	sp,	#ARM_IPC_STACK_SIZE
	str	r12,	[sp, #0]	/* Save r12 to SP (user SP was in r12) */
	stmdb	sp,	{r0-r11, r14}	/* Save user registers, syscall num to r12 */
	mrs	r0,	spsr		/* Get user CPSR */
	sub	sp,	sp,	#(PT_SIZE-12)	/* Fix stack to be an arm_irq_context_t */
	str	r0,	[sp, #PT_CPSR]	/* Save user CPSR */

	SET_KERNEL_DACR	    /* Macro uses register ip/r12 */

	ldr	r1,	=arm_misc_l4_syscall
	mov	r0,	sp
	adr	lr,	arm_common_return2
	mov	pc,	r1
END_PROC_TRAPS(arm_l4_kdebug_syscall)

BEGIN_PROC_TRAPS(arm_prefetch_abort_exception)
	/* Save R14, SPSR */
	sub	lr,	lr,	#4
	str	lr,	tmp_r14
	mrs	lr,	spsr
	str	lr,	tmp_spsr

	/* Enter supervisor mode, IRQ/FIQ disabled */
	msr	cpsr_c,	#0x000000d3

	/* since SAVE_ALL_INT only does user's banked lr */
	str	lr,	[sp, #(-PT_SIZE + PT_KLR)]
	/* Load saved user's program counter */
	ldr	lr,	tmp_r14

	/* Even if the fault came from the kernel, it won't be on the current
	 * stack as KTCBs are faulted on for allocation prior to the use of
	 * their stacks 
	 */

	SAVE_ALL_INT_TMP_LINKED_DACR	/* Macro sets r12 */

	/* Pointer to base of current arm_irq_context_t record */
	mov	r2,	sp

	/* Faulting address */
	ldr	r1,	[sp, #PT_PC]

	SET_KERNEL_DACR_LINKED	    /* Macro uses register ip/r12 */

	/* Assume it was a page fault for now */
	ldr	r4,	=arm_page_fault

	/* Fault status - not updated on prefetch abort */
	mov	r0,	#0
	/* Signal a prefetchabort */
	mov	r3,	#0

	/* Process the page fault */
	adr	lr,	arm_abort_return
	mov	pc,	r4
END_PROC_TRAPS(arm_prefetch_abort_exception)

BEGIN_PROC_TRAPS(arm_data_abort_exception)
	/* Save R14, SPSR */
	sub	lr,	lr,	#8
	str	lr,	tmp_r14
	mrs	lr,	spsr
	str	lr,	tmp_spsr

	/* Enter supervisor mode, IRQ/FIQ disabled */
	msr	cpsr_c,	#0x000000d3

	/* since SAVE_ALL_INT only does user's banked lr */
	str	lr,	[sp, #(-PT_SIZE + PT_KLR)]
	/* Load saved user's program counter */
	ldr	lr,	tmp_r14

	/* Even if the fault came from the kernel, it won't be on the current
	 * stack as KTCBs are faulted on for allocation prior to the use of
	 * their stacks
	 */
	
	SAVE_ALL_INT_TMP_LINKED_DACR	/* Macro sets r12 */

	/* Pointer to base of current arm_irq_context_t record */
	mov	r2,	sp

	ldr	r4,	=arm_page_fault

	SET_KERNEL_DACR_LINKED	    /* Macro uses register ip/r12 */

	/* Fault status */
	mrc	p15, 0, r0, c5, c0, 0
	/* Faulting address */
	mrc	p15, 0, r1, c6, c0, 0
	/* Signal a dataabort */
	mov	r3,	#1

	/* Process the page fault */
	adr	lr,	arm_abort_return
	mov	pc,	r4
END_PROC_TRAPS(arm_data_abort_exception)

BEGIN_PROC_TRAPS(arm_abort_return)
//XXX do we really need to switch modes back?
	SET_USER_DACR_K
	/* Puts user's program counter/status reg in tmp_r14/tmp_spsr */
	RESTORE_ALL_ABT

	ldr	lr,	[sp, #(-PT_SIZE + PT_KLR)]

	/* Enter abort mode, IRQ/FIQ disabled */
	msr	cpsr_c,	#0x000000d7

	/* Restore R14, SPSR */
	ldr	sp,	tmp_spsr
	/* Load saved user's program counter */
	ldr	lr,	tmp_r14
	msr	spsr,	sp

	movs	pc,	lr
END_PROC_TRAPS(arm_abort_return)

BEGIN_PROC_TRAPS(arm_irq_exception)
//XXX do we really need to switch modes back?
	/* Save R14, SPSR */
	sub	lr,	lr,	#4
	str	lr,	tmp_r14
	mrs	lr,	spsr
	str	lr,	tmp_spsr

	/* Enter supervisor mode, IRQ/FIQ disabled */
	msr	cpsr_c,	#0x000000d3

	/* since SAVE_ALL_INT only does user's banked lr */
	str	lr,	[sp, #(-PT_SIZE + PT_KLR)]

	ldr	lr,	tmp_r14

	SAVE_ALL_INT_TMP_LINKED_DACR	/* Macro sets r12 */

	ldr	r4,	=arm_irq
	mov	r0,	sp

	SET_KERNEL_DACR_LINKED	    /* Macro uses register ip/r12 */

	mov	lr,	pc
	mov	pc,	r4

	SET_USER_DACR_K
	RESTORE_ALL_ABT

	ldr	lr,	[sp, #(-PT_SIZE + PT_KLR)]

	/* Enter irq mode, IRQ/FIQ disabled */
	msr	cpsr_c, #0x000000d2

	/* Restore R14, SPSR */
	ldr	sp,	tmp_spsr
	ldr	lr,	tmp_r14
	msr	spsr,	sp

	movs	pc,	lr
END_PROC_TRAPS(arm_irq_exception)

	.balign 32
/* these accessed at compile address */
.global domain_dirty
domain_dirty:
	.word	0xdeadbeef

.global current_domain
current_domain:
	.word	0xdeadbeef

.global current_pid
current_pid:
	.word	0xdeadbeef

	.balign 32
/* these accessed at remapped address
 * !note avoid cache alias with above */
VECTOR_WORD(tmp_r14)
	.word	0xdeadbeef
VECTOR_WORD(tmp_spsr)
	.word	0xdeadbeef
VECTOR_WORD(kernel_access)
	.word	0x55555555
VECTOR_WORD(time_ptr)
	.word	_ZN11scheduler_t12current_timeE

.ltorg

.balign 4096

