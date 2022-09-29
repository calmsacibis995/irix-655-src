#!smake
# Makefile for the client command rfind.
#
include ../commondefs

CFILES=rfind.c getaliases.c getprotobyname.c pathcanon.c
TARGETS=rfind
LLDLIBS=$(LIBRFIND)

default: ${TARGETS}

include ../commonrules

rfind:	${OBJECTS}
	${CCF} ${OBJECTS} ${LDFLAGS} -o $@

install: default
	${INSTALL} -u bin -g bin -F /usr/sbin -m 755 rfind
	${INSTALL} -u bin -g bin -F /etc -idb "config(noupdate)" -m 644 -src rfind.aliases.ship rfind.aliases
	${INSTALL} -u bin -g bin -F /etc -idb "noship.sw.rfind config(noupdate)" -m 644 rfind.aliases
