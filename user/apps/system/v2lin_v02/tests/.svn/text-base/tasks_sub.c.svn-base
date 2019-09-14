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
#include <errno.h>
#include <assert.h>
#include "v2lpthread.h"
#include "vxw_hdrs.h"
#include "test.h"

int task10_id;
int task8_id;
int task3_id;
int task5_id;
int task4_id;
int task6_id;
int task7_id;
int task9_id;

unsigned char task5_restarted = 0;


int task10(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
		   int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	STATUS err;
	int i;
	TRACEF();

	for (i = 0; i < 10; i++) {
		taskDelay(50);
		//puts("Task 10 Not Suspended.");
	}
	//  First wait on empty enable1 in pre-determined task order to test  semaphore flush task waking order.
	CHECK0(semTake(enable10, WAIT_FOREVER));
	CHECK0(semTake(enable1, WAIT_FOREVER));
	// "Signalling complt10 to Task 1 - Task 10 ready to test SEM_Q_FIFO.");
	CHECK0(semGive(complt10));
	/************************************************************************
	 **  Next wait on empty SEM1 in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	// puts("Task 10 waiting on enable10 to begin acquiring token from SEM1");
	CHECK0(semTake(enable10, WAIT_FOREVER));

	//puts("Task 10 signalling complt10 to Task 1 to indicate Task 10 ready.");
	CHECK0(semGive(complt10));
	CHECK0(semTake(sem1_id, WAIT_FOREVER));
	//  demonstrate semTake without wait.
	CHECK0(semGive(complt10));
	CHECK0(semTake(enable10, WAIT_FOREVER));
	CHECK0(semTake(sem2_id, NO_WAIT));
	CHECK0(semGive(complt10));

	/************************************************************************
	 **  Next wait on SEM3 in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	CHECK0(semTake(enable10, WAIT_FOREVER));
	// puts("Task 10 signalling complt10 to Task 1 to indicate Task 10 ready.");
	CHECK0(semGive(complt10));
	CHECK0(semTake(sem3_id, 100));
	//puts("Signalling complt10 to Task 1 - Task 10 ready for semDelete test.");
	CHECK0(semGive(complt10));
	CHECK0(semTake(sem1_id, WAIT_FOREVER));
	//puts("Signalling complt10 to Task 1 - Task 10 finished semDelete test.");
	CHECK0(semGive(complt10));
	err = taskDelete(0);
	return (0);
}
int task8(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
		  int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	STATUS err;
	TRACEF();
	// puts("Task 8 waiting on enable8 to begin testing Watchdog 2");
	while (1) {
		CHECK0(semTake(enable8, WAIT_FOREVER));
		TRACEF("Watchdog 2 count %d msec", wdog2_cycle);
	}
	return 0;
}

int task7(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
		  int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	STATUS err;
	TRACEF();

	/************************************************************************
	 **  First wait on empty enable1 in pre-determined task order to test
	 **  semaphore flush task waking order.
	 ************************************************************************/
	//puts("Task 7 waiting on enable7 to begin acquiring token from enable1");
	CHECK0(semTake(enable7, WAIT_FOREVER));
	CHECK0(semTake(enable1, WAIT_FOREVER));
	CHECK0(semGive(complt7));
	/************************************************************************
	 **  Next wait on empty SEM1 in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	//puts("Task 7 waiting on enable7 to begin acquiring token from SEM1");
	CHECK0(semTake(enable7, WAIT_FOREVER));
	//puts("Task 7 signalling complt7 to Task 1 to indicate Task 7 ready.");
	CHECK0(semGive(complt7));
	CHECK0(semTake(sem1_id, WAIT_FOREVER));
	CHECK0(semGive(complt7));
	//puts("Task 7 waiting on enable7 to begin acquiring token from SEM2");
	CHECK0(semTake(enable7, WAIT_FOREVER));
	CHECK0(semTake(sem2_id, NO_WAIT));
	CHECK0(semGive(complt7));

	/************************************************************************
	 **  Next wait on SEM3 in pre-determined task order to test priority-based
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	//puts("Task 7 waiting on enable7 to begin acquiring token from SEM3");
	CHECK0(semTake(enable7, WAIT_FOREVER));
	CHECK0(semGive(complt7));
	// **  Consume one token from SEM3.
	// puts("Task 7 waiting up to 1 second to acquire token from SEM3");
	CHECK0(semTake(sem3_id, 100));
	CHECK0(semGive(complt7));
	CHECK0(semTake(sem1_id, WAIT_FOREVER));
	CHECK0(semGive(complt7));
	taskDelete(0);
	return 0;
}

extern task_t *task_for(int taskid);



int task3(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
		  int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	STATUS err;
	TRACEF();
	union
	{
		char blk[128];
		my_qmsg_t msg;
	} bigmsg;
	int i;
	test_queue_order_wait();
	test_queue_delete();
	err = taskDelete(0);
	return (0);
}

void test_automatic_deletion_safety()
{
	/************************************************************************
	 **  Now wait on enable2 to demonstrate mutex automatic deletion safety
	 **  behavior.
	 ************************************************************************/
	//puts("Task 2 waiting on enable2 to begin auto deletion safety test");
	CHECK0(semTake(enable2, WAIT_FOREVER));

	CHECK0(semTake(mutex2_id, WAIT_FOREVER));

	// puts("Task 2 signalling complt2 to Task 1 to indicate Mutex 2 acquired.");
	CHECK0(semGive(complt2));

	//puts("Task 1 blocked... task 2 not deletable while it owns Mutex 2.");

	//puts("Task 2 unlocking Mutex 2 and becoming deletable again");
	CHECK0(semGive(mutex2_id));
}

int task2(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
		  int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	STATUS err;
	TRACEF();

	/************************************************************************
	 **  First wait on empty MSQ1 in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	CHECK0(semTake(enable2, WAIT_FOREVER));

	//  Attempt to unlock Mutex 1, which Task 2 does not currently own.
	//printf("Task 2 attempting to unlock Mutex 1");
	CHECK0(semGive(mutex1_id));
	CHECK0(semGive(complt2));
	test_priority_inversion_protection();
	test_automatic_deletion_safety();

	taskDelay(200);

	return (0);
}
