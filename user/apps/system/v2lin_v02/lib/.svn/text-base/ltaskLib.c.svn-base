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

#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include "v2lpthread.h"
#include "vxw_defs.h"
#include "vxw_hdrs.h"
#include "internal.h"
#include "v2ldebug.h"

/*
**  selfRestart is a system function used by a task to restart itself.
**              The function creates a temporary watchdog timer which restarts 
**              the terminated task and then deletes itself. 
*/
extern void selfRestart(task_t * restart_task);

extern BOOL roundRobinIsEnabled(void);

/*
**  task_list is a linked list of pthread task structs.
**            It is used to perform en-masse operations on all v2pthread
**            tasks at once.
*/
task_t *task_list = NULL;

pthread_mutex_t task_list_lock = PTHREAD_MUTEX_INITIALIZER;

/*
**  v2pthread_task_lock is a mutex used to make taskLock exclusive to one
**                    thread at a time.
*/
pthread_mutex_t v2pthread_task_lock = PTHREAD_MUTEX_INITIALIZER;

/*
**  locker contains the pthread ID of the thread which currently
**                   has the scheduler locked (or NULL if it is unlocked).
*/
static pthread_t locker = 0;

/*
**  taskLock_level tracks recursive nesting levels of taskLock/unlock calls
**                 so the scheduler is only unlocked at the outermost
**                 taskUnlock call.
*/
static unsigned long taskLock_level = 0;

/*
**  taskLock_change is a condition variable which signals a change from
**                  locked to unlocked or vice-versa.
*/
static pthread_cond_t taskLock_change = PTHREAD_COND_INITIALIZER;

typedef void *(*start_routine) (void *);
// TODO:do we need ts_malloc ts_free?
// thread-safe malloc
void *ts_malloc(size_t blksize)
{
	void *blkaddr;
	static pthread_mutex_t malloc_lock = PTHREAD_MUTEX_INITIALIZER;

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &malloc_lock);
	pthread_mutex_lock(&malloc_lock);

	blkaddr = malloc(blksize);

	pthread_cleanup_pop(1);

	return (blkaddr);
}

// thread-safe free
void ts_free(void *blkaddr)
{
	static pthread_mutex_t free_lock = PTHREAD_MUTEX_INITIALIZER;

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &free_lock);
	pthread_mutex_lock(&free_lock);

	free(blkaddr);

	pthread_cleanup_pop(1);
}

/*****************************************************************************
**  my_task - returns a pointer to the task struct for the calling task
*****************************************************************************/
task_t *my_task(void)
{
	pthread_t my_pthrid;
	task_t *t;
	my_pthrid = pthread_self();
	/*
	 **  No locking
	 **  of the task list is done here since the access is read-only.
	 **  NOTE that a task being appended to the task_list MUST have its
	 **  nxt_task member initialized to NULL before being linked into
	 **  the list. 
	 */
	for (t = task_list; t != NULL; t = t->nxt_task) {
		if (my_pthrid == t->pthrid) {
			return t;
		}
	}
	return NULL;
}

/*****************************************************************************
** task_for - returns the address of the task struct for the task
**           idenified by taskid
*****************************************************************************/
task_t *task_for(int taskid)
{
	task_t *t;
	if (!taskid)
		return my_task();

	for (t = task_list; t != NULL; t = t->nxt_task) {
		if (t->taskid == taskid) {
			return t;
		}
	}
	return NULL;
}


/*****************************************************************************
** taskLock - 'locks the scheduler' to prevent preemption of the current task
**           by other task-level code.  Because we cannot actually lock the
**           scheduler in a pthreads environment, we temporarily set the
**           dynamic priority of the calling thread above that of any other
**           thread, thus guaranteeing that no other tasks preempt it.
*****************************************************************************/
STATUS taskLock(void)
{
	pthread_t my_pthrid = pthread_self();
	task_t *task = my_task();
	int got_lock = FALSE;
	
	/*
	 **  v2pthread_task_lock ensures that only one v2pthread pthread at a time gets
	 **  to run at max_priority (effectively locking out all other v2pthread
	 **  pthreads).  Due to the semantics of the pthread_cleanup push/pop
	 **  pairs (which protect against deadlocks in the event a thread gets
	 **  terminated while holding the mutex lock), we cannot safely leave
	 **  the mutex itself locked until taskUnlock() is called.  Therefore,
	 **  we instead use the mutex to provide 'atomic access' to a global
	 **  flag indicating if the scheduler is currently locked.  We will
	 **  'spin' and briefly suspend until the scheduler is unlocked, and
	 **  will then lock it ourselves before proceeding.
	 */
	//TRACEF("%x",my_pthrid);
	/*
	 **  'Spin' here until locker == NULL or our pthread ID
	 **  This effectively prevents more than one pthread at a time from
	 **  setting its priority to max_priority.
	 */
	do {
		pthread_mutex_clean_lock(&v2pthread_task_lock);
		if (!locker || locker == my_pthrid) {
			locker = my_pthrid;
			taskLock_level++;
			if (taskLock_level == 0L)
				taskLock_level--;
			got_lock = TRUE;
			pthread_cond_broadcast(&taskLock_change);
			TRACEF("level %i", taskLock_level);
		} else {
			TRACEF("waiting locker %x level %i", locker, taskLock_level);
			if ( task ) task-> waiting_m = &v2pthread_task_lock;
			pthread_cond_wait(&taskLock_change, &v2pthread_task_lock);
			if ( task ) task-> waiting_m = NULL;
		}
		pthread_mutex_unlock(&v2pthread_task_lock);

		//  Add a cancellation point to this loop, since there are no others.
		pthread_testcancel();
		pthread_cleanup_pop(0);
	} while (got_lock == FALSE);
	assert(pthread_self() == locker);
	/*
	 **  task_list_lock prevents other v2pthread pthreads from modifying
	 **  the v2pthread pthread task list while we're searching it and modifying
	 **  the calling task's priority level.
	 */
	pthread_mutex_clean_lock(&task_list_lock);
	if (task) {
		int max_priority, sched_policy;
		//int sched_policy2;
		struct sched_param schedparam;
		//struct sched_param schedparam2;
		pthread_attr_getschedpolicy(&task->attr, &sched_policy);
		pthread_attr_getschedparam(&task->attr, &schedparam);
		max_priority = sched_get_priority_max(sched_policy);
		/*
		TRACEF();
		TRACEV("%i", sched_policy);
		TRACEV("%i", max_priority);
		CHK0(pthread_getschedparam(task->pthrid, &sched_policy2, &schedparam2));
		TRACEV("%x", task->pthrid);
		TRACEV("%i", sched_policy2);
		TRACEV("%i", schedparam2.sched_priority);
		TRACEV("%i", schedparam.sched_priority);
		//assert(sched_policy == sched_policy2);
		//assert(schedparam.sched_priority == schedparam2.sched_priority);
		*/
		schedparam.sched_priority = max_priority;
		CHK0(pthread_attr_setschedparam(&task->attr, &schedparam));
		CHK0(pthread_setschedparam(task->pthrid, sched_policy, &schedparam));
		/*

		assert(0 == pthread_attr_getschedpolicy(&task->attr, &sched_policy));
		assert(0 == pthread_attr_getschedparam(&task->attr, &schedparam));
		assert(schedparam.sched_priority = sched_get_priority_max(sched_policy));
		CHK0(pthread_getschedparam(task->pthrid, &sched_policy2, &schedparam2));
		TRACEV("%x", task->pthrid);
		TRACEV("%i", sched_policy2);
		TRACEV("%i", schedparam2.sched_priority);
		*/
	  exit:{
		}
	}
	pthread_cleanup_pop(1);
	return OK;
}

/*****************************************************************************
** taskUnlock - 'unlocks the scheduler' to allow preemption of the current
**             task by other task-level code.  Because we cannot actually lock
**             the scheduler in a pthreads environment, the dynamic priority of
**             the calling thread was temporarily raised above that of any
**             other thread.  Therefore, we now restore the priority of the
**             calling thread to its original value to 'unlock' the task
**             scheduler.
*****************************************************************************/
STATUS taskUnlock(void)
{
	task_t *task;
	int sched_policy;
	/*
	 **  locker ensures that only one v2pthread pthread at a time gets
	 **  to run at max_priority (effectively locking out all other v2pthread
	 **  pthreads).  Unlock it here to complete 'unlocking' of the scheduler.
	 */
	pthread_mutex_clean_lock(&v2pthread_task_lock);
	TRACEF("locker %x level %i",locker,taskLock_level);

	if (locker == pthread_self()) {
		TRACEF();
		if (taskLock_level > 0L)
			taskLock_level--;
		if (taskLock_level < 1L) {
			/*
			 **  task_list_lock prevents other v2pthread pthreads from modifying
			 **  the v2pthread pthread task list while we're searching it and
			 **  modifying the calling task's priority level.
			 */
			pthread_mutex_clean_lock(&task_list_lock);
			task = my_task();
			if (task) {
				struct sched_param schedparam;
				pthread_attr_getschedpolicy(&(task->attr), &sched_policy);
				pthread_attr_getschedparam(&(task->attr), &schedparam);
				//pthread_getschedparam(task->pthrid, &sched_policy2, &schedparam2);
				schedparam.sched_priority = task->prv_priority.sched_priority;

				pthread_attr_setschedparam(&(task->attr), &schedparam);
				CHK0(pthread_setschedparam(task->pthrid, sched_policy, &schedparam));
			  exit:{
				}
			}
			pthread_cleanup_pop(1);

			locker = 0;
			pthread_cond_broadcast(&taskLock_change);
		}
		//TRACEF("%x level %i",locker, taskLock_level);
	} else {
		//TRACEF("locking tid %ld my tid %lx", locker, pthread_self());
	}

	pthread_cleanup_pop(1);
	return OK;
}

/*****************************************************************************
** link_susp_task - appends a new task pointer to a linked list of task pointers
**                 for tasks suspended on the object owning the list.
*****************************************************************************/
void link_susp_task(task_t ** list_head, task_t * new_entry)
{
	//TRACEF("%x %x",list_head,new_entry);
	if (!new_entry) 
		return;
	pthread_mutex_clean_lock(&task_list_lock);
	new_entry->nxt_susp = NULL;
	task_t **i = list_head;
	while (*i) { 
		if (*i==new_entry) {
			TRACEF("warning: double entry");
			*i = (*i)->nxt_susp;	// remove the task
			continue;
		}
		i = &(*i)->nxt_susp;	// look for the tail
	}
	*i = new_entry;

	/*
	 **  Initialize the suspended task's pointer back to suspend list
	 **  This is used for cleanup during task deletion.
	 */
	//new_entry->suspend_list = *list_head;
	new_entry->state |= PEND;

	pthread_cleanup_pop(1);
}

/*****************************************************************************
** unlink_susp_task - removes task pointer from a linked list of task pointers
**                   for tasks suspended on the object owning the list.
*****************************************************************************/
void unlink_susp_task(task_t ** list_head, task_t * entry)
{
	task_t **i = list_head;
	TRACEF("%x %x", list_head, entry);
	if (!entry) 
		return;
	pthread_mutex_clean_lock(&task_list_lock);
	while (*i && (*i != entry) )
		i = &(*i)->nxt_susp;
	if (*i) {
		//TRACEF("%x", entry);
		*i = (*i)->nxt_susp;	// remove the task
		entry->nxt_susp = NULL;
		entry->state &= ~PEND;
	} else {
		TRACEF("warning: entry not found");
	}
	pthread_cleanup_pop(1);
}

/*****************************************************************************
** signal_for_my_task - searches the specified 'pended task list' for the
**                      task to be selected according to the specified
**                      pend order.  If the selected task is the currently
**                      executing task, the task is deleted from the
**                      specified pended task list and returns a non-zero
**                      result... otherwise the pended task list is not
**                      modified and a zero result is returned.
*****************************************************************************/
int signal_for_my_task(task_t ** list_head, int pend_order)
{
	// used in lmsgQLib.c
	TRACEF();
	task_t *signalled_task;
	task_t *t;
	int result;

	result = FALSE;
	TRACEF("list head = %p", *list_head);
	if (!list_head)
		return result;
	signalled_task = *list_head;

	//  First determine which task is being signalled
	if (pend_order != 0) {
		/*
		 **  Tasks pend in priority order... locate the highest priority
		 **  task in the pended list.
		 */
		for (t = *list_head; t; t = t->nxt_susp) {
			if ((t->prv_priority).sched_priority > (signalled_task->prv_priority).sched_priority)
				signalled_task = t;
			TRACEF("%x priority %d", t, (t->prv_priority).sched_priority);
		}
	}
	/*
	   else
	   **
	   ** Tasks pend in FIFO order... signal is for task at list head.
	 */

	//  Signalled task located... see if it's the currently executing task.
	if (signalled_task == my_task()) {
		// The currently executing task is being signalled...
		result = TRUE;
	}
	TRACEF("signalled task @ %p my task @ %p", signalled_task, my_task());

	return result;
}

/*****************************************************************************
** new_tid - assigns the next unused task ID for the caller's task
*****************************************************************************/
static int new_tid(void)
{
	task_t *t;
	int new_taskid;

	new_taskid = 1;
	//  Get the highest previously assigned task id and add one.
	for (t = task_list; t; t = t->nxt_task) {
		/*
		 **  We use a kluge here to prevent address-based task IDs created
		 **  by explicit taskInit calls from polluting the normal sequence
		 **  for task ID numbers.  This is based on the PROBABILITY that
		 **  Linux will not allocate pthread data space below the 64K
		 **  address space.  NOTE that this will BREAK taskSpawn if you
		 **  create 64K tasks or more.
		 */
		if ((t->taskid < 65536) && (t->taskid >= new_taskid)) {
			new_taskid = t->taskid + 1;
		}
	}

	return new_taskid;
}

/*****************************************************************************
** translate_priority - translates a v2pthread priority into a pthreads priority
*****************************************************************************/
static int translate_priority(int v2pthread_priority, int sched_policy, int *errp)
{
	int max_priority, min_priority, pthread_priority;

	//  Validate the range of the user's task priority.
	if ((v2pthread_priority > MIN_V2PT_PRIORITY) | (v2pthread_priority < MAX_V2PT_PRIORITY))
		*errp = S_taskLib_ILLEGAL_PRIORITY;

	/*
	 **  Translate the v2pthread priority into a pthreads priority.
	 **  'Invert' the caller's priority so highest priority == highest number.
	 */
	pthread_priority = MIN_V2PT_PRIORITY - v2pthread_priority;

	//  Next get the allowable priority range for the scheduling policy.
	min_priority = sched_get_priority_min(sched_policy);
	max_priority = sched_get_priority_max(sched_policy);

	/*
	 **  'Telescope' the v2pthread priority (0-255) into the smaller pthreads
	 **  priority range (1-97) on a proportional basis.  NOTE that this may
	 **  collapse some distinct v2pthread priority levels into the same priority.
	 **  Use this technique if the tasks span a large priority range but no
	 **  tasks are within fewer than 3 levels of one another.
	 */
	pthread_priority *= max_priority;
	pthread_priority /= (MIN_V2PT_PRIORITY + 1);

	/*
	 **  Now 'clip' the new priority level to within priority range.
	 **  Reserve max_priority level for temporary use during system calls.
	 **  Reserve (max_priority level - 1) for the system exception task.
	 **  NOTE that relative v2pthread priorities may not translate properly
	 **  if the v2pthread priorities used span several multiples of max_priority.
	 **  Use this technique if the tasks span a small priority range and their
	 **  relative priorities are within 3 or fewer levels of one another.
	 */
	pthread_priority %= (max_priority - 1);
	if (pthread_priority < min_priority)
		pthread_priority = min_priority;

	return pthread_priority;
}

/*****************************************************************************
** task_delete - deletes a pthread task struct from the task_list
**              and frees the memory allocated for the task
*****************************************************************************/
static void task_delete(task_t * task)
{
	task_t **i;

	TRACEF("%x %x", task, task->waiting);
	//unlink_susp_task(&task->suspend_list, task);
	if (task->waiting) unlink_susp_task(&task->waiting->first_susp, task);
	pthread_mutex_clean_lock(&task_list_lock);
	for (i = &task_list; *i; i = &(*i)->nxt_task) {
		if (task == *i) {
			TRACEF("%x th %x", task,task->pthrid);
			*i = (*i)->nxt_task;	// remove
			break;
		}
	}
	pthread_cleanup_pop(1);

	if (task->taskname)
		ts_free(task->taskname);
	if (!task->static_task)
		ts_free(task);
}

/*****************************************************************************
** notify_task_delete - notifies any pended tasks of the specified task's
**                      deletion.
*****************************************************************************/
void notify_task_delete(task_t * task)
{
	if (!task->first_susp) return;
	/*
	 **  Task just made deletable... ensure that we awaken any
	 **  other tasks pended on deletion of this task
	 */
	pthread_mutex_clean_lock(&(task->dbcst_lock));
	pthread_mutex_clean_lock(&task->tdelete_lock);
	TRACEF("");
	pthread_cond_broadcast(&(task->t_deletable));

	pthread_cleanup_pop(1);

	/*
	 **  Wait for all pended tasks to receive deletion notification.
	 **  The last task to receive the notification will signal the
	 **  delete broadcast-complete condition variable.
	 */
	TRACEF("wait till pended tasks respond @ task %p", task);
	while (task->first_susp != (task_t *) NULL)
		pthread_cond_wait(&(task->delete_bcplt), &(task->dbcst_lock));

	TRACEF("all pended tasks responded @ task %p", task);
	pthread_cleanup_pop(1);
}


void task_delete_unlock(task_t * task)
{
	TRACEF();
	task_delete(task);
	taskUnlock();
}


/*****************************************************************************
** taskDeleteForce - removes the specified task(s) from the task list,
**                   frees the memory occupied by the task struct(s),
**                   and kills the pthread(s) associated with the task(s).
*****************************************************************************/
STATUS taskDeleteForce(int tid)
{
	task_t *t;
	task_t *self_task = my_task();
	STATUS error = OK;

	taskLock();

	t = task_for(tid);
	TRACEF("%#x %x",tid,t);

	if (!t) {
		error = S_objLib_OBJ_DELETED;
		goto exit;
	}
	notify_task_delete(t);

	if ( t != self_task) {
		/*
		 **  Task being deleted is not the current task.
		 **  Kill the task pthread and wait for it to die.
		 **  Then de-allocate its data structures.
		 */
		TRACEF("%i %x", tid, t);
		pthread_cancel(t->pthrid);
		pthread_join(t->pthrid, NULL);
		task_delete(t);
	} else {
		/*
		 **  Kill the currently executing task's pthread
		 **  and then de-allocate its data structures.
		 */
		TRACEF("self %i %x", tid, t);
		//pthread_detach(self_task->pthrid);
		//pthread_cleanup_push((void (*)(void *)) task_delete_unlock, self_task);
		task_delete_unlock(self_task);
		//pthread_exit(NULL);
		// not executed
		//pthread_cleanup_pop(1);
	
	}

  exit:
	taskUnlock();
	if (error != OK) {
		errno = (int) error;
		error = ERROR;
	}
	return error;
}

/*****************************************************************************
**  task_wrapper_cleanup ensures that a killed pthread releases the
**                         scheduler lock if it owned it.
*****************************************************************************/
static void task_wrapper_cleanup(void *task)
{
	task_t *mytask;
	TRACEF();
	mytask = (task_t *) task;
	pthread_mutex_lock(&v2pthread_task_lock);

	if (locker == pthread_self()) {
		taskLock_level = 0;
		locker = (pthread_t) NULL;
		// or call Unlock?
	}
	pthread_mutex_unlock(&v2pthread_task_lock);
}

/*****************************************************************************
**  task_wrapper is a pthread used to 'contain' a v2pthread task.
*****************************************************************************/
void *task_wrapper(task_t *task)
{
	errno = 0;

	//TRACEF("%x %x %i", task,pthread_self(),getpid());

	// Ensure that this pthread will release the scheduler lock if killed.
	pthread_cleanup_push(task_wrapper_cleanup, task);

	// Call the v2pthread task.  Normally this is an endless loop and doesn't
	/*
	 * TODO: replace this ugly patch with waiting for conditional variable on task->thrid
	 * or better yet, fill the task->pthrid with pthread_self(). Check if this can be done
	 * with atomic_xchange here and in the taskActivate function simultaneously
	 */
	sched_yield();
	while (0 == (volatile pthread_t) task->pthrid) {
		sched_yield();
		usleep(5000);
		TRACEF("task_wrapper() PATCH! task (%s) wait for task->pthrid to get its value... \n",
			   task->taskname ? task->taskname : "no-name");
	}
	(*(task->entry_point)) (task->parms[0], task->parms[1], task->parms[2], task->parms[3],
							task->parms[4], task->parms[5], task->parms[6], task->parms[7],
							task->parms[8], task->parms[9]);
	//  If for some reason the task above DOES return, clean up the  pthread and task resources and kill the pthread.
	pthread_cleanup_pop(1);

	// NOTE taskDelete takes no action if the task has already been deleted.
	taskDeleteForce(task->taskid);
	TRACEF("DONE");
	return NULL;
}

/*****************************************************************************
** taskDelay - suspends the calling task for the specified number of ticks.
**            ( one tick is currently implemented as ten milliseconds )
*****************************************************************************/
STATUS taskDelay(int interval)
{
	struct timeval now, timeout;
	TRACEF("%i",interval);
	task_t *task = my_task();
	if ( task ) task->state |= DELAY;

	// Also it could be implemented via pthread_cond_timed_wait

	usleep(interval * V2PT_TICK * 1000);

	if ( task ) task->state &= ~ DELAY;

	return (OK);
}

/*****************************************************************************
** taskIdListGet - returns a list of active task identifiers
*****************************************************************************/
int taskIdListGet(int list[], int maxIds)
{
	task_t *t;
	int count;
	count = 0;
	taskLock();
	assert(list);
	for (t = task_list; t; t = t->nxt_task) {
		if (count < maxIds) {
			list[count] = t->taskid;
			count++;
		}
	}
	taskUnlock();
	return (count);
}

/*****************************************************************************
** taskIdSelf - returns the identifier of the calling task
*****************************************************************************/
int taskIdSelf(void)
{
	task_t *self_task;
	int my_tid;

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &task_list_lock);
	pthread_mutex_lock(&task_list_lock);

	self_task = my_task();

	pthread_cleanup_pop(1);

	if (self_task)
		my_tid = self_task->taskid;
	else
		my_tid = 0;

	return my_tid;
}

/*****************************************************************************
** taskIdVerify - indicates whether the specified task exists or not
** returns OK==0, if a task exists
*****************************************************************************/
STATUS taskIdVerify(int taskid)
{
	task_t *task;
	STATUS error;

	pthread_mutex_clean_lock(&task_list_lock);

	task = task_for(taskid);

	pthread_cleanup_pop(1);

	if (task != NULL)
		error = OK;
	else						/* NULL TCB pointer */
		error = S_objLib_OBJ_ID_ERROR;

	if (error != OK) {
		errno = (int) error;
		error = ERROR;
	}
	return error;
}

/*****************************************************************************
** taskInit - initializes the requisite data structures to support v2pthread 
**            task behavior not directly supported by Posix threads.
*****************************************************************************/
extern STATUS taskInit(WIND_TCB * task, char *name, int pri,
					   int opts, char *pstack, int stksize, FUNCPTR entry, int arg1, int arg2,
					   int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9,
					   int arg10);
STATUS taskInit(task_t * task, char *name, int pri, int opts,
				char *pstack, int stksize,
				int (*funcptr) (int, int, int, int, int, int, int, int, int, int),
				int arg1, int arg2, int arg3, int arg4, int arg5,
				int arg6, int arg7, int arg8, int arg9, int arg10)
{
	int i, new_priority, sched_policy;
	STATUS error = OK;
	TRACEF("%x %s %x", task, name, funcptr);

	if (opts != 0) {
		// no options are currently implemented.
		errno = ENOSYS;
		return (errno);
	}
	if (!task) {
		error = S_objLib_OBJ_ID_ERROR;
		goto error;
	}

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &task_list_lock);
	pthread_mutex_lock(&task_list_lock);

	memset(task,0,sizeof(*task));
	task->pthrid = 0;
	task->taskid =  (int)task;

	if (name) {
		i = strlen(name) + 1;
		task->taskname = ts_malloc(i);
		if (task->taskname)
			strncpy(task->taskname, name, i);
	} else
		task->taskname = NULL;

	task->vxw_priority = pri;

	pthread_attr_init(&task->attr);

	pthread_attr_getschedparam(&task->attr, &task->prv_priority);

	if (roundRobinIsEnabled())
		sched_policy = SCHED_RR;
	else
		sched_policy = SCHED_FIFO;
	pthread_attr_setschedpolicy(&task->attr, sched_policy);

	new_priority = translate_priority(pri, sched_policy, &error);
	CHK0(error);
exit:
	task->prv_priority.sched_priority = new_priority;
	CHK0(pthread_attr_setschedparam(&task->attr, &task->prv_priority));

	task->entry_point = funcptr;
	task->static_task = 1;
	task->flags = opts;
	task->state = DEAD;
	//task->suspend_list = NULL;
	task->nxt_susp = NULL;
	task->nxt_task = NULL;
	task->delete_safe_count = 0;

	// Mutex and Condition variable for task delete 'pend'
	pthread_mutex_init(&task->tdelete_lock, NULL);
	pthread_cond_init(&task->t_deletable, NULL);

	// Mutex and Condition variable for task delete 'broadcast'
	pthread_mutex_init(&task->dbcst_lock, NULL);
	pthread_cond_init(&task->delete_bcplt, NULL);

	task->first_susp = NULL;

	task->parms[0] = arg1;
	task->parms[1] = arg2;
	task->parms[2] = arg3;
	task->parms[3] = arg4;
	task->parms[4] = arg5;
	task->parms[5] = arg6;
	task->parms[6] = arg7;
	task->parms[7] = arg8;
	task->parms[8] = arg9;
	task->parms[9] = arg10;
	if (error == OK) {
		task_t **i = &task_list;
		while (*i)
			i = &(*i)->nxt_task;	// search_last
		*i = task;					// add to tail
	}
error:
	pthread_mutex_unlock(&task_list_lock);
	pthread_cleanup_pop(0);
	if (error != OK) {
		errno = (int) error;
		error = ERROR;
	}
	return error;
}

/*****************************************************************************
** taskIsReady - indicates if the specified task is ready to run
*****************************************************************************/
BOOL taskIsReady(int taskid)
{
	task_t *task;
	BOOL result = FALSE;
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &task_list_lock);
	pthread_mutex_lock(&task_list_lock);

	task = task_for(taskid);

	if (!task) {
		TRACEF("no task");
	}
	else {
		TRACEF("%x %x",task,task->state);
		if ((task->state & RDY_MSK) == READY)
			result = TRUE;
	}

	pthread_cleanup_pop(1);

	return result;
}

/*****************************************************************************
** taskIsSuspended - indicates if the specified task is explicitly suspended 
*****************************************************************************/
BOOL taskIsSuspended(int taskid)
{
	errno = ENOSYS;
	return (0);
}

/*****************************************************************************
** taskActivate -  creates a pthread containing the specified v2pthread task
*****************************************************************************/
STATUS taskActivate(int tid)
{
	task_t *task;
	STATUS error;
	//TRACEF("%x",tid);
	error = OK;

	/*
	 **  'Lock the v2pthread scheduler' to defer any context switch to a higher
	 **  priority task until after this call has completed its work.
	 */
	taskLock();

	task = task_for(tid);
	if (task ) {
		/*
		 **  Found our task struct.
		 **  Start a new real-time pthread for the task. 
		 */
		if (task->state == DEAD) {
			task->state = READY;
			TRACEF("%x",task);

			/*
			 * TODO There is un ugly patch here to ensure the called thread has a valid task->pthrid
			 * check if that can be done better with atomic_set
			 */
			task->pthrid = 0;
			if (pthread_create(&task->pthrid, &task->attr, (start_routine) task_wrapper, task) != 0) {
				TRACEF("taskActivate pthread_create returned error:");
				error = S_memLib_NOT_ENOUGH_MEMORY;
				task_delete(task);
			}
		} else {
			/*
			 ** task already made runnable
			 */
			TRACEF("taskActivate task @ task %p already active", task);
		}
	} else
		error = S_objLib_OBJ_ID_ERROR;

	/*
	 **  'Unlock the v2pthread scheduler' to enable a possible context switch
	 **  to a task made runnable by this call.
	 */
	taskUnlock();

	//TRACEF("%x %s vxw_Prio=%d, Linux-Prio=%d",
	//      task, task->taskname, task->pthrid, task->vxw_priority, task->prv_priority.sched_priority);

	//TRACEF("ADDED task=%x (%s) task=%x, vxw_Prio=%d, Linux-Prio=%d",
	//   (int) task->pthrid, task->taskname ,
	//   (int) task, task->vxw_priority, (task->prv_priority).sched_priority);

	if (error != OK) {
		errno = (int) error;
		error = ERROR;
	}
	return error;
}

/*****************************************************************************
** taskSpawn -   initializes the requisite data structures to support v2pthread 
**               task behavior not directly supported by Posix threads and
**               creates a pthread to contain the specified v2pthread task.
*****************************************************************************/
int taskSpawn(char *name, int pri, int opts, int stksize,
			  int (*funcptr) (int, int, int, int, int, int, int, int, int,
							  int), int arg1, int arg2, int arg3, int arg4,
			  int arg5, int arg6, int arg7, int arg8, int arg9, int arg10)
{
	task_t *task;
	int my_tid;
	STATUS error;
	char myname[16];

	task = ts_malloc(sizeof(*task));
	memset(task,0,sizeof(*task));
	if (!task) {

		error = S_smObjLib_NOT_INITIALIZED;
		my_tid = (int) error;
		goto exit;
	}
	//Synthesize a default task name if none specified
	my_tid = new_tid();
	TRACEF("%i %s %i %x", my_tid, name, pri, funcptr);
	if (!name) {
		sprintf(myname, "t%d", my_tid);
		name = &(myname[0]);
	}

	error = taskInit(task, name, pri, opts, (char *) NULL, stksize, funcptr,
					 arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
	if (error != OK) {
		ts_free((void *) task);
		goto exit;
	}
	// Establish 'normal' task identifier, overwriting taskid set by taskInit call.
	//pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &task_list_lock);
	//pthread_mutex_lock(&task_list_lock);
	task->taskid = my_tid;

	task->static_task = 0;		// Indicate TCB dynamically allocated

  exit:
	//pthread_mutex_unlock(&task_list_lock);
	//pthread_cleanup_pop(0);

	error = taskActivate(my_tid);
	if (error != OK) {
		my_tid = errno = (int) error;
	}
	return (my_tid);
}


/*****************************************************************************
** taskSuspend - suspends the specified v2pthread task
*****************************************************************************/
STATUS taskSuspend(int tid)
{
	errno = ENOSYS;
	return (errno);
}

/*****************************************************************************
** taskResume - creates a pthread to contain the specified v2pthread task and
*****************************************************************************/
STATUS taskResume(int tid)
{
	errno = ENOSYS;
	return (errno);
}

/*****************************************************************************
** taskPriorityGet - examines the current priority for the specified task
*****************************************************************************/
STATUS taskPriorityGet(int tid, int *priority)
{
	task_t *task;
	STATUS error  = OK;

	taskLock();

	task = task_for(tid);
	if (task) {
		if (priority != (int *) NULL)
			*priority = task->vxw_priority;
	} else
		error = S_objLib_OBJ_ID_ERROR;

	taskUnlock();

	if (error != OK) {
		errno = (int) error;
		error = ERROR;
	}
	return (error);
}


/*****************************************************************************
** taskPrioritySet - sets a new priority for the specified task
*****************************************************************************/
STATUS taskPrioritySet(int tid, int pri)
{
	task_t *task;
	int new_priority, sched_policy;
	STATUS error;

	error = OK;

	taskLock();

	task = task_for(tid);

	if (!task) {
		error = S_objLib_OBJ_ID_ERROR;
		goto exit;
	}
	//  Translate the v2pthread priority into a pthreads priority
	pthread_attr_getschedpolicy(&(task->attr), &sched_policy);
	//pthread_getschedparam(task->pthrid, &sched_policy2, &schedparam2);
	new_priority = translate_priority(pri, sched_policy, &error);

	/*
	 **  Update the TCB with the new priority
	 */
	task->vxw_priority = pri;
	(task->prv_priority).sched_priority = new_priority;

	/*
	 **  If the selected task is not the currently-executing task,
	 **  modify the pthread's priority now.  If the selected task
	 **  IS the currently-executing task, the taskUnlock operation
	 **  will restore this task to the new priority level.
	 */
	if ((tid != 0) && (task != my_task())) {
		struct sched_param schedparam;
		pthread_attr_setschedparam(&task->attr, &task->prv_priority);
		pthread_attr_getschedparam(&task->attr, &schedparam);
		schedparam.sched_priority = new_priority;
		pthread_attr_setschedparam(&task->attr, &schedparam);
		pthread_setschedparam(task->pthrid, sched_policy, &schedparam);
	}
  exit:
	taskUnlock();

	if (error != OK) {
		errno = (int) error;
		error = ERROR;
	}
	return (error);
}

/*****************************************************************************
** taskName - returns the name of the specified v2pthread task
*****************************************************************************/
char *taskName(int tid)
{
	task_t *t;
	char *taskname;
	static char NullTaskName[] = "NULLTASK";
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &task_list_lock);
	pthread_mutex_lock(&task_list_lock);

	t = task_for(tid);

	if (t)
		taskname = t->taskname;
	else
		taskname = NullTaskName;

	pthread_cleanup_pop(1);

	return taskname;
}

/*****************************************************************************
** taskNametoId - identifies the named v2pthread task
*****************************************************************************/
int taskNameToId(char *name)
{
	task_t *t;
	int tid;

	tid = (int) ERROR;

	if (!name)
		return tid;
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &task_list_lock);
	pthread_mutex_lock(&task_list_lock);

	for (t = task_list; t; t = t->nxt_task) {
		if ((strcmp(name, t->taskname)) == 0) {
			tid = t->taskid;
			break;
		}
	}

	pthread_cleanup_pop(1);

	return tid;
}

// utility function for taskDelete
// NOTE: called after taskLock, and exits in locked state
void task_pend_delete(task_t * self_task, task_t * t)
{
	/*
	 **  Specified task is a different task, but not deletable.
	 **  Our task must pend until the specified task becomes
	 **  deletable.  Add our task to the list of tasks waiting to
	 **  delete the specified task.
	 */
	TRACEF("%x %x ", self_task, t);
	link_susp_task(&t->first_susp, self_task);

	/*
	 **  Lock mutex for task delete_safe_count & condition variable
	 */
	TRACEF("taskDelete - lock delete cond var mutex @ task %p", t);
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &(t->tdelete_lock));
	pthread_mutex_lock(&(t->tdelete_lock));

	//  Unlock scheduler to allow other tasks to make specified task deletable.
	taskUnlock();

	//  Wait without timeout for task to become deletable.
	TRACEF("taskDelete - wait till task @ task %p deletable", t);
	while (t->delete_safe_count > 0) {
		pthread_cond_wait(&(t->t_deletable), &(t->tdelete_lock));
	}

	TRACEF("taskDelete - task @ task %p now deletable", t);
	taskLock();

	/*
	 **  Remove the calling task's task from the pended task list
	 **  for the task being deleted.  Clear the calling task 's
	 **  suspend list pointer since the TCB it was suspended on is
	 **  being deleted and deallocated.
	 */
	//unlink_susp_task(&(t->first_susp), self_task);
	TRACEV("%x", self_task->waiting);
	if (self_task->waiting) unlink_susp_task(&self_task->waiting->first_susp, self_task);
	//self_task->suspend_list = NULL;

	/*
	 **  If our task was the last one pended, signal the task
	 **  which enabled the deletion and indicate that all pended
	 **  tasks have been awakened.
	 */
	if (t->first_susp == (task_t *) NULL) {
		// Lock mutex for task delete broadcast completion
		TRACEF("taskDelete - lock del bcast mutex @ task %p", t);
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &(t->dbcst_lock));
		pthread_mutex_lock(&(t->dbcst_lock));

		// Signal task delete broadcast completion. 
		TRACEF("taskDelete - bcast delete complt @ task %p", t);
		pthread_cond_broadcast(&(t->delete_bcplt));

		//  Unlock the task delete broadcast completion mutex. 
		TRACEF("taskDelete - unlock del bcast mutex @ task %p", t);
		pthread_cleanup_pop(1);
		//task_deletable = TRUE;
	}
	// Unlock the mutex for the condition variable and clean up.
	TRACEF("taskDelete - unlock delete cond var mutex @ task %p", t);
	pthread_cleanup_pop(1);
}

/*****************************************************************************
** taskDelete - removes the specified task(s) from the task list,
**              frees the memory occupied by the task struct(s),
**              and kills the pthread(s) associated with the task(s).
*****************************************************************************/
STATUS taskDelete(int tid)
{
	task_t *t;
	task_t *self_task = my_task();
	int task_deletable;
	STATUS error  = OK;

	taskLock();
	t = task_for(tid);
	if (!t) {
		error = S_objLib_OBJ_DELETED;
		TRACEF("%x not found",tid);
		goto exit;
	}
	/*
	 **  If the current task is one of several attempting to delete the
	 **  specified task, then only the first of the deleting tasks will
	 **  actually succeed in deleting the target task.
	 */
	TRACEF("%x", t);
	pthread_mutex_clean_lock(&t->tdelete_lock);

	if (t->delete_safe_count > 0)
		task_deletable = FALSE;
	else
		task_deletable = TRUE;

	pthread_cleanup_pop(1);

	if (task_deletable == TRUE)
		error = taskDeleteForce(tid);

	if (task_deletable == FALSE) {
		if (t == self_task) {
			/*
			 **  Task being deleted is currently executing task, and is
			 **  delete-protected at this time...
			 **  Task cannot block or delete itself while delete-protected.
			 */
			TRACEF("can't self-delete prot task @ task %p", t);
			error = S_objLib_OBJ_UNAVAILABLE;
		} else {
			/*
			 **  Specified task is a different task, but not deletable.
			 **  Our task must pend until the specified task becomes
			 **  deletable.  Add our task to the list of tasks waiting to
			 **  delete the specified task.
			 */
			task_pend_delete(self_task, t);
		}
	}

  exit:
	taskUnlock();

	if (error != OK) {
		errno = (int) error;
		error = ERROR;
	}

	return error;
}

/*****************************************************************************
** taskRestart - terminates the specified task and starts it over from its
**               entry point.
*****************************************************************************/
STATUS taskRestart(int tid)
{
	task_t *t;
	taskLock();
	task_t *self_task= my_task();
	STATUS error = OK;

	if (tid == 0)
		t = self_task;
	else
		t = task_for(tid);

	if (!t) {
		error = S_objLib_OBJ_ID_ERROR;
		goto exit;
	}
	TRACEF("%x ", t);
	//unlink_susp_task(&t->suspend_list, t); // ???
	//TRACEV("%x", t->waiting);
	if (t->waiting) unlink_susp_task(&t->waiting->first_susp, t);

	if (t != self_task) {
		/*
		 **  Task being restarted is not the current task.
		 **  Kill the task pthread and wait for it to die.
		 */
		TRACEF("taskRestart - other task @ %x", t);
		pthread_cancel(t->pthrid);
		pthread_join(t->pthrid, (void **) NULL);

		//  Start a new pthread using the existing task struct.
		t->pthrid = (pthread_t) NULL;
		t->state = READY;

		if (pthread_create(&t->pthrid, &t->attr,(start_routine) task_wrapper, (void *) t) != 0) {
			perror("taskRestart pthread_create returned error:");
			error = S_memLib_NOT_ENOUGH_MEMORY;
		}
	} else {
		//  Restart the currently executing task.
		TRACEF(" self task @ %p", t);

		//  Detach our task's pthread to clean up memory, etc. without a 'join' operation.
		pthread_detach(self_task->pthrid);

		//  Start a watchdog timer to restart our task after we terminate.
		selfRestart(self_task);

		// Kill the currently executing task's pthread.
		pthread_exit(NULL);
	}

	taskUnlock();
  exit:
	if (error != OK) {
		errno = (int) error;
		error = ERROR;
	}
	return (error);
}

/*****************************************************************************
** taskSafe - marks the calling task as safe from explicit deletion.
*****************************************************************************/
STATUS taskSafe(void)
{
	task_t *t;
	TRACEF();	
	taskLock();

	t = my_task();

	/*
	 **  Lock mutex for task delete_safe_count & condition variable
	 */
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &(t->tdelete_lock));
	pthread_mutex_lock(&(t->tdelete_lock));
	TRACEF(" lock delete cond var mutex @ task %p", t);

	// Increment task delete_safe_count and adjust for any overflow.
	t->delete_safe_count++;
	if (t->delete_safe_count <= 0) {
		fprintf(stderr, "delete_safe_count overflow");
		t->delete_safe_count--;
	}
	TRACEF("new delete_safe_count %d @ task %p", t->delete_safe_count, t);
	pthread_cleanup_pop(1);

	taskUnlock();

	return OK;
}

/*****************************************************************************
** taskUnsafe - marks the calling task as subject to explicit deletion.
*****************************************************************************/
STATUS taskUnsafe(void)
{
	task_t *t;
	int task_made_deletable;

	taskLock();

	t = my_task();

	task_made_deletable = FALSE;

	TRACEF("lock delete cond var mutex @ task %p", t);
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &(t->tdelete_lock));
	pthread_mutex_lock(&(t->tdelete_lock));

	if (t->delete_safe_count > 0) {
		t->delete_safe_count--;
		if (t->delete_safe_count == 0)
			task_made_deletable = TRUE;
	}
	TRACEF("new delete_safe_count %d @ task %p", t->delete_safe_count, t);

	TRACEF("unlock delete cond var mutex @ task %p", t);
	pthread_cleanup_pop(1);

	taskUnlock();

	if (task_made_deletable) {
		/*
		 **  Task just made deletable... ensure that we awaken any
		 **  other tasks pended on deletion of this task
		 */
		if (t->first_susp != (task_t *) NULL) {
			notify_task_delete(t);
		}
	}

	return ((STATUS) OK);
}

WIND_TCB *taskTcb(int taskid)
{
	task_t *task;

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &task_list_lock);
	pthread_mutex_lock(&task_list_lock);

	task = task_for(taskid);

	pthread_cleanup_pop(1);

	return task;
}

void display_task(FILE * out, ulong tid)	// OLD
{
	TRACEF();
	int policy;
	int detachstate;
	struct sched_param schedparam;
	task_t *cur_task;

	cur_task = task_for(tid);

	if (cur_task == (task_t *) NULL)
		return;

	fprintf(out, "Task %x Name: %s  Task ID: %d  Thread ID: %lx  Vxworks priority: %d",
			cur_task,
			cur_task->taskname, cur_task->taskid, cur_task->pthrid, cur_task->vxw_priority);

	pthread_attr_getschedpolicy(&cur_task->attr, &policy);
	//pthread_getschedparam(task->pthrid, &sched_policy2, &schedparam2);
	switch (policy) {
	case SCHED_FIFO:
		fprintf(out, "schedpolicy: SCHED_FIFO ");
		break;
	case SCHED_RR:
		fprintf(out, "schedpolicy: SCHED_RR ");
		break;
	case SCHED_OTHER:
		fprintf(out, "schedpolicy: SCHED_OTHER ");
		break;
	default:
		fprintf(out, "schedpolicy: %d ", policy);
	}
	pthread_attr_getschedparam(&cur_task->attr, &schedparam);
	fprintf(out, " priority %d ", schedparam.sched_priority);
	fprintf(out, " prv_priority %d ", cur_task->prv_priority.sched_priority);
	pthread_attr_getdetachstate(&cur_task->attr, &detachstate);
	fprintf(out, " detachstate %d ", detachstate);
	fprintf(out, "\n");
}

void taskShow(FILE * out, task_t * t)
{
	//TRACEF("%x", t);
	int policy;
	int detachstate;
	struct sched_param schedparam;

	fprintf(out, "%#10x %10x %10s ", t->taskid, t, t->taskname);
	fprintf(out, "th:%x ", t->pthrid);
	fprintf(out, "s:%#x ", t->state);
	if ( t->state & PEND  ) fprintf(out, "P ");
	if ( t->state & DELAY  ) fprintf(out, "D " );
	fprintf(out, "w:%x ", t->waiting);
	if (t->waiting && t->waiting->current_owner)  {
		fprintf(out, "%s ", t->waiting->current_owner->taskname);
	}
	fprintf(out, "w:%x ", t->waiting_m);
	fprintf(out, "s:%x ", t->first_susp);
	//fprintf(out, " Thread ID: %lx  Vxworks priority: %d", t->pthrid, t->vxw_priority);

	pthread_attr_getschedpolicy(&t->attr, &policy);
	//pthread_getschedparam(task->pthrid, &sched_policy2, &schedparam2);
	switch (policy) {
	case SCHED_FIFO:
		fprintf(out, "SCHED_FIFO ");
		break;
	case SCHED_RR:
		fprintf(out, "SCHED_RR ");
		break;
	case SCHED_OTHER:
		fprintf(out, "SCHED_OTHER ");
		break;
	default:
		fprintf(out, "schedpolicy: %d ", policy);
	}
	pthread_attr_getschedparam(&t->attr, &schedparam);
	fprintf(out, " pri %d ", schedparam.sched_priority);
	fprintf(out, " prv_pri %d ", t->prv_priority.sched_priority);
	pthread_attr_getdetachstate(&t->attr, &detachstate);
	//fprintf(out, " detachstate %d ", detachstate);
	fprintf(out, "\n");
}

int taskList(FILE * out, int mem)
{
	int c=0;
	TRACEF();
	fprintf(out, "locker=%x\n", locker);
	fprintf(out, "taskLock_level=%x\n", taskLock_level);
	task_t *t = task_list;
	while (t) {
		taskShow(out, t);
		if (mem) {
			int *w;
			for (w = (int *) t; w < (int *) (t + 1); w++)
				fprintf(out, "%x ", *w);
			fprintf(out, "\n");
		}
		t = t->nxt_task;
		c++;
	}
	return c;
}
#define ERR(e) case e: return #e;
char * VxWorksError(STATUS status)
{
	/*
	case (status && 0xFF0000) {
	
		TASK_ERRS
		MEM_ERRS
		MSGQ_ERRS
		OBJ_ERRS 
		SEM_ERRS  
		SM_OBJ_ERRS
*/
	switch ( status ) {
		ERR(S_memLib_NOT_ENOUGH_MEMORY);
		ERR(S_msgQLib_INVALID_MSG_LENGTH);
		ERR(S_objLib_OBJ_DELETED);
		ERR(S_objLib_OBJ_ID_ERROR)
		ERR(S_objLib_OBJ_TIMEOUT);
		ERR(S_objLib_OBJ_UNAVAILABLE);
		ERR(S_semLib_INVALID_OPERATION);
		ERR(S_smObjLib_NOT_INITIALIZED);
		ERR(S_taskLib_ILLEGAL_PRIORITY);
		default:
			return strerror(status);
	}
}
