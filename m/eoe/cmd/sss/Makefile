#
# Makefile for sss
#
#       "$Revision: 1.10 $"
#
include $(ROOT)/usr/include/make/commondefs


HEADERS_SUBDIRS=include console availmon sgmtask
EXPORTS_SUBDIRS=ssdb libssdb sgmtask
SUBDIRS = $(EXPORTS_SUBDIRS) eventmond semd notifier availmon utils configmon \
	  userenv console sgmtask 

default install $(COMMONTARGS): $(_FORCE)
	$(SUBDIRS_MAKERULE)

headers: $(_FORCE)
	$(HEADERS_SUBDIRS_MAKERULE)

exports: $(_FORCE)
	$(EXPORTS_SUBDIRS_MAKERULE)

$(SUBDIRS): $(_FORCE)
	cd $@; $(MAKE)


$(_FORCE):
