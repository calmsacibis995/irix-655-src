#!smake
#
# Makefile for lincprom
#
# $Revision: 1.8 $
#
#

DEPTH=../../..

include ../firmcommondefs

# PTEXTADDR=0xbfc00000
# PBSSADDR=0x80010000
# LLDFLAGS=-T $(PTEXTADDR) -B $(PBSSADDR) -elsmap

ELFSPEC=lincprom.elspec
LDEFINES=
LLDFLAGS=-elspec $(ELFSPEC)

TARGETS=lincprom

PROMOBJECTS=promvect.o promstart.o prommain.o promnmi.o

CONVERT=$(TOOLS_SUBDIR)/convert
CONVERT2=$(TOOLS_SUBDIR)/conv2readmemh 8

FLUFF=lincprom.hex lincprom.S lincprom.srec

MACHTAGS=-idb "mach(CPUBOARD=IP27 CPUBOARD=IP30)"

install default:	$(TARGETS)

fluff: $(FLUFF)

$(PROMOBJECTS):	Makefile

lincprom:	$(PROMOBJECTS) $(ELFSPEC) $(FIRMDIR)/lib/liblinc.a
	$(LD) $(LDFLAGS) $(PROMOBJECTS) -o lincprom -llinc

lincprom.hex:	lincprom
	echo "@00000000" > lincprom.hex
	$(CONVERT) -f pure lincprom | $(CONVERT2) >> lincprom.hex
	cat fhdr.hex >> lincprom.hex

lincprom.srec:	lincprom
	$(CONVERT) -f s3rec lincprom > lincprom.srec

lincprom.S: lincprom
	$(DIS) -S -I $(FIRMDIR)/lib lincprom > lincprom.S

clobber:
	rm -f $(PROMOBJECTS) $(FLUFF) $(TARGETS)
clean:
	rm -f $(PROMOBJECTS) $(FLUFF)

