#ifndef __VARARGS_H__
#define __VARARGS_H__
#ifdef __cplusplus
extern "C" {
#endif
/* Copyright (C) 1989 Silicon Graphics, Inc. All rights reserved.  */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "$Revision: 7.9 $"

/* ANSI C Notes:
 *
 *  IN GENERAL, THE ANSI FUNCTIONALITY IMPLEMENTED IN stdarg.h SHOULD
 *  BE USED IN PREFERENCE TO THIS HEADER, WHICH IS OBSOLETE.  In
 *  particular, ANSI prototypes provide better consistency checking in
 *  the compiler.
 *
 *  This non-standard header file has identifiers that collide with the
 *  ones in <stdarg.h>.
 *
 *  gb - went back to rev. 1.10 of varargs.h for MIPS 3.00 release, as
 *  	 we have implemented __builtin_alignof in all current revisions
 *	 (which bypasses the need for <stamp.h>),
 *	 and our version is easier to read.
 *	 -- minor changes for __lint, etc.
 *
 * - The variable arguments are accessed in sequence using the
 *   va_arg macro, specifying the assumed argument type as the second
 *   argument, which yields a value of that type:
 *		va_arg ( vp, mode )
 *   Because of parameter passing conventions in C:
 *	use mode=int for char, and short types
 *	use mode=double for float types
 *	use a pointer for array types
 *
 * The compiler built-in symbol _VA_INIT_STATE:
 *	-returns 1 if the va_alist marker is the first
 *		parameter in the parameter list, or
 *	-returns 2 if the va_alist marker is the second
 *		parameter in the parameter list, and the
 *		first parameter has type double, or
 *	-returns 0 otherwise.
 *
 * The compiler built-in function __builtin_classof(type):
 *	-returns 0 if type is integer or pointer
 *	-returns 1 if floating point
 *	-returns 2 if aggregate
 */
#include <sgidefs.h>
#define _INT 0
#define _FP  1
#define _STRUCT 2

/* Define the va_list type.  Note that many of the implementing
 * expressions in this file depend on it being a char *, since they
 * do arithmetic on it assuming no scaling of the other operands.
 * Changes would therefore require changes in the implementations.
 */
#ifndef _VA_LIST_
#define _VA_LIST_
typedef char *va_list;
#endif /* !_VA_LIST_ */

#if (_MIPS_SIM == _MIPS_SIM_NABI32)
#define va_dcl 	long long va_alist;
#else
#define va_dcl 	long va_alist;
#endif
#define va_end(__list)

#ifndef __lint

#if defined(_COMPILER_VERSION) && (_COMPILER_VERSION>=400) /* Ragnarok */

/* Identify the register size: */
#if (_MIPS_SIM != _MIPS_SIM_ABI32)
# define __VA_REGBYTES	8
  /* Scalar parameters smaller than register size are right-justified
   * for big-endian targets.  Observe that variable FP parameters are
   * always at least doubles (hence register-sized), and struct parms
   * are not right-justified.
   */
# ifdef _MIPSEB /* big-endian */
#   define __VA_PADJUST(mode)	               \
      (__NO_CFOLD_WARNING(                     \
         ((__builtin_classof(mode) == _INT) && \
	  (sizeof(mode) < __VA_REGBYTES))      \
	    ? __VA_REGBYTES-sizeof(mode) : 0 ))
# else /* ! big-endian */
#   define __VA_PADJUST(mode)	0
# endif /* ! big-endian */
#else /* _MIPS_SIM == _MIPS_SIM_ABI32 */
# define __VA_REGBYTES	4
  /* For a 32-bit target with sizeof(int) = register size, no
   * right-justification is ever required:
   */
# define __VA_PADJUST(mode)	0
#endif

/* Define a pointer-sized integer type for casting use: */
typedef unsigned long __va_iptr_t;

/* For the 64-bit ABI, we don't support leading floats without
 * prototypes, but for the original 32-bit ABI we need to, which we
 * do by adjusting the start address of the va_list using the
 * the compiler built-in function _VA_INIT_STATE described above:
 */
#if (_MIPS_SIM != _MIPS_SIM_ABI32)
# define __VA_SADJUST	0
#else /* _MIPS_SIM == _MIPS_SIM_ABI32 */
# define __VA_SADJUST	_VA_INIT_STATE
#endif

/* va_start makes vp point past the parmN: */
#define va_start(vp) (vp = ((va_list)&va_alist) - __VA_SADJUST)

/* Parameter alignment building blocks: */
/* Save area alignment for a parameter of type 'mode': */
#define __VA_MALIGN(mode) \
    (__NO_CFOLD_WARNING(  \
       (__builtin_alignof(mode) > __VA_REGBYTES)  \
	  ? (__va_iptr_t)__builtin_alignof(mode) \
	  : (__va_iptr_t)__VA_REGBYTES ))
/* 'p' aligned for parameter type 'mode': */
#define	__VA_PALIGN(p,mode)	\
  ( ( ((__va_iptr_t)p)+(__VA_MALIGN(mode)-1) ) & (-__VA_MALIGN(mode)) )

/* __VA_STACK_ARG is the default implementation of va_arg, for
 * parameters which must be on the stack or are beyond the point where
 * we need to worry about leading doubles in FP registers, including
 * all 64-bit compilations.  It aligns for the mode (minimum of
 * register size) and then does right-justification if required:
 */
#define __VA_STACK_ARG(vp,mode)	( vp = (va_list) \
	(__VA_PALIGN(vp,mode)+__VA_PADJUST(mode)+sizeof(mode)) )

#if (_MIPS_SIM != _MIPS_SIM_ABI32)
  /* For 64-bit programs, we make no effort to cope specially with
   * leading FP parameters.  Variable FP parameters are not supported
   * without ANSI prototypes.
   */
# define va_arg(vp,mode) ((mode *)(void *)__VA_STACK_ARG(vp,mode))[-1]

#else /* _MIPS_SIM == _MIPS_SIM_ABI32 */
  /* __VA_DOUBLE_ARG checks the status in the lower-order 2 bits
   * of the "list" pointer, and correctly extracts arguments with
   * type double either from the arguements stack, or from the
   * floating point argument register spill area.
   */
# define __VA_DOUBLE_ARG(vp,mode) ( \
    (((__va_iptr_t)vp & 0x1) /* 1 byte aligned? */ \
      ? ((vp = ((va_list)vp + 7)),((va_list)vp-6))\
      : (((__va_iptr_t)vp & 0x2) /* 2 byte aligned? */ \
	  ? ((vp = ((va_list)vp +10)),((va_list)vp-24)) \
	  : __VA_STACK_ARG(vp,mode) )))

# define va_arg(vp,mode) ((mode*)(void *)(  \
	(__NO_CFOLD_WARNING(                \
	   (__builtin_classof(mode)==_FP && \
	    __builtin_alignof(mode)==sizeof(double)) \
	      ? __VA_DOUBLE_ARG(vp,mode) \
	      : __VA_STACK_ARG(vp,mode))))) [-1]
#endif

#else /* ! Ragnarok */
#ifdef _CFE

#define va_start(list) list = (char *) &va_alist - _VA_INIT_STATE

/* align p at least to 4-byte alignment, or a if a is larger */
#define _VA_ALIGN(p,a) (((unsigned int)((p)+((a)>4?(a):4)-1)) & -((a)>4?(a):4))

/*
** "va_stack_arg" is the old MIPS va_arg, which we fall back
** on when we're dealing with arguments on the stack.
*/
#define __va_stack_arg(list,mode)\
(\
	((list)=(char *)_VA_ALIGN(list,__builtin_alignof(mode))+ \
		_VA_ALIGN(sizeof(mode),4)), \
 	((list) - (_VA_ALIGN(sizeof(mode),4) - sizeof(mode))) \
)

/*
** "_va_double_arg" checks the status in the lower-order 2 bits
** of the "list" pointer, and correctly extracts arguments with
** type double either from the arguements stack, or from the
** floating point argument register spill area.
*/
#define __va_double_arg(list,mode) (\
   (((long)list & 0x1) /* 1 byte aligned? */ \
   ?(list = (char *)((long)list + 7),(char *)((long)list-6-_VA_FP_SAVE_AREA))\
     :(((long)list & 0x2) /* 2 byte aligned? */ \
      ?(list = (char *)((long)list +10),(char *)((long)list-24-_VA_FP_SAVE_AREA))  :__va_stack_arg(list,mode) )))

#define va_arg(list,mode) ((mode*)(\
	((__builtin_classof(mode)==_FP &&		     \
	  __builtin_alignof(mode)==sizeof(double)) \
				   ? __va_double_arg(list,mode)\
	 			   : __va_stack_arg(list,mode))))[-1]


#else /* !_CFE */

#define va_start(__list) __list = (char *) &va_alist
#define va_arg(__list, __mode) ((__mode *)(__list = (char *) \
  ( \
    (__builtin_alignof(__mode) == 8) ? \
     ((long)((long)__list + sizeof(__mode) + 8 - 1) & -8L ) : \
     ((long)((long)__list + ((sizeof(__mode)>4)?sizeof(__mode):4) + 4 - 1) & -4L) \
  ) \
  )) [-1]


#endif /* !_CFE */

#endif /* ! Ragnarok */

#else /* __lint */
#define va_start(__list) __list = (char *) &va_alist
#define va_arg(list, mode) ((mode *)(list += sizeof(mode)))[-1]
#endif

#ifdef __cplusplus
}
#endif
#endif /* !__VARARGS_H__ */



