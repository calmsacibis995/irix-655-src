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
; diag.h Header file for Roadrunner diags.
;
; $Revision: 1.0 $	$Date: 1996/11/13 01:55:20 $
;
.ifdef HISTORY
*
* $Log: diag.h,v $
* Revision 1.0  1996/11/13 01:55:20  irene
* No Message Supplied
*
*
.endif
;
;===========================================================================
;
;; Define Road runner general-purpose CPU registers
;;  r0 is hardwired zero
.reg	r0,%%0
.reg	r1,%%1
.reg	r2,%%2
.reg	r3,%%3
.reg	r4,%%4
.reg	r5,%%5
.reg	r6,%%6
.reg	r7,%%7
.reg	r8,%%8
.reg	r9,%%9
.reg	r10,%%10
.reg	r11,%%11
.reg	r12,%%12
.reg	r13,%%13
.reg	r14,%%14
.reg	r15,%%15
.reg	r16,%%16
.reg	r17,%%17
.reg	r18,%%18
.reg	r19,%%19
.reg	r20,%%20
.reg	r21,%%21
.reg	r22,%%22
.reg	r23,%%23
.reg	r24,%%24
.reg	r25,%%25
.reg	r26,%%26
.reg	r27,%%27
;; next 4 are special
.reg	regBase,%%28	; base for accessing RR special registers
.reg	sp, %%29	; stack ptr
.reg	loword,%%30	; r30 used for low word of doubleword store/reads
.reg	ra,%%31		; r31 for return address in jalr

SRAM_SIZE = 0x40000	; 256 MB SRAM

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

