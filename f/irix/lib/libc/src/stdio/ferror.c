/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/ferror.c	1.2"
/*LINTLIBRARY*/

#ifdef __STDC__
#ifndef  _LIBC_ABI
	#pragma weak ferror_unlocked = ferror
	#pragma weak ferror_locked = ferror
#endif /* _LIBC_ABI */
#endif
#include "synonyms.h"
#include <stdio.h>

#undef ferror

int
ferror(iop)
	FILE *iop;
{
	return iop->_flag & _IOERR;
}
