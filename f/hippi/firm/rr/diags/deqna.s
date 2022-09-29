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
; Roadrunner SRAM diag:	 data = ~address test.
;
; $Revision: 1.1 $	$Date: 1996/11/13 03:51:27 $
;
.ifdef HISTORY
*
* $Log: deqna.s,v $
* Revision 1.1  1996/11/13 03:51:27  irene
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
;; r2 - ~address
;; r3 - pattern read
;; r4 - pass counter of how many times we've gone round memory
;; r20 - SRAM_SIZE

	.block 12		;  cache align halt near end of cacheline
start:
	lui	r25,0x8000	; EEPROM address base
	load_l	r20,SRAM_SIZE
	load_s	r4,0		; pass = 0
1:		
	addi	r4,r4,1		; pass++
	load_s	r1,memstart	; addr = memstart
2:		
	;; Write each word of memory (from memstart to SRAM_SIZE)
	;; with the logical not of its address.
	nor	r2,r1,r1	;  pattern = ~addr
	sw	r2,r1,0		; *addr = pattern (~addr)
	addi	r1,r1,4		; addr += 4
	bne	r1,r20,2b	; if (addr != SRAM_SIZE) goto 2
	noop

	;; Now read back each word of memory and see if it matches
	;; the logical not of its own address
	load_s	r1,memstart	; addr = memstart
3:	
	lw	r3,r1,0		; readback = *addr
	nor	r2,r1,r1	; target pattern = ~addr
	bne	r2,r3,stop	;  if readback != pattern goto halt
	noop
	addi	r1,r1,4		; addr += 4
	bne	r1,r20,3b	; if (addr != SRAM_SIZE) goto 3
	noop

	;; Make another pass
	beq	r0,r0,1b	; repeat at top of SRAM
	noop
stop:
	sw	r0,r25,0	; EEPROM access as trigger point for Ken
	halt
	noop
memstart:
