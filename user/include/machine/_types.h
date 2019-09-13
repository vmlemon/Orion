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

//
//	In order to be compatible with FreeBSD headers, we need to lace through all of the following:
//  		../../include/sys/_types.h:40:9: error: unknown type name ‘__int32_t’
typedef L4_SignedWord32_t __int32_t;
//		../../include/sys/_types.h:41:9: error: unknown type name ‘__int64_t’
typedef L4_SignedWord64_t __int64_t;
//		../../include/sys/_types.h:43:9: error: unknown type name ‘__uint32_t’
typedef L4_Word32_t __uint32_t;
//		../../include/sys/_types.h:44:9: error: unknown type name ‘__uint64_t’
typedef L4_Word64_t __uint64_t;
//		../../include/sys/_types.h:45:9: error: unknown type name ‘__uint64_t’
//		../../include/sys/_types.h:46:9: error: unknown type name ‘__uint32_t’
//		../../include/sys/_types.h:47:9: error: unknown type name ‘__int64_t’
//		../../include/sys/_types.h:48:9: error: unknown type name ‘__uint64_t’
//		../../include/sys/_types.h:50:9: error: unknown type name ‘__int32_t’
//		../../include/sys/_types.h:51:9: error: unknown type name ‘__uint16_t’
typedef L4_Word16_t __uint16_t;
//		../../include/sys/_types.h:54:9: error: unknown type name ‘__uint64_t’
//		../../include/sys/_types.h:55:9: error: unknown type name ‘__int64_t’
//		../../include/sys/_types.h:56:9: error: unknown type name ‘__int64_t’
//		../../include/sys/_types.h:57:9: error: unknown type name ‘__int32_t’
//		../../include/sys/_types.h:58:9: error: unknown type name ‘__int64_t’
//		../../include/sys/_types.h:61:9: error: unknown type name ‘__uint8_t’
typedef L4_Word8_t __uint8_t;
//		../../include/sys/_types.h:62:9: error: unknown type name ‘__uint32_t’
//		../../include/sys/_types.h:66:9: error: unknown type name ‘__uint32_t’
//		../../include/sys/_types.h:71:9: error: unknown type name ‘__int64_t’
//		../../include/sys/_types.h:97:9: error: unknown type name ‘__uint_least16_t’
//These are seemingly-specific to FreeBSD
typedef __uint16_t      __uint_least16_t;
typedef __uint32_t      __uint_least32_t;
//		../../include/sys/_types.h:98:9: error: unknown type name ‘__uint_least32_t’
//		../../include/sys/_types.h:113:9: error: unknown type name ‘__uint64_t’
//		../../include/sys/_types.h:115:9: error: unknown type name ‘__uint32_t’
//		../../include/sys/_types.h:123:2: error: unknown type name ‘__int64_t’
//		../../include/sys/_types.h:126:9: error: unknown type name ‘__uintmax_t’
typedef __uint64_t      __uintmax_t;
//


#endif
