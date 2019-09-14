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
#include "vxw_defs.h"
#include "test.h"
#include "v2ldebug.h"

// This function sequences through a series of actions to exercise
// the various features and characteristics of v2pthreads semaphores
int test_binary_semaphores(void)
{
	STATUS err;
	TRACEF();

	// Non-Mutex Semaphore Flush Test

	//puts(".......... Next we enable Tasks 4, 7, and 10 to wait for");
	//puts("           a token from enable1 in reverse-priority order.");
	//puts("           Then we flush enable1, waking all waiting tasks");

	//puts("Task 1 enabling Tasks 4, 7, and 10 to consume enable1 tokens.");
	CHK0(semGive(enable4));
	taskDelay(2);
	CHK0(semGive(enable7));
	taskDelay(2);
	CHK0(semGive(enable10));
	taskDelay(2);

	CHK0(semFlush(enable1));

	//puts("Task 1 blocking until Tasks 4, 7, and 10 complete semFlush test.");
	CHK0(semTake(complt4, WAIT_FOREVER)); // 1
	CHK0(semTake(complt7, WAIT_FOREVER));
	CHK0(semTake(complt10, WAIT_FOREVER));
	TRACEF("finished");
	return OK;
}

int test_counting_semaphores(void)
{
	STATUS err;
	int i;
	TRACEF();

	CHK(sem1_id = semCCreate(SEM_Q_FIFO, 0));
	CHK(sem2_id = semCCreate(SEM_Q_FIFO, 2));
	// SEM_Q_PRIORITY is not implemented
	CHK(0 == (sem3_id = semCCreate(SEM_Q_PRIORITY, 0)));
	CHK(errno == ENOSYS);

	// Semaphore Waiting and Task Queueing Order Test
	/*
	   puts(".......... Next we enable Tasks 4, 7, and 10 to wait for");
	   puts("           a token from sem1_id in reverse-priority order.");
	   puts("           Then we send three tokens to sem1_id and wait to see");
	   puts("           the order in which the tasks get tokens.");
	   puts("           This tests the semaphore post and queueing logic.");
	   puts("           The token should be acquired by Tasks 4, 7, and 10");
	   puts("           in that order.");

	   puts("Task 1 enabling Tasks 4, 7, and 10 to consume sem1_id tokens.");
	 */
	CHK0(semGive(enable4));
	CHK0(semGive(enable7));
	CHK0(semGive(enable10));

	//puts("Task 1 blocking for handshake from Tasks 4, 7, and 10...");
	CHK0(semTake(complt4, WAIT_FOREVER)); // 2
	CHK0(semTake(complt7, WAIT_FOREVER));
	CHK0(semTake(complt10, WAIT_FOREVER));

	for (i = 0; i < 3; i++) {
		CHK0(semGive(sem1_id));
	}

	//puts("Task 1 blocking for handshake from Tasks 4, 7, and 10...");
	CHK0(semTake(complt4, WAIT_FOREVER)); // 3
	CHK0(semTake(complt7, WAIT_FOREVER));
	CHK0(semTake(complt10, WAIT_FOREVER));

	//puts(".......... Next Tasks 4, 7, and 10 look for tokens from sem2_id");
	//puts("           in reverse-priority order.  However, sem2_id has only two");
	//puts("           tokens available, so one task will fail to acquire one.");
	//puts("           Since the tasks did not wait on the semaphore, the");
	//puts("           loser of the race will return an error 0x3d0002");
	//puts("Task 1 enabling Tasks 4, 7, and 10 to consume sem2_id tokens.");
	CHK0(semGive(enable4));
	CHK0(semGive(enable7));
	CHK0(semGive(enable10));
	//puts("Task 1 blocking for handshake from Tasks 4, 7, and 10...");
	CHK0(semTake(complt4, WAIT_FOREVER)); // 4
	CHK0(semTake(complt7, WAIT_FOREVER));
	CHK0(semTake(complt10, WAIT_FOREVER));

	//sem3_id is SEM_Q_PRIORITY, which is not implemented

	//  Next Tasks 4, 7, and 10 look for tokens from sem3_id
	// in reverse-priority order.  However, sem3_id will be sent");
	// only two tokens, so one task will fail to acquire one.");
	// Since the tasks do wait on the semaphore, the lowest");
	// priority task will return an errno 0x3d0004");

	//puts("Task 1 enabling Tasks 4, 7, and 10 to consume sem3_id tokens.");
	CHK0(semGive(enable4));
	taskDelay(2);
	CHK0(semGive(enable7));
	taskDelay(2);
	CHK0(semGive(enable10));
	taskDelay(2);

	//puts("Task 1 blocking for handshake from Tasks 4, 7, and 10...");
	CHK0(semTake(complt4, WAIT_FOREVER)); // 5
	CHK0(semTake(complt7, WAIT_FOREVER));
	CHK0(semTake(complt10, WAIT_FOREVER));

	// Give ony twice
	for (i = 0; i < 2; i++) {
		CHK(-1 == semGive(sem3_id));
	}
	//puts("Task 1 blocking until Tasks 4, 7, and 10 consume sem3_id tokens.");
	CHK0(semTake(complt4, WAIT_FOREVER)); // 6
	CHK0(semTake(complt7, WAIT_FOREVER));
	CHK0(semTake(complt10, WAIT_FOREVER));

	/************************************************************************
	 **  Semaphore Deletion Test
	 ************************************************************************/

	//puts(".......... Next Tasks 4, 7, and 10 look for tokens from sem1_id");
	//puts("           in priority order.  Task 1 will delete sem1_id before any");
	//puts("           tokens become available.  Tasks 4, 7, and 10 should be");
	//puts("           awakened and return errno 0x3d0001.");
	//puts("           sem2_id will be deleted with no tasks waiting.");
	//puts("           This tests the semDelete logic.");
	taskDelay(2);
	//puts("Task 1 deleting semaphore sem1_id.");
	CHK0(semDelete(sem1_id));
	CHK0(semDelete(sem2_id));
	//puts("Task 1 blocking until Tasks 4, 7, and 10 complete semDelete test.");
	CHK0(semTake(complt4, WAIT_FOREVER)); // 7 
	CHK0(semTake(complt7, WAIT_FOREVER));
	CHK0(semTake(complt10, WAIT_FOREVER));
	/************************************************************************
	 **  Semaphore-Not-Found Test
	 ************************************************************************/

	//puts(".......... Finally, we verify the error codes returned when");
	//puts("           a non-existent semaphore is specified.");

	CHK(-1 == semGive(sem1_id));
	CHK(-1 == semTake(sem1_id, NO_WAIT));
	CHK(-1 == semTake(sem1_id, WAIT_FOREVER));
	CHK(-1 == semDelete(sem1_id));
	TRACEF("finished");
	return OK;
}

int task4(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
		  int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	STATUS err;
	TRACEF();

	/************************************************************************
	 **  First wait on empty enable1 in pre-determined task order to test
	 **  semaphore flush task waking order.
	 ************************************************************************/
	//puts("Task 4 waiting on enable4 to begin acquiring token from enable1");
	CHK0(semTake(enable4, WAIT_FOREVER));
	//  Consume one token from enable1.
	CHK0(semTake(enable1, WAIT_FOREVER));

	CHK0(semGive(complt4)); // 1

	/************************************************************************
	 **  Next wait on empty sem1_id in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	CHK0(semTake(enable4, WAIT_FOREVER));
	CHK0(semGive(complt4)); // 2

	CHK0(semTake(sem1_id, WAIT_FOREVER));

	/************************************************************************
	 **  Next wait on sem2_id to demonstrate semTake without wait.
	 ************************************************************************/
	CHK0(semGive(complt4)); // 3

	//puts("Task 4 waiting on enable4 to begin acquiring token from sem2_id");
	CHK0(semTake(enable4, WAIT_FOREVER));

	//  Consume a token from sem2_id without waiting.
	// puts("Task 4 attempting to acquire token from sem2_id without waiting.");
	CHK0(semTake(sem2_id, NO_WAIT));
	//puts("Signalling complt4 to Task 1 - Task 4 ready to test SEM_Q_PRIORITY.");
	CHK0(semGive(complt4)); // 4

	/************************************************************************
	 **  Next wait on sem3_id in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	// puts("Task 4 waiting on enable4 to begin acquiring token from sem3_id");
	CHK0(semTake(enable4, WAIT_FOREVER));

	//puts("Task 4 signalling complt4 to Task 1 to indicate Task 4 ready.");
	CHK0(semGive(complt4)); // 5

	// **  Consume one token from sem3_id.
	CHK(-1 == semTake(sem3_id, 100));

	//puts("Signalling complt4 to Task 1 - Task 4 ready for semDelete test.");
	CHK0(semGive(complt4)); // 6
	//CHK0(semTake(sem1_id, WAIT_FOREVER));

	// puts("Signalling complt4 to Task 1 - Task 4 finished semDelete test.");
	CHK0(semGive(complt4)); // 7
	CHK0(taskDelete(0));
	return (0);
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
	CHK0(semTake(enable7, WAIT_FOREVER));
	CHK0(semTake(enable1, WAIT_FOREVER));
	CHK0(semGive(complt7));
	/************************************************************************
	 **  Next wait on empty sem1_id in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	//puts("Task 7 waiting on enable7 to begin acquiring token from sem1_id");
	CHK0(semTake(enable7, WAIT_FOREVER));
	//puts("Task 7 signalling complt7 to Task 1 to indicate Task 7 ready.");
	CHK0(semGive(complt7));
	CHK0(semTake(sem1_id, WAIT_FOREVER));
	CHK0(semGive(complt7));
	//puts("Task 7 waiting on enable7 to begin acquiring token from sem2_id");
	CHK0(semTake(enable7, WAIT_FOREVER));
	CHK0(semTake(sem2_id, NO_WAIT));
	CHK0(semGive(complt7));
	/************************************************************************
	 **  Next wait on sem3_id in pre-determined task order to test priority-based
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	CHK0(semTake(enable7, WAIT_FOREVER));
	CHK0(semGive(complt7));
	CHK(-1 == semTake(sem3_id, 100));
	CHK0(semGive(complt7));

	//CHK0(semTake(sem1_id, WAIT_FOREVER));
	CHK0(semGive(complt7));
	taskDelete(0);
	return 0;
}

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
	CHK0(semTake(enable10, WAIT_FOREVER));
	CHK0(semTake(enable1, WAIT_FOREVER));
	// "Signalling complt10 to Task 1 - Task 10 ready to test SEM_Q_FIFO.");
	CHK0(semGive(complt10));
	/************************************************************************
	 **  Next wait on empty sem1_id in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	// puts("Task 10 waiting on enable10 to begin acquiring token from sem1_id");
	CHK0(semTake(enable10, WAIT_FOREVER));

	//puts("Task 10 signalling complt10 to Task 1 to indicate Task 10 ready.");
	CHK0(semGive(complt10));
	CHK0(semTake(sem1_id, WAIT_FOREVER));
	//  demonstrate semTake without wait.
	CHK0(semGive(complt10));
	CHK0(semTake(enable10, WAIT_FOREVER));
	CHK0(semTake(sem2_id, NO_WAIT));
	CHK0(semGive(complt10));

	/************************************************************************
	 **  Next wait on sem3_id in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	CHK0(semTake(enable10, WAIT_FOREVER));
	CHK0(semGive(complt10));
	CHK(-1 == semTake(sem3_id, 100));
	CHK0(semGive(complt10));
	//CHK0(semTake(sem1_id, WAIT_FOREVER));
	CHK0(semGive(complt10));
	err = taskDelete(0);
	return (0);
}

SEM_ID s1;
int test_semaphores_1_status=ERROR;

STATUS test_sem_1_sub()
{
	TRACEF();
	semTake(s1,1000000);
	test_semaphores_1_status=OK;
	return 0;
}

// simpliest semaphore test
STATUS test_semaphores_1()
{
	int t;
	TRACEF();
	test_semaphores_1_status = ERROR;
	CHK(s1 = semCCreate(SEM_Q_FIFO, 0));
	TRACEV("%i",test_semaphores_1_status);
	CHK(t = taskSpawn(NULL, 20, 0, 0, test_sem_1_sub, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	//taskDelay(0);
	semGive(s1);
	//taskDelay(0);
	TRACEV("%i",test_semaphores_1_status);
	CHK(test_semaphores_1_status==OK);
	//assert(test_semaphores_1_status==OK);
	//exit(1);
	return test_semaphores_1_status;
}

STATUS test_semaphores()
{
	TRACEF();
	STATUS status=OK;
	int t;
	CHK0(test_semaphores_1());
	CHK0(test_binary_semaphores());
	CHK0(test_counting_semaphores());
	return status;
}
