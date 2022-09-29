#!smake
#
# Makefile for hippi/kern/mips
#
# $Revision: 1.17 $

DEPTH?=..
include $(ROOT)/usr/include/make/kcommondefs
KPATH=$(TOP)/mips
.PATH:$(KPATH)


#
# Different handling depending on whether driver isfor 
# HIO (Challenge) or XIO (Origin) hardware:
#

#if $(CPUBOARD) == "IP27" || $(CPUBOARD) == "IP30"

KCINCS=-I.. -I../../firm/linc/include -I$(ROOT)/usr/include
KCOPTS=-fullwarn -woff 1412

# PEER_TO_PEER_DMA_WAR -
#	This causes dma's to and from host mem for src and dst firmware
#	deadman timers and hardware status flags to work around a LINC bug
#	with peer to peer dma.

KCDEFS=-DXIO_HIPPI -DHIPPI_BP -DLINC_BRINGUP -DLINC1_MBOX_WAR \
	-DPEER_TO_PEER_DMA_WAR -DHPS_CALLBACK -DUSE_MAILBOX

# debug defines
#KCDEFS=-DXIO_HIPPI -DHIPPI_BP -DLINC_BRINGUP -DLINC1_MBOX_WAR \
#	-DPEER_TO_PEER_DMA_WAR -DHPS_CALLBACK -DUSE_MAILBOX \
#	-DDEBUG -DHPS_DEBUG -DHIPPI_BP_DEBUG

TARGETS= hps.o hippibp.o if_hip.o
#else
KCINCS=-I../../firm/ehip
KCOPTS=-fullwarn 
KCDEFS=-DHIO_HIPPI
TARGETS= hippi.o if_hip.o
#endif

#
# Rules
#
include $(ROOT)/usr/include/make/kcommonrules

$(KCOMMONPREF)default: $(TARGETS)
	${KLINKINSTALL} ${TARGETS}

$(KCOMMONPREF)install: $(KCOMMONPREF)default 
	${KINSTALL} $(LBOOTPOSTOP) ${TARGETS} 
