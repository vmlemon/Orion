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


int test_child_id;

MSG_Q_ID queue1_id;
MSG_Q_ID queue2_id;
MSG_Q_ID queue3_id;


/*****************************************************************************
**  test_msg_queues
**         This function sequences through a series of actions to exercise
**         the various features and characteristics of v2pthreads message
**         queues.
**
*****************************************************************************/
int test_msg_queues(void)
{
	STATUS err;
	int message_num;
	int msg_count;
	msgblk_t msg;
	msgblk_t rcvd_msg;
	char msg_string[80];
	TRACEF();

	CHK(queue1_id = msgQCreate(9, 16, MSG_Q_PRIORITY));
	CHK(queue2_id = msgQCreate(4, 128, MSG_Q_PRIORITY));
	CHK(queue3_id = msgQCreate(0, 16, MSG_Q_PRIORITY));

	// First we created three message queues
	// Next we enable Tasks 3, 6, and 9 to consume
	// messages from MSQ1 in reverse-priority order.
	// The enables are sent to lowest-priority tasks first.

	//puts("Task 1 enabling Tasks 3, 6, and 9 to consume MSQ1 messages.");
	CHK0(semGive(enable3));
	CHK0(semGive(enable6));
	CHK0(semGive(enable9));

	//  Delay to allow consumer tasks to recognize enable signals.
	taskDelay(2);

	// Next we attempt to send nine messages to each queue
	// This tests message queue full logic.
	// The message queue MSQ1 should return no errors
	// but MSQ2 should return five 0x3d0002 errs
	// and MSQ3 should return nine 0x3d0002 errs

	// This is a 'sneaky trick' to null-terminate the object name string.
	msg.msg.nullterm = (ulong) NULL;
	msg.msg.qname[0] = 'M';
	msg.msg.qname[1] = 'S';
	msg.msg.qname[2] = 'Q';
	//  Post a unique message to each of the message queues
	//msg.msg.t_cycle = test_cycle;
	for (message_num = 1; message_num < 10; message_num++) {
		msg.msg.msg_no = message_num;
		msg.msg.qname[3] = '1';
		CHK0(msgQSend(queue1_id, msg.blk, 16, NO_WAIT, MSG_PRI_NORMAL));
	}
	for (message_num = 1; message_num < 5; message_num++) {
		msg.msg.msg_no = message_num;
		msg.msg.qname[3] = '2';
		CHK0(msgQSend(queue2_id, msg.blk, 16, NO_WAIT, MSG_PRI_NORMAL));
	}
	for (message_num = 5; message_num < 10; message_num++) {
		msg.msg.msg_no = message_num;
		msg.msg.qname[3] = '2';
		CHK(-1 == msgQSend(queue2_id, msg.blk, 16, NO_WAIT, MSG_PRI_NORMAL));
		CHK(errno == S_objLib_OBJ_UNAVAILABLE);
	}
	for (message_num = 1; message_num < 10; message_num++) {
		msg.msg.msg_no = message_num;
		msg.msg.qname[3] = '3';
		CHK(-1 == msgQSend(queue3_id, msg.blk, 16, NO_WAIT, MSG_PRI_NORMAL));
		CHK(errno == S_objLib_OBJ_UNAVAILABLE);
	}

	/*Sending a message to a message queue which
	   is larger than the queue's maximum message size would
	   either have to truncate the message or cause buffer
	   overflow - neither of which is desirable.  For this
	   reason, an attempt to do this generates an error 0x410001.
	   This tests the overlength message detection logic.
	 */
	CHK(-1 == msgQSend(queue1_id, msg_string, 80, NO_WAIT, MSG_PRI_NORMAL));
	CHK(errno == S_msgQLib_INVALID_MSG_LENGTH);
	/*
	   Receiving a message from a message queue which
	   is larger than the caller's message buffer size would
	   either have to truncate the message or cause buffer
	   overflow - neither of which is desirable.  For this
	   reason, an attempt to do this generates an error 0x410001.
	   This tests the underlength buffer detection logic.
	 */
	CHK(-1 == msgQReceive(queue2_id, rcvd_msg.blk, 16, NO_WAIT));
	CHK(errno == S_msgQLib_INVALID_MSG_LENGTH);
	//printf("16-byte msgQReceive for 128-byte MSQ2 returned error %x\n", errno);
	/************************************************************************
	 **  Waiting Task 'Queuing Order' (FIFO vs. PRIORITY) Test
	 ************************************************************************/
	/*
	   puts(".......... Earlier we enabled Tasks 3, 6, and 9 to consume");
	   puts("           messages from MSQ1 in reverse-priority order.");
	   puts("           The enables were sent to lowest-priority tasks first.");
	   puts("           Since the queues awaken tasks in PRIORITY order, this");
	   puts("           tests the task queueing order logic.\n");
	   puts("           Tasks 9, 6, and 3 - in that order - should each");
	   puts("           receive 3 messages from MSQ1");

	   puts("Task 1 blocking while messages are consumed...");
	   puts("Task 1 waiting to receive ALL of complt3, complt6, complt9 tokens.");
	 */
	CHK0(semTake(complt3, WAIT_FOREVER));	//1 
	CHK0(semTake(complt6, WAIT_FOREVER));
	CHK0(semTake(complt9, WAIT_FOREVER));
	/*
	   puts(".......... Next we send a message to zero-length MSQ3 with");
	   puts("           Task 9 waiting on MSQ3... This should succeed.");
	   puts("           This tests the zero-length queue send logic.");

	   puts("Task 1 enabling Task 9 (priority 10) to consume MSQ3 messages.");
	 */
	CHK0(semGive(enable9));

	//puts("Task 1 blocking for handshake from Task 9...");
	CHK0(semTake(complt9, WAIT_FOREVER));
	taskDelay(2);

	//printf("Task 1 Sending msg %d to %s", message_num, msg.msg.qname);
	msg.msg.msg_no = message_num;
	CHK0(msgQSend(queue3_id, msg.blk, 16, NO_WAIT, MSG_PRI_NORMAL));

	//puts("Task 1 blocking while message is consumed...");
	CHK0(semTake(complt9, WAIT_FOREVER));

	/************************************************************************
	 **  Message Queue-Delete Test
	 ************************************************************************/
	/*
	   puts(".......... Next we enable Tasks 3, 6, and 9 to wait for");
	   puts("           a message on MSQ1.  Then we delete MSQ1.");
	   puts("           This should wake each of Tasks 3, 6, and 9,");
	   puts("           and they should each return an error 0x3d0003.");
	   puts("           This tests the queue delete logic.");

	   puts("Task 1 enabling Tasks 3, 6, and 9 to consume MSQ1 messages.");
	 */
	CHK0(semGive(enable3));
	CHK0(semGive(enable6));
	CHK0(semGive(enable9));

	//puts("Task 1 blocking for handshake from Tasks 3, 6, and 9...");
	CHK0(semTake(complt3, WAIT_FOREVER));	// 2
	CHK0(semTake(complt6, WAIT_FOREVER));
	CHK0(semTake(complt9, WAIT_FOREVER));
	taskDelay(2);

	//puts("Task 1 deleting MSQ1");
	CHK0(msgQDelete(queue1_id));

	//puts("Task 1 blocking until consumer tasks acknowledge deletion...");
	//puts("Task 1 waiting to receive ALL of complt3, complt6, complt9 tokens.");
	CHK0(semTake(complt3, WAIT_FOREVER));	// 3 see test_queue_delete
	CHK0(semTake(complt6, WAIT_FOREVER));
	CHK0(semTake(complt9, WAIT_FOREVER));

	/************************************************************************
	 **  Message Queue Wait-to-Send Test
	 ************************************************************************/
	/*
	   puts(".......... During the queue-full tests above, four messages");
	   puts("           were sent, filling message queue MSQ2.");
	   puts("           Now we will attempt to send another message and ");
	   puts("           Task 1 will block waiting on room in the queue for");
	   puts("           the new message.  After a short delay, a consumer task");
	   puts("           will receive the first message in the queue, creating");
	   puts("           space for the new message and unblocking Task 1.");

	   puts("Task 1 enabling Task 3 to consume one MSQ2 message after delay.");
	 */
	CHK0(semGive(enable3));
	//puts("Task 1 blocking for handshake from Task 3...");
	CHK0(semTake(complt3, WAIT_FOREVER));	// 4

	msg.msg.msg_no = ++message_num;
	msg.msg.qname[3] = '2';
	//printf("Task 1 waiting indefinitely to send msg %d to %s", message_num, msg.msg.qname);
	CHK0(msgQSend(queue2_id, msg.blk, 16, WAIT_FOREVER, MSG_PRI_NORMAL));
	taskDelay(10);
	/*
	   puts(".......... Next we will attempt to send another message and ");
	   puts("           Task 1 will block waiting on room in the queue for");
	   puts("           the new message.  After a 1 second delay, Task 1");
	   puts("           will quit waiting for space and will return an error");
	   puts("           message 0x3d0004.");
	 */
	msg.msg.msg_no = ++message_num;
	msg.msg.qname[3] = '2';
	CHK(-1 == msgQSend(queue2_id, msg.blk, 16, 100, MSG_PRI_NORMAL));
	CHK(errno==S_objLib_OBJ_TIMEOUT);
	/*
	   puts(".......... Next Task 6 will attempt to send a message to MSQ3.");
	   puts("           Task 6 will block waiting on room in the queue for");
	   puts("           the new message.  After a 1 second delay, Task 1");
	   puts("           will delete MSQ3 and Task 6 will return an error");
	   puts("           message 0x3d0003.");

	   puts("Task 1 enabling Task 6 to send one MSQ3 message.");
	 */
	CHK0(semGive(enable6));
	//puts("Task 1 blocking for handshake from Task 6...");
	CHK0(semTake(complt6, WAIT_FOREVER));

	message_num++;
	taskDelay(100);

	//puts("Task 1 deleting MSQ3 with Task6 waiting for queue space");
	CHK0(msgQDelete(queue3_id));

	//puts("Task 1 blocking for handshake from Task 6...");
	CHK0(semTake(complt6, WAIT_FOREVER));

	/************************************************************************
	 **  Message Queue Urgent Message Test
	 ************************************************************************/
	/*
	   puts(".......... During the queue-full tests above, four messages");
	   puts("           were sent, filling message queue MSQ2.");
	   puts("           Now we will send an urgent message and then enable");
	   puts("           a consumer task to receive all the messages in MSQ2.");
	   puts("           The consumer task should receive five messages in all");
	   puts("           from MSQ2, starting with the urgent message.");
	 */
	msg.msg.msg_no = ++message_num;
	msg.msg.qname[3] = '2';
	//printf("Task 1 Sending urgent msg %d to %s", message_num, msg.msg.qname);
	CHK0(msgQSend(queue2_id, msg.blk, 16, NO_WAIT, MSG_PRI_URGENT));

	//puts("Task 1 enabling Task 6 to consume MSQ2 messages.");
	CHK0(semGive(enable6));
	//puts("Task 1 blocking for handshake from Task 6...");
	CHK0(semTake(complt6, WAIT_FOREVER));

	//puts("Task 1 blocking while messages are consumed...");
	CHK0(semTake(complt6, WAIT_FOREVER));

	/************************************************************************
	 **  Message Queue Number of Messages and Queue-Not_Found Test
	 ************************************************************************/
	//puts(".......... Finally, we test the msgQNumMsgs logic...");
	//puts("           Then we verify the error codes returned when");
	//puts("           a non-existent queue is specified.");

	CHK(0 <= msgQNumMsgs(queue2_id));

	CHK(-1 == msgQNumMsgs(queue1_id));
	TRACEV("%i",msg_count);
	//if (msg_count == ERROR)
	//  printf("msgQNumMsgs for MSQ1 returned error\n");
	//else
	//  printf("msgQNumMsgs for MSQ1 returned %d messages\n", msg_count);

	CHK(-1 == msgQSend(queue1_id, msg.blk, 16, NO_WAIT, MSG_PRI_NORMAL));
	CHK(errno==S_objLib_OBJ_ID_ERROR);

	CHK(-1 == msgQReceive(queue1_id, rcvd_msg.blk, 16, NO_WAIT));
	CHK(errno==S_objLib_OBJ_ID_ERROR);

	CHK(-1 == msgQReceive(queue1_id, rcvd_msg.blk, 16, WAIT_FOREVER));
	CHK(errno==S_objLib_OBJ_ID_ERROR);

	CHK(-1 == msgQDelete(queue1_id));
	CHK(errno==S_objLib_OBJ_ID_ERROR);
	TRACEF("finished");
	return OK;
}

union
{
	char blk[128];
	my_qmsg_t msg;
} bigmsg;

int task6(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
		  int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	msgblk_t msg;
	TRACEF();
	int i;

	/************************************************************************
	 **  First wait on empty MSQ1 in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	// puts("Task 6 waiting on enable6 to begin receive on MSQ1");
	CHK0(semTake(enable6, WAIT_FOREVER));

	for (i = 0; i < 3; i++) {
		CHK(16 == msgQReceive(queue1_id, msg.blk, 16, WAIT_FOREVER));
		//printf("Task 6 rcvd Test Cycle %d Msg No. %d from %s\n",
		//          msg.msg.t_cycle, msg.msg.msg_no, msg.msg.qname);
	}
	//puts("Signalling complt6 to Task 1 - Task 6 finished queuing order test.");
	CHK0(semGive(complt6));

	/************************************************************************
	 **  Now wait along with other tasks on empty MSQ1 to demonstrate
	 **  queue delete behavior.
	 ************************************************************************/
	//puts("Task 6 waiting on enable6 to begin receive on MSQ1");
	CHK0(semTake(enable6, WAIT_FOREVER));

	CHK0(semGive(complt6));

	CHK(-1 == msgQReceive(queue1_id, msg.blk, 16, 100));
	CHK(errno == S_objLib_OBJ_DELETED);
	CHK0(semGive(complt6));

	// Now send a message to MSQ3 to demonstrate wait-to-send behavior during queue deletion.
	CHK0(semTake(enable6, WAIT_FOREVER));

	// puts("Task 6 signalling complt6 to Task 1 to indicate Task 6 ready.");
	CHK0(semGive(complt6));
	msg.msg.msg_no = 13;
	msg.msg.qname[3] = '3';
	CHK(-1 == msgQSend(queue3_id, msg.blk, 16, WAIT_FOREVER, MSG_PRI_NORMAL));
	CHK(errno == S_objLib_OBJ_DELETED);

	CHK0(semGive(complt6));

	// Now wait for messages on MSQ2 to demonstrate urgent message send  behavior.
	CHK0(semTake(enable6, WAIT_FOREVER));

	// puts("Task 6 signalling complt6 to Task 1 to indicate Task 6 ready.");
	CHK0(semGive(complt6));

	//  Consume messages until no available messages remain.
	while (1) {
		int len;
		CHK(0 < (len = msgQReceive(queue2_id, bigmsg.blk, 128, NO_WAIT)));
		if (len != 128)
			break;
		//  printf("Task 6 rcvd Test Cycle %d Msg No. %d from %s\n",
		//          bigmsg.msg.t_cycle, bigmsg.msg.msg_no, bigmsg.msg.qname);
	}
	CHK0(semGive(complt6));

	CHK0(taskDelete(0));

	return (0);
}

int task9(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
		  int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	STATUS err;
	msgblk_t msg;
	int i;
	TRACEF();

	/************************************************************************
	 **  First wait on empty MSQ1 in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	CHK0(semTake(enable9, WAIT_FOREVER));

	for (i = 0; i < 3; i++) {
		CHK(0 < msgQReceive(queue1_id, msg.blk, 16, WAIT_FOREVER));
		//printf("Task 9 rcvd Test Cycle %d Msg No. %d from %s\n",
		//         msg.msg.t_cycle, msg.msg.msg_no, msg.msg.qname);
	}
	//puts("Signalling complt9 to Task 1 - Task 9 finished queuing order test.");
	CHK0(semGive(complt9));

	/************************************************************************
	 **  Next wait on empty MSQ3 to demonstrate q_send to zero-length queue.
	 ************************************************************************/
	CHK0(semTake(enable9, WAIT_FOREVER));

	CHK0(semGive(complt9));

	CHK(16 == msgQReceive(queue3_id, msg.blk, 16, 100));
	CHK(-1 == msgQReceive(queue3_id, msg.blk, 16, 100));
	CHK(errno==S_objLib_OBJ_TIMEOUT);
	CHK0(semGive(complt9));

	/************************************************************************
	 **  Now wait along with other tasks on empty MSQ1 to demonstrate
	 **  queue delete behavior.
	 ************************************************************************/
	CHK0(semTake(enable9, WAIT_FOREVER));
	CHK0(semGive(complt9));
	while (1) {
		int len;
		CHK(len == (len = msgQReceive(queue1_id, msg.blk, 16, 100)));
		if (len != 16)
			break;
		TRACEF("Task 9 rcvd Test Cycle %d Msg No. %d from %s",
			   msg.msg.t_cycle, msg.msg.msg_no, msg.msg.qname);
	}
	CHK0(semGive(complt9));
	CHK0(taskDelete(0));
	return (0);
}

int test_queue_order_wait()
{
	int i;
	msgblk_t msg;
	/************************************************************************
	 **  First wait on empty MSQ1 in pre-determined task order to test
	 **  task wait-queueing order ( FIFO vs. PRIORITY ).
	 ************************************************************************/
	//puts("Task 3 waiting on enable3 to begin receive on MSQ1");
	CHK0(semTake(enable3, WAIT_FOREVER));

	for (i = 0; i < 3; i++) {
		CHK(16 == msgQReceive(queue1_id, msg.blk, 16, WAIT_FOREVER));
	}
	CHK0(semGive(complt3));		// 1
	return OK;
}

int test_queue_delete()
{
	msgblk_t msg;
	/************************************************************************
	 **  Now wait along with other tasks on empty MSQ1 to demonstrate
	 **  queue delete behavior.
	 ************************************************************************/
	// puts("Task 3 waiting on enable3 to begin receive on MSQ1");
	CHK0(semTake(enable3, WAIT_FOREVER));
	//puts("Task 3 signalling complt3 to Task 1 to indicate Task 3 ready.");
	CHK0(semGive(complt3));		// 2

	// Consume messages until 1 second elapses without an available message.
	// puts("Task 3 waiting up to 1 sec to receive msgs on MSQ1");
	while (1) {
		int len;
		CHK(-1 ==  (len = msgQReceive(queue1_id, msg.blk, 16, 100)));
		CHK(errno == S_objLib_OBJ_DELETED);
		if (len < 0)
			break;
	}
	CHK0(semGive(complt3));		// 3
	CHK0(semTake(enable3, WAIT_FOREVER));
	CHK0(semGive(complt3));		// 4
	taskDelay(100);
	CHK(0 < msgQReceive(queue2_id, bigmsg.blk, 128, 100));
	CHK0(semGive(complt3));		// 5
	return OK;
}

int task3(int dummy0, int dummy1, int dummy2, int dummy3, int dummy4,
		  int dummy5, int dummy6, int dummy7, int dummy8, int dummy9)
{
	STATUS err;
	TRACEF();
	int i;
	test_queue_order_wait();
	test_queue_delete();
	CHK0(taskDelete(0));
	return (0);
}
