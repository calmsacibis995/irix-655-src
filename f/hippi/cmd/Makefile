#
# Makefile for Challenge HIPPI and Origin HIPPI-Serial commands
#

# $Revision: 1.3 $ $Date: 1997/09/19 23:28:15 $

include	$(ROOT)/usr/include/make/commondefs

# Subdirs are
#    common - commands common to both HIPPI cards
#    xio    - commands specific to the Origin Xtalk-IO HIPPI-Serial card
#    hio    - commands specific to the Challenge HIO HIPPI card.

SUBDIRS= common hio xio

#HEADERS_SUBDIRS=
#EXPORTS_SUBDIRS=

default install $(COMMONTARGS): $(_FORCE)
	$(SUBDIRS_MAKERULE)

headers: $(_FORCE)
	$(HEADERS_SUBDIRS_MAKERULE)

exports: $(_FORCE)
	$(EXPORTS_SUBDIRS_MAKERULE)

$(SUBDIRS): $(_FORCE)
	@$(ECHO) "====\t$@"; cd $@; $(MAKE)

$(_FORCE):
