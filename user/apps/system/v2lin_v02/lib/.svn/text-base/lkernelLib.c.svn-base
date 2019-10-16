
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
**  process_timer_list is a system function used to service watchdog timers
**                     when a system clock tick expires.  It is called from
**                     the system exception task once per clock tick.
*/
extern void process_timer_list(void);

/*
**  Task control blocks for the v2pthread system tasks.
*/
static task_t root_task;
static task_t excp_task;

static unsigned char round_robin_enabled = 0;

void disableRoundRobin(void)
{
	round_robin_enabled = 0;
}

void enableRoundRobin(void)
{
	round_robin_enabled = 1;
}

BOOL roundRobinIsEnabled(void)
{
	return ((BOOL) round_robin_enabled);
}

/*****************************************************************************
** kernelTimeSlice - turns Round-Robin Timeslicing on or off in the scheduler
*****************************************************************************/
STATUS kernelTimeSlice(int ticks_per_quantum) 
{
	task_t *task;
	int sched_policy;
	TRACEF("%i",ticks_per_quantum);

	taskLock();

	/*
	 **  Linux doesn't allow the round-robin quantum to be changed, so
	 **  we only use the ticks_per_quantum as an on/off value for
	 **  round-robin scheduling.
	 */
	if (ticks_per_quantum == 0) {
		/*
		 **  Ensure Round-Robin Timeslicing is OFF for all tasks, both
		 **  existing and yet to be created.
		 */
		round_robin_enabled = 0;
		sched_policy = SCHED_FIFO;
	} else {
		/*
		 **  Ensure Round-Robin Timeslicing is ON for all tasks, both
		 **  existing and yet to be created.
		 */
		round_robin_enabled = 1;
		sched_policy = SCHED_RR;
	}

	struct sched_param schedparam;
	//  Change the scheduling policy for all tasks in the task list.
	for (task = task_list; task ; task = task->nxt_task) {

		/*
		 **  First set the new scheduling policy attribute.  Since the
		 **  max priorities are identical under Linux for both real time
		 **  scheduling policies, we know we don't have to change priority.
		 */
		pthread_attr_setschedpolicy(&task->attr, sched_policy);

		pthread_attr_getschedparam(&task->attr, &schedparam);
		//  Activate the new scheduling policy
		CHK0(pthread_setschedparam(task->pthrid, sched_policy, &schedparam));
		// { perror("kernelTimeSlice pthread_setschedparam returned error:"); }
	}

	taskUnlock();

	return (OK);
}

/*****************************************************************************
**  system exception task
**
**  In the v2pthreads environment, the exception task serves only to
**  handle watchdog timer functions and to allow self-restarting of other
**  v2pthread tasks.
*****************************************************************************/
int exception_task(int dummy0, int dummy1, int dummy2, int dummy3,
				   int dummy4, int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	TRACEF("%x",exception_task);
	while (1) {
		/*
		 **  Process system watchdog timers (if any are defined).
		 **  NOTE that since ALL timers must be handled during a single
		 **  10 msec system clock tick, timers should be used sparingly.
		 **  In addition, the timeout functions called by watchdog timers
		 **  should be "short and sweet".
		 */
		process_timer_list();

		/*
		 **  Delay for one timer tick.  Since this is the highest-priority
		 **  task in the v2pthreads virtual machine (except for the root task,
		 **  which stays blocked almost all the time), any processing done
		 **  in this task can impose a heavy load on the remaining tasks.
		 **  For this reason, this task and all watchdog timeout functions
		 **  should be kept as brief as possible.
		 */
		taskDelay(1);
	}
	return 0;
}

/*****************************************************************************
**  v2pthread main program
**
**  This function serves as the entry point to the v2pthreads emulation
**  environment.  It serves as the parent process to all v2pthread tasks.
**  This process creates an initialization thread and sets the priority of
**  that thread to the highest allowable value.  This allows the initialization
**  thread to complete its work without being preempted by any of the task
**  threads it creates.
*****************************************************************************/

int v2lin_init(void)
{
	int max_priority;
	TRACEF();
	// Start system root task
	taskInit(&root_task, "tUsrRoot", 0, 0, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	max_priority = sched_get_priority_max(SCHED_FIFO);
	(root_task.prv_priority).sched_priority = max_priority;
	pthread_attr_setschedparam(&(root_task.attr), &(root_task.prv_priority));
	root_task.state = READY;
	root_task.pthrid = pthread_self();
	taskActivate(root_task.taskid);

	// Start system exception task.
	taskInit(&excp_task, "tExcTask", 0, 0, 0, 0, exception_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	max_priority = sched_get_priority_max(SCHED_FIFO);
	(excp_task.prv_priority).sched_priority = max_priority - 1;
	pthread_attr_setschedparam(&(excp_task.attr), &(excp_task.prv_priority));

	taskActivate(excp_task.taskid);

	return 0;
}
