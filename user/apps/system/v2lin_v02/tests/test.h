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


int task1(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
int task2(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
int task3(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
int task4(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
int task5(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
int task6(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
int task7(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
int task8(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
int task9(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
int task10(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);

extern int task2_id;
extern int task3_id;
extern int task4_id; 
extern int task5_id;
extern int task6_id;
extern int task7_id; 
extern int task8_id;
extern int task9_id;
extern int task10_id;

extern MSG_Q_ID queue1_id;
extern MSG_Q_ID queue2_id;
extern MSG_Q_ID queue3_id;

extern SEM_ID mutex1_id;
extern SEM_ID mutex2_id;
extern SEM_ID mutex3_id;

extern SEM_ID enable1;
extern SEM_ID enable2;
extern SEM_ID enable3;
extern SEM_ID enable4;
extern SEM_ID enable5;
extern SEM_ID enable6;
extern SEM_ID enable7;
extern SEM_ID enable8;
extern SEM_ID enable9;
extern SEM_ID enable10;

extern SEM_ID complt1;
extern SEM_ID complt2;
extern SEM_ID complt3;
extern SEM_ID complt4;
extern SEM_ID complt5;
extern SEM_ID complt6;
extern SEM_ID complt7;
extern SEM_ID complt8;
extern SEM_ID complt9;
extern SEM_ID complt10;

extern SEM_ID sem1_id;
extern SEM_ID sem2_id;
extern SEM_ID sem3_id;

typedef struct qmessage
{
	char qname[4];
	ulong nullterm;
	int t_cycle;
	int msg_no;
} my_qmsg_t;

typedef union
{
	char blk[16];
	my_qmsg_t msg;
} msgblk_t;

extern int wdog1_cycle;
extern int wdog2_cycle;

extern unsigned char task5_restarted;
extern int temp_taskid;
extern int test_cycle;


int test_tasks_restart();
STATUS test_semaphores();
int test_mutexes(void);
int test_msg_queues(void);
int test_watchdog_timers(void);
int test_tasks(void);
