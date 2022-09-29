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

#ident "$Id: vhost_lp.c,v 1.2 1999/05/14 20:13:13 lord Exp $"

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/idbgentry.h>
#include <sys/ktrace.h>
#include <ksys/cell_config.h>

#include "vhost_private.h"
#include "phost_private.h"

vhost_t		*local_vhp = NULL;

void
vhost_lp_init()
{
	/*
	 * Cell init calls vhost_init
	 */
	CELL_NOT(vhost_init());
}

void
vhost_cell_init()
{
	local_vhp = vhost_create();
}

vhost_t *
vhost_local(void)
{
	return local_vhp;
}

#if DEBUG
void
idbg_vhost_bhv_print(
	vhost_t *vhp)
{
	bhv_desc_t      *bhv;

	bhv = VHOST_BHV_FIRST(vhp);

	if (BHV_POSITION(bhv) == VHOST_BHV_PP) {
		qprintf("phost 0x%x:\n", BHV_PDATA(bhv));
	} else
		qprintf("Unknown behavior position %d\n", BHV_POSITION(bhv));
}
#endif
