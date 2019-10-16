#ifndef _MACHINE_STDINT
#define _MACHINE_STDINT

//QNX defines this to be wide-255
#ifdef __LP64__
#define	INT64_MIN	(-0x7fffffffffffffff-1)
#define	INT64_MAX	0x7fffffffffffffff
#define UINT64_MAX 0xffffffffffffffff
#else
#define	INT64_MIN	(-0x7fffffffffffffffLL-1)
#define	INT64_MAX	0x7fffffffffffffffLL
#define	UINT64_MAX	0xffffffffffffffffULL
#endif

#define SIZE_MAX     UINT32_MAX
#define UINT32_MAX 0xffffffff

#define	INTMAX_MIN	INT64_MIN
#define	INTMAX_MAX	INT64_MAX
#define	UINTMAX_MAX	UINT64_MAX

#endif


