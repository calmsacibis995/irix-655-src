#
# Makefile for Challenge HIPPI and Origin HIPPI-Serial firmware
#

# $Revision: 1.1 $ $Date: 1996/12/04 06:28:10 $

include	$(ROOT)/usr/include/make/commondefs

# Subdirs are
#    ehip - f/w for Challenge HIO HIPPI cards.
#    rr   - Road runner f/w (for RRs on the XIO HIPPI-Serial card)
#           This must be made before linc as it gets picked up by the
#           LINC fw.
#    linc - fw for the 4640s on the XIO HIPPI-Serial card. 

SUBDIRS= ehip rr linc

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
