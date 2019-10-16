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

//
// This file implements defaulut int main() function and call user's user_sysinit and user_syskill functions
// To be compiled with uset's project which implements user_sysinit and user_syskill
//

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
#include "v2ldebug.h"

/*
**  user_sysinit is a user-defined function.  It contains all initialization
**               calls to create any tasks and other objects reqired for
**               startup of the user's RTOS system environment.  It is called
**               from (and runs in) the system root task context.
*/
extern void user_sysinit(void);

/*
**  user_syskill is a user-defined function.  It is called from the main
**               process context of the v2pthreads virtual machine.
**               At its simplest it is an unconditional infinite loop.
**               It may optionally wait for some condition, shut down the
**               user's RTOS system environment, clean up the resources used
**               by the various RTOS objects, and then return to the main
**               process of the v2pthreads virtual machine.
**               The v2pthreads virtual machine terminates upon its return.
*/
extern void user_syskill(void);

/*
**  Task control blocks for the v2pthread system tasks.
*/

/*****************************************************************************
**  system root task
**
**  In the v2pthreads environment, the root task serves only to
**  start the system exception task and to provide a context in which the
**  user_sysinit function may call any supported v2pthread function, even
**  though that function might block.
*****************************************************************************/
int exception_task(int dummy0, int dummy1, int dummy2, int dummy3,
				   int dummy4, int dummy5, int dummy6, int dummy7, int dummy8, int dummy9);
int root_task(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
			  int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	TRACEF("%x",root_task);
	task_t excp_task;
	taskInit(&excp_task, "tExcTask", 0, 0, 0, 0, exception_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	excp_task.prv_priority.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	pthread_attr_setschedparam(&(excp_task.attr), &(excp_task.prv_priority));
	taskActivate(excp_task.taskid);

	user_sysinit();

	while (1)
		taskDelay(500);

	return (0);
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
char ** _argv;
int exit_value=0;
int main(int argc, char **argv)
{
	TRACEF();
	_argv = argv;
	task_t rt; // root task

	taskInit(&rt, "tUsrRoot", 0, 0, 0, 0, root_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	rt.prv_priority.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_attr_setschedparam(&rt.attr, &rt.prv_priority);

	taskActivate(rt.taskid);
	errno = 0;
	// Wait for the return.
	user_syskill();
	exit(exit_value);
}
