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
 * serial.c
 *
 * Handle "software UART" in LINC environment.
 *
 *	GPIO0 - port 0 in
 *	GPIO1 - port 0 out
 *
 * NOTE: 4-5-96:  These routines work fairly well in simulation with
 * about a 6% variance in bit-time transitions due to variable cache
 * behavior.  If this becomes a problem, I think the best solution
 * would be to have the serial_intr() routine calculate the next GPIO
 * output transitions and have the following serial interrupt write
 * the GPIO register as the first thing it does in the trap handler.
 * This way, the variation of time between timer interrupt and writing the
 * GPIO register will be minimized.
 *
 * $Revision: 1.4 $
 */

#include <sys/types.h>
#include <sys/sbd.h>

#include "sys/PCI/linc.h"
#include "r4650.h"
#include "lincutil.h"
#include "serial.h"

#ifdef RDBGDEBUG
#define NPORTS 2
#else
#define NPORTS 1
#endif

typedef struct {
	u_char	i_hd, i_tl;
	u_char	i_q[ SERIAL_QSIZE ];
	u_char	o_hd, o_tl;
	u_char	o_q[ SERIAL_QSIZE ];

	u_char	i_state, i_buf;
	u_char	o_state, o_buf;
} port_t;
#define NXTPTR(x)	(((x)+1)&(SERIAL_QSIZE-1))

static port_t	ports[ NPORTS ];
static u_int	serial_nextcompare;

int	serial_intson;

/* GPIO pin to port to pin definitions.
 */
#define PORTI_SHFT(x)	(0+LINC_GPIO_DATA_IN_SHFT+(x)*2)
#define PORTO_SHFT(x)	(1+LINC_GPIO_DATA_OUT_SHFT+(x)*2)
#define PORTI_MASK(x)	(1<<PORTI_SHFT(x))
#define PORTO_MASK(x)	(1<<PORTO_SHFT(x))


void
serialinit(void)
{
	int	i;
	port_t	*port;

	serial_intson = 0;

	for (i=0; i<NPORTS; i++) {
		port = & ports[i];

		port->i_hd = 0;
		port->i_tl = 0;
		port->o_hd = 0;
		port->o_tl = 0;

		port->i_state = 0;
		port->o_state = 0;
	}

	/* Set output drivers to high.
	 */
	LINC_WRITEREG( LINC_GPIO, LINC_GPIO_OUT_EN_W(0xa) |
		LINC_GPIO_DATA_OUT_MASK );
}


void
serial_on(void)
{
	serial_intson = 1;
	serial_nextcompare = get_r4k_count() + SERIAL_TMR_TICK*4;
	set_r4k_compare( serial_nextcompare );
}

void
serial_off(void)
{
	int	i;
	port_t	*port;

	/* Wait until all output queues are drained.
	 */
	do {
		for (i=0; i<NPORTS; i++) {
			port = & ports[i];
			if ( port->o_hd != port->o_tl || port->o_state )
				break;
		}
	} while ( i<NPORTS );

	serial_intson = 0;

	/* Clear out buffers.
	 */
	for (i=0; i<NPORTS; i++) {
		port = & ports[i];

		port->i_hd = 0;
		port->i_tl = 0;
		port->o_hd = 0;
		port->o_tl = 0;

		port->i_state = 0;
		port->o_state = 0;
	}

	/* Set output drivers to high.
	 */
	LINC_WRITEREG( LINC_GPIO, LINC_GPIO_OUT_EN_W(0xa) |
		LINC_GPIO_DATA_OUT_MASK );
}


/************************************************************************/


/* Returns typed character if available, otherwise -1.
 */
int
PollSerial(void)
{
	int	c;

#ifdef DEBUG
	if ( ! serial_intson )
		breakpoint();
#endif

	if ( ports[0].i_hd == ports[0].i_tl )
		return -1;
	
	c = ports[0].i_q[ ports[0].i_tl ];
	ports[0].i_tl = NXTPTR( ports[0].i_tl );
	return c;
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
	u_char	nxt = NXTPTR( ports[0].o_hd );

#ifdef DEBUG
	if ( ! serial_intson )
		breakpoint();
#endif

	/* Block until there's room in serial queue! */
	while ( nxt == ports[0].o_tl )
		;
	
	ports[0].o_q[ ports[0].o_hd ] = c;
	ports[0].o_hd = nxt;
}

void
SerialSync(void)
{
	int	i;
	port_t	*port;

	/* Wait until all output queues are drained.
	 */
	do {
		for (i=0; i<NPORTS; i++) {
			port = & ports[i];
			if ( port->o_hd != port->o_tl || port->o_state )
				break;
		}
	} while ( i<NPORTS );
}


#if RDBGDEBUG
void
PutSerial1(char c)
{
	u_char	nxt = NXTPTR( ports[1].o_hd );

	/* Block until there's room in serial queue! */
	while ( nxt == ports[1].o_tl )
		;
	
	ports[1].o_q[ ports[1].o_hd ] = c;
	ports[1].o_hd = nxt;
}
#endif /* RDBGDEBUG */


/*************************************************************************/

void
serialintr(void)
{
	register port_t	*port;
	u_int	gpio;
	int	i = 0;

	/* Set up next serial timer interrupt.
	 */
	serial_nextcompare += SERIAL_TMR_TICK;
	set_r4k_compare( serial_nextcompare );

	if ( serial_intson ) {

	   gpio = LINC_READREG( LINC_GPIO );

	   for (i=0; i<NPORTS; i++) {
		port = & ports[i];

		/* Input PORT 0
		 */
		if ( port->i_state == 0 ) {
			/* Look for start bit */
			if ( 0 == (gpio & PORTI_MASK(i)) ) {
				port->i_state = 1;
				port->i_buf = '\0';
			}
		}
		else if ( port->i_state > 0 && port->i_state < 72 ) {

			if ( (port->i_state & 7) == 2 ) {
				
				/* Sample time! */
				port->i_buf = (port->i_buf>>1) |
				   ( (gpio & PORTI_MASK(i)) ? 0x80 : 0x00 );

				/* We're really counting by fives but five is
				 * hard to divide by so I add 3 extra every
				 * sample time to make it easier to decode
				 * state.
				 */
				port->i_state += 4;
			}
			else
				port->i_state++;
		}
		else if ( port->i_state < 75 ) {
			port->i_state++;
		}
		else if ( port->i_state == 75 ) {
			if ( 0 == (gpio & PORTI_MASK(i)) ) {
				/* No stop bit-- either a break or an
				 * error. */
				port->i_state = 99;
			}
			else {
				/* Got a character! */
				port->i_q[ port->i_hd ] = port->i_buf;
				port->i_hd = NXTPTR(port->i_hd);
				port->i_state = 0;
			}
		}
		else {
			/* Wait for stop. */
			if ( 0 != (gpio & PORTI_SHFT(i)) )
				port->i_state =0;
		}


		/* Output PORT 0
		 */
		if ( port->o_state == 0 ) {
			/* Got anything to send? */
			if ( port->o_hd != port->o_tl ) {
				port->o_buf = port->o_q[ port->o_tl ];
				port->o_tl = NXTPTR(port->o_tl);
				port->o_state = 1;
				gpio &= ~PORTO_MASK(i);
			}
			else
				gpio |= PORTO_MASK(i);
		}
		else if ( port->o_state > 0 && port->o_state < 69 ) {
			if ( (port->o_state & 7) == 5 ) {
				gpio = (gpio & ~PORTO_MASK(i)) |
					((port->o_buf & 1)<<PORTO_SHFT(i));
				port->o_buf >>= 1;

				/* We're really counting by fives but five is
				 * hard to divide by so I add 3 extra every
				 * sample time to make it easier to decode
				 * state.
				 */
				port->o_state += 4;
			}
			else
				port->o_state++;
		}
		else {
			/* stop bit */
			gpio |= PORTO_MASK(i);
			port->o_state++;
			if ( port->o_state == 74 )
				port->o_state = 0;
		}

	   } /* each port */

	   /* Set output pins. */
	   LINC_WRITEREG( LINC_GPIO, gpio );

	} /* serial_intson */
}

