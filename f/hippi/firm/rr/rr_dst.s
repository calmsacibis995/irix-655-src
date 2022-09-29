; =======================================================================
; 
; Copyright 1996 Silicon Graphics, Inc.
; All Rights Reserved.
;
; This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
; the contents of this file may not be disclosed to third parties, copied or
; duplicated in any form, in whole or in part, without the prior written
; permission of Silicon Graphics, Inc.
;
; RESTRICTED RIGHTS LEGEND:
; Use, duplication or disclosure by the Government is subject to restrictions
; as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
; and Computer Software clause at DFARS 252.227-7013, and/or in similar or
; successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
; rights reserved under the Copyright Laws of the United States.
;
; ========================================================================
;
; DESTINATION-side Roadrunner firmware for the Lego HIPPI-Serial card.
;
; $Revision: 1.22 $	$Date: 1997/07/11 03:31:07 $
;
.ifdef HISTORY
*
* $Log: rr_dst.s,v $
* Revision 1.22  1997/07/11 03:31:07  ddh
* Special case - dma ends on a 64K boundary, so hold the rr2l till we know
* if EOP should be set.
*
* Revision 1.21  1997/02/10 06:13:40  jimp
* support for mbox change of dest rr accept/reject
*
# Revision 1.20  1997/01/22  07:03:59  jimp
# fixed bug for exactly 64KB DMA's
#
# Revision 1.19  1996/12/18  01:46:03  irene
# Fix for problem with losing DMA Asst completion notifications.
# Check for producer catching up with Reference rather than with Consumer.
#
# Revision 1.18  1996/12/17  05:04:11  irene
# Run with parity-checking enabled, toggling it off/on around hw rcv desc
# accesses.
#
# Revision 1.17  1996/12/12  23:45:07  irene
# Fixed bug in .ifdef TEST_PLD code.
#
# Revision 1.16  1996/09/25  20:11:03  irene
# Added error-stat counter to keep track of write DMA attns.
#
# Revision 1.12  1996/09/23  23:57:15  irene
# Backed out enable of CPU parity error
#
# Revision 1.11  1996/09/21  06:06:16  irene
# Added code (not turned on - ".ifdef TEST_PLD") to test Bypass register
# by sending an interrupt everytime a descriptor is sent up to dst SDRAM.
#
# Revision 1.9  1996/09/21  02:03:27  irene
# Fixed bug in send_des() stack pointer manipulation.
#
# Revision 1.7  1996/09/07  00:20:18  irene
# support for mbox31 you-alive? queries.
#
# Revision 1.5  1996/08/02  20:27:39  irene
# Added alternate simulation addresses for LINC environment.
#
# Revision 1.4  1996/07/02  02:35:45  irene
# Moved Glink/INIT status mbox from mbox2 to mbox31.
# Changed all addiu's to addi's because addiu is going away in Rev B
# roadrunner chip.
#
# Revision 1.3  1996/06/27  00:47:29  irene
# Moved status-reporting from mbox0 to mbox2.
#
# Revision 1.2  1996/05/14  00:07:47  irene
# Changed to use DMA assist engine. This is controlled by turning on USE_DA compilation flag.
#
# Revision 1.0  1996/05/02  23:02:01  irene
# First pass.
#
*
*
*
.endif
;
;===========================================================================
; Firmware for destination side Roadrunner
;
; some compilation defines:
;   TRACE - compile to include tracing code.
;   SIM   - for the Indigo2 simulation bringup.
;           Doesn't hurt to leave this on, it just affects some
; 	    mailbox init values which the 4640 overwrites.
;   RIO_GRANDE - stuff specific to Rio Grande cards, not the
;		 real lego HIPPI-serial h/w.
;   REV_A - workarounds for Rev. A RR chip errata.
;   USE_DA - if defined, use DMA Assist, otherwise f/w directly
;	     programs the DMA registers
;   EN_PAR - if defined, enable parity checking, but turn it off
;            every time we read the recv descriptor, and back on again.
;            If not defined, then just run without par checking all the time.
;            ( RR Rev A errata, not fixed in Rev B - writes bad parity to
;            recv descriptors. )

DST_RR = 1
USE_DA = 1
TRACE = 1
SIM = 1
; RIO_GRANDE = 1	
HALF_DUPLEX = 1
; DEBUG = 1
EN_PAR = 1

.include "rr.h"

; Note on flow of control:
;   The idle loop is "main_loop", where the RR sits watching its
;   event register. If any of the events that it is interested in
;   is triggered, it jumps to the event handler (eh_*) for that event.
;   Event handlers always return to main_loop. Besides event handlers,
;   there are routines which can be called by event handlers. The
;   convention is that routines always preserve s* registers, and
;   return to the address in register ra, whereas event handlers feel
;   free to clobber s* registers, and don't use ra.
;
;; Data is defined as ".text 0", instructions in ".text 1" because
;; I want the data to be before the text, in low mem - the mboxes
;; and GCA have to be there for host access. Also, need to make
;; sure that all data labels are < 32K so we can use offset(r0) to
;; access.

.text 0	    ;; really data - want it in low mem above text

;; -------------------- Begin GCA definition ---------------
;; First 512 bytes of SRAM is GCA, accessible by host and RR. 
;; First 128 bytes of GCA is 32 mailboxes. We only care about
;; the 1st 2.
.ifdef SIM
mbox0:	.word	start
mbox1:	.word	0
.block	112
mbox30:	.word 0
mbox_status:	.word	0
.else
.equ	mbox0, 0
.equ	mbox1, 4	    ; dst timeout? currently unused
.equ	mbox30, 120
.equ	mbox_status, 124    ; RR writes init/Glink status here
.endif
;; Next 128 bytes of GCA is init info from LINC's 4640 containing 
;; CPCI-bus-addressed ptrs to various things in its SDRAM.
.ifdef SIM
.ifdef G2P
l2rtabp:    .word   0		; unused by dst side
r2ldesc_s:  .word   0x08f80008	; Receive Desc Ring Start
r2ldesc_e:  .word   0x08f80800	; Receive Desc Ring End
r2ldata_s:  .word   0x08f80800	; Receive Data Ring Start
r2ldata_e:  .word   0x09000000	; Receive Data Ring End
lcons_p:    .word   0x08f80000	; to 2 consumer pointers
.else
l2rtabp:    .word   0		; unused by dst side
r2ldesc_s:  .word   0x80000008	; Receive Desc Ring Start
r2ldesc_e:  .word   0x80000800	; Receive Desc Ring End
r2ldata_s:  .word   0x80000800	; Receive Data Ring Start
r2ldata_e:  .word   0x80010000	; Receive Data Ring End
lcons_p:    .word   0x80000000	; to 2 consumer pointers
.endif
.else
.equ	l2rtabp,   128	; not used on destination side
.equ	r2ldesc_s, 132	; Receive Desc Ring Start
.equ	r2ldesc_e, 136	; Receive Desc Ring End
.equ	r2ldata_s, 140	; Receive Data Ring Start
.equ	r2ldata_e, 144	; Receive Data Ring Start
.equ	lcons_p,   148	; to 2 consumer pointers
.endif
;; Next 2 blocks of 128 each reserved for cmd data and acks should
;; we ever need any further commands

;; ------------------- End of GCA ---------------------------

.org 0x200

; Data begins here, followed by instructions at label "start:"
;   First word is start address, so 4640 knows what to put in
;   our PC before un-halting us.

startpc:    .word start
trace_ix:   .word 0	; next entry to write in trace buffer ring
lcons_desc: .word 0	; local copy of LINC's consumer pointers
lcons_data: .word 0
r2ldata_c:  .word 0	; Linc's Data Consumer
r2ldata_size: .word 0	; Linc Data Ring Size (r2ldata_e - r2ldata_s)
r2ldesc_p:  .word 0	; Linc Descr Ring Producer
r2ldesc_c:  .word 0	; Linc Descr Ring Consumer
r2ldesc_size:	.word 0 ; Linc Descr Ring Size (r2ldesc_e - r2ldesc_s)
nextDMA:    .word 0	; next write DMA is DATA or DESC?
DATA = 1
DESC = 2
begPkt:	    .word 0	; is next data DMA the beginning of a packet?
nextvb:	    .word 0	; next Validity Bit to use in Linc rcv descriptors

; Offsets for Linc Receive descriptor entry
LDESC_ENTRY_SIZE = 12
HADDR = 0	; host addr in word 0
DLEN  = 4	; length of data described by this descriptor
FLAGS = 8	; flags (b31=VB, b30=IFP, b29=EOP) and status (bits 15-0)

; Defines for flag bits, shifted right 16, use "lui" to load reg
FLAG_VB  = 0x8000   ; validity bit
FLAG_IFP = 0x4000   ; Ifield present (first word of data is Ifield)
FLAG_EOP = 0x2000   ; End of packet (if zero, packet cont'd in next descr)

; -------------------- Miscellaneous statistics ----------------
s_rxattns:	    .word 0
s_wdmaattns:	    .word 0
s_dtimeouts:	    .word 0
s_lincdesc_waits:   .word 0
s_lincdata_waits:   .word 0
s_dadesc_waits:	    .word 0

; ----------------- receive descriptors going back to host ------
LINC_DESC_ENTRYSIZE = 12
LINC_DESC_NENTRIES = 32
LINC_DESC_TABSIZE = (LINC_DESC_ENTRYSIZE * LINC_DESC_NENTRIES)

ldtab_s:    .block LINC_DESC_TABSIZE
ldtab_e:

;; ------------------- DMA assist rings -------------------------
.align 4*DAD_RINGSIZE
DA_base:
DARHring:	.block 512  ; read hipri
DAWHring:	.block 512  ; write hipri
DARLring:	.block 512  ; read lopri
DAWLring:	.block 512  ; write lopri

;; ------------------------ Trace Buffer
; Each trace entry is 4 words:
;	word 0:	    opcode (2 bytes), timestamp (2 bytes)
;	words 1,2,3:	trace args, depending on opcode
; trace_ix is index of next entry to write trace to.
TRACE_ENTRY_SIZE = 16
TRACE_BUF_SIZE	= 4096
TRACE_BUF_MASK	= TRACE_BUF_SIZE - 1

.align	TRACE_BUF_SIZE
trace_buf:	.block TRACE_BUF_SIZE
trace_end:

;; ================== End of data section ====================== 
.eject 
;; ================== Begin Text ===============================
.text 1	    ;; instructions begin here

.ifdef TRACE
; trace(opcode, arg1, arg2, arg3)
;	expects args in registers a0,a1,a2,a3
;	a0 should contain opcode in most signif. byte, i.e. caller
;	   should use lui to load a0 with appropriate TOC_*    
; each trace entry is 4 words, 
;	1st word is opcode (msB) | timestamp (3 lsB)
;	next 3 args are opcode-dependent
; Variable trace_ix contains index (sort of) of next trace entry to
; use. It is incremented not by 1, but by trace entry size.
trace:
	lw	t1,regBase,TimerLowReg
	lw	t0,r0,trace_ix
	load_l	t2,0xffffff
	and	t1,t1,t2
	or	t1,t1,a0

	sw	t1,t0,trace_buf
	sw	a1,t0,trace_buf+4
	sw	a2,t0,trace_buf+8
	sw	a3,t0,trace_buf+12

	; update trace_ix, check wrap
	addi	t0,t0,TRACE_ENTRY_SIZE
	andi	t0,t0,TRACE_BUF_MASK
	jr	ra
	sw	t0,r0,trace_ix
.endif	;TRACE

start:
;; Initialize everything in sight. Don't assume vars contain
;; zeros as we may have been reset and memory is in unknown state.
init:
	; clear init status field, will post result when done
	sw	r0,r0,mbox_status
	load_l	sp,TX_DESC_BEGIN
	sw	r0,r0,trace_ix    ; trace_ix = 0
	lui	regBase, (REGBASE >> 16)

	; ---------- Set PCI Latency Timer to max. Otherwise DMA
	;  on the Indigo2 slows to 5MB/sec.
	;
	;  XXX - should this be set for real HIPPI Serial card?

	load_s	t0,0xff00
	sw	t0,regBase,PCI_BistLatReg

	; ---------- disable/reset DMA
	zero_rr	AssistStateReg   ; disable Assist first
	load_s	t0,1
	lw	t0,regBase,DW_StateReg	    ; DW_StateReg = 1 (reset)
	lw	t0,regBase,DR_StateReg	    ; DR_StateReg = 1 (reset)

	;; --------------  Reset/disable HIPPI interfaces
	sw	r0,regBase,HX_StateReg	; enable after initializing descrs

	; HR_StateReg = 0xff800002 (clear all error bits, 
	;			    force to passive reset - NO ENABLE)
	load_l	t0,0xff800002
	sw	t0,regBase,HR_StateReg

.ifdef RIO_GRANDE
.ifdef HALF_DUPLEX
	; leave initializing to host program glinksync
.else
	;; -------------- Init G-links 
	; init sequence, write 6, 7, 3 to ExtSerReg
	load_s	t0,6
	sw	t0,regBase,ExtSerDataReg
	load_s	t1,100
1:
	bne	t1,r0,1b
	addi	t1,t1,-1

	load_s	t0,7
	sw	t0,regBase,ExtSerDataReg
	load_s	t1,100
1:
	bne	t1,r0,1b
	addi	t1,t1,-1

	load_s	t0,3
	sw	t0,regBase,ExtSerDataReg
	load_s	t1,100
1:
	bne	t1,r0,1b
	addi	t1,t1,-1
.endif
.endif
	; --------------- Init Misc Control Registers
	; MiscHostReg = 0
	zero_rr	MiscHostReg

	; MiscLocalReg = 0x2702 (enable dcache, lpar chk, 
	;			 dbl descs, add'l SRAM. Clear intr)
	; Rev A bugs: 1. dcache occasionally causes loads of 
	;		 wrong values to registers
	;	      2. Receive hardware writes wrong parity to
	;		 receive descriptors.
	
; XXX - DON'T ENABLE DCACHE TILL REV B !
.ifdef REV_A
	MISC_LCL = 0x0200
.else
.ifdef TEST_PLD
	MISC_LCL = 0xd200
.else
	MISC_LCL = 0xc200
	;; ideally, should 0xe600 but I'm not sure that enabling
	;; dcache at this point is worth the risk. This bug supposedly
	;; fixed in Rev B, but then so were a bunch of others.
	;; -- Irene.12/13/96. (Someone reading this after I'm gone
	;; is welcome to try it.)
.endif
.endif
	
.ifdef EN_PAR
	MISC_LCL_PAR = (MISC_LCL | 0x0400)
	load_s  t0, MISC_LCL_PAR
.else
	load_s  t0,MISC_LCL
.endif
	sw	t0,regBase,MiscLocalReg	

	load_l	t0,0xffffffff
	sw	t0,regBase,MboxEvtReg	    ; clear MboxEvtReg
	zero_rr	TimerHiReg		    ; TimerHiReg = 0
	zero_rr	TimerLowReg		    ; TimerLowReg = 0
	lui	t0,0x6000
	sw	t0,regBase,TimerRefReg	    ; TimerRefReg = 0x60000000
	zero_rr	CPUPriReg		    ; CPUPriReg = 0

	;; --------------  Set up receive ptrs
	load_l	t0,RX_BUF_BEGIN
	sw	t0,regBase,RCV_BaseReg	    ; RCV_BaseReg = RX_BUF_BEGIN
	sw	t0,regBase,RCV_ProdReg	    ; RCV_ProdReg = RX_BUF_BEGIN
	sw	t0,regBase,RCV_ConsReg	    ; RCV_ConsReg = RX_BUF_BEGIN
	or	dataRef,t0,t0		    ; dataRef     = RX_BUF_BEGIN

	load_l	t0,RX_DESC_BEGIN
	sw	t0,regBase,RCV_DescProdReg  ; RCV_DescBaseReg = RX_DESC_BEGIN
	sw	t0,regBase,RCV_DescConsReg  ; RCV_DescConsReg = RX_DESC_BEGIN
	sw	t0,regBase,RCV_DescRefReg   ; RCV_DescRefReg = RX_DESC_BEGIN

	;; -------------- Set up transmit ptrs
	load_l	t0,TX_BUF_BEGIN
	sw	t0,regBase,XMT_BaseReg	    ; XMT_BaseReg = TX_BUF_BEGIN
	sw	t0,regBase,XMT_ConsReg	    ; XMT_ConsReg = TX_BUF_BEGIN
	sw	t0,regBase,XMT_ProdReg	    ; XMT_ProdReg = TX_BUF_BEGIN

	load_l	t0,TX_DESC_BEGIN
	sw	t0,regBase,XMT_DescProdReg  ; XMT_DescBaseReg = TX_DESC_BEGIN
	sw	t0,regBase,XMT_DescConsReg  ; XMT_DescConsReg = TX_DESC_BEGIN
	sw	t0,regBase,XMT_DescRefReg   ; XMT_DescRefReg = TX_DESC_BEGIN

	;; -------------- HIPPI Overhead register
	; XXX temp for RR bug - see rev A errata, disable OH1 transmission
	; H_OvrheadReg = 0x00020000
	lui	t0,0x0002
	sw	t0,regBase,H_OvrheadReg

	; Check to make sure OH8 is synchronized (bit 0 == 1)
	; *** NOTE: Must take Receive out of reset-passive state before
	; ***       OH8-synced bit will turn on.
	load_l	t0,0xff800000		; not passive-reset, but not enabled
	sw	t0,regBase,HR_StateReg

	; Loop forever or till it comes on. While looping,
	; write mbox_status every 100 reads to let 4640 know that
	; we are still alive.
	load_s	t1,100
1:
	lw	t0,regBase,H_OvrheadReg
	bgtz	t1,2f
	load_s	t2,GLINK_NOT_RDY
	sw	t2,r0,mbox_status	; report init status
	noop
	noop
	beq	r0,r0,1b
	load_s	t1,100
2:
	andi	t0,t0,1
	beq	t0,r0,1b
	addi	t1,t1,-1
	;; ------------- OH8 sync OK.


	;; ------------- HIPPI Tx & Rx State Registers
	; count 100 waiting for G-links to issue Link Ready
	load_s	t1,100
	load_l	t2,RX_STATE_LNKRDY
1:
	lw	t0,regBase,HR_StateReg
	bgtz	t1,2f
	load_s	t3,GLINK_NOT_RDY
	sw	t3,r0,mbox_status
	noop
	noop
	beq	r0,r0,1b
	load_s	t1,100
2:
	and	t0,t0,t2
	beq	t0,r0,1b
	addi	t1,t1,-1

.ifdef HALF_DUPLEX
	; HR_StateReg = 0xff800010  // clear all error bits, NO ENABLE
	load_l	t0,0xff800010
	; HX_StateReg = 0x01000000
	load_l	t1,0x01000000
.else
	; HR_StateReg = 0xff800001  // clear all error bits, NO ENABLE
	load_l	t0,0xff800000
	; HX_StateReg = 0x01000001
	load_l	t1,0x01000001
.endif
	sw	t0,regBase,HR_StateReg
	sw	t1,regBase,HX_StateReg

	;; --------------- Host DMA registers
	zero_rr	DW_HAddrHiReg
	zero_rr	DW_HAddrLowReg
	zero_rr	DR_HAddrHiReg
	zero_rr	DR_HAddrLowReg
	zero_rr	DR_LengthReg
	zero_rr	DW_LAddrReg
	zero_rr	DW_ChksumReg
	zero_rr	DW_LengthReg

	zero_rr	DR_LAddrReg
	zero_rr	DR_ChksumReg

	load_s	t0,DARHring
	sw	t0,regBase,AssistBaseReg    ; AssistBaseReg = DARHring
	; Read-Hi Prod,Cons.Ref = DARHring
	sw	t0,regBase,DRHi_ProdReg
	sw	t0,regBase,DRHi_ConsReg
	sw	t0,regBase,DRHi_RefReg

	; Write-Hi Prod,Cons.Ref = DAWHring
	load_s	t0,DAWHring
	sw	t0,regBase,DWHi_ProdReg
	sw	t0,regBase,DWHi_ConsReg
	sw	t0,regBase,DWHi_RefReg

	; Read-Lo Prod,Cons.Ref = DARLring
	load_s	t0,DARLring
	sw	t0,regBase,DRLo_ProdReg
	sw	t0,regBase,DRLo_ConsReg
	sw	t0,regBase,DRLo_RefReg

	; Write-Lo Prod,Cons.Ref = DAWLring
	load_s	t0,DAWLring
	sw	t0,regBase,DWLo_ProdReg
	sw	t0,regBase,DWLo_ConsReg
	sw	t0,regBase,DWLo_RefReg

	;; Set all the DA read descriptors with StateReg field
	;; of 0x16 (threshold=1, active=1, noswap=1, no checksumming)
	load_s	t0,0x16
	load_s	t1,DARHring+DAD_STATE
.ifdef NOTYET
; .irp's confuse assembler, throws listing off so instr and source
; don't match.
	.irp	param,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	sw	t0,t1,\param * DAD_SIZE
	.endr
.else
	sw	t0,t1,0
	sw	t0,t1,DAD_SIZE
	sw	t0,t1,2*DAD_SIZE
	sw	t0,t1,3*DAD_SIZE
	sw	t0,t1,4*DAD_SIZE
	sw	t0,t1,5*DAD_SIZE
	sw	t0,t1,6*DAD_SIZE
	sw	t0,t1,7*DAD_SIZE
	sw	t0,t1,8*DAD_SIZE
	sw	t0,t1,9*DAD_SIZE
	sw	t0,t1,10*DAD_SIZE
	sw	t0,t1,11*DAD_SIZE
	sw	t0,t1,12*DAD_SIZE
	sw	t0,t1,13*DAD_SIZE
	sw	t0,t1,14*DAD_SIZE
	sw	t0,t1,15*DAD_SIZE
.endif

	load_s	t1,DARLring+DAD_STATE
.ifdef NOTYET
	.irp	param,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	sw	t0,t1,\param * DAD_SIZE
	.endr
.else
	sw	t0,t1,0
	sw	t0,t1,DAD_SIZE
	sw	t0,t1,2*DAD_SIZE
	sw	t0,t1,3*DAD_SIZE
	sw	t0,t1,4*DAD_SIZE
	sw	t0,t1,5*DAD_SIZE
	sw	t0,t1,6*DAD_SIZE
	sw	t0,t1,7*DAD_SIZE
	sw	t0,t1,8*DAD_SIZE
	sw	t0,t1,9*DAD_SIZE
	sw	t0,t1,10*DAD_SIZE
	sw	t0,t1,11*DAD_SIZE
	sw	t0,t1,12*DAD_SIZE
	sw	t0,t1,13*DAD_SIZE
	sw	t0,t1,14*DAD_SIZE
	sw	t0,t1,15*DAD_SIZE
.endif
	;; Set all the DA write descriptors with StateReg field
	;; of 0x20016 (disable producer-compare, no checksumming,
	;;	       threshold = 1, active = 1, noswap = 1)
	lui	t0,2
	ori	t0,t0,0x16
	load_s	t1,DAWHring+DAD_STATE
.ifdef NOTYET
	.irp	param,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	sw	t0,t1,\param * DAD_SIZE
	.endr
.else
	sw	t0,t1,0
	sw	t0,t1,DAD_SIZE
	sw	t0,t1,2*DAD_SIZE
	sw	t0,t1,3*DAD_SIZE
	sw	t0,t1,4*DAD_SIZE
	sw	t0,t1,5*DAD_SIZE
	sw	t0,t1,6*DAD_SIZE
	sw	t0,t1,7*DAD_SIZE
	sw	t0,t1,8*DAD_SIZE
	sw	t0,t1,9*DAD_SIZE
	sw	t0,t1,10*DAD_SIZE
	sw	t0,t1,11*DAD_SIZE
	sw	t0,t1,12*DAD_SIZE
	sw	t0,t1,13*DAD_SIZE
	sw	t0,t1,14*DAD_SIZE
	sw	t0,t1,15*DAD_SIZE
.endif

	load_s	t1,DAWLring+DAD_STATE
.ifdef NOTYET
	.irp	param,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	sw	t0,t1,\param * DAD_SIZE
	.endr
.else
	sw	t0,t1,0
	sw	t0,t1,DAD_SIZE
	sw	t0,t1,2*DAD_SIZE
	sw	t0,t1,3*DAD_SIZE
	sw	t0,t1,4*DAD_SIZE
	sw	t0,t1,5*DAD_SIZE
	sw	t0,t1,6*DAD_SIZE
	sw	t0,t1,7*DAD_SIZE
	sw	t0,t1,8*DAD_SIZE
	sw	t0,t1,9*DAD_SIZE
	sw	t0,t1,10*DAD_SIZE
	sw	t0,t1,11*DAD_SIZE
	sw	t0,t1,12*DAD_SIZE
	sw	t0,t1,13*DAD_SIZE
	sw	t0,t1,14*DAD_SIZE
	sw	t0,t1,15*DAD_SIZE
.endif

.ifdef USE_DA
	; ***** Caution: If Assist is enabled, you can't directly
	; *****          control the DMA registers even if you leave
	; *****          the queues empty. This means you can't use 
	; *****          assist for one direction only!

	; Assist Rings all initialized, now enable Assist
	load_s	t0,DEFAULT_ASST_STATE
	sw	t0,regBase,AssistStateReg
.endif

	; Initialize evtMask register to Rx attn and Rx started/complete
	load_l	evtMask,DEFAULT_EVTMASK

	; r2ldata_c = r2ldata_p = r2ldata_s
	; r2ldata_size = r2ldata_e - r2ldata_s
	; spaceleft = r2ldata_e - r2ldata_p - 1024;
	lw	r2ldata_p,r0,r2ldata_s
	lw	t0,r0,r2ldata_e
	sw	r2ldata_p,r0,r2ldata_c
	subu	t0,t0,r2ldata_p
	sw	t0,r0,r2ldata_size
	addi	spaceleft,t0,-1024

	; lento64kb = 0x10000 - (r2ldata_p & 0xffff);
	lui	lento64kb,1
	andi	t0,r2ldata_p,0xffff
	subu	lento64kb,lento64kb,t0

	; r2ldesc_c = r2ldesc_p = r2ldesc_s
	; r2ldesc_size = (r2ldesc_e - r2ldesc_s)
	lw	t0,r0,r2ldesc_s
	lw	t1,r0,r2ldesc_e
	sw	t0,r0,r2ldesc_p
	sw	t0,r0,r2ldesc_c
	subu	t1,t1,t0
	sw	t1,r0,r2ldesc_size

	; Initialize first entry in local Linc descriptor table
	load_s	ldtab_p,ldtab_s	; ldtab_p = &ldtab
	load_l	t0,0x03ffffff
	and	t0,t0,r2ldata_p
	sw	t0,ldtab_p,HADDR ;ldtab_p->haddr = r2ldata_p & 0x03ffffff;
	sw	r0,ldtab_p,DLEN	 ;ldtab_p->dlen = 0;
	lui	t0,FLAG_VB
	sw	t0,ldtab_p,FLAGS ;ldtab_p->flags = 0x80000000 //validity bit
	sw	r0,r0,nextvb	; nextvb = 0
	load_s	t0,DATA
	sw	t0,r0,nextDMA	; nextDMA = DATA

	load_s	t0,1
	sw	t0,r0,begPkt	; begPkt = TRUE

.ifdef TRACE
	; TRACE (TOC_INIT)
	jal	trace
	lui	a0,TOC_INIT
.endif
	;; Finished init, tell 4640 by writing "1" in mbox_status
	load_s	t0,INIT_SUCCESS
	sw	t0,r0,mbox_status

main_loop:
	lw	t0,regBase,MainEvtReg
	load_a	t1,evt_jump_table
	and	t0,t0,evtMask
	beq	t0,r0,main_loop
	pri	t2,t0,evtMask	    ; delay slot
	joff	t1,t2
	noop

	; NOT REACHED

.align 256
evt_jump_table:
	j	eh_sw0	    	; event bit 0
	noop
	j	eh_sw1		; event bit 1
	noop
	j	eh_txdone	; event bit 2
	noop
	j	eh_sw2		; event bit 3
	noop
	j	eh_sw3		; event bit 4
	noop
	j	eh_receive	; event bit 5, receive complete
	noop
	j	eh_receive	; event bit 6, receive started
	noop
	j	eh_sw4		; event bit 7
	noop
	j	eh_sw5		; event bit 8
	noop
	j	eh_sw6		; event bit 9
	noop
	j	eh_sw7		; event bit 10
	noop
	j	eh_DAwldone	; event bit 11
	noop
	j	eh_DArldone	; event bit 12
	noop
	j	eh_DAwhdone	; event bit 13
	noop
	j	eh_DArhdone	; event bit 14
	noop
	j	eh_sw8		; event bit 15
	noop
	j	eh_wrDMAdone	; event bit 16
	noop
	j	eh_rdDMAdone	; event bit 17
	noop
	j	eh_wrDMAattn	; event bit 18
	noop
	j	eh_rdDMAattn	; event bit 19
	noop
	j	eh_sw9		; event bit 20
	noop
	j	eh_sw10		; event bit 21
	noop
	j	eh_extserial	; event bit 22
	noop
	j	eh_mbox		; event bit 23
	noop
	j	eh_sw11		; event bit 24
	noop
	j	eh_sw12		; event bit 25
	noop
	j	eh_rxattn	; event bit 26
	noop
	j	eh_sw13		; event bit 27
	noop
	j	eh_txattn	; event bit 28
	noop
	j	eh_sw14		; event bit 29
	noop
	j	eh_timer	; event bit 30
	noop
	j	eh_sw15		; event bit 31
	noop

;; ----------------------- Event handlers:
eh_sw0:
eh_sw1:
eh_txdone:
eh_sw2:
eh_sw3:
eh_sw4:

eh_sw5:
eh_sw6:
eh_sw7:
eh_DAwldone:
eh_DArldone:
.ifndef USE_DA
eh_DAwhdone:
eh_DArhdone:
.endif
eh_sw8:
.ifdef USE_DA
eh_wrDMAdone:
eh_rdDMAdone:
.endif
eh_rdDMAattn:
eh_sw9:
eh_sw10:
eh_extserial:

eh_sw11:
eh_sw12:
eh_sw13:
eh_txattn:
eh_sw14:
eh_timer:
eh_sw15:
;; Unexpected event. Panic?!?!
	sw	t2,r0,mbox30
.ifdef TRACE
	move	a1,t2	    ; the event we joff-ed on.
	jal	trace
	lui	a0,TOC_BADEVT
.endif
	noop
	noop
	noop
	noop
	noop
	noop	; noops needed for trace_ix update to clear pipeline
	halt
	noop

.eject
;; ===================================================================
;; eh_mbox
;; Event handler for mbox msg. This is the 4640 querying to see
;; if we are alive or to tell us if accept state should change.
;; if msg is REJECT_CONN, then receive interface is disabled.
;; Completion is signified by reseting the mbox to link state.
eh_mbox:
	load_l	t0,0xffffffff
	sw	t0,regBase,MboxEvtReg   ; clear mbox event

	lw	t0, r0, mbox_status
	load_l	t1, 0
	beq	t0, t1, 3f	; don't change receive status?
	load_s	t1, REJECT_CONN
	
	beq	t0, t1, 2f
	;; next instruction is in delay slot!
.ifdef HALF_DUPLEX
	load_s	t1,0x10
.else
	load_s	t1,0x00
.endif

1:	; enable connections
	ori	t1, t1, 1
2:	
	sw	t1,regBase,HR_StateReg
3:	
	load_s	t1,INIT_SUCCESS
	j	main_loop		; return to main loop
	sw	t1,r0,mbox_status	; report init status
.eject	
; --------------- Event Handler for Write DMA attn ------------------
eh_wrDMAattn:
	lw	t0,r0,s_wdmaattns
	lw	s0,regBase,DWHi_ConsReg
	addi	t0,t0,1
	sw	t0,r0,s_wdmaattns
.ifdef TRACE
	; trace (TOC_WATTN1, haddr, laddr, len)
	; trace (TOC_WATTN2, dma state, dma asst cons, dma asst state)

	lw	a1,regBase,DW_HAddrLowReg
	lw	a2,regBase,DW_LAddrReg
	lw	a3,regBase,DW_LengthReg
	lw	s1,regBase,AssistStateReg
	jal	trace
	lui	a0,TOC_WATTN1

	lw	a1,regBase,DW_StateReg
	move	a2,s0
	move	a3,s1
	jal	trace
	lui	a0,TOC_WATTN2
.endif
	noop
	noop
	noop
	noop
	noop
.ifdef USE_DA
	; First pause the DMA assist
	load_s	t0,(DEFAULT_ASST_STATE | DA_PAUSE_BIT)
	sw	t0,regBase,AssistStateReg

	;;  According to Mark Bryers, when we reset the DMA
	;; it should re-load the current DMA assist descr, but it
	;; doesn't - it goes on to the next one. So we back
	;; up the consumer ptr before reset/clear the DMA.
	addi	t1,s0,-DAD_SIZE
	sw	t1,regBase,DWHi_ConsReg
.endif	

	; Reset the write DMA state reg. 

	load_s	t0,1			    ; Put it in reset
	sw	t0,regBase,DW_StateReg
	noop
	noop
	noop
	noop
	noop
	sw	r0,regBase,DW_StateReg	    ; take it out of reset
	noop
	noop
	noop
	noop

.ifdef USE_DA
	; Now un-pause the DMA Assist.
	; This should cause the DMA assist
	; engine to reload the current DMA asst descr.

	load_s	t0,DEFAULT_ASST_STATE
	sw	t0,regBase,AssistStateReg

	j	main_loop
	noop
.else
	; XXX Retry? but not sure whether the benefits of that are worth
	; saving haddr/laddr/len? Besides Rev A fifo overrun bug, this
	; should not happen unless f/w has screwed up address/length
	; calculation.
	j	main_loop
	noop
.endif

; -----------------Event handler for Receive Attention ----------------
; Just increment the statistic and clear the error bits so that
; the event doesn't trigger again.

eh_rxattn:
	;; Rx attn handler
.ifdef TRACE
	lw	a1,regBase,HR_StateReg
	lw	a2,regBase,RCV_DescProdReg
.endif

	lw	t1,r0,s_rxattns
.ifdef HALF_DUPLEX
	; HR_StateReg = 0xff800011  // clear all error bits, enable rx
	load_l	t0,0xff800011
.else
	; HR_StateReg = 0xff800001  // clear all error bits, enable rx
	load_l	t0,0xff800001
.endif
	sw	t0,regBase,HR_StateReg

	addi	t1,t1,1
	sw	t1,r0,s_rxattns

.ifdef TRACE
	jal	trace
	lui	a0,TOC_RXATTN
.endif

	j	main_loop
	noop


; ---------------- Event handler for Read DMA completed ------------
;
; Cancel event notification and update registers from the DMA 
; destination buffer. If we were blocked for resources and resources
; have freed up, go take action.
;
.ifdef USE_DA
eh_DArhdone:
	; ++ refReg
	lw	t0,regBase,DRHi_RefReg
	noop
	addi	t0,t0,DAD_SIZE
	sw	t0,regBase,DRHi_RefReg
.else
eh_rdDMAdone:
	; evtMask &= ~EVT_RDMADONE
	lui	t0,(EVT_RDMADONE >> 16)
	nor	t0,t0,t0
	and	evtMask,evtMask,t0
.endif

.ifdef TRACE
	; trace (TOC_RDMADONE, lcons_desc, lcons_data);
	lw	a1,r0,lcons_desc
	lw	a2,r0,lcons_data
	jal	trace
	lui	a0,TOC_RDMADONE
.endif

.ifdef USE_DA
; if ((rrState == NORMAL_STATE) || (rrState == DADESC_WAIT_STATE)) {
	load_s	t0,NORMAL_STATE
	beq	rrState,t0,3f
	load_s	t0,DADESC_WAIT_STATE
	bne	rrState,t0,1f
3:
.else
; if (rrState == NORMAL_STATE) { 
	load_s	t0,NORMAL_STATE
	bne	rrState,t0,1f
.endif
	lw	t1,r0,lcons_data
	lw	t2,r0,lcons_desc
	sw	t1,r0,r2ldata_c		; r2ldata_c = lcons_data
	sw	t2,r0,r2ldesc_c		; r2ldesc_c = lcons_desc
	; if ((spaceleft = r2ldata_c - r2ldata_p) <= 0)
	;    spaceleft += r2ldata_size;
	; spaceleft -= 1024
	subu	spaceleft,t1,r2ldata_p
	bgtz	spaceleft,2f
	lw	t3,r0,r2ldata_size
	noop
	addu	spaceleft,spaceleft,t3
2:
	j	main_loop
	addi	spaceleft,spaceleft,-1024
;}
1:

; if (rrState == LDESC_WAIT_STATE) {
	load_s	t0,LDESC_WAIT_STATE
	bne	rrState,t0,2f
	lw	t1,r0,lcons_data
	lw	t2,r0,lcons_desc
	lw	t3,r0,r2ldesc_c

	; if ((spaceleft = r2ldata_c - r2ldata_p) <= 0)
	;    spaceleft += r2ldata_size;
	; spaceleft -= 1024
	subu	spaceleft,t1,r2ldata_p
	bgtz	spaceleft,1f
	lw	t4,r0,r2ldata_size
	noop
	addu	spaceleft,spaceleft,t4
1:
	addi	spaceleft,spaceleft,-1024

;; if (r2ldesc_c != lcons_desc) {
	beq	t2,t3,nochange
	sw	t1,r0,r2ldata_c		; r2ldata_c = lcons_data
	sw	t2,r0,r2ldesc_c		; r2ldesc_c = lcons_desc
	load_s	rrState,NORMAL_STATE	; rrState = NORMAL_STATE
.ifdef USE_DA
	; Turn on rcv done/started notification
	load_s	t0,(EVT_RXDONE | EVT_RXSTARTED)
	or	evtMask,evtMask,t0
.endif
	j	send_desc		; send_desc returns to main_loop
	load_s	ra,main_loop
;; }
; }
2:
; else (rrState == LDATA_WAIT_STATE) {	// waiting for space in LINC's dataring
	lw	t2,r0,lcons_data
	lw	t3,r0,r2ldata_c
	lw	t1,r0,lcons_desc

;; if (r2ldata != lcons_data) {
	beq	t2,t3,nochange
	sw	t1,r0,r2ldesc_c		; r2ldesc_c = lcons_desc
	sw	t2,r0,r2ldata_c		; r2ldata_c = lcons_data

	; if ((spaceleft = r2ldata_c - r2ldata_p) <= 0)
	;    spaceleft += r2ldata_size;
	; spaceleft -= 1024
	subu	spaceleft,t2,r2ldata_p
	bgtz	spaceleft,1f
	lw	t4,r0,r2ldata_size
	noop
	addu	spaceleft,spaceleft,t4
1:
	addi	spaceleft,spaceleft,-1024

	load_s	rrState,NORMAL_STATE	; rrState = NORMAL_STATE
	load_s	t0,(EVT_RXSTARTED | EVT_RXDONE)
	j	main_loop
	or	evtMask,evtMask,t0      ; evtMask|=(EVT_RXSTARTED|EVT_RXDONE)
;; }
;}

nochange:
	j	update_linc_cons
	load_s	ra,main_loop

; --------------- End of eh_rdDMAdone --------------------------
.eject
.ifdef USE_DA
; -------------- Event Handler for Write DMA Assist complete ----------
; This event is triggered whenever the Write High DMA Assist's
; Reference and Consumer pointers are different.
;
eh_DAwhdone:
	; DMA_Assist_Desc * dap	    // register s0
	; dap = DWHi_RefReg;
	lw	s0,regBase,DWHi_RefReg
	noop

; while (dap != DWHi_ConsReg) {
1:
.ifdef TRACE
	; trace (TOC_WDMADONE,dap->haddr,dap->laddr,dap->dmalen)
	lw	a1,s0,DAD_HADDR
	lw	a2,s0,DAD_LADDR
	lw	a3,s0,DAD_DMALEN
	jal	trace
	lui	a0,TOC_WDMADONE
.endif

	; RCV_ConsReg = dap->fw1
	; ++dap
	lw	t0,s0,DAD_FW1
	addi	s0,s0,DAD_SIZE
	; Store and re-load of Ref reg uses h/w to deal with ring wrap.
	sw	s0,regBase,DWHi_RefReg
	lw	s1,regBase,DWHi_ConsReg
	lw	s0,regBase,DWHi_RefReg
	sw	t0,regBase,RCV_ConsReg	; release buf space to rcv h/w
	bne	s0,s1,1b
	noop
; }

	load_s	t1,DADESC_WAIT_STATE
	beq	rrState,t1,3f
	lw	t0,r0,nextDMA
2:
	j	main_loop
	noop
3:
	; back to normal state, with rcv started/done events turned on.
	load_s	rrState,NORMAL_STATE
	ori	evtMask,evtMask,(EVT_RXSTARTED | EVT_RXDONE)

	; if (nextDMA == DESC) { send_desc();}
	load_s	t1,DESC
	bne	t0,t1,2b
	noop
	j	send_desc
	load_s	ra,main_loop

.else
; -------------- Event Handler for Write DMA complete ----------

eh_wrDMAdone:

.ifdef TRACE
	; trace (TOC_WDMADONE,haddr,laddr)
	lw	a1,regBase,DW_HAddrLowReg
	lw	a2,regBase,DW_LAddrReg
	lw	a3,regBase,DW_LengthReg
	jal	trace
	lui	a0,TOC_WDMADONE
.endif

	; evtMask &= ~EVT_WDMADONE
	lui	t0,(EVT_WDMADONE >> 16)
	nor	t0,t0,t0
	and	evtMask,evtMask,t0

	; release data buffer space to rcv h/w
	sw	dataRef,regBase,RCV_ConsReg

	; if (nextDMA == DESC) { send_desc(); return}
	lw	t0,r0,nextDMA
	load_s	t1,DESC
	bne	t0,t1,1f
	noop
	j	send_desc
	load_s	ra,main_loop
1:
	; else { evtMask |= (RXSTARTED|RXDONE); return;	}
	j	main_loop
	ori	evtMask,evtMask,(EVT_RXSTARTED | EVT_RXDONE)
; -------------- End of eh_wrDMAdone ------------------------
.endif
.eject
; -------------- Event handler for Receive started or complete
; eh_receive:
;
; Sends data out of h/w receive ring into LINC's receive ring.
; Register usage:
;	s0 - pointer to h/w's Rx descr for this chunk of data
;	s1 - validlen, amount of data in hand for current packet
; ifdef USE_DA
;	s2 - dap, ptr to DMA Assist descr to use for this dma
; endif
;	s3 - rxdone, flag indicating whether this packet is completely rec'd.
;	s4 - maintains adjusted count of how much we can dma
;	     for now, subject to various restrictions

eh_receive:
.ifdef TRACE
	lw	a1,regBase,RCV_DescConsReg
	lw	a2,regBase,RCV_DescRefReg
	lw	a3,regBase,RCV_DescProdReg
	jal	trace
	lui	a0,TOC_RCV
.endif	
	lw	s0,regBase,RCV_DescConsReg

;  if (spaceleft <= 0) {
	bgtz	spaceleft,2f

	; ++s_lincdata_waits, rrState = LDATA_WAIT_STATE
	lw	t0,r0,s_lincdata_waits
	load_s	rrState,LDATA_WAIT_STATE
	addi	t0,t0,1
	sw	t0,r0,s_lincdata_waits

.ifdef TRACE
	jal	trace
	lui	a0,TOC_LDATA_WAIT
.endif
	
.ifdef DTIMEOUT
	; evtMask &= ~(EVT_TIMER|EVT_RXDONE|EVT_RXSTARTED)
	load_l	t0,(EVT_TIMER|EVT_RXDONE|EVT_RXSTARTED)
.else
	; evtMask &= ~(EVT_RXDONE|EVT_RXSTARTED);
	load_s	t0,(EVT_RXDONE|EVT_RXSTARTED)
.endif
	nor	t0,t0,t0
	and	evtMask,evtMask,t0

	j	update_linc_cons
	load_s	ra,main_loop
; }
2:

.ifdef USE_DA
	; DMA_Assist_Desc * dap // in reg s2
	; dap = DWHi_ProdReg
	lw	s2,regBase,DWHi_ProdReg

; if (dap + 1 == DWHi_RefReg) { // ring full
	lw	t0,regBase,DWHi_RefReg
	; 0x200 bytes in the ring, 0x20 bytes per entry.
	load_s	t1,0x1e0
	subu	t2,s2,t0
	andi	t2,t2,0x1ff
	bne	t2,t1,1f

	; s_dadesc_waits++, rrState = DADESC_WAIT_STATE
	lw	t2,r0,s_dadesc_waits
	load_s	rrState, DADESC_WAIT_STATE
	addi	t2,t2,1
	sw	t2,r0,s_dadesc_waits

.ifdef TRACE
	move	a1,t0		; DWHi_ConsReg
	move	a2,s2		; DWHi_ProdRef
	lw	a3,regBase,DWHi_RefReg
	jal	trace
	lui	a0,TOC_DAWAIT_DATA
.endif

.ifdef DTIMEOUT
	; evtMask &= ~(EVT_RXSTARTED | EVT_RXDONE | EVT_TIMER);
	load_l	t0,(EVT_RXSTARTED | EVT_RXDONE | EVT_TIMER)
.else
	; evtMask &= ~(EVT_RXSTARTED | EVT_RXDONE);
	load_s	t0,(EVT_RXSTARTED | EVT_RXDONE)
.endif
	nor	t0,t0,t0
	j	main_loop
	and	evtMask,evtMask,t0

; } // if ring full
1:
.endif	; USE_DA

; if (begPkt) {	     // beginning of packet
	lw	t0,r0,begPkt
	noop
	beq	t0,r0,4f

	sw	r0,r0,begPkt	    ; begPkt = 0, safe in branch delayslot
.ifdef EN_PAR
	;; turn off parity checking before accessing rcv descriptor
	load_s  t2,MISC_LCL
	sw	t2,regBase,MiscLocalReg
	noop
.endif
	lw	t1,s0,0
	noop
.ifdef EN_PAR
	;; re-enable parity checking
	load_s  t2,MISC_LCL_PAR
	sw	t2,regBase,MiscLocalReg
.endif

; MSB of word 0 in hw rx descriptor indicates "Same Ifield". If 1, 
; 1st double word of data is just pad. If 0, 2nd word of the double
; word has a valid Ifield which must be DMAed to host.
;;   if (rrdp->SI) {	    // SI flag is msb
	bgez	t1,3f
	load_l	t2,RX_BUF_END
;;	if ((dataRef += 8) == RX_BUF_END) dataRef = RX_BUF_BEGIN;
	addi	dataRef,dataRef,8
	bne	dataRef,t2,4f
	noop
	load_l	dataRef,RX_BUF_BEGIN
	bgez	r0,4f
	noop
;;   } else {
3:
;;	dataRef += 4, ldtab_p->flags |= IFP
	lw	t1,ldtab_p,FLAGS
	addi	dataRef,dataRef,4
	lui	t0,FLAG_IFP
	or	t1,t1,t0
	sw	t1,ldtab_p,FLAGS
;;   }
;  }	// if (begPkt)
4:

	lw	s1,regBase,RCV_ProdReg	    ; end_data = rcv data prod
	noop
	noop
	noop
	noop
	noop
	noop

; if (RCV_DescProdReg == rrdp+4) {  ; rcv not complete
	lw	t0,regBase,RCV_DescProdReg
	addi	t1,s0,4
	bne	t1,t0,1f
	noop
;; if (end_data == dataRef) { // no new data received. Hung?
	bne	s1,dataRef,2f
	load_s	s3,0		    ; rxdone = 0

.ifdef DTIMEOUT
;;; if !(evtMask & EVT_TIMER) {
;;;;  timerRefReg = timerLowReg + DTIMEO, evtMask |= EVT_TIMER;
	lui	t0,(EVT_TIMER >> 16)
	and	t1,evtMask,t0
	bne	t1,r0,3f
	lw	t2,regBase,timerLowReg
	load_l	t3,DTIMEO
	add	t2,t2,t3
	sw	t2,regBase,timerLowReg
	or	evtMask,evtMask,t0
;;; }
3:
.endif
	j	main_loop
	noop
;; }
; } else {	// RCV_DescProdReg != rrdp+4) i.e. descriptor is complete
1:
	; end_data = rrdp->end_address
.ifdef EN_PAR
	;; turn off parity checking before accessing rcv descriptor
	load_s  t2,MISC_LCL
	sw	t2,regBase,MiscLocalReg
	noop
.endif
	ldw	s1,s0,0
	load_l	t0,0x1fffff
	and	s1,s1,t0
	;; s1 now has just ending address (without SI bit)
	;; loword (r30) has the status field.
.ifdef EN_PAR
	;; re-enable parity checking
	load_s  t2,MISC_LCL_PAR
	sw	t2,regBase,MiscLocalReg
.endif

;; if (rrdp->status & LAST_WORD_ODD) {
	lui	t1,2	    ; LAST_WORD_ODD is bit 17
	and	t0,loword,t1
	beq	t0,r0,2f
	load_s	s3,1	    ; rxdone = 1, in delay slot

	addi	s1,s1,-4    ; end_data -= 4, this may put it above 
			    ; the ring start, but wrap adjustment later
			    ; will take care of it.
;; } // if (rrdp->status & LAST_WORD_ODD)
; } // else (RCV_DescProdReg != rrdp+4)
2:

.ifdef DTIMEOUT
	; evtMask &= ~EVT_TIMER
	lui	t0,(EVT_TIMER >> 16)
	nor	t0,t0,t0
	and	evtMask,evtMask,t0
.endif

	; if ((validlen = end_data - dataRef) == 0) goto fill_desc
	subu	s1,s1,dataRef
	beq	s1,r0,fill_desc
.ifdef USE_DA
	; dap->haddr = r2ldata_p
	sw	r2ldata_p,s2,DAD_HADDR    ; delay slot but harmless
.else
	; DW_HAddrLowReg = r2ldata_p
	sw	r2ldata_p,regBase,DW_HAddrLowReg    ; delay slot but harmless
.endif
	; if (validlen < 0) validlen += RX_BUF_SIZE;
	bgtz	s1,1f
	load_l	t0,RX_BUF_SIZE
	addu	s1,s1,t0
1:	
.ifdef USE_DA
	; DW_LAddrReg = dataRef
	sw	dataRef,s2,DAD_LADDR
.else
	; DW_LAddrReg = dataRef
	sw	dataRef,regBase,DW_LAddrReg
.endif

	; OK, we have the "validlen" of data received, now check against
	; resource constraints before DMA.

	; Check against space left in LINC data ring
	; if ((dmalen = validlen) > spaceleft) dmalen = spaceleft
	subu	t0,s1,spaceleft
	blez	t0,2f
	ori	s4,s1,0
	ori	s4,spaceleft,0
2:
	; Check if dma will exceed 64KB - dmalen h/w register is
	; only 16 bits.
	; if ((dmalen >> 16) != 0) dmalen = 0xfffc;
	srl 	t1,s4,16
	beq	t1,r0,6f
	noop
	ori	s4,r0,0xfffc
6:
	; Will this DMA cross a 64KB boundary? H/W can't handle that.
	; if (dmalen >= lento64kb)
	;	{dmalen=lento64kb; nextDMA=DESC; lento64kb=0x10000}
	subu	t1,s4,lento64kb
	blez	t1,3f
	load_s	t2,DESC
	move	s4,lento64kb
	sw	t2,r0,nextDMA
	beq	r0,r0,4f
	lui	lento64kb,1
3:
	; else lento64kb -= dmalen;
	subu	lento64kb,lento64kb,s4
4:
	subu	spaceleft,spaceleft,s4

.ifdef TRACE
	move	a1,r2ldata_p
.endif
	; Note that the LINC's data ring is constrained to end at a
	; 64kb boundary, so the 64kb check catches the ring end case.
	; if (r2ldata_p += dmalen) == r2ldata_e) {
	;	r2ldata_p=r2ldata_s;
	;	lento64kb = 0x10000 - (r2ldata_p & 0xffff)
	lw	t0,r0,r2ldata_e
	addu	r2ldata_p,r2ldata_p,s4
	bne	r2ldata_p,t0,5f
	noop
	lw	r2ldata_p,r0,r2ldata_s
	lui	lento64kb,1
	andi	t0,r2ldata_p,0xffff
	subu	lento64kb,lento64kb,t0
5:

.ifdef USE_DA
	sw	s4,s2,DAD_DMALEN
.else
	sw	s4,regBase,DW_LengthReg
.endif

.ifdef TRACE
	; trace (TOC_LDATA, a1, dataRef, dmalen);
	move	a2,dataRef
	move	a3,s4
	jal	trace
	lui	a0,TOC_LDATA
.endif

.ifdef USE_DA
	addi	t0,s2,DAD_SIZE
	sw	t0,regBase,DWHi_ProdReg	    ; Descr is now queued
.else
	load_l	t0,0x20016
	sw	t0,regBase,DW_StateReg	    ; Kick off DMA
.endif

	; ldtab_p->dlen += dmalen; dataRef += dmalen;
	lw	t0,ldtab_p,DLEN
	addu	dataRef,dataRef,s4
	addu	t0,t0,s4
	sw	t0,ldtab_p,DLEN

; if (rxdone && (dmalen == validlen)) {
	beq	s3,r0,1f	; s3=rxdone
	noop
	bne	s1,s4,1f	; s1=validlen, s4=dmalen
fill_desc:
	; nextDMA = DESC
	load_s	t0,DESC
	sw	t0,r0,nextDMA

	; ldtab_p->flags |= (EOP | (rrdp->status & 0xffff));
	lw	t2,ldtab_p,FLAGS
	andi	t0,loword,0xffff
	lui	t1,FLAG_EOP
	or	t0,t0,t1
	or	t0,t0,t2
	sw	t0,ldtab_p,FLAGS

	; begPkt = 1; // next chunk of data will be beginning of packet
	load_s	t2,1
	sw	t2,r0,begPkt

	; if packet'slast word was odd, adjust to longword bdry
	; if (dataRef & 4) dataRef += 4;
	andi	t0,dataRef,4
	beq	t0,r0,2f
	noop
	addi	dataRef,dataRef,4
2:

	; All done with this hardware descriptor.
	; RCV_DescConsReg += 8, RCV_DescRefReg += 8
	addi	s0,s0,8
	sw	s0,regBase,RCV_DescConsReg
	sw	s0,regBase,RCV_DescRefReg
; } // if (rxdone && (dmalen == validlen))
1:

	; if (dataRef >= RX_BUF_END) dataRef -= RX_BUF_SIZE;
	load_l	t0,RX_BUF_END
	subu	t1,dataRef,t0
	bltz	t1,3f
	load_l	t0,RX_BUF_SIZE
	subu	dataRef,dataRef,t0
3:

.ifdef USE_DA
	lw	t0,r0,nextDMA
	load_s	t1,DESC
	bne	t0,t1,4f
	sw	dataRef,s2,DAD_FW1
	jal	send_desc
	noop
4:
.else

	; evtMask |= EVT_WDMADONE, evtMask &= ~(EVT_RXDONE | EVT_RXSTARTED)
	lui	t0,(EVT_WDMADONE >> 16)
	or	evtMask,evtMask,t0
	load_s	t0,(EVT_RXDONE | EVT_RXSTARTED)
	nor	t0,t0,t0
	and	evtMask,evtMask,t0
.endif

	; if (spaceleft < 0x20000) update_linc_cons();
	lui	t1,2
	subu	t1,spaceleft,t1
	bgez	t1,5f
	noop
	j	update_linc_cons	; update linc consumers and return
	load_s	ra,main_loop		; to main_loop from there.
5:
	j	main_loop
	noop

; ---------------- end of eh_receive ------------------------
.eject
; ------------------- Routines --------------------------------
; update_linc_cons()
;
; Program Read DMA channel to get the LINC consumer pointers.
; These are loaded into the DMA target area. Event handler for 
; Read DMA done will check downloaded values against actual
; variables.
.ifdef USE_DA

update_linc_cons:	; DMA Assist version

	; DMA_Assist_Desc * dap    // in register t3
	; if ((dap = DRHi_ProdReg) != DRHi_RefReg) return // already in prog.

	lw	t3,regBase,DRHi_ProdReg
	lw	t0,regBase,DRHi_RefReg
	noop
	beq	t0,t3,1f
	noop
	jr	ra
	noop
1:
	lw	t0,r0,lcons_p
	load_s	t1,lcons_desc
	sw	t0,t3,DAD_HADDR		    ; dap->haddr = lcons_p
	sw	t1,t3,DAD_LADDR		    ; dap->laddr = &lcons_desc
	load_s	t2,8
	sw	t2,t3,DAD_DMALEN	    ; dap->dmalen = 8

	; simple increment, hardware takes care of descr ring wrap
	addi	t3,t3,DAD_SIZE
	jr	ra
	sw	t3,regBase,DRHi_ProdReg

.else	

update_linc_cons:   ; direct DMA version

	; if (DR_StateReg & DMA_ACTIVE_BIT) return;
	lw	t0,regBase,DR_StateReg
	noop
	andi	t0,t0,4		    ; DMA active bit
	beq	t0,r0,1f
	noop
	jr	ra
1:

	lw	t0,r0,lcons_p
	load_s	t1,lcons_desc
	sw	t0,regBase,DR_HAddrLowReg   ; DR_HAddrLowReg = lcons_p
	sw	t1,regBase,DR_LAddrReg	    ; DR_LAddrReg = &lcons_desc
	load_s	t2,8
	sw	t2,regBase,DR_LengthReg	    ; DR_LengthReg = 8

	load_s	t0,0x16
	sw	t0,regBase,DR_StateReg	    ; DR_StateReg = 0x16

	; evtMask |= EVT_RDMADONE
	lui	t0, (EVT_RDMADONE >> 16)
	jr	ra
	or	evtMask,evtMask,t0
.endif
; end update_linc_cons ---------------------------------------------
.eject
;-------------------------------------------------------------------
; 
; send_desc()
; 
; This routine is called to send the (filled-out) descriptor addressed
; by ldtab_p to the LINC. If unsuccessful, rrState will be set to some
; non-null (non-NORMAL) state.
;
; Reg usage:
;	s0 = r2ldesc producer   (next one we write)
;	s1 = r2ldesc consumer	(where LINC is in reading)


send_desc:
.ifdef TRACE
	addi	sp,sp,-4
	sw	ra,sp,0
	jal	trace
	lui	a0,TOC_SENDDESC
	lw	ra,sp,0
	addi	sp,sp,4
.endif

	addi	sp,sp,-8
	sw	s0,sp,0
	sw	s1,sp,4

; Is there room in the LINC descriptor ring?
; if ((r2ldesc_p - r2ldesc_c) == ring size - 12) { // adjust for wrap of course
	lw	s1,r0,r2ldesc_c
	lw	s0,r0,r2ldesc_p
	lw	t0,r0,r2ldesc_size
	; t1 = (prod - cons); if (t1 < 0) t1 += r2ldesc_size
	subu	t1,s0,s1
	bgez	t1,2f
	noop
	addu	t1,t1,t0
2:
	addi	t1,t1,LDESC_ENTRY_SIZE
	bne	t1,t0,1f

	; rrState = LDESC_WAIT_STATE; ++s_lincdesc_waits
	lw	t0,r0,s_lincdesc_waits	    ; caution: delay slot
	load_s	rrState,LDESC_WAIT_STATE
	addi	t0,t0,1

.ifdef USE_DA
.ifdef DTIMEOUT
	; evtMask &= ~(EVT_TIMER | EVT_RXSTARTED | EVT_RXDONE); 
	load_l	t1,(EVT_TIMER | EVT_RXSTARTED | EVT_RXDONE)
	nor	t1,t1,t1
	and	evtMask,evtMask,t1
.else
	; evtMask &= ~(EVT_RXSTARTED | EVT_RXDONE); 
	load_s	t1,(EVT_RXSTARTED | EVT_RXDONE)
	nor	t1,t1,t1
	and	evtMask,evtMask,t1
.endif
.else	; not USE_DA
.ifdef DTIMEOUT
	; evtMask &= ~EVT_TIMER; 
	lui	t1,(EVT_TIMER >> 16)
	nor	t1,t1,t1
	and	evtMask,evtMask,t1
.endif
.endif	; not USE_DA
	; update_linc_cons(); return; // Don't change ra - has return addr
	lw	s0,sp,0
	lw	s1,sp,4
	addi	sp,sp,8
	j	update_linc_cons
	sw	t0,r0,s_lincdesc_waits
; } // if ((r2ldesc_p - r2ldesc_c) == ring size - 12) 
1:

.ifdef USE_DA
	; DMA_Assist_Desc * dap // in reg t4
	; dap = DWHi_ProdReg
	lw	t4,regBase,DWHi_ProdReg

; if (dap + 1 == DWHi_RefReg) { // ring full
	lw	t0,regBase,DWHi_RefReg
	; 0x200 bytes in the ring, 0x20 bytes per entry.
	load_s	t1,0x1e0
	subu	t2,t4,t0
	andi	t2,t2,0x1ff
	bne	t2,t1,2f

	; s_dadesc_waits++, rrState = DADESC_WAIT_STATE
	lw	t2,r0,s_dadesc_waits
	load_s	rrState, DADESC_WAIT_STATE
	addi	t2,t2,1
	sw	t2,r0,s_dadesc_waits
.ifdef TRACE
	addi	sp,sp,-4
	sw	ra,sp,0
	move	a1,t0		; DWHi_ConsReg
	move	a2,t4		; DWHi_ProdReg
	lw	a3,regBase,DWHi_RefReg
	jal	trace
	lui	a0,TOC_DAWAIT_DESC
	lw	ra,sp,0
	addi	sp,sp,4
.endif
		
.ifdef DTIMEOUT
	; evtMask &= ~(EVT_RXSTARTED | EVT_RXDONE | EVT_TIMER);
	load_l	t0,(EVT_RXSTARTED | EVT_RXDONE | EVT_TIMER)
.else
	; evtMask &= ~(EVT_RXSTARTED | EVT_RXDONE);
	load_s	t0,(EVT_RXSTARTED | EVT_RXDONE)
.endif
	nor	t0,t0,t0
	and	evtMask,evtMask,t0
	lw	s0,sp,0
	lw	s1,sp,4
	jr	ra
	addi	sp,sp,8

; } // if ring full
2:
	sw	s0,t4,DAD_HADDR		    ; dap->haddr = r2ldesc_p
	sw	ldtab_p,t4,DAD_LADDR	    ; dap->laddr = ldtab_p
	load_s	t2,LDESC_ENTRY_SIZE
	sw	t2,t4,DAD_DMALEN	    ; dap->dmalen = 32
	sw	dataRef,t4,DAD_FW1	    ; dap->fw1 = dataRef
	addi	t4,t4,DAD_SIZE
	sw	t4,regBase,DWHi_ProdReg	    ; DMA is now queued
.else	; not USE_DA

	; Kick off DMA
	sw	s0,regBase,DW_HAddrLowReg
	sw	ldtab_p,regBase,DW_LAddrReg
	load_s	t2,LDESC_ENTRY_SIZE
	sw	t2,regBase,DW_LengthReg
	load_l	t2,0x20016		    ; disable producer compare
	sw	t2,regBase,DW_StateReg

	lui	t0,(EVT_WDMADONE >> 16)
	or	evtMask,evtMask,t0	    ; evtMask |= EVT_WDMADONE
.endif

.ifdef TRACE
	; trace (TOC_LDESC, ldtab_p->haddr, ldtab_p->dlen, ldtab_p->flags)
	addi	sp,sp,-4
	sw	ra,sp,0
	lw	a1,ldtab_p,HADDR
	lw	a2,ldtab_p,DLEN
	lw	a3,ldtab_p,FLAGS
	jal	trace
	lui	a0,TOC_LDESC

	lw	ra,sp,0
	addi	sp,sp,4
.endif

	load_s	t0,DATA
	sw	t0,r0,nextDMA		    ; nextDMA = DATA, 

	; if (++r2ldesc_p == r2ldesc_e)	r2ldesc_p = r2ldesc_s;
	lw	t1,r0,r2ldesc_e
	addi	s0,s0,LDESC_ENTRY_SIZE
	bne	s0,t1,3f
	noop
	lw	s0,r0,r2ldesc_s
	noop
3:
	sw	s0,r0,r2ldesc_p


	; if (++ldtab_p == ldtab_e) ldtab_p = ldtab_s
	addi	ldtab_p,ldtab_p,LDESC_ENTRY_SIZE
	load_s	t0,ldtab_e
	bne	t0,ldtab_p,4f
	noop
	load_s	ldtab_p,ldtab_s
4:

	; Init next descriptor
	; ldtab_p->haddr = r2ldata_p & 0x03ffffff;
	load_l	t0,0x03ffffff
	and	t0,t0,r2ldata_p
	sw	t0,ldtab_p,HADDR
	; ldtab_p->dlen = 0; ldtab_p->flags = nextvb
	lw	t1,r0,nextvb
	sw	r0,ldtab_p,DLEN
	sw	t1,ldtab_p,FLAGS

	; nextvb ^= 0x80000000;
	lui	t0,FLAG_VB
	xor	t1,t1,t0
	sw	t1,r0,nextvb
.ifdef TEST_PLD
	lui	t0,0x8000	;  address of "BYPASS register"
	;; make sure bits BS1,BS2 are clear before writing
5:	
	lw	t1,t0,0		;  while (*bypass & 0x60000000) ;
	lui	t2,0x6000
	and	t1,t1,t2
	bne	t1,r0,5b
	lui	t3,0x100
	sw	t3,t0,0		; *bypass = 0x01000000
.endif	
	lw	s0,sp,0
	lw	s1,sp,4
	jr	ra
	addi	sp,sp,8

; ----------------- end of routine send_desc() ---------------------
