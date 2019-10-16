#  Copyright (C) 2004, 2005, 2006 v2lin Team <http://v2lin.sf.net>
#  
#  This file is part of the v2lin Library.
#  VxWorks is a registered trademark of Wind River Systems, Inc.
#  
#  Initial implementation Gary S. Robertson, 2000, 2001.
#  Contributed by Andrew Skiba, skibochka@sourceforge.net, 2004.
#  Contributed by Mike Kemelmakher, mike@ubxess.com, 2005.
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
#  

all:
	make -C lib
	make -C samples
	make -C tests

test:
	make -C tests run

tgz:
	make distclean
	tar czf ../`basename $(PWD)`.tgz -C .. `basename $(PWD)`
	
TODO:
	grep TODO -w . -rn -1 --color

include rules.mk
-include ~/rules.mk

clean:
	make -C lib clean
	make -C tests clean
	make -C samples clean
	rm -f .deps.mk


defined:
	@echo List of defined finctions:
	@ctags -x --c-kinds=p --file-scope=no lib/*.h | cut -f 1 -d ' '
