/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */
/*  shlib.h 1.1 */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * This header file contains all the macros and definitons
 *  needed for importing symbols for libc_s
 * 
 */
#if SHLIB

#ifdef __cplusplus
extern "C" {
#endif

#define _ctype	(* _libc__ctype)

#define _getflthw	(* _libc__getflthw)

#define _cleanup (* _libc__cleanup)
#define environ  (* _libc_environ)		
#define end	 (* _libc_end)

#define malloc	(* _libc_malloc)		
#define free	(* _libc_free)	
#define realloc (* _libc_realloc)


#define _sibuf  (* _libc__sibuf)
#define _sobuf  (* _libc__sobuf)
#define _smbuf  (* _libc__smbuf)
#define _iob 	(* _libc__iob)
#define _lastbuf	(* _libc__lastbuf)
#define _bufendtab	(* _libc__bufendtab)

#ifdef __cplusplus
}
#endif

#endif
