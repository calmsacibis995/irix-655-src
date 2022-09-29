/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/tic_error.c	1.6"
/*********************************************************************
*                         COPYRIGHT NOTICE                           *
**********************************************************************
*        This software is copyright (C) 1982 by Pavel Curtis         *
*                                                                    *
*        Permission is granted to reproduce and distribute           *
*        this file by any means so long as no fee is charged         *
*        above a nominal handling fee and so long as this            *
*        notice is always included in the copies.                    *
*                                                                    *
*        Other rights are reserved except as explicitly granted      *
*        by written permission of the author.                        *
*                Pavel Curtis                                        *
*                Computer Science Dept.                              *
*                405 Upson Hall                                      *
*                Cornell University                                  *
*                Ithaca, NY 14853                                    *
*                                                                    *
*                Ph- (607) 256-4934                                  *
*                                                                    *
*                Pavel.Cornell@Udel-Relay   (ARPAnet)                *
*                decvax!cornell!pavel       (UUCPnet)                *
*********************************************************************/

/*
 *	tic_error.c -- Error message routines
 *
 *  $Log: tic_error.c,v $
 *  Revision 1.4  1998/09/18 19:47:22  sherwood
 *  SJIS feature for 6.5.2f
 *
 *  Revision 1.5  1998/09/16 02:31:51  rkm
 *  i18n message cleanup
 *
 *  Revision 1.4  1998/01/30 02:42:05  ktill
 *  merged HCL changes
 *
 *  Revision 1.3.18.1  1997/10/31 08:46:00  scm
 *  I18N changes for SJIS/BIG5 support.
 *
 * Revision 1.3.18.2  1997/10/04  19:21:46  rajkr
 * New Coding and Cataloguing Changes
 *
 * Revision 1.3  1993/09/09  00:02:55  igehy
 * Converted to 64-bit.
 *
 * Revision 1.2  1993/08/04  22:29:29  wtk
 * Fullwarn of libcurses
 *
 * Revision 1.1  1991/12/06  14:06:05  daveh
 *
 * Revision 2.1  82/10/25  14:45:31  pavel
 * Added Copyright Notice
 * 
 * Revision 2.0  82/10/24  15:16:32  pavel
 * Beta-one Test Release
 * 
 * Revision 1.3  82/08/23  22:29:31  pavel
 * The REAL Alpha-one Release Version
 * 
 * Revision 1.2  82/08/19  19:09:44  pavel
 * Alpha Test Release One
 * 
 * Revision 1.1  82/08/12  18:36:02  pavel
 * Initial revision
 * 
 *
 */

#include "compiler.h"
#include <stdlib.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

extern short term_names;
extern char *progname;
extern char *string_table;

#include <locale.h>
#include <nl_types.h>
#include <msgs/uxeoe.h>
extern nl_catd catd;

void
/* VARARGS1 */
#ifdef __STDC__
warning(char *fmt, ...)
#else
warning(va_alist)
va_dcl
#endif
{
#ifndef __STDC__
    register char *fmt;
#endif
    va_list args;

#ifdef __STDC__
    va_start(args, fmt);
#else
    va_start(args);
    fmt = va_arg(args, char *);
#endif

    fprintf (stderr, 
	     CATGETS(catd, _MSG_TIC_WARN_LINE), 
	     progname, 
	     curr_line);
    fprintf (stderr, 
	     CATGETS(catd, _MSG_TIC_TERMINAL),
	     string_table+term_names);
    vfprintf (stderr, fmt, args);
    fprintf (stderr, "\n");
    va_end(args);
}


void
/* VARARGS1 */
#ifdef __STDC__
err_abort(char *fmt, ...)
#else
err_abort(va_alist)
va_dcl
#endif
{
#ifndef __STDC__
    register char *fmt;
#endif
    va_list args;

#ifdef __STDC__
    va_start(args, fmt);
#else
    va_start(args);
    fmt = va_arg(args, char *);
#endif

    fprintf (stderr, CATGETS(catd, _MSG_TIC_LINE), 
		progname, 
		curr_line);
    fprintf (stderr, 
	     CATGETS(catd, _MSG_TIC_TERMINAL),
	     string_table+term_names);
    vfprintf (stderr, fmt, args);
    fprintf (stderr, "\n");
    va_end(args);
    exit(1);
}


void
/* VARARGS1 */
#ifdef __STDC__
syserr_abort(char *fmt, ...)
#else
syserr_abort(va_alist)
va_dcl
#endif
{
#ifndef __STDC__
    register char *fmt;
#endif
    va_list args;

#ifdef __STDC__
    va_start(args, fmt);
#else
    va_start(args);
    fmt = va_arg(args, char *);
#endif

    fprintf (stderr, CATGETS(catd, _MSG_TIC_PROG_ERR), curr_line);
    fprintf (stderr, 
	     CATGETS(catd, _MSG_TIC_TERMINAL),
	     string_table+term_names);
    vfprintf (stderr, fmt, args);
    fprintf (stderr, "\n");
    fprintf(stderr, CATGETS(catd, _MSG_TIC_CORRUPT_FILE));
    va_end(args);
    exit(1);
}
