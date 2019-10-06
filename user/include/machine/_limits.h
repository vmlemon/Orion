//https://raw.githubusercontent.com/freebsd/freebsd/1d6e4247415d264485ee94b59fdbc12e0c566fd0/sys/x86/include/_limits.h

//#define	SSIZE_MAX	__SSIZE_MAX	/* max value for an ssize_t */
//#define	__SHRT_MAX	0x7fff		/* max value for a short */
//#define	__SHRT_MIN	(-0x7fff - 1)	/* min value for a short */
//
#ifdef	__LP64__
#define	__LONG_BIT	64
#else
#define	__LONG_BIT	32
#endif
