#ifndef _MACHINE_TYPES
#define _MACHINE_TYPES

//Adaptive Endian Technology, for POSIX compatibility
//Part of JUEL (liblinux)

/* On x86-64 */
#ifdef __amd64__
#include <l4/amd64/types.h>
#endif

/* On PowerPC 64-bit (standard configuration) */
#ifdef __powerpc64__
#include <l4/powerpc64/types.h>
#endif

#ifdef __powerpc__
/* On PowerPC 32-bit (standard configuration) */
#include <l4/powerpc/types.h>
#endif

//In order to be compatible with FreeBSD headers, we need to lace through all of the following:
typedef L4_SignedWord32_t __int32_t;
typedef L4_SignedWord64_t __int64_t;
typedef L4_SignedWord16_t __int16_t;
typedef	L4_SignedWord8_t __int8_t;
typedef L4_Word32_t __uint32_t;
typedef L4_Word64_t __uint64_t;
typedef L4_Word16_t __uint16_t;
typedef L4_Word8_t __uint8_t;

//Should this be here?
typedef __int64_t int64_t;
typedef __uint64_t    uint64_t;


//These are seemingly-specific to FreeBSD
typedef __uint16_t      __uint_least16_t;
typedef __uint32_t      __uint_least32_t;
typedef __uint64_t      __uintmax_t;

/* Dynamic size_t() - on FreeBSD, this is __uint64_t (LP64), or __uint32_t */
typedef	L4_Size_t	__size_t; 

/* This is the way that FreeBSD does it... */
#ifdef	__LP64__
typedef	__int64_t	__ssize_t;	
#else
typedef	__int32_t	__ssize_t;
#endif

//__clock_t can be either unsigned long, or __int32_t
#ifdef	__LP64__
typedef	__int32_t	__clock_t;		/* clock()... */
typedef	__int64_t	__time_t;		/* time()... */
typedef	__int64_t	__critical_t;
typedef	__int64_t	__register_t;
typedef	__int64_t	__segsz_t;		/* segment size (in pages) */
typedef	__uint64_t	__u_register_t;
typedef	__uint64_t	__vm_paddr_t;
typedef	__uint64_t	__vm_offset_t;
typedef	__uint64_t	__vm_size_t;
typedef	__int64_t	__intptr_t;
typedef	__uint64_t	__uintptr_t;
#else
typedef	unsigned long	__clock_t;
typedef	__int32_t	__time_t;
typedef	__int32_t	__critical_t;
typedef	__int32_t	__register_t;
typedef	__int32_t	__segsz_t;
typedef	__uint32_t	__u_register_t;
typedef	__uint64_t	__vm_paddr_t;
typedef	__uint32_t	__vm_offset_t;
typedef	__uint32_t	__vm_size_t;
typedef	__int32_t	__intptr_t;
typedef	__uint32_t	__uintptr_t;
#endif

typedef	__int64_t	__intmax_t;

//Fast/Least integer types, from x86/FreeBSD
typedef	__int32_t	__int_fast8_t;
typedef	__int32_t	__int_fast16_t;
typedef	__int32_t	__int_fast32_t;
typedef	__int64_t	__int_fast64_t;
typedef	__int8_t	__int_least8_t;
typedef	__int16_t	__int_least16_t;
typedef	__int32_t	__int_least32_t;
typedef	__int64_t	__int_least64_t;

//Fast/Least unsigned integers
typedef	__uint32_t	__uint_fast8_t;
typedef	__uint32_t	__uint_fast16_t;
typedef	__uint32_t	__uint_fast32_t;
typedef	__uint64_t	__uint_fast64_t;
typedef	__uint8_t	__uint_least8_t;
typedef	__uint16_t	__uint_least16_t;
typedef	__uint32_t	__uint_least32_t;
typedef	__uint64_t	__uint_least64_t;

#endif
