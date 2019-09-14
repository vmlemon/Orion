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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "v2lpthread.h"
#include "vxw_hdrs.h"
#include <v2ldebug.h>

char **_argv;

int main(int argc, char **argv)
{
	_argv = argv;
	TRACEF("{");
	v2lin_init();
	TRACEF("}");
	return 0;
}
