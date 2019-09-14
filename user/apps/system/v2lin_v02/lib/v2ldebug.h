/****************************************************************************
 * This file is part of the v2lin Library.
 * VxWorks is a registered trademark of Wind River Systems, Inc.
 *
 * Copyright (C) 2006 Constantine Shulyupin, conan.sh@gmail.com
 *
 * The v2lin library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * The v2lin Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 ****************************************************************************/

#ifndef __V2LIN_DEBUG__
#define __V2LIN_DEBUG__

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>

#include <sys/types.h>
#include <linux/unistd.h>
pid_t gettid(void);

#include "internal.h"

extern int trace;
extern int trace_cnt;
extern int tid;
extern char trace_fn[100];

extern struct timeval prev_tv;
extern struct timeval start_tv;

void trace_time();
#ifdef DEBUG

// TRACEV is for printing variable name and value
#define TRACEV(F,V) do { if (trace)  { \
	if ( tid != gettid() ) { \
		tid = gettid();\
		fprintf(stderr,"thread=%i\n",tid);}; \
	fprintf(stderr,#V"="F"\n",V);	\
} } while (0)

#define TRACEF(args ... ) do { if (trace)  { \
	if ( tid != gettid() ) { \
		tid = gettid();\
		fprintf(stderr,"\nthread=%i task=%x %s\n",(int)tid,(int)my_task(),my_task()?my_task()->taskname:NULL);}; \
	trace_time(); \
	fprintf(stderr,"%s:%i", __FILE__, __LINE__);	\
	if ( strcmp(trace_fn,__FUNCTION__) ) \
		strcpy(trace_fn,__FUNCTION__) && fprintf(stderr," %s()",trace_fn); \
	fprintf(stderr," "args);	\
	fprintf(stderr,"\n");	\
} } while (0)
#else
#define TRACEV(F,V)
#define TRACEF(args ... )
#endif

#define FNSTART int status = OK;
#define FNFINISH exit: return status;

#ifdef TRACE_IN_OUT
#define FN_IN(s) TRACEF("%s {",s)
#define FN_OUT(s) TRACEF("%s }",s)
#else
#define FN_IN(s)
#define FN_OUT(s)
#endif

// ON_ERR defines common error processing behaveur:
// a) ignore error and continue code exection
// b) goto to finalization sections of a functions

#define ON_ERR // ignore
//#define ON_ERR goto exit

// CHK and CHK0 - common macros for error processing
// and optional code tracing. This marcos prints error messages and
// optionally processes errors in case of subfunction failures.
#define TRACE_errno() fprintf(stderr,"\terrno=%i(%x) %s \n",errno, errno, VxWorksError(errno))

// CHK supposes !0 is success, suitable for checking pointers
// and functions in form: CHK(0<read(fd,buff,count));
#define CHK(command)    \
	do {	int ret;\
		FN_IN(#command); \
		if ( ! (ret=(int) (command)) ) {    \
			TRACEF("ERROR: %s \n\treturned 0",#command);   \
			TRACE_errno(); \
			ON_ERR ;\
		}   \
		FN_OUT(#command); \
	} while (0)

// CHK0 supposes 0 is success, sutable for checking most of libc functions
#define CHK0(command) 	\
	do { \
		int status;  \
		FN_IN(#command); \
		if ((status = (command)) != 0 ) {    \
			TRACEF("ERROR: %s \n\treturned status: #%i (%x):\"%s\"",\
				#command,status,status, VxWorksError(status));   \
			TRACE_errno(); \
			ON_ERR;\
		}   \
		FN_OUT(#command); \
	} while (0)

//#define TRACE_PTHREAD
#ifdef TRACE_PTHREAD

#define pthread_mutex_lock(m)	\
do { \
	if ( EBUSY == pthread_mutex_trylock(m)) { TRACEF("pthread_mutex_lock %s %x EBUSY",#m,m); \
	pthread_mutex_lock(m); } \
	TRACEF("pthread_mutex_lock %s %x OK",#m,m); \
} while ( 0 )

#define pthread_mutex_unlock(m) \
do { \
	TRACEF("pthread_mutex_unlock %s %x OK",#m,m); \
	pthread_mutex_unlock(m); \
} while ( 0 )

#define pthread_cond_timedwait(args ... ) \
	( fprintf(stderr,"%s:%i %s", __FILE__, __LINE__, __FUNCTION__), \
	fprintf(stderr,"pthread_cond_timedwait %x\n",#args), pthread_cond_timedwait(args))

#define thread_cleanup_pop(i) do { \
        TRACEF("thread_cleanup_pop %s OK",#i); \
        thread_cleanup_pop(m); \
} while ( 0 
#endif

#endif
