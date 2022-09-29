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
;
;===========================================================================
.include "diag.h"
; 
; Roadrunner SRAM diag:	 Writes to EEPROM
; Does a series of writes to a (non existent) eeprom. test trigger point
; for Ken.

.text 0
;; -------------------- Begin GCA definition ---------------
;; First 512 bytes of SRAM is GCA, accessible by host and RR. 
;; First 128 bytes of GCA is 32 mailboxes. 
;; Put the instructions in the rest of GCA.

	.word start		;  start PC, info for downloader
	.block	124		; skip mailboxes

;; Register usage:
;; r0 - hardcoded zero
;; r1 - some pattern, 0xdeadface
;; r2 - EEPROM base address

start:
	;; init MiscLocalReg to enable EEPROM writes
	load_l	regBase,REGBASE
	load_s	r1,0xd40a
	sw	r1,regBase,MiscLocalReg

	lui	r2,0x8000	; EEPROM address base
	load_l	r1,0xdeadface	; r1 hold pattern 0xdeadface

	;; Issue a series of 8 writes to the EEPROM
	sw	r1,r2,0
	noop
	sw	r1,r2,8
	noop
	sw	r1,r2,16
	noop
	sw	r1,r2,24
	noop
	sw	r1,r2,32
	noop
	sw	r1,r2,40
	noop
	sw	r1,r2,48
	noop
	sw	r1,r2,56
	noop
1:
	beq 	r0,r0,1b
	noop
