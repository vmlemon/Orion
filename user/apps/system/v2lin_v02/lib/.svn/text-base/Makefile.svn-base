#  Copyright (C) 2004, 2005, 2006 v2lin Team <http://v2lin.sf.net>
#
#  This file is part of the v2lin Library.
#  VxWorks is a registered trademark of Wind River Systems, Inc.
#
#  Contributed by Constantine Shulyupin, conan.sh@gmail.com, 2006.
#
#  The v2lin library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  The v2lin Library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.

top=..

include $(top)/defs.mk
include defs.mk

LIB_SHORT = v2lin
LIB_FULL = lib$(LIB_SHORT).so

TARGETS=$(LIB_FULL) libv2linmain.so

LDFLAGS+=-lpthread -L. #-l$(LIB_SHORT)
#LDFLAGS+=-lrt # for clock_getres  

#export LD_LIBRARY_PATH=.

all: .deps.mk $(TARGETS)

$(LIB_FULL): lkernelLib.o ltaskLib.o lmsgQLib.o lsemLib.o lwdLib.o v2ldebug.o v2ltime.o

libv2linmain.so: main_impl.o

show:
	# LIB_FULL $($(LIB_FULL)

include ../rules.mk
