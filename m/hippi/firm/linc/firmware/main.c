/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1997, Silicon Graphics, Inc                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/
/*
 * main.c
 *
 * $Revision: 1.22 $
 *
 */

#include <sys/types.h>
#include <sys/errno.h>
#include "r4650.h"

#include "sys/PCI/linc.h"
#include "hippi_sw.h"

#include "eframe.h"
#include "rdbg.h"
#include "lincutil.h"

state_t		*State;

trace_t 	*Tracep;
trace_t 	*Trace_base;


int debug = 0;

void
main(void)
{
    State = (state_t *)PHYS_TO_K0(STATE_BASE);
    State->hcmd = (hip_hc_t *)PHYS_TO_K1(HCMD_BASE);

    init_linc();

    init_rdbg((eframe_t *) PHYS_TO_K1( FIRM_EFRAME ));

    init_intrs();


#ifdef DEBUG

#ifndef NO_BREAKPOINTS		/* for gdb */

#ifdef HIPPI_SRC_FW
    debug = 0;
#else /* Destination fw */
    debug = 0;
#endif
    breakpoint();
#endif
#endif

    init_mem();

    init_board(State->hostp, State->wirep);

    /* loop until INIT command recv'd */
    while(State->old_cmdid == State->hcmd->cmd_id) {
	continue;
    }

    process_hcmd(State->hostp, State->wirep);

    dprintf(1, ("main: entering main_loop\n"));
    main_loop(State->hostp, State->wirep);

    /* should never return */
    assert(0);
}






