/*
 * promstart.s
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
 * $Revision: 1.9 $
 */

#include <sys/asm.h>
#include <sys/regdef.h>

#include "r4650.h"
#include "sys/PCI/linc.h"
#include "lincprom.h"
#include "lincutil.h"
#include "hippi_sw.h"


.extern	prommain
.extern	promnmi


	.text

/*
 * reset(void)
 *
 * Reset, Soft Reset, and NMI all come here.
 *
 */
	.set	noreorder

	.globl	reset
	.ent	reset,0
reset:
	/* Check for NMI or Warm RESET by looking at SR bit.
	 */
	mfc0	k1,C0_SR
	srl	k1,k1,16
	andi	k1,k1,(SR_SR>>16)
	beq	k1,zero,1f
	 nop
	
	/* Check CISR for NMI sources.  Otherwise, we are going to assume
	 * a warm reset.
	 */
	li	k0,PHYS_TO_K1(LINC_CONTROLLER_INTERRUPT_STATUS)
	lw	k0,0(k0)
	li	k1,LINC_CISR_NMI_SOURCES
	and	k0,k0,k1
	beq	k0,zero,1f		/* must be warm reset */
	 nop
	
	j	promnmi			/* do NMI */
	 nop

1:
	/* It's a real reset. */
	SETLEDS(PROGRESS_RESET)

	/* Speed up PROM accesses and reset byte-bus errors.
	 */
	li	k0,(LINC_BBCSR_EN_ERR | 				\
			LINC_BBCSR_RST_ERR | 		/* RW1C */	\
			LINC_BBCSR_PROM_SZ_ERR | 	/* RW1C */	\
			LINC_BBCSR_PAR_ERR | 		/* RW1C */	\
			LINC_BBCSR_WR_TO | 		/* RW1C */	\
			LINC_BBCSR_BBUS_EN |				\
			LINC_BBCSR_PULS_WID_W( 0x0f ) | 		\
			LINC_BBCSR_A_TO_CS_W( 0 ) | 			\
			LINC_BBCSR_CS_TO_EN_W( 0 ) | 			\
			LINC_BBCSR_EN_WID_W( 0x04 ) | 			\
			LINC_BBCSR_CS_TO_A_W( 0 ) );
	sw	k0,PHYS_TO_K1(LINC_BYTEBUS_CONTROL)

	/* Read-to-clear all these error bits */
	lw	zero,PHYS_TO_K1(LINC_BUFMEM_ERROR)
	lw	zero,PHYS_TO_K1(LINC_CHILD_PCI_ERR)
	lw	zero,PHYS_TO_K1(LINC_PPCI_ERROR)

	/* Clear out CISR. */
	li	k0,~(LINC_CISR_SET_NMI|LINC_CISR_FIRM_INTR)
	sw	k0,PHYS_TO_K1(LINC_CONTROLLER_INTERRUPT_STATUS)

	/* Turn off byte-bus and CPCI reset.  Also turn off IGNORE_ERRS.
	/* Roadrunner might still be hitting memory - reset it before
	 * memory tests are started.
	 */

	li	t0,LINC_LCSR_BOOTING | LINC_LCSR_RESET_CPCI
	sw	t0,PHYS_TO_K1(LINC_LINC_CONTROL_STATUS_RESET)

	li	t0, LINC_LCSR_BOOTING
	sw	t0,PHYS_TO_K1(LINC_LINC_CONTROL_STATUS_RESET)

	mtc0	zero,C0_CAUSE		# clear software interrupts
	mtc0	zero,C0_IWATCH		# clear/disable watchpoint interrupt
	mtc0	zero,C0_DWATCH

	mtc0	zero,C0_IBASE		# unfortunately, I think zero addr
	mtc0	zero,C0_IBOUND		# accesses will still work! :-(
	mtc0	zero,C0_DBASE
	mtc0	zero,C0_DBOUND

	/* Set SR.  But leave DE set so that we won't get cache
	 * errors.
	 */
	li	k0,(SR_CU1|SR_CU0|SR_FR|SR_BEV|SR_DE)
	mtc0	k0,C0_SR

	mthi	zero			# this is really just for SIM--
	mtlo	zero			# it gets the X's out of HI/LO.

	SETLEDS(PROGRESS_INIT_SDRAM)

	/*******************************************************
	 * Initialize SDRAM
	 *******************************************************/
	
	li	t1,PHYS_TO_K1(LINC_BUFMEM_CONTROL)	# set BUFMEM_CONTROL
	li	t0,BUFMEM_CTL_VAL			# (see lincprom.h)
	sw	t0,0(t1)

	li	a0,LINC_BMO_DO_PRECH		# pre-charge
	jal	bufmemop
	 nop
	
	li	a0,(LINC_BMO_MODE_SET | BUFMEM_OPMODE)
	jal	bufmemop			# set op mode
	 nop

	li	a0,LINC_BMO_DO_REF
	jal	bufmemop			# refresh once
	 nop

	li	a0,LINC_BMO_DO_REF
	jal	bufmemop			# refresh twice
	 nop

	SETLEDS(PROGRESS_INIT_CACHES)

#if !defined(SIM) && !defined(SABLE)

	/******************************************************
	 * Initialize Caches
	 *	This is a matter of invalidating the both caches
	 *	and then filling them with data so that there
	 *	will be correct parity everywhere.  Then the DE
	 *	bit in the SR can be cleared to enable future parity
	 *	errors in the caches.
	 ******************************************************/
	
	jal	nuke_dcache		/* totally invalidate caches */
	 nop

	jal	nuke_icache
	 nop

#if DCACHE_SIZE >= ICACHE_SIZE
	li	t3,DCACHE_SIZE		/* t1 = larger of cache sizes */
#else
	li	t3,ICACHE_SIZE
#endif
	move	t2,t3
	li	a0,PHYS_TO_K1( LINC_SDRAM_ADDR )
	la	a1,__start
1:
	lw	t0,0(a1)		/* Copy LINCPROM to memory as */
	addiu	a1,a1,4			/* part of initialization. */
	addiu	t2,t2,-4
	sw	t0,0(a0)
	bgtz	t2,1b
	 addiu	a0,a0,4

	move	t2,t3
	li	a0,PHYS_TO_K0( LINC_SDRAM_ADDR )
	la	a1,__start
2:
	lw	t0,0(a0)		/* Now do compare test using cache */
	addiu	t2,t2,-4		/* dcache.  This also initializes */
	lw	t1,0(a1)		/* the dcache. */
	bne	t0,t1,badmem
	addiu	a1,a1,4
	bgtz	t2,2b
	 addiu	a0,a0,4

	li	a0,PHYS_TO_K0( LINC_SDRAM_ADDR )	/* Fill I-cache */
	li	t3,ICACHE_SIZE
3:
	cache	(CACH_PI|C_FILL), 0(a0)
	addiu	t3,t3,-ICACHE_LINESIZE
	bgtz	t3,3b
	 addiu	a0,a0,ICACHE_LINESIZE

	li	k0,(SR_CU1|SR_CU0|SR_FR|SR_BEV)		/* Clear DE bit */
	mtc0	k0,C0_SR

	SETLEDS(PROGRESS_TEST_SDRAM)


	/******************************************************
	 * Test memory                                        *
	 ******************************************************/
	la	t0,memtest
	la	t1,__start
	subu	t0,t0,t1
	li	t2,PHYS_TO_K0( LINC_SDRAM_ADDR )
	addu	t0,t0,t2

	li	a0,PHYS_TO_K1( LINC_SDRAM_ADDR )
	li	a1,SDRAM_SIZE

	jalr	t0
	 nop
	beq	v0,zero,1f
	 move	a1,v0

badmem:				
	jal	bevdie
	 li	a0,DIE_SDRAMBAD
1:

#endif /* !SIM && !SABLE */

	SETLEDS(PROGRESS_MEMTEST_DONE)

	/* Set up temporary mailbox space.
	 */
	li	t1,PHYS_TO_K1(LINC_MAILBOX_BASE_ADDRESS)
	li	t0,(LINC_SDRAM_ADDR+SDRAM_SIZE-0x100)
	sw	t0,0(t1)

	/* Now that we have memory, we can set up a stack
	 * and call C routines.
	 */
	la	sp,LPROM_STACK

	jal	prommain
	 nop
	
	.end	reset

/*
 * bevexception(void);
 *
 * Exception handler while still in boot mode.
 */
	.globl	bevexception
	.ent	bevexception,0
	.set	noat
bevexception:

	mfc0	a1,C0_CAUSE
	nop

	mfc0	a2,C0_EPC
	nop

#ifdef SIM
	li	k1,PHYS_TO_K1(LINC_BYTEBUS_ADDR)
	sw	a1,0(k0)
	li	k1,PHYS_TO_K1(LINC_BYTEBUS_ADDR)
	sw	a2,0(k0)
#endif


	jal	bevdie
	 li	a0,DIE_BADEXCEPT
	
	.end	bevexception
	.set	at


/*
 * void
 * bevdumpcache(void);
 */
LEAF(bevdumpcache)

	li	t0,PHYS_TO_K1(LINC_SDRAM_ADDR+0x400)

	li	t3,ICACHE_INDEXES
	li	t1,K0BASE
1:
	cache	CACH_PI | C_ILT, 0(t1)
	addiu	t1,t1,ICACHE_LINESIZE
	mfc0	t4,C0_TAGLO
	addiu	t3,t3,-1
	sw	t4,0(t0)
	bgtz	t3,1b
	 addiu	t0,t0,4
	
	li	t3,ICACHE_INDEXES
	li	t1,K0BASE+CACHEB
1:
	cache	CACH_PI | C_ILT, 0(t1)
	addiu	t1,t1,ICACHE_LINESIZE
	mfc0	t4,C0_TAGLO
	addiu	t3,t3,-1
	sw	t4,0(t0)
	bgtz	t3,1b
	 addiu	t0,t0,4
	
	li	t3,DCACHE_INDEXES
	li	t1,K0BASE
1:
	cache	CACH_PD | C_ILT, 0(t1)
	addiu	t1,t1,DCACHE_LINESIZE
	mfc0	t4,C0_TAGLO
	addiu	t3,t3,-1
	sw	t4,0(t0)
	bgtz	t3,1b
	 addiu	t0,t0,4
	
	li	t3,DCACHE_INDEXES
	li	t1,K0BASE+CACHEB
1:
	cache	CACH_PD | C_ILT, 0(t1)
	addiu	t1,t1,DCACHE_LINESIZE
	mfc0	t4,C0_TAGLO
	addiu	t3,t3,-1
	sw	t4,0(t0)
	bgtz	t3,1b
	 addiu	t0,t0,4

	jr	ra
	 nop

END(bevdumpcache)


/*
 * void
 * bevcacheerr(void);
 *
 */
	.globl	bevcacheerr
	.ent	bevcacheerr,0
	.set	noat
bevcacheerr:

	mfc0	a1,C0_CACHE_ERR
	nop

	mfc0	a2,C0_ERROR_EPC
	nop

	jal	bevdumpcache
	 nop

#ifdef SIM
	li	k1,PHYS_TO_K1(LINC_BYTEBUS_ADDR)
	sw	a1,0(k0)
	li	k1,PHYS_TO_K1(LINC_BYTEBUS_ADDR)
	sw	a2,0(k0)
#endif

	jal	bevdie
	 li	a0,DIE_CACHEEXC
	
	.end	bevcacheerr
	.set	at


/*
 * bevdie( int code, int auxdata0, int auxdata1 );
 *
 * Die and (somehow) return error code.
 */
LEAF(bevdie)
#ifdef SIM
	li	t1,PHYS_TO_K1(LINC_SIMFAIL)
	sb	zero,0(t1)
#endif

	/* store auxdata into unused DMA registers so host can see them */
	li	t0,PHYS_TO_K1(LINC_IWDL)
	sw	a1,0(t0)
	li	t0,PHYS_TO_K1(LINC_IWDH)
	sw	a2,0(t0)

	/* Flash DIE code on LED<2>, leaving progress LED<1:0> alone. */
	li	t0,PHYS_TO_K1(LINC_LED)
	lw	t4,0(t0)		# load LED val into t4

1:
	/* reset leds, clear leds for a long time, set all error leds for a 
	 * long time, clear them, and then start winking error sequence.
	 */
	li	t1, 0xf
	sw	t1,0(t0)		# store LED = ~0x0
	mfc0	t1,C0_COUNT
	li	t2,(CPU_HZ/2)		# hold for a sec
	addu	t1,t1,t2
2:
	mfc0	t2,C0_COUNT
	subu	t2,t2,t1
	blez	t2,2b
	 nop

	li	t1, 0
	sw	t1,0(t0)		# store LED = 0x0
	mfc0	t1,C0_COUNT
	li	t2,(CPU_HZ)		# hold for a 2 sec
	addu	t1,t1,t2
3:
	mfc0	t2,C0_COUNT
	subu	t2,t2,t1
	blez	t2,3b
	 nop

	li	t1, 0xf
	sw	t1,0(t0)		# store LED = ~0x0
	mfc0	t1,C0_COUNT
	li	t2,(CPU_HZ/2)		# hold for a sec
	addu	t1,t1,t2
4:
	mfc0	t2,C0_COUNT
	subu	t2,t2,t1
	blez	t2,4b
	 nop

	# Start blinking error code	
	move	t3,a0
	sw	t4,0(t0)		# store old LED values
10:	
	lw	t1,0(t0)		# load LED val
	ori	t1,t1,0x04		# clear led<2>
	sw	t1,0(t0)		# store LED val

	mfc0	t1,C0_COUNT
	li	t2,(CPU_HZ/4)		# off for 500ms
	addu	t1,t1,t2
11:
	mfc0	t2,C0_COUNT
	subu	t2,t2,t1
	blez	t2,11b
	 nop

	lw	t1,0(t0)		# load LED val
	andi	t1,t1,0x0b		# set led<2>
	sw	t1,0(t0)		# store LED val

	mfc0	t1,C0_COUNT
	li	t2,(CPU_HZ/8)		# on for 250ms
	addu	t1,t1,t2
12:
	mfc0	t2,C0_COUNT
	subu	t2,t2,t1
	blez	t2,12b
	 nop

	lw	t1,0(t0)		# load LED val
	ori	t1,t1,0x04		# clear led<2>
	sw	t1,0(t0)		# store LED val

	mfc0	t1,C0_COUNT
	li	t2,(CPU_HZ/8)		# off for 250ms
	addu	t1,t1,t2
13:
	mfc0	t2,C0_COUNT
	subu	t2,t2,t1
	blez	t2,13b
	 nop

	addiu	t3,t3,-1
	bgtz	t3,10b
	 nop
		
	b	1b
	 nop
END(bevdie)

/* void bufmemop( int opmode );
 */
LEAF(bufmemop)
	li	t1,PHYS_TO_K1(LINC_BUFMEM_OPERATION)	# start operation
	sw	a0,0(t1)

	li	t2,(LINC_BMO_MODE_SET|LINC_BMO_DO_PRECH|LINC_BMO_DO_REF)

	li	t3,10000			# max number of polling loops
1:
	lw	t0,0(t1)
	and	t0,t0,t2
	beq	t0,zero,2f			# operation complete?
	 addi	t3,t3,-1
	bgez	t3,1b				# hit max polls?
	 nop
	
	move	a1,a0				# die
	jal	bevdie
	 li	a0,DIE_SDRAMBAD

2:	
	jr	ra
	 nop
END(bufmemop)


/* These are for the memory tests. */
#define MEMSEED		0x7a7a7a7a
#define MEMINCR		0x973ac26f

/*
 * int
 * memtest(char *addr, int len);
 *
 * Do a generic memory test on the region specified with addr/len.
 *
 * Returns 0 if the test passes, non-zero if it fails.
 *
 * This piece of code needs to be relocatable because it might be
 * copied to memory and executed out of SDRAM.
 *
 */

LEAF(memtest)

#if defined(SIM) || defined(SABLE)
	li	a1,0x80			/* shorten tests for simulation */
#endif

	addu	t1,a0,a1		/* last word (inclusive) to test */
	addiu	t1,t1,-4

	/* Write pattern to memory */
	li	t3,MEMSEED
	li	t4,MEMINCR
	move	v0,a0
1:
	sw	t3,0(v0)		/* store pattern */
	addu	t3,t3,t4
	bne	v0,t1,1b
	 addiu	v0,v0,4

	/* Check pattern in memory */
	li	t3,MEMSEED
	move	v0,a0
1:
	lw	t2,0(v0)
	bne	t2,t3,9f
	 addu	t3,t3,t4
	bne	v0,t1,1b
	 addiu	v0,v0,4
	
	/* Write complement pattern to memory */
	li	t3,(~MEMSEED)
	li	t4,-MEMINCR
	move	v0,a0
1:
	sw	t3,0(v0)		/* store pattern */
	addu	t3,t3,t4
	bne	v0,t1,1b
	 addiu	v0,v0,4

	/* Check complement pattern in memory */
	li	t3,(~MEMSEED)
	move	v0,a0
1:
	lw	t2,0(v0)
	bne	t2,t3,9f
	 addu	t3,t3,t4
	bne	v0,t1,1b
	 addiu	v0,v0,4


	li	v0,0			/* test passed. */
9:
	jr	ra
	 nop

END(memtest)

