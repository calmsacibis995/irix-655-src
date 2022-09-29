#!smake
#
include ../commondefs

CVERSION=-cckr

SHFILES=runfsdump.sh fslist.build.sh
CFILES=fsdump.c ht_time.c qsumfenv.c rcsfenv.c symlinkfenv.c jdm.c
LIBDISK=-ldisk
LLDLIBS=$(LIBDISK) $(LIBRFIND) -lm -lgen -B static -ldm -B dynamic

TARGETS=fsdump runfsdump fslist.build

default: $(TARGETS)

include ../commonrules

fsdump:	${OBJECTS}
	${CCF} -g ${OBJECTS} ${LDFLAGS} -o $@

install: default
	${INSTALL} -u root -g sys -F /var/rfindd -m 755 fsdump
	${INSTALL} ${USR_GRP} -F /var/spool/cron/crontabs -idb 'config(suggest)' -m 644 -src crontab rfindd
	${INSTALL} -idb "delhist" -dir /usr/spool/cron/crontabs/rfindd
	${INSTALL} -u bin -g bin -F /var/rfindd -idb 'exitop("test x$$diskless = xnone && chroot $$rbase /bin/sh /var/rfindd/fslist.build || true")' fslist.build
	${INSTALL} ${USR_GRP} -F /var/rfindd runfsdump
	${INSTALL} ${USR_GRP} -F /var/rfindd -idb 'preop("test -d $$rbase/usr/lib/rfindd -a ! -d $$rbase/var/rfindd && mv $$rbase/usr/lib/rfindd $$rbase/var/rfindd || true") config(noupdate)' -m 644 fslist fsoptions
