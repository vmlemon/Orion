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

//#if __XSI_VISIBLE || __POSIX_VISIBLE >= 200809
#define	LONG_BIT	__LONG_BIT
#define	WORD_BIT	__WORD_BIT
//#endif
