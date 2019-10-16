/****************************************************************************
 * Copyright (C) 2004, 2005, 2006 v2lin Team <http://v2lin.sf.net>
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

#include <sysLib.h>
#include <tickLib.h>
#include <v2ldebug.h>

void user_sysinit(void)
{
	TRACEF();
}

void user_syskill()
{
	TRACEV("%i",sysClkRateGet());
	TRACEV("%i",tickGet());
	tickSet(1000000000);
	TRACEV("%i",tickGet());
	taskDelay(1);
	TRACEV("%i",tickGet());
	taskDelay(10);
	TRACEV("%i",tickGet());
}
