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
; SOURCE-side Roadrunner firmware for the Lego HIPPI-Serial card.
;
; $Revision: 1.27 $	$Date: 1998/03/05 03:46:04 $
;
.ifdef HISTORY
*
* $Log: rr_src.s,v $
* Revision 1.27  1998/03/05 03:46:04  avr
* This changes the check for TX descriptor producer wrap to make sure the
* producer is never set equal to the reference because it creates a race
* condition where we could lose a TX done event and stop forward progress.
* I also added some commented out code that allows the trace function to
* be called anywhere in the code when we are debugging (needed to save the
* t regs it uses).
*
* Revision 1.26  1997/12/23 01:23:53  avr
* This changes how the flow through data mode works when data buffer is near
* being full. Now the xmit data producer pointer is not incremented when a
* read high dma that is part of a break up of a single linc xmit descriptor
* completes, unless all of the linc xmit data can fit in the data buffer.
*
* Revision 1.25  1997/10/01 08:19:34  ddh
* conditional was checking when dma assist producer caught up with
* the consumer, needed to compare rather against the reference for
* ring wrap.
*
* Revision 1.24  1997/06/19 01:43:35  ddh
* When a dummy descriptor is encountered, is_dd must wait for all
* prior xmit descriptors to complete prior to processing the dd
* since the (a) rr hw bug requires us to be in perm conn during the
* dd.  Meanwhile, other events must continued to be handled or we'll
* be in a deadlock situation.  Hence the code now runs through the
* main loop and processes the dd over and over until it's finally
* really ready to be handled.
*
# Revision 1.23  1997/06/12  00:55:36  ddh
# Fix to is_dd to cause our compare against the XMT_Desc to wrap to start
# of descriptor ring.  Apparently similar problem in do_dma though it never
# seems to have been encountered for some reason.
#
# Revision 1.19  1997/03/04  02:43:03  ddh
# fix bug in rr xmit q flushing code wherein the linc was not being
# told that it could reuse its descriptors resulting ultimately in
# deadlock on errors.
#
# Revision 1.16  1996/12/18  01:46:14  irene
# Fix for problem with losing DMA Asst completion notifications.
# Check for producer catching up with Reference rather than with Consumer.
#
# Revision 1.15  1996/12/16  23:25:33  irene
# Added code to txattn event handler to reset interface even if tx queue is
# empty.
#
# Revision 1.14  1996/11/28  00:38:17  irene
# Fixed bug in mbox event handler based on misunderstanding of the spec.
#
# Revision 1.13  1996/10/08  03:13:41  jimp
# Changed DMA_HEADSTART back to 64 words
#
# Revision 1.12  1996/09/23  23:57:42  irene
# Increased DMA_HEADSTART back up to 1024 again.
#
# Revision 1.11  1996/09/21  03:28:52  irene
# Reduce DMA_HEADSTART back to 64.
#
# Revision 1.10  1996/09/20  03:45:25  irene
# ~Increased counter waiting for HEADSTART. Also changed RD-DMA-done handler
# not to assume that we were successful waiting for HEADSTART & update the TxDataProd
# register.
#
# Revision 1.8  1996/09/07  00:20:19  irene
# support for mbox31 you-alive? queries.
#
# Revision 1.7  1996/09/04  02:37:39  irene
# Swapped address and flagsnlen fields in tx descriptor so that validity bit
# is in the first word fetched.
#
# Revision 1.4  1996/08/02  20:27:45  irene
# Added alternate simulation addresses for LINC environment.
#
# Revision 1.3  1996/07/02  02:37:09  irene
# Moved Glink/INIT status mbox from mbox2 to mbox31.
# Changed all addiu's to addi's because addiu is going away in Rev B
# roadrunner chip.
# Added eh_rxattn for monitoring and reporting Glink status changes.
#
# Revision 1.2  1996/06/27  00:47:40  irene
# Moved status-reporting from mbox0 to mbox2.
#
# Revision 1.0  1996/05/02  23:02:04  irene
# First pass.
#
*
*
*
.endif
;
;===========================================================================
;; Firmware for source roadrunner on Lego HIPPI-serial card.
;;

; XXX TBD: add Rx attn to default evt mask, and Rx attn handler
; to determine link state, write appropriate code to mbox0 and
; send an interrupt to 4640. Subject to Jim Pinkerton agreement.
;

; some compilation defines:
;   SRC_RR - defined for Source RR, distinguishes src & dst diffs
;	     in the common include file.
;   TRACE - compile to include tracing code.
;   SIM   - for the Indigo2 simulation bringup.
;   RIO_GRANDE - stuff specific to Rio Grande cards, not the
;		 real lego HIPPI-serial h/w.
;   REV_A - workarounds for Rev. A RR chip errata.

SRC_RR = 1
TRACE = 1
SIM = 1
RIO_GRANDE = 1
HALF_DUPLEX = 1
.include "rr.h"

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
mbox1:	.word	0x500000    ; contains src timeout (written by 4640)
.block	108
mbox_retries:	.word	0   ; count of source pkts retried on error
mbox30:		.word	0   ; panic code written by RR
mbox_status:	.word	0   ; init status to be written by RR
.else
.equ	mbox0, 0
.equ	mbox1, 4	    ; contains src timeout (written by 4640)
.equ	mbox_retries, 116
.equ	mbox30, 120
.equ	mbox_status, 124
.endif
;; Next 128 bytes of GCA is init info from LINC's 4640 containing 
;; CPCI-bus-addressed ptrs to various things in its SDRAM.
.ifdef SIM
.ifdef G2P			;  GIO2PCI on Indigo2
l2rtabp:    .word   0x08f00800	; linc2rr tx desc table
r2ltabp:    .word   0x08f00000	; rr2linc event table
r2ltabendp: .word   0x08f007fc	; end of rr2linc event table
.else				;  LINC SDRAM, continuous prefetch
l2rtabp:    .word   0x98000800	; linc2rr tx desc table
r2ltabp:    .word   0x80000000	; rr2linc event table
r2ltabendp: .word   0x800007fc	; end of rr2linc event table
.endif
.else
.equ	l2rtabp, 128	; linc2rr tx desc table
.equ	r2ltabp, 132	; rr2linc event table
.equ	r2ltabendp, 136	; end of rr2linc event table
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

; LINC's xmit descr structure:
LXD_FLAGSNLEN	= 0	; 15 msbs are flags, 17 lsb is length
LXD_DATAP	= 4	; 1st word is CPCI-bus addr of data in LINC SDRAM
LXD_IFIELD	= 8	; 3rd word is I-field
LXDSIZE		= 16	; 4th word is pad, so lxdtab entry won't straddle
			; 4640 cacheline (32bytes).

LXD_LENMASK	= 0x1FFFF; mask for retrieving len

; defines for linc xmit descr flags. These are defined for masking
; with the lxd_flagsnlen (32 bit) field after it has been shifted
; right 16 bits.
LXDF_MB	     = 0x8000	; "More" Bit => packet continued in next descr
LXDF_DD	     = 0x4000	; "Dummy Descr", for disconnects on PermConns
LXDF_WR	     = 0x2000	; must be zero, but we don't check
LXDF_VB	     = 0x1000	; "Validity Bit", this toggles
LXDF_SI	     = 0x0800	; "Same Ifield" as last descriptor
LXDF_PC	     = 0x0400	; "Perm Conn" - turn on "Permanent" mode
LXDF_WN	     = 0x0200	; "Wrap Next",	end of ring, wrap for next desc
LXDF_CC	     = 0x0100	; "Continue Conn", aid for flushing PC descs in
			; case of error
LXDF_RETRIES = 0x001e	; Retried on error how many times?

RRSRC_RETRY_MAX = 15	; must fit in RETRIES field

NUM_LXD	= 8	; 8 entries in local table
LXDTABSIZE  = LXDSIZE * NUM_LXD

lxdtab:	.block	LXDTABSIZE
lxdtab_end:

;; ------------ Some other global vars -----------------
next_txdvbit:	.word 0	; next validity bit for xmit desc ring
linc_evrp:	.word 0	; next event entry to use in LINC SDRAM
perm_conn:	.word 0 ; counter for toggling PERM CONN mode
saved_haddr:	.word 0 ; host dma address for to-be-cont'd processing
saved_dlen:	.word 0 ; dma length for to-be-cont'd processing
txdata_cons:	.word 0 ; data buffer consumer (because XMT_ConsReg is
			; too quirky to depend on)
;; Event queue waiting to DMA to LINC. Must be even number of entries
;; aligned on 8-byte boundary, because we use bit 2 of each entry's
;; address to determine the toggling validity bit. The valid entries
;; in the ring are delimited by a consumer and producer pointer

evtab_prod:	.word 0 ; next event entry to use in local queue
evtab_cons:	.word 0 ; next event to start DMA at.
.align 8
evtab:		.block 512 
evtab_end:

tx_errcodes:	.word	(EOC_CONN_TIMEO << 16)
		.word	(EOC_CONN_REJ << 16)
		.word	(EOC_DST_DISCON << 16)
		.word	(EOC_SRC_PERR << 16)

;; ----------- Some local stats ------------------------
s_timeout:	.word	0
s_dmard_attn:	.word	0
s_dmawr_attn:	.word	0
s_tx_attn:	.word	0
s_desc_full:	.word	0
s_dbuf_full:	.word	0
s_rdhi_full:	.word	0
s_hung_dma:	.word	0
s_falsealarm:	.word	0
s_falseattn:	.word	0
;; ------------------- DMA assist rings -------------------------
;; D.A. rings are 16 descr per ring. Hardware requires DMA assist 
;; rings to be contiguous 512 byte blocks, starting on 2048 alignment,
;; and in the following order:

.align 4*DAD_RINGSIZE
DA_base:
DARHring:	.block 512  ; read hipri
DAWHring:	.block 512  ; write hipri
DARLring:	.block 512  ; read lopri
DAWLring:	.block 512  ; write lopri

DMA_HEADSTART = 64	    ; 256 words
MAX_DMA = 0xfffc	    ; max amt handled per DMA

;; ------------------------ Trace Buffer
; Each trace entry is 4 words:
;	word 0:	    opcode (1 byte), timestamp (3 bytes)
;	words 1,2,3:	trace args, depending on opcode
; trace_ix is index of next entry to write trace to.

.align	TRACE_BUF_SIZE
trace_buf:	.block TRACE_BUF_SIZE
trace_end:

;; ------------------------ Txd shadow table
; Parallels the hw tx descr table, one-for-one, used for stashing add'l
; info applicable to that hw tx descr. Each entry is 2 words, same
; size as the tx desc entry, so we can use the same indexing.
;   word 0: address of relevant tx descriptor in LINC SDRAM, so
;	    we can send appropriate acks to the 4650.
;   word 1: end of this descr's data+pad in databuf ring, used
;	    for updating txdata_cons when the tx completes.
;
.align TX_DESC_RINGSIZE
txd_shadow_tab:	    .block TX_DESC_RINGSIZE

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
;
;  To add traces in code that uses t regs uncomment marked code. I am
;  subtracting 80 becuase if you do the reasonable thing it breaks and
;  when you use 80 it works. I didn't take the time to track down why.
;     -avr	
trace:
; uncomment to save t regs
;	addi	sp,sp,-80
;	sw	t0,sp,4
;	sw	t1,sp,8
;	sw	t2,sp,12
	
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

; uncomment to save t regs
;	sw	t0,r0,trace_ix
;	lw	t2,sp,12
;	lw	t1,sp,8
;	lw	t0,sp,4
;	addi	sp,sp,80
	jr	ra
	sw	t0,r0,trace_ix
; uncomment this and comment out above line if saving t regs
;	noop
.endif	;TRACE

start:
;; Initialize everything in sight. Don't assume vars contain
;; zeros as we may have been reset and memory is in unknown state.
init:
	; clear init-status field, will post result when done
	sw	r0,r0,mbox_status	    ; mbox_status = 0;

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
	sw	r0,regBase,HX_StateReg	; will enable after initializing descrs

	; HR_StateReg = 0xff800002 (clear all error bits, 
	;			    force to passive reset - NO ENABLE)
	load_l	t0,0xff800002
	sw	t0,regBase,HR_StateReg

.ifdef RIO_GRANDE
.ifdef HALF_DUPLEX
	; initializing is done by the host utility "glinksync"
.else
	; Init G-links using Ext. Serial Reg.
	;   bit0:   0=reset, 1=normal
	;   bit1:   0=loopback, 1=normal
	;   bit2:   0=turn on OH1 framing? I suspect so because
	;	    when I turn this bit on I see the HIPPI Overhead
	;	    registers on both sides turn on the OH8-sync'ed bit
	; Init sequence consists of writing 6,7,3 to register, with
	; count 100 in between each write.
	;
	load_s	t0,6	; reset G-link
	sw	t0,regBase,ExtSerDataReg
	load_s	t1,100
1:
	bne	t1,r0,1b
	addi	t1,t1,-1

	load_s	t0,7	; enable G-link
	sw	t0,regBase,ExtSerDataReg
	load_s	t1,100
1:
	bne	t1,r0,1b
	addi	t1,t1,-1

	load_s	t0,3	; turn on OH8 framing clock?
	sw	t0,regBase,ExtSerDataReg
	load_s	t1,100
1:
	bne	t1,r0,1b
	addi	t1,t1,-1
.endif
.endif

	;; --------------  Init misc local vars
	sw	r0,r0,perm_conn	    ; perm_conn = 0;
	sw	r0,r0,saved_dlen    ; saved_dlen = 0

	;; --------------  Init event ring variables
	lw	t1,r0,r2ltabp
	load_s	t2,evtab
	sw	t1,r0,linc_evrp	    ;linc_evrp = init_info.r2ltabp
	sw	t2,r0,evtab_prod    ;evtab_prod = &evtab[]
	sw	t2,r0,evtab_cons    ;evtab_cons = &evtab[]

	;; --------------- Init tx descr vars
	lw	lincXDp,r0,l2rtabp  ;lincXDp = init_info.l2rtabp
	load_s	t0,LXDF_VB
	sw	t0,r0,next_txdvbit  ;next_txvbit = LXDF_VB

	;; --------------- Init general control registers
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
	load_s	t0,0x0702
.else
	;; load_s	t0,0x2702
	load_s	t0,0xc602
.endif
	sw	t0,regBase,MiscLocalReg	

	; Leave SW3 bit turned on all the time in MainEvtReg (RR h/w reg)
	; and just toggle it in the EventMask reg (general CPU register)
	load_s	t1,EVT_SW3
	sw	t1,regBase,MainEvtReg	    ; MainEvtReg = EVT_SW3

	load_l	t0,0xffffffff
	sw	t0,regBase,MboxEvtReg	    ; clear MboxEvtReg
	zero_rr	TimerHiReg	    ; TimerHiReg = 0
	zero_rr	TimerLowReg	    ; TimerLowReg = 0
	lui	t0,0x6000
	sw	t0,regBase,TimerRefReg	    ; TimerRefReg = 0x60000000
	zero_rr	CPUPriReg	    ; CPUPriReg = 0

	;; --------------  Set up receive ptrs
	load_l	t0,RX_BUF_BEGIN
	sw	t0,regBase,RCV_BaseReg	    ; RCV_BaseReg = RX_BUF_BEGIN
	sw	t0,regBase,RCV_ProdReg	    ; RCV_ProdReg = RX_BUF_BEGIN
	sw	t0,regBase,RCV_ConsReg	    ; RCV_ConsReg = RX_BUF_BEGIN

	load_l	t0,RX_DESC_BEGIN
	sw	t0,regBase,RCV_DescProdReg  ; RCV_DescBaseReg = RX_DESC_BEGIN
	sw	t0,regBase,RCV_DescConsReg  ; RCV_DescConsReg = RX_DESC_BEGIN
	sw	t0,regBase,RCV_DescRefReg   ; RCV_DescRefReg = RX_DESC_BEGIN

	;; -------------- Set up transmit ptrs
	load_l	t0,TX_BUF_BEGIN
	sw	t0,regBase,XMT_BaseReg	    ; XMT_BaseReg = TX_BUF_BEGIN
	sw	t0,regBase,XMT_ConsReg	    ; XMT_ConsReg = TX_BUF_BEGIN
	sw	t0,r0,txdata_cons	    ; txdata_cons = TX_BUF_BEGIN
	addi	txdatap,t0,4	; Ifield always begins on odd word
	addi	t0,t0,8
	sw	t0,regBase,XMT_ProdReg	    ; XMT_ProdReg = TX_BUF_BEGIN
	; XMT_ProdReg must be to where data will start, else h/w
	; thinks it's ring wrap and starts transmission immediately.

	load_l	t0,TX_DESC_BEGIN
	sw	t0,regBase,XMT_DescProdReg  ; XMT_DescBaseReg = TX_DESC_BEGIN
	sw	t0,regBase,XMT_DescConsReg  ; XMT_DescConsReg = TX_DESC_BEGIN
	sw	t0,regBase,XMT_DescRefReg   ; XMT_DescRefReg = TX_DESC_BEGIN

	;; -------------- HIPPI State and Overhead regs
	; H_OvrheadReg = 0x00020000 
	; XXX temp for RR bug - see rev A errata, disable OH1 transmission
	lui	t0,0x0002
	sw	t0,regBase,H_OvrheadReg

	; Receive side has to be taken out of passive reset for
	; correct OH8 synced, LinkReady and FlagSyncd bits to show up.
	load_l	t0,0xff800000	; clear all error bits
	sw	t0,regBase,HR_StateReg

	; ------ Check to make sure OH8 is synchronized (bit 0 == 1)
	; Loop forever until it comes on. Write mbox_status every 100 reads
	; in the meantime.
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
	; ------------- OH8 sync OK.

	;; ------------- HIPPI Tx & Rx State Registers
	; Now make sure that LNKRDY and FLGSYNCD bits are on in Rx State Reg
	; write mbox_status with bad status every 100 reads, until we get link
	; up.
	load_s	t1,100
	load_l	t2,RX_STATE_LNKRDY|RX_STATE_FLGSYNCD
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
	bne	t0,t2,1b
	addi	t1,t1,-1

	; Clear the error bits in the Rcv State Reg
.ifdef HALF_DUPLEX
	load_l	t0,0xff800000	    ; no enable-conns
	; HX_StateReg = 0x01000011  (half-duplex transmit, enable xmit)
	load_l	t1,0x01000011
.else
	; enable both transmit and receive
	load_l	t0,0xff800001
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

	; Assist Rings all initialized, now enable Assist
	load_s	t0,DEFAULT_ASST_STATE
	sw	t0,regBase,AssistStateReg

	; Initialize evtMask register to  default EVTMASK
	lui	evtMask,(DEF_EVTMASK_HI)
	ori	evtMask,evtMask,(DEF_EVTMASK_LO)

.ifdef TRACE
	; TRACE (TOC_INIT)
	jal	trace
	lui	a0,TOC_INIT
.endif
	;; Finished init, tell 4640 by writing "1" in mbox_status
	load_s	t0,INIT_SUCCESS
	sw	t0,r0,mbox_status
	;; Fall through to get_tx_cmds

;; get_tx_cmds:
; 
;  queue a LoPriRd DMA Assist request to fetch another block
;  of 8 descriptors from LINC SDRAM to local SRAM. Target LINC
;  address to fetch from is contained in register lincXDp
get_tx_cmds:
.ifdef NOTNEEDED
	; TRACE (TOC_FETCH, lincXDp)
	move	a1,lincXDp
	lw	a2,regBase,DRLo_ProdReg
	jal	trace
	lui	a0,TOC_FETCH
.endif
	; Fill out the next descriptor
	lw	t0,regBase,DRLo_ProdReg	    ; t0=DRLoProd
	load_s	loword,lxdtab
	sdw	lincXDp,t0,0	; DRLoProd->{haddr,laddr}={lincXDp,lxdtab}
	load_s	t1,LXDTABSIZE
	sw	t1,t0,DAD_DMALEN    ; DRLoProd->dmalen = LXDTABSIZE
	load_s	localXDp,lxdtab	    ; localXDp = lxdtab

	; Advance Producer Reg to kick off DMA
	; don't have to check for ring wrap as high order bits in 
	; register are not writeable
	addi	t0,t0,DAD_SIZE
	sw	t0,regBase,DRLo_ProdReg

	; rrState = CMD_WAIT_STATE
	load_s	rrState,CMD_WAIT_STATE

	;; Fall through to main loop

;; XXX - does main_loop have to be cache-line aligned?
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
	j	eh_rxdone	; event bit 5
	noop
	j	eh_rxstarted	; event bit 6
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
eh_sw2:
eh_rxdone:
eh_rxstarted:
eh_sw4:

eh_sw5:
eh_sw6:
eh_sw7:
eh_DAwhdone:
eh_sw8:

eh_wrDMAdone:
eh_rdDMAdone:
eh_wrDMAattn:
eh_rdDMAattn:
eh_sw9:
eh_sw10:
eh_extserial:

eh_sw11:
eh_sw12:
eh_sw13:
eh_sw14:
eh_sw15:
;; Unexpected event. Panic?!?!
	sw	t2,r0,mbox30
.ifdef TRACE
	move	a1,t2	    ; the event we joff-ed on.
	jal	trace
	lui	a0,TOC_BADEVT
.endif
	; These noops let the trace_ix reach memory before the halt.
	noop
	noop
	noop
	noop
	noop
	noop
	halt
	noop
.eject
;; ===================================================================
;; eh_mbox
;; Event handler for mbox msg. This is the 4640 querying to see
;; if we are alive. It writes a 0 in mbox31. We respond by
;; rewriting a 1 or 2 there, depending on whether the Glink is
;; up or down.
eh_mbox:
	load_l	t0,0xffffffff
	sw	t0,regBase,MboxEvtReg   ; clear mbox event

	;; read HIPPI Recv State Reg. If both RX_STATE_LNKRDY
	;; and RX_STATE_FLGSYNCD bits are on, write INIT_SUCESS
	;; to the status mbox, otherwise write GLINK_NOT_RDY.
	lw	t0,regBase,HR_StateReg
	load_l	t1,RX_STATE_LNKRDY|RX_STATE_FLGSYNCD
	and	t2,t1,t0
	beq	t2,t1,1f
	load_s	t3,INIT_SUCCESS
	load_s	t3,GLINK_NOT_RDY
1:	
	j	main_loop		; return to main loop
	sw	t3,r0,mbox_status	; report init status

.eject
; ====================================================================
;; eh_DArldone - Event Handler for DMA Assist Read Low Done.
;
;  Implication is that we've fetched another 8 Tx descs from LINC
;  for processing.
;
eh_DArldone:
.ifdef NOTNEEDED
	; TRACE (TOC_RDLODONE, lincXDp, DRLo_RefReg)
	move	a1,lincXDp
	lw	a2,regBase,DRLo_RefReg
	jal	trace
	lui	a0,TOC_RDLODONE
.endif
	; move Ref to match Consumer to turn off event
	lw	t0,regBase,DRLo_ConsReg
	lui	t1,(FLUSH_STATE >> 16)

	; if (rrState & FLUSH_STATE) turn off FLUSH bit, goto flush_queue, 
	; (and return to main loop from there)
	and	t2,t1,rrState
	beq	t2,r0,eh_sw3
	sw	t0,regBase,DRLo_RefReg  ;DRLo_RefReg = DRLo_ConsReg

	j	flush_queue
	load_s	rrState,NORMAL_STATE
	; else fall through to sw3 event handler
eh_sw3:
	; Register use:
	; s0 = tx descr prod reg
	; s1 = linc xd flags (upper 16 bits of flagsnlen, 
	;		      loaded into low 16 bits of s1)
	; s2 = ptr to next RdHi D.A. descr to use.
	; s3 = length of data in packet to DMA. (This gets decremented
	;      as we set up sections of DMA.)
	addi	t2,r0,~EVT_SW3			; t2 = ~EVT_SW3
	lw	t0,r0,next_txdvbit		; t0 = next_txdvbit
	lhu	t1,localXDp,LXD_FLAGSNLEN	; t1 = localXDp->flags
	and	evtMask,evtMask,t2	; evtMask &= ~EVT_SW3

	; if ((localXDp->flags & LXDF_VB) != next_txdvbit) 
	;	goto get_tx_cmds    // and to main loop from there
	andi	t3,t1,LXDF_VB   
	bne	t3,t0,get_tx_cmds

	; flip the validity bit
	xori	t0,t0,LXDF_VB
	sw	t0,r0,next_txdvbit

.ifdef TRACE
	; TRACE (TOC_SW3, lincXDp, localXDp)
	move	a1,lincXDp
	move	a2,localXDp
	jal	trace
	lui	a0,TOC_SW3
.endif

	; Check descr ring state. If prod + 1 = ref we are in danger
	; of wrapping the ring so we won't receive tx dones, so if 
	; ref != cons we can free up some space by ack'ing some
	; descriptors.
	lw	s0,regBase,XMT_DescProdReg	; s0 = txdProducer
	lw	t0,regBase,XMT_DescRefReg	; t0 = txdReference
	lw	t1,regBase,XMT_DescConsReg      ; t1 = txdConsumer
	
	addi	t2,s0,TX_DESC_SIZE	; t2 = txdProd + 1
	andi	t2,t2,TX_DESC_INDEX	; t2 = index(txdProd + 1)
	andi	t0,t0,TX_DESC_INDEX	; t0 = index(txdRef)

	; if ((index(txdProd + 1) == index(txdRef)) &&
	;     (index(txdRef) != index(txdCons)) {
	;       // we need to send acks to free up a TC descriptor
	bne	t2,t0,2f
	andi	t1,t1,TX_DESC_INDEX	; t1 = index(txdCons)
	beq	t0,t1,1f
	noop
	jal	ack_xmit_ok	; this routine adjusts the txd Ref.
	noop
	beq	r0,r0,2f
1:	
	; } else {
	;  // we are flow controlled on TX descriptors and event
	;  // handlers for tx done/attn will come back to desc_wait_retry
	lw	t3,r0,s_desc_full
	load_s	rrState,DESC_WAIT_STATE	; rrState = DESC_WAIT_STATE
	addi	t3,t3,1
	j	main_loop
	sw	t3,r0,s_desc_full	; ++s_descfuls
2:      ; }

desc_wait_retry:
	; This is an entry point from tx done and attn event handlers
	; as well as fall-through from above eh_sw3 code.

	lw	s0,regBase,XMT_DescProdReg	; s0 = txdProducer
	lw	loword,localXDp,LXD_FLAGSNLEN	; loword = localXDp->flags,len
	lhu	s1,localXDp,LXD_FLAGSNLEN	; s1 = localXDp->flags

	andi	t0,s0,TX_DESC_INDEX
	sw	lincXDp,t0,txd_shadow_tab

	move	t2,txdatap			; t2 = txdatap
	andi	t3,s1,LXDF_SI			; t3 = flags & LXDF_SI
	; if (localXDp->flags & LXDF_SI)
	;	set SI bit in hw's tx desc
	beq	t3,r0,3f	; if (flags & LXDF_SI)
	lui	t4,0x8000	; 
	or	t2,t2,t4	;	t2 |= 0x80000000
3:
	sdw	t2,s0,0		; txdProd->si,addr = si,txdatap
				; txdProd->flags,len = localXDp->flags,len

	; Check for dummy descriptor
	andi	t0,s1,LXDF_DD	; t0 = (localXDp->flags & LXDF_DD)
	bne	t0,r0,is_dd	; returns to advance_ptrs

	; Check RdHi DMA assist ring.
	; if (Producer + 1 == reference) 
	;       we are in danger of losing event notifications
	;       on DMA Asst completion event. So
	;	set state to RDHIDA_WAIT and go back to main
	;	loop. Event handler for RdHiDa complete (or
	;	tx attn) will come back here in that state.
	;
	lw	s2,regBase,DRHi_ProdReg	    ; s2 = RdHiDAprod
	lw	t1,regBase,DRHi_RefReg	    ; t1 = RdHiDAref
	addi	t0,s2,DAD_SIZE
	andi	t0,t0,DAD_RINGMASK  ; t0= (prod + size) & mask
	andi	t1,t1,DAD_RINGMASK  ; t1 = ref & mask
	bne	t0,t1,rdhida_wait_retry
	lw	t2,r0,s_rdhi_full
	load_s	rrState,RDHIDA_WAIT_STATE
	addi	t2,t2,1
	j	main_loop
	sw	t2,r0,s_rdhi_full

rdhida_wait_retry:
	; entry point from RdHi DA done and tx attn event handlers
	; if state is RDHIDA_WAIT_STATE
databuf_wait_retry:
	; entry point from tx done and tx attn event handlers
	; for state DATABUF_WAIT_STATE
	
	; Register use:
	;   s0 = txdp (h/w tx descr to use)
	;   s1 = flags (localXDP->flags)
	;   s2 = rdhidap (ptr to RdHiDA desc to use)
	;   s3 = spaceleft (in databuf ring)
	;   s4 = haddr (host address to DMA data from)
	;   s5 = dmalen (length of data still to DMA from host, total 
	;        length of data for descriptor, decremented as we set
	;	 up the DMA assist descriptors.)
	;   s6 = more (boolean indicating we hit wrap at end of databuf ring)
	;   t5 = thislen (length of "this" DMA)
	; spaceleft = (Tx Buf Consumer - (txdatap + 12)) & 0xfffffc00
	lw	s3,r0,txdata_cons
	lw	s2,regBase,DRHi_ProdReg	    ; s2 = Rd Hi DA desc ptr
	subu	s3,s3,txdatap		    ; s3 = txDataCons - txdatap
	bgtz	s3,1f
	lhu	s1,localXDp,LXD_FLAGSNLEN   ; either branch, s1=LincDesc flags
	load_l	t0,TX_BUF_SIZE
	addu	s3,s3,t0		    ; adjust for wrap
1:
	addi	s3,s3,-12
	lui	t0,0xffff
	ori	t0,t0,0xfc00
	and	s3,s3,t0    ; s3 = spaceleft, rounded down to KB

	bgtz	s3,2f				; if (spaceleft <= 0) {
	lw	t1,r0,s_dbuf_full
	load_s	rrState,DATABUF_WAIT_STATE	;   rrState = DATABUF_WAIT
	addi	t1,t1,1				;   ++s_dbuf_full
	j	main_loop			;   goto main_loop
	sw	t1,r0,s_dbuf_full		; }
2:

	; if saved_dlen is 0, this must be the
	; start of processing for this descriptor (we haven't
	; loaded the Ifield into the databuffer yet). Otherwise
	; it is work-in-progress and we need to retrieve the stashed
	; values of where we left off.
; if ((dmalen = saved_dlen) == 0) {
	lw	s5,r0,saved_dlen	; dmalen = saved_dlen
	lw	t1,localXDp,LXD_IFIELD
	bne	s5,r0,3f		
	load_l	t2,TX_BUF_END		;   
	lw	s0,regBase,XMT_DescProdReg  ; txdp = TxDescProdReg
	sw	t1,txdatap,0		;   *txdatap = localXDp->Ifield
	addi	txdatap,txdatap,4	;   txdatap += 4
	bne	t2,txdatap,4f		;   if (txdatap == TX_BUF_END)
	noop
	load_l	txdatap,TX_BUF_BEGIN	;	txdatap = TX_BUF_BEGIN
4:
	lw	s5,localXDp,LXD_FLAGSNLEN
	lui	t0,1
	ori	t0,t0,0xffff
	and	s5,s5,t0		;   dmalen = localXDp->len
	andi	t1,s1,LXDF_PC		;   if (flags & LXDF_PC)
	bne	t1,r0,is_pc		;	go turn on PERM_CONN bit
	; is_pc returns to do_dma, foll. delay slot instr execs either way.
	lw	s4,localXDp,LXD_DATAP	;   haddr = localXDp->datap
	j	do_dma
	noop
; } else {
3:
	lw	s0,regBase,XMT_DescProdReg 
	lw	s4,r0,saved_haddr	; haddr = saved_haddr
	addi	s0,s0,-TX_DESC_SIZE	; s0 = TxDescProd - 1
	load_l	t0,TX_DESC_BEGIN	; and check for ring wrap
	andi	s0,s0,TX_DESC_INDEX
	or	s0,s0,t0
; }

do_dma:
	;; In setting up DMA, we don't have to worry about Tx buffer ring
	;; wrap as the hardware takes care of that for us. We do however
	;; have to check for
	;;	- space left in the buffer
	;;      - MAX_DMA (64K) which is the max amount that the h/w
	;;        allows us to set up a single DMA for
	;;	- host buffer crossing a 64K boundary - the h/w wraps
	;;	  to the top of the 64K region instead of crossing the
	;;	  boundary.

	sw	s0,s2,DAD_FW1		; rdhidap->fw1 = txdp
	sw	s4,s2,DAD_HADDR		; rdhidap->haddr = haddr
	sw	txdatap,s2,DAD_LADDR	; rdhidap->laddr = txdatap

	; t5 = "thislen", length we can DMA this time, 
	;	min (MAX_DMA, dmalen, spaceleft, len_to_host64Kbound)

	; first, min (dmalen, MAX_DMA)
	load_s	t0,MAX_DMA
	subu	t0,t0,s5
	bgez	t0,1f		;if (dmalen > MAX_DMA)
	move	t0,s5		;   t0 = MAX_DMA
	load_s	t0,MAX_DMA	; else t0 = dmalen
1:

	; don't exceed spaceleft
	subu	t1,s3,t0	; if (spaceleft >= t0)
	bgez	t1,2f		;   thislen = t0
	move	t5,t0		; else thislen = spaceleft
	move	t5,s3
2:
	; don't cross host 64K address, limit length to
	; (0x10000 - (haddr & 0x7ffff))
	lui	t0,1		; t0 = 0x10000
	andi	t1,s4,0xffff	; t1 = haddr & 0xffff
	subu	t0,t0,t1	; t0 = 0x10000 - (haddr & 0xffff)
	subu	t1,t5,t0
	blez	t1,1f		; if (thislen > t0)
	noop
	move	t5,t0		;	thislen = t0
1:

	sw	t5,s2,DAD_DMALEN	; rdhidap->dmalen = thislen

	addi	t1,s2,DAD_SIZE
	sw	t1,regBase,DRHi_ProdReg	; ++DARdHiProd, queue the DMA
					; h/w takes care of ring wrap
.ifdef TRACE
	move	a1,s4	    ; haddr
	move	a2,txdatap  ; laddr
	move	a3,t5	    ; dmalen
	jal	trace
	lui	a0,TOC_RDHIDMA
.endif
	addu	txdatap,txdatap,t5	; txdatap += thislen
	subu	s5,s5,t5		; dmalen -= thislen

	bne	s5,r0,3f		; if (dmalen == 0) {
	; all done with this descriptor, pad to 1KB boundary from
	; start of packet data
	;    txdatap = txdp->address + 4 // for I-field
	;	      + ((txdp->length + 0x3ff) & 0xfffffc00);

	lw	t1,s0,0
	lui	t0,0x7fff
	ori	t0,t0,0xffff
	and	t1,t1,t0		; t1 = txdp->address // stripped SI bit
	addi	t1,t1,4			; t1 += 4

	lw	t0,s0,4
	lui	t2,1
	ori	t2,t2,0xffff
	and	t0,t0,t2		; t0 = txdp->flagsnlen & lenmask
	addi	t0,t0,0x3ff		; t0 += 0x3ff
	lui	t2,0xffff
	ori	t2,t2,0xfc00		; t2 = 0xfffffc00
	and	t0,t0,t2		; t0 &= 0xfffffc00

	addu	txdatap,t0,t1
	sw	r0,r0,saved_dlen	; saved_dlen = 0
	j	lab1
	move	s6,r0			; more = FALSE
					; } else {
3:
	; didn't get it all, dmalen > 0 still
	subu	s3,s3,t5		;   spaceleft -= thislen
	addu	s4,s4,t5		;   haddr += thislen
	ori	s6,r0,1			;   more = TRUE
					; }
lab1:

	; if (txdatap >= TX_BUF_END)
	;	txdatap -= TX_BUF_SIZE;
	load_l	t2,TX_BUF_END
	subu	t0,t2,txdatap
	bgtz	t0,4f
	load_l	t1,TX_BUF_SIZE
	subu	txdatap,txdatap,t1
4:
	; rdhidap->fw2 = txdatap;
	; if (more == 0) {
	;    // mark end for updating txdata_cons in txdone.
	;    txd_shadow_tab[this txd index] = txdatap;
	;    txdatap += 4 // advance to odd word for next desc
	;    rdhidap->fw2 = txdatap +4;
	;}
	bne	s6,r0,5f
	sw	txdatap,s2,DAD_FW2  ; delay slot execs either branch

	andi	t0,s0,TX_DESC_INDEX
	addi	t0,t0,4
	sw	txdatap,t0,txd_shadow_tab   ; stash end of data+pad
					    ; used for updating txdata_cons

	addi	txdatap,txdatap,4   ; plus 4 for next Ifield

	addi	t0,txdatap,4	; plus 4 more for start of next pkt's
				; data, stashed for updating tx data Producer
	bne	t0,t2,5f	; if (t0 == TX_BUF_END)
	sw	t0,s2,DAD_FW2	; delay slot
	load_l	t1,TX_BUF_SIZE
	subu	t0,t0,t1	    ;	    t0 -= TX_BUF_SIZE;
	sw	t0,s2,DAD_FW2
5:

	; TxDescProd = txdp + 1 // this is unnecessary but harmless for
	;			//a txdesc that was work-in-progress
	lw	t1,regBase,XMT_DescRefReg
	addi	t0,s0,TX_DESC_SIZE
	andi	t0,t0,TX_DESC_INDEX
	load_l	t2,TX_DESC_BEGIN
	addu	t0,t0,t2
	; if (txdp == TxDescRef) {
	;	timerRefReg = timerReg + stimeo;
	;	EventMaskReg |= TimerEvtBit;
	; }
	bne	t1,s0,6f
	sw	t0,regBase,XMT_DescProdReg  ; delay execs either way

	lw	t2,regBase,TimerLowReg
	lw	t1,r0,mbox1		    ; t1 = stimeo
	lui	t0,(EVT_TIMER>>16)
	addu	t2,t2,t1		    ; t2 = timerReg + stimeo
	sw	t2,regBase,TimerRefReg	    ; timerRefReg = timerReg + stimeo
	or	evtMask,evtMask,t0	    ; evtMask |= EVT_TIMER
6:

	; If (rdhidap == DRHi_RefReg), the DMA we just queued is the
	; next one to go, so let it get a headstart and tell the h/w
	; all the data is there already.
	lw	t0,regBase, DRHi_RefReg	    ;
	noop				    ;
	bne	t0,s2,chk_more		    ; if (rdhidap == DRHi_RefReg) {
	addi	t7,t7,-DMA_HEADSTART	    ;	target len -= DMA_HEADSTART;
	bgtz	t7,7f	    ; if (target len < 0), = 0
	noop
	ori	t7,r0,0

;; spin waiting for this DMA to start

; need a timeout here - if this DMA hangs for some reason, we
; will spin here forever! Need to be able to return to main loop to
; deal with the DMA attn. Countdown on t6 - set it to a sufficiently
; high number it should never expire except for hung DMAs.
	lui	t6,0x80
; while (1) {
7:
	lw	t1,regBase,DRHi_ConsReg
	lw	t0,regBase,AssistStateReg
	bne	t1,s2,8f	; consumer past ref => this DMA is complete

	andi	t2,t0,(DA_RDHISEL_BIT|DA_RDACT_BIT)
	beq	t2,r0,7b	; if !(AsstState & RDHISEL|RDACT) continue;

	lw	t3,regBase,DR_StateReg
	lw	t4,regBase,DR_LengthReg
	andi	t3,t3,DMA_ACTIVE
	beq	t3,r0,7b	; if !(DMAreadState & DMA_ACTIVE) continue;
	subu	t4,t4,t7	; if (DR_LengthReg <= target length) break;
	blez	t4,8f
	addi	t6,t6,-1
	bgtz	t6,7b
	lw	t0,r0,s_hung_dma
	noop
	addi	t0,t0,1
	blez	t6,chk_more	; bailout, don't advance TxDataProducer
	sw	t0,r0,s_hung_dma
; } // end while loop
8:
	;; // Tell hardware data has arrived if we got everything or if we have
	;; // space in the data buffer for everything in this TX Desc.
	;; if (!more || (spaceleft > dmalen)) {
	bne	s6,r0,10f
	subu	t4,s3,s5	; spaceleft - dmalen
	blez	t4,10f
	noop
9:
	; XMT_ProdReg = rdhidap->fw2	// claim that all the data has arrived
	lw	t4,s2,DAD_FW2
	noop
	sw	t4,regBase,XMT_ProdReg
	j	chk_more
	;; }
	;; else {
	;;      // If we don't have room set FW2 to not advance the producer so
	;;	// eh_DArhdone won't start the transmission.
	;;	rdhidap->fw2 = XMT_ProdReg
10:
	lw	t4,regBase,XMT_ProdReg
	noop
	sw	t4,s2,DAD_FW2
	;; }

chk_more:
	beq	s6,r0,advance_ptrs	    ; if (more != 0)
	lw	s2,regBase,DRHi_ProdReg	    ; s2 = RdHiDAprod
	lw	t1,regBase,DRHi_RefReg	    ; t1 = Reference
	addi	t0,s2,DAD_SIZE
	andi	t0,t0,DAD_RINGMASK  ; t0= (prod + size) & mask
	andi	t1,t1,DAD_RINGMASK  ; t1 = cons & mask
	bne	t0,t1,1f
	; if (prod + 1 == cons) { // RdHi ring is full
	lw	t2,r0,s_rdhi_full
	load_s	rrState,RDHIDA_WAIT_STATE
	addi	t2,t2,1
	beq	r0,r0,2f
	sw	t2,r0,s_rdhi_full
	; } else if (spaceleft > 0) goto do_dma
1:
	bgtz	s3,do_dma
	; else {
	lw	t2,r0,s_dbuf_full
	load_s	rrState,DATABUF_WAIT_STATE
	addi	t2,t2,1
	sw	t2,r0,s_dbuf_full
	; }
2:

	sw	s5,r0,saved_dlen	    ;   saved_dlen = dmalen
	j	main_loop		    ;	
	sw	s4,r0,saved_haddr	    ;	saved_haddr = haddr
					    ;   goto main_loop
					    ;   // don't adv ptrs cause not
					    ;   // done with this LINC desc
					    ; }	// (more != 0)

advance_ptrs:
	; Finished processing a linc xmit descriptor. Adjust localXDp 
	; and lincXDp accordingly
	
	; if (localXDp->flags & LXDF_WN) {
	;	lincXDp = top of Linc ring
	;	goto get_tx_cmds
	; }
	lhu	t2,localXDp,LXD_FLAGSNLEN	; t2 = localXDp->flags
	noop

	andi	t0,t2,LXDF_WN
	beq	t0,r0,1f
	noop
	j	get_tx_cmds
	lw	lincXDp,r0,l2rtabp  ;lincXDp = init_info.l2rtabp
1:
	; else ++lincXDp
	addi	lincXDp,lincXDp,LXDSIZE

	addi	localXDp,localXDp,LXDSIZE
	load_s	t0,lxdtab_end
	bne	localXDp,t0,2f
	noop
	j	get_tx_cmds
2:
	; jump delay slot - safe instr
	load_s	rrState,NORMAL_STATE	    ; rrState = NORMAL
	j	main_loop		    ; return to main loop
	ori	evtMask,evtMask,EVT_SW3	    ; evtMask |= EVT_SW3, remember
					    ;	to come back here if nothing
					    ;	more pressing to deal with
;=====================================================================
.eject
;=====================================================================
;
; is_dd:
;
; Digression from eh_sw3 handler. Infrequent branch removed from
; main flow to avoid wasted I-cache.
; (s0 = txd prod reg, s1 = linc desc flags)
;
; if (localXDp->flags & LXDF_DD) {  // dummy descriptor
;   // means that MB and CC must be off, and length = 0
;   fill in remainder of txd_shadow_tab
;   if (perm_conn == 0) {
;	turn on perm conn config bit
;	perm_conn++
;   }
;   queue dummy descr
;   if (--perm_conn == 0) {
;	turn off perm conn config bit;
;   }
;   goto advance_ptrs;
; }
;
; XXX do I need to be paranoid and make sure that length = 0?
;

is_dd:
	; before we can do anything the xmit desc queue must have drained
	; need to fill in txd_shadow_tab w/ data ring ptr,
	; interleave with checking for perm_conn on to keep
	; delay slots filled
	lw      t1,regBase,XMT_DescConsReg	; t1 = txdConsumer
	andi	t0,s0,TX_DESC_INDEX
	beq	s0,t1,1f			; prod == cons?
	lw      t3,r0,perm_conn			; t3 = perm_conn
	; flip the validity bit back
	lw	t0,r0,next_txdvbit		; t0 = next_txdvbit
	noop
	xori	t0,t0,LXDF_VB
	sw	t0,r0,next_txdvbit
	j	main_loop
	ori	evtMask,evtMask,EVT_SW3	        ; evtMask |= EVT_SW3, try again

1:
	addi	t0,t0,4				; point to desc 2nd word 
	bne     t3,r0,2f			; already in perm_conn?
	sw	txdatap,t0,txd_shadow_tab	; fill shadow_tab buf ptr

	; perm_conn not currently turned on ==> end of continued pkt
	; enable HX_STATE_PERM which is required to be on due to a
	; road runner bug which will not look at the dd unless this
	; is the case.
	lw	t2,regBase,HX_StateReg	; t2 = HX_StateReg
	addi    t3,t3,1		        ; t3 = perm_conn +1
	ori	t2,t2,HX_STATE_PERM
	sw	t2,regBase,HX_StateReg  ; HX_StateReg |= HX_STATE_PERM

	; all done with this linc descriptor, advance hw tx desc prod
2:
	addi    s0,s0,TX_DESC_SIZE
	andi	s0,s0,TX_DESC_INDEX
	load_l	t4,TX_DESC_BEGIN
	addu	s0,s0,t4
	sw      s0,regBase,XMT_DescProdReg

	; must drain descriptor prior to changing HX_StateReg
	lw      t1,regBase,XMT_DescConsReg ; t1 = txdConsumer
3:
	noop
	bne     s0,t1,3b        ; (prod == cons) => empty ring
	lw      t1,regBase,XMT_DescConsReg ; t1 = txdConsumer

	; should we disable HX_STATE_PERM?
	addi    t3,t3,-1        ; t3 = perm_conn -1
	bne     t3,r0,4f	; stay in perm conn mode?
	sw      t3,r0,perm_conn ; perm_conn -= 1

	lw      t2,regBase,HX_StateReg  ; t2 = HX_StateReg
	addi    t0,r0,~HX_STATE_PERM    ; t0 = ~HX_STATE_PERM
	and     t2,t2,t0
	sw      t2,regBase,HX_StateReg  ; HX_StateReg &= ~HX_STATE_PERM
4:
	; and go to bottom of linc desc processing loop.
	j       advance_ptrs
	noop


;==================================================================
;
; is_pc:
;
; Digression from eh_sw3 handler
; if linc desc flags has Perm Conn bit on, increment Perm Conn and
; change RR h/w Tx State register to Permanent mode. Increment
; perm_conn counter. 
;
is_pc:
	lw	t0,r0,perm_conn		; t0 = perm_conn
	lw	t2,regBase,HX_StateReg	; t2 = HX_StateReg
	addi	t0,t0,1
	ori	t2,t2,HX_STATE_PERM
	sw	t0,r0,perm_conn		; perm_conn += 1
	j	do_dma
	sw	t2,regBase,HX_StateReg  ; HX_StateReg |= HX_STATE_PERM

;===================================================================
.eject
;===================================================================
flush_queue:
	;; jump here when we are flushing descriptors trying to find
	;; end of packet/connection - throw out everything before
	;; that.

	bne	r0,rrState,1f
; if (rrState == NORMAL_STATE) {
	; check next local xmit cmd desc for validity
	lhu	t1,localXDp,LXD_FLAGSNLEN	; t1 = localXDp->flags
	lw	t0,r0,next_txdvbit		; t0 = next_txdvbit
	andi	t2,t1,LXDF_VB

	beq	t2,t0,3f    ; validity bit is right value
	noop

	; if (localXDp entry not valid) {
	; Still in flush mode, but we need more cmds.
	; Fill out the next Rd LowPri DMA assist descriptor.
get_more2flush:
	load_s	localXDp,lxdtab	    ; localXDp = lxdtab
	lw	t0,regBase,DRLo_ProdReg	    ; t0=DRLoProd
	load_s	loword,lxdtab
	sdw	lincXDp,t0,0	; DRLoProd->{haddr,laddr}={lincXDp,lxdtab}
	load_s	t1,LXDTABSIZE
	sw	t1,t0,DAD_DMALEN    ; DRLoProd->dmalen = LXDTABSIZE

	; Advance Producer Reg to kick off DMA
	; don't have to check for ring wrap as high order bits in 
	; register are not writeable
	addi	t0,t0,DAD_SIZE
	sw	t0,regBase,DRLo_ProdReg

	lui	rrState,(FLUSH_STATE >> 16)
	j main_loop
	ori	rrState,rrState,CMD_WAIT_STATE
	; } // if (G_localXDp entry not valid) {
3:
	; flip next_txdvbit
	xori	t0,t0,LXDF_VB
	sw	t0,r0,next_txdvbit

; } else // work-in-progress, linc desc validity bit already checked and flipped
;	G_rrState = NORMAL;
1:
	load_s	rrState,NORMAL_STATE
2:
	; if (localXDp->flag & (MB | CC))
	;     send FLUSHED ack to let 4640 know desc avail for reuse
	;     goto flush_next_cmd;

	lhu	s0,localXDp,LXD_FLAGSNLEN	; s0 = localXDp->flags
	noop
	andi	t0,s0,(LXDF_CC | LXDF_MB)
	beq	t0,r0,6f

        ; queue_evt_msg (G_lincXDp, DESC_FLUSH);
	; goto  flush_next_cmd
        move    a0,lincXDp
        jal     queue_evt_msg
        lui     a1,EOC_DESC_FLUSH
	j	flush_next_cmd
	noop

6:
	; if (flag & DUMMY_DESC)
	;   if (--G_perm_conn <= 0)
	;	turn off perm conn config bit;
	andi	t1,s0,LXDF_DD
	beq	t1,r0,4f
	lw	t0,r0,perm_conn
	noop
	addi	t0,t0,-1
	sw	t0,r0,perm_conn
	bgtz	t0,4f
	lw	t1,regBase,HX_StateReg
	addi	t2,r0,~HX_STATE_PERM		;t2=~HX_STATE_PERM, sign-extended
	and	t1,t1,t2
	sw	t1,regBase,HX_StateReg
4:
 
	; Found the end of flush that we are waiting for.
	; send FLUSHED ack to let 4640 know desc avail for reuse

	; queue_evt_msg (G_lincXDp, DESC_FLUSH); 
	move	a0,lincXDp
	jal	queue_evt_msg
	lui	a1,EOC_DESC_FLUSH

	; go to normal desc processing loop
	j	advance_ptrs
	noop

flush_next_cmd:
	; All done with this linc tx desc entry, advance both local
	; mem and sdram pointers to next tx desc

	; if (G_localXDp->flags & WRAP_NEXT)
	;   rest of stuff we fetched is useless as it was out of range
	andi	t0,s0,LXDF_WN
	beq	t0,r0,5f
	noop
	j	get_more2flush
	lw	lincXDp,r0,l2rtabp  ;lincXDp = init_info.l2rtabp
5:
	; else ++lincXDp;
	addi	lincXDp,lincXDp,LXDSIZE

	; if (++localXDp == end of lxdtab) // end of local table
	;	j get_more2flush
	addi	localXDp,localXDp,LXDSIZE
	load_s	t0,lxdtab_end
	beq	localXDp,t0,get_more2flush
	noop
	j	flush_queue	    ; top of loop, process next cmd
	noop
;----------------- end of flush_queue code ------------------------
.eject
;==================================================================
; eh_DArhdone:
;   event handler for Read HiPri DMA Assist done. We use this assist
;   engine for DMA-ing data to be transmitted, from LINC SDRAM, to
;   the RR h/w tx data buffer. For each completed DMA, we advance
;   the Tx Data Producer register to the (1KB-padded) end of the data
;   for that descriptor. This value was stashed in the firmware2 field
;   of the DMA assist descriptor. Then if the queue is still not empty,
;   we spin-wait for the next DMA to get a headstart, then tell the tx
;   h/w that it's all there.
;
eh_DArhdone:
	; register usage: s0 = reference
	;		  s1 = producer
	;		  s2 = consumer

.ifdef TRACE
	lw	a1,regBase,DRHi_RefReg
	lui	a0,TOC_RDHIDONE
	jal	trace
	lw	a2,a1,DAD_LADDR
.endif
	; Since we work in flow-through mode, the *Reference
	; descriptor's data has already been claimed to be present,
	; unless we timed out waiting for the headstart to arrive.
	; So, do the TxDataProd=ref->fw2 assignment anyway.
	lw	s0,regBase,DRHi_RefReg	    ; s0 = ref = DRHi_RefReg
	lw	s1,regBase,DRHi_ProdReg	    ; s1 = prod = DRHi_ProdReg
	lw	t4,s0,DAD_FW2		    ; t4 = ref->fw2
	addi	s0,s0,DAD_SIZE		    ; ref++
	sw	t4,regBase,XMT_ProdReg	    ; TxDataProd = t4
	sw	s0,regBase,DRHi_RefReg	    ; DRHi_RefReg = ref
	lui	t6,0x80		    	 ; t6 = countdown index = 0x800000
; while (ref != prod) {	// if (ref == prod) ring is empty, no more to do
darh_loop:
	;;  read back DRHi_RefReg to adjust for ring wrap
	lw	s0,regBase,DRHi_RefReg
	noop
	beq	s0,s1,darh_ret

	lw	s2,regBase,DRHi_ConsReg	    ; s2 = cons = DRHi_ConsReg
	lw	t4,s0,DAD_FW2		    ; t4 = ref->fw2

	; if (reference != consumer) {	// this dma has completed
	beq	s0,s2,1f
	lw	t5,s0,DAD_DMALEN	    ; t5 = ref->dmalen
	addi	s0,s0,DAD_SIZE		    ; ref++
	sw	s0,regBase,DRHi_RefReg	    ; DRHi_RefReg = ref
	j	darh_loop		    ; continue
	sw	t4,regBase,XMT_ProdReg	    ; TxDataProd = t4
1:
	; }
	; else, *ref is the next one to start, wait for headstart
	; and advance Tx Data Producer register

	; if !(assist state & DA_RDHISEL_BIT) continue;
	lw	t0,regBase,AssistStateReg
	lw	t1,regBase,DR_StateReg
	andi	t0,t0,(DA_RDHISEL_BIT|DA_RDACT_BIT)
	beq	t0,r0,2f

	; if !(DR_State & active) continue
	lw	t3,regBase,DR_LengthReg
	andi	t1,t1,DMA_ACTIVE
	beq	t1,r0,2f

	; if (DR_Length Reg > (ref->dmalen + DMA_HEADSTART) continue
	subu	t3,t3,t5
	addi	t3,t3,-DMA_HEADSTART
	bgtz	t3,2f
	noop

	; XMT_ProdReg = ref->fw2	// claim that all the data has arrived
	sw	t4,regBase,XMT_ProdReg
	j	darh_ret
2:
	; if (--counter > 0) continue
	addi	t6,t6,-1
	bgtz	t6,darh_loop
	lw	t0,r0,s_hung_dma
	noop
	addi	t0,t0,1
	sw	t0,r0,s_hung_dma
; } // end while loop


darh_ret:
	load_s	t0,RDHIDA_WAIT_STATE
	beq	rrState,t0,rdhida_wait_retry
	noop
	j	main_loop
	noop

;-------------- end of eh_DArhdone event handler ------------------
.eject
;==================================================================
;
; eh_DAwldone
;   event handler for DMA assist Write Lopri completion. This is the
;   the assist engine used to DMA event messages back to LINC SDRAM.
;   We only queue one such write dma at a time. In this event handler
;   we check if we have any more events waiting in the ring, and if
;   so, set up another DMA to dispatch what we have (or as much as
;   ring wraps will allow).
;
eh_DAwldone:
.ifdef TRACE
	lw	a1,regBase,DWLo_RefReg
	lui	a0,TOC_WRLODONE
	jal	trace
	lw	a2,a1,DAD_LADDR
.endif
	lw	t1,regBase,DWLo_ConsReg
	lw	t0,regBase,DWLo_ProdReg	; dadp = DWLo_ProdReg
	sw	t1,regBase,DWLo_RefReg	; Reference=Consumer, turns off event
	; assert t0 == t1 at this point

	lw	s0,r0,evtab_prod	; evprod = evtab_prod
	lw	s1,r0,evtab_cons	; evcons = evtab_cons

	lw	t2,r0,linc_evrp

	subu	t1,s0,s1	; t1 = dmalen = evprod - evcons
	beq	t1,r0,main_loop	; if (evprod == evcons) nothing to ack
	; foll. delay instr is harmless if branch taken
	sw	t2,t0,DAD_HADDR	; dadp->hostaddr = linc_evrp
	bgtz	t1,4f		; if (evprod > evcons)	;
	sw	s1,t0,DAD_LADDR ; dadp->localaddr = evcons
	load_s	t1,evtab_end	; else
	subu	t1,t1,s1	;	dmalen = evtab_end - evcons;
4:
.ifdef TRACE
	move	a1,t2		; haddr
	move	a2,s1		; laddr
.endif

	lw	t4,r0,r2ltabendp; t4 = linc_evring_end
	addu	t3,t2,t1	; t3 = linc_evrp + dmalen
	
	subu	t5,t4,t3
	bgtz	t5,5f		; if (t4 <= t3) {
	lw	t6,r0,r2ltabp	;
	subu	t1,t4,t2	;	dmalen = linc_evring_end - linc_evrp
	beq	r0,r0,6f	;	linc_evrp = &linc_evring[]
	sw	t6,r0,linc_evrp ; }
5:
	addu	t2,t2,t1	; else
	sw	t2,r0,linc_evrp	;   linc_evrp += dmalen
6:

	addu	s1,s1,t1	; evcons += dmalen
	load_s	t2,evtab_end
	bne	s1,t2,7f	; if (evcons == evtab_end)
	sw	t1,t0,DAD_DMALEN; dadp->dmalen = dmalen
	load_s	s1,evtab	;	evcons = evtab
7:
	sw	s1,r0,evtab_cons

	addi	t0,t0,DAD_SIZE
	sw	t0,regBase,DWLo_ProdReg
.ifdef TRACE
	move	a3,t1		; dmalen
	jal	trace
	lui	a0,TOC_WRLODMA	; trace (toc_wrlodma, haddr, laddr, len)
.endif
	j	main_loop
	noop

;------ end of eh_wrlodone ------------------------------------
.eject
;==================================================================
;
; eh_txdone
;   event handler for transmit done event.
;   Check Consumer Register, send ack to LINC for that desc 
;   (this implies acks for everything up to here)
;
eh_txdone:
.ifdef TRACE
	lw	a1,regBase,XMT_DescRefReg
	lw	a2,regBase,XMT_DescConsReg
	lw	a3,regBase,XMT_DescProdReg
	jal	trace
	lui	a0,TOC_TXDONE
.endif

	jal	ack_xmit_ok
	noop

	; if (rrState == DESC_WAIT_STATE) goto desc_wait_retry
	load_s	t0,DESC_WAIT_STATE
	beq	t0,rrState,desc_wait_retry

	; if (rrState == DATABUF_WAIT_STATE) goto databuf_wait_retry
	load_s	t0,DATABUF_WAIT_STATE
	beq	t0,rrState,databuf_wait_retry
	noop

	j	main_loop;
	noop

; ----- end of eh_txdone -------------------------------------------
.eject
;====================================================================
;
; eh_txattn
;
;   Event handler for transmit attention event.
;   The RR tx hw is halted at this point, waiting for us to clean up
;   and restart it. TxD Consumer should be pointing at the problem
;   descriptor. If descr requiring attention does not have MB or
;   CC set, we just send back an error event for this tx desc. Otherwise
;   we need to flush to end of packet or connection.
;
;   We can also arrive here through the timeout handler, for an
;   xmit packet which is not moving.
;
;   Register usage:
;	s0 = tdp, Tx Desc Ptr to problem descriptor. If flushing
;	     this moves down the ring to the last packet/conn continued
;	     descr in the sequence.
;	s1 = Tx Desc producer. i.e. end of valid descrs in ring
;	s2 = wip_flushed, boolean to indicate work-in-progress was flushed
;	s3 = dma_flushed, boolean to indicate some DMAs were flushed

eh_txattn:
.ifdef TRACE
	lw	t0,regBase,XMT_DescConsReg
	lui	a0,TOC_TXATTN
	lw	a1,t0,0	    ; word 0 of problem descr
	lw	a2,t0,4	    ; word 1 of problem descr
.if 0	
	lui	t1,0x8000
	nand	t1,t1,t1
	and	t1,t1,a1    ; mask off SI bit for address
	jal	trace
	lw	a3,t1,0	    ; Ifield
.endif
	jal	trace
	lw	a3,regBase,HX_StateReg
.endif
	lw	t0,r0,s_tx_attn
	lw	s0,regBase,XMT_DescConsReg  ; the problem descriptor
	lw	s1,regBase,XMT_DescProdReg  ; producer
	addi	t0,t0,1
	sw	t0,r0,s_tx_attn		; ++s_tx_attn

	; if (XMT_DescConsReg == XMT_DescProdReg) { // empty queue
	;;     // no clean up to do, just reset the hw interface.
	;;     s_false_attns += 1
	;;     HX_State &= ~ENABLE
	;;     HX_State |= ENABLE
	bne	s0,s1,8f
	lw	t0,r0,s_falseattn
	lw	t1,regBase,HX_StateReg
	addi	t0,t0,1
	load_s	t2,HX_STATE_ENABLE
	nor	t2,t2,t2
	and	t3,t2,t1
	sw	t3,regBase,HX_StateReg	; HX_State &= ~ENABLE
	sw	t0,r0,s_falseattn
	ori	t3,t3,HX_STATE_ENABLE
	sw	t3,regBase,HX_StateReg	; HX_State |= ENABLE
	j	main_loop
	noop
	;; } // empty Tx queue
8:
	; if (HX_State & IdleDesc) {
	;;	HX_State = HX_State // reset idle event bit
	;;	j main loop
	lw	t3,regBase,HX_StateReg
	lui	t1,0x0100
	and	t1,t1,t3
	beq	t1,r0,1f
	sw	t3,regBase,HX_StateReg
	j	main_loop
	noop
	;; }
1:
	; if (HX_State & ErrorEvent && tdp->address & SIField == 0)
	;;	if (tdp->flags & (MB|CC) == 0 && tdp->retries != RETRYMAX)
	;;		tdp->retries++
	;;		mbox_retries++
	;;		tdp->flags |= RETRY
	;;		HX_State &= ~ENABLE
	;;		HX_State |= ENABLE
	;;		j main loop
	lui	t1,0x0600
	and	t1,t1,t3
	beq	t1,r0,1f
	noop
	lw	t0,s0,0
	lui	t1,0x8000
	and	t0,t0,t1
	bne	t0,r0,1f
	noop
	lw	t0,s0,4
	lui	t1,(LXDF_MB|LXDF_CC)
	and	t1,t0,t1
	bne	t1,r0,1f

	srl	t1,t0,17
	andi	t1,t1,0xf
	addi	t2,r0,RRSRC_RETRY_MAX
	beq	t2,t1,1f
	addi	t1,t1,1
	sll	t1,t1,17
	lui	t2,LXDF_RETRIES
	nor	t2,t2,t2
	and	t0,t0,t2

	lw	t4,r0,mbox_retries
	or	t0,t0,t1
	addi	t4,t4,1
	sw	t4,r0,mbox_retries
	sw	t0,s0,4
	load_s	t1,HX_STATE_ENABLE
	nor	t1,t1,t1
	and	t2,t1,t3		; t3 is HX_State still
	sw	t2,regBase,HX_StateReg	; HX_State &= ~ENABLE
	ori	t2,t2,HX_STATE_ENABLE
	sw	t2,regBase,HX_StateReg	; HX_State |= ENABLE
	j	main_loop
	noop

1:
	; Pause the DMA Assist as we may need to flush dma descrs.
	; This allows current DMA to complete, but no more to start.
	lw	t1,regBase,AssistStateReg
	lw	t2,regBase,XMT_DescRefReg
	ori	t1,t1,DA_PAUSE_BIT
	sw	t1,regBase,AssistStateReg

	load_s	s2,0			; wip_flushed = FALSE

	; if (cons != ref) some earlier good xmits have not been acked,
	; so ack those to 4640 first. ack_xmit_ok() acks everything
	; from ref up to last one before cons, & sets ref to cons
	beq	s0,t2,9f
	noop
	jal	ack_xmit_ok
	noop
9:
	; scan descrs in queue, flush MB|CC descrs.
	load_l	t5,TX_DESC_BEGIN
scan_loop:
	; if (tdp->flags & (MB|CC) == 0) {
	lw	t0,s0,4
	lui	t1,(LXDF_MB|LXDF_CC)
	and	t0,t0,t1
	bne	t0,r0,1f
	;	if ((tdp+1 == prod) && saved_dlen != 0) {
	addi	t1,s0,TX_DESC_SIZE
	andi	t1,t1,TX_DESC_INDEX
	andi	t0,s1,TX_DESC_INDEX
	bne	t0,t1,2f
	lw	t2,r0,saved_dlen
	noop
	beq	t2,r0,2f
	noop
	load_s	s2,1			; wip_flushed = TRUE
	;	}
2:
	j	end_scanloop
	noop
	;}
1:
	; if (++tdp != TxDescProd) { continue;
	move	t4,s0			; save old desc
	addi	s0,s0,TX_DESC_SIZE	; ++tdp, chk for ring wrap
	andi	s0,s0,TX_DESC_INDEX
	addu	s0,s0,t5
	bne	s0,s1,scan_loop
	noop
	; }

	; else end of queued descrs, end not found
	lui	t0,(FLUSH_STATE >> 16)
	or	rrState,rrState,t0	; rrState |= FLUSH_STATE
	move	s0,t4			; reset tdp to last descr
	; fall through out of loop
end_scanloop:

	; At this point s0 points to last problem descriptor
	; to be error acked. We now scan the queued RdHi DMA assist
	; descriptors (data DMA) and cancel anything dma requests
	; for data we no longer need.

	lw	s4,regBase,DRHi_ConsReg
	lw	s5,regBase,DRHi_ProdReg
	load_s	s3,0			; dma_flushed = 0

; for (s4 = DRHi_Cons; s4 != DRHi_Prod; s4++) {
	beq	s4,s5,3f

	; s6 = wrap = (tdp >= XMT_DescCons) ? 0 : 1
	; t6 = XMT_DescConsReg
	lw	t6,regBase,XMT_DescConsReg
	noop
	subu	t0,s0,t6
	bltz	t0,1f
	load_s	s6,1
	load_s	s6,0
1:
;     if (s4->firmware1 <= tdp, relative to Cons) {
	lw	t0,s4,DAD_FW1

;	    if (! wrap) {
	bne	s6,r0,2f
;	        if ((temp->fw1 < TxDescCons) || (temp->fw1 > L_tdp)))
;		    break;

	subu	t1,t6,t0
	bgtz	t1,5f
	subu	t1,t0,s0
	bgtz	t1,5f
	noop
	beq	r0,r0,4f
	noop
; } else {	// wrap
2:
;    if ((temp->fw1 < TxDescCons) && (temp->fw1 > L_tdp)))
;	break;
	subu	t1,t6,t0
	blez	t1,4f
	subu	t1,t0,s0
	bgtz	t1,5f
; } // else wrap
4:

	lw	t0,s4,DAD_FW2
	sw	r0,s4,DAD_DMALEN	    ; s4->dmalen = 0
	sw	t0,regBase,XMT_ProdReg	    ; XMT_ProdReg = s4->fw2

	addi	s3,s3,1			    ; ++dma_flushed 

	addi	s4,s4,DAD_SIZE
	andi	s4,s4,DAD_RINGMASK
	addi	s4,s4,DARHring
	bne	s4,s5,1b
	noop	
; } //  for (s4 = cons; s4 != prod; s4++)
5:	

; if (dma_flushed) {
	beq	s3,r0,3f

	; spin-wait for dma to complete before resetting Consumer
	; & reference register
6:
	lw	t2,regBase,DR_StateReg
	noop
	andi	t2,t2,(DA_RDACT_BIT | DA_WRACT_BIT)
	bne	t2,r0,6b    
	noop

	sw	s4,regBase,DRHi_RefReg
	sw	s4,regBase,DRHi_ConsReg
	; } // if (dma_flushed)
3:
	; AssistStateReg &= ~DA_PAUSE_BIT
	lw	t0,regBase,AssistStateReg
	addi	t1,r0,~DA_PAUSE_BIT
	and	t0,t0,t1
	sw	t0,regBase,AssistStateReg

tx_err_ack:
	; s0 should be pointing to the last problem descr
	; to be acked.
	lw	t1,regBase,HX_StateReg

	andi	t0,s0,TX_DESC_INDEX
	ldw	a0,t0,txd_shadow_tab

	; Error event is in bits 25-26 of HX_StateReg
	;   0 = no error, most likely we got here through timeout
	;   1 = Conn Rej
	;   2 = Discon error
	;   3 = Internal parity error

	srl	t1,t1,23
	andi	t1,t1,0xc

	sw	loword,r0,txdata_cons

	jal	queue_evt_msg
	lw	a1,t1,tx_errcodes

	;; Restart the interface
	addi	s0,s0,TX_DESC_SIZE
	sw	s0,regBase,XMT_DescRefReg
	sw	s0,regBase,XMT_DescConsReg

	lw	t0,regBase,XMT_DescProdReg
	lw	s0,regBase,XMT_DescRefReg   ; so I don't have to check wrap
	noop

	beq	s0,t0,7f
	; if (TxDesc producer != consumer) { // ring is not empty
	lw	t1,regBase,TimerLowReg
	lw	t2,r0,mbox1

	lui	t3,(EVT_TIMER >> 16)
	addu	t1,t1,t2
	sw	t1,regBase,TimerRefReg	; TimerRefReg = TimerReg + stimeo

	or	evtMask,evtMask,t3	; evtMask |= EVT_TIMER

	; XXX do I need to do this?
	; Set XMT consumer register to next good descriptor
	; t0 = s0->start_addr (mask out SI bit, bit 31)
	lw	t0,s0,0
	lui	t1,0x8000
	nor	t1,t1,t1
	and	t0,t0,t1
	j	restart_interface
	sw	t0,regBase,XMT_ConsReg
7:
	; else ring is empty, initialize everything to top of buffer
	lw	t0,regBase,XMT_BaseReg
	noop
	sw	t0,regBase,XMT_ConsReg
	sw	t0,r0,txdata_cons
	addi	txdatap,t0,4		; Ifield always begins on odd word
	addi	t0,t0,8
	sw	t0,regBase,XMT_ProdReg

	sw	r0,r0,saved_dlen
	lui	t3,(EVT_TIMER >> 16)
	nor	t3,t3,t3
	and	evtMask,evtMask,t3	; evtMask &= ~EVT_TIMER

restart_interface:
	lw	t0,regBase,HX_StateReg
	load_s	t1,HX_STATE_ENABLE
	nor	t1,t1,t1
	and	t3,t0,t1
	sw	t3,regBase,HX_StateReg	; HX_State &= ~ENABLE
	noop
	noop
	ori	t3,t3,HX_STATE_ENABLE
	sw	t3,regBase,HX_StateReg	; HX_State |= ENABLE


; if (dma_flushed) {
	beq	s3,r0,8f
	lw	t2,regBase,DRHi_RefReg
	lw	t0,regBase,DRHi_ProdReg

	; if there is another data dma queued, wait for it
	; to start and tell RR xmit logic that it's arrived

; if (RdHi DA reference != producer) {

	lui	t6,0x80
	beq	t0,t2,3f
	
	lw	t5,t2,DAD_DMALEN
	noop
	addi	t5,t5,-DMA_HEADSTART
	;; while (DRHi_ConsReg != DRHi_RefReg) {
	;;     
; while (1) {
1:
	lw	t1,regBase,DRHi_ConsReg
	lw	t2,regBase,DRHi_RefReg
	lw	t0,regBase,AssistStateReg
	bne	t1,t2,2f	; consumer past ref => this DMA is complete

	andi	t3,t0,(DA_RDHISEL_BIT|DA_RDACT_BIT)
	beq	t3,r0,1b	; if !(AsstState & RDHISEL|RDACT) continue;

	lw	t3,regBase,DR_StateReg
	lw	t4,regBase,DR_LengthReg	    ; len remaining to be xfered
	andi	t3,t3,DMA_ACTIVE
	beq	t3,r0,1b	; if !(DMAreadState & DMA_ACTIVE) continue;
	subu	t4,t4,t5	; if (DR_LengthReg <= target length) break;
	blez	t4,2f
	addi	t6,t6,-1
	bgtz	t6,1b
	lw	t0,r0,s_hung_dma
	noop
	addi	t0,t0,1
	blez	t6,8f		; bailout, don't advance Tx ptr
	sw	t0,r0,s_hung_dma
; } // end while loop
2:
	lw	t1,t2,DAD_FW2
	noop
	sw	t1,regBase,XMT_ProdReg
; // if (RdHi DA reference != producer)

3:		
	; if ((G_rrState == RDHIDA_WAIT_STATE) && !wip_flushed)
	;    jump rdhida_wait_retry;
	load_s	t0,RDHIDA_WAIT_STATE
	bne	rrState,t0,8f
	noop
	beq	s2,r0,rdhida_wait_retry
	noop
; } // end if (dma_flushed)
8:

; if (G_rrState & FLUSH_STATE) {
	lui	t0,(FLUSH_STATE >> 16)
	and	t1,t0,rrState
	beq	t1,r0,10f

	; if (G_rrState == (FLUSH_STATE | CMD_WAIT_STATE)) goto mainloop
	ori	t2,t0,CMD_WAIT_STATE
	beq	rrState,t2,main_loop

	; G_rrState &= ~FLUSH_STATE;
	nor	t0,t0,t0
	and	rrState,rrState,t0

	; if (G_rrState == NORMAL_STATE) G_evtMask &= ~SW3BIT;
	bne	rrState,r0,1f
	load_s	t0,EVT_SW3
	nor	t0,t0,t0
	and	evtMask,evtMask,t0
1:
	j	flush_queue
	noop
; } // if (G_rrState & FLUSH_STATE)
10:
	; if (wip_flushed) goto advance_ptrs
	bne	s2,r0,advance_ptrs

	; if (G_rrState == DESC_WAIT_STATE) goto desc_wait_retry
	load_s	t0,DESC_WAIT_STATE
	beq	rrState,t0,desc_wait_retry

	; if (G_rrState == DATABUF_WAIT_STATE) goto databuf_wait_retry;
	load_s	t0,DATABUF_WAIT_STATE
	beq	t0,rrState,databuf_wait_retry

	j	main_loop
	noop
;------------------- end of eh_txattn -------------------------------
.eject
;====================================================================
; eh_rxattn
;
; Event handler for rx attn. Update the Glink status bit in mbox 31
; and send an interrupt to the 4640.
; Clear error bits in HR_StateReg by writing back what we read.
; (Error bits are cleared by writing a 1. Other bits are read-only
; or read-write, so are unchanged by writeback.
eh_rxattn:
	lw	t0,regBase,HR_StateReg
	load_l	t1,RX_STATE_LNKRDY|RX_STATE_FLGSYNCD
	sw	t0,regBase,HR_StateReg		; Clear error bits.

	and	t2,t1,t0
	beq	t2,t1,1f
	load_s	t3,INIT_SUCCESS
	load_s	t3,GLINK_NOT_RDY
1:
	lw	t4,regBase,MiscLocalReg
	sw	t3,r0,mbox_status
	ori	t4,t4,4
.ifndef SIM
	sw	t4,regBase,MiscLocalReg		; Interrupt on Pin A
.endif

.ifdef TRACE
	; TRACE	(TOC_RXATTN, HR_StateReg)
	move	a1,t0
	jal	trace
	lui	a0,TOC_RXATTN
.endif	;TRACE
	j	main_loop
	noop
;------------------- end of eh_rxattn -------------------------------
.eject
;=====================================================================
; Event handler for transmit timeout, which is the only timer we set
; (for dest hang or switch retry when CAMP-ON bit is set).
;
eh_timer:
.ifdef TRACE
	lw	a1,regBase,XMT_DescRefReg
	lw	a2,regBase,XMT_DescConsReg
	lw	a3,regBase,XMT_DescProdReg
	jal	trace
	lui	a0,TOC_TIMEO
.endif

	lw	t0,r0,s_timeout
	lw	s0,regBase,XMT_DescConsReg
	lw	s1,regBase,XMT_DescRefReg
	addi	t0,t0,1
	sw	t0,r0,s_timeout

	; make sure it isn't because we've just been too busy to
	; deal with tx done events. If so, just advance the TimerRef
	; to turn off the timeout event and return to the main loop,
	; we'll go to eh_txdone from there.
	beq	s0,s1,1f
	lw	s2,regBase,XMT_DescProdReg

	; if (XMT_Desc Consumer != Reference) { // xmt done, but we didn't
	;                                       service	event

	; timerRefReg = timerReg + G_stimeo;
	lw	t0,regBase,TimerLowReg	; current time
	lw	t1,r0,mbox1		; stimeo
	noop
	addu	t0,t0,t1
	sw	t0,regBase,TimerRefReg

	; if (txDescProdReg == txDescConsReg) // empty tx queue
	;    EventMaskReg &= ~TimerEvtBit;
	bne	s0,s2,2f
	lui	t0,(EVT_TIMER >> 16)
	nor	t0,t0,t0
	and	evtMask,evtMask,t0
2:
	j	main_loop	; // go deal with the tx done from regular EH
	; } // if if (XMT_DescCons != Ref)
1:
	noop

	; assert (txDescProdReg != txDescConsReg)
	bne	s0,s2,3f
	lw	t0,r0,s_falsealarm
	lui	t1,(EVT_TIMER >> 16)
	addi	t0,t0,1
	sw	t0,r0,s_falsealarm
	nor	t1,t1,t1
	j	main_loop
	and	evtMask,evtMask,t1
3:
	; HX_StateReg &= ~ENABLE_TRANSMIT_BIT, disable xmit while
	; we cleanup.
	lw	t0,regBase,HX_StateReg
	load_s	t1,HX_STATE_ENABLE
	nor	t1,t1,t1
	and	t0,t0,t1
	j	eh_txattn
	sw	t0,regBase,HX_StateReg
	; cleanup done in eh_txattn, return main loop or elsewhere from there

; ------------------ end of eh_timer ---------------------------------
.eject
; ====================================================================
;  Subroutines.
;  Each one executed through a jal[r], and return though j ra.
;  If any args are expected, they are passed through a0-a4.
;  They undertake not to muck with s0-s? registers.
;
; =====================================================================
;
; ack_xmit_ok()
;   sends an ack up for all descriptors in the tx ring between
;   the reference and the current consumer. Also advances the
;   reference up to the acked descriptor.
;   Called by txdone event handler and the main linc-command
;   processing code (sw3 event handler)
;
ack_xmit_ok:
	addi	sp,sp,-4
	sw	ra,sp,0
	lw	t2,regBase,XMT_DescConsReg
	lw	t3,regBase,XMT_DescProdReg

	lw	t0,regBase,TimerLowReg	    ; current time
	; if (cons == prod), queue is empty, turn off timer event
	bne	t2,t3,1f
	sw	t2,regBase,XMT_DescRefReg   ; XMT_DescRef = XMT_DescCons
					    ; either branch
	lui	t1,(EVT_TIMER >> 16)
	nor	t1,t1,t1		    ; t1 = ~EVT_TIMER
	beq	r0,r0,2f
	and	evtMask,evtMask,t1
1:
	; else {
	lw	t1,r0,mbox1		    ; stimeo
	noop
	addu	t0,t0,t1
	sw	t0,regBase,TimerRefReg	    ; TimerRefReg = TimerReg + stimeo
	; }
2:

	addi	t2,t2,-TX_DESC_SIZE	    ; to last one xmitted
	andi	t2,t2,TX_DESC_INDEX	    ; t2 = index of last descr sent
	; update txdata_cons with stashed end-of-data value
	ldw	a0,t2,txd_shadow_tab	    ; a0 = addr of LINC descr
	noop
	noop
	sw	loword,r0,txdata_cons

	jal	queue_evt_msg
	lui	a1,EOC_OK		    ; a1 = EOC_OK
	
	lw	ra,sp,0
	addi	sp,sp,4
	jr	ra
	noop

; --------- end of routine ack_xmit_ok() --------------------------
.eject
;==================================================================
;
; queue_evt_msg(LINCxdp, result)
; a0 = LINCxdp - address of the descriptor in LINC SDRAM that
;		  this ack applies to
; a1 = result  - the result opcode, placed in the high byte of
;		  register a1.
; Register usage:
; s0 = evprod   // next one in queue to write
; s1 = evcons   // first one in queue (write DMA starts here)
; s2 = last     // one we'd like to piggy back on

queue_evt_msg:
	addi	sp,sp,-28
	sw	s0,sp,0
	sw	s1,sp,4
	sw	s2,sp,8
	sw	ra,sp,24

.ifdef TRACE
	sw	a0,sp,12
	sw	a1,sp,16
	move	a2,a1
	move	a1,a0
	jal	trace
	lui	a0,TOC_EVTMSG

	lw	a0,sp,12
	lw	a1,sp,16
.endif

	load_l	t0,0xffffff
	and	a0,a0,t0		; strip off high byte

	lw	s0,r0,evtab_prod	; evprod = evtab_prod
	lw	s1,r0,evtab_cons	; evcons = evtab_cons
	load_s	t0,evtab
	beq	s0,s1,no_piggyback	; ring is empty
	noop

	bne	s0,t0,1f		; if (evprod == evtab)
	addi	s2,s0,-4		;   last = evtab_end - 4
	ori	s2,r0,(evtab_end-4)	; else last = evprod - 4
1:
	lw	t0,s2,0
	lui	t1,0x7f00		; mask for the opcode
	and	t2,t0,t1		; t2 = last->opcode

	bne	t2,a1,no_piggyback

	; if we can piggyback, replace last 3 bytes with new 
	; LINCxdp and return
	lui	t1,0xff00
	and	t2,t0,t1		; t2 = last->{vb,opcode}
	or	t2,t2,a0
	j	qem_ret
	sw	t2,s2,0

no_piggyback:
	; form the new event msg by OR-ing a0 and a1. Also set
	; the validity bit. This bit is 1 for even entries in
	; evtab, and 0 for odd entries
	andi	t0,s0,4		; t0 = evprod & 4
	bne	t0,r0,2f
	or	t1,a0,a1	;delay slot, either br, t1=result|lincXDp
	lui	t2,0x8000
	or	t1,t1,t2	; t1 |= 0x80000000
2:
	sw	t1,s0,0		; *evprod = t1
	addi	s0,s0,4		; evprod += 4
	load_s	t0,evtab_end	; check for ring wrap
	bne	s0,t0,3f	; if (evprod == evtab_end)
	lw	t1,regBase,DWLo_ConsReg
	load_s	s0,evtab	;	evprod = evtab
3:
	sw	s0,r0,evtab_prod; evtab_prod = evprod

	; If there is already a write lo assist DMA queued, the handler
	; for that completion will kick off another batch. Otherwise
	; queue the job now.
	lw	t0,regBase,DWLo_ProdReg	; dadp = DWLo_ProdReg
	lw	t2,r0,linc_evrp
	bne	t1,t0,qem_ret	; if wr lo DMA asst ring not empty, return
	subu	t3,s0,s1	; t3 = evprod - evcons

	sw	t1,regBase,DWLo_RefReg	; DWLo_RefReg = DWLo_ConsReg
				; update Reference, turns off event
				
	sw	t2,t0,DAD_HADDR	; dadp->hostaddr = linc_evrp
	sw	s1,t0,DAD_LADDR ; dadp->localaddr = evcons
.ifdef TRACE
	move	a1,t2		; haddr
	move	a2,s1		; laddr
.endif
	bgtz	t3,4f		; if (evprod > evcons)
	subu	t1,s0,s1	; 	dmalen = evprod - evcons
	load_s	t1,evtab_end	; else
	subu	t1,t1,s1	;	dmalen = evtab_end - evcons
4:

	lw	t4,r0,r2ltabendp; t4 = linc_evring_end
	addu	t3,t2,t1	; t3 = linc_evrp + dmalen
	
	subu	t5,t4,t3
	bgtz	t5,5f		; if (t4 <= t3) {
	lw	t6,r0,r2ltabp	;
	subu	t1,t4,t2	;	dmalen = linc_evring_end - linc_evrp
	beq	r0,r0,6f	;	linc_evrp = &linc_evring[]
	sw	t6,r0,linc_evrp ; }
5:
	addu	t2,t2,t1	; else
	sw	t2,r0,linc_evrp	;   linc_evrp += dmalen
6:

	addu	s1,s1,t1	; evcons += dmalen
	load_s	t2,evtab_end
	bne	s1,t2,7f	; if (evcons == evtab_end)
	sw	t1,t0,DAD_DMALEN; dadp->dmalen = dmalen
	load_s	s1,evtab	;	evcons = evtab
7:
	sw	s1,r0,evtab_cons

	addi	t0,t0,DAD_SIZE
	sw	t0,regBase,DWLo_ProdReg

.ifdef TRACE
	move	a3,t1		; dmalen
	jal	trace
	lui	a0,TOC_WRLODMA
.endif

qem_ret:
	lw	ra,sp,24
	lw	s0,sp,0
	lw	s1,sp,4
	lw	s2,sp,8
	jr	ra
	addi	sp,sp,28

; ------- end of routine queue_evt_msg(LINCxdp, result) ---------------

