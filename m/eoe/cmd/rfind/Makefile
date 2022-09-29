#
# top level Makefile for:
#	rfind client (rfind), server (rfindd) and dumper (fsdump).

include ${ROOT}/usr/include/make/commondefs

SUBDIRS=lib fsdump rfind rfindd config init.d rotatelogs

default install ${COMMONTARGS}: $(_FORCE)
	$(SUBDIRS_MAKERULE)

# An always-unsatisfied target.  The name is unlikely to occur in a file tree,
# but if _force existed in a make's current directory, this target would be
# always-satisfied and targets that depended on it would not be made.
#
$(_FORCE):

