/****************************************************************************
 * Copyright (C) 2004, 2005, 2006 v2lin Team <http://v2lin.sf.net>
 * Copyright (C) 2000,2001  Monta Vista Software Inc.
 * 
 * This file is part of the v2lin Library.
 * VxWorks is a registered trademark of Wind River Systems, Inc.
 * 
 * Initial implementation Gary S. Robertson, 2000, 2001.
 * Contributed by Andrew Skiba, skibochka@sourceforge.net, 2004.
 * Contributed by Mike Kemelmakher, mike@ubxess.com, 2005.
 * Contributed by Constantine Shulyupin, conan.sh@gmail.com, 2006.
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

#ifndef v2lpthread_h
#define v2lpthread_h
#include <pthread.h>
//#include <asm/atomic.h>

__BEGIN_DECLS;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  !FALSE
#endif
#define V2PT_TICK 10			/* milliseconds per v2pthread scheduler tick */
/*
**  Task Scheduling Priorities in v2pthread are higher as numbers decrease...
**  define the largest and smallest possible priority numbers.
*/
#define MIN_V2PT_PRIORITY 255
#define MAX_V2PT_PRIORITY 0
#ifndef OK
#define OK     0				/* Normal return value */
#endif
#ifndef ERROR
#define ERROR  -1				/* Error return value */
#endif
// Task state bit masks
#define READY   0x0000
#define PEND    0x0001
#define DELAY   0x0002
#define SUSPEND 0x0004
#define TIMEOUT 0x0008
#define INH_PRI 0x0010
#define DEAD    0x0080
#define RDY_MSK 0x008f
//
// Control block for pthread wrapper for v2pthread task
typedef struct task_s
{
	int check;
	int taskid;
	char *taskname;
	pthread_t pthrid;
	pthread_attr_t attr;
	struct sched_param prv_priority;
	int (*entry_point) (int, int, int, int, int, int, int, int, int, int);
	int parms[10];
	int static_task;	// Flag indicating if task control block allocated dynamically ( == 0 )
	int flags;
	int state;
	int vxw_priority;
	// struct task_s *suspend_list; use waiting instead
	struct task_s *nxt_susp;	// Next task control block in list of tasks waiting on object
	int delete_safe_count;		// Nesting level for number of taskSafe calls
	// Mutex and Condition variable for task delete 'pend'
	pthread_mutex_t tdelete_lock;
	pthread_cond_t t_deletable;

	// Mutex and Condition variable for task delete 'broadcast'
	pthread_mutex_t dbcst_lock;
	pthread_cond_t delete_bcplt;

	// First task control block in list of tasks waiting on this task (for deletion purposes)
	struct task_s *first_susp;

	// Next task control block in list
	struct task_s *nxt_task;

	// the task waits this objects:
	struct v2lsem *waiting; 
	pthread_mutex_t *waiting_m; 
} task_t;
#define WIND_TCB task_t

__END_DECLS
#endif
