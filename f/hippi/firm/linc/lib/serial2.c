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
 * serial2.c
 *
 * Use shared memory scheme to talk serial to the host.
 *
 * $Revision: 1.2 $
 */

#include <sys/types.h>
#include <sys/sbd.h>

#include "sys/PCI/linc.h"
#include "lincutil.h"
#include "serial.h"

#ifdef RDBGDEBUG
#define NPORTS 2
#else
#define NPORTS 1
#endif

#define TOLINC_ADDR	0x240
#define FROMLINC_ADDR	0x248

int	serial_intson;

void
serialinit(void)
{
	serial_intson = 0;
	LINC_WRITEREG( TOLINC_ADDR, 0 );
	LINC_WRITEREG( FROMLINC_ADDR, 0 );
}


void
serial_on(void)
{
}

void
serial_off(void)
{
}

void
serialintr(void)
{
}

/************************************************************************/


/* Returns typed character if available, otherwise -1.
 */
int
PollSerial(void)
{
	int	c;

	if ( LINC_READREG( TOLINC_ADDR ) & 0x80000000 ) {
		c = (LINC_READREG( TOLINC_ADDR ) & 0xff);
		LINC_WRITEREG( TOLINC_ADDR, 0 );
		return c;
	}
	else
		return -1;
}

/* Blocks until a character is typed.
 */
int
GetSerial(void)
{
	int	c;
	while ( (c=PollSerial()) == -1 )
		;
	return c;
}

void
PutSerial(char c)
{
	/* Block until the previous character has been read. */
	while ( 0 != (LINC_READREG( FROMLINC_ADDR ) & 0x80000000) )
		;
	LINC_WRITEREG( FROMLINC_ADDR, c | 0x80000000 );
}


#if RDBGDEBUG
void
PutSerial1(char c)
{
}
#endif /* RDBGDEBUG */



