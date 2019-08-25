/* Automatically generated, don't edit */
/* Generated on: PPC64U */
/* At: Sun, 25 Aug 2019 10:26:21 +0000 */
/* Linux version 5.2.0-8-generic (buildd@bos02-ppc64el-015) (gcc version 9.1.0 (Ubuntu 9.1.0-6ubuntu2)) #9-Ubuntu SMP Mon Jul 8 13:05:34 UTC 2019 */

/* Pistachio Kernel Configuration System */

/* Hardware */

/* Basic Architecture */
#undef  CONFIG_ARCH_X86
#undef  CONFIG_ARCH_POWERPC
#define CONFIG_ARCH_POWERPC64 1


/* X86 Processor Architecture */
#undef  CONFIG_SUBARCH_X32
#undef  CONFIG_SUBARCH_X64


/* Processor Type */
#undef  CONFIG_CPU_X86_I486
#undef  CONFIG_CPU_X86_I586
#undef  CONFIG_CPU_X86_I686
#undef  CONFIG_CPU_X86_P4
#undef  CONFIG_CPU_X86_K8
#undef  CONFIG_CPU_X86_C3
#undef  CONFIG_CPU_X86_SIMICS


/* Platform */
#undef  CONFIG_PLAT_PC99


/* Platform */
#define CONFIG_PLAT_OFG5 1
#undef  CONFIG_PLAT_OFPOWER3
#undef  CONFIG_PLAT_OFPOWER4


/* Processor Type */
#undef  CONFIG_CPU_POWERPC64_POWER3
#undef  CONFIG_CPU_POWERPC64_POWER3p
#undef  CONFIG_CPU_POWERPC64_POWER4
#undef  CONFIG_CPU_POWERPC64_POWER4p
#define CONFIG_CPU_POWERPC64_PPC970 1


/* Miscellaneous */
#define CONFIG_BOOTMEM_PAGES 1024



/* Kernel */
#define CONFIG_EXPERIMENTAL 1

/* Experimental Features */
#define CONFIG_X_PAGER_EXREGS 1

/* Kernel scheduling policy */
#undef  CONFIG_SCHED_RR
#define CONFIG_X_SCHED_HS 1


#define CONFIG_IPC_FASTPATH 1
#define CONFIG_DEBUG 1
#define CONFIG_DEBUG_SYMBOLS 1
#undef  CONFIG_STATIC_TCBS
#undef  CONFIG_PPC64_TRASH_OF


/* Debugger */

/* Kernel Debugger Console */
#define CONFIG_KDB_CONS_RTAS 1
#define CONFIG_KDB_BOOT_CONS 0

#define CONFIG_KDB_DISAS 1
#undef  CONFIG_KDB_ON_STARTUP
#undef  CONFIG_KDB_BREAKIN
#undef  CONFIG_KDB_BREAKIN_BREAK
#undef  CONFIG_KDB_NO_ASSERTS

/* Trace Settings */
#define CONFIG_VERBOSE_INIT 1
#define CONFIG_TRACEPOINTS 1
#define CONFIG_KMEM_TRACE 1
#define CONFIG_TRACEBUFFER 1



/* Code Generator Options */


/* Derived symbols */
#define CONFIG_HAVE_MEMORY_CONTROL 1
#undef  CONFIG_X86_PSE
#define CONFIG_BIGENDIAN 1
#undef  CONFIG_PPC_MMU_TLB
#undef  CONFIG_X86_SYSENTER
#undef  CONFIG_X86_PGE
#undef  CONFIG_X86_FXSR
#undef  CONFIG_IS_32BIT
#undef  CONFIG_X86_HTT
#undef  CONFIG_X86_PAT
#undef  CONFIG_PPC_BOOKE
#define CONFIG_IS_64BIT 1
#undef  CONFIG_MULTI_ARCHITECTURE
#undef  CONFIG_X86_EM64T
#undef  CONFIG_PPC_CACHE_L1_WRITETHROUGH
#undef  CONFIG_PPC_TLB_INV_LOCAL
#undef  CONFIG_PPC_CACHE_ICBI_LOCAL
#undef  CONFIG_X86_SMALL_SPACES_GLOBAL
#undef  CONFIG_X86_HVM
#undef  CONFIG_PPC_MMU_SEGMENTS
#undef  CONFIG_X86_TSC
/* That's all, folks! */
#define AUTOCONF_INCLUDED
