/**************************************************************************
 *									  *
 * 		 Copyright (C) 1994-1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ident "$Id: sgicell_noncell.c,v 1.1 1999/05/14 20:13:13 lord Exp $"

/*
 * Contains non-cell versions of routines where there are cell and non-cell
 * specific routines
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/syssgi.h>
/*
 * Non-cell sgicell
 */
/* ARGSUSED */
int
sgicell(struct syssgia *uap, rval_t *rvp)
{
	switch ((int)uap->arg1) {
	case SGI_IS_OS_CELLULAR:
		return 1;
	default:
		return ENOTSUP;
	}
}
