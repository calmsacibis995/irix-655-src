#!smake

#
# eoe/cmd/miser/libmiser/Makefile
#	Make libmiser.a
#

include $(ROOT)/usr/include/make/commondefs


# bison and flex are now defined in commondefs
BISON_FLAGS	= -d
FLEX_FLAGS	=
FLEX_CFLAGS	= -woff 1110


LIBMISER	= libmiser.a
LIBOBJS		= cmd.o miser.o parse.o parse_gram.o parse_scan.o print.o util.o
TESTS		= tparse

TARGETS		= $(LIBMISER) $(TESTS)

default all:	$(TARGETS)
install:	default
libmiser:	default

include $(COMMONRULES)
LCFLAGS		= 
LDLIBS		= $(LIBMISER)
LDIRT		= parse_gram.c parse_gram.h parse_scan.c
LWOFF		= ,1110


$(LIBMISER):	$(LIBOBJS)
	$(AR) c$(ARFLAGS) $(LIBMISER) $(LIBOBJS)

parse_gram.c:	parse_gram.y
	$(BISON) $(BISON_FLAGS) $*.y
	mv $*.tab.c $*.c
	mv $*.tab.h $*.h

parse_scan.c:	parse_scan.l
	$(FLEX) $(FLEX_FLAGS) $*.l
	mv lex.yy.c $*.c

parse_gram.h:	parse_gram.c

parse_gram.o:	parse_gram.c

parse_scan.o:	parse_scan.c parse_gram.h


tparse:		tparse.c $(LIBMISER)
