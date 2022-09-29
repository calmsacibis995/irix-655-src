#ifndef __STDARG_H__
#define __STDARG_H__
#ifdef __cplusplus
extern "C" {
#endif
/* Copyright (C) 1987,1989 Silicon Graphics, Inc. All rights reserved.  */
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/* #ident "$Revision: 7.9 $" */

/* ANSI C Notes:
 *
 * - THE IDENTIFIERS APPEARING OUTSIDE OF #ifdef __EXTENSIONS__ IN THIS
 *   standard header ARE SPECIFIED BY ANSI!  CONFORMANCE WILL BE ALTERED
 *   IF ANY NEW IDENTIFIERS ARE ADDED TO THIS AREA UNLESS THEY ARE IN ANSI's
 *   RESERVED NAMESPACE. (i.e., unless they are prefixed by __[a-z] or
 *   _[A-Z].  For external objects, identifiers with the prefix _[a-z] 
 *   are also reserved.)
 *
 * - At each call to a function with a variable number of arguments,
 *   either the function definition or prototype must be visible.
 *
 *   NOTE: the 64-bit ABI requires use of the ANSI stdarg.h
 *   varargs mechanism if floating point parameters are passed in the
 *   variable part of the parameter list.  Failure to do so will result
 *   in incorrect code being generated.
 *
 * - A varargs function must declare a variable of type "va_list," which
 *   is the "vp" object appearing in the macros.  That object must be
 *   initialized with va_start in the varargs function, but may then
 *   be passed to another function for processing of the list.  vp may
 *   NOT be assumed to be assignable.
 *
 *   NOTE:  The va_list object passed into a stdarg.h function must NOT
 *   come from a pre-ANSI varargs.h function.  The respective
 *   mechanisms may initialize differently, e.g. for floating point
 *   parameters.
 *
 * - A varargs routine must have at least one named parameter.  The
 *   last named parameter, immediately preceding the '...', must be
 *   the second parameter of the va_start macro:
 *		va_start ( vp, parmN );
 *
 * - The variable arguments are then accessed in sequence using the
 *   va_arg macro, specifying the assumed argument type as the second
 *   argument, which yields a value of that type:
 *		va_arg ( vp, mode )
 *   Because of parameter passing conventions in C:
 *	use mode=int for char, and short types
 *	use mode=double for float types
 *	use a pointer for array types
 *
 * - Processing of a variable argument list should be concluded by
 *   invoking the va_end macro:
 *		va_end ( vp );
 */
#include <sgidefs.h>
#define _INT 0
#define _FP  1
#define _STRUCT 2

/* Define the va_list type: */
#ifndef _VA_LIST_
#define _VA_LIST_
typedef char *va_list;
#endif /* !_VA_LIST_ */

/* No cleanup processing is required for the end of a varargs list: */
#define va_end(__list)

#if defined(_COMPILER_VERSION) && (_COMPILER_VERSION>=400) /* Ragnarok */

/* Identify the register size: */
#if (_MIPS_SIM==_MIPS_SIM_NABI32 || _MIPS_SIM==_MIPS_SIM_ABI64)
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
#else /* _MIPS_SIM=_MIPS_SIM_ABI32 */
# define __VA_REGBYTES	4
  /* For a 32-bit target with sizeof(int) = register size, no
   * right-justification is ever required:
   */
# define __VA_PADJUST(mode)	0
#endif

/* Define a pointer-sized integer type for casting use: */
typedef unsigned long __va_iptr_t;

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

/* va_start makes vp point past the parmN: */
#define va_start(vp, parmN) (vp = ((va_list)&parmN + sizeof(parmN)))

/* va_arg aligns for the mode (with a minimum alignment of the
 * register size) and then does right-justification if required:
 */
#define va_arg(vp,mode)	((mode *)(void *)( vp = (va_list) \
	(__VA_PALIGN(vp,mode)+__VA_PADJUST(mode)+sizeof(mode)) )) [-1]

#else /* ! Ragnarok */

/*****  The rest of this file is here only for pre-4.00 compatibility *****/

#if defined(_CFE)
#if defined(__STDC__) && (__STDC__ != 0 )
	/* va_start makes list point past the parmN */
#define va_start(list, parmN) (list = ((char *)&parmN + sizeof(parmN)))
#else
#define va_start(list, name) (void) (list = (void *)((char *)&...))
#endif


/* align p at least to 4-byte alignment, or a if a is larger */
#define _VA_ALIGN(p,a) (((unsigned int)(((char *)p)+((a)>4?(a):4)-1)) & -((a)>4?(a):4))

/*
** "va_stack_arg" is the old MIPS va_arg, which we fall back
** on when we're dealing with arguments on the stack.
*/
#define __va_stack_arg(list,mode)\
(\
	((list)=(char *)_VA_ALIGN(list,__builtin_alignof(mode))+ \
		_VA_ALIGN(sizeof(mode),4)), \
 	(((char *)list) - (_VA_ALIGN(sizeof(mode),4) - sizeof(mode))) \
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

/* these with with both ANSI and traditional SGI C */
#define va_start(__list, __parmN) (__list = (char *) \
 ( \
  (__builtin_alignof(__parmN) == 8) ? \
   (((long)&__parmN + (long)sizeof(__parmN) + 8 - 1) & -8L ) : \
   (((long)&__parmN + (((long)sizeof(__parmN) > 4)?(long)sizeof(__parmN):4) + 4 - 1) & (unsigned long) -4L) \
  ) \
 )

#define va_arg(__list, __mode) ((__mode *)(__list = (char *) \
    ( \
      (__builtin_alignof(__mode) == 8) ? \
       (((long)__list + (long)sizeof(__mode) + 8 - 1) & -8 ) : \
     (((long)__list + (((long)sizeof(__mode) > 4)?sizeof(__mode):4) + 4 - 1)\
      & (unsigned long) -4) \
    ) \
    )) [-1]



#endif /* !_CFE */

#endif /* ! Ragnarok */

#ifdef __cplusplus
}
#endif
#endif /* !__STDARG_H__ */
