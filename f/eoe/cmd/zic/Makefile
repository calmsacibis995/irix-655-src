#!smake
#
# Makefile for eoe/cmd/zic
#

include $(ROOT)/usr/include/make/commondefs

TZDIR = /usr/lib/locale/TZ

CFILES = ialloc.c scheck.c zic.c 

FILES =	africa asia australasia etcetera europe factory \
	northamerica pacificnew solar87 solar88 solar89 \
	southamerica backward 

LCOPTS = -fullwarn

YEARSRC = yearistype.sh

YEARCMD = yearistype

TARGETS = zic $(YEARCMD)

default: $(TARGETS) 

include $(COMMONRULES)

zic:	$(OBJECTS)
	$(CCF) $(LCOPTS) $(OBJECTS) -o $@ $(LDFLAGS)

# Install zic  
# Create directory and copy in all data files
# Build TZ data file in tz sub dir, do install 

install: default $(YEARSRC)
	$(INSTALL) -F /usr/bin $(TARGETS)

	for i in $(FILES); do \
            ./zic -d $(ROOT)/$(TZDIR) $$i; \
        done

	$(INSTALL) -F $(TZDIR) -m 444 $(FILES) 
