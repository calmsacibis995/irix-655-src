/**************************************************************************
 *									  *
 * 		 Copyright (C) 1989-1993, Silicon Graphics, Inc.	  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ifndef __SYS_TRAPLOG_H__
#define __SYS_TRAPLOG_H__


#if TRAPLOG_DEBUG && (_PAGESZ == 16384)

/* The following definitions determine the size and location of the traplog
 * buffer.  It must be in the PDA page (so area is permanently mapped in tlb)
 * and hence must also be addressable as a 16-bit negative offset from zero.
 * Otherwise the TRAPLOG macro will not work correctly in locore & utlbmiss.
 */

#define	TRAPLOG_PTR		0xffffffffffffd000
#define	TRAPLOG_BUFSTART	0xffffffffffffd040
#define TRAPLOG_BUFEND		0xffffffffffffdfc0
#define	TRAPLOG_ENTRYSIZE	0x40

#define TRAPLOG(code)	\
	ld	k1,TRAPLOG_PTR; \
	beqz	k1, 1f;	\
	nop;		\
	DMFC0(k0, C0_EPC); \
	sd	k0, 0(k1); \
	DMFC0(k0, C0_BADVADDR); \
	sd	k0, 8(k1); \
	DMFC0(k0, C0_CAUSE); \
	sd	k0, 0x10(k1); \
	DMFC0(k0, C0_SR); \
	sd	k0, 0x18(k1); \
	sd	ra, 0x20(k1); \
	ld	k0, VPDA_CURUTHREAD(zero); \
	sd	k0, 0x28(k1); \
	sd	sp, 0x30(k1); \
	li	k0, code; \
	sd	k0, 0x38(k1); \
	daddiu	k1, TRAPLOG_ENTRYSIZE;	\
	LI	k0, TRAPLOG_BUFEND; \
	slt	k0, k1, k0; \
	bnez	k0, 2f; \
	nop; 		\
	LI	k1, TRAPLOG_BUFSTART; \
2:	sd	k1, TRAPLOG_PTR; \
1:

#ifndef TFP
#define TRAPLOG_UTLBMISS(code)	\
	sd	k1,TRAPLOG_PTR+24;	\
	ld	k1,TRAPLOG_PTR; \
	beqz	k1, 1f;	\
	sd	k0,TRAPLOG_PTR+16;	\
	DMFC0(k0, C0_EPC); \
	sd	k0, 0(k1); \
	DMFC0(k0, C0_BADVADDR); \
	sd	k0, 8(k1); \
	DMFC0(k0, C0_CAUSE); \
	sd	k0, 0x10(k1); \
	DMFC0(k0, C0_SR); \
	sd	k0, 0x18(k1); \
	sd	ra, 0x20(k1); \
	ld	k0, VPDA_CURUTHREAD(zero); \
	sd	k0, 0x28(k1); \
	sd	sp, 0x30(k1); \
	li	k0, code; \
	sd	k0, 0x38(k1); \
	daddiu	k1, TRAPLOG_ENTRYSIZE;	\
	LI	k0, TRAPLOG_BUFEND; \
	slt	k0, k1, k0; \
	bnez	k0, 2f; \
	nop; 		\
	LI	k1, TRAPLOG_BUFSTART; \
2:	sd	k1, TRAPLOG_PTR; \
	ld	k0, TRAPLOG_PTR+16; \
1:	ld	k1, TRAPLOG_PTR+24
	
#endif /* !TFP */

#else
#define TRAPLOG(code)
#define TRAPLOG_UTLBMISS(code)
#endif	/* TRAPLOG_DEBUG && (_PAGESZ == 16384) */


#ifdef _EXCEPTION_TIMER_CHECK

#define SETUP_TIMER_CHECK_DATA()		\
	.data					;\
EXPORT(exception_timer_save)			;\
	.repeat (((5*SZREG)+7)/8)		;\
	.dword		0			;\
	.endr					;\
EXPORT(exception_timer_eframe)			;\
	.repeat	(EF_SIZE/8)			;\
	.dword		0			;\
	.endr					;\
	.text

/* macro called at exception time */
#define DO_EXCEPTION_TIMER_CHECK()		\
	la	k0,exception_timer_save		;\
	sreg	AT,(0*SZREG)(k0)		;\
	sreg	k1,(1*SZREG)(k0)		;\
	sreg	t0,(2*SZREG)(k0)		;\
	MFC0(k1,C0_CAUSE)			;\
	NOP_0_4					;\
	sreg	k1,(3*SZREG)(k0)		;\
	MFC0(k1,C0_COUNT)			;\
	NOP_0_4					;\
	sreg	k1,(4*SZREG)(k0)		;\
	.set	at				;\
	la	t0,exception_timer_enable	;\
	lw	t0,0(t0)			;\
	beqz	t0,4f				;\
	nop					;\
	la	t0,exception_timer_bypass	;\
	lw	t0,0(t0)			;\
	beqz	t0,4f				;\
	nop					;\
	la	t0,r4k_compare_shadow		;\
	lw	t0,0(t0)			;\
	subu	k1,t0				;\
	subu	k1,200000000	/* 200 MHZ (about 1 second) */		;\
	bltz	k1,4f		/* --> not too late */			;\
	nop					;\
EXPORT(exception_timer_panic)			;\
	la	t0,exception_timer_bypass	;\
	sw	zero,0(t0)			;\
	la	k1,exception_timer_eframe	;\
	.set	noat				;\
	lreg	AT,(0*SZREG)(k0)		;\
	sreg	AT,EF_AT(k1)	# saved in ecc springboard		;\
	SAVEAREGS(k1)				;\
	SAVESREGS(k1)				;\
	sreg	gp,EF_GP(k1)			;\
	sreg	sp,EF_SP(k1)			;\
	sreg	ra,EF_RA(k1)			;\
	lreg	t0,(2*SZREG)(k0)		;\
						;\
	mfc0	a0,C0_SR			;\
	mfc0	a1,C0_CAUSE			;\
	DMFC0(a2,C0_EPC)			;\
	DMFC0(a3,C0_BADVADDR)			;\
	sreg	a0,EF_SR(k1)			;\
	sreg	a1,EF_CAUSE(k1)			;\
	sreg	a2,EF_EPC(k1)			;\
	sreg	a3,EF_BADVADDR(k1)		;\
						;\
	jal	tfi_save	# save tregs, v0,v1, mdlo,mdhi		;\
	nop					;\
						;\
	lreg	ra,EF_RA(k1)			;\
	lreg	k1,(0*SZREG)(k0)		;\
	.set	at				;\
	LA	a0,2f				;\
	j	stk1				;\
	nop					;\
	.data					;\
2:	.asciiz	"Exception loop at 0x%x sp:0x%x k1:0x%x\12\15"		;\
	.text					;\
	.set	noat				;\
4:						;\
	lreg	t0,(2*SZREG)(k0)		;\
	lreg	AT,(0*SZREG)(k0)

#endif /* _EXCEPTION_TIMER_CHECK */


#ifdef	R4000_LOG_EXC
#if (! SP) || (! R4000)
	<< error: R4000_LOG_EXC allowed only if SP R4000 or equivalent>>
#endif
	
#define	R4000_LOG_SIZE	256
#define	R4000_LOG_INC		16
#define R4000_EXC_LOG_ADDR 0xa0000300	

#define DO_R4000_LOG_EXC()						\
	ABS(exc_log,R4000_EXC_LOG_ADDR)					;\
									;\
	lui	k1, (R4000_EXC_LOG_ADDR >> 16)				;\
	ori	k1, (R4000_EXC_LOG_ADDR & 0xFFFF)			;\
	sreg	t0, 8(k1)		         			;\
	move	t0, k1							;\
	lw	k1, 0(t0)						;\
	nop								;\
	bne	k1, zero, 1f		                          	;\
	nop								;\
	addi	k1, t0, R4000_LOG_INC	                             	;\
	sw	k1, 0(t0)						;\
	nop								;\
1:	sw	k0, 0(k1)		                 		;\
	nop								;\
	mfc0	k0, C0_BADVADDR		                             	;\
	NOP_0_4								;\
	sw	k0, 4(k1)		                     		;\
	nop								;\
	mfc0	k0, C0_EPC						;\
	NOP_0_4								;\
	sw	k0, 8(k1)						;\
	sw	ra, 12(k1)						;\
	lw	k0, 0(k1)		                      		;\
									;\
	addi	k1, R4000_LOG_INC		           		;\
	sw	k1, 0(t0)						;\
	sub	k1, t0			                           	;\
	sub	k1, R4000_LOG_SIZE - R4000_LOG_INC			;\
	bltz	k1, 1f			                 		;\
	addi	k1, t0,R4000_LOG_INC					;\
	sw	k1, 0(t0)		                       		;\
	nop								;\
1:	lreg	t0, 8(t0)		             			;\
									;\
	MFC0(k0,C0_CAUSE)
#endif	/* R4000_LOG_EXC */

#endif	/* __SYS_TRAPLOG_H__ */
