######################################################################
##                
## Copyright (C) 2001-2002,  Karlsruhe University
##                
## File path:     Makefile
## Description:   Create documents (PS,PDF) from SGML source
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.3 2002/02/08 21:04:56 ud3 Exp $
##                
######################################################################

DB2PDF=	db2pdf
DB2PS=	db2ps

RM_F=		rm -f
CP=		cp
TOUCH=		touch
SORT=		sort -bfi
MKDIR=		mkdir
SHELL=		bash

TARGET=		design

all:	$(TARGET).pdf

everything: ps pdf

ps: $(TARGET).ps
pdf: $(TARGET).pdf

%.pdf:	%.sgml
	@(which $(DB2PDF) > /dev/null 2>&1) || \
	 (echo "*** You need to install DocBook stylesheets ***"; \
	 exit 1)
	$(DB2PDF) $<

%.ps:	%.sgml
	@(which $(DB2PS) > /dev/null 2>&1) || \
	 (echo "*** You need to install DocBook stylesheets ***"; \
	 exit 1)
	$(DB2PS) $<

realclean: clean
	${RM_F} ${TARGET}.pdf ${TARGET}.ps

