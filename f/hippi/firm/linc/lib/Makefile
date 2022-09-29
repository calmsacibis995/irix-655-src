#!smake
#
# Makefile for linc libraries
#
# $Revision: 1.9 $
#
#

DEPTH=../../..
include ../firmcommondefs

TARGETS=liblinc.a

#if $(BREAKPOINTS) == "1"
RDBGOBJ= rdbg_gdb.o
#else
RDBGOBJ= rdbg_cmdl.o
#endif

# RDBGOBJ= rdbg_kdbx.o

OBJECTS=serial.o util.o cpuops.o \
	rdbg.o rdbg_brkpt.o $(RDBGOBJ)


HEADERS=$(HEADERS_SUBDIR)/*.h $(KERN_HEADERS_SUBDIR)/hip_errors.h

AR=/usr/bin/ar

install default: $(TARGETS)

liblinc.a: $(OBJECTS) $(HEADERS) Makefile
	rm -f liblinc.a
	$(AR) -qc liblinc.a $(OBJECTS)

$(OBJECTS): $(HEADERS) Makefile

clobber:
	rm -f $(OBJECTS) *.o $(TARGETS)
clean:
	rm -f $(OBJECTS) *.o
