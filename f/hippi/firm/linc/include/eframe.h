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
 * eframe.h
 *
 * Exception frames.  Each exception frame has an area to save the
 * context of the processor and some stack space.  These are used
 * for handling exceptions and interrupts.  The locations of registers
 * saved in the frame are matched up to correspond to register number
 * designations in gdb or kdbx.
 *
 * $Revision: 1.5 $
 */

#ifndef _EFRAME_H_
#define _EFRAME_H_

#define EFRAMESZ	4096	/* total of registers and stack */


#define REGSIZE		(_MIPS_SZLONG/8)


#ifdef GDB

/* GDB register definitions */
#define	R_R0 0
#define	R_F0 32
#define	R_EPC 64
#define	R_CAUSE 65
#define	R_BADVADDR 66
#define	R_MDHI 67
#define	R_MDLO 68
#define	R_C1_SR 69
#define	R_C1_EIR 70
#define NUM_REGS	71	/* number of regs returned by 'g' cmd */

/* XXX: gdb is weird.  it has no access to the status register.  But,
 * these registers must be saved in addition to the others.
 */
#define R_SR		71
#define	R_COUNT		72
#define	R_COMPARE	73
#define	R_ERREPC	74

#define MAX_REGS	75

#else

#define	R_R0 0
#define	R_F0 32
#define	R_EPC 64
#define	R_CAUSE 65
#define	R_MDHI 66
#define	R_MDLO 67
#define	R_C1_SR 68
#define	R_C1_EIR 69
#define	R_SR 70
#define	R_TLBHI 71
#define	R_TLBLO 72
#define	R_BADVADDR 73
#define	R_INX 74
#define	R_RAND 75
#define	R_CTXT 76
#define	R_EXCTYPE 77
#define	R_TLBLO1 78
#define	R_PGMSK 79
#define	R_WIRED 80
#define	R_COUNT 81
#define	R_COMPARE 82
#define	R_LLADDR 83
#define	R_WATCHLO 84
#define	R_WATCHHI 85
#define	R_ECC 86
#define	R_CACHERR 87
#define	R_TAGLO 88
#define	R_TAGHI 89
#define	R_ERREPC 90

#define NUM_REGS 91
#define MAX_REGS 91

#endif /* ! GDB */

#ifdef _LANGUAGE_ASSEMBLY
#if REGSIZE == 8
#define	sreg	sd
#define lreg	ld
#else
#define	sreg	sw
#define lreg	lw
#endif
#endif


#ifdef _LANGUAGE_C

#if REGSIZE == 8
typedef __uint64_t reg_t;
#else
typedef __uint32_t reg_t;
#endif

typedef struct eframe_s {
	reg_t		regs[ MAX_REGS ];
	__uint32_t excstack[ (EFRAMESZ-REGSIZE*MAX_REGS)/sizeof(__uint32_t) ];
} eframe_t;

#endif /* _LANGUAGE_C */
#endif /* _EFRAME_H_ */
