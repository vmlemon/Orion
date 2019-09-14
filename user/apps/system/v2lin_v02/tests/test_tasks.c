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
#include "v2ldebug.h"


int test_child_id;
int temp_taskid;

int task10_id;
int task3_id;
int task5_id;
int task4_id;
int task6_id;
int task7_id;
int task8_id;
int task9_id;

unsigned char task5_restarted = 0;

int temp_task(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
			  int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	int i;
	STATUS status = 0;
	TRACEF();

	temp_taskid = taskIdSelf();
	TRACEF();
	//status_show();
	CHK0(semTake(enable1, WAIT_FOREVER));
	TRACEV("%x", enable1);
	TRACEF();
	//status_show();
	taskSafe();

	//puts("Signalling complt1 to Task 1 - Temp Task ready to test deletion control");
	CHK0(semGive(complt1));

	for (i = 0; i < 3; i++) {
		//puts("Temp Task Not Deleted.");
		taskDelay(50);
	}

	//puts("Temp Task calling taskUnsafe() to allow task deletion.");
	taskUnsafe();

	while (1) {
		taskDelay(20);
	}
}

task_t fixed_task;

int test_tasks_tmp()
{
	TRACEF();
	// create a temporary task without starting it
	// Use a statically allocated task control block for temporary task.
	CHK0(taskInit(&fixed_task, "TEMP_TASK", 10, 0, 0, 0, &temp_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	// call taskName with a NULL taskid to check our task's name.
	TRACEF("%s", taskName(0));
	assert(0 == strcmp(taskName(0), "TESTER"));
	assert(0 == strcmp(taskName(fixed_task.taskid), "TEMP_TASK"));

	assert(!taskIsReady(fixed_task.taskid));
	CHK0(taskActivate(fixed_task.taskid));
	// now fixed_task is in PEN state
	//CHK(taskIsReady(fixed_task.taskid));
  exit:;
	return OK;
}

int test_tasks_suspend()
{
	TRACEF();
	// Feature is not implemented
	// kernelTimeSlice() test
	//display_task(stderr,task5_id);
	CHK0(kernelTimeSlice(0));
	//display_task(stderr,task5_id);
	CHK0(kernelTimeSlice(5));
	CHK0(taskDelay(200));
	CHK(ENOSYS == taskSuspend(task10_id));
	CHK0(taskDelay(150));
	CHK(ENOSYS == taskSuspend(task10_id));
	CHK(0 == taskIsSuspended(task10_id));
	CHK(errno == ENOSYS);
	CHK(ENOSYS == taskResume(task10_id));
	CHK(0 == taskIsSuspended(task10_id));
	CHK(errno == ENOSYS);
	CHK0(taskDelay(400));
  exit:;
	return OK;
}

int test_tasks_delete()
{
	TRACEF();
	/************************************************************************
	 **  Task Deletion Control Test
	 ************************************************************************/
	//puts(".......... Now test task deletion control.");
	//puts("           The temporary task will call taskSafe() to prevent");
	//puts("           itself from being deleted, and will signal Task 1.");
	//puts("           Task 1 will then attempt to delete the task, and");
	//puts("           Task 1 should block until the temporary task calls");
	//puts("           taskUnsafe(), at which time the deletion will proceed.");

	//puts("Task 1 signalling Temporary Task to begin task deletion safety test.");
	CHK0(semGive(enable1));

	//puts("Task 1 blocking for handshake from Temporary Task...");
	CHK0(semTake(complt1, WAIT_FOREVER));

	//puts(".......... Call taskIdVerify to verify ID of Temporary Task...");
	//puts("           This call and subsequent calls use the task ID");
	//puts("           returned earlier by Temp Task's call to taskIdSelf.");
	CHK0(taskIdVerify(temp_taskid));
	//puts(".......... Call taskIsReady to verify it indicates task ready.");
	CHK(taskIsReady(temp_taskid));

	CHK0(taskDelete(temp_taskid));
	CHK(!taskIdVerify(temp_taskid));

	//puts(".......... Now test forced task deletion.");
	//puts("           First recreate the temporary task just deleted.");
	//puts("           The temporary task will call taskSafe() to prevent");
	//puts("           itself from being deleted, and will signal Task 1.");
	//puts("           Task 1 will then attempt to forcibly delete the task,");
	//puts("           and the deletion should proceed without waiting for");
	//puts("           the temporary task to make a taskUnsafe call.");

	//puts("Starting New (Unnamed) Temporary Task at priority level 10");
	errno = 0;
	temp_taskid = taskSpawn((char *) 0, 10, 0, 0, temp_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	//puts("Task 1 signalling Temporary Task to begin forced task deletion test.");
	errno = 0;
	CHK0(semGive(enable1));

	//puts("Task 1 blocking for handshake from Temporary Task...");
	CHK0(semTake(complt1, WAIT_FOREVER));

	//puts(".......... Call taskIdVerify to verify ID of Temporary Task...");
	CHK0(taskIdVerify(temp_taskid));
	//puts("taskIdVerify indicates Temporary Task does exist.");
	//puts(".......... Call taskIsReady to verify it indicates task ready.");
	CHK0(!taskIsReady(temp_taskid));

	//printf("Temporary Task automatically named %s\n", taskName(temp_taskid));

	CHK0(taskDeleteForce(temp_taskid));
	CHK0(0 == taskIdVerify(temp_taskid));
  exit:;
	TRACEF("finished");
	return OK;
}

int test_tasks_priority()
{
	TRACEF();
	/************************************************************************
	 **  Task Get/Set Priority Test
	 ************************************************************************/
	int oldpriority;
	//puts(".......... Next test dynamic task priority manipulation.");
	//puts("           Call taskPriorityGet to save Task 2's priority.");
	CHK0(taskPriorityGet(task2_id, &oldpriority));
	TRACEV("%i", oldpriority);
	//puts(".......... Next call taskPrioritySet to raise priority on Task 2.");
	//puts("Task 1 setting priority to 18 for Task 2.");
	CHK0(taskPrioritySet(task2_id, 18));
	//CHK0(taskPriorityGet(task2_id, &oldpriority));
	//TRACEV("%i",oldpriority);
	//taskShow(stderr, task_for(task2_id));
	printf("");

	//puts(".......... Next call taskPrioritySet to restore Task 2's priority.");
	//puts("Task 1 restoring Task 2 to original priority setting.");
	CHK0(taskPrioritySet(task2_id, oldpriority));
	//display_task(stderr,task2_id);
	TRACEF("finished");
  exit:;
	return OK;
}


int test_tasks_ident()
{
	TRACEF();
	/************************************************************************
	 **  Task Identification Test
	 ************************************************************************/
	int i, num_tasks;
	int my_taskid;
	int tid_list[15];
	task_t *tmp;
	//puts(".......... Finally, we test the task Identification logic...");

	//puts("Task 1 calling taskNameToId.");
	my_taskid = taskNameToId("TSK3");
	//CHK0();
	//printf("taskNameToId for TSK3 returned ID %x... task3_id == %x\n", my_taskid, task3_id);

	//puts("Task 1 calling taskTcb.");
	tmp = taskTcb(task3_id);
	//printf("taskTcb for TSK3 returned TCB @ %p containing\n", tmp);
	//display_task(stderr,tmp->taskid);
	//printf("");

	//puts("Task 1 calling taskIdListGet.");
	num_tasks = taskIdListGet(tid_list, 15);
	//printf("taskIdListGet returned the following %d Task IDs\n", num_tasks);
	//for (i = 0; i < num_tasks; i++)
	//  printf("%d ", tid_list[i]);
	//printf("");
  exit:;
	TRACEF("finished");
	return OK;
}

int taskLock_test()
{
	task_t *task = taskTcb(0);
	int sched_policy;
	struct sched_param schedparam;
	//assert(pthread_self()==locker);

	pthread_attr_getschedparam(&task->attr, &schedparam);
	TRACEF();
	//CHK(schedparam.sched_priority == sched_get_priority_max(sched_policy));
	return OK;
}

//************************************************************************
//  test_tasks
//
// This function sequences through a series of actions to exercise
// the various features and characteristics of v2pthreads tasks

int start_tasks()
{
	TRACEF();
	// start consumer tasks 

	CHK(task2_id = taskSpawn("TSK2", 20, 0, 0, task2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	CHK(task3_id = taskSpawn("TSK3", 20, 0, 0, task3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	CHK(task4_id = taskSpawn("TSK4", 20, 0, 0, task4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	CHK(task5_id = taskSpawn("TSK5", 15, 0, 0, task5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	CHK(task6_id = taskSpawn("TSK6", 15, 0, 0, task6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	CHK(task7_id = taskSpawn("TSK7", 15, 0, 0, task7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	CHK(task8_id = taskSpawn("TSK8", 10, 0, 0, task8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	CHK(task9_id = taskSpawn("TSK9", 10, 0, 0, task9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	CHK(task10_id = taskSpawn("TSKA", 10, 0, 0, task10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	TRACEF("finished");
	return OK;
}

int test_tasks(void)
{
	STATUS status = 0;
	int oldpriority;
	int i, num_tasks;
	TRACEF();

	//  Task Creation/Start and Preemptibility Control Test
	//TRACEF(".......... Next call taskLock to make Task 1 non-preemptible.");
	taskLock();

	taskLock_test();
	//display_task(stderr,test_child_id);
	//taskList(stderr,0);
	printf("");

	TRACEF();
	//status_show();
	//taskList(stderr,0);

	// TODO: test taskLock/taskUnlock
	taskUnlock();
	//taskShow(stderr, task_for(test_child_id));
	test_tasks_tmp();
	start_tasks();
	CHK0(test_tasks_suspend());
	CHK0(test_tasks_delete());
	CHK0(test_tasks_priority());
	CHK0(test_tasks_restart());
	CHK0(test_tasks_ident());
  exit:;
	TRACEF("finished");
	return status;
}
