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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/time.h>
#include "internal.h"


extern void *ts_malloc(size_t blksize);
extern void ts_free(void *blkaddr);
extern task_t *task_for(int taskid);
void *task_wrapper(void *arg);

static v2pt_wdog_t *wdog_list;

static pthread_mutex_t wdog_list_lock = PTHREAD_MUTEX_INITIALIZER;

/*****************************************************************************
**  wdog_valid - verifies whether the specified watchdog still exists, and if
**                so, locks exclusive access to the watchdog for the caller.
*****************************************************************************/
static int wdog_valid(v2pt_wdog_t * wdId)
{

	v2pt_wdog_t *current_wdog;
	int found_wdog;

	found_wdog = FALSE;
#ifdef TRACE
	printf("looking for watchdog @ %p\n", wdId);
#endif

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &wdog_list_lock);
	pthread_mutex_lock(&wdog_list_lock);

	for (current_wdog = wdog_list;
		 current_wdog != (v2pt_wdog_t *) NULL; current_wdog = current_wdog->next) {
#ifdef TRACE
		printf("checking watchdog @ %p", current_wdog);
#endif
		if (current_wdog == wdId) {
			/*
			 ** Lock mutex for watchdog access (it is assumed that a
			 ** 'pthread_cleanup_push()' has already been performed
			 **  by the caller in case of unexpected thread termination.)
			 */
			pthread_mutex_lock(&(wdId->wdog_lock));

			found_wdog = TRUE;
			break;
		}
	}

	pthread_cleanup_pop(1);

	return found_wdog;
}

/*****************************************************************************
** link_wdog - appends a new watchdog control block pointer to the wdog_list
*****************************************************************************/
static void link_wdog(v2pt_wdog_t * new_wdog)
{
	v2pt_wdog_t **i = &wdog_list;

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &wdog_list_lock);
	pthread_mutex_lock(&wdog_list_lock);

	new_wdog->next = NULL;
	while (*i)
		i = &((*i)->next);	// look for tail
	*i = new_wdog;
#if 0
	v2pt_wdog_t *current_wdog;
	if (wdog_list != (v2pt_wdog_t *) NULL) {
		/*
		 **  One or more watchdogs already exist in the watchdog list...
		 **  Insert the new entry at the tail of the list.
		 */
		for (current_wdog = wdog_list;
			 current_wdog->next != (v2pt_wdog_t *) NULL; current_wdog = current_wdog->next);
		current_wdog->next = new_wdog;
#ifdef TRACE
		printf("\r\nadd watchdog cb @ %p to list @ %p", new_wdog, current_wdog);
#endif
	} else {
		/*
		 **  this is the first watchdog being added to the watchdog list.
		 */
		wdog_list = new_wdog;
#ifdef TRACE
		printf("\r\nadd watchdog cb @ %p to list @ %p", new_wdog, &wdog_list);
#endif
	}
#endif
	pthread_cleanup_pop(1);
}

/*****************************************************************************
** unlink_wdog - removes a watchdog control block pointer from the wdog_list
*****************************************************************************/
static v2pt_wdog_t *unlink_wdog(v2pt_wdog_t * wdId)
{
	v2pt_wdog_t **i = &wdog_list;
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &wdog_list_lock);
	pthread_mutex_lock(&wdog_list_lock);
	while (*i) {
		if ( *i == wdId) {
			TRACEF("removing watchdog cb @ %p\n", wdId);
			*i = (*i)->next;	// remove
			break;
		}
		i = &((*i)->next);
	}
	pthread_cleanup_pop(1);
#if 0
	O, my god!
	v2pt_wdog_t *current_wdog;
	v2pt_wdog_t *selected_wdog;

	selected_wdog = (v2pt_wdog_t *) NULL;

	if (wdog_list != (v2pt_wdog_t *) NULL) {
		/*
		 **  One or more watchdogs exist in the watchdog list...
		 **  Protect the watchdog list while we examine and modify it.
		 */
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &wdog_list_lock);
		pthread_mutex_lock(&wdog_list_lock);

		/*
		 **  Scan the watchdog list for a wdog with a matching watchdog ID
		 */
		if (wdog_list == wdId) {
			/*
			 **  The first watchdog in the list matches the selected watchdog ID
			 */
			selected_wdog = wdog_list;
			wdog_list = selected_wdog->next;
#ifdef TRACE
			printf("\r\ndel watchdog cb @ %p from list @ %p", selected_wdog, &wdog_list);
#endif
		} else {
			/*
			 **  Scan the next wdog for a matching wdId while retaining a
			 **  pointer to the current wdog.  If the next wdog matches,
			 **  select it and then unlink it from the watchdog list.
			 */
			for (current_wdog = wdog_list;
				 current_wdog->next != (v2pt_wdog_t *) NULL;
				 current_wdog = current_wdog->next) {
				if (current_wdog->next == wdId) {
					/*
					 **  Queue ID of next wdog matches...
					 **  Select the wdog and then unlink it by linking
					 **  the selected wdog's next wdog into the current wdog.
					 */
					selected_wdog = current_wdog->next;
					current_wdog->next = selected_wdog->next;
#ifdef TRACE
					printf("\r\ndel watchdog cb @ %p from list @ %p", selected_wdog, current_wdog);
#endif
					break;
				}
			}
		}

		/*
		 **  Re-enable access to the watchdog list by other threads.
		 */
		pthread_mutex_unlock(&wdog_list_lock);
		pthread_cleanup_pop(0);
	}
#endif
	return wdId;
}

/*****************************************************************************
** process_tick_for - performs service on the specified watchdog timer after
**                    a system timer tick has elapsed.  Returns the address
**                    of the next watchdog timer in the timer list, or NULL
**                    if no further timers remain to  be processed
*****************************************************************************/
static v2pt_wdog_t *process_tick_for(v2pt_wdog_t * wdId)
{
	v2pt_wdog_t *next;

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &wdId->wdog_lock);
	if (wdog_valid(wdId)) {
		// Mark the next watchdog timer (if any) in the timer list.
		next = wdId->next;
		//TRACEF("%x %i", wdId, wdId->ticks_remaining);
		if (wdId->ticks_remaining > 0) {
			//  Decrement ticks_remaining, and if this was the last tick, check for a timeout handler function.
			wdId->ticks_remaining--;
			if ( ! wdId->ticks_remaining && wdId->timeout_func ) {
				/*  Unlock the queue mutex so the timeout handler can call wdStart if desired.  
				This would create a race condition, but: 
				1) the task executing this function cannot be preempted at this point by other v2pthread tasks, and
				2) the timer list itself is locked at this point. 
				*/
				pthread_mutex_unlock(&wdId->wdog_lock);

				//TRACEF(" %x calling function @ %x", wdId, wdId->timeout_func);
				(*wdId->timeout_func)(wdId->timeout_parm);
			} else
				pthread_mutex_unlock(&wdId->wdog_lock);
		} else {
			pthread_mutex_unlock(&wdId->wdog_lock);
		}
	} else {
		// Invalid watchdog timer specified
		next = NULL;
		TRACEF("nwatchdog @ %p is invalid\n", wdId);
	}
	pthread_cleanup_pop(0);
	return next;
}

/*****************************************************************************
** process_timer_list - performs service on all active watchdog timers after
**                      a system timer tick has elapsed.
*****************************************************************************/
void process_timer_list(void)
{
	v2pt_wdog_t *next;

	for (next = wdog_list; next;) {
		//TRACEF(next);
		next = process_tick_for(next); // complexity is O^2 
	}
}

/*****************************************************************************
** wdCancel - De-activates an existing watchdog timer, but does not remove it
**            from the timer list.  The same watchdog may be re-started or
**            re-used for other timer purposes.
*****************************************************************************/
STATUS wdCancel(WDOG_ID wdId)
{
	STATUS error = OK;

	// First ensure that the specified watchdog exists and that we have exclusive access to it.
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &wdId->wdog_lock);
	if (wdog_valid(wdId)) {

		// Zero the ticks remaining without executing the timeout function.
		wdId->ticks_remaining = 0;
		pthread_mutex_unlock(&(wdId->wdog_lock));
	} else {
		errno =  S_objLib_OBJ_ID_ERROR;
		error = ERROR;
	}
	pthread_cleanup_pop(0);

	return error;
}

/*****************************************************************************
** wdCreate - Creates a new (inactive) watchdog timer.
*****************************************************************************/
v2pt_wdog_t *wdCreate(void)
{
	v2pt_wdog_t *new_wdog;

	//  Allocate memory for the watchdog timer control block.
	new_wdog = (v2pt_wdog_t *) ts_malloc(sizeof(v2pt_wdog_t));
	if ( new_wdog ) {
		pthread_mutex_init(&(new_wdog->wdog_lock), (pthread_mutexattr_t *) NULL);
		//  Zero ticks remaining until timeout so watchdog is inactive.
		new_wdog->ticks_remaining = 0;
		link_wdog(new_wdog);
	}
	return new_wdog;
}

/*****************************************************************************
** wdDelete - Removes te specified watchdog timer from the watchdog list
**            and deallocates the memory used by the watchdog.
*****************************************************************************/
STATUS wdDelete(v2pt_wdog_t * wdId)
{
	STATUS error  = OK;

	// First ensure that the specified watchdog exists and that we have exclusive access to it.
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &(wdId->wdog_lock));
	if (wdog_valid(wdId)) {
		unlink_wdog(wdId);
		pthread_mutex_unlock(&(wdId->wdog_lock));
		ts_free(wdId);
	} else {
		errno = S_objLib_OBJ_ID_ERROR;
		error = ERROR;
	}

	pthread_cleanup_pop(0);

	return (error);
}

/*****************************************************************************
** wdStart - Activates or reactivates an existing watchdog timer.
*****************************************************************************/
STATUS wdStart(v2pt_wdog_t * wdId, int delay, void (*f) (int), int parm)
{
	STATUS error = OK;
	TRACEF("%i",delay);
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &wdId->wdog_lock);
	if (wdog_valid(wdId)) {
		/*
		 ** Ticks remaining until timeout (zero if watchdog already expired).
		 ** (On an x86 Linux system, the minimum taskDelay appears to be
		 ** 20 msec - not 10 msec. TODO: check
		 */
		wdId->ticks_remaining = delay / 2;
		if (wdId->ticks_remaining < 1)
			wdId->ticks_remaining = 1;

		wdId->timeout_func = f;
		wdId->timeout_parm = parm;
		pthread_mutex_unlock(&wdId->wdog_lock);
	} else {
		errno = S_objLib_OBJ_ID_ERROR;
		error = ERROR;
	}
	pthread_cleanup_pop(0);
	return error;
}

/*****************************************************************************
** self_starter - called from a task-restart watchdog timer.  Starts a new
**                pthread for the specified task and deletes the watchdog.
*****************************************************************************/
static v2pt_wdog_t *restart_wdog;

static void self_starter(int taskid)
{
	task_t *task;

	/*
	 **  Delete the task restart watchdog regardless of the success or failure
	 **  of the restart operation.
	 */
	wdDelete(restart_wdog);

	/*
	 **  Get the address of the task control block for the specified task ID.
	 */
	task = task_for(taskid);
	if (task != (task_t *) NULL) {
		/*
		 **  Mark the task as ready to run and create a new pthread using the
		 **  attributes and entry point defined in the task control block.
		 */
		task->state = READY;
		task->pthrid = (pthread_t) NULL;
		pthread_create(&(task->pthrid), &(task->attr), task_wrapper, (void *) task);
	}
}

/*****************************************************************************
** selfRestart - restarts the specified (existing) v2pthread task after said
**               task has terminated itself.
*****************************************************************************/
void selfRestart(task_t * restart_task)
{
	/*
	 **  First create a watchdog timer to handle the restart operation.
	 */
	restart_wdog = wdCreate();

	if (restart_wdog != (v2pt_wdog_t *) NULL) {
		/*
		 **  Start the watchdog timer to expire after one tick and to call
		 **  a function which will first restart the specified task and
		 **  then delete the restart watchdog timer.
		 */
		wdStart(restart_wdog, 1, self_starter, restart_task->taskid);
	}
}

int wdogShow(FILE * out)
{
	v2pt_wdog_t *wd;
	int i =0;
	pthread_mutex_clean_lock(&wdog_list_lock);
	for (wd = wdog_list;  wd; wd = wd->next) {
		fprintf(out,"%x %i %i\n", wd->timeout_func, wd->ticks_remaining, wd->timeout_parm);
		i++;
	}
	pthread_cleanup_pop(1);
	return i;
}
