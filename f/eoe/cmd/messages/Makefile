#!smake
include $(ROOT)/usr/include/make/commondefs

COMMONPREF=msgs

TARGETS=mkinclmsg

# This will compile mkinclmsg in order to produce
# old style macros (SGI_...)
LCDEFS=-DWITH_OLD_FORMAT

HEADERS_SUBDIRS= \
	uxawk uxcore uxcore.abi uxdfm uxed.abi \
	uxeoe uxepu uxlibc uxlibxnet uxmesg uxnsu \
	uxsgicore uxsgicsh uxsgidemo \
	uxsgidesktop uxsgiimpr uxsgish \
	uxsysadm uxue.abi uxue

SUBDIRS= $(HEADERS_SUBDIRS)

default install $(COMMONTARGS) : $(COMMONPREF)$$@ $(_FORCE)
	$(SUBDIRS_MAKERULE)

headers: $(COMMONPREF)default
	$(HEADERS_SUBDIRS_MAKERULE)

include $(COMMONRULES)

$(COMMONPREF)default: $(TARGETS)

$(COMMONPREF)install: $(COMMONPREF)default
	$(INSTALL) -F /usr/bin $(TARGETS)

mkinclmsg: mkinclmsg.c
	$(HOST_CC) $(LCDEFS) -O -o mkinclmsg mkinclmsg.c

$(COMMONPREF)install: mkinclmsg
