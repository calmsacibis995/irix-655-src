/*
 * |-----------------------------------------------------------|
 * | Copyright (c) 1991, 1990 MIPS Computer Systems, Inc.      |
 * | All Rights Reserved                                       |
 * |-----------------------------------------------------------|
 * |          Restricted Rights Legend                         |
 * | Use, duplication, or disclosure by the Government is      |
 * | subject to restrictions as set forth in                   |
 * | subparagraph (c)(1)(ii) of the Rights in Technical        |
 * | Data and Computer Software Clause of DFARS 252.227-7013.  |
 * |         MIPS Computer Systems, Inc.                       |
 * |         950 DeGuigne Avenue                               |
 * |         Sunnyvale, California 94088-3650, USA             |
 * |-----------------------------------------------------------|
 */
/* $Header: /hosts/bonnie.mti/depot/cmplrs.src/v7.2+/include/RCS/ieeefp.h,v 1.7 1997/07/23 23:59:25 vegas Exp $ */
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef __IEEEFP_H__
#define __IEEEFP_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Floating point enviornment for machines that support
 * the IEEE 754 floating-point standard.  This file currently 
 * supports the 3B2 and 80*87 families.
 *
 * This header defines the following interfaces:
 *	1) Classes of floating point numbers
 *	2) Rounding Control
 *	3) Exception Control
 *	4) Exception Handling
 *	5) Utility Macros
 *	6) Full Exception Environment Control
 */

/* CLASSES of floating point numbers *************************
 * IEEE floating point values fall into 1 of the following 10
 * classes
 */
typedef	enum	fpclass_t {
	FP_SNAN = 0,	/* signaling NaN */
	FP_QNAN = 1,	/* quiet NaN */
	FP_NINF = 2,	/* negative infinity */
	FP_PINF = 3,	/* positive infinity */
	FP_NDENORM = 4, /* negative denormalized non-zero */
	FP_PDENORM = 5, /* positive denormalized non-zero */
	FP_NZERO = 6,	/* -0.0 */
	FP_PZERO = 7,   /* +0.0 */
	FP_NNORM = 8,	/* negative normalized non-zero */
	FP_PNORM = 9	/* positive normalized non-zero */
	} fpclass_t;

#if (defined(__STDC__) || defined(__SVR4__STDC))
extern fpclass_t fpclass(double);	/* get class of double value */
extern int	finite( double );
extern int	unordered( double, double );
#else
extern fpclass_t fpclass();	/* get class of double value */
#endif

#if (defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400))


extern fpclass_t fpclassl (long double);
extern int unorderedl (long double, long double);
extern int finitel(long double);

#if (defined(__EXTENSIONS__))

#define	qfpclass fpclassl
#define	qunordered unorderedl
#define qfinite	finitel

#endif

#endif

/* ROUNDING CONTROL ******************************************
 *
 * At all times, floating-point math is done using one of four
 * mutually-exclusive rounding modes.
 */

/*
 * NOTE: the values given are chosen to match those used by the
 * R*010 rounding mode field in the control word.
 */
typedef	enum	fp_rnd {
    FP_RN = 0,	/* round to nearest representable number, tie -> even */
    FP_RZ = 1,	/* round toward zero (truncate)			      */
    FP_RP = 2,  /* round toward plus infinity                         */
    FP_RM = 3   /* round toward minus infinity                        */
    } fp_rnd;


#if (defined(__STDC__) || defined(__SVR4__STDC))
extern fp_rnd   fpsetround(fp_rnd);     /* set rounding mode, return previous */
extern fp_rnd   fpgetround(void);       /* return current rounding mode       */

#else
extern fp_rnd	fpsetround();	/* set rounding mode, return previous */
extern fp_rnd	fpgetround();	/* return current rounding mode       */

#endif

/* EXCEPTION CONTROL *****************************************
 *
 */

#define	fp_except	int

#define	FP_DISABLE	0	/* exception will be ignored	*/
#define	FP_ENABLE	1	/* exception will cause SIGFPE	*/
#define	FP_CLEAR	0	/* exception has not occurred	*/
#define	FP_SET		1	/* exception has occurred	*/

/*
 * There are six floating point exceptions, which can be individually
 * ENABLED (== 1) or DISABLED (== 0).  When an exception occurs
 * (ENABLED or not), the fact is noted by changing an associated
 * "sticky bit" from CLEAR (==0) to SET (==1).
 *
 * NOTE: the bit positions in fp_except are chosen to match those of
 * the R*010 control word mask bits.
 */

/* an fp_except can have the following (not exclusive) values:  */
#define FP_X_IMP        0x01    /* imprecise (loss of precision)*/
#define FP_X_UFL        0x02    /* underflow exception          */
#define FP_X_OFL        0x04    /* overflow exception		*/
#define FP_X_DZ         0x08    /* divide-by-zero exception     */
#define FP_X_INV        0x10    /* invalid operation exception  */


#if (defined(__STDC__) || defined(__SVR4__STDC))
extern fp_except fpgetmask(void);               /* current exception mask       */
extern fp_except fpsetmask(fp_except);          /* set mask, return previous    */
extern fp_except fpgetsticky(void);             /* return logged exceptions     */
extern fp_except fpsetsticky(fp_except);        /* change logged exceptions     */

#else
extern fp_except fpgetmask();	/* current exception mask       */
extern fp_except fpsetmask();	/* set mask, return previous    */
extern fp_except fpgetsticky();	/* return logged exceptions     */
extern fp_except fpsetsticky();	/* change logged exceptions     */

#endif 

/* UTILITY MACROS ********************************************
 */

#if (defined(__STDC__) || defined(__SVR4__STDC))
extern int isnanf(float);               
extern int isnand(double);

#else
extern int isnand();
#define isnanf(x)	(((*(long *)&(x) & 0x7f800000L)==0x7f800000L)&& \
			 ((*(long *)&(x) & 0x007fffffL)!=0x00000000L) )
#endif

/* EXCEPTION HANDLING ****************************************
 *
 * When a signal handler catches an FPE, it will have a freshly initialized
 * coprocessor.  This allows signal handling routines to make use of
 * floating point arithmetic, if need be.  The previous state of the R*010
 * chip is available, however.
 *
 * If the handler was set via sigaction(), the new, SVR4, method should be
 * used: the third argument to the handler will be a pointer to a ucontext
 * structure (see sys/ucontext.h).  The uc_mcontext.fpregs member of the
 * ucontext structure holds the saved floating-point registers.  This can be
 * examined and/or modified.  By modifying it, the state of the coprocessor
 * can be changed upon return to the main task.
 */

/* details TBD */

/* The structure of the FP Control/Status register (FCR31) is  given by the
 * following struct.
 */
#ifdef _MIPSEB
struct _fcr {
	unsigned    res1:   8,  /* not unused                           */
                    cond:   1,  /* condition bit (result of compare)    */
                    res2:   5,  /* not unused                           */
                    unimp:  1,  /* unimplemented (or denorm, Nan, etc)  */
                    excp:   5,  /* exceptions                           */
                    enab:   5,  /* exception enables                    */
                    sticky: 5,  /* exception sticky bits                */
		    rnd:    2   /* rounding control field               */
	;
};
#endif /* _MIPSEB */
#ifdef _MIPSEL
struct _fcr {
 	unsigned    rnd:    2,  /* rounding control field               */
                    sticky: 5,  /* exception sticky bits                */
                    enab:   5,  /* exception enables                    */
                    excp:   5,  /* exceptions                           */
                    unimp:  1,  /* unimplemented (or denorm, Nan, etc)  */
                    res2:   5,  /* not unused                           */
                    cond:   1,  /* condition bit (result of compare)    */
		    res1:   8   /* not unused                           */
	;
};
#endif /* _MIPSEL */

#ifdef __cplusplus
}
#endif

#endif  /* __IEEEFP_H__ */
