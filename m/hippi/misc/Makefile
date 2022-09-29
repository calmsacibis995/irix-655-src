#
# Makefile for hippi/misc, to install master.d, system files under /var/sysgen.
#
# $Revision: 1.14 $  $Date: 1998/05/06 21:55:09 $
#

include $(ROOT)/usr/include/make/commondefs

HIOMACHTAGS=-idb "mach(CPUBOARD=IP19 CPUBOARD=IP21 CPUBOARD=IP25)"
XIOMACHTAGS=-idb "mach(CPUBOARD=IP27 CPUBOARD=IP30)"
MACHTAGS=-idb "mach(CPUBOARD=IP19 CPUBOARD=IP21 CPUBOARD=IP25 CPUBOARD=IP27 CPUBOARD=IP30)"

DEVFLAGS=-F /dev -m 600 -u root -g sys
BPDEVFLAGS=-F /dev -m 666 -u root -g sys

default:

include $(COMMONRULES)

headers: install

exports:

$(COMMONPREF)install: $(_FORCE)
	$(INSTALL) -m 444 -F /var/sysgen/master.d $(HIOMACHTAGS) hippi
	$(INSTALL) -m 444 -F /var/sysgen/master.d $(HIOMACHTAGS) \
		   -src if_hip.hio if_hip
	$(INSTALL) -m 444 -F /var/sysgen/system   $(HIOMACHTAGS) hippi.sm
	$(INSTALL) -m 444 -F /var/sysgen/master.d $(XIOMACHTAGS) hps hippibp
	$(INSTALL) -m 444 -F /var/sysgen/master.d $(XIOMACHTAGS) \
		   -src if_hip.xio if_hip
	$(INSTALL) -m 444 -F /var/sysgen/system   $(XIOMACHTAGS) hippi_s.sm
	$(INSTALL) -m 444 -F /var/sysgen/ioconfig $(XIOMACHTAGS) \
		   -src hippi.xioconfig hippi
	$(INSTALL) -m 555 -F /etc/init.d $(XIOMACHTAGS) -src init.d.hippi.xio hippi
	$(INSTALL) -m 555 -F /etc/init.d $(HIOMACHTAGS) -src init.d.hippi.hio hippi
	$(INSTALL) -F /etc/rc2.d $(MACHTAGS) -lns /etc/init.d/hippi  S26hippi
	$(INSTALL) -m 555 -F /etc/init.d $(MACHTAGS) network.hippi
	$(INSTALL) -idb "config(noupdate)" -m 444 -F /usr/etc $(MACHTAGS) hippi.imap
	if [ "$(RAWIDB)" != "" ] ; then \
	  $(INSTALL) $(DEVFLAGS) $(HIOMACHTAGS) -chr 251,0x0000 hippi0 ; \
	  $(INSTALL) ${DEVFLAGS} $(HIOMACHTAGS) -chr 251,0x1000 hippi1 ; \
	  $(INSTALL) ${DEVFLAGS} $(HIOMACHTAGS) -chr 251,0x2000 hippi2 ; \
	  $(INSTALL) ${DEVFLAGS} $(HIOMACHTAGS) -chr 251,0x3000 hippi3 ; \
	  $(INSTALL) ${DEVFLAGS} $(HIOMACHTAGS) -chr 251,0x4000 hippi4 ; \
	  $(INSTALL) ${DEVFLAGS} $(HIOMACHTAGS) -chr 251,0x5000 hippi5 ; \
	  $(INSTALL) ${DEVFLAGS} $(HIOMACHTAGS) -chr 251,0x6000 hippi6 ; \
	  $(INSTALL) ${DEVFLAGS} $(HIOMACHTAGS) -chr 251,0x7000 hippi7 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x0800 hippibp000 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x0801 hippibp001 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x0802 hippibp002 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x0803 hippibp003 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x0804 hippibp004 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x0805 hippibp005 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x0806 hippibp006 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x0807 hippibp007 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x1800 hippibp100 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x1801 hippibp101 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x1802 hippibp102 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x1803 hippibp103 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x1804 hippibp104 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x1805 hippibp105 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x1806 hippibp106 ; \
	  $(INSTALL) ${BPDEVFLAGS} $(HIOMACHTAGS) -chr 251,0x1807 hippibp107 ; \
	fi
	if [ "$(RAWIDB)" = "" -a "$(ROOT)" != "" ] ; then \
	   $(INSTALL) -F /usr/sysgen/master.d hippi ; \
	   $(INSTALL) -F /usr/sysgen/master.d -src if_hip.sloppy if_hip ; \
	   $(INSTALL) -F /usr/sysgen/master.d hps ; \
	   $(INSTALL) -F /usr/sysgen/master.d hippibp ; \
	fi
