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
 * errors.c
 *
 * $Revision: 1.16 $
 *
 */

#include <sys/types.h>
#include <sys/errno.h>
#include "r4650.h"

#include "sys/PCI/linc.h"
#include "hippi_sw.h"

#include "rdbg.h"

#include "lincutil.h"
#include "serial.h"

extern state_t   *State;

void
die( int code, int auxdata )
{
    int i, j;

    /* disable normal lighting of LED's through timer interrupt */

    State->flags |=  FLAG_FATAL_ERROR;
    dprintf( 1, ("code = 0x%x, auxdata = 0x%x\n", code, auxdata));

    /* Disable all LINC interrupts except NMI and sw INT */
    LINC_WRITEREG( LINC_CONTROLLER_INTERRUPT_MASK,
		   ~(LINC_CISR_NMI_BUTTON |
		     LINC_CISR_FIRM_INTR) );
#ifdef RINGBUS_WAR
    (void)LINC_READREG( LINC_SDRAM_ADDR );
#endif

    flush_dcache();
    trace(TOP_MISC, T_MISC_DIE,0, code, auxdata);

#ifndef NO_BREAKPOINTS
    breakpoint();
#endif

    if (code != CDIE_ASSFAIL) {
        /* sign is written by assfail, else write it here */
        State->hcmd->sign = (HIP_SIGN_CDIE<<HIP_SIGN_MODE_SHIFT)
	    | ((code << HIP_SIGN_CDIE_MAJ_SHIFT) & HIP_SIGN_CDIE_MAJ_MASK) 
	    | (auxdata & HIP_SIGN_CDIE_MIN_MASK);
    }

    while (1) {
        if (code == CDIE_OPP_DEAD)
	    /* don't wink LED if opposite was the one that's dead */
	    LINC_WRITEREG(LINC_LED, ~0x5);
	else
	    blink_error_leds(HIP_SIGN_CDIE, code, auxdata);
    }
}

void
fatal_cisr_errs(u_int linc_cisr)
{
  /* don't read detailed status because it clears error bits */
	int minor = 0;

	dprintf(1, ( "fatal_cisr_errs: linc_cisr = 0x%x\n", linc_cisr ));

	if (linc_cisr & LINC_CISR_SYSAD_DPAR ) {	    
	    dprintf(1, ( "fatal SYSAD write data parity error\n"));
	    minor = CDIE_AUX_CISR_SYSAD_ERR;
	}
	
	if ( (linc_cisr & LINC_CISR_BUFMEM_RD_ERR)
	     || (linc_cisr & LINC_CISR_BUFMEM_ERR)) {
	    dprintf(1, ( "fatal bufmem read parity error or other error in BME register\n"));
	    minor = CDIE_AUX_CISR_SDRAM_ERR;
	}

	if ( (linc_cisr & LINC_CISR_CPCI_RD_ERR)
	     || (linc_cisr & LINC_CISR_CPCI_ERROR)) {
	    dprintf(1, ( "fatal CPCI read error or other error in CERR register\n"));
	    minor = CDIE_AUX_CISR_CPCI_ERR;
	}
	    
	if (linc_cisr & LINC_CISR_PPCI_ERROR ) {	    
	    dprintf(1, ( "fatal PPCI error - see PCCSR\n"));
	    minor = CDIE_AUX_CISR_PPCI_ERR;
	}
	
	if ( (linc_cisr & LINC_CISR_BBUS_RTO)
	     || (linc_cisr & LINC_CISR_BBUS_PARERR)
	     || (linc_cisr & LINC_CISR_BBUS_ERR)) {
	    dprintf(1, ( "fatal BBUS error\n"));
	    minor = CDIE_AUX_CISR_BBUS_ERR;
	}

	if (linc_cisr & LINC_CISR_DMA0_ERR) {
	    dprintf(1, ("fatal_cisr_errs: dma0 - look at dcsr\n"));
	    minor = CDIE_AUX_CISR_DMA0_ERR;
	}

	if (linc_cisr & LINC_CISR_DMA1_ERR) {
	    dprintf(1, ("fatal_cisr_errs: dma1 - look at dcsr\n"));
	    minor = CDIE_AUX_CISR_DMA1_ERR;
	}

	if (minor == 0)
	    minor = CDIE_AUX_CISR_MISC;

	die( CDIE_CISR_ERR, minor);
}


