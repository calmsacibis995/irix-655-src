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
 * intr.c
 *
 * $Revision: 1.18 $
 */

#include <sys/types.h>

#include "r4650.h"
#include "sys/PCI/linc.h"
#include "lincutil.h"
#include "hippi_sw.h"

#include "eframe.h"
#include "rdbg.h"
#include "serial.h"

extern state_t   	*State;

/* eframe for interrupts. (An eframe contains a saved context and
 * a small stack).
 */
eframe_t	*intframep, iframe[2];


u_int	 	tmr_compare;
u_int		timer_ticks;

static int	intr_timer_active;
static int	intr_flags;
#define INTR_FLAG_ENABLE_SYNCH 1


void
init_intrs(void)
{
	/* Start the timer.
	 */
	tmr_compare = get_r4k_count()+TMR_TICK*4;
	set_r4k_compare( tmr_compare );
	timer_ticks = 0;
	intr_timer_active = 0;

	/* Clear these LINC interrupts. */
	LINC_WRITEREG( LINC_CONTROLLER_INTERRUPT_STATUS,
		LINC_CISR_CLR_FIRM_INT );
#ifdef RINGBUS_WAR
	(void)LINC_READREG( LINC_SDRAM_ADDR );
#endif

	/* Enable these LINC interrupts. */
	LINC_WRITEREG( LINC_CONTROLLER_INTERRUPT_MASK,
		~( LINC_CISR_ERRS_MASK |
		   LINC_CISR_NMI_BUTTON |
		   LINC_CISR_FIRM_INTR) );
#ifdef RINGBUS_WAR
	(void)LINC_READREG( LINC_SDRAM_ADDR );
#endif
		      
		      
#ifdef RINGBUS_WAR
	(void)LINC_READREG( LINC_SDRAM_ADDR );
#endif

	/* Enable interrupts.
	 */
	intframep = &iframe[0];
	setsr( getsr() | SR_IE | SR_IBIT8 | SR_IBIT3 );
}


void
timerintr(void)
{
	int	i;

	timer_ticks++;
	
	if ( !(State->flags & FLAG_FATAL_ERROR)) {

	  if (timer_ticks % CHECK_RR_TIMER == 0)
	    /* check roadrunner is alive */
	    State->flags |= FLAG_CHECK_RR_EN;
	  if (State->flags & FLAG_GOT_INIT) {
	    if (timer_ticks % CHECK_OPP_TIMER == 0) {
	      /* update state from opposite side */
	      State->flags |= FLAG_CHECK_OPP_EN;
	    }
	    if (timer_ticks%PUSH_TO_OPP_TIMER == 0) {
	      /* push state to opposite side */
	      State->flags |= FLAG_PUSH_TO_OPP;
	    }
	  }
	  set_leds(timer_ticks);
	}
}


/* Handler is entered with IE and EXL bits reset.
 */
void
interrupt_handler( u_int cause, u_int sr, eframe_t *iframe )
{
	u_int	ibits;

	intframep++;

	/* Handle timer interrupts.  Timer interrupts can happen
	 * at two different speeds depending wether serial interrupts
	 * are enabled.  When serial interupts are enabled, we take
	 * care of the serial interrupt and then check to see if we
	 * need to take care of the regular, slower interrupts.
	 *
	 * When we call the "regular" timer interrupt handler during
	 * a serial interrupt, we'll re-enable timer interrupts so
	 * that serial interrupts can continue.
	 */
	if ( (sr & SR_IBIT8) && (cause & CAUSE_IP8) ) {
		if ( serial_intson ) {

			/* Take care of serial ports.  Sets up next
			 * timer expiration.
			 */
			serialintr();

			/* If not in debug-stop mode, we'll keep calling
			 * the local timer interrupt handler.
			 */
			if ( !debugger_mode &&
			     (int)(get_r4k_count()-tmr_compare) >= 0 ) {
				tmr_compare += TMR_TICK;

				ASSERT( ! intr_timer_active );

				intr_timer_active = 1;
				setsr( (sr&(SR_IMASK7|~SR_IMASK)) | SR_IE );

				timerintr();

				intr_timer_active = 0;
			}

		}
		else {
			timerintr();

			/* Set up the next timer interrupt.
			 */
			tmr_compare += TMR_TICK;
			set_r4k_compare( tmr_compare );
		}
	}
	
	/* Handle LINC generated interrupt bits.
	 */
	if ( (sr & SR_IBIT3) && (cause & CAUSE_IP3) ) {
		setsr( (sr & (SR_IMASK3|~SR_IMASK)) | SR_IE );

		/* External LINC interrupt. */
		ibits = LINC_READREG( LINC_CONTROLLER_INTERRUPT_STATUS );

		if ( 0 != (ibits & LINC_CISR_ERRS_MASK) )
			fatal_cisr_errs(ibits);
		if ( 0 != (ibits & LINC_CISR_DMA0_INT) )
			dmaintr();
		if ( 0 != (ibits & LINC_CISR_DMA1_INT) )
			dmaintr();
		if ( 0 != (ibits & LINC_CISR_FIRM_INTR) ) {
			/* This is the host telling us to stop.
			 */
			LINC_WRITEREG( LINC_CONTROLLER_INTERRUPT_STATUS,
				LINC_CISR_CLR_FIRM_INT );
#ifdef RINGBUS_WAR
			(void)LINC_READREG( LINC_SDRAM_ADDR );
#endif

			flush_dcache();

#ifndef NO_BREAKPOINTS
			breakpoint();
#endif
		}
	}

	intframep--;
}

