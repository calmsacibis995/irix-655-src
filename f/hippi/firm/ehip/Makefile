
#
# Makefile for hippi/firm directory
#

include $(ROOT)/usr/include/make/commondefs

include ../../hippi.defs

ASFILES	=ehip.s
OBJECTS =$(ASFILES:.s=.o)
FIRMS=$(ASFILES:.s=.firm)
VERSS=$(ASFILES:.s=.vers)
TARGETS	=$(FIRMS) $(VERSS)
LISTINGS=$(ASFILES:.s=.lst)
DBGSTUFF=ehip_debug.o ehip_debug.lst

#give assembler a series of decimal, not octal, numbers for a version
VERS	=`date '+-DVERS_Y=%y -DVERS_M=%m -DVERS_D=%d -DVERS_H=%H \
	    -DVERS_MM=%M' | sed -e 's/=0/=/g'`

# These flags are not understood by cc and cause problems for makedepend
# normal listing
AS29FLAGS=$(VERS) -s -l -fP4 -fo -fs -fg
# list everything including #include files
#AS29FLAGS=$(VERS) -s -l -fP4 -fo -fs -fg -fm -fi

TDIR	=$(TOOLROOT)/usr/a29k/usr/bin
AS	=$(TDIR)/as29i
C2FIRM	=$(TDIR)/coff2firm
GREP	=$(TOOLROOT)/usr/bin/grep
LDIRT	=*.lst

FIRM_TEXT=0x06000000

default:$(TARGETS) ehip_debug.lst

clobber:
	rm -f $(TARGETS) $(OBJECTS) $(LISTINGS) $(DBGSTUFF)
clean:
	rm -f $(OBJECTS) $(LISTINGS) $(DBGSTUFF)

list:	$(LISTINGS)

install: default $(OBJECTS) $(LISTINGS) $(DBGSTUFF)
	$(INSTALL) -idb noship -m 444 $(HIOMACHTAGS) \
		-F /usr/local/lib/hippi-noship \
		$(TARGETS) $(OBJECTS) $(LISTINGS) $(DBGSTUFF)


# header file dependencies, because makedepend does not recognize the
# non-cpp include files used by as29i

ehip.lst ehip.o:std.h ehip.h ehiphw.h
ehip_debug.lst ehip_debug.o:std.h ehip.h ehiphw.h

#
# Generate a debug version of the firmware that halts on error.
#
ehip_debug.o ehip_debug.lst: $(ASFILES)
	$(AS) $(AS29FLAGS) -DHALT_ON_ERR=1 -o ehip_debug.o ehip.s \
		> ehip_debug.lst

.SUFFIXES:.lst .firm .vers .hex

.lst.o:
	@#done by the .lst rule

.s.lst:
	$(AS) $(AS29FLAGS) $< > $*.lst

.s.o:
	$(AS) $(AS29FLAGS) $< > $*.lst

.o.firm:
	$(C2FIRM) -dc -n $* -b $(FIRM_TEXT) -i $< -o $*.firm

.firm.vers:
	$(GREP) $*_vers $*.firm > $*.vers

.o.hex:
	$(C2FIRM) -h -i $< -o $*.hex
