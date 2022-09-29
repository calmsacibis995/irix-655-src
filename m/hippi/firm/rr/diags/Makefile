#!smake
#
# Makefile for HIPPI-Serial card's Roadrunner Firmware
#
# $Revision: 1.2 $  $Date: 1996/12/13 05:45:44 $
#

include $(ROOT)/usr/include/make/commondefs

include ../../../hippi.defs

TARGETS=walk1.fw deqa.fw deqna.fw

LISTINGS=walk1.list deqa.list deqna.list

LDIRT=*.o *.list

GASRR=../../../tools/gas/gasrr
GASOD=../../../tools/gas/objdumprr
NAWK=nawk

default: $(TARGETS)

install: $(TARGETS)
	$(INSTALL) -idb noship -m 444 $(XIOMACHTAGS) \
		-F /usr/local/lib/hippi-noship/firm/rr_diags \
		$(TARGETS) $(LISTINGS) README diag.tcl

.SUFFIXES:	.fw

.o.fw:
	$(GASOD) -s $*.o | \
	$(NAWK) -f raw.awk - > $*.fw

.s.o:	diag.h
	$(GASRR) -als -o $*.o $*.s > $*.list

include $(COMMONRULES)
