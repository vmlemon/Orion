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
typedef L4_Word32_t __uint32_t;
typedef L4_Word64_t __uint64_t;
typedef L4_Word16_t __uint16_t;
typedef L4_Word8_t __uint8_t;

//These are seemingly-specific to FreeBSD
typedef __uint16_t      __uint_least16_t;
typedef __uint32_t      __uint_least32_t;
typedef __uint64_t      __uintmax_t;



#endif
