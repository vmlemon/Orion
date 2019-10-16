//https://raw.githubusercontent.com/freebsd/freebsd/1d6e4247415d264485ee94b59fdbc12e0c566fd0/sys/x86/include/_limits.h

//#define	SSIZE_MAX	__SSIZE_MAX	/* max value for an ssize_t */
//#define	__SHRT_MAX	0x7fff		/* max value for a short */
//#define	__SHRT_MIN	(-0x7fff - 1)	/* min value for a short */
//
#ifdef	__LP64__
#define	__LONG_BIT	64
#define	__ULONG_MAX	0xffffffffffffffff	/* max for an unsigned long */
#define	__LONG_MAX	0x7fffffffffffffff	/* max for a long */
#define	__LONG_MIN	(-0x7fffffffffffffff - 1) /* min for a long */
#else
#define	__LONG_BIT	32
#define	__ULONG_MAX	0xffffffffUL
#define	__LONG_MAX	0x7fffffffL
#define	__LONG_MIN	(-0x7fffffffL - 1)
#endif

#define	__UINT_MAX	0xffffffff	/* max value for an unsigned int */
#define	__INT_MAX	0x7fffffff	/* max value for an int */
