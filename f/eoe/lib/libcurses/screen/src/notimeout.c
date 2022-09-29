/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/notimeout.c	1.7"
#include	"curses_inc.h"

int
notimeout(WINDOW *win, bool bf)
{
    win->_notimeout = (unsigned) (bf ? TRUE : FALSE);
    return (OK);
}
