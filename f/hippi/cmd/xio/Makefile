#!smake
#
# Makefile for hippi/cmd
#
# $Revision: 1.14 $

include $(ROOT)/usr/include/make/commondefs

TARGETS=hipcntl
NOSHIPPROGS= hippi_src.firm hippi_dst.firm\
	 lincprom_dst.firm lincprom_src.firm
CONVERT= ./elf2hipfw 

#LCOPTS = -g
LCINCS = -I ../../kern -I ../../firm/linc/include
LDIRT = $(NOSHIPPROGS)

SRC_FW=   ../../firm/linc/firmware/src_hippilincfw
DST_FW=   ../../firm/linc/firmware/dst_hippilincfw
LINCPROM= ../../firm/linc/lincprom/lincprom

OFFSET=0xbfc08000
LINCPROM_OFFSET=0xbfc00000

MACHTAGS=-idb "mach(CPUBOARD=IP27 CPUBOARD=IP30)"

default: $(NOSHIPPROGS) $(TARGETS)

include $(COMMONRULES)

LLDLIBS = -lelf

install: default 
	$(INSTALL) $(MACHTAGS) -F /usr/etc $(TARGETS)

#ifndef SRC_PRD
hippi_src.firm: elf2hipfw $(SRC_FW)
	$(CONVERT) -s -f $(SRC_FW) $(OFFSET) 

hippi_dst.firm: elf2hipfw $(DST_FW)
	$(CONVERT) -d -f  $(DST_FW) $(OFFSET)

lincprom_src.firm: elf2hipfw $(LINCPROM)
	$(CONVERT) -s  $(LINCPROM) $(LINCPROM_OFFSET)

lincprom_dst.firm: elf2hipfw $(LINCPROM)
	$(CONVERT) -d  $(LINCPROM) $(LINCPROM_OFFSET)
#endif

hipcntl.o: hippi_src.firm hippi_dst.firm lincprom_src.firm lincprom_dst.firm

hipcntl: hippi_src.firm hippi_dst.firm lincprom_src.firm lincprom_dst.firm\
          hipcntl.o hipbpcntl.o hipcntl.h
	$(CC) $(CFLAGS) $(LCOPTS) hipcntl.o hipbpcntl.o  -o hipcntl



