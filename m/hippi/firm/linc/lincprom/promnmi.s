/*
 * promnmi.s
 *
 * Copyright 1996, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON
 * GRAPHICS, INC.
 *
 *
 * $Revision: 1.5 $
 */

#include <sys/asm.h>
#include <sys/regdef.h>

#include "r4650.h"
#include "sys/PCI/linc.h"
#include "lincprom.h"
#include "lincutil.h"


	.text

	.extern	bevdie

/* void
 * promnmi(void);
 *
 */
	.set	noreorder
	.set	noat

	.globl	promnmi
	.ent	promnmi,0
promnmi:
	/* This clears out the RD_ONLY bit which sometimes gets
	 * set inadvertantly.
	 */
	li	k0,PHYS_TO_K1( LINC_CONTROLLER_COMMAND_STATUS )
	lw	zero,0(k0)

#ifdef DEBUG
	/* Did NMI happen in "real" firmware?  We can jump to the
	 * debugger (hopefully).
	 */
	mfc0	k1,C0_ERROR_EPC
	li	k0,0xbfc00000
	sltu	k0,k1,k0
	beqz	k0,1f
	 nop
	
	/* Clear NMI's that are caused manually.
	 */
	li	k0,PHYS_TO_K1(LINC_CONTROLLER_INTERRUPT_STATUS)
	li	k1,(LINC_CISR_CLR_NMI|LINC_CISR_NMI_BUTTON)
	sw	k1,0(k0)

	/* Jump to firmware "pseudo-vector" for
	 * handling NMI's.
	 */
	li	k0,0x800001c0
	jr	k0
	 nop

1:
#endif /* DEBUG */

	/* Start dumping our context to SDRAM.
	 */
	li	k0,PHYS_TO_K1( LINC_SDRAM_ADDR + 0x200 )

	/* First, dump all general registers. */
	sreg	$0,0(k0)
	sreg	$1,4(k0)
	sreg	$2,8(k0)
	sreg	$3,12(k0)
	sreg	$4,16(k0)
	sreg	$5,20(k0)
	sreg	$6,24(k0)
	sreg	$7,28(k0)
	sreg	$8,32(k0)
	sreg	$9,36(k0)
	sreg	$10,40(k0)
	sreg	$11,44(k0)
	sreg	$12,48(k0)
	sreg	$13,52(k0)
	sreg	$14,56(k0)
	sreg	$15,60(k0)
	sreg	$16,64(k0)
	sreg	$17,68(k0)
	sreg	$18,72(k0)
	sreg	$19,76(k0)
	sreg	$20,80(k0)
	sreg	$21,84(k0)
	sreg	$22,88(k0)
	sreg	$23,92(k0)
	sreg	$24,96(k0)
	sreg	$25,100(k0)
	sreg	$26,104(k0)
	sreg	$27,108(k0)
	sreg	$28,112(k0)
	sreg	$29,116(k0)
	sreg	$30,120(k0)
	sreg	$31,124(k0)

	/* Dump CP0 registers. */
	mfc0	k1,C0_EPC
	sreg	k1,R_EPC*REGSIZE(k0)
	mfc0	k1,C0_CAUSE
	sreg	k1,R_CAUSE*REGSIZE(k0)
	mfc0	k1,C0_BADVADDR
	sreg	k1,R_BADVADDR*REGSIZE(k0)
	mfhi	k1
	sreg	k1,R_MDHI*REGSIZE(k0)
	mflo	k1
	sreg	k1,R_MDLO*REGSIZE(k0)
	mfc0    k1,C0_COUNT
	sreg    k1,R_COUNT*REGSIZE(k0)
	mfc0    k1,C0_COMPARE
	sreg    k1,R_COMPARE*REGSIZE(k0)
	mfc0	k1,C0_SR
	sreg	k1,R_SR*REGSIZE(k0)
	mfc0	k1,C0_ERROR_EPC
	sreg	k1,R_ERREPC*REGSIZE(k0)

	.set	at

	mfc0	a1,C0_ERROR_EPC
	nop
	mfc0	a2,C0_CAUSE
	nop

	/* Clear out NMI bits so next warm reset will be treated
	 * like a reset.
	 */
	li	k0,PHYS_TO_K1(LINC_CONTROLLER_INTERRUPT_STATUS)
	li	k1,( (LINC_CISR_NMI_SOURCES|LINC_CISR_CLR_NMI) & \
			~LINC_CISR_SET_NMI)
	sw	k1,0(k0)

	j	bevdie
	 li	a0,DIE_NMI

	.end	promnmi

