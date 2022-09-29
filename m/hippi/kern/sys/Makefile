#
# Makefile for hippi/kern/sys, to install header files under /usr/include.
#

include $(ROOT)/usr/include/make/commondefs

default:

include $(COMMONRULES)

headers: install

exports:

$(COMMONPREF)install: $(_FORCE)
	$(INSTALL) -m 444 -F /usr/include/sys hippi.h
	$(INSTALL) -idb "noship" -m 444 -F /usr/include/sys hippidev.h hippibp.h if_hip.h if_hip_ext.h hippi_firm.h hippibp_firm.h hps_ext.h
