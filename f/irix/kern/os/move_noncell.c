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
#ident "$Id: move_noncell.c,v 1.1 1999/05/14 20:13:13 lord Exp $"

/*
 * Contains non-cell versions of routines where there are cell and non-cell
 * specific routines
 */

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/uio.h>
/*
 * Non-cell uiomove support
 */
short
get_local_uio_segflg(uio_t *uiop)
{
	return(uiop->uio_segflg);
}

int
kcopyin(
	caddr_t	src,
	caddr_t	dst,
	size_t	len)
{
	bcopy(src, dst, len);

	return 0;
}

int
kcopyout(
	caddr_t	src,
	caddr_t	dst,
	size_t	len)
{
	bcopy(src, dst, len);

	return 0;
}
