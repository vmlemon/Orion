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
#define __USE_GNU // for TIMEVAL_TO_TIMESPEC
#include <time.h>
//#define __USE_XOPEN2K // for pthread_mutex_timedlock
#include <pthread.h>
#include "v2lpthread.h"
#include "vxw_hdrs.h"
#include "test.h"
#include "v2ldebug.h"


int test_child_id;
int task2_id;

MSG_Q_ID queue1_id;
MSG_Q_ID queue2_id;
MSG_Q_ID queue3_id;

SEM_ID sem1_id;
SEM_ID sem2_id;
SEM_ID sem3_id;

SEM_ID enable1;
SEM_ID enable2;
SEM_ID enable3;
SEM_ID enable4;
SEM_ID enable5;
SEM_ID enable6;
SEM_ID enable7;
SEM_ID enable8;
SEM_ID enable9;
SEM_ID enable10;

SEM_ID complt1;
SEM_ID complt2;
SEM_ID complt3;
SEM_ID complt4;
SEM_ID complt5;
SEM_ID complt6;
SEM_ID complt7;
SEM_ID complt8;
SEM_ID complt9;
SEM_ID complt10;




void smaphores_create()
{
	TRACEF();
	//  First we create twenty binary semaphores which will  be used for task synchronization during tests.
	CHK(enable1 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(complt1 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(enable2 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(complt2 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(enable3 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(complt3 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	//CHECK0(semGive(enable3));
	//CHECK0(semTake(enable3, WAIT_FOREVER));
	CHK(enable4 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(complt4 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(enable5 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(complt5 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(enable6 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(complt6 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(enable7 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(complt7 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(enable8 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(complt8 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(enable9 = semBCreate(SEM_Q_FIFO, SEM_FULL));
	CHK0(semTake(enable9, WAIT_FOREVER));
	CHK(complt9 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(enable10 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	CHK(complt10 = semBCreate(SEM_Q_FIFO, SEM_EMPTY));
	//semList(stderr, 0);
  exit:;
}

/*****************************************************************************
**  test_child
**         This is the 'sequencer' task.  It orchestrates the production of
**         messages, enables, and semaphore tokens in such a way as to provide
**         a predictable sequence of enables for the validation record.  All
**         other tasks 'handshake' with test_child to control the timing of their
**         activities.
**
*****************************************************************************/
pthread_mutex_t test_finished;


void status_show()
{
	TRACEF();
	taskList(stderr, 0);
	semList(stderr, 0);
}

int test_child(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
			   int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	TRACEF();
	//  Indicate messages originated with the first test cycle.
	//test_cycle = 1;
	smaphores_create();
	TRACEF();
	//status_show();
	CHK0(test_tasks());
	//test_cycle++;
	CHK0(test_semaphores());
	//test_cycle++;
	CHK0(test_mutexes());
	//test_cycle++;
	CHK0(test_msg_queues());
	CHK0(test_watchdog_timers());
	CHK0(pthread_mutex_unlock(&test_finished));
	TRACEF("finished");
	return 0;
}

void sighandler(int x);
int test()
{
	trace = 1;
	TRACEF("");
	signal(SIGSEGV, sighandler);
	msgQList(stderr, 1);
	enableRoundRobin();
	CHK(test_child_id = taskSpawn("TESTER", 5, 0, 0, test_child, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));

	printf("");
  exit:
	return 0;
}

#ifdef _USR_SYS_INIT_KILL

void user_sysinit(void)
{
	TRACEF();
	test();
}

extern int exit_value;

int pthread_mutex_timedlock(pthread_mutex_t * mutex, const struct timespec *abs_timeout);

void test_wait()
{
	STATUS status = OK;
	errno = OK;
	{
		// wait up tp 30 sec for end of the test
		struct timeval tv;
		struct timespec ts;
		gettimeofday (&tv, NULL);
		TIMEVAL_TO_TIMESPEC (&tv, &ts);
		ts.tv_sec += 30;
		CHK0(pthread_mutex_init(&test_finished, NULL));
		CHK0(pthread_mutex_lock(&test_finished));
		CHK0(pthread_mutex_timedlock(&test_finished,&ts)); 
	}
	TRACEF();
	TRACEV("%i", status);
	TRACE_errno();
	CHK(3 == taskList(stderr, 0));
	switch (errno) {
		case OK:
			puts("Test finished");
			break;
		case S_objLib_OBJ_TIMEOUT:
			TRACEF("ERROR: timeout");
			TRACEF();
			semList(stderr, 0);
			//puts("test failed with timeout");
			TRACEF();
			//gdb();
			//trace=0;
			//trace=1;
			exit_value = -2;
			break;
		default:
			puts("test failed");
			exit_value = -1;
	}
}

void user_syskill(void)
{
	TRACEF();
	// user_syskill is called before user_sysinit
	//while (getchar() != (int) 'q') 
	//sleep(1);
	test_wait();
	TRACEF("finished");
}

#else

int main(int argc, char **argv)
{
	v2lin_init();
	test();
	while (getchar() != (int) 'q')
		sleep(1);
	return 0;
}

#endif
