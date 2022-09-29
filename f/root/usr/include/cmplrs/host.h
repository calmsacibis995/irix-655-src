/*
 * Copyright 1992 Silicon Graphics,  Inc.
 * ALL RIGHTS RESERVED
 * 
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF SGI
 * The copyright notice above does not evidence any  actual  or
 * intended  publication of this source code and material is an
 * unpublished work by Silicon  Graphics,  Inc.  This  material
 * contains CONFIDENTIAL INFORMATION that is the property and a
 * trade secret of Silicon Graphics, Inc. Any use,  duplication
 * or  disclosure  not  specifically  authorized  in writing by
 * Silicon Graphics is  strictly  prohibited.  THE  RECEIPT  OR
 * POSSESSION  OF  THIS SOURCE CODE AND/OR INFORMATION DOES NOT
 * CONVEY ANY RIGHTS TO REPRODUCE, DISCLOSE OR  DISTRIBUTE  ITS
 * CONTENTS,  OR  TO MANUFACTURE, USE, OR SELL ANYTHING THAT IT
 * MAY DESCRIBE, IN WHOLE OR IN PART.
 * 
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND
 * Use, duplication or disclosure by the Government is  subject
 * to  restrictions  as  set  forth  in  FAR 52.227.19(c)(2) or
 * subparagraph (c)(1)(ii) of the Rights in Technical Data  and
 * Computer  Software  clause  at  DFARS 252.227-7013 and/or in
 * similar or successor clauses in the FAR, or the DOD or  NASA
 * FAR  Supplement.  Unpublished  --  rights reserved under the
 * Copyright Laws of the United States. Contractor/manufacturer
 * is Silicon Graphics, Inc., 2011 N. Shoreline Blvd., Mountain
 * View, CA 94039-7311
 */
/*
 * |-----------------------------------------------------------|
 * | Copyright (c) 1991 MIPS Computer Systems, Inc.            |
 * | All Rights Reserved                                       |
 * |-----------------------------------------------------------|
 * |          Restricted Rights Legend                         |
 * | Use, duplication, or disclosure by the Government is      |
 * | subject to restrictions as set forth in                   |
 * | subparagraph (c)(1)(ii) of the Rights in Technical        |
 * | Data and Computer Software Clause of DFARS 52.227-7013.   |
 * |         MIPS Computer Systems, Inc.                       |
 * |         950 DeGuigne Drive                                |
 * |         Sunnyvale, CA 94086                               |
 * |-----------------------------------------------------------|
 */
/* $Header: /hosts/bonnie.mti/depot/cmplrs.src/v7.2+/include/cmplrs/RCS/host.h,v 1.10 1995/03/06 23:18:45 yogesh Exp $ */

#ifndef _HOST_H
#define _HOST_H
/*
**  host.h
**
**  Basic type declarations, macros, ... to promote reuse and
**  portability throughout the compiler. 
**
**  Include this file before all others.
*/

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))

typedef int boolean;
#ifndef TRUE
#define TRUE	(1)
#define FALSE	(0)
#endif

typedef char *string;
typedef char char_t;
#ifndef __sgi
/* gb - sys/types.h defines these types, which is included by <signal.h> */
typedef unsigned char uchar_t;
#endif 
typedef signed short short_t;
#ifndef __sgi
typedef unsigned short ushort_t;
#endif
typedef signed int int_t;
#ifndef __sgi
typedef unsigned int uint_t;
#endif
typedef signed long long_t;
#ifndef __sgi
typedef unsigned long ulong_t;
#else
#include <sys/types.h>
#endif

#if defined(_LONGLONG)
typedef signed long long longlong_t;
typedef unsigned long long ulonglong_t;
#else
typedef signed long longlong_t;
typedef unsigned long ulonglong_t;
#endif

typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int16;
typedef unsigned short uint16;
typedef signed int int32;
typedef unsigned int uint32;
typedef longlong_t int64;
typedef ulonglong_t uint64;

typedef void *pointer;          /* a generic pointer type */
typedef double double_t;
typedef float float_t;
typedef int32 fsize_t; /* Size of a "hidden length" when passing Fortran CHARACTER arguments */
/* Another reasonable choice:  (requires <string.h>)
**    typedef size_t fsize_t;
*/


#endif

#if defined(_LANGUAGE_PASCAL)

#if defined(_LONGLONG)
type long_integer = integer64;
type long_cardinal = cardinal64;
#else
type long_integer = integer;
type long_cardinal = cardinal;
#endif
#endif

#endif

