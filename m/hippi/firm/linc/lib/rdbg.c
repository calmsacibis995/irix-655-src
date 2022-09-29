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
 * rdbg.c
 *
 * $Revision: 1.10 $
 *
 */

#include <sys/types.h>

#ifdef R4650
#include "r4650.h"
#else
#include <sys/sbd.h>
#endif

#include "eframe.h"
#include "rdbg.h"
#include "serial.h"
#include "lincutil.h"
#include "hippi_sw.h"

#ifdef _LINC_FIRMWARE
#include <sys/PCI/linc.h>
#endif

eframe_t *eframep;
u_int	badaddr, baddr_cause;
int	debugger_mode;


extern state_t *State;



void
init_rdbg(eframe_t *ep)
{
	eframep = ep;

	initbp();
	serialinit();
	badaddr = 0;
	debugger_mode = 0;
}

/* The exception handler is entered with the EXL bit set.
 */
void
exception_handler( u_int cause, u_int sr )
{
	int	save_serial_intson;
	int	signal;

	debugger_mode++;

	sr &= ~(SR_EXL|SR_IMASK);
	setsr( sr );

	save_serial_intson = serial_intson;
	if ( ! serial_intson )
		serial_on();

	sr |= (SR_IBIT8 | SR_IE);
	setsr( sr );

#ifdef RDBGDEBUG
	printf1( "exception_handler( cause = 0x%x )\n", cause );
#endif

	if ( 0 != (sr & SR_SR) ) {
		/* NMI! */
		signal = SIGQUIT;
		eframep->regs[ R_EPC ] = eframep->regs[ R_ERREPC ];
	}
	else
		switch (cause & CAUSE_EXCMASK) {

#ifndef R4650
		case EXC_RMISS:
		case EXC_WMISS:
#else
		case EXC_IBOUND:
		case EXC_DBOUND:
#endif
		case EXC_RADE:
		case EXC_WADE:
		case EXC_DBE:
			signal = SIGBUS;
			break;

		case EXC_BREAK:
			signal = SIGTRAP;
			break;
		case EXC_II:
			signal = SIGILL;
			break;
		case EXC_FPE:
			signal = SIGFPE;
			break;
		default:
			signal = SIGINT;
			break;
		}

	debug_exc( signal, eframep );

	if ( ! save_serial_intson )
		serial_off();
	
	debugger_mode--;
}

int
read_mem_word( u_int *addr, u_int *val )
{
	badaddr = 1;
	*val = *addr;
	if ( badaddr ) {
		badaddr = 0;
		return 0;
	}
	else {
#ifdef RDBGDEBUG
		printf1( "read_mem_word failed. baddr_cause = 0x%x\n",
			baddr_cause );
#endif
		return baddr_cause;
	}
}

int
write_mem_word( u_int *addr, u_int val )
{
	badaddr = 1;
	*addr = val;
	if ( badaddr ) {
		badaddr = 0;
		return 0;
	}
	else {
#ifdef RDBGDEBUG
		printf1( "write_mem_word failed. baddr_cause = 0x%x\n",
			baddr_cause );
#endif
		return baddr_cause;
	}
}

int
read_mem_byte( u_char *addr, u_char *val )
{
	badaddr = 1;
	*val = *addr;
	if ( badaddr ) {
		badaddr = 0;
		return 0;
	}
	else {
#ifdef RDBGDEBUG
		printf1( "read_mem_byte failed. baddr_cause = 0x%x\n",
			baddr_cause );
#endif
		return baddr_cause;
	}
}

int
write_mem_byte( u_char *addr, u_char val )
{
	badaddr = 1;
	*addr = val;
	if ( badaddr ) {
		badaddr = 0;
		return 0;
	}
	else {
#ifdef RDBGDEBUG
		printf1( "write_mem_byte failed. baddr_cause = 0x%x\n",
			baddr_cause );
#endif
		return baddr_cause;
	}
}

#ifdef RDBGDEBUG

/* These routines are so that we can debug the debugger.
 * printf1() uses a second serial port to send printf messages
 * to another machine.
 */

static void
puts1( char *s )
{
	while ( *s ) {
		if ( *s == '\n' )
			PutSerial1( '\r' );
		PutSerial1( *s++ );
	}
}

void
printf1( char *fmt, ... )
{
	char	buffer[ 1024 ];
	vasprintf( buffer, fmt, (long *)(&fmt + 1) );
	puts1( buffer );
}
#endif

int
file_to_num(char *file) {
  if (strcmp(file, "sfw.c") == 0)
    return ASSFAIL_FILENO_SFW;

  if (strcmp(file, "common.c") == 0)
     return ASSFAIL_FILENO_COMMON;

  if (strcmp(file, "dfw.c") == 0)
     return ASSFAIL_FILENO_DFW;

  if (strcmp(file, "dqueues.c") == 0)
     return ASSFAIL_FILENO_DQUEUE;

  if (strcmp(file, "intr.c") == 0)
     return ASSFAIL_FILENO_INTR;

  if (strcmp(file, "sbypass.c") == 0)
     return ASSFAIL_FILENO_SBYPASS;

  if (strcmp(file, "squeues.c") == 0)
     return ASSFAIL_FILENO_SQUEUE;

  if (strcmp(file, "dbypass.c") == 0)
     return ASSFAIL_FILENO_DBYPASS;

  if (strcmp(file, "dma.c") == 0)
     return ASSFAIL_FILENO_DMA;

  if (strcmp(file, "errors.c") == 0)
     return ASSFAIL_FILENO_ERRORS;

  if (strcmp(file, "main.c") == 0)
     return ASSFAIL_FILENO_MAIN;

  return 0;
}

void
assfail(char *file, int lineno )
{
        int i;

	dprintf(1, ( "ASSERT failure in file %s, line %d\n", file, lineno ));
	i = file_to_num(file);
	State->hcmd->sign = (HIP_SIGN_ASSFAIL<<HIP_SIGN_MODE_SHIFT) | 
	  ( (i & 0xff) << HIP_SIGN_ASSFAIL_FILE_SHIFT) |
	  (lineno & HIP_SIGN_ASSFAIL_LINE_MASK);

	die(CDIE_ASSFAIL, 0);
}
