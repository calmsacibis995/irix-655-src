
/*
 * rdbg_cmdl.c
 *
 * This module does LINC debugging using a simple command-line interface.
 * Since all the debugging facilities are really quite independent of
 * wether we use gdb, kdbx, etc., this is just one more serial protocol,
 * i.e. one understandable by humans.  This module might come in handy
 * someday to debug simple problems.
 *
 * $Revision: 1.4 $
 */

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

#ifdef _LINC_FIRMWARE
#include "sys/PCI/linc.h"
#endif

static char outbuf[512], inbuf[256];

static void
rdbg_puts( char *buffer )
{
	while ( *buffer ) {
		if ( *buffer == '\n' ) {
			PutSerial( '\r' );
			PutSerial( '\n' );
		}
		else
			PutSerial( *buffer );
		buffer++;
	}
}

/* Allow a program to put console messages through the gdb
 * interface.  This routine is exported and should only be
 * used by the "victim", er, I mean, program being debugged.
 */
void
printf( char *fmt, ... )
{
	vasprintf( outbuf, fmt, (long *)(&fmt + 1) );
	serial_on();
	rdbg_puts( outbuf );
	SerialSync();
	serial_off();
}

/* Same as above but assumes serial port is actie. */
static void
rdbg_printf( char *fmt, ... )
{
	vasprintf( outbuf, fmt, (long *)(&fmt + 1) );
	rdbg_puts( outbuf );
}

static void
rdbg_getline( char *prompt, char *line, int maxline )
{
	register int	i = 0, c;

	rdbg_puts( prompt );

	for (;;) {
		c = GetSerial();
		if ( c == '\r' ) {
			PutSerial( '\r' );
			PutSerial( '\n' );
			line[ i ] = '\0';
			return;
		}
		else if ( c == '\n' )
			;
		else if ( c == '\b' ) {
			if ( i>0 ) {
				PutSerial( '\x08' );
				PutSerial( ' ' );
				PutSerial( '\x08' );
				i--;
			}
		}
		else if ( c == '\x03' ) {
			PutSerial( '\r' );
			PutSerial( '\n' );
			rdbg_puts( prompt );
			i = 0;
		}
		else if ( (c&127) >= 32 && i<maxline-1 ) {
			line[ i++ ] = c;
			PutSerial( c );
		}
		else  {
			PutSerial( '^' );
			PutSerial( c+64 );
		}
	}
}

static char *gprnames[] = {
	"r0 ", "at ", "v0 ", "v1 ",
	"a0 ", "a1 ", "a2 ", "a3 ",
	"t0 ", "t1 ", "t2", "t3",
	"t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3",
	"s4", "s5", "s6", "s7",
	"t8", "t9", "k0", "k1",
	"gp", "sp", "fp", "ra" };
static char *auxnames[] = {
	"EPC", "CAUSE", "BADVADDR", "MDHI", "MDLO",
	"", "", "SR", "COUNT", "COMPARE", "ERREPC", 0 };
static char *excodenames[] = {
	"Interrupt", "-", "IBound", "DBound", "Addr Exc Load",
	"Addr Exc Store", "Bus Err I", "Bus Err D", "Syscall",
	"Break", "Rsvd Instr", "Cop U", "Arith OVFlw", "Trap",
	"-", "FP Exc", "-", "-", "-", "-", "-", "-", "-",
	"Watch", "-","-","-","-","-","-","-","-" };

static void
rdbg_dumpregs( eframe_t *eframe, int verbose )
{
	int	i;

	if ( verbose )
	    for (i=0; auxnames[i]; i++) {
		if ( auxnames[i][0] != '\0' )
			rdbg_printf( "%s: 0x%x\n", auxnames[i],
				(long) eframe->regs[ i+R_EPC ] );
		if ( i+R_EPC == R_CAUSE )
			rdbg_printf( "   EXC_CODE: %s\n", excodenames[
				( eframe->regs[ R_CAUSE ] &
				  CAUSE_EXCMASK ) >> CAUSE_EXCSHIFT ] );
	    }
	for (i=0; i<32; i++) {
		rdbg_printf( "r%d/%s: 0x%x", i, gprnames[i],
			(long) eframe->regs[ i ] );
		if ( (i&3) == 3 ) {
			PutSerial( '\r' );
			PutSerial( '\n' );
		}
		else {
			PutSerial( ' ' );
			PutSerial( ' ' );
		}
	}
}

static u_int
rdbg_getval( char **cpp )
{
	char	c;
	u_int	val = 0;

	while ( **cpp == ' ' || **cpp == '\t' )
		++*cpp;

	if ( **cpp == '0' && *(*cpp+1) == 'x' ) {
		*cpp += 2;
	
		for (;;) {
			c = **cpp;
			if ( c >= '0' && c <= '9' ) {
				val = (val<<4) | (c-'0');
				++*cpp;
			}
			else if ( c >= 'a' && c <= 'f' ) {
				val = (val<<4) | (c-'a'+10);
				++*cpp;
			}
			else if ( c >= 'A' && c <= 'F' ) {
				val = (val<<4) | (c-'A'+10);
				++*cpp;
			}
			else
				break;
		}
	}
	else {
		for (;;) {
			c = **cpp;
			if ( c >= '0' && c <= '9' ) {
				val = val*10 + (c-'0');
				++*cpp;
			}
			else
				break;
		}
	}

	return val;
}

void
rdbg_dumpmem( u_int *addr, int n )
{
	int	i;
	u_int	val;

	for (i=0; i<n; i++) {
		if ( i==0 || ((long)addr&0x0c) == 0 )
			rdbg_printf( "0x%x: ", (long)addr );

		if ( read_mem_word( (u_int*)addr, &val ) ) {
			rdbg_printf( "Got a fault reading addr 0x%x\n",
				addr );
			return;
		}

		rdbg_printf( "0x%x ", val );

		if ( i==n-1 || ((long)addr&0x0c) == 0x0c )
			rdbg_puts( "\n" );

		addr++;
	}
}

void
debug_exc( int signal, eframe_t *eframe )
{
	int	n;
	char	*cp;
	u_int	addr, val;

	fixup_brkpts();
	if ( brkpt_cont ) {
		brkpt_cont = 0;
		if ( signal == SIGTRAP )
			return;
	}

	rdbg_printf( "\nLINC debugger hit exception #%d at PC: 0x%x !\n",
		signal, (long)eframe->regs[ R_EPC ] );
	rdbg_printf( "  CAUSE: 0x%x <EXC_CODE=%s>\n",
		(long) eframe->regs[ R_CAUSE ],
		excodenames[ ( eframe->regs[ R_CAUSE ] &
			  CAUSE_EXCMASK ) >> CAUSE_EXCSHIFT ] );
	
	if ( signal==SIGTRAP && eframe->regs[ R_EPC ] == (long)&breakpoint )
		eframe->regs[ R_EPC ] += sizeof(inst_t);

	for (;;) {

		rdbg_getline( "LDBG: ", inbuf, sizeof(inbuf) );

		/* Skip spaces */
		cp = inbuf;
		while ( *cp == ' ' || *cp == '\t' )
			cp++;
		
		/* What command? */
		switch ( *cp ) {

		case '\0':
			/* No command */
			break;

		case 'c':
			/* Continue */
			addr = rdbg_getval( &cp );
			if ( addr )
				eframe->regs[ R_EPC ] = addr;
			cont( eframe );
			rdbg_printf( "LINC debugger continuing from 0x%x\n",
				(long) eframe->regs[ R_EPC ] );
			return;
		
		case 's':
			/* single-step */
			addr = rdbg_getval( &cp );
			if ( addr )
				eframe->regs[ R_EPC ] = addr;
			step1( eframe );
			rdbg_printf( "LINC debugger stepping from 0x%x\n",
				(long) eframe->regs[ R_EPC ] );
			return;
		
		case 'r':
		case 'R':
			/* Dump registers */
			rdbg_dumpregs( eframe, *cp == 'R' );
			break;
		
		case 'p':
			/* Put a value */
			cp++;
			addr = rdbg_getval( &cp );
			val = rdbg_getval( &cp );
			if ( addr == 0 )
				rdbg_printf( "Usage: p <addr> <val>\n" );
			else if ( write_mem_word( (u_int*)addr, val ) )
				rdbg_printf( "Got a fault writing addr 0x%x\n",
					addr );
			break;
		case '0':
			/* Print a value */
			addr = rdbg_getval( &cp );
			if ( addr == 0 ) {
				rdbg_printf( "Usage: <addr> <count>\n" );
				break;
			}
			n = rdbg_getval( &cp );
			if ( n == 0 )
				n = 1;

			rdbg_dumpmem( (u_int*)addr, n );

			break;
		default:
			rdbg_printf( "Unknown command: %c\n", *cp );
		case '?':
		case 'h':
			rdbg_printf( "\tr\t\t- dump registers.\n" );
			rdbg_printf( "\tR\t\t- dump all registers.\n" );
			rdbg_printf( "\tc\t\t- continue.\n" );
			rdbg_printf( "\tp <addr> <val>\t - put a value.\n" );
			rdbg_printf( "\t<addr> [n]\t- dump n words\n" );
			break;
		}
	}
	/*NOTREACHED*/
}

