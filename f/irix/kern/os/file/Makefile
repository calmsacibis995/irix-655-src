#!smake
# Copyright 1995, Silicon Graphics Inc., Mountain View, CA. 
#
#ident  "$Id: Makefile,v 1.4 1999/05/14 20:13:13 lord Exp $"

DEPTH?=../..
include ${DEPTH}/kcommondefs
KPATH=$(TOP)/os/file
.PATH:$(KPATH)

FILE_NAME=os.a
FILE_CSRCS= vfile.c pfile.c

SUBDIRS=
FILE_CSRCS+=vfile_lp.c

FILE_OBJS=${FILE_CSRCS:.c=.o}
$(FILE_NAME):$(FILE_NAME)($(FILE_OBJS)) MAKELIB

TARGETS=$(FILE_NAME)
.PRECIOUS:$(TARGETS)

LINTNAME=os
CFILES=$(FILE_CSRCS)

#
# Rules
#
include ${DEPTH}/kcommonrules

$(KCOMMONPREF)default:$(TARGETS)

$(KCOMMONPREF)install: $(KCOMMONPREF)default
