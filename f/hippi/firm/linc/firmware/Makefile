#!smake
#
# Makefile for hippi firmware
#
# $Revision: 1.12 $
#
#

DEPTH=../../..

include $(ROOT)/usr/include/make/commondefs

include ../../../hippi.defs

include ../firmcommondefs

ELFSPEC=hippilincfw.elspec

FTEXTADDR=0x80000000
FBSSADDR=0x80010000

LDEFINES= -DHIPPI_BP
LLDFLAGS=-elspec $(ELFSPEC) -e _start

# LLDFLAGS=-T $(FTEXTADDR) -B $(FBSSADDR) -elsmap

TARGETS=src_hippilincfw dst_hippilincfw
#TARGETS=src_hippilincfw 

FLUFF=src_hippilincfw.hex src_hippilincfw.S src_hippilincfw.srec \
	dst_hippilincfw.hex dst_hippilincfw.S dst_hippilincfw.srec \

SFIRMOBJECTS=vect.o locore.o smain.o intr.o \
	dma.o sfw.o sbypass.o errors.o scommon.o squeues.o

DFIRMOBJECTS=vect.o locore.o dmain.o intr.o \
	dma.o dfw.o dbypass.o errors.o dcommon.o dqueues.o

LISTINGS=src_hippilincfw.S dst_hippilincfw.S

#
# XXX: probably a better way than this...
#

HEADERS=$(HEADERS_SUBDIR)/*.h \
	$(KERN_HEADERS_SUBDIR)/hippibp_firm.h \
	$(KERN_HEADERS_SUBDIR)/hippibp.h \
	$(KERN_HEADERS_SUBDIR)/hippi_firm.h \
	$(KERN_HEADERS_SUBDIR)/hippi.h \
	$(RR_HEADERS)/*.h

CONVERT=$(TOOLS_SUBDIR)/convert
CONVERT2=$(TOOLS_SUBDIR)/conv2memload

#default:	$(TARGETS)
default:	$(TARGETS) $(FLUFF)

fluff:		$(FLUFF)

$(SFIRMOBJECTS) : $(HEADERS) Makefile
$(DFIRMOBJECTS) : $(HEADERS) Makefile


smain.o: $(HEADERS) main.c
	$(CCF) -c $(CFLAGS) -DHIPPI_SRC_FW -o smain.o main.c

dmain.o: $(HEADERS) main.c
	$(CCF) -c $(CFLAGS) -DHIPPI_DST_FW -o dmain.o main.c

scommon.o: $(HEADERS) common.c
	$(CCF) -c $(CFLAGS) -DHIPPI_SRC_FW -o scommon.o common.c

dcommon.o: $(HEADERS) common.c
	$(CCF) -c $(CFLAGS) -DHIPPI_DST_FW -o dcommon.o common.c

install: $(TARGETS) $(FLUFF) $(_FORCE)
	$(INSTALL) -idb noship -m 444 $(XIOMACHTAGS) \
	-F /usr/local/lib/hippi-noship/firm $(LISTINGS)

$(_FORCE):

src_hippilincfw.hex:	src_hippilincfw
	 $(CONVERT) -f pure src_hippilincfw | \
	 $(CONVERT2) $(FTEXTADDR) > src_hippilincfw.hex

src_hippilincfw:	$(SFIRMOBJECTS) $(ELFSPEC)  \
	$(LIB_SUBDIR)/liblinc.a
	$(LD) $(LDFLAGS) $(SFIRMOBJECTS) -o src_hippilincfw -llinc

src_hippilincfw.S: src_hippilincfw
	$(DIS) -I../lib -S src_hippilincfw > src_hippilincfw.S
	$(NM) -nx src_hippilincfw >> src_hippilincfw.S
	$(ELFDUMP) -h src_hippilincfw >> src_hippilincfw.S

src_hippilincfw.srec: src_hippilincfw
	$(CONVERT) -f s3rec src_hippilincfw > src_hippilincfw.srec

dst_hippilincfw.hex:	dst_hippilincfw
	 $(CONVERT) -f pure dst_hippilincfw | \
	 $(CONVERT2) $(FTEXTADDR) > dst_hippilincfw.hex

dst_hippilincfw:	$(DFIRMOBJECTS) $(ELFSPEC)  \
	$(LIB_SUBDIR)/liblinc.a
	$(LD) $(LDFLAGS) $(DFIRMOBJECTS) -o dst_hippilincfw -llinc

dst_hippilincfw.S: dst_hippilincfw
	$(DIS) -I../lib -S dst_hippilincfw > dst_hippilincfw.S
	$(NM) -nx dst_hippilincfw >> dst_hippilincfw.S
	$(ELFDUMP) -h dst_hippilincfw >> dst_hippilincfw.S

dst_hippilincfw.srec: dst_hippilincfw
	$(CONVERT) -f s3rec dst_hippilincfw > dst_hippilincfw.srec

clobber:
	rm -f $(SFIRMOBJECTS) $(DFIRMOBJECTS) $(FLUFF) $(TARGETS) 
clean:
	rm -f $(SFIRMOBJECTS) $(DFIRMOBJECTS) $(FLUFF) $(FIRMCOMMON)



