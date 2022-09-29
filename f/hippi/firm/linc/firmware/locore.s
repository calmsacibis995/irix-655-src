/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1997, Silicon Graphics, Inc                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/

/*
 * locore.s for HIPPI firmware
 *
 * $Revision: 1.7 $
 *
 */

#include <sys/asm.h>
#include <sys/regdef.h>

#include "r4650.h"
#include "sys/PCI/linc.h"
#include "lincutil.h"
#include "hippi_sw.h"

#include "eframe.h"
#include "rdbg.h"

IMPORT(intframep, 4)
IMPORT(badaddr,4)
IMPORT(baddr_cause,4)
IMPORT(last_compare,4)

.extern	exception_handler
.extern interrupt_handler
.extern debug_serial_int

.extern _fbss
.extern _end

#define SAVEGREGS() \
	sreg	$0,0(k0) ;\
	sreg	$1,4(k0) ;\
	sreg	$2,8(k0) ;\
	sreg	$3,12(k0) ;\
	sreg	$4,16(k0) ;\
	sreg	$5,20(k0) ;\
	sreg	$6,24(k0) ;\
	sreg	$7,28(k0) ;\
	sreg	$8,32(k0) ;\
	sreg	$9,36(k0) ;\
	sreg	$10,40(k0) ;\
	sreg	$11,44(k0) ;\
	sreg	$12,48(k0) ;\
	sreg	$13,52(k0) ;\
	sreg	$14,56(k0) ;\
	sreg	$15,60(k0) ;\
	sreg	$16,64(k0) ;\
	sreg	$17,68(k0) ;\
	sreg	$18,72(k0) ;\
	sreg	$19,76(k0) ;\
	sreg	$20,80(k0) ;\
	sreg	$21,84(k0) ;\
	sreg	$22,88(k0) ;\
	sreg	$23,92(k0) ;\
	sreg	$24,96(k0) ;\
	sreg	$25,100(k0) ;\
	sreg	$26,104(k0) ;\
	sreg	$27,108(k0) ;\
	sreg	$28,112(k0) ;\
	sreg	$29,116(k0) ;\
	sreg	$30,120(k0) ;\
	sreg	$31,124(k0)

#define RESTOREGREGS() \
	lreg	$1,4(k0) ;\
	lreg	$2,8(k0) ;\
	lreg	$3,12(k0) ;\
	lreg	$4,16(k0) ;\
	lreg	$5,20(k0) ;\
	lreg	$6,24(k0) ;\
	lreg	$7,28(k0) ;\
	lreg	$8,32(k0) ;\
	lreg	$9,36(k0) ;\
	lreg	$10,40(k0) ;\
	lreg	$11,44(k0) ;\
	lreg	$12,48(k0) ;\
	lreg	$13,52(k0) ;\
	lreg	$14,56(k0) ;\
	lreg	$15,60(k0) ;\
	lreg	$16,64(k0) ;\
	lreg	$17,68(k0) ;\
	lreg	$18,72(k0) ;\
	lreg	$19,76(k0) ;\
	lreg	$20,80(k0) ;\
	lreg	$21,84(k0) ;\
	lreg	$22,88(k0) ;\
	lreg	$23,92(k0) ;\
	lreg	$24,96(k0) ;\
	lreg	$25,100(k0) ;\
	/* skip $26, it's k0 */ \
	lreg	$27,108(k0) ;\
	lreg	$28,112(k0) ;\
	lreg	$29,116(k0) ;\
	lreg	$30,120(k0) ;\
	lreg	$31,124(k0)

	.text
	.set noreorder



/*******************************************
 * _start
 *******************************************/

	.globl	_start
	.globl	_gp
	.ent	_start,0
_start:
	/* Set it and forget it! */
	la	gp,_gp

#ifdef SSRAM
	/* If we're loaded into SSRAM, we have to copy our
	 * vectors to buffer memory before turning off the
	 * BEV bit.
	 */
	li	t0,FIRM_START
	li	t1,K0_TO_K1(VECT_START+LINC_SDRAM_WG8_ADDR)
	li	t2,VECT_SIZE
1:
	lw	t3,0(t0)
	sw	t3,0(t1)
	addiu	t2,t2,-4
	addiu	t0,t0,4
	bgtz	t2,1b
	 addiu	t1,t1,4
#endif /* SSRAM */

	/* Turn off BEV bit.  Disable interrupts for now.
	 */
	li	t0,(SR_CU1|SR_CU0|SR_FR)
	mtc0	t0,C0_SR

#ifndef SABLE
	/* Use R4650 interrupt vector at 0x200.  Also clear s/w interrupts */
	li	t0,CAUSE_IV
	mtc0	t0,C0_CAUSE
#endif

#if !defined(SIM) && !defined(SABLE)
	/* Zero out BSS area. */
	la	t0,.bss
	la	t1,_end
	subu	t1,t1,t0
1:
	sw	zero,0(t0)
	addiu	t1,t1,-4
	bgtz	t1,1b
	 addiu	t0,t0,4
#endif /* ! SIM && ! SABLE */

	/* Set up stack pointer. */
	la	sp,FIRM_STACK

	jal	main
	 nop

	/* should never return but just in case... */
just_die_here:
	j	just_die_here
	 nop

	.end	_start


/*******************************************
 * My own exception handler
 *******************************************/
	.globl	exception_entry0
	.ent	exception_entry0,0

	.set	noat
exception_entry0:

	mfc0	k1,C0_CAUSE
	andi	k0,k1,CAUSE_EXCMASK

	/* Ordinary interrupt? */
	beq	k0,zero,interrupt_entry0

	/* Check for "protected" memory access.
	 */
	 sltiu	k1,k0,EXC_IBOUND
	bne	k1,zero,excpt0_dordbg
	 sltiu	k1,k0,EXC_DBE+1
	beq	k1,zero,excpt0_dordbg
	 nop

	la	k1,baddr_cause
	sw	k0,0(k1)

	/* If badaddr is set when a bus or address error
	 * occurs, just reset badaddr, skip past that
	 * load/store, and return from exception.
	 */
	lw	k0,badaddr
	beq	k0,zero,excpt0_dordbg
	 nop
	
	mfc0	k0,C0_EPC
	addi	k0,k0,4
	mtc0	k0,C0_EPC

	la	k1,badaddr
	sw	zero,0(k1)

	eret

excpt0_dordbg:

	/*
	 * Other exception cases.  Save context to k1seg eframe
	 * (in case remote debugger doesn't work we can see context in
	 * SDRAM from host).
	 *
	 */
	li	k0,PHYS_TO_K1( FIRM_EFRAME )

	/* save ordinary registers in eframe */
	SAVEGREGS()

	/* save fp registers ? */

	.set	at

	mfc0	t0,C0_EPC
	sreg	t0,R_EPC*REGSIZE(k0)
	mfhi	t0
	sreg	t0,R_MDHI*REGSIZE(k0)
	mflo	t0
	sreg	t0,R_MDLO*REGSIZE(k0)
	mfc0	t0,C0_BADVADDR
	sreg	t0,R_BADVADDR*REGSIZE(k0)

	mfc0	t0,C0_COUNT
	sreg	t0,R_COUNT*REGSIZE(k0)
	mfc0	t0,C0_COMPARE
	sreg	t0,R_COMPARE*REGSIZE(k0)
	mfc0	a1,C0_SR				# use a1 so arg1 is sr
	sreg	a1,R_SR*REGSIZE(k0)
	mfc0	t0,C0_ERROR_EPC
	sreg	t0,R_ERREPC*REGSIZE(k0)

#ifndef GDB
	mfc0	t0,C0_IWATCH
	sreg	t0,R_WATCHLO*REGSIZE(k0)
	mfc0	t0,C0_DWATCH
	sreg	t0,R_WATCHHI*REGSIZE(k0)
	mfc0	t0,C0_ECC
	sreg	t0,R_ECC*REGSIZE(k0)
	mfc0	t0,C0_CACHE_ERR
	sreg	t0,R_CACHERR*REGSIZE(k0)
	mfc0	t0,C0_TAGLO
	sreg	t0,R_TAGLO*REGSIZE(k0)
#endif /* !GDB */

	mfc0	a0,C0_CAUSE			/* get cause */
	sreg	a0,R_CAUSE*REGSIZE(k0)		/* use a0 so arg0 is cause */

	/* Access the stack through kseg0.  Up to this point, we've
	 * been doing uncached stores to the eframe.
	 */
	li	sp,PHYS_TO_K0( FIRM_EFRAME+EFRAMESZ-32 )

	/* We use an "la" and "jalr" to jump to the handler because
	 * we might've been called from the cache error vector and
	 * now we want to attempt to run in cache again.
	 */
	la	t0,exception_handler
	jalr	t0				/* jump to handler */
	 nop
	
	/* If handler returns, we'll restore the whole e-frame and continue.
	 */
	la	k0,PHYS_TO_K1( FIRM_EFRAME )

	/* Restore SR (sets EXL bit)
	 */
	lreg	t0,R_SR*REGSIZE(k0)
	li	t1,~SR_IE
	and	t1,t1,t0
	mtc0	t1,C0_SR		/* first set EXL but without IE */
	mtc0	t0,C0_SR		/* now restore IE */

	/* restore hi/lo */
	lreg	t0,R_MDHI*REGSIZE(k0)
	mthi	t0
	lreg	t0,R_MDLO*REGSIZE(k0)
	mtlo	t0

	/* restore count/compare registers */
	lreg	t0,R_COMPARE*REGSIZE(k0)
	mtc0	t0,C0_COMPARE
	lreg	t0,R_COUNT*REGSIZE(k0)
	mtc0	t0,C0_COUNT

	lreg	t0,R_EPC*REGSIZE(k0)
	 nop
	mtc0	t0,C0_EPC
	 nop
	
	.set	noat

	/* restore registers from eframe */
	RESTOREGREGS()

	eret

	.set	at

	.end	exception_entry0

/**********************************************************************
 * NMI exception handler
 **********************************************************************/

	.set	noat
	.globl	nmi_exception0
	.ent	nmi_exception0,0

nmi_exception0:

	/* For now, just go to code that saves context and drops into
	 * debugger.  Later I may move ERROR_EPC into EPC just to fudge
	 * gdb into giving proper back-traces.
	 */
	
	j	excpt0_dordbg
	 nop

	.set	at
	.end	nmi_exception0

/**********************************************************************
 * Cache error exception handler
 **********************************************************************/

	.set	noat
	.globl	cacheerr_exception0
	.ent	cacheerr_exception0,0

cacheerr_exception0:

	/* What we really want to do is determine if there's a REAL
	 * cache error or if we just had some bad address access.
	 * If we think the cache is still working, we want to jump
	 * back into cached execution space and run the remote-debugger
	 * exception handler.  If not, we can't do much.  We'll just dump
	 * what we can to SDRAM and bail out.
	 *
	 * As it stands, the ordinary exception handler will store the
	 * context into SDRAM using k1seg space (it always does).  Then
	 * it will attempt to call the remote-debugger's exception handler
	 * via k0seg space.  If that goes wrong, we're probably hosed because
	 * it'll come back here and loop forever and we'll lose the
	 * original context.  Hopefully, most cache errors are driver error.
	 *
	 * To be continued when I get my hands on real hardware and see
	 * what typical failures are...
	 */
	
	j	excpt0_dordbg
	 nop

	.set	at
	.end	cacheerr_exception0

/**********************************************************************
 * Interrupt exception handler
 **********************************************************************/

	.set	noat
	.globl	interrupt_entry0
	.ent	interrupt_entry0,0

interrupt_entry0:

	/*
	 * Interrupt exception case.
	 *
	 * Save context into frame that intframep points.  The
	 * interrupt handler can modify intframep if it wants
	 * re-enable interrupts at a higher priorities.
	 */
	lw	k0,intframep

	/* Save general registers */
	SAVEGREGS()

	.set	at

	mfc0	t0,C0_EPC
	sreg	t0,R_EPC*REGSIZE(k0)

	/* Save HI/LO */
	mfhi	t0
	sreg	t0,R_MDHI*REGSIZE(k0)
	mflo	t0
	sreg	t0,R_MDLO*REGSIZE(k0)

	mfc0	a1,C0_SR			/* arg1 is SR */
	sreg	a1,R_SR*REGSIZE(k0)

	mfc0	a0,C0_CAUSE			/* put cause in arg0 */
	sreg	t0,R_CAUSE*REGSIZE(k0)

	move	a2,k0				/* arg2 is the frame ptr */

	li	t1,~(SR_EXL|SR_IE)		/* clear EXL and disable */
	and	a1,a1,t1			/* all interrupts. */
	mtc0	a1,C0_SR

	jal	interrupt_handler
	 addi	sp,k0,(EFRAMESZ-32)		/* leave stack space for args*/
	
	lw	k0,intframep

	/* Restore SR (also sets EXL) */
	lreg	t0,R_SR*REGSIZE(k0)
	li	t1,~SR_IE
	and	t1,t1,t0
	mtc0	t1,C0_SR		/* first, set EXL without IE */
	mtc0	t0,C0_SR		/* now restore IE */

	/* Restore HI/LO */
	lreg	t0,R_MDHI*REGSIZE(k0)
	mthi	t0
	lreg	t0,R_MDLO*REGSIZE(k0)
	mtlo	t0

	/* Restore EPC */
	lreg	t0,R_EPC*REGSIZE(k0)
	mtc0	t0,C0_EPC

	.set	noat

	/* Restore general registers */
	RESTOREGREGS()

	eret

	.end	interrupt_entry0

