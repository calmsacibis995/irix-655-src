#!smake
#
# Makefile for hippi ism
#
# The main targets at the top level are:
#
# default:this performs a bootstrap (headers and exports) then
#	builds all of the software in place. No rawidb is generated and
#	targets are not installed anywhere. No ism image is generated
#
# startversion: this generates a version number.
#	The developer just needs to invoke the target 
#	(see $(ROOT)/usr/include/make/startversion for more info).
#	The generated version number can be embedded as ident string in 
#	executables by setting the envariable RLS_ID_OBJECT to 
#	${WORKAREA}/.identfile. The VERSION envariable is also set for use 
#	with the spec file (at gendist time).
#	This target is supplied by ismcommonrules and it saves/documents the 
#	version number in $(WORKAREA)/.version_number. 
#
# headers:install all headers. This should always be run without RAWIDB set.
#
# exports:builds and installs all libraries and data files that are required
#	to build the rest of the ism. This should always be run without
#	RAWIDB set.
#
# install:builds and installs the entire ism. 'headers' and 'exports'
#	must be run previous to 'install'. With RAWIDB set this will
#	generate the rawidb for the ism. Without RAWIDB set, all the
#	ism objects will be installed in $ROOT
#
# images:descends into the build directory and generates the files needed 
#	for the build of entire system. 
#	'rawidb' must be run previous to 'images'.
#
# ism:descends into the build directory and generates ism images. 
#	'rawidb' must be run previous to 'ism'.
#
# clean:removes .o's
# clobber:removes all non-source. The effective of 'clobber' can be checked
#	by running p_check -w after running 'clobber'
# rmtargets:removes TARGETS only
#
#
# "$Revision: 1.9 $"

include $(ROOT)/usr/include/make/ismcommondefs

## customize for subdirs you have (tools etc)
## i.e. replicate/rename src directory as needed

#ifndef SRC_PRD
SUBDIRS= books build tools firm cmd kern misc man relnotes noship
#else
SUBDIRS= tools cmd kern misc
#endif

## other subdirectory list that may apply
#HEADERS_SUBDIRS=
#EXPORTS_SUBDIRS=

COMMONPREF=hippi
SRC=`/sbin/pwd`
IDBFILE="`/sbin/pwd`/build/IDB"
ISM_NAME=hippi
## override to default ALPHA envariable setting, if desired
#ALPHA=000001

default:headers exports $(_FORCE)
	$(SUBDIRS_MAKERULE)

headers:$(_FORCE)
##	uncomment next line if there are headers to be generated
#	$(HEADERS_SUBDIRS_MAKERULE)

exports:$(_FORCE)
##	uncomment next line if there are libraries to be generated
#	$(EXPORTS_SUBDIRS_MAKERULE)

install:$(_FORCE) 
	$(SUBDIRS_MAKERULE)

$(COMMONTARGS):$(_FORCE) $(COMMONPREF)$$@
	$(SUBDIRS_MAKERULE)

include $(ISMCOMMONRULES)

$(SUBDIRS):$(_FORCE)
	cd $@; $(MAKE)

rawidb: $(_FORCE)
	@RAWIDB=$(IDBFILE); export RAWIDB ; \
	if [ -f $$RAWIDB ] ; then mv $$RAWIDB $$RAWIDB.prev ; fi ;\
	echo "RAWIDB=$$RAWIDB SRC=$(SRC) $(MAKE) install" ;\
	$(MAKE) SRC=$(SRC) install

ism: $(_FORCE)
	cd build; $(MAKE) buildism

images: $(_FORCE)
	cd build; $(MAKE) buildimages
