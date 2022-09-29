#!smake
#
# Make miser 
#
# $Revision: 1.17 $ 
#
include $(ROOT)/usr/include/make/commondefs

DEBUG	=
#DEBUG	= -g -DDEBUG

SUBDIRS	= libmiser


IDB_TAG		= -u root -g sys -m 755
DFLT_POLICY	= default_policy.o
REPACK_POLICY	= repack_policy.o
CXXFILES	= $(DFLT_POLICY) $(REPACK_POLICY) job.o main.o queue.o miser_debug.o
CFILES		= policy_defs.o
CMDS		= miser_qinfo miser_reset miser_jinfo miser_move miser_submit miser_cpuset miser_kill
MISER		= miser
TARGETS		= $(MISER) $(CMDS) 

LDFLAGS		+= -OPT:Olimit=2960 -Llibmiser -lmiser 
LCXXFLAGS	= $(DEBUG) -OPT:Olimit=2960 -Ilibmiser
LCFLAGS		= $(DEBUG) -Ilibmiser
LDIRT		= ii_files


default:	$(TARGETS)
	$(SUBDIRS_MAKERULE)

COMMONPREF=local_
include $(COMMONRULES)

$(TARGETS):	$(SUBDIRS)

$(SUBDIRS):	$(_FORCE)
	$(SUBDIRS_MAKERULE)

$(MISER):	$(OBJECTS)
	$(TOOLROOT)/usr/bin/CC $(OBJECTS) $(LDFLAGS) -n32 -o $@

$(COMMONPREF)install: default
	$(INSTALL) -F /usr/etc $(IDB_TAG) -o miser
	$(INSTALL) -F /usr/sbin $(IDB_TAG) -o miser_qinfo
	$(INSTALL) -F /usr/sbin $(IDB_TAG) -o miser_jinfo	
	$(INSTALL) -F /usr/sbin $(IDB_TAG) -o miser_reset
	$(INSTALL) -F /usr/sbin $(IDB_TAG) -o miser_move	
	$(INSTALL) -F /usr/sbin $(IDB_TAG) -o miser_submit
	$(INSTALL) -F /usr/sbin $(IDB_TAG) -o miser_kill
	$(INSTALL) -F /usr/sbin $(IDB_TAG) -o miser_cpuset
	$(INSTALL) -F /etc/init.d $(IDB_TAG) -src miserrc.sh miser
	$(INSTALL) -F /etc/config $(IDB_TAG) -src miser.config miser
	$(INSTALL) -F /etc/config $(IDB_TAG) miser.options 
	$(INSTALL) -F /etc	$(IDB_TAG) -idb "config(suggest)" miser_system.conf
	$(INSTALL) -F /etc	$(IDB_TAG) -idb "config(suggest)" miser_default.conf
	$(INSTALL) -F /etc	$(IDB_TAG) -idb "config(suggest)" miser.conf
	$(INSTALL) -F /etc/rc2.d -lns ../init.d/miser S98miser

$(COMMONTARGS) install: $(COMMONPREF)$$@
	$(SUBDIRS_MAKERULE)
