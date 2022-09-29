/*
 *               Copyright (C) 1997 Silicon Graphics, Inc.                     
 *                        
 *  These coded instructions, statements, and computer programs  contain
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and
 *  are protected by Federal copyright law.  They  may  not be disclosed
 *  to  third  parties  or copied or duplicated in any form, in whole or
 *  in part, without the prior written consent of Silicon Graphics, Inc.
 *                        
 *
 *  Filename: st_timers.c
 *  Description: timing routines for the ST protocol stack.
 *
 *  $Author: kaushik $
 *  $Date: 1999/04/30 21:15:17 $
 *  $Revision: 1.1 $
 *  $Source: /proj/irix6.5f/isms/irix/kern/bsd/netinet/RCS/st_timers.c,v $
 *
 */


#include 	"sys/param.h"
#include 	"sys/types.h"
#include	"sys/systm.h"
#include	"sys/uio.h"
#include	"sys/debug.h"
#include 	"sys/kmem.h"
#include 	"sys/cmn_err.h"

#include	"st.h"
#include	"st_var.h"
#include	"st_macros.h"
#include	"st_debug.h"



void
st_cancel_timer(struct stpcb *sp, uint timer_id)
{
	/* this has races: we'll deal with that later!!!
	 * for now, keep fingers crossed, pray, etc. */

	dprintf(10, ("Clearing timer %u (sp 0x%x)\n", timer_id, sp));
	sp->s_timer[timer_id] = 0;
}


void
st_set_timer(struct stpcb *sp, uint timer_id, uint timerval)
{
	/* this has races: we'll deal with that later!!!
	 * for now, keep fingers crossed, pray, etc. */

	sp->s_timer[timer_id] = timerval;
}


void
st_cancel_timers(struct stpcb *sp)
{
	int		i;

	dprintf(10, ("Clearing all ST timers on PCB 0x%x\n", sp));
	for(i = 0; i < STPT_NTIMERS; i++)  {
		dprintf(10, ("Cancelling timer %u (sp 0x%x)\n", i, sp));
		sp->s_timer[i] = 0;
	}
}


struct stpcb *
st_timers(struct stpcb *sp, __psint_t timer)
{
	uint		expired_id;
	int		rv;

	if(IS_VC_TIMER(timer))  {
		stvc_detach(sp->s_so);
		return(NULL);
	}
	else if(IS_TX_TIMER(timer))  {
		expired_id = TIMER_ID_TO_TID(timer);
		dprintf(10, ("Timer for tx %u went off\n", expired_id));
		rv = st_tx_timer_expiry(sp, TIMER_ID_TO_TID(timer));
		if(rv)  {
			dprintf(0, ("TX timer expiry wasn't caught by ST\n"));
		}
	}
	else if(IS_RX_TIMER(timer))  {
		expired_id = TIMER_ID_TO_RID(timer);
		dprintf(10, ("Timer for rx %u went off\n", expired_id));
		rv = st_rx_timer_expiry(sp, TIMER_ID_TO_RID(timer));
		if(rv)  {
			dprintf(0, ("RX timer expiry wasn't caught by ST\n"));
		}
	}
	else if(IS_SLOT_TIMER(timer)) {
		dprintf(0, ("Slot timer went off\n"));
		rv = st_slots_timer_expiry(sp);
		return(NULL);
	}
	else {
		cmn_err(CE_PANIC, "ST protocol error; "
			"Unknown timer %d went off!\n", timer);
	}

	return sp;
}
