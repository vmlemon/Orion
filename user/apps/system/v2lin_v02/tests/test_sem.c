/****************************************************************************
 * Copyright (C) 2004, 2005, 2006 v2lin Team <http://v2lin.sf.net>
 * 
 * This file is part of the v2lin Library.
 * VxWorks is a registered trademark of Wind River Systems, Inc.
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

// testSemaphore.cpp : 
//
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <sched.h>

#include "vxw_hdrs.h"
#include "v2lpthread.h"

typedef enum
{
	INITIAL_STATE,

	BINARY_SAME,				/* same thread test - give 1 time, take 1 time - no block */

	BINARY_OTHER,				/* main thread tries to take and blocks */

	BINARY_OPPOSITE,			/* other thread gives, and main unblocks */

	/****************/

	COUNTING_SAME,				/* same thread test - create with INIT_COUNT, take INIT_COUNT
								   times, give MAX_COUNT times, take MAX_COUNT times */

	COUNTING_OTHER,				/* main thread tries to take and blocks */

	COUNTING_OPPOSITE,			/* other thread gives and main is unblocked */

	/****************/

	MUTEX_SAME_TAKE,			/* same thread is able to take MUTEX_MAX times without a block */

	MUTEX_OTHER_GIVE,			/* other thread gets an error when trying to give a mutex which
								   belongs to another thread */

	MUTEX_OTHER_WAIT,			/* other thread is blocked when trying to take */

	MUTEX_GIVE_SOME,			/* main thread gives MUTEX_MAX-1 times, other is still blocked */

	MUTEX_GIVE_ALL,				/* main thread gives 1 time, other is unblocked */

	MUTEX_OPPOSITE_TO,			/* now mutex belongs to the second thread, so main is timed out
								   when trying to take */

	MUTEX_OPPOSITE_GIVE,		/* other gives the mutex back */

	MUTEX_UNBLOCK,				/* the second thread gives, and the main is able to take again */
} e_TestState;

e_TestState g_state = INITIAL_STATE;

/////////////////////////////////////////////////////////////////////////////

#define TEST_SEM_TIMEOUT		1000
#define	TEST_SEM_INIT_COUNT		25
#define	TEST_SEM_MAX_COUNT		50
#define TEST_MUTEX_MAX			32000

unsigned cGive, cTake, cTakeTimeout, cTakeErr, cGiveErr;

SEM_ID s_binary;
SEM_ID s_counting;
SEM_ID s_mutex;

int RandomizerThreadFunc(int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9,
						 int p10)
{
	int operation, semIdx, semTimeout;
	int stat = 0;

	while (1) {
		operation = (int) (rand() * 2.) / RAND_MAX;
		semIdx = (int) (rand() * 2.) / RAND_MAX;
		semTimeout = (int) rand() * 5. / RAND_MAX;

		switch (operation) {
		case 0:
			cTake++;
			switch (semIdx) {
			case 0:
				stat = semTake(s_binary, semTimeout);
				break;
			case 1:
				stat = semTake(s_counting, semTimeout);
				break;
			}
			if (OK != stat) {
				if (S_objLib_OBJ_TIMEOUT == errno || S_objLib_OBJ_UNAVAILABLE == errno) {
					cTakeTimeout++;
				} else {
					cTakeErr++;
					puts("");
					perror("Error taking semaphore");
				}
			}
			break;

		case 1:
			cGive++;
			switch (semIdx) {
			case 0:
				stat = semGive(s_binary);
				break;
			case 1:
				stat = semGive(s_counting);
				break;
			}
			if (OK != stat)
				cGiveErr++;
			break;
		default:
			break;

		}						//switch( operation )

	}							//while(1)
}

void Go2State(e_TestState state, const char *state_name)
{
	printf("\nEntering %s state ", state_name);
#ifdef ENTER_STATE_KB_DELAY
	printf(" - press <return> to continue...");
	char buf[20];
	fgets(buf, sizeof(buf), stdin);
#endif
	g_state = state;
}

void Wait4State(e_TestState a_state)
{
	while (g_state != a_state) {
		sched_yield();
		usleep(5000);
	}
	sleep(1);					//to ensure the main is blocked well
	if (g_state != a_state)
		printf("Error waiting for %u state: state changed to %d\n", a_state, g_state);
}

void CheckState(e_TestState old_state, e_TestState new_state)
{
	if (g_state != new_state) {
		printf
			("Error in waiting in %d state: should unblock in %d state, but unblocked in %d state\n",
			 old_state, new_state, g_state);
		Wait4State(new_state);
	}
}

int OtherThreadFunc(int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10)
{
	Wait4State(BINARY_OTHER);

	Go2State(BINARY_OPPOSITE, "BINARY_OPPOSITE");
	if (semGive(s_binary) != OK)
		perror("Error giving in BINARY_OTHER state");

	Wait4State(COUNTING_OTHER);

	Go2State(COUNTING_OPPOSITE, "COUNTING_OPPOSITE");
	if (semGive(s_counting) != OK)
		perror("Error giving in COUNTING_OTHER state");

	Wait4State(MUTEX_OTHER_GIVE);
	if (semGive(s_mutex) == OK)
		printf("Error in MUTEX_OTHER_GIVE: give operation must fail, but it succeeded\n");

	Go2State(MUTEX_OTHER_WAIT, "MUTEX_OTHER_WAIT");
	if (semTake(s_mutex, WAIT_FOREVER) != OK)
		perror("Error taking in MUTEX_OTHER_WAIT state");

	CheckState(MUTEX_OTHER_WAIT, MUTEX_GIVE_ALL);

	Go2State(MUTEX_OPPOSITE_TO, "MUTEX_OPPOSITE_TO");

	Wait4State(MUTEX_OPPOSITE_GIVE);
	if (semGive(s_mutex) != OK)
		perror("Error giving in MUTEX_OPPOSITE_GIVE state");

	Wait4State(MUTEX_UNBLOCK);
	printf("Other thread is complete.\n");

	return 0;
}

#ifdef _USR_SYS_INIT_KILL
void user_sysinit(void)
{
#else
int main(int argc, char **argv)
{
#endif
	int randomizer_thread1, randomizer_thread2;
	int other_thread;
	int i;

#ifndef _USR_SYS_INIT_KILL
	v2lin_init();
#endif
	s_binary = semBCreate(SEM_Q_FIFO, 0);
	s_counting = semCCreate(SEM_Q_FIFO, TEST_SEM_INIT_COUNT);
	s_mutex = semMCreate(SEM_Q_FIFO);

	other_thread = taskSpawn("other", 0, 0, 0, OtherThreadFunc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	printf("\n===============SEMPAHORE TEST START======================\n");

	Go2State(BINARY_SAME, "BINARY_SAME");
	if (semGive(s_binary) != OK)
		perror("Error giving in BINARY_SAME state");
	if (semTake(s_binary, WAIT_FOREVER) != OK)
		perror("Error taking in BINARY_SAME state");

	Go2State(BINARY_OTHER, "BINARY_OTHER");
	if (semTake(s_binary, WAIT_FOREVER) != OK)
		perror("Error taking in BINARY_OTHER state");

	CheckState(BINARY_OTHER, BINARY_OPPOSITE);

	Go2State(COUNTING_SAME, "COUNTING_SAME");
	for (i = 0; i < TEST_SEM_INIT_COUNT; i++)
		if (semTake(s_counting, WAIT_FOREVER) != OK)
			perror("Error taking in COUNTING_SAME state (first pass)");
	for (i = 0; i < TEST_SEM_MAX_COUNT; i++)
		if (semGive(s_counting) != OK)
			perror("Error giving in COUNTING_SAME state");
	for (i = 0; i < TEST_SEM_MAX_COUNT; i++)
		if (semTake(s_counting, WAIT_FOREVER) != OK)
			perror("Error taking in COUNTING_SAME state (second pass)");

	Go2State(COUNTING_OTHER, "COUNTING_OTHER");
	if (semTake(s_counting, WAIT_FOREVER) != OK)
		perror("Error taking in COUNTING_OTHER state");

	CheckState(COUNTING_OTHER, COUNTING_OPPOSITE);

	Go2State(MUTEX_SAME_TAKE, "MUTEX_SAME_TAKE");
	for (i = 0; i < TEST_MUTEX_MAX; i++)
		if (semTake(s_mutex, WAIT_FOREVER) != OK)
			perror("Error taking in MUTEX_SAME_TAKE state");

	Go2State(MUTEX_OTHER_GIVE, "MUTEX_OTHER_GIVE");

	Wait4State(MUTEX_OTHER_WAIT);

	Go2State(MUTEX_GIVE_SOME, "MUTEX_GIVE_SOME");
	for (i = 0; i < TEST_MUTEX_MAX - 1; i++)
		if (semGive(s_mutex) != OK)
			perror("Error giving in MUTEX_GIVE_SOME state");

	Go2State(MUTEX_GIVE_ALL, "MUTEX_GIVE_ALL");
	if (semGive(s_mutex) != OK)
		perror("Error giving in MUTEX_GIVE_ALL state");

	Wait4State(MUTEX_OPPOSITE_TO);
	if (semTake(s_mutex, 2000) == OK)	//2 seconds
		printf("Error: taking in MUTEX_OPPOSITE_TO state must fail\n");
	if (errno != S_objLib_OBJ_TIMEOUT)
		perror("Error taking in MUTEX_OPPOSITE_TO");

	Go2State(MUTEX_OPPOSITE_GIVE, "MUTEX_OPPOSITE_GIVE");
	if (semTake(s_mutex, WAIT_FOREVER) != OK)
		perror("Error taking in MUTEX_OPPOSITE_GIVE");

	Go2State(MUTEX_UNBLOCK, "MUTEX_UNBLOCK");

	sleep(1);					// to let the other thread finish

	//==================== RANDOM TEST ===========================
	//printf("\n\nRandom test - press ^C to stop\n");


	srand((unsigned) time(NULL));
	cGive = cTake = cTakeErr = cTakeTimeout = cGiveErr = 0;

	randomizer_thread1 =
		taskSpawn("thread1", 0, 0, 0, RandomizerThreadFunc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	randomizer_thread2 =
		taskSpawn("thread2", 0, 0, 0, RandomizerThreadFunc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#ifdef _USR_SYS_INIT_KILL
}
void user_syskill(void)
{
#endif
	//while (1) 
	{
		sleep(30);

		printf("\nTaken:%u Given:%u Take timeout:%u Fail to Take %u Fail to Give:%u\n\n",
				cTake, cGive, cTakeTimeout, cTakeErr, cGiveErr);
	}
#ifndef _USR_SYS_INIT_KILL
	return 0;
#endif
}
