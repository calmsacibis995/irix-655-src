/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: /hosts/bonnie.mti/depot/cmplrs.src/v7.2+/include/RCS/alloca.h,v 7.9 1993/07/17 01:51:32 rdahl Exp $ */
#ifndef __ALLOCA_H
#define __ALLOCA_H

#ifdef __cplusplus
extern "C" {
#endif

/*
** Synopsis
**   #include <alloca.h>
**   void *alloca(unsigned int);
**
** Description
** alloca() is a built-in routine which allocates space in the
** local stack frame.
**
** alloca() is faster than malloc(3), but not as portable, since not all
** systems have an alloca() routine..
**
** There is no library routine alloca(), only the compiler built-in
** accessed using this header.  Consequently, one *must* use this
** header file to access alloca().
**
** Library versions of alloca() (such routines use malloc() internally)
** are available (in source) at no charge from various sources on USENET
** and the Internet.
** 
** Unlike malloc(), one never needs to free(3) space allocated with
** alloca().  Indeed, one must *not* free() space allocated by
** alloca().   Space allocated by alloca() is automatically
** freed when the function returns.
*/

void *alloca(unsigned int);
#define alloca  __builtin_alloca

#ifdef __cplusplus
}
#endif

#endif

