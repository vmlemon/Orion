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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "v2lpthread.h"
#include "vxw_hdrs.h"
#include "test.h"
#include "v2ldebug.h"

//int test_cycle;
int wdog1_cycle;
int wdog2_cycle;


static WDOG_ID wdog1_id;
static WDOG_ID wdog2_id;

void t_250_msec(int dummy0)
{
	//TRACEF();
	CHK0(wdStart(wdog2_id, 25, t_250_msec, 0));
	wdog2_cycle += 250;
	CHK0(semGive(enable8));
}

void t_500_msec(int dummy0)
{
	//TRACEF();
	CHK0(wdStart(wdog1_id, 50, t_500_msec, 0));
	wdog1_cycle += 500;
	CHK0(semGive(enable5));
}

void t_1000_msec(int dummy0)
{
	TRACEF();
	wdog2_cycle += 1000;

	CHK0(semGive(enable8));
}

int task8_id;

int test_watchdog_timers(void)
{
	STATUS err;
	TRACEF();

	wdog1_cycle = 0;
	wdog2_cycle = 0;

	CHK(wdog1_id = wdCreate());
	CHK(wdog2_id = wdCreate());

	/************************************************************************
	 **  WatchDog Timer Start Test
	 ************************************************************************/
	//puts(".......... Next we start Watchdog 1 to time out after 1/2 second.");
	//puts("           Watchdog 1's timeout function will restart the timer");
	//puts("           for another half second each time it expires.");
	//puts("           Then we start Watchdog 2 with a single 1 second delay.");

	CHK0(wdStart(wdog1_id, 50, t_500_msec, 0));
	//puts("Starting Watchdog 2");
	CHK0(wdStart(wdog2_id, 25, t_1000_msec, 0));
	//puts("Task 1 sleeping for 2.5 seconds while timers run.");
	CHK0(taskDelay(250));

	//  WatchDog Timer Cancellation/Restart Test
	//puts(".......... Next we cancel Watchdog 1 and restart Watchdog 2");
	//puts("           with a repeating 250 msec timeout function.");

	//puts("Cancelling Watchdog 1");
	CHK0(wdCancel(wdog1_id));
	CHK0(wdStart(wdog2_id, 25, t_250_msec, 0));
	//puts("Task 1 sleeping for 1.5 seconds while timers run.");
	CHK0(taskDelay(150));

	/************************************************************************
	 **  WatchDog Timer Deletion Test
	 ************************************************************************/
	//puts(".......... Finally we delete both Watchdog 1 and Watchdog 2");

	CHK0(wdDelete(wdog1_id));
	//puts("Deleting Watchdog 2");
	CHK0(wdDelete(wdog2_id));
	//puts("Task 1 sleeping for 1 second to show timers not running.");
	CHK0(taskDelay(100));
	CHK(0 == wdogShow(stderr));
	TRACEF("finished");
	return OK;
}


int task5(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
		  int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	STATUS err;
	int i;
	task_t *task;
	TRACEF();

	while (!task5_restarted)
		taskDelay(1);

	task = task_for(task5_id);

	if (task5_restarted == 1) {
		task5_restarted++;
		CHK0(taskRestart(0));
	}
	// else puts("Task 5 restarted itself.");

	for (i = 0; 1; i += 10) {
		/* 
		   printf( "pthrid %ld %d ms", task->pthrid, i );
		   if ( task->pthrid  > 0L )
		   break;
		 */
		usleep(100L);
		break;
	}
	/************************************************************************
	 **  First wait on empty MSQ1 in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	//puts("Task 5 waiting on enable5 to begin testing Watchdog 1");

	while (1) {
		CHK0(semTake(enable5, WAIT_FOREVER));
		TRACEF("Watchdog 1 count %d msec", wdog1_cycle);
	}
	CHK0(taskDelete(task8_id));
	return 0;
}

int task8(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
		  int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	STATUS err;
	TRACEF();
	// puts("Task 8 waiting on enable8 to begin testing Watchdog 2");
	while (1) {
		CHK0(semTake(enable8, WAIT_FOREVER));
		TRACEF("Watchdog 2 count %d msec", wdog2_cycle);
	}
	return 0;
}

int test_tasks_restart()
{
	TRACEF();
	/************************************************************************
	 **  Task Restart Test
	 ************************************************************************/
	//puts(".......... Next, we test the task restart logic...");
	//puts("           Task 8 will be restarted, and Task 1 will sleep");
	//puts("           briefly to allow Task 8 to run.");

	//display_task(stderr,task8_id);
	//printf("");

	//puts("Task 1 calling taskRestart for Task 8");
	CHK0(taskRestart(task8_id));
	CHK0(taskDelay(2));

	//display_task(stderr,task8_id);
	//printf("");
	// TODO: test
	//puts(".......... Next, we test the task self-restart logic...");
	//puts("           Task 5 will be restarted, and Task 1 will sleep");
	//puts("           briefly to allow Task 5 to run.");

	//display_task(stderr,task5_id);
	//printf("");

	task5_restarted++;
	taskDelay(50);
	taskDelete(task8_id);
	taskDelete(task5_id);
	//display_task(stderr,task5_id);
  exit:;
	TRACEF("finished");
	return OK;
}
