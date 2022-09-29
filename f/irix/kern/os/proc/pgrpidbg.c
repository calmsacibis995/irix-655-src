/**************************************************************************
 *									  *
 * 		 Copyright (C) 1989-1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

#ident "$Revision: 1.6 $"

#include <sys/types.h>
#include <ksys/behavior.h>
#include <sys/idbgentry.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <ksys/vpgrp.h>

#include "vpgrp_private.h"

static void idbg_vpgrp(__psint_t);
static void idbg_vpgrp_bhv_print(vpgrp_t *);


void
pgrpidbg_init(void)
{
	idbg_addfunc("vpgrp", idbg_vpgrp);
}

static vpgrp_t *
idbg_vpgrp_lookup(
	pid_t	pgid)
{
	vpgrptab_t	*vq;

	if (pgid < 0 || pgid > MAXPID)
		return NULL;
	vq = VPGRP_ENTRY(pgid);
	return vpgrp_qlookup_locked(&vq->vpgt_queue, pgid);
}

static void
vpgrpprint(vpgrp_t *vpgrp, vpgrptab_t *vq)
{
	qprintf("vpgrp 0x%x:\n", vpgrp);
	qprintf("    pgid %d sid %d ref %d refcnt_lock @0x%x\n",
		vpgrp->vpg_pgid, vpgrp->vpg_sid, vpgrp->vpg_refcnt,
		&vpgrp->vpg_refcnt_lock);
	qprintf("    vq 0x%x bhvh 0x%x\n", vq, &vpgrp->vpg_bhvh);
	idbg_vpgrp_bhv_print(vpgrp);
}

static void
idbg_vpgrp_bhv_print(
	vpgrp_t	*vpg)
{
	bhv_desc_t	*bhv;

	bhv = VPGRP_TO_FIRST_BHV(vpg);

	switch (BHV_POSITION(bhv)) {
	case VPGRP_POSITION_PP:
		idbg_pgrp((__psint_t)BHV_PDATA(bhv));
		return;
	default:
		qprintf("Unknown behavior position %d\n", BHV_POSITION(bhv));
	}
}

static void
idbg_vpgrp(__psint_t x)
{
	kqueue_t	*kq;
	int		i;
	vpgrptab_t	*vq;
	vpgrp_t		*vpgrp;

	if (x == -1) {
		qprintf("Dumping vpgrp table for cell %d\n", cellid());
		for (i = 0; i < vpgrptabsz; i++) {
			vq = &vpgrptab[i];
			kq = &vq->vpgt_queue;
			/*
			 * for every element on this hash queue
			 */
			for (vpgrp = (vpgrp_t *)kqueue_first(kq);
			     vpgrp != (vpgrp_t *)kqueue_end(kq);
			     vpgrp = (vpgrp_t *)kqueue_next(&vpgrp->vpg_queue)) {
				vpgrpprint(vpgrp, vq);
			}
		}
	} else if (x < 0) {
		/* Let's check this first */
		vpgrp = (vpgrp_t *)x;
		if (vpgrp != idbg_vpgrp_lookup(vpgrp->vpg_pgid)) {
			qprintf("WARNING: vpgrp 0x%x, pgid %d look-up fails\n",
				vpgrp, vpgrp->vpg_pgid);
		}
		vpgrpprint((vpgrp_t *)x, NULL);
	} else { /* x > 0 */
		vpgrp = idbg_vpgrp_lookup((pid_t) x);
		if (vpgrp != NULL)
			vpgrpprint(vpgrp, NULL);
	}
}

