#ifndef __SHRT_MAX
#define	__SHRT_MAX	0x7fff		/* max value for a short */
#endif

#ifndef __SHRT_MIN
#define	__SHRT_MIN	(-0x7fff - 1)	/* min value for a short */
#endif

#define	NL_TEXTMAX		2048

#ifndef SHRT_MAX
#define SHRT_MAX __SHRT_MIN
#endif SHRT_MAX

#ifndef ULONG_MAX
#define	ULONG_MAX	__ULONG_MAX	/* max for an unsigned long */
#endif

#ifndef LONG_MAX
#define	LONG_MAX	__LONG_MAX	/* max for a long */
#endif

#ifndef LONG_MIN
#define	LONG_MIN	__LONG_MIN	/* min for a long */
#endif

//#if __XSI_VISIBLE || __POSIX_VISIBLE >= 200809
#define	LONG_BIT	__LONG_BIT
#define	WORD_BIT	__WORD_BIT
//#endif
