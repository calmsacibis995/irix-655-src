#!smake
#
# Top level Makefile for all the LINC stuff...
#
# $Revision: 1.3 $
#

DEPTH=../..

include $(ROOT)/usr/include/make/commondefs

SUBDIRS= lib lincprom firmware

COMMONPREF=linc

default install $(COMMONTARGS): $(_FORCE)
	$(SUBDIRS_MAKERULE)

include $(COMMONRULES)

$(SUBDIRS): $(_FORCE)
	@$(ECHO) "====\t$@"; cd $@; $(MAKE)
