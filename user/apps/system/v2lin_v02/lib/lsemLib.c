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
#include "v2lpthread.h"
#include "vxw_defs.h"
#include "internal.h"
#include <assert.h>
#include "v2ldebug.h"
#include "vxw_hdrs.h"

#define SEM_OPT_MASK       0x0f
#define SEM_TYPE_MASK      0xf0

#define BINARY_SEMA4       0x00
#define MUTEX_SEMA4        0x10
#define COUNTING_SEMA4     0x20

#define SEND  0
#define FLUSH 1
#define KILLD 2

/*****************************************************************************
**  Control block for v2pthread semaphore
**
**  The basic POSIX semaphore does not provide for time-bounded waits nor
**  for selection of a thread to ready based either on FIFO or PRIORITY-based
**  waiting.  This 'wrapper' extends the POSIX pthreads semaphore to include
**  the attributes of a v2pthread semaphore.
**
*****************************************************************************/

static v2lsem_t *sem_list;

static pthread_mutex_t sem_list_lock = PTHREAD_MUTEX_INITIALIZER;

void semShow(v2lsem_t *s, FILE * out, int mem)
{
	task_t *t;
	fprintf(out,"o:%x ",s->current_owner);
	if (s->current_owner ) { 
		pthread_mutex_clean_lock(&task_list_lock);
		// look for the task t
		for (t = task_list; t; t = t->nxt_task) 
			if (t == s->current_owner) break;
		pthread_cleanup_pop(1);
		if (!t) // task not found
			fprintf(out,"<deleted> ");
		else fprintf(out,"%s ",s->current_owner->taskname);
	}
	fprintf(out,"rl:%i ",s->recursion_level);
	fprintf(out,"tc:%i ",s->token_count);
	fprintf(out,"(");
	for ( t = s->first_susp; t; t = t->nxt_susp ) {
		fprintf(out,"%x ",t);
		fprintf(out,"%s ",t->taskname);
	}
	fprintf(out,") ");
	fprintf(out,"%x ",s->send_type);

	if ( mem ) {
		int * i;
		for ( i = (int*)s; i < (int*) (s+1);i++) 
			fprintf(out,"%x ",*i);
	}
	fprintf(out,"\n");
}


int semList(FILE * out, int mem)
{
	int c=0;
	TRACEF();
	v2lsem_t *s = sem_list;
	while (s) {
		c++;
		fprintf(out,"%i %x: ",c,s);
		semShow(s,out,mem);
		s=s->nxt_sem;
	}
	return c;
}


/*****************************************************************************
**  sem_find_lock - verifies whether the specified semaphore still exists, and if
**                so, locks exclusive access to the semaphore for the caller.
*****************************************************************************/
static int sem_find_lock(v2lsem_t * sem)
{

	v2lsem_t *current_smcb;

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &sem_list_lock);
	pthread_mutex_lock(&sem_list_lock);

	for (current_smcb = sem_list; current_smcb != NULL; current_smcb = current_smcb->nxt_sem) {
		if (current_smcb == sem) {
			/*
			 ** Lock mutex for semaphore access (it is assumed that a
			 ** 'pthread_cleanup_push()' has already been performed
			 **  by the caller in case of unexpected thread termination.)
			 */
			pthread_mutex_lock(&(sem->sem_lock));
			break;
		}
	}

	pthread_mutex_unlock(&sem_list_lock);
	pthread_cleanup_pop(0);

	return current_smcb != NULL;
}


/*****************************************************************************
** link_smcb - appends a new semaphore control block pointer to the sem_list
*****************************************************************************/
static void link_smcb(v2lsem_t * new_sem)
{
	v2lsem_t **i;

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &sem_list_lock);
	pthread_mutex_lock(&sem_list_lock);

	new_sem->nxt_sem = (v2lsem_t *) NULL;
	i = &sem_list;
	while (*i) i =& (*i)->nxt_sem;
	*i=new_sem;
	pthread_mutex_unlock(&sem_list_lock);
	pthread_cleanup_pop(0);
}

/*****************************************************************************
** unlink_smcb - removes a semaphore control block pointer from the sem_list
*****************************************************************************/
static void unlink_smcb(v2lsem_t * smid)
{
	v2lsem_t **i =  & sem_list;

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &sem_list_lock);
	pthread_mutex_lock(&sem_list_lock);
	while (*i) {
		if ( *i == smid ) {
			TRACEF("%p", smid);
			*i = (*i)->nxt_sem;	// remove
			break;
		}
		i = &((*i)->nxt_sem);
	}
	pthread_mutex_unlock(&sem_list_lock);
	pthread_cleanup_pop(0);
}

/*****************************************************************************
** new_sem - creates a new v2pthread semaphore using pthreads resources
*****************************************************************************/
v2lsem_t *new_sem(int count)
{
	v2lsem_t *semaphore;

	semaphore = (v2lsem_t *) ts_malloc(sizeof(v2lsem_t));
	if (! semaphore ) {
		return NULL;
	}
	memset(semaphore,0,sizeof(*semaphore));
	semaphore->token_count = count;

	/*
	 ** Mutex and Condition variable for semaphore send/pend
	 */
	pthread_mutex_init(&semaphore->sem_lock, NULL);
	pthread_cond_init(&semaphore->sem_send, NULL);

	pthread_mutex_init(&semaphore->smdel_lock, NULL);
	pthread_cond_init(&semaphore->smdel_cplt, NULL);

	semaphore->send_type = SEND;
	semaphore->recursion_level = 0;
	semaphore->current_owner = NULL;
	semaphore->first_susp =  NULL;

	return semaphore;
}

/*****************************************************************************
** semBCreate - creates a v2pthread binary semaphore
*****************************************************************************/
v2lsem_t *semBCreate(int opt, int initial_state)
{
	v2lsem_t *semaphore;

	if ((opt & SEM_Q_PRIORITY)
			|| (opt & SEM_DELETE_SAFE)
			|| (opt & SEM_INVERSION_SAFE)) {
		errno = ENOSYS;
		return (NULL);
	}

	if (initial_state == 0)
		semaphore = new_sem(0);
	else
		semaphore = new_sem(1);

	if (! semaphore ) return NULL;
	TRACEF("%x %#x %x", semaphore,opt,initial_state);

	semaphore->flags = (opt & SEM_Q_PRIORITY) | BINARY_SEMA4;

	link_smcb(semaphore);
	return semaphore;
}

/*****************************************************************************
** semCCreate - creates a v2pthread counting semaphore
*****************************************************************************/
v2lsem_t *semCCreate(int opt, int initial_count)
{
	v2lsem_t *semaphore;

	if ((opt & SEM_Q_PRIORITY)
			|| (opt & SEM_DELETE_SAFE)
			|| (opt & SEM_INVERSION_SAFE)) {
		errno = ENOSYS;
		return (NULL);
	}

	semaphore = new_sem(initial_count);

	if (! semaphore ) return NULL;
	TRACEF("Creating counting semaphore - id %p", semaphore);

	semaphore->flags = (opt & SEM_Q_PRIORITY) | COUNTING_SEMA4;

	link_smcb(semaphore);

	return semaphore;
}

/*****************************************************************************
** semMCreate - creates a v2pthread mutual exclusion semaphore
*****************************************************************************/
v2lsem_t *semMCreate(int opt)
{
	v2lsem_t *semaphore;

	if ((opt & SEM_Q_PRIORITY)
			|| (opt & SEM_DELETE_SAFE)
			|| (opt & SEM_INVERSION_SAFE)) {
		TRACEF("WARNING not implemented");
		errno = ENOSYS;
		return (NULL);
	}
	semaphore = new_sem(1);

	if ( ! semaphore ) return NULL;
	TRACEF("Creating mutex semaphore - id %p", semaphore);

	semaphore->flags = (opt & SEM_OPT_MASK) | MUTEX_SEMA4;

	link_smcb(semaphore);

	return semaphore;
}

/*****************************************************************************
** semDelete - removes the specified semaphore from the semaphore list and
**             frees the memory allocated for the semaphore control block.
*****************************************************************************/
STATUS semDelete(v2lsem_t * semaphore)
{
	STATUS error = OK;

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &semaphore->sem_lock);
	if (! sem_find_lock(semaphore)) {
		error = S_objLib_OBJ_ID_ERROR;	/* Invalid semaphore specified */
		goto exit;
	}
	TRACEF("task @ %p delete semaphore @ %p", my_task(), semaphore);
	//  Send signal and block while any tasks are still waiting on the semaphore
	taskLock();
	if (semaphore->first_susp !=  NULL) {
		TRACEF("isemDelete - tasks pending on semaphore list @ %p", semaphore->first_susp);
		// Lock mutex for semaphore delete completion
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &semaphore->smdel_lock);
		pthread_mutex_lock(&(semaphore->smdel_lock));

		semaphore->send_type = KILLD;

		pthread_cond_broadcast(&(semaphore->sem_send));

		pthread_mutex_unlock(&(semaphore->sem_lock));

		/*
		 **  Wait for all pended tasks to receive delete message.
		 **  The last task to receive the message will signal the
		 **  delete-complete condition variable.
		 */
		while (semaphore->first_susp != (task_t *) NULL)
			pthread_cond_wait(&(semaphore->smdel_cplt), &(semaphore->smdel_lock));

		pthread_cleanup_pop(0);
	}

	unlink_smcb(semaphore);
	ts_free((void *) semaphore);
	taskUnlock();

	exit:
	{}
	pthread_cleanup_pop(0);

	if (error != OK) {
		errno = (int) error;
		error = ERROR;
	}
	return error;
}

/*****************************************************************************
** semFlush - unblocks all tasks waiting on the specified semaphore
*****************************************************************************/
STATUS semFlush(v2lsem_t * semaphore)
{
	STATUS error = OK;
	TRACEF("%x",semaphore);
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &semaphore->sem_lock);
	if (!sem_find_lock(semaphore)) {
		error = S_objLib_OBJ_ID_ERROR;	/* Invalid semaphore specified */
		goto exit;
	}
	if ((semaphore->flags & SEM_TYPE_MASK) == MUTEX_SEMA4) {
		error = S_semLib_INVALID_OPERATION;	/* Flush invalid for mutex */
		pthread_mutex_unlock(&semaphore->sem_lock);
		goto exit;
	}
	TRACEF("task @ %p flush semaphore list @ %p", my_task(), &(semaphore->first_susp));
	/*
	 **  Send signal and block while any tasks are still waiting
	 **  on the semaphore
	 */
	taskLock();
	if (semaphore->first_susp ) {
		// Lock mutex for semaphore delete completion
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &semaphore->smdel_lock);
		pthread_mutex_lock(&(semaphore->smdel_lock));

		semaphore->send_type = FLUSH;
		pthread_cond_broadcast(&semaphore->sem_send);
		pthread_mutex_unlock(&semaphore->sem_lock);

		/*
		 **  Wait for all pended tasks to receive delete message.
		 **  The last task to receive the message will signal the
		 **  delete-complete condition variable.
		 */
		while (semaphore->first_susp != NULL)
			pthread_cond_wait(&(semaphore->smdel_cplt), &(semaphore->smdel_lock));

		pthread_mutex_unlock(&(semaphore->smdel_lock));
		pthread_cleanup_pop(0);
	}
	// TODO ??? else pthread_mutex_unlock(&(semaphore->sem_lock));
	taskUnlock();
exit:
	{}
	pthread_cleanup_pop(0);

	if (error != OK) {
		errno = (int) error;
		error = ERROR;
	}

	return error;
}

/*****************************************************************************
** semGive - releases a v2pthread semaphore token and awakens the first
**           selected task waiting on the semaphore.
*****************************************************************************/
STATUS semGive(v2lsem_t * semaphore)
{
	task_t *our_task  = my_task();
	STATUS error = OK;

	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &semaphore->sem_lock);
	if (! sem_find_lock(semaphore)) {
		TRACEF("WARNING not found %x",semaphore);
		error = S_objLib_OBJ_ID_ERROR;
		goto exit;
	}
	TRACEF("%x %i %i",semaphore,semaphore->recursion_level, semaphore->token_count);
	//  If semaphore is a mutex, make sure we own it before giving up the token.
	if (((semaphore->flags & SEM_TYPE_MASK) == MUTEX_SEMA4) &&
			(semaphore->current_owner != our_task)) {
		error = S_semLib_INVALID_OPERATION;	
		goto unlock;
	}
	/*
	 **  Either semaphore isn't a mutex or we currently own the mutex.
	 **  If semaphore is a mutex, recursion level should be > zero.
	 **  In this case, decrement recursion level, and if level == zero
	 **  after decrement, relinquish mutex ownership.
	 */
	//TRACEF("Semaphore list @ %p recursion level = %d", &semaphore->first_susp, semaphore->recursion_level);
	if (semaphore->recursion_level > 0) {
		--semaphore->recursion_level;
		if ( ! semaphore->recursion_level ) {
			semaphore->token_count++;
			TRACEF("--rl=%i ++tc=%i",semaphore->recursion_level,semaphore->token_count);
			semaphore->current_owner =  NULL;
			if (semaphore->flags & SEM_DELETE_SAFE)
				// Task was made deletion-safe when mutex acquired...  Remove deletion safety now.
				taskUnsafe();
			if (semaphore->flags & SEM_INVERSION_SAFE) {
				/*
				 **  Task priority may have been boosted during
				 **  ownership of semaphore...
				 **  Restore to original priority now.  Call taskLock
				 **  so taskUnlock can be called to restore priority.
				 */
				TRACEF("Restoring task priority for owner of semaphore list @ %p",
						&(semaphore->first_susp));
				taskLock();
				taskUnlock();
			}
		}
		// else semaphore->recursion_level > 0
	} else  {
		++semaphore->token_count;
		TRACEF("++tc=%i",semaphore->token_count);
	}

	if (semaphore->first_susp) {
		TRACEF("%x post to semaphore list @ %x", our_task, semaphore->first_susp);
		pthread_cond_broadcast(&semaphore->sem_send);
	}
unlock:
	pthread_mutex_unlock(&semaphore->sem_lock);
	TRACEF("%x %i %i",semaphore,semaphore->recursion_level, semaphore->token_count);
exit:
	{}
	pthread_cleanup_pop(0);

	if (error != OK) {
		errno = (int) error;
		error = ERROR;
	}
	TRACEF("%x",semaphore);
	return error;
}

BOOL time_expired(struct timespec *timeout)
{
	struct timeval now;
	ulong usec;
	gettimeofday(&now, NULL);
	if (timeout->tv_sec < now.tv_sec)
		return TRUE;
	if (timeout->tv_sec > now.tv_sec)
		return FALSE;
	assert(timeout->tv_sec == now.tv_sec);
	return timeout->tv_nsec <= now.tv_usec * 1000;
}

/*****************************************************************************
** waiting_on_sem - returns a nonzero result unless a qualifying event
**                    occurs on the specified semaphore which should cause the
**                    pended task to be awakened.  The qualifying events
**                    are:
**                        (1) a token is returned to the semaphore and the
**                            current task is selected to receive it
**                        (2) the semaphore is deleted or flushed
*****************************************************************************/
static int waiting_on_sem(v2lsem_t * semaphore, struct timespec *timeout, int *retcode)
{
	int result;
	TRACEF("%x %i", semaphore, semaphore->token_count);
	if ((semaphore->send_type & KILLD) || (semaphore->send_type & FLUSH)) {
		//  Semaphore has been killed... waiting is over.
		result = 0;
		*retcode = 0;
		goto exit;
	}
	// Semaphore still in service... check for token availability.
	//  Initially assume no token available for our task
	result = 1;

	/*  Multiple posts to the semaphore may be represented by only
	 **  a single signal to the condition variable, so continue
	 **  checking for a token for our task as long as more tokens
	 **  are available. */
	while (semaphore->token_count > 0) {
		//  Available token arrived... see if it's for our task.
		if ((signal_for_my_task(&semaphore->first_susp, (semaphore->flags & SEM_Q_PRIORITY)))) {
			/**  Token was destined for our task specifically...
			 **  waiting is over.  	 */
			semaphore->token_count--;
			result = 0;
			*retcode = 0;
			break;
		} else {
			/*
			 **  Token isn't for our task...  Sleep awhile to
			 **  allow other tasks ahead of ours in the queue of tasks
			 **  waiting on the semaphore to get their tokens, bringing
			 **  our task to the head of the list.
			 */
			pthread_mutex_unlock(&(semaphore->sem_lock));
			taskDelay(1);
			pthread_mutex_lock(&(semaphore->sem_lock));
		}

		if ( timeout && time_expired (timeout))
				break;
	}
exit:
	return result;
}

void priority_inversion(v2lsem_t * semaphore, int max_wait, task_t * our_task)
{
	task_t *task;
	int my_priority, owners_priority, sched_policy;
	taskLock();

	my_priority = our_task->prv_priority.sched_priority;

	task = semaphore->current_owner;
	owners_priority = task->prv_priority.sched_priority;

	TRACEF("Task @ %p priority %d owns mutex", task, owners_priority);
	TRACEF("Calling task @ %p priority %d wants mutex", our_task, my_priority);
	/*
	 **  If mutex owner's priority is lower than ours, boost it
	 **  to our priority level tempororily until owner releases mutex.
	 **  This avoids 'priority inversion'.
	 */
	if (owners_priority < my_priority) {
		struct sched_param schedparam;
		pthread_attr_getschedpolicy(&task->attr, &sched_policy);
		pthread_attr_getschedparam(&task->attr, &schedparam);
		schedparam.sched_priority = my_priority;
		pthread_attr_setschedparam(&task->attr, &schedparam);
		pthread_setschedparam(task->pthrid, sched_policy, &schedparam);
	}

	taskUnlock();

}
/*****************************************************************************
** wait_for_token - blocks the calling task until a token is available on the
**                  specified v2pthread semaphore.  If a token is acquired and
**                  the semaphore is a mutex type, this function also handles
**                  priority inversion and deletion safety issues as needed.
*****************************************************************************/
STATUS wait_for_token(v2lsem_t * semaphore, int max_wait, task_t * our_task)
{
	struct timeval now;
	struct timespec timeout;
	int retcode = 0;
	long sec, usec;
	STATUS error = OK;

	TRACEF("%x", our_task);
	link_susp_task(&semaphore->first_susp, our_task);

	if (max_wait == NO_WAIT) {
		//  Caller specified no wait on semaphore token...
		//  Check the condition variable with an immediate timeout.
		gettimeofday(&now, (struct timezone *) NULL);
		timeout.tv_sec = now.tv_sec;
		timeout.tv_nsec = now.tv_usec * 1000;
		while ((waiting_on_sem(semaphore, &timeout, &retcode)) && (retcode != ETIMEDOUT)) {
			TRACEF("pthread_cond_timedwait {");
			retcode = pthread_cond_timedwait(&semaphore->sem_send, &semaphore->sem_lock, &timeout);
			TRACEF("pthread_cond_timedwait }");
		}
		if (retcode &&  retcode != ETIMEDOUT) {
			error = S_objLib_OBJ_UNAVAILABLE;
			TRACEF("...no token available");
		}
	} else {
		//  Caller expects to wait on semaphore, with or without a timeout.
		//  If the semaphore is a mutex type, check for and handle any
		//  priority inversion issues.
		if ((semaphore->flags & SEM_INVERSION_SAFE) && semaphore->current_owner ) {
			priority_inversion(semaphore, max_wait, our_task);
		}

		if (max_wait == WAIT_FOREVER) {
			while (waiting_on_sem(semaphore, 0, &retcode)) {
				TRACEF("pthread_cond_timedwait {");
				pthread_cond_wait(&semaphore->sem_send, &semaphore->sem_lock);
				TRACEF("pthread_cond_timedwait }");
			}
		} else {
			TRACEF("with timeout %i",max_wait);
			/*
			 **  Wait on semaphore message arrival with timeout...
			 **  Calculate timeout delay in seconds and microseconds.
			 */
			sec = 0;
			usec = max_wait * V2PT_TICK * 1000;
			gettimeofday(&now, (struct timezone *) NULL);
			usec += now.tv_usec;
			if (usec > 1000000) {
				sec = usec / 1000000;
				usec = usec % 1000000;
			}
			timeout.tv_sec = now.tv_sec + sec;
			timeout.tv_nsec = usec * 1000;

			/*
			 **  Wait for a semaphore message for the current task or
			 **  for the timeout to expire.  The loop is required since
			 **  the task may be awakened by signals for semaphore
			 **  tokens which are not ours, or for signals other than
			 **  from a semaphore token being returned.
			 */
			while ((waiting_on_sem(semaphore, &timeout, &retcode)) && (retcode != ETIMEDOUT)) {
				TRACEF("pthread_cond_timedwait {");
				retcode = pthread_cond_timedwait(&(semaphore->sem_send),
						&(semaphore->sem_lock), &timeout);
				TRACEF("pthread_cond_timedwait }");
			}
		}
		if (retcode == ETIMEDOUT) {
			error = S_objLib_OBJ_TIMEOUT;
			TRACEF("...timed out");
		}
	}

	/*
	 **  Remove the calling task's task from the waiting task list
	 **  for the semaphore.  Clear our TCB's suspend list pointer in
	 **  case the semaphore was killed & its ctrl blk deallocated.
	 */
	if (our_task) {
		TRACEF("%x", our_task);
		unlink_susp_task(&semaphore->first_susp, our_task);
		//our_task->suspend_list = NULL;
	}
	//  See if we were awakened due to a semDelete or a semFlush.
	if ((semaphore->send_type & KILLD) || (semaphore->send_type & FLUSH)) {
		if (semaphore->send_type & KILLD) {
			error = S_objLib_OBJ_ID_ERROR;	// Semaphore deleted 
			TRACEF("...semaphore deleted");
		} else {
			TRACEF("...semaphore flushed");
		}

		if ( ! semaphore->first_susp ) {
			pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &semaphore->smdel_lock);
			pthread_mutex_lock(&semaphore->smdel_lock);
			pthread_cond_broadcast(&semaphore->smdel_cplt);
			semaphore->send_type = SEND;
			pthread_cleanup_pop(1);
		}
		return error;
	} 
	if ( retcode == OK) {
		/*
		 **  Just received a token from the semaphore...
		 **  If the semaphore is a mutex, indicate the mutex is now owned
		 **  by the current task, and then see if the task owning the
		 **  token is to be made deletion-safe.  
		 */
		semaphore->current_owner = our_task;
		if ((semaphore->flags & SEM_TYPE_MASK) == MUTEX_SEMA4) {
			semaphore->current_owner = our_task;
			semaphore->recursion_level++; // is it a good place?
			if (semaphore->flags & SEM_DELETE_SAFE)
				taskSafe();
		}
		TRACEF("OK");
	}
	return error;
}

/*****************************************************************************
** semTake - blocks the calling task until a token is available on the
**           specified v2pthread semaphore.
*****************************************************************************/
STATUS semTake(v2lsem_t * semaphore, int max_wait)
{
	task_t *our_task;
	STATUS error = OK;
	TRACEF();
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &semaphore->sem_lock);
	if (!sem_find_lock(semaphore)) {
		TRACEF("WARNING not found %x ",semaphore);
		error = S_objLib_OBJ_ID_ERROR;
		goto exit;

	}
	//TRACEF("%i %x %i %i",max_wait, semaphore,semaphore->recursion_level, semaphore->token_count);
	// If the semaphore is a mutex, check to see if this task already owns the token.
	our_task = my_task();
	if ( our_task ) our_task->waiting = semaphore;
	if (((semaphore->flags & SEM_TYPE_MASK) == MUTEX_SEMA4) &&
			(semaphore->current_owner == our_task)) {
		// TODO: why not just use pthread_mutex_lock with PTHREAD_MUTEX_RECURSIVE mutex ?
		// because of timeout? -> pthread_cond_timedwait

		//  Current task already owns the mutex... 
		// simply increment the  ownership recursion level and return.
		semaphore->recursion_level++; // is i a good place?
		//TRACEF("... recursion level = %d", semaphore->recursion_level);
	} else {
		// Either semaphore is not a mutex or current task doesn't own it
		// Wait for timeout or acquisition of token
		error = wait_for_token(semaphore, max_wait, our_task);
	}
	if ( our_task ) our_task->waiting = NULL;
	if ( error == OK ) {
		//++semaphore->recursion_level; // alternate place
		// TRACEF("++rl  = %i", semaphore->recursion_level);
	}
	pthread_mutex_unlock(&(semaphore->sem_lock));
	TRACEF("%x %i %i", semaphore,semaphore->recursion_level, semaphore->token_count);
exit:
	{}
	pthread_cleanup_pop(0);

	if (error != OK) {
		errno = (int) error;
		error = ERROR;
	}
	return error;
}

