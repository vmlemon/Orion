/****************************************************************************
 * Copyright (C) 2006 v2lin Team <http://v2lin.sf.net>
 * 
 * This file is part of the v2lin Library.
 * VxWorks is a registered trademark of Wind River Systems, Inc.
 * 
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

#include "tickLib.h"
#include "v2ldebug.h"
#include <time.h>
#include <stdio.h>

int sysClkRateGet(void)
{
	struct timespec res;
	clock_getres(CLOCK_REALTIME, &res);
	TRACEV("%i", res.tv_nsec);
	return 1E9 / res.tv_nsec;
}

static int long long offset;

void tickSet(ULONG ticks)
{
	TRACEF();
	FILE *f = fopen("/proc/uptime", "r");
	double uptime = 0;
	fscanf(f, "%lf", &uptime);
	TRACEV("%lf", uptime);
	fclose(f);
	offset = ticks - uptime * 1000 / V2PT_TICK;
}

ULONG tickGet(void)
{
	TRACEF();
	FILE *f = fopen("/proc/uptime", "r");
	double uptime = 0;
	fscanf(f, "%lf", &uptime);
	TRACEV("%lf", uptime);
	fclose(f);
	return offset + uptime * 1000 / V2PT_TICK;
}
