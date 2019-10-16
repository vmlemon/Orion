######################################################################
#                
# Copyright (C) 1999, 2000, 2001,  Karlsruhe University
#                
# File path:     config/Makefile
# Description:   Makefile for pistachio configurator
#                
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#                
# $Id: Makefile,v 1.11 2004/11/29 17:02:14 skoglund Exp $
#                
######################################################################

CMLPATH=	$(SRCDIR)/../contrib/cml2
CMLCOMPILE=	@$(CMLPATH)/cmlcompile.py
CMLCONFIG=	@$(CMLPATH)/cmlconfigure.py
CONFIGTRANS=	@$(CMLPATH)/configtrans.py

INPUT=		$(addprefix $(SRCDIR)/config/, rules.cml \
		alpha.cml \
		amd64.cml \
		arm.cml \
		ia32.cml \
		ia64.cml \
		mips64.cml \
		powerpc.cml \
		powerpc64.cml)
CONFIG_H=	config.h

TRANSLATE=	@$(CONFIGTRANS) -h $(CONFIG_H) -s .config config.out; echo "Rewriting Makeconf.local ..."; awk -F'[_=]' '/^CONFIG_ARCH_[^_]*=y/ { ARCH=$$3 } /^CONFIG_CPU_[^_]*_[^_]*=y/ { CPU=$$4 } /^CONFIG_PLAT_[^_]*=y/ { PLATFORM=$$3 } END{ { printf(",s/^ARCH *=.*/ARCH=%s/\n,s/^CPU *=.*/CPU=%s/\n,s/^PLATFORM *=.*/PLATFORM=%s/\nwq\n", tolower(ARCH), tolower(CPU), tolower(PLATFORM)) }}' .config | ed -s $(BUILDDIR)/Makeconf.local

rules.out:	$(INPUT)
	$(CMLCOMPILE) $<

config:	rules.out
	$(CMLCONFIG) -i config.out $<
	$(TRANSLATE)

batchconfig:	rules.out
	$(CMLCONFIG) -b -i config.out $<
	$(TRANSLATE)

ttyconfig:	rules.out
	$(CMLCONFIG) -t -i config.out $<
	$(TRANSLATE)

menuconfig:	rules.out
	$(CMLCONFIG) -c -i config.out $<
	$(TRANSLATE)

xconfig:	rules.out
	$(CMLCONFIG) -x -i config.out $<
	$(TRANSLATE)
