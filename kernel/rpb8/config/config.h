/* Automatically generated, don't edit */
/* Generated on: fedora28.novalocal */
/* At: Sun, 08 Sep 2019 06:00:47 +0000 */
/* Linux version 4.16.3-301.fc28.ppc64 (mockbuild@buildvm-ppc64-06.ppc.fedoraproject.org) (gcc version 8.0.1 20180324 (Red Hat 8.0.1-0.20) (GCC)) #1 SMP Mon Apr 23 21:44:46 UTC 2018 */

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


/* Processor Type */
#undef  CONFIG_CPU_POWERPC_PPC440
#undef  CONFIG_CPU_POWERPC_IBM750
#undef  CONFIG_CPU_POWERPC_PPC604


/* Platform */
#undef  CONFIG_PLAT_OFPPC
#undef  CONFIG_PLAT_PPC44X


/* Sub-Platform */
#undef  CONFIG_SUBPLAT_440_BGP
#undef  CONFIG_SUBPLAT_440_EBONY


/* Platform */
#undef  CONFIG_PLAT_OFG5
#undef  CONFIG_PLAT_OFPOWER3
#define CONFIG_PLAT_OFPOWER4 1


/* Processor Type */
#undef  CONFIG_CPU_POWERPC64_POWER3
#undef  CONFIG_CPU_POWERPC64_POWER3p
#define CONFIG_CPU_POWERPC64_POWER4 1
#undef  CONFIG_CPU_POWERPC64_POWER4p
#undef  CONFIG_CPU_POWERPC64_PPC970


/* Miscellaneous */
#define CONFIG_BOOTMEM_PAGES 1024

#undef  CONFIG_SMP


/* Kernel */
#define CONFIG_EXPERIMENTAL 1

/* Experimental Features */
#define CONFIG_X_PAGER_EXREGS 1

/* Kernel scheduling policy */
#define CONFIG_SCHED_RR 1
#undef  CONFIG_X_SCHED_HS


#undef  CONFIG_IPC_FASTPATH
#define CONFIG_DEBUG 1
#define CONFIG_DEBUG_SYMBOLS 1
#undef  CONFIG_STATIC_TCBS
#undef  CONFIG_PPC64_TRASH_OF


/* Debugger */

/* Kernel Debugger Console */
#undef  CONFIG_KDB_CONS_BGP_JTAG
#undef  CONFIG_KDB_CONS_BGP_TREE
#define CONFIG_KDB_CONS_COM 1
#define CONFIG_KDB_COMPORT 0x0
#define CONFIG_KDB_COMSPEED 115200
#undef  CONFIG_KDB_CONS_RTAS
#define CONFIG_KDB_BOOT_CONS 0

#undef  CONFIG_KDB_DISAS
#undef  CONFIG_KDB_ON_STARTUP
#undef  CONFIG_KDB_BREAKIN
#undef  CONFIG_KDB_BREAKIN_BREAK
#define CONFIG_KDB_BREAKIN_ESCAPE 1
#undef  CONFIG_KDB_NO_ASSERTS

/* Trace Settings */
#undef  CONFIG_VERBOSE_INIT
#define CONFIG_TRACEPOINTS 1
#undef  CONFIG_KMEM_TRACE
#undef  CONFIG_TRACEBUFFER



/* Code Generator Options */
#define CONFIG_SYSV_ABI 1
#define CONFIG_PPC_MULTIWORD_INSTR 1


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
