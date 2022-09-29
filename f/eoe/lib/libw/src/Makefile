#!smake
#ident "$Revision: 1.8 $"

.NOTPARALLEL:
include $(ROOT)/usr/include/make/commondefs

### For I18N support
MAKE=smake I18N_MAKE=$(I18N_MAKE)

SUBDIRS= wmisc wstring

default install $(COMMONTARGS)!
	$(SUBDIRS_MAKERULE)

.MAKE:default install $(COMMONTARGS)
