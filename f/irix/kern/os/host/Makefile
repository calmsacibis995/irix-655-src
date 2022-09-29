#!smake
#
# Makefile for host which contains global state management. 
#
#ident	"$Id: Makefile,v 1.4 1999/05/14 20:13:13 lord Exp $"

DEPTH?=../..
include ${DEPTH}/kcommondefs
KPATH=$(TOP)/os/host
.PATH:$(KPATH)

KCINCS=-I${BOOTAREA}

HOST_NAME=os.a
HOST_CSRCS=vhost.c phost.c

SUBDIRS=
HOST_CSRCS+=vhost_lp.c

HOST_OBJS=${HOST_CSRCS:.c=.o}
$(HOST_NAME):$(HOST_NAME)($(HOST_OBJS)) MAKELIB

TARGETS=$(HOST_NAME)
.PRECIOUS:$(TARGETS)

# lint
LINTNAME=host
CFILES=$(HOST_CSRCS)

#
# Rules
#
include ${DEPTH}/kcommonrules

# This default rule is always invoked in BOOTAREA
$(KCOMMONPREF)default:$(TARGETS)

$(KCOMMONPREF)install: $(KCOMMONPREF)default
