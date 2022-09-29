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
 * rdbg_kdbx.c
 *
 * $Revision: 1.2 $
 *
 */

#ifndef GDB

/**********************************************************************
 *
 * kdbx stubs
 *
 **********************************************************************/

#include <sys/types.h>
#include <sys/errno.h>

#include "eframe.h"
#include "rdbg.h"
#include "serial.h"
#include "lincutil.h"

#define SYN	'\026'
#define DLE	'\020'

static u_char	seq_out;
static u_char	seq_in;

#define MKSTATUS(signal_code,exit_stat)	(((signal_code)<<8)|(exit_stat))

/* XXX #define WSTOPPED	0004 */
#define WSTOPFLG	0177
#define SIGTRAP 	5

int
poll_serial_ms( int ms )
{
	__uint32_t	t0;
	int	c;

#ifndef SABLE
	t0 = get_r4k_count() + ms*(CPU_HZ/2000);
#else
	t0 = get_r4k_count() + ms*50;
#endif

	do {
		c = PollSerial();
	} while ( c<0 && (int)(get_r4k_count()-t0) < 0 );

	return c;
}

void
kdbx_ack(void)
{
	PutSerial( SYN );			/* SYN */
	PutSerial( 0x60 ); 			/* TYPE_LEN */
	PutSerial( 0x40 ); 			/* LEN1 */
	PutSerial( 0x40 + seq_in );		/* SEQ */

	PutSerial( 0x40 );
	PutSerial( 0x43 + ((seq_in&0x20)>>6) );
	PutSerial( 0x40 | (seq_in^0x20) );
}

void
kdbx_putp( u_char *buf )
{
	int	i, c;
	u_int	len;
	u_int	cksum;
	u_char	*p;

	/* Get length and checksum */
	cksum = 0;
	len = 0;
	p = buf;
	while ( *p ) {
		cksum += *p;
		len++;
		p++;
	}
	cksum += ( 0x40 + (len>>6) );		/* TYPE_LEN */
	cksum += ( 0x40 + (len&0x3f) );		/* LEN1 */
	cksum += ( 0x40 + seq_out );		/* SEQ */

	for (;;) {
#ifdef RDBGDEBUG
		printf1( "Sending msg: '%s'\n", buf );
#endif
		PutSerial( SYN );		/* SYN */
		PutSerial( 0x40 + (len>>6) );	/* TYPE_LEN */
		PutSerial( 0x40 + (len&0x3f) );	/* LEN1 */
		PutSerial( 0x40 + seq_out );	/* SEQ */

		/* DATA */
		p = buf;
		while ( *p ) {
			/* XXX: escape with DLE?? */
			PutSerial( *p++ );
		}

		/* CSUM1..CSUM3 */
		PutSerial( 0x40 + ((cksum>>12)&0x3f) );
		PutSerial( 0x40 + ((cksum>>6)&0x3f) );
		PutSerial( 0x40 + (cksum&0x3f) );

		/* Get acknowledgement! */
		do {
			c = poll_serial_ms( 1000 );
		} while ( c >= 0 && c != SYN );
		if ( c != SYN )
			continue;
		c = poll_serial_ms( 1000 );
		if ( c != 0x60 )
			continue;
		c = poll_serial_ms( 1000 );
		if ( c != 0x40 )
			continue;
		c = poll_serial_ms( 1000 );
		if ( c != 0x40+((seq_out+1)&0x3f) )
			continue;
		c = poll_serial_ms( 1000 );
		if ( c != 0x40 )
			continue;
		c = poll_serial_ms( 1000 );
		if ( c != 0x43+(((seq_out+1)&0x20)>>6) )
			continue;
		c = poll_serial_ms( 1000 );
		if ( c != 0x40+(((seq_out+1)&0x3f)^0x20) )
			continue;
		
		/* success! */
		seq_out = (seq_out+1)&0x3f;
#ifdef RDBGDEBUG
		printf1( "Sent (and ack'ed) msg: '%s'\n", buf );
#endif
		return;
	}
}

void
kdbx_getp( u_char *buf )
{
	int	i, c, len;
	u_char	*p, seq;
	u_int	cksum, cksum_in;

	for (;;) {

	again:
		/* Get SYN */
		do {
			c = PollSerial();
		} while ( c != SYN );

	gotsyn:

		cksum = 0;

		/* Get TYPE_LEN-- ignore any ACK packets */
		c = poll_serial_ms( 1000 );
		if ( c<0 )
			goto again;
		if ( c == SYN )
			goto gotsyn;
		if ( 0 != (c & 0x20) )
			continue;
		len = (c&0x1f) << 6;
		cksum += c;

		/* Get LEN1 */
		c = poll_serial_ms( 1000 );
		if ( c<0 )
			goto again;
		if ( c == SYN )
			goto gotsyn;
		len |= c&0x3f;
		cksum += c;

		/* Get SEQ */
		c = poll_serial_ms( 1000 );
		if ( c<0 )
			goto again;
		if ( c == SYN )
			goto gotsyn;
		seq = c&0x3f;
		cksum += c;

		/* Get DATA */
		p = buf;
		for (i=0; i<len; i++) {
			c = poll_serial_ms( 1000 );
			if ( c<0 )
				goto again;
			if ( c == SYN )
				goto gotsyn;

			cksum += c;

			/* Handle DLE escape sequences.  Probably not needed
			 * since it turns out the MIPS debugging protocol
			 * is all ASCII!
			 */
			if ( c == DLE ) {
				c = poll_serial_ms( 1000 );
				if ( c<0 )
					goto again;
				switch (c) {
				case 'S':
					c = SYN;
					break;
				case 'D':
					c = DLE;
					break;
				case 'C':
					c = '\003';
					break;
				case 's':
					c = '\023';
					break;
				case 'q':
					c = '\021';
					break;
				default:
					continue;
				}
			}

			*p++ = c;
		}
		*p = '\0';

		/* Get CSUM1..CSUM3 */
		cksum_in = 0;
		for (i=0; i<3; i++) {
			c = poll_serial_ms( 1000 );
			if ( c<0 )
				goto again;
			if ( c == SYN )
				goto gotsyn;
			cksum_in = (cksum_in<<6) | (c&0x3f);
		}

		if ( cksum_in == cksum ) {
			seq_in = (seq+1) & 0x3f;
			kdbx_ack();
			return;
		}
		else {
			/* Use old seq number to "NAK" */
			kdbx_ack();
#ifdef RDBGDEBUG
			printf1( "bad cksum on input packet\n" );
#endif
		}
	}
}

__uint64_t
gethex( char **p )
{
	__uint64_t val = 0;

	while ( **p == ' ' )
		++*p;

	if ( **p != '0' && *(*p+1) != 'x' )
		return 0;
	*p += 2;

	for (;;) {
		if ( **p >= '0' && **p <= '9' ) {
			val = (val<<4) | (**p-'0');
			++*p;
		}
		else if ( **p >= 'A' && **p <= 'F' ) {
			val = (val<<4) | (**p-'A'+10);
			++*p;
		}
		else if ( **p >= 'a' && **p <= 'f' ) {
			val = (val<<4) | (**p-'a'+10);
			++*p;
		}
		else
			return val;
	}
}

void
puthex( char **p, __uint64_t val )
{
	int	i, hexdigit;

	*(*p)++ = '0';
	*(*p)++ = 'x';

	if ( val == 0 ) {
		*(*p)++ = '0';
		return;
	}

	i=0;
	while ( (val & 0xf000000000000000LL) == 0 ) {
		val = (val<<4);
		i++;
	}
	while (i<16) {
		hexdigit = (int) (val>>60);
		if ( hexdigit < 10 )
			*(*p)++ = '0'+hexdigit;
		else
			*(*p)++ = 'a'+hexdigit-10;
		val = (val<<4);
		i++;
	}
}
		

void
kdbx_reply( int proc, char cmd, __uint64_t val1, __uint64_t val2 )
{
	char	buf[ 0x200 ], *p;

	p = buf;
	puthex( &p, proc );
	*p++ = ' ';
	*p++ = cmd;
	*p++ = ' ';
	puthex( &p, val1 );
	*p++ = ' ';
	puthex( &p, val2 );
	*p = '\0';

	kdbx_putp( buf );
}

void
kdbx_g_reply( int proc, u_int mask, eframe_t *eframe, char *buf )
{
	int	i;
	char	*p;

	/* XXX: I use debug_exc()'s buf space to conserve stack space. */
	p = buf;

	if ( mask&1 ) {
		*p++ = ' ';
		puthex( &p, eframe->regs[ R_EPC ] );
	}
	mask >>= 1;

	for (i=1; i<32; i++) {
		if ( mask&1 ) {
			*p++ = ' ';
			puthex( &p, eframe->regs[ i ] );
		}
		mask >>= 1;
	}

	kdbx_putp( buf );
}

void
debug_exc( int signal, eframe_t *eframe )
{
	char	buf[ 0x800 ], *p, cmd;
	int	proc;
	__uint64_t val1, val2;
	u_int	lval;

#ifdef RDBGDEBUG
	printf1( "debug_exc( %d, 0x%x )\n", signal, (long)eframe );
#endif

	if ( signal == SIGTRAP && eframe->regs[ R_EPC ] == (long)&breakpoint )
		eframe->regs[ R_EPC ] += sizeof(inst_t);
	
	fixup_brkpts();
	if ( brkpt_cont ) {
		brkpt_cont = 0;
		if ( signal == SIGTRAP )
			return;
	}

	kdbx_reply( 1, 'b', 0, MKSTATUS(SIGTRAP, WSTOPFLG) );

	for (;;) {
		kdbx_getp( buf );
#ifdef RDBGDEBUG
		printf1( "Got msg: '%s'\n", buf );
#endif
		p = buf;
		proc = (int) gethex( &p );

		while ( *p == ' ' )
			p++;
		cmd = *p++;

		val1 = gethex( &p );
		val2 = gethex( &p );

		switch (cmd) {
		case 'i':
		case 'd':
			if ( read_mem_word( (u_int *)val1, &lval ) )
				kdbx_reply( proc, cmd, -1, EFAULT );
			else
				kdbx_reply( proc, cmd, 0, (__uint64_t)lval);
			break;
		case 'I':
		case 'D':
			if ( write_mem_word( (u_int *)val1, val2 ) )
				kdbx_reply( proc, cmd, -1, EFAULT );
			else
				kdbx_reply( proc, cmd, 0, val2 );
			break;
		case 'r':
			kdbx_reply( proc, cmd, 0, eframe->regs[ val1 ] );
			break;
		case 'R':
			eframe->regs[ val1 ] = val2;
			kdbx_reply( proc, cmd, 0, val2 );
			break;
		case 'c':
			cont( eframe );
			kdbx_reply( proc, cmd, 0, 0 );
			return;
		case 'x':
			kdbx_reply( proc, cmd, 0, 0 );
#ifdef RDBGDEBUG
			printf1( "process killed by kdbx\n" );
#endif
			while (1)
				;
			break;
		case 'g':
			kdbx_g_reply( proc, (u_int)val1, eframe, buf );
			break;
		case 's':
			step1( eframe );
			kdbx_reply( proc, cmd, 0, 0 );
			return;
		default:
			kdbx_reply( proc, cmd, -1, EINVAL );
			break;
		}
	}
	/*NOTREACHED*/
}

/* Allow a program to put console messages through the kdbx
 * interface.
 */
void
printf( char *fmt, ... )
{
}

#endif /* ! GDB */
