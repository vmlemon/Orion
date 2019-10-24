/* Automatically generated, don't edit */
/* Generated on: fedora28.novalocal */
/* At: Thu, 24 Oct 2019 00:42:43 +0000 */
/* Linux version 4.16.3-301.fc28.ppc64 (mockbuild@buildvm-ppc64-06.ppc.fedoraproject.org) (gcc version 8.0.1 20180324 (Red Hat 8.0.1-0.20) (GCC)) #1 SMP Mon Apr 23 21:44:46 UTC 2018 */

/* Pistachio Kernel Configuration System */

/* Hardware */

/* Basic Architecture */
#undef  CONFIG_ARCH_IA32
#undef  CONFIG_ARCH_IA64
#undef  CONFIG_ARCH_POWERPC
#define CONFIG_ARCH_POWERPC64 1
#undef  CONFIG_ARCH_AMD64
#undef  CONFIG_ARCH_ALPHA
#undef  CONFIG_ARCH_MIPS64
#undef  CONFIG_ARCH_ARM
#undef  CONFIG_ARCH_SPARC64


/* Platform */
#undef  CONFIG_PLAT_PC99


/* Platform */
#undef  CONFIG_PLAT_OFG5
#undef  CONFIG_PLAT_OFPOWER3
#define CONFIG_PLAT_OFPOWER4 1


/* Processor Type */
#undef  CONFIG_CPU_POWERPC64_POWER3
#undef  CONFIG_CPU_POWERPC64_POWER3p
#undef  CONFIG_CPU_POWERPC64_POWER4
#define CONFIG_CPU_POWERPC64_POWER4p 1
#undef  CONFIG_CPU_POWERPC64_PPC970


/* Miscellaneous */
#define CONFIG_BOOTMEM_PAGES 1024
#undef  CONFIG_DISABLE_ALIGNMENT_EXCEPTIONS



/* Kernel */
#undef  CONFIG_IPC_FASTPATH
#define CONFIG_DEBUG 1
#undef  CONFIG_PPC64_TRASH_OF


/* Debugger */
#undef  CONFIG_KDB

/* Consoles */
#undef  CONFIG_KDB_CONS_RTAS

#undef  CONFIG_KDB_DISAS
#undef  CONFIG_KDB_ON_STARTUP
#undef  CONFIG_KDB_BREAKIN
#define CONFIG_KDB_NO_ASSERTS 1
#undef  CONFIG_DEBUG_SYMBOLS

/* Trace Settings */
#undef  CONFIG_VERBOSE_INIT
#undef  CONFIG_TRACEPOINTS
#undef  CONFIG_KMEM_TRACE
#undef  CONFIG_TRACEBUFFER



/* Code Generator Options */


/* Derived symbols */
#undef  CONFIG_IA32_FXSR
#undef  CONFIG_IA32_PGE
#define CONFIG_IS_64BIT 1
#undef  CONFIG_PLAT_OFSPARC64
#undef  CONFIG_IA32_HTT
#define CONFIG_BIGENDIAN 1
#undef  CONFIG_SPARC64_SAB82532
#undef  CONFIG_IS_32BIT
#undef  CONFIG_CPU_SPARC64_ULTRASPARC
#undef  CONFIG_ARM_BIG_ENDIAN
#undef  CONFIG_SWIZZLE_IO_ADDR
#undef  CONFIG_IA32_SMALL_SPACES_GLOBAL
#define CONFIG_HAVE_MEMORY_CONTROL 1
#undef  CONFIG_IA32_PSE
#undef  CONFIG_ARM_V5
#undef  CONFIG_SPARC64_ULTRASPARC2I
#undef  CONFIG_ARM_THUMB_SUPPORT
#undef  CONFIG_IA32_TSC
#undef  CONFIG_SPARC64_ULTRASPARC1
#undef  CONFIG_ACPI
#undef  CONFIG_SPARC64_Z8530
#undef  CONFIG_ALPHA_FASTPATH
#undef  CONFIG_SPARC64_ULTRASPARC2
#undef  CONFIG_IA32_SYSENTER
/* That's all, folks! */
#define AUTOCONF_INCLUDED
