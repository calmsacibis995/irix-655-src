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
#ident "$Id: uthread_noncell.c,v 1.1 1999/05/14 20:13:13 lord Exp $"

/*
 * Contains non-cell versions of routines where there are cell and non-cell
 * specific routines
 */

#include <sys/types.h>
#include <sys/proc.h>
#include <sys/debug.h>

char
get_current_abi()
{
	return(curuthread ? curuthread->ut_pproxy->prxy_abi : 0);
}

uthreadid_t
get_current_utid()
{
	return(0);
}

void
get_current_flid(flid_t *flidp)
{
	ASSERT(curuthread);
	*flidp = curuthread->ut_flid;
}
