#!smake
# Generated automatically from Makefile.in by configure.
# Main Makefile for GNU m4.
# Copyright (C) 1992, 1993, 1994 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

PRODUCT = m4
VERSION = 1.4

SHELL = /bin/sh
srcdir = .


# This directory's subdirectories are mostly independent; you can cd
# into them and run `make' without going through this Makefile.
# To change the values of `make' variables: instead of editing Makefiles,
# (1) if the variable is set in `config.status', edit `config.status'
#     (which will cause the Makefiles to be regenerated when you run `make');
# (2) otherwise, pass the desired values on the `make' command line.

CC = cc
CFLAGS = -O2
INSTALL = ./install-sh -c
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_PROGRAM = ${INSTALL}
LDFLAGS = 
LIBS = 


prefix = /usr/local
exec_prefix = ${prefix}
bindir = $(exec_prefix)/bin
infodir = $(prefix)/info

MDEFINES = CC='$(CC)' CFLAGS='$(CFLAGS)' LDFLAGS='$(LDFLAGS)' LIBS='$(LIBS)' \
prefix='$(prefix)' exec_prefix='$(exec_prefix)' \
bindir='$(bindir)' infodir='$(infodir)'

SUBDIRS = lib src

DISTFILES = README NEWS TODO THANKS COPYING INSTALL ChangeLog c-boxes.el \
configure.in acconfig.h aclocal.m4 mkinstalldirs install-sh Makefile.in \
stamp-h.in config.h.in configure BACKLOG

.SUFFIXES:

default : all

all : config.h
	for subdir in $(SUBDIRS); do \
	  echo making $@ in $$subdir; \
	  (cd $$subdir && $(MAKE) $(MDEFINES) $@) || exit 1; \
	done

info dvi:
	cd doc && $(MAKE) $@

check: all
	cd checks && $(MAKE) $@

mostlyclean: mostlyclean-recursive mostlyclean-local

clean: clean-recursive clean-local

distclean: distclean-recursive distclean-local
	rm config.status

realclean: realclean-recursive realclean-local
	rm config.status

mostlyclean-recursive clean-recursive distclean-recursive realclean-recursive:
	for subdir in $(SUBDIRS); do \
 	  target=`echo $@ | sed 's/-recursive//'`; \
	  echo making $$target in $$subdir; \
	  (cd $$subdir && $(MAKE) $$target) || exit 1; \
	done

mostlyclean-local:

clean-local: mostlyclean-local

distclean-local: clean-local
	rm -f Makefile config.cache config.h config.log stamp-h

realclean-local: distclean-local

dist: $(DISTFILES)
	rm -rf $(PRODUCT)-$(VERSION)
	mkdir $(PRODUCT)-$(VERSION)
	chmod 777 $(PRODUCT)-$(VERSION)
	@echo "Copying distribution files"
	@for file in $(DISTFILES); do \
	  ln $(srcdir)/$$file $(PRODUCT)-$(VERSION) 2> /dev/null \
	    || cp -p $(srcdir)/$$file $(PRODUCT)-$(VERSION); \
	done
	for subdir in $(SUBDIRS); do \
	  echo making $@ in $$subdir; \
	  mkdir $(PRODUCT)-$(VERSION)/$$subdir; \
	  chmod 777 $(PRODUCT)-$(VERSION)/$$subdir; \
	  (cd $$subdir && $(MAKE) $@) || exit 1; \
	done
	chmod -R a+r $(PRODUCT)-$(VERSION)
	tar chozf $(PRODUCT)-$(VERSION).tar.gz $(PRODUCT)-$(VERSION)
	rm -rf $(PRODUCT)-$(VERSION)

# For an explanation of the following Makefile rules, see node
# `Automatic Remaking' in GNU Autoconf documentation.
Makefile: Makefile.in config.status
	CONFIG_FILES=$@ CONFIG_HEADERS= ./config.status
config.status: configure
	./config.status --recheck
configure: configure.in aclocal.m4
	cd $(srcdir) 

config.h: stamp-h
stamp-h: config.h.in config.status
	CONFIG_FILES= CONFIG_HEADERS=config.h ./config.status
config.h.in: stamp-h.in
stamp-h.in: configure.in aclocal.m4 acconfig.h
	cd $(srcdir) && autoheader
	date > $(srcdir)/stamp-h.in

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:

include $(ROOT)/usr/include/make/commondefs
include $(COMMONRULES)

install exports: all
	$(INSTALL) -F /usr/bin -idb TOOLS -src src/m4 gnum4
