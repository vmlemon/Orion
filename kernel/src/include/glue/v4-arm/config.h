/*********************************************************************
 *                
 * Copyright (C) 2003-2005,  National ICT Australia (NICTA)
 *                
 * File path:     glue/v4-arm/config.h
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
 * $Id: config.h,v 1.23 2004/12/01 23:57:01 cvansch Exp $
 *                
 ********************************************************************/

#ifndef __GLUE__V4_ARM__CONFIG_H__
#define __GLUE__V4_ARM__CONFIG_H__

#include INC_API(config.h)
#include INC_ARCH(page.h)

#if defined(ASSEMBLY)
#define UL(x)	x
#else
#define UL(x)	x##ul
#endif

/**
 * Size of a kernel TCB in bytes
 */
#define KTCB_PGSIZE	pgent_t::size_4k
#define KTCB_PAGESIZE	PAGE_SIZE_4K
#define KTCB_PAGEBITS	PAGE_BITS_4K
#define KTCB_BITS	11
#define KTCB_SIZE	(UL(1) << KTCB_BITS)
#define KTCB_MASK	(~(KTCB_SIZE - 1))

#define UTCB_BITS	8
#define UTCB_SIZE	(1ul << UTCB_BITS)
#define UTCB_MASK	(~(UTCB_SIZE - 1))

#define UTCB_AREA_PAGESIZE	PAGE_SIZE_4K
#define UTCB_AREA_PAGEBITS	PAGE_BITS_4K
#define UTCB_AREA_PGSIZE pgent_t::size_4k

/**
 * number of message registers
 */
#define IPC_NUM_MR			32

/**
   attributes for system call functions
   @param x is the name of the system call lacking the leading sys_ .
   This makes it possible to place every system call in its own section
   if required. Default is empty.
 */
#define KIP_SYSCALL(x)  ((word_t) (x) - (word_t) &kip)

/**
 * endianess and word size
 */

#ifdef CONFIG_BIGENDIAN
  #define KIP_API_FLAGS	{SHUFFLE2(endian:1, word_size:0)} /* 32-bit, big endian */
#else
  #define KIP_API_FLAGS	{ endian:0, word_size:0 } /* 32-bit, little endian */
#endif

/**
 * minimum size of UTCB area and number of UTCBs in this
 */
#define KIP_UTCB_INFO   { SHUFFLE3(multiplier: 1, alignment: UTCB_BITS, 0) }

/**
 * size of kernel interface page
 */
#define KIP_KIP_BITS    PAGE_BITS_4K
#define KIP_KIP_SIZE    (1 << KIP_KIP_BITS)
#define KIP_KIP_PGSIZE  pgent_t::size_4k
#define KIP_KIP_AREA	{ 0 }

/**
 * supported page sizes and access bits
 */
#define KIP_ARCH_PAGEINFO { SHUFFLE2(rwx:2, size_mask:HW_VALID_PGSIZES >> 10) }

/**
 * Special ARM thumb syscalls
 */
#define SET_KIP_THUMB_SYSCALL(x) \
    this->x##_thumb_syscall = KIP_SYSCALL(user_thumb_##x)|1	/* 1 - set thumb mode */

#define	ARCH_SPECIAL_SYSCALLS			\
    SET_KIP_THUMB_SYSCALL(space_control);	\
    SET_KIP_THUMB_SYSCALL(thread_control);	\
    SET_KIP_THUMB_SYSCALL(processor_control);	\
    SET_KIP_THUMB_SYSCALL(memory_control);	\
    SET_KIP_THUMB_SYSCALL(ipc);			\
    SET_KIP_THUMB_SYSCALL(lipc);		\
    SET_KIP_THUMB_SYSCALL(unmap);		\
    SET_KIP_THUMB_SYSCALL(exchange_registers);	\
    SET_KIP_THUMB_SYSCALL(thread_switch);	\
    SET_KIP_THUMB_SYSCALL(schedule);

#define	ARCH_DUMP_SPECIAL_SYSCALLS			\
    printf ("\nThumb system call offsets:\n");		\
    printf ("  %16s 0x%08x    %17s 0x%08x\n",		\
	    "SpaceControl", kip->space_control_thumb_syscall,	\
	    "ThreadControl", kip->thread_control_thumb_syscall);	\
    printf ("  %16s 0x%08x    %17s 0x%08x\n",		\
	    "ProcessorControl", kip->processor_control_thumb_syscall,	\
	    "MemoryControl", kip->memory_control_thumb_syscall);	\
    printf ("  %16s 0x%08x    %17s 0x%08x\n",		\
	    "Ipc", kip->ipc_thumb_syscall,		\
	    "Lipc", kip->lipc_thumb_syscall);		\
    printf ("  %16s 0x%08x    %17s 0x%08x\n",		\
	    "Unmap", kip->unmap_thumb_syscall,		\
	    "ExchangeRegisters", kip->exchange_registers_thumb_syscall);\
    printf ("  %16s 0x%08x    %17s 0x%08x\n",		\
	    "Schedule", kip->schedule_thumb_syscall,	\
	    "ThreadSwitch", kip->thread_switch_thumb_syscall);

#if !defined(ASSEMBLY) && defined(__cplusplus)
extern "C" void SECTION(".user.syscall.ipc") user_thumb_ipc();
extern "C" void SECTION(".user.syscall.lipc") user_thumb_lipc();
extern "C" void SECTION(".user.syscall.exregs") user_thumb_exchange_registers();
extern "C" void SECTION(".user.syscall.threadctrl") user_thumb_thread_control();
extern "C" void SECTION(".user.syscall.threadswtch") user_thumb_thread_switch();
extern "C" void SECTION(".user.syscall.schedule") user_thumb_schedule();
extern "C" void SECTION(".user.syscall.unmap") user_thumb_unmap();
extern "C" void SECTION(".user.syscall.spacectrl") user_thumb_space_control();
extern "C" void SECTION(".user.syscall.procctrl") user_thumb_processor_control();
extern "C" void SECTION(".user.syscall.memctrl") user_thumb_memory_control();

#define ARCH_SPECIAL_SYSCALLS_PTRS		\
    syscall_t	space_control_thumb_syscall;	\
    syscall_t	thread_control_thumb_syscall;	\
    syscall_t	processor_control_thumb_syscall;\
    syscall_t	memory_control_thumb_syscall;	\
    syscall_t	ipc_thumb_syscall;		\
    syscall_t	lipc_thumb_syscall;		\
    syscall_t	unmap_thumb_syscall;		\
    syscall_t	exchange_registers_thumb_syscall;\
    syscall_t	unused_thumb_syscall;		\
    syscall_t	thread_switch_thumb_syscall;	\
    syscall_t	schedule_thumb_syscall;

#endif

/* tracebuffer size */
#define TBUFF_SIZE (CONFIG_TRACEBUF_PAGES * PAGE_SIZE_4K)

/* memory layout
 *
 * The first (14/16ths = 3.54GB) are user address space.  Immediately after this, is the
 * beginning of kernel space (2/16ths = 512MB).
 * There is 256 MB of KTCBs, giving 16 valid bits for thread IDs.
 *
 * Next is 128 MB of space for kernel code and structures (such as the CPD,
 * root page table and bootstack).
 *
 * After this is 64 MB of space for the copy area.
 *
 * Following this is 16 MB of space for storing variables for space_t such as
 * kip_area, utcb_area, thread_count and domain. It is important that the
 * 2 LSBs _MUST_ be 00, when using these for data or security may be compromised.
 * 
 * Following is 47 MB for IO space mappings, then
 * 
 * Finally a 1 MB section is reserved for mapping in the exception vectors.
 */


/* 0x00000000 */
#define USER_AREA_SECTIONS	((ARM_NUM_SECTIONS * 13) / 16)
#define USER_AREA_START		0ul
#define USER_AREA_SIZE		(USER_AREA_SECTIONS * ARM_SECTION_SIZE)
#define USER_AREA_END		(USER_AREA_START + USER_AREA_SIZE)

/* The UTCB area contains n (currently 256) 1MB sections, where each section
 * can be assigned to one address space. This limits the number of address
 * spaces to 256.
 * Once a section is assigned to an address space, the kernel allocates
 * UTCBs for that address space in that section.
 * Note: An entire section is used per address space so that the kernel
 * can use domain ids to enforce permissions.
 */
/* 0xd0000000 */
#define UTCB_AREA_SECTIONS	256
#define UTCB_AREA_START		USER_AREA_END
#define UTCB_AREA_SIZE		(UTCB_AREA_SECTIONS * ARM_SECTION_SIZE)
#define UTCB_AREA_END		(UTCB_AREA_START + UTCB_AREA_SIZE)

#define KTCB_AREA_BITS		28
#define VALID_THREADNO_BITS	(KTCB_AREA_BITS - KTCB_BITS)
#define VALID_THREADNO_MASK	((1ul << VALID_THREADNO_BITS)-1)
/* 0xe0000000 */
#define KTCB_AREA_SECTIONS	(KTCB_AREA_SIZE / ARM_SECTION_SIZE)
#define KTCB_AREA_START		UTCB_AREA_END
#define KTCB_AREA_SIZE		(1ul << KTCB_AREA_BITS)
#define KTCB_AREA_END		(KTCB_AREA_START + KTCB_AREA_SIZE)

/* 0xf0000000 */
#define KERNEL_AREA_SECTIONS	64
#define KERNEL_AREA_START	KTCB_AREA_END
#define KERNEL_AREA_SIZE	(KERNEL_AREA_SECTIONS * ARM_SECTION_SIZE)
#define KERNEL_AREA_END		(KERNEL_AREA_START + KERNEL_AREA_SIZE)

/* 0xf4000000 */
#define UNCACHE_AREA_SECTIONS	64
#define UNCACHE_AREA_START	KERNEL_AREA_END
#define UNCACHE_AREA_SIZE	(UNCACHE_AREA_SECTIONS * ARM_SECTION_SIZE)
#define UNCACHE_AREA_END	(UNCACHE_AREA_START + UNCACHE_AREA_SIZE)

/* 0xf8000000 */
#define VAR_AREA_SECTIONS	16
#define VAR_AREA_START		UNCACHE_AREA_END
#define VAR_AREA_SIZE		(VAR_AREA_SECTIONS * ARM_SECTION_SIZE)
#define VAR_AREA_END		(VAR_AREA_START + VAR_AREA_SIZE)

/* 0xf9000000 */
#define IO_AREA_SECTIONS	96
#define IO_AREA_START		VAR_AREA_END
#define IO_AREA_SIZE		(IO_AREA_SECTIONS * ARM_SECTION_SIZE)
#define IO_AREA_END		(IO_AREA_START + IO_AREA_SIZE)

/* Note: First page in MISC area is used for KIP/UTCB reference page
 *    We reserve the next (14) entries for the UTCB used bit
 *    array. This limits us to 420 threads per space ( 14 * 30 used bits )
 *    { we can't use the bottom two bits in the pt entries, they must be 0 }
 */
/* 0xff000000 */
#define MISC_AREA_SECTIONS	15
#define MISC_AREA_START		IO_AREA_END
#define MISC_AREA_SIZE		(MISC_AREA_SECTIONS * ARM_SECTION_SIZE)
#define MISC_AREA_END		(MISC_AREA_START + MISC_AREA_SIZE)

/* User KIP page / UTCB reference at 0xff000000 */

#define USER_KIP_PAGE		MISC_AREA_START

/* 0xfff00000 */
#define EXCPT_AREA_START	MISC_AREA_END
#define EXCPT_AREA_SIZE		ARM_SECTION_SIZE
#define EXCPT_AREA_END		(EXCPT_AREA_START + EXCPT_AREA_SIZE)

/* 1MB IO Areas in the Virtual Address space. Define more if needed */
#define IO_AREA0_VADDR		(IO_AREA_START + (ARM_SECTION_SIZE*0))
#define IO_AREA1_VADDR		(IO_AREA_START + (ARM_SECTION_SIZE*1))
#define IO_AREA2_VADDR		(IO_AREA_START + (ARM_SECTION_SIZE*2))
#define IO_AREA3_VADDR		(IO_AREA_START + (ARM_SECTION_SIZE*3))
#define IO_AREA4_VADDR		(IO_AREA_START + (ARM_SECTION_SIZE*4))
#define IO_AREA5_VADDR		(IO_AREA_START + (ARM_SECTION_SIZE*5))
#define IO_AREA6_VADDR		(IO_AREA_START + (ARM_SECTION_SIZE*6))
#define IO_AREA7_VADDR		(IO_AREA_START + (ARM_SECTION_SIZE*7))
#define PHYSMAPPING_VADDR	(IO_AREA_START + (ARM_SECTION_SIZE*10))

/* Note on io area usage:
 *
 * SA1100 uses:
 *  #define CONSOLE_VADDR		IO_AREA0_VADDR
 *  #define ZERO_BANK_VADDR		IO_AREA1_VADDR
 *  #define SA1100_OS_TIMER_BASE	IO_AREA2_VADDR
 *
 * XSCALE uses:
 *  #define IODEVICE_VADDR		IO_AREA0_VADDR
 *
 * OMAP1510 uses:
 *  #define IODEVICE_VADDR		IO_AREA0_VADDR
 *
 * CSB337 uses:
 *  #define SYS_VADDR			IO_AREA0_VADDR
 *
 * LN2410SBC users:
 *  #define CONSOLE_VADDR               IO_AREA0_VADDR
 *  #define TIMER_VADDR               IO_AREA1_VADDR
*/

/**
 * Base address of the root task's UTCB area
 */
#define ROOT_UTCB_START		(USER_AREA_END - ARM_SECTION_SIZE)

/**
 * Base address of the root task's UTCB area
 */
#define ROOT_KIP_START		(USER_KIP_PAGE)

/**
 * Address of start of arm_high_vector - exception handling code
 */
#define ARM_HIGH_VECTOR_VADDR	(EXCPT_AREA_START | 0x000f0000 )

/**
 * Address of the syscall area (for prefect aborts) is in
 * the last page of virtual memory. Do not map anything
 * with user access here.
 */
#define ARM_SYSCALL_VECTOR	(0xffffff00)

#include INC_PLAT(timer.h)

#endif /* __GLUE__V4_ARM__CONFIG_H__ */
