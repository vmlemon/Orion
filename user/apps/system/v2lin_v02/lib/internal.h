/****************************************************************************
 * Copyright (C) 2004, 2005, 2006 v2lin Team <http://v2lin.sf.net>
 * Copyright (C) 2000  Monta Vista Software Inc.
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


#ifndef v2l_internal_h
#define v2l_internal_h

#include "v2lpthread.h"
#include "vxw_defs.h"
#include "vxw_hdrs.h"
#include "v2ldebug.h"


typedef struct v2pt_wdog
{
	pthread_mutex_t wdog_lock;
	int ticks_remaining;		// Ticks remaining until timeout (zero if watchdog already expired).
	void (*timeout_func) (int);
	int timeout_parm;
	struct v2pt_wdog *next;
} v2pt_wdog_t;

typedef struct v2lsem
{
	// Mutex and Condition variable for semaphore post/pend
	pthread_mutex_t sem_lock;
	pthread_cond_t sem_send;
	int flags;

	//Mutex and Condition variable for semaphore delete
	pthread_mutex_t smdel_lock;
	pthread_cond_t smdel_cplt;

	//  Count of available 'tokens' for semaphore.
	int token_count;

	// Type of send operation last performed on semaphore
	int send_type;

	// Ownership nesting level for mutual exclusion semaphore.
	int recursion_level;

	// Task control block ptr for task which currently owns semaphore
	task_t *current_owner;

	struct v2lsem *nxt_sem;

	// First task control block in list of tasks waiting on semaphore
	task_t *first_susp;
} v2lsem_t;

/*
**  task_list is a linked list of pthread task control blocks.
**            It is used to perform en-masse operations on all v2pthread
**            tasks at once.
*/
extern task_t *task_list;

/*
**  task_list_lock is a mutex used to serialize access to the task list
*/
extern pthread_mutex_t task_list_lock;

/*
**  round_robin_enabled is a system-wide mode flag indicating whether the
**                      v2pthread scheduler is to use FIFO or Round Robin
**                      scheduling.
*/

#define pthread_mutex_clean_lock(m) \
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, m); \
	pthread_mutex_lock(m);

extern void *ts_malloc(size_t blksize);
extern void ts_free(void *blkaddr);
extern task_t *my_task(void);
extern void link_susp_task(task_t ** list_head, task_t * new_entry);
extern void unlink_susp_task(task_t ** list_head, task_t * entry);
extern int signal_for_my_task(task_t ** list_head, int pend_order);
void status_show();
void taskShow(FILE * out, task_t * t);
task_t *task_for(int taskid);
#endif
