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
; Include file for both SOURCE and DESTINATION side Roadrunner firmware
; for the Lego HIPPI-Serial card. Compilation flags SRC_RR and DST_RR
; are used where differences occur.
;
; $Revision: 1.9 $	$Date: 1997/02/10 06:13:39 $
;
.ifdef HISTORY
*
* $Log: rr.h,v $
* Revision 1.9  1997/02/10 06:13:39  jimp
* support for mbox change of dest rr accept/reject
*
 * Revision 1.8  1996/09/23  22:01:21  irene
 * cleaned up comments.
 *
 * Revision 1.6  1996/09/13  23:52:41  irene
 * Turn on mbox event in DEF_EVTMASK_HI.
 *
 * Revision 1.5  1996/09/07  00:20:17  irene
 * support for mbox31 you-alive? queries.
 *
 * Revision 1.4  1996/07/02  02:32:23  irene
 * Added Rxattn to src event mask. Added TOC_RXATTN for src trace events.
 *
 * Revision 1.3  1996/05/14  00:07:43  irene
 * Changed to use DMA assist engine. This is controlled by turning on USE_DA compilation flag.
 *
 * Revision 1.1  1996/05/02  22:50:00  irene
 * First pass.
 *
*
.endif
;
;===========================================================================
;
;; Define Road runner general-purpose CPU registers
;;  r0 is hardwired zero
.reg	r0,%%0
.reg	v0,%%1
.reg	a0,%%2
.reg	a1,%%3
.reg	a2,%%4
.reg	a3,%%5
.reg	a4,%%6
.reg	t0,%%7
.reg	t1,%%8
.reg	t2,%%9
.reg	t3,%%10
.reg	t4,%%11
.reg	t5,%%12
.reg	t6,%%13
.reg	t7,%%14
.ifdef SRC_RR
.reg	t8,%%15
.else
.reg	spaceleft,%%15	; in LINC's data ring
.endif
.reg	s0,%%16
.reg	s1,%%17
.reg	s2,%%18
.reg	s3,%%19
.reg	s4,%%20
.reg	s5,%%21
.ifdef SRC_RR
.reg	s6,%%22
.else
.reg	lento64kb,%%22	; barrier to watch out for in flowthrough mode,
			; RR h/w can't cross 64k boundaries in a single DMA.
.endif
;; frequently-used globals that need to be in registers
.reg	regBase,%%23	; base for accessing RR special registers
.ifdef SRC_RR
.reg	rrState,%%24	; state of FSM
.reg	evtMask,%%25	; for joff stuff
.reg	lincXDp,%%26	; next LINC Xmit Desc to process
.reg	localXDp,%%27	; to local copy of same
.reg	txdatap, %%28	; to next place in tx data ring to write
.else ; DST_RR
.reg	rrState,%%24	; state of FSM
.reg	evtMask,%%25	; for joff stuff
.reg	ldtab_p,%%26	; next LINC Rcv Desc to send to BUFMEM
.reg	r2ldata_p,%%27	; to next place in Linc's rcv data ring to write
.reg	dataRef, %%28	; to next place in Linc's rcv descr ring to write
.endif
;; next 3 are special
.reg	sp, %%29	; stack ptr
.reg	loword,%%30	; r30 used for low word of doubleword store/reads
.reg	ra,%%31		; r31 for return address in jalr

;; ------------- Hardware tx/rx descriptors and buffers -------------
;; RR tx/rx hardware requires descriptors and data buffers
;; at end of SRAM - logically 2MB size, physically 256K, replicated
;; 8 times in address space. Hardware requires a contiguous layout of:
;; tx descr ring, rx descr ring, tx data buffer, rx data buffer.

.ifdef SRC_RR
RX_BUF_SIZE = 0x1000	; vestigial receive buffer
TX_BUF_SIZE = 0x21000	; 132K transmit buffer
.else
RX_BUF_SIZE = 0x21000	; 132K receive buffer
TX_BUF_SIZE = 0x1000	; vestigial transmit buffer
.endif
RX_DESC_RINGSIZE= 0x800	; 256 descrs * 8 bytes = 2KB
TX_DESC_SIZE = 8
TX_DESC_RINGSIZE= 128 * TX_DESC_SIZE ; 128 descrs * 8 bytes = 1KB
TX_DESC_INDEX = TX_DESC_RINGSIZE - 1

RX_BUF_END    = 0x200000	; end of SRAM = "2MB"
RX_BUF_BEGIN  = RX_BUF_END - RX_BUF_SIZE
TX_BUF_END    = RX_BUF_BEGIN
TX_BUF_BEGIN  = TX_BUF_END - TX_BUF_SIZE
RX_DESC_END   = TX_BUF_BEGIN
RX_DESC_BEGIN = RX_DESC_END - RX_DESC_RINGSIZE
TX_DESC_END   = RX_DESC_BEGIN
TX_DESC_BEGIN = TX_DESC_END - TX_DESC_RINGSIZE

RX_BUF_MASK   = RX_BUF_SIZE - 1
TX_BUF_MASK   = TX_BUF_SIZE - 1

;; ------------------- DMA assist rings -------------------------
;; DMA Assist Descriptor structure, offsets and size:
DAD_HADDR   = 0	    ; host address
DAD_LADDR   = 4	    ; local address
DAD_DMALEN  = 8	    ; dma length is last 2 bytes of this 4 byte field
DAD_STATE   = 12    ; DMA state register
DAD_FW1     = 24    ; firmware-defined field. For RdHiDA we use this to
; store the address of the hw tx desc which this dma applies to.
DAD_FW2	    = 28    ; firmware-defined field. For RdHiDA we use this to
; store the value we will write to the Tx Data Producer register once
; this DMA has commenced. The fw2 value may involve a pad to 1KB if this
; is the last DMA descriptor for a particular Tx descriptor.
DAD_SIZE    = 32

;; D.A. rings are 16 descr per ring. Hardware requires DMA assist 
;; rings to be contiguous 512 byte blocks, starting on 2048 alignment.
DAD_RINGSIZE = DAD_SIZE * 16
DAD_RINGMASK = DAD_RINGSIZE - 1

;; ---------------- Defines for RR special registers ---------------
REGBASE	= 0xC0000000	
; Following Regs are addressed through 0xc0000000 - 0xc00001ff
;; Config space regs
PCI_DVReg       =     0x00
PCI_CmdStatReg  =     0x04
PCI_ClassRevReg =     0x08
PCI_BistLatReg  =     0x0C
PCI_BaseReg     =     0x10
PCI_IntReg      =     0x3C

MiscHostReg     =     0x40
MiscLocalReg    =     0x44
CPU_PCReg       =     0x48
CPU_BrkptReg    =     0x4C
TimerHiReg      =     0x50
TimerLowReg     =     0x54
TimerRefReg     =     0x58
PCIStateReg	=     0x5C
MainEvtReg      =     0x60
MboxEvtReg      =     0x64
WindowBaseReg   =     0x68
HR_StateReg     =     0x70
HX_StateReg     =     0x74
H_OvrheadReg    =     0x78
ExtSerDataReg	=     0x7C

DW_HAddrHiReg   =     0x80
DW_HAddrLowReg  =     0x84
DR_HAddrHiReg   =     0x90
DR_HAddrLowReg  =     0x94
DR_LengthReg    =     0x9C
DW_StateReg     =     0xA0
DW_LAddrReg     =     0xA4
DW_ChksumReg    =     0xA8
DW_LengthReg    =     0xAC
DR_StateReg     =     0xB0
DR_LAddrReg     =     0xB4
DR_ChksumReg    =     0xB8

RCV_BaseReg     =     0xC0
RCV_ProdReg     =     0xC4
RCV_ConsReg     =     0xC8
CPUPriReg	=     0xCC
XMT_BaseReg     =     0xD0
XMT_ProdReg     =     0xD4
XMT_ConsReg     =     0xD8
DelLineStateReg	=     0xDC
RCV_DescProdReg =     0xE0
RCV_DescConsReg =     0xE4
RCV_DescRefReg  =     0xE8
XMT_DescProdReg =     0xF0
XMT_DescConsReg =     0xF4
XMT_DescRefReg  =     0xF8

DRHi_ProdReg    =     0x140
DRHi_ConsReg    =     0x144
DRHi_RefReg     =     0x148
DWHi_ProdReg    =     0x150
DWHi_ConsReg    =     0x154
DWHi_RefReg     =     0x158
AssistStateReg	=     0x15C
DRLo_ProdReg    =     0x160
DRLo_ConsReg    =     0x164
DRLo_RefReg     =     0x168
DWLo_ProdReg    =     0x170
DWLo_ConsReg    =     0x174
DWLo_RefReg     =     0x178
AssistBaseReg	=     0x17C

;; ----------- DMA Assist State Reg Settings
.ifdef SRC_RR
; Default bits on are: 32bitHostAddrFormat, OneDMAchannelActive,DAenabled
DEFAULT_ASST_STATE = 0xD
.else
; DST_RR uses both DMA channels
; Default bits on are: 32bitHostAddrFormat, DAenabled
DEFAULT_ASST_STATE = 9
.endif
; Pause bit used when we need to clean up stuff on attn (error) events
DA_PAUSE_BIT = 2
DA_RDACT_BIT   = 0x1000 ; Read DMA Channel currently active
DA_RDHISEL_BIT = 0x2000 ; Currently active Read DMA is from Rd Hi ring
			; - only valid if RdActive bit is on.
DA_WRACT_BIT   = 0x0100 ; Write DMA Channel currently active
DA_RDHISEL_BIT = 0x0200 ; Currently active Write DMA is from Wr Hi ring
			; - only valid if RdActive bit is on.

DMA_ACTIVE = 4	; bit in DMA Read State reg indicating DMA active
;; ------------ FSM states -------------------
.ifdef SRC_RR
NORMAL_STATE	    = 0
CMD_WAIT_STATE	    = 1
DESC_WAIT_STATE	    = 2
DATABUF_WAIT_STATE  = 3
RDHIDA_WAIT_STATE   = 4
FLUSH_STATE	    = 0x80000000    ;; High bit or-ed with one of above
.else
NORMAL_STATE	 = 0
LDESC_WAIT_STATE = 1	    ; LINC's descriptor ring is full
LDATA_WAIT_STATE = 2	    ; LINC's data ring is full
DADESC_WAIT_STATE = 3	    ; our DMA Assist Descriptor ring is full
.endif

;; ------------ Event Mask Bits --------------
.ifdef SRC_RR
; bits always turned on are: TxAttn, RxAttn, mbox DMARdAttn, DMAWrAttn,  
; AsstRdHiDone, AsstRdLoDone, AsstWrLoDone, TxDone
; DEFAULT_EVTMASK = 0x148c5804
DEF_EVTMASK_HI = 0x148c
DEF_EVTMASK_LO = 0x5804
.else
; bits turned on are RxAttn (bit 26), mbox (bit 23),
; Rd DMA attn (b19), Wr DMA attn (b18)
; Rx Started (b6), RxComplete (b5)
.ifdef USE_DA
; turn on RdHi and WrHi dma assist done events
DEFAULT_EVTMASK = 0x048c6060
.else
DEFAULT_EVTMASK = 0x048c0060
.endif
.endif

; Bits we toggle as needed:
EVT_TIMER	= 0x40000000 ; bit 30
EVT_RXATTN	= 0x04000000 ; bit 26
EVT_RDMADONE	= 0x00020000 ; bit 17
EVT_WDMADONE	= 0x00010000 ; bit 16
EVT_RXSTARTED	= 0x00000040 ; bit 6
EVT_RXDONE	= 0x00000020 ; bit 5
EVT_SW3		= 0x00000010 ; bit 4, have linc tx descs in local mem
			     ; to work on

;; ------------ HIPPI Transmit State register bits:
HX_STATE_PERM	= 2
HX_STATE_ENABLE = 1

;; ------------ HIPPI Receive State register bits:
RX_STATE_LNKRDY	  = 0x00010000
RX_STATE_FLGSYNCD = 0x00400000

;; ----------- Init Status codes
INIT_SUCCESS  = 1
GLINK_NOT_RDY = 2

;; ----------- mbox[31] commands
ACCEPT_CONN   = 3
REJECT_CONN   = 4

;; ------------------------ Trace Buffer
; Each trace entry is 4 words:
;	word 0:	    Trace opcode (1 byte), timestamp (3 bytes)
;	words 1,2,3:	trace args, depending on opcode
TRACE_ENTRY_SIZE = 16
TRACE_BUF_SIZE	= 4096
TRACE_BUF_MASK	= TRACE_BUF_SIZE - 1

;; ------------ Trace Opcodes
.ifdef SRC_RR
TOC_INIT    = 0x0100 ; init complete, no args
TOC_FETCH   = 0x0200 ; fetching block of Linc XDs. a1=lincXDp, a2=RdLoProdReg
TOC_RDLODONE= 0x0300 ; fetched block of LINC XDs. a1 = lincXDp, a2=RdLoDescReg
TOC_SW3	    = 0x0400 ; processing linc desc cmd, lincXDp, localXDp
TOC_RDHIDMA = 0x0500 ; queued a rd hi dma, a1=haddr, a2=laddr, a3=len
TOC_RDHIDONE= 0x0600 ; rd hi dma done, a1=DRHi_RefReg, a2=laddr
TOC_WRLODMA = 0x0700 ; queued a wr lo dma, a1=haddr, a2=laddr, a3=len
TOC_WRLODONE= 0x0800 ; wr low dma done, a1=DWLo_RefReg, a2=laddr
TOC_EVTMSG  = 0x0900 ; queue_evt_msg() called, a1=LINCxdp, a2=result
TOC_TXDONE  = 0x0a00 ; tx done event, a1=TxDescRef,a2=TxDescCons,a3=TxDescProd
TOC_TXATTN  = 0x0b00 ; tx attn event, a1 = TxDesc word0, a2=word1, a3=Ifield
TOC_TIMEO   = 0x0c00 ; timeout event, a1 = TxDesc Ref, a2 = cons, a3 = prod
TOC_RXATTN  = 0x0d00 ; rx attn event, a1 = HIPPI State Register
TOC_BADEVT  = 0x0e00 ; unexpected event - panic!, a1 = event.
.else
; Dst RR's trace opcodes:
TOC_INIT    = 0x0100
TOC_RXATTN  = 0x0200	; Rcv Attn event, a1=RxStateReg, a2=RcvDescProd
TOC_RCV     = 0x0300	; rcv handler event, a1=RcvDescCons, a2=RcvDescRef,
			; a3=RcvDescProd
TOC_DAWAIT_DESC = 0x0400; hit DADESC_WAIT_STATE while sending LINC desc,
			; a1=Cons, a2=Prod a3=Ref
TOC_DAWAIT_DATA = 0x0500; hit DADESC_WAIT_STATE while sending LINC data,
			; a1=cons, a2=prod, a3=Ref
TOC_RDMADONE= 0x0600	; Rd DMA done, a1=LINC's DescCons, a2=LINC's DataCons
TOC_WDMADONE= 0x0700	; Wr DMA done, a1=haddr, a2=laddr, a3=len
TOC_LDESC   = 0x0800	; Desc sent to Linc, args are words 0, 1, 2 of desc
TOC_LDATA   = 0x0900	; Data sent to Linc, a1=Linc addr, a2=loc addr, a3=len
TOC_DTIMEO  = 0x0a00	; Timeout, part1. a1=Desc Prod, a2=Cons, a3=Ref
TOC_LDATA_WAIT = 0x0b00	; Hit LDATA_WAIT_STATE
TOC_WATTN1  = 0x0c00	; Write DMA attn part1, a1=haddr,a2=laddr,a3=len
TOC_BADEVT  = 0x0d00    ; unexpected event - panic!, a1 = event.
TOC_WATTN2  = 0x0e00	; Write DMA attn part2,a1=state,a2=DA cons,a3=DAstate
TOC_SENDDESC= 0x0f00	; send_desc() called.
.endif

.ifdef SRC_RR
;; ------------ Event Opcodes (returned to 4640)
; The event message consists of a ptr to a LINC desc and an opcode.
; We are not obliged to ack every single descriptor if the result was
; good. The implication is that all unacked descriptors before
; this one were transmitted OK. So a good ack means everything before and
; including this was successful. A bad ack says everything before
; this was good but this is bad.
; The event message is 32 bits. High bit is the validity bit.
; bits 30-24 are result opcode, defined below so that a lui puts
; the opcode in the correct position for OR-ing.
; bits 23-0 is the pointer to the LINC descriptor that this ack is for.

EOC_OK		= 0 ; transmitted OK
EOC_DESC_FLUSH	= 0x0100 ; flushed, must have been preceded by a bad ack,
			; in which case all not-end-of-pkt and not-end-of-
			; connection would have been flushed.
EOC_CONN_TIMEO	= 0x0300 ; connection timed out, pkt not sent
EOC_DST_DISCON	= 0x0400 ; destination disconnected, not sen
EOC_CONN_REJ	= 0x0500 ; connection rejected, not sent
EOC_SRC_PERR	= 0x0700 ; local parity forced bad parity on HIPPI channel
.endif

;; ----------- Macros -----------------------

;; load_s - load a constant which is less than 16 bits
.macro load_s reg, ashort
    ori	\reg, r0, \ashort
.endm

;; load_l - load a 32-bit constant
.macro load_l reg, along
    lui	\reg, (\along) >> 16
    ori \reg, \reg, \along
.endm

;; load_a - load a relocatable routine address
;; XXX ???? what do I do if address is > 16 bits
.macro	load_a reg, r_addr
    ori	\reg, r0, \r_addr
.endm

;; move - move toreg, fromreg
;   set one register to another
.macro	move	toreg, fromreg
    or	\toreg,	r0, \fromreg
.endm

;; zero out a RR register
.macro	zero_rr  regoffset
    sw	r0,regBase,\regoffset
.endm

;; Noop for delay slots
.macro	noop
    ori r0, r0, 0
.endm

