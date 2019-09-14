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
#include <v2ldebug.h>

SEM_ID mutex1_id;
SEM_ID mutex2_id;
SEM_ID mutex3_id;

/*****************************************************************************
**  test_mutexes
**         This function sequences through a series of actions to exercise
**         the various features and characteristics of v2pthreads mutexes
**
*****************************************************************************/
int test_mutexes(void)
{
	STATUS err;
	int i;
	TRACEF();

	CHK(mutex1_id = semMCreate(SEM_Q_FIFO));
	CHK0(semTake(mutex1_id, WAIT_FOREVER));
	// SEM_DELETE_SAFE semaphore is not implemented
	CHK(0 == (mutex2_id = semMCreate(SEM_Q_FIFO | SEM_DELETE_SAFE)));
	// SEM_INVERSION_SAFE semaphore is not implemented
	CHK(0 == (mutex3_id = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE)));

	/************************************************************************
	 **  Mutex Semaphore Recursive semTake Test
	 ************************************************************************/
	//puts(".......... Next we attempt to recursively lock Mutex 3.");
	//puts("           This should return no errors.");

	for (i = 1; i < 4; i++) {
		//printf("Task 1 recursively locking Mutex 3 - Locking pass %d", i);
		CHK(-1 == semTake(mutex3_id, WAIT_FOREVER));
	}

	/************************************************************************
	 **  Mutex Semaphore Recursive semGive Test
	 ************************************************************************/
	//puts(".......... Now we recursively unlock Mutex 3.");
	//puts("           This should return no errors on passes 1 through 3,");
	//puts("           but should return error 0x160068 on pass 4");
	//puts("           since we only recursively locked the mutex 3 times.");

	for (i = 1; i < 5; i++) {
		//printf("Task 1 recursively unlocking Mutex 3 - Unlocking pass %d", i);
		CHK(-1 == semGive(mutex3_id));
	}

	/************************************************************************
	 **  Mutex Semaphore semGive (Not Owner) Test 
	 ************************************************************************/
	//puts(".......... Next we enable Task 2 to attempt to unlock Mutex 1,");
	//puts("           which Task 1 previously locked and still 'owns'.");
	//puts("           This should return an error 0x160068.");

	//puts("Task 1 signalling Task 2 to attempt to unlock Mutex 1.");
	CHK0(semGive(enable2));
	//puts("Task 1 blocking for handshake from Task 2...");
	CHK0(semTake(complt2, WAIT_FOREVER));

	/************************************************************************
	 **  Mutex Semaphore Flush Test
	 ************************************************************************/

	// Next we attempt to flush Mutex 1.  Flushes are not 
	// allowed for mutex semaphores, so this should fail with
	// an error 0x160068.");

	CHK0(S_semLib_INVALID_OPERATION == semFlush(mutex1_id));

	/************************************************************************
	 **  Mutex Semaphore Priority Inversion Safety Test 
	 ************************************************************************/
	//puts(".......... Next we test mutex priority inversion protection.");
	//puts("           First task 2 (priority 20) will lock Mutex 3.  Then");
	//puts("           task 1 (priority 5) will attempt to lock Mutex 3, and");
	//puts("           will block.  At this point task 2's priority should be");
	//puts("           boosted to equal that of task 1.  Task 2 will then");
	//puts("           unlock the mutex, at which point task 2 should drop");
	//puts("           back to its initial priority setting, and task 1 should");
	//puts("           acquire the mutex.");

	//puts("Task 1 enabling Task 2 to acquire ownership of Mutex 3.");
	CHK0(semGive(enable2));

	//puts("Task 1 blocking for handshake from Task 2...");
	CHK0(semTake(complt2, WAIT_FOREVER));

	CHK(semTake(mutex3_id, WAIT_FOREVER));
	//puts("Task 1 blocking until Task 2 completes priority inversion test.");
	CHK0(semTake(complt2, WAIT_FOREVER));

	/************************************************************************
	 **  Mutex Semaphore Task Deletion Safety Test 
	 ************************************************************************/
	//puts(".......... Next we test mutex automatic deletion safety.");
	//puts("           First task 2 will lock Mutex 2, making task 2 safe");
	//puts("           from deletion.  Task 1 will then attempt to delete");
	//puts("           task 2, and should block.  Task 2 will then unlock the");
	//puts("           mutex, at which point task 2 should become deletable,");
	//puts("           and task 1 should complete the deletion of task 2.");

	//puts("Task 1 enabling Task 2 to acquire ownership of Mutex 2.");
	CHK0(semGive(enable2));

	//puts("Task 1 blocking for handshake from Task 2...");
	CHK0(semTake(complt2, WAIT_FOREVER));

	//printf("Task 1 attempting to delete Task 2");
	CHK0(taskDelete(task2_id));

	//puts("Task 1 calling taskIdVerify to confirm Task 2 deleted...");
	CHK(-1 == taskIdVerify(task2_id));

	/************************************************************************
	 **  Mutex Semaphore Deletion Test 
	 ************************************************************************/
	//puts(".......... Finally we test mutex deletion behavior for a");
	//puts("           mutex owned by the task deleting it.  (This is the");
	//puts("           recommended technique for mutex deletion.)");
	//puts("           Then we test deletion of a mutex not owned by the task");
	//puts("           which is deleting it.  No errors should be returned.");

	//puts("Task 1 deleting Mutex 1.");
	CHK0(semDelete(mutex1_id));

	//puts("Task 1 deleting Mutex 3.");
	CHK(-1 == semDelete(mutex3_id));
	//puts("Task 1 deleting Mutex 2.");
	CHK(-1 == semDelete(mutex2_id));
	TRACEF("finished");
	return OK;
}

void test_priority_inversion_protection()
{
	TRACEF();
	// Now wait on enable2 to demonstrate mutex priority inversion protection behavior.
	CHK0(semTake(enable2, WAIT_FOREVER));
	CHK(-1 == semTake(mutex3_id, WAIT_FOREVER));

	//taskShow(stderr,task_for(task2_id));

	// puts("Task 2 signalling complt2 to Task 1 to indicate Mutex 3 acquired.");
	CHK0(semGive(complt2));

	//puts("Task 1 blocked... task 2 priority boosted as shown.");
	//taskShow(stderr,task_for(task2_id));
	//printf("");

	taskDelay(20);

	CHK(-1 == semGive(mutex3_id));
	taskDelay(2);

	//puts("Task 1 priority restored as shown after releasing Mutex 3.");
	//display_task(stderr,task2_id);
	//printf("");

	// puts("Signalling complt2 to Task 1 - Task 2 finished priority inversion test.");
	CHK0(semGive(complt2));

}

void test_automatic_deletion_safety()
{
	/************************************************************************
	 **  Now wait on enable2 to demonstrate mutex automatic deletion safety
	 **  behavior.
	 ************************************************************************/
	//puts("Task 2 waiting on enable2 to begin auto deletion safety test");
	CHK0(semTake(enable2, WAIT_FOREVER));

	CHK(-1 == semTake(mutex2_id, WAIT_FOREVER));

	// puts("Task 2 signalling complt2 to Task 1 to indicate Mutex 2 acquired.");
	CHK0(semGive(complt2));

	//puts("Task 1 blocked... task 2 not deletable while it owns Mutex 2.");

	//puts("Task 2 unlocking Mutex 2 and becoming deletable again");
	CHK0(semGive(mutex2_id));
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
	CHK0(semTake(enable2, WAIT_FOREVER));

	//  Attempt to unlock Mutex 1, which Task 2 does not currently own.
	CHK(-1 == semGive(mutex1_id));
	CHK(errno == S_semLib_INVALID_OPERATION);
	CHK0(semGive(complt2));
	test_priority_inversion_protection();
	test_automatic_deletion_safety();

	taskDelay(200);

	return (0);
}
