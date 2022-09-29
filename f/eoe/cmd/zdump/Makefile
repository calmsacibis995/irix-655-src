#!smake
#
# Makefile for eoe/cmd/zdump
#

include $(ROOT)/usr/include/make/commondefs

TZDIR = /usr/lib/locale/TZ

CFILES = zdump.c 

TARGETS = zdump

LCOPTS = -fullwarn

default: $(TARGETS) 

include $(COMMONRULES)

zdump:	$(OBJECTS)
	$(CCF) $(LCOPTS) $(OBJECTS) -o $@ $(LDFLAGS)

# Install zdump  

install: default
	$(INSTALL) -F /usr/bin $(TARGETS)


