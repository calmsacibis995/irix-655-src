/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/vidputs.c	1.10"
#include	"curses_inc.h"

int
#ifdef __STDC__
vidputs(chtype a, int (*b)(int))
#else
vidputs(a,b)
chtype	a;
int	(*b)();
#endif
{
    vidupdate(a,cur_term->sgr_mode,b);
    return (OK);
}
