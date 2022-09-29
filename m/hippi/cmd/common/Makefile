#!smake
#
# Makefile for hippi/cmd
#
# $Revision: 1.6 $

include $(ROOT)/usr/include/make/commondefs

# LCOPTS = -g

LCINCS = -I ../../kern

TARGETS= hipmap hiptest

MACHTAGS=-idb "mach(CPUBOARD=IP19 CPUBOARD=IP21 CPUBOARD=IP25 CPUBOARD=IP27 CPUBOARD=IP30)"

default: $(TARGETS)

include $(COMMONRULES)

install: default 
	$(INSTALL) $(MACHTAGS) -F /usr/etc $(TARGETS)
