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

LDFLAGS+=-L $(top)/lib -lpthread -ldl -l v2lin -l v2linmain -l rt
TARGETS = test test_sem test_time

#PROG = test_sem

#export LD_LIBRARY_PATH=../lib

all: $(TARGETS)

test:	test_tasks.o test_semaphores.o test_msgq.o test_watchdog.o test_mutexes.o ../lib/main_impl.o

gdb:
	export LD_LIBRARY_PATH=../lib;gdb -q --command run.gdb test
	
run:
	make -C ..
	# Running test, takes about 1 min
	export LD_LIBRARY_PATH=../lib; time nice -n 15 ./test 2> test.log; echo $$?
	export LD_LIBRARY_PATH=../lib; time ./test_sem > test_sem.log
	#export LD_LIBRARY_PATH=../lib; time nice -n 15 ./test echo $$?
	wc -l test.log
	@grep ERROR -w test.log && echo -n "Errors: " && grep ERROR -wc test.log && exit 1 \
		|| echo No errors, Test passed

include $(top)/rules.mk
