#!smake
#ident "$Revision: 1.2 $"

BASEVERSION=base
ALTVERSIONS=troot
COMMANDS=wchrtbl

include $(ROOT)/usr/include/make/cmdcommondefs

default:$(TARGETS)

include $(CMDCOMMONRULES)

baseinstall: default
	${INSTALL} -F /usr/bin $(COMMANDS)

trootinstall: default
	${INSTALL} -F /usr/bin $(COMMANDS)
