#!smake
# $Revision: 1.11 $

DEPTH=.
IMTOP=true
include $(ROOT)/usr/include/make/kcommondefs
KPATH=$(TOP)
.PATH:$(KPATH) $(BOOTAREA)
include $(RELEASEDEFS)
.NOTPARALLEL:

# Release 3.1 is for Origin+Octane only!!
#ifndef SRC_RPD
EVERYPRODUCT=EVEREST BB_EVEREST EVEREST32 T5_EVEREST SN0 RACER
#else
EVERYPRODUCT=EVEREST BB_EVEREST T5_EVEREST SN0 RACER
#endif

#if defined(PRODUCT) && !empty(PRODUCT)
NOPROD=ignore
PROD=
#else
NOPROD=
PROD=prod
#endif

SUBDIRS=mips

HEADERS_SUBDIRS=sys

TARGETS=

#
# Rules
#
# If PRODUCT is set then the $(PROD) rules are used else
# the $(NOPROD) rules are used.
# We set KOPREF to override the rules in kcommonrules
#
KOPREF=kk
include $(ROOT)/usr/include/make/kcommonrules

#
# Rules when PRODUCT is set
#
$(PROD)install!$(BOOTAREA) bootheaders headers installsubdirs $(TARGETS)

$(PROD)default!$(BOOTAREA) $(SUBDIRS) $(TARGETS)

$(PROD)subdirs!.RECURSIVE
$(PROD)subdirs!
	@RULE=default; export $$RULE; $(KSUBDIRS_MAKERULE)

$(PROD)headers!doheadersubdirs

doheadersubdirs!.RECURSIVE
doheadersubdirs!
	@RULE=headers; export $$RULE; \
	for d in $(HEADERS_SUBDIRS); do $(SUBDIR_MAKERULE); done

$(PROD)bootheaders!
	@RAWIDB= ; export RAWIDB; \
	RULE=headers; export $$RULE; \
	for d in $(HEADERS_SUBDIRS); do $(SUBDIR_MAKERULE); done

$(PROD)clobber!doclobbersubdirs dormtargets
	-rm -fr $(BOOTAREA)

doclobbersubdirs!.RECURSIVE
doclobbersubdirs!
	@RULE=clobber; export $$RULE; $(KSUBDIRS_MAKERULE)

$(PROD)clean!docleansubdirs

docleansubdirs!.RECURSIVE
docleansubdirs!
	@RULE=clean; export $$RULE; $(KSUBDIRS_MAKERULE)

$(PROD)rmtargets!dormtargetssubdirs dormtargets

dormtargets!

dormtargetssubdirs!.RECURSIVE
dormtargetssubdirs!
	@RULE=rmtargets; export $$RULE; $(KSUBDIRS_MAKERULE)

#
# Rules when PRODUCT is not set
#
$(NOPROD)install $(NOPROD)default $(NOPROD)version $(NOPROD)subdirs \
$(NOPROD)clean $(NOPROD)rmtargets!
	@for p in $(EVERYPRODUCT) ; do \
		PRODUCT=$$p; export PRODUCT ;\
		echo "PRODUCT=$$p $(MAKE) $(@:S/${NOPROD}/$(PROD)/)" ;\
		$(MAKE) $(@:S/${NOPROD}/$(PROD)/) ;\
	done

$(NOPROD)clobber!wipeallprodbootarea
	@for p in $(EVERYPRODUCT) ; do \
		PRODUCT=$$p; export PRODUCT ;\
		echo "PRODUCT=$$p $(MAKE) $(@:S/${NOPROD}/$(PROD)/)" ;\
		$(MAKE) $(@:S/${NOPROD}/$(PROD)/) ;\
	done

wipeallprodbootarea!
	-rm -fr *bootarea

# Currently headers are not machine specific
$(NOPROD)headers!$(PROD)headers

#
# Allow smake -n to work well
#
.RECURSIVE: $(NOPROD)install $(NOPROD)default $(NOPROD)version \
	$(NOPROD)subdirs $(NOPROD)clobber $(NOPROD)clean $(NOPROD)rmtargets

installsubdirs!.RECURSIVE
installsubdirs!
	@RULE=install; export $$RULE; $(KSUBDIRS_MAKERULE)
