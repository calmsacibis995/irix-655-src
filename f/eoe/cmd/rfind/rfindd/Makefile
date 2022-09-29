#!smake
# Makefile for the daemon rfindd.
#
include ../commondefs

CVERSION=-cckr

SHFILES=passwd.add.sh
CFILES=rpc.rfindd.c rfindd.c rpc_io.c
TARGETS=rfindd passwd.add
LLDLIBS=$(LIBRFIND) -lgen

default: ${TARGETS}

include ../commonrules

rfindd:	${OBJECTS}
	${CCF} ${OBJECTS} ${LDFLAGS} -o $@

install: ${TARGETS}
	${INSTALL} ${USR_GRP} -F /var/rfindd -m 755 rfindd
	${INSTALL} -u bin -g bin -F /var/rfindd -idb 'exitop("test x$$diskless = xnone && chroot $$rbase /bin/sh /var/rfindd/passwd.add || true")' passwd.add
	${INSTALL} ${USR_GRP} -F /var/rfindd -idb 'config(noupdate)' -m 644 -src forward .forward
	${INSTALL} ${USR_GRP} -F /var/rfindd -idb 'config(update)' -m 644 README

rf: rfindd.c		# debug standalone variant of rfindd
	${CCF} -o rf.o -I../include -I -I/usr/include -g -c -Dstandalone rfindd.c
	${CCF} -o rf rf.o ../lib/librfind.a -lgen
