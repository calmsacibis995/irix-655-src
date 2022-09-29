#!smake
#
# Makefile for hippi_s/tools/misc
#
# $Revision: 1.5 $ $Date: 1997/02/26 08:05:58 $

include $(ROOT)/usr/include/make/commondefs

TARGETS = dis_rr dumpbridge

XIOMACHTAGS=-idb "mach(CPUBOARD=IP27 CPUBOARD=IP30)"

LCOPTS = -g

default: $(TARGETS) $(NOSHIPPROGS)

include $(COMMONRULES)

install: default 
	$(INSTALL) -idb noship $(XIOMACHTAGS) -F /usr/local/bin -src dumpbridge hipdumpbridge
	$(INSTALL) -idb noship $(XIOMACHTAGS) -m 744 -u root -g sys -F /usr/bin hipgetmac
