#
# Makefile for hippi/tools
#
# $Revision: 1.2 $ $Date: 1996/12/04 19:19:59 $

include	$(ROOT)/usr/include/make/commondefs

#ifndef SRC_PRD
SUBDIRS= misc tcl gas
#else
SUBDIRS= misc tcl
#endif

MAKE=make

#HEADERS_SUBDIRS=
#EXPORTS_SUBDIRS=

default install $(COMMONTARGS): $(_FORCE)
	$(SUBDIRS_MAKERULE)

headers: $(_FORCE)
	$(HEADERS_SUBDIRS_MAKERULE)

exports: $(_FORCE)
	$(EXPORTS_SUBDIRS_MAKERULE)

$(SUBDIRS): $(_FORCE)
	cd $@; $(MAKE)

$(_FORCE):
