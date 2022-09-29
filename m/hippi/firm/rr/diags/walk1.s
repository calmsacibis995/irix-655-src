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
; Roadrunner SRAM diag:	 walking 1 test.
; Writes a walking 1 in each word of SRAM.
; 
;
; $Revision: 1.0 $	$Date: 1996/11/13 01:55:29 $
;
.ifdef HISTORY
*
* $Log: walk1.s,v $
* Revision 1.0  1996/11/13 01:55:29  irene
* No Message Supplied
*
.endif
;
;===========================================================================
.include "diag.h"

.text 0
;; -------------------- Begin GCA definition ---------------
;; First 512 bytes of SRAM is GCA, accessible by host and RR. 
;; First 128 bytes of GCA is 32 mailboxes. 
;; Put the instructions in the rest of GCA.

	.word start		;  start PC, info for downloader
	.block	124		; skip mailboxes

;; Register usage:
;; r0 - hardcoded zero
;; r1 - address of word under test
;; r2 - pattern to write
;; r3 - pattern read
;; r4 - pass counter of how many times we've gone round memory
;; r20 - SRAM_SIZE
;; Note if test halts because of mem compare failure, note
;; that reading r2 will give shifted value (as shift went in delay
;; slot) and not the actual value written.

	.block	24		; cache-align so halt is near end of cacheline
start:
	lui	r25,0x1		; target trigger address 0x10000
	load_l	r20,SRAM_SIZE
	load_s	r4,0		; pass = 0
1:		
	addi	r4,r4,1		; pass++
	load_l	r1,0x20000	; addr = memstart
2:	
	load_s	r2,1		; pattern = 1
3:		
	sw	r2,r1,0		; *addr = pattern
	noop			; delay slot
	lw	r3,r1,0		; readback = *addr
	noop			; delay slot
	bne	r2,r3,stop	;  if readback != pattern goto halt
	noop
	sll	r2,r2,1		; pattern <<= 1
	bne	r2,r0,3b	; if (pattern != 0) goto 3
	noop
	addi	r1,r1,4		; addr++
	bne	r1,r20,2b	; if (addr != SRAM_SIZE) goto 2
	noop
	beq	r0,r0,1b	; repeat at top of SRAM
	noop
stop:
	sw	r0,r25,0	; trigger point for Ken, write to 0x10000
4:
	beq 	r0,r0,4b
	noop
	noop
memstart:
