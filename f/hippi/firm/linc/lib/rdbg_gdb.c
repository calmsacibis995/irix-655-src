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
 * rdbg_gdb.c
 *
 * $Revision: 1.2 $
 *
 */

#ifdef GDB

/**********************************************************************
 *
 * GDB stubs
 *
 **********************************************************************/

#include <sys/types.h>

#include "eframe.h"
#include "rdbg.h"
#include "serial.h"
#include "lincutil.h"

#ifdef R4650
#include "r4650.h"
#else
#include <sys/sbd.h>
#endif

#ifdef RDBGDEBUG
int debug_gdb;
#endif

#define MAXGDBPKT	1024
#define GDB_TIMEOUT	5000	/* msecs */

static char gdbbuf[ MAXGDBPKT ];

static char
tohex( int x )
{
	return ( x > 9 ? x+'a'-10 : x+'0' );
}

static void
prepare_resume_reply( char *buf, int sig )
{
	*buf++ = 'S';
	*buf++ = tohex( (sig>>4) & 0x0f );
	*buf++ = tohex( sig & 0x0f );
	*buf++ = '\0';
}

static void
prepare_error_reply( char *buf, int errno )
{
	*buf++ = 'E';
	*buf++ = tohex( (errno>>4) & 0x0f );
	*buf++ = tohex( errno & 0x0f );
	*buf++ = '\0';
}

static long
readhexint( char **buf, int maxlen )
{
	long val = 0;

	for (;;) {
		if ( maxlen == 0 )
			break;
		else if ( **buf >= '0' && **buf <= '9' ) {
			val = (val<<4) | (**buf - '0');
			++*buf;
			maxlen--;
		}
		else if ( **buf >= 'a' && **buf <= 'f' ) {
			val = (val<<4) | (**buf - 'a' + 10);
			++*buf;
			maxlen--;
		}
		else if ( **buf >= 'A' && **buf <= 'F' ) {
			val = (val<<4) | (**buf - 'A' + 10);
			++*buf;
			maxlen--;
		}
		else
			break;
	}

	return val;
}

static int
fromhex( char c )
{
	if ( c>='0' && c<= '9' )
		return c-'0';
	else if ( c>='a' && c<='f' )
		return c-'a'+10;
	else if ( c>='A' && c<='F' )
		return c-'A'+10;
	else
		return 0; /* XXX */
}

void
gdb_putp( char *buf )
{
	int	i, c;
	u_char	cksum;
	char	*p;

	/* checksum the packet */
	p = buf;
	cksum = 0;
	while ( *p != '\0' )
		cksum += *p++;

	/* Send it over and over until we get a positive ack. */
	do {
		PutSerial( '$' );
		for (p=buf; *p; p++)
			PutSerial( *p );
		PutSerial( '#' );
		PutSerial( tohex( (cksum>>4) & 0x0f ) );
		PutSerial( tohex( cksum & 0x0f ) );

		i=0;
		while ( i < GDB_TIMEOUT*10 ) {
			c = PollSerial();
			if ( c == '+' || c == '-' )
				break;
			DELAY( 100 );
			i++;
		}

	} while ( c != '+' );
}

void
gdb_getp( char *buf )
{
	int	c;
	char	*p;
	u_char	cksum, c1, c2;

	for (;;) {

		do {
			c = GetSerial();
		} while ( c != '$' );

	gotsyn:
		p = buf;
		cksum = 0;

		for (;;) {
			c = GetSerial();
			if (c == '#' )
				break;
			if (c == '$' )
				goto gotsyn;
			*p++ = c;
			cksum += c;
			if ( p-buf >= MAXGDBPKT ) {
#ifdef RDBGDEBUG
				printf1( "packet too big in gdb_getp\n" );
#endif
				goto gdb_getp_err;
			}
		}
		*p = '\0';

		c = GetSerial();
		if (c == '$' )
			goto gotsyn;
		c1 = fromhex( c );

		c = GetSerial();
		if (c == '$' )
			goto gotsyn;
		c2 = fromhex( c );

		if ( cksum == (c1<<4)+c2 ) {
			PutSerial( '+' );
			return;
		}
#ifdef RDBGDEBUG
		printf1( "bad checksum in gdb_getp\n" );
#endif

gdb_getp_err:
		PutSerial( '-' );

	}
}

#define OKAY(buf)	( (buf)[0]='O',(buf)[1]='K',(buf)[2]='\0' )

void
debug_exc( int signal, eframe_t *eframe )
{
	int	i, j;
	u_long	addr, len;
	char	*bp;
	u_int	lval;
	u_char	bval;
	reg_t	regval;
	static int hit_first_bp;

#ifdef RDBGDEBUG
	printf1( "debug_exc( %d, 0x%x ) pc = 0x%x\n",
		signal, (long)eframe, (long)eframe->regs[ R_EPC ] );
#endif

	(void)fixup_brkpts();
	if ( brkpt_cont ) {
		brkpt_cont = 0;
		if ( signal == SIGTRAP )
			return;
	}

	if ( signal==SIGTRAP && eframe->regs[ R_EPC ] == (long)&breakpoint ) {
		eframe->regs[ R_EPC ] += sizeof(inst_t);
		if ( hit_first_bp ) {
			prepare_resume_reply( gdbbuf, signal );
			gdb_putp( gdbbuf );
		}
		else
			hit_first_bp++;
	}
	else {
		prepare_resume_reply( gdbbuf, signal );
		gdb_putp( gdbbuf );
	}

	for (;;) {
		gdb_getp( gdbbuf );

#ifdef RDBGDEBUG
		if ( debug_gdb )
			printf1( "Got debug cmd: '%s'\n", gdbbuf );
#endif

		switch( gdbbuf[0] ) {
		case '?':
			prepare_resume_reply( gdbbuf, signal );
			break;
		case 'g':
			bp = gdbbuf;
			for (i=0; i<NUM_REGS; i++) {

				/* XXX: gdb only gets 32 bits of all the
				 * registers.
				 */

				u_char	*rp =
				    (u_char *) & eframe->regs[ i ] +
					(REGSIZE-4);

				for (j=0; j<sizeof(reg_t); j++) {
					*bp++ = tohex( ((*rp)>>4) & 0x0f );
					*bp++ = tohex( (*rp) & 0x0f );
					rp++;
				}
			}
			*bp = '\0';
			break;
		
		case 'G':
			bp = gdbbuf+1;
			i=0;
			while ( i<NUM_REGS && *bp != '\0' ) {
				regval = readhexint( &bp, 2*REGSIZE );
				eframe->regs[ i++ ] = regval;
			}
			OKAY(gdbbuf);
			break;
		
		case 'P':
			bp = gdbbuf+1;
			i = readhexint( &bp, -1 );
			if ( *bp++ != '=' )
				goto gdb_input_err;
			regval = readhexint( &bp, 2*REGSIZE );
			eframe->regs[ i ] = regval;
			OKAY(gdbbuf);
			break;

		case 'm':
			bp = gdbbuf+1;
			addr = readhexint( &bp, -1 );
			if ( *bp++ != ',' )
				goto gdb_input_err;
			len = readhexint( &bp, -1 );
			if ( *bp++ != '\0' )
				goto gdb_input_err;
			
			if ( len*2 >= MAXGDBPKT-2 )
				goto gdb_input_err;
			
			bp = gdbbuf;
			if ( (addr&3) == 0 && (len&3) == 0 ) {
			    for (i=0; i<len; i += 4) {
				if ( read_mem_word( (u_int*) addr, &lval ) ) {
				    prepare_error_reply( gdbbuf, 14 /*EFAULT*/);
				    goto gdb_err2;
				}
				for (j=0; j<4; j++) {
				    *bp++ = tohex( (lval>>28) &0x0f );
				    *bp++ = tohex( (lval>>24) & 0x0f );
				    lval <<= 8;
				}
				addr += 4;
			    }
			}
			else {
			    for (i=0; i<len; i++) {
				if ( read_mem_byte( (u_char*)addr, &bval ) ) {
				    prepare_error_reply( gdbbuf, 14 /*EFAULT*/);
				    goto gdb_err2;
				}
				*bp++ = tohex( (bval>>4) & 0x0f );
				*bp++ = tohex( bval & 0x0f );
				addr++;
			    }
			}
			*bp ='\0';
			break;

		case 'M': {
			u_long	addr0, len0;

			bp = gdbbuf+1;
			addr = readhexint( &bp, -1 );
			if ( *bp++ != ',' )
				goto gdb_input_err;
			len = readhexint( &bp, -1 );
			if ( *bp++ != ':' )
				goto gdb_input_err;
			
			/* Save these so we can fix cache. */
			addr0 = addr;
			len0 = len;

			while ( len > 0 ) {

			   if ( (addr&3) == 0 && (len&3) == 0 ) {
				lval = readhexint( &bp, 8 );
				if ( write_mem_word( (u_int*) addr, lval ) ) {
					prepare_error_reply( gdbbuf,
						14 /*EFAULT*/);
					goto gdb_err2;
				}

				addr += 4;
				len -= 4;
			   }
			   else {
				bval = (u_char) readhexint( &bp, 2 );
				if ( write_mem_byte( (u_char*) addr, bval ) ) {
					prepare_error_reply( gdbbuf,
						14 /*EFAULT*/);
					goto gdb_err2;
				}
				addr++;
				len--;
			   }
			}

			/* gdb uses the M command to set breakpoints.
			 * fix cache.
			 */
			wbinval_dcache( (inst_t*) addr0, (int)len0 );
			invalidate_icache( (inst_t*) addr0, (int)len0 );

			OKAY(gdbbuf);

		}
			break;

		case 'q':
			/* query command. */
			if ( strcmp( &gdbbuf[1], "Offsets" ) == 0 ) {
				strcpy( gdbbuf, "Text=0;Data=0;Bss=0" );
				break;
			}
			prepare_error_reply( gdbbuf, 99 /* XXX */ );
			break;
		
		case 'k':
			/* Just die */
			OKAY( gdbbuf );
			gdb_putp( gdbbuf );
#ifdef RDBGDEBUG
			printf1( "Program killed by gdb\n" );
#endif
			for (;;)
				;
			/*NOTREACHED*/
			break;
		
		case 'c':
			if ( gdbbuf[1] != '\0' ) {
				bp = &gdbbuf[1];
				eframe->regs[ R_EPC ] = readhexint( &bp, -1 );
			}

			cont( eframe );

#ifdef RDBGDEBUG
			if ( debug_gdb )
				printf1( "Continuing at address 0x%x\n",
					eframe->regs[ R_EPC ] );
#endif

			return;

		case 's':
			if ( gdbbuf[1] != '\0' ) {
				bp = &gdbbuf[1];
				eframe->regs[ R_EPC ] = readhexint( &bp, -1 );
			}

			step1( eframe );

#ifdef RDBGDEBUG
			if ( debug_gdb )
				printf1( "Stepping from address 0x%x\n",
					eframe->regs[ R_EPC ] );
#endif

			return;

		case 'H':
			/* Can't really do anything with H command. */
			OKAY( gdbbuf );
			break;
		
		case 'd':
		case 't':
		case 'Q':
		case 'C':
		case 'S':
#ifdef RDBGDEBUG
			printf1( "Unimplemented debug command: '%c'\n",
				gdbbuf[0] );
#endif
			/*FALLTHROUGH*/
		gdb_input_err:
			prepare_error_reply( gdbbuf, 22 /*EINVAL*/ );
			break;

		default:
			/* "reserved" commands get null response */
			gdbbuf[0] = '\0';
			break;
		}

	gdb_err2:

		gdb_putp( gdbbuf );
	}
	/*NOTREACHED*/
}

/* Allow a program to put console messages through the gdb
 * interface.  Note that printf() uses gdbbuf.  If we hit any
 * exceptions (other than ordinary interrupts) during a printf(),
 * the printf() won't complete properly because gdbbuf will be
 * wiped out.  Hopefully we won't have to debug too many problems
 * in this routine.
 *
 * Whatever you do, you don't want to single-step through this routine!!
 */
void
printf( char *fmt, ... )
{
	gdbbuf[0] = 'O';
	vasprintf( gdbbuf+1, fmt, (long *)(&fmt + 1) );
	serial_on();
	gdb_putp( gdbbuf );
	serial_off();
}

#endif /* GDB */
