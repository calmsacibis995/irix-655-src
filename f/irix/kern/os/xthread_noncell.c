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
#ident "$Id: xthread_noncell.c,v 1.1 1999/05/14 20:13:13 lord Exp $"

/*
 * Contains non-cell versions of routines where there are cell and non-cell
 * specific routines
 */

#include <sys/types.h>
#include <sys/cred.h>
#include <ksys/xthread.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/systm.h>
#include <sys/kthread.h>

/*
 * Non-cell specific xthread routines
 */
/*
 * KTOPS for xthreads
 */
cred_t *
xthread_get_cred(kthread_t *kt)
{
	return KT_TO_XT(kt)->xt_cred;
}

void
xthread_set_cred(kthread_t *kt, cred_t *cr)
{
	ASSERT(cr->cr_ref > 0);
	KT_TO_XT(kt)->xt_cred = cr;
}
