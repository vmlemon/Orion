/****************************************************************************
 * This file is part of the v2lin Library.
 * VxWorks is a registered trademark of Wind River Systems, Inc.
 * 
 * Copyright (C) 2006 Constantine Shulyupin, conan.sh@gmail.com
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
#include <unistd.h>
#include <signal.h>
#include "v2lpthread.h"
#include <stdlib.h>
#include "v2ldebug.h"
#include <sys/wait.h>

_syscall0(pid_t,gettid)

int tid;
extern char * _argv[];
char trace_fn[100];
struct timeval start_tv={0,0};
int trace = 1;
struct timeval prev_tv={0,0};

void gdb()
{
	static int gdb_pid;
	char spid[20];
	snprintf(spid, 19, "%i", getpid());
	spid[19] = 0;
	if (gdb_pid) {
		TRACEF("gdb already started");
		exit(1);
	}
	gdb_pid = fork();
	if (gdb_pid == 0) {
		// define _argv and open comment
		// execlp("gdb", "gdb", "-q",_argv[0], spid, NULL);
	} else {
		waitpid(gdb_pid, NULL, 0);
	}

}

void sighandler(int x)
{
	gdb();
}

void signal_init()
{
	signal(SIGSEGV,sighandler);
}

void trace_time()
{
	time_t time_cur;
	time(&time_cur);
	struct timeval now_tv,uptime={0,0},passed_tv={0,0};

	struct timezone tz;

	gettimeofday(&now_tv, &tz);
        if (!start_tv.tv_sec) { 
		start_tv = now_tv;
		struct tm* now_tm;
		now_tm = localtime(&time_cur);
		fprintf(stderr,"time=%04d-%02d-%02d %02d:%02d:%02d.%06d\n",
				now_tm->tm_year+1900, now_tm->tm_mon+1, now_tm->tm_mday,
				now_tm->tm_hour,now_tm->tm_min,now_tm->tm_sec, (int)now_tv.tv_usec);
	}
	time_cur = now_tv.tv_sec;
	timersub(&now_tv,&start_tv,&uptime);
	if (timerisset(&prev_tv)) timersub(&now_tv,&prev_tv,&passed_tv);

	//if ( passed > 0.01 )
	if ( !timerisset(&passed_tv) || passed_tv.tv_usec > 10000)
	{
                fprintf(stderr,"uptime=%d.%06d s ",(int)uptime.tv_sec,(int)uptime.tv_usec);
		fprintf(stderr,"+%d.%06d ",(int)passed_tv.tv_sec,(int)passed_tv.tv_usec);
		fprintf(stderr,"\n");
		prev_tv = now_tv;
	}
}
