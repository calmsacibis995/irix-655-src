#!smake
#
# Makefile for HIPPI-Serial card's Roadrunner Firmware
#
# $Revision: 1.7 $  $Date: 1996/12/13 05:45:42 $
#

include $(ROOT)/usr/include/make/commondefs

include ../../hippi.defs


SUBDIRS=diags

COMMONPREF=rr

TARGETS=rr_src_fw.h rr_dst_fw.h rr_src.fw rr_dst.fw
LDIRT=*.o *.list

LISTINGS=rr_src.list rr_dst.list

GASRR=../../tools/gas/gasrr
GASOD=../../tools/gas/objdumprr
NAWK=nawk

default: $(TARGETS)

$(COMMONTARGS): $(COMMONPREF)$$@
	@for d in $(SUBDIRS); do \
		$(ECHO) "\tcd $$d; $(MAKE) $@"; \
		cd $$d; $(MAKE) $@; cd ..; \
	done

install: $(TARGETS)
	$(INSTALL) -idb noship -m 444 $(XIOMACHTAGS) \
		-F /usr/local/lib/hippi-noship/firm $(LISTINGS)
	@for d in $(SUBDIRS); do \
		$(ECHO) "\tcd $$d; $(MAKE) $@"; \
		cd $$d; $(MAKE) $@; cd ..; \
	done

############ Destination side Roadrunner ####################
rr_dst.o: rr_dst.s rr.h
	$(GASRR) -als -o rr_dst.o rr_dst.s > rr_dst.list

# Generate the header file to be included by 4640 firmware
rr_dst_fw.h: rr_dst.o
	$(GASOD) -s rr_dst.o | \
	$(NAWK) -f header.awk which=rr_dst - >rr_dst_fw.h

# Generate a stream of raw words to be downloaded by rrdbg
rr_dst.fw: rr_dst.o
	$(GASOD) -s rr_dst.o | \
	$(NAWK) -f raw.awk - >rr_dst.fw

############ Source side Roadrunner ####################
rr_src.o: rr_src.s rr.h
	$(GASRR) -als -o rr_src.o rr_src.s >rr_src.list

# Generate the header file to be included by 4640 firmware
rr_src_fw.h: rr_src.o
	$(GASOD) -s rr_src.o | \
	$(NAWK) -f header.awk which=rr_src - >rr_src_fw.h

# Generate a stream of raw words to be downloaded by rrdbg
rr_src.fw: rr_src.o
	$(GASOD) -s rr_src.o | \
	$(NAWK) -f raw.awk - >rr_src.fw

include $(COMMONRULES)
