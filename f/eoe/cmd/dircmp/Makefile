#!smake
#ident "$Revision: 1.7 $"

include $(ROOT)/usr/include/make/commondefs

TARGETS=dircmp dircmp.sh

default:$(TARGETS)

dircmp.sh:
	chmod +x dircmp_msg.sh
	dircmp_msg.sh

include $(COMMONRULES)

install: default
	${INSTALL} -F /usr/bin dircmp 
