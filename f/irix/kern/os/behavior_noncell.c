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
#ident "$Id: behavior_noncell.c,v 1.1 1999/05/14 20:13:13 lord Exp $"

/*
 * Contains non-cell versions of routines where there are cell and non-cell
 * specific routines
 */

#include <sys/types.h>
#include <sys/cred.h>
#include <ksys/xthread.h>
#include <sys/cpumask.h>
#include <ksys/behavior.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/debug.h>
/*
 * Non-cell specific behavior routines
 */
/*
 * Global initialization function called out of main.
 */
void
bhv_global_lp_init()
{
	bhv_global_init();
}

#define BHVMAGIC	((bhv_head_lock_t*)0xf00dLL)

/* ARGSUSED */
void
bhv_head_init(
	bhv_head_t *bhp,
        char *name)
{
	bhp->bh_first = NULL;
#if defined(CELL_CAPABLE)
	bhp->bh_lockp = BHVMAGIC;
#endif
}

void
bhv_head_reinit(
	bhv_head_t *bhp)
{
	ASSERT(bhp->bh_first == NULL);
#if defined(CELL_CAPABLE)
	ASSERT(bhp->bh_lockp == BHVMAGIC);
#endif
}

void
bhv_insert_initial(
	bhv_head_t *bhp,
	bhv_desc_t *bdp)
{
	ASSERT(bhp->bh_first == NULL);
#if defined(CELL_CAPABLE)
	ASSERT(bhp->bh_lockp == BHVMAGIC);
#endif
	(bhp)->bh_first = bdp;
}

void
bhv_head_destroy(
	bhv_head_t *bhp)
{
	ASSERT(bhp->bh_first == NULL);
#if defined(CELL_CAPABLE)
	ASSERT(bhp->bh_lockp == BHVMAGIC);
	bhp->bh_lockp = NULL;
#endif
}


#if defined(CELL_CAPABLE)

/* ARGSUSED */
void
bhv_read_lock(bhv_head_lock_t **lp)
{
}

/* ARGSUSED */
void
bhv_read_unlock(bhv_head_lock_t **lp)
{
}

/* ARGSUSED */
void
bhv_write_lock(bhv_head_lock_t **lp)
{
}

/* ARGSUSED */
void
bhv_write_unlock(bhv_head_lock_t **lp)
{
}

#endif /* CELL CAPABLE */
