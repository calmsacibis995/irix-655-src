#!smake
#ident "$Id: Makefile,v 1.1 1999/03/05 23:44:05 cwf Exp $"
include ${ROOT}/usr/include/make/commondefs

CFILES= fsr.c
TARGETS= fsr

LCOPTS= -fullwarn

all: ${TARGETS}

include ${COMMONRULES}

fsr:	${OBJECTS}
	${CCF} ${OBJECTS} ${LDFLAGS} -o $@

default: all

install: all
	${INSTALL} -F /usr/etc -idb "std.sw.unix" ${TARGETS}

