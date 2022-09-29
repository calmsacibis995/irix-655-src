#!smake
#
# Makefile for hippi_s/tools/tcl
#
# $Revision: 1.9 $ $Date: 1997/07/16 08:15:19 $

OBJECT_STYLE = 32

include $(ROOT)/usr/include/make/commondefs

TARGETS = 
NOSHIPPROGS = hipdbg

XIOMACHTAGS=-idb "mach(CPUBOARD=IP27 CPUBOARD=IP30)"

# 5.3 and 6.3 locations of tcl, define TCLDIR accordingly
OTCLDIR = $(ROOT)/usr/freeware
NTCLDIR = $(ROOT)/usr/sgitcl
TCLDIR = $(NTCLDIR)
FIRMDIR = ../../firm/linc
KERNDIR = ../../kern

LDIRT = $(NOSHIPPROGS)

LCINCS = -I. -I$(TCLDIR)/include -I$(FIRMDIR)/include -I$(KERNDIR)/sys
LCOPTS = -g
# define G2P_SIM for Indigo2 gio2pci simulation
#LCDEFS = -DRR_DEBUG -DG2P_SIM
LCDEFS = -DRR_DEBUG -DBRIDGE_B_WAR -DHIPPI_BP

LLDOPTS = -L $(TCLDIR)/lib 
LLDLIBS = -B static -ltcl -B dynamic -lm

default: $(TARGETS) $(NOSHIPPROGS)

include $(COMMONRULES)

install: default 
	$(INSTALL) -idb noship $(XIOMACHTAGS) -F /usr/local/bin $(NOSHIPPROGS)


