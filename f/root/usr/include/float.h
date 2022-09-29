#ifndef __FLOAT_H__
#define __FLOAT_H__

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 *									  *
 * 		 Copyright (C) 1984,1989 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/* --------------------------------------------------- */
/* | Copyright (c) 1986 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                            | */
/* --------------------------------------------------- */
/* $Revision: 7.9 $ $Date: 1994/07/31 11:40:08 $                              */

/* ANSI C Notes:
 *
 * - THE IDENTIFIERS APPEARING OUTSIDE OF #ifdef __EXTENSIONS__ IN THIS
 *   standard header ARE SPECIFIED BY ANSI!  CONFORMANCE WILL BE ALTERED
 *   IF ANY NEW IDENTIFIERS ARE ADDED TO THIS AREA UNLESS THEY ARE IN ANSI's
 *   RESERVED NAMESPACE. (i.e., unless they are prefixed by __[a-z] or
 *   _[A-Z].  For external objects, identifiers with the prefix _[a-z] 
 *   are also reserved.)
 *
 * - This file specifies the characteristics of floating types
 *   as required by the proposed ANSI C standard.
 *   The following parameters must be defined in this file, and each 
 *   must have the minimum value indicated in the commentary.  
 *
 *    Parameters defined:  
 *		{FLT,DBL,LDBL}_{DIG, MANT_DIG, 
 *				MIN, MIN_EXP, MIN_10_EXP, 
 *				MAX, MAX_EXP, MAX_10_EXP,
 *				EPSILON }
 *		FLT_RADIX, FLT_ROUNDS
 *	
 *   No additional parameters may be defined outside of __EXTENSIONS__. 
 *				
 */

/*
 * Note: For now, all constants that refer to long doubles (LDBL) are
 * filled in with values for DBL.
 */

/* ANSI - the RADIX must be >= 2 */
#define	FLT_RADIX	2

/* ANSI - no specification is given for FLT_ROUNDS.  The
	meanings of the values are: (-1) = indeterminate; 0 = toward zero;
	1 = to nearest; 2 = toward +inf; 3 = toward -inf. */
#define	FLT_ROUNDS	1

/* ANSI - no specification is made about the number of mantissa digits */
#define	FLT_MANT_DIG	24
#define	DBL_MANT_DIG	53
#if (defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400))
#define	LDBL_MANT_DIG	107
#else
#define LDBL_MANT_DIG   DBL_MANT_DIG
#endif /* (defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400)) */

/* ANSI - FLT_EPSILON must be >= 1E-5; {LDBL,DBL}_EPSILON must be <= 1E-9 */
#ifdef __STDC__
#define	FLT_EPSILON	1.19209290E-07F
#define	DBL_EPSILON	2.2204460492503131E-16
#if (defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400))
#define	LDBL_EPSILON	1.232595164407830945955825883254353E-32L
#else
#define	LDBL_EPSILON	DBL_EPSILON
#endif /* defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400) */
#else
#define	FLT_EPSILON	1.19209290E-07
#define	DBL_EPSILON	2.2204460492503131E-16
#if (defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400))
#define	LDBL_EPSILON	1.232595164407830945955825883254353E-32
#else
#define	LDBL_EPSILON	DBL_EPSILON
#endif /* defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400) */
#endif

/* ANSI - FLT_DIG must be >= 6; others >= 10 */
#define FLT_DIG		6
#define DBL_DIG		15
#if (defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400))
#define LDBL_DIG 	31
#else
#define LDBL_DIG 	DBL_DIG
#endif /* (defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400)) */

/* ANSI - No specification is given for {FLT,DBL,LDBL}_MIN_EXP, 
	but they are obviously governed by the corresponding _MIN_10_EXP. */
#define FLT_MIN_EXP	-125
#define DBL_MIN_EXP	-1021
#define LDBL_MIN_EXP	DBL_MIN_EXP

/* ANSI - {FLT,DBL,LDBL}_MIN must be <= 1e-37 */
#ifdef __STDC__
#define FLT_MIN		1.17549435E-38F
#define DBL_MIN		2.2250738585072014E-308
#if (defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400))
#define LDBL_MIN	2.225073858507201383090232717332404E-308L
#else
#define LDBL_MIN	DBL_MIN
#endif /* defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400) */
#else
#define FLT_MIN		1.17549435E-38
#define DBL_MIN		2.2250738585072014E-308
#if (defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400))
#define LDBL_MIN	2.225073858507201383090232717332404E-308
#else
#define LDBL_MIN	DBL_MIN
#endif /* defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400) */
#endif

/* ANSI - {FLT,DBL,LDBL}_MIN_10_EXP must be <= (-37) */
#define FLT_MIN_10_EXP	-37
#define DBL_MIN_10_EXP	-307
#define LDBL_MIN_10_EXP	DBL_MIN_10_EXP

/* ANSI - No specification is given for {FLT,DBL,LDBL}_MAX_EXP, 
	but they are obviously governed by the corresponding _MAX_10_EXP. */
#define	FLT_MAX_EXP	128
#define	DBL_MAX_EXP	1024
#define	LDBL_MAX_EXP	DBL_MAX_EXP

/* ANSI - {FLT,DBL,LDBL}_MAX must be >= 1E37 */
#ifdef __STDC__
#define FLT_MAX		3.40282347E+38F
#define DBL_MAX		1.7976931348623157E+308
#if (defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400))
#define LDBL_MAX	1.797693134862315807937289714053023E+308L
#else
#define LDBL_MAX	DBL_MAX
#endif /* defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400) */
#else
#define FLT_MAX		3.40282347E+38
#define DBL_MAX		1.7976931348623157E+308
#if (defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400))
#define LDBL_MAX	1.797693134862315807937289714053023E+308
#else
#define LDBL_MAX	DBL_MAX
#endif /* defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 400) */
#endif

/* ANSI - {FLT,DBL,LDBL}_MAX_10_EXP must be >= 37 */
#define FLT_MAX_10_EXP	38
#define DBL_MAX_10_EXP	308
#define LDBL_MAX_10_EXP	DBL_MAX_10_EXP

#ifdef __cplusplus
}
#endif

#endif /* !__FLOAT_H__ */
