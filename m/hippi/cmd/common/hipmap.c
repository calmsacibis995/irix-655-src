/*
 * hipmap.c
 *
 * $Revision: 1.8 $
 *
 * Usage:
 *	hipmap [-D] [ -f <filename> ]
 *	hipmap [-d] <hostname> <I-field> [<ula>]
 *	hipmap -a
 *
 *	-D	flush the table before starting
 *	-d	delete entry or entries
 *
 *	The final parameters are either a single IP/I-field entry or
 *	the name of a file that has multiple IP/I-field entries.
 *
 *	hipmap -a prints the entire table as it is configured in the kernel.
 *
 */

static char *usage =
"%s: usage:\n\
\thipmap [-D] [ -f <filename> ]\n\
\thipmap [-d] <hostname> <I-field> [<ula>]\n\
\thipmap -a [-n]\n";

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/raw.h>
#include <net/soioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "sys/hippi.h"
#include "sys/if_hip.h"

#define DEFAULT_INPUT_FILE	"/usr/etc/hippi.imap"

static int nonames = 0;	/* print hostnames by default */
static int get_raw_socket();

void
print_harp_entry( struct in_addr addr, u_int I, u_char *ula, u_char flags )
{
	char	*hname = "";

	if ( nonames )
		hname = inet_ntoa(addr);
	else {
		struct hostent *hoste;
		hoste = gethostbyaddr( (void *) &addr, sizeof(addr), AF_INET );
		if ( ! hoste )
			hname = inet_ntoa(addr);
		else
			hname = hoste->h_name;
	}

	printf("%-30.30s\t0x%08X", hname, I );
	if ( (int)ula[0]+ula[1]+ula[2]+ula[3]+ula[4]+ula[5] != 0 )
		printf("\t%x:%x:%x:%x:%x:%x\n",
			ula[0], ula[1], ula[2], ula[3], ula[4], ula[5] );
	else
		printf("\n");
	
	/* later...possibly for HIPPI ARP support...
	 *
	 * if ( flags & HTF_PERM )
	 *	printf("PERM ");
	 * if ( flags & HTF_SRCROUTE )
	 *	printf("SRCROUTE ");
	 */

}



u_long
harp_dolookup( char *hostname )
{
	u_long	saddr;
	struct hostent *he;

	if ( hostname[0] >= '0' && hostname[0] <= '9' ) {
		saddr = inet_addr( hostname );
		if ( saddr == INADDR_NONE ) {
			fprintf(stderr,"hipmap: malformed address name: %s\n",
				hostname );
			exit(1);
		}
		else
			return saddr;
	}
	else {
		he = gethostbyname( hostname );
		if (!he) {
			fprintf(stderr,
			    "hipmap: warning: couldn't resolve name: %s\n",
			    hostname );
			return 0;
		}
		return *( (u_long *) he->h_addr );
	}
	/*NOTREACHED*/
}


void
harp_all()
{
	int	sock;
	off_t	offset;
	struct	harptab *ht;
	register int i;

	/* Get a raw socket so we can make request.
	 */
	sock = get_raw_socket();

	ht = (harptab_t *) malloc( sizeof(harptab_t)*HARPTAB_SIZE );
	if ( ioctl( sock, SIOCGHARPTBL, ht ) < 0 )
		perror( "hipmap: trouble reading harptable" ),exit(1);

	printf("%-30.30s\t%s\t\t%s\n", "Address", "IFIELD", "ULA" );

	for (i=0; i<HARPTAB_SIZE; i++)
		if ( ht[i].ht_iaddr.s_addr )
			print_harp_entry( ht[i].ht_iaddr, ht[i].ht_I,
				ht[i].ht_ula, ht[i].ht_flags );
	
	free( ht );
}

void
harp_flush()
{
	int	sock;
	off_t	offset;
	struct	harptab *ht;
	struct harpreq req;
	struct sockaddr_in *sin;
	register int i;

	/* Get a raw socket so we can make request.
	 */
	sock = get_raw_socket();

	ht = (harptab_t *) malloc( sizeof(harptab_t)*HARPTAB_SIZE );
	if ( ioctl( sock, SIOCGHARPTBL, ht ) < 0 )
		perror( "hipmap: trouble reading harptable" ),exit(1);
	
	bzero( &req, sizeof(req) );
	sin = (struct sockaddr_in *) &req.harp_pa;
	sin->sin_family = AF_INET;

	for (i=0; i<HARPTAB_SIZE; i++)
		if ( ht[i].ht_iaddr.s_addr ) {
			sin->sin_addr.s_addr = ht[i].ht_iaddr.s_addr;
			if ( ioctl( sock, SIOCDHARP, &req ) < 0 )
				perror( "hipmap: trouble flushing harp entry" ),
				exit(1);
		}
	
	free( ht );
}

void
harp_file( char *filename, int deleteflag )
{
	FILE	*fin;
	char	*c, *c2;
	char	line[2048], hostname[2048];
	int	temp[6], i;
	unsigned int j;
	struct harpreq req;
	struct sockaddr_in *sin = (struct sockaddr_in *) &req.harp_pa;
	int	sock;

	if ( strcmp( filename, "-" ) == 0 )
		fin = stdin;
	else
		fin = fopen( filename, "r" );
	if (fin==NULL)
		perror("couldn't open input file"),exit(1);
	
	while ( ! feof(fin) ) {
		if ( fgets( line, sizeof(line), fin ) == NULL )
			break;
		
		/* Remove everything after a pound sign or newline.
		 */
		c = line;
		while ( *c != '\0' && *c != '#' && *c != '\n' && *c != '\r' )
			c++;
		*c = '\0';

		/* Skip leading white-space.
		 */
		c = line;
		while ( *c != '\0' && isspace(*c) )
			c++;
		
		/* Ignore blank lines.
		 */
		if ( *c == '\0' )
			continue;
		
		/* get hostname
		 */
		i=0;
		while ( *c != '\0' && ! isspace(*c) )
			hostname[i++] = *c++;
		hostname[i] = '\0';
		if ( *c == '\0' )
			fprintf(stderr,"hipmap: malformed line: %s\n", line ),
			exit(1);
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = harp_dolookup( hostname );

		if ( ! sin->sin_addr.s_addr )
			continue;

		/* Skip space.
		 */
		while (*c != '\0' && isspace(*c) )
			c++;

		/* Get I-field
		 */
		req.harp_swaddr = strtoul( c, &c2, 16 );
		if ( c == c2 && ! deleteflag )
		    fprintf(stderr,"hipmap: malformed I-field in line: %s\n",
				line ),exit(1);
		c = c2;

		/* Skip space.
		 */
		while ( *c != '\0' && isspace(*c) )
			c++;
		
		/* Get ULA (if present)
		 */
		req.harp_ula.sa_family = AF_UNSPEC;
		if ( *c != '\0' ) {
		   for (i=0; i<HIPPI_ULA_SIZE; i++) {
			j = strtoul( c, &c2, 16 );
			if (c == c2 || j > 0xFF)
			   fprintf(stderr, "hipmap: malformed ULA in line: %s\n",
			   	   line ),exit(1);
			req.harp_ula.sa_data[i] = j;
			c = c2;
			if (*c == ':')
				c++;
		   }
		}
		else
			bzero( req.harp_ula.sa_data, HIPPI_ULA_SIZE );

		/* Skip space.
		 */
		while (*c != '\0' && isspace(*c) )
			c++;

		/* Get any options.
		 */
		req.harp_flags = HTF_PERM;
		while ( *c != '\0' ) {
			/* Find end of word.
			 */
			c2 = c;
			while ( *c2 != '\0' && !isspace(*c2) )
				c2++;
			if ( *c2 != '\0' )
				*c2++ = '\0';

			if ( strncasecmp( c, "src", 3 ) == 0 )
				req.harp_flags |= HTF_SRCROUTE;
			else if ( strncasecmp( c, "sourc", 5 ) == 0 )
				req.harp_flags |= HTF_SRCROUTE;
			else if ( strncasecmp( c, "tmp", 3 ) == 0 )
				req.harp_flags &= ~HTF_PERM;
			else if ( strncasecmp( c, "temp", 4 ) == 0 )
				req.harp_flags &= ~HTF_PERM;
			else
				fprintf(stderr,"hipmap: unknown option: %s in line: %s\n", c, line ),exit(1);
			
			/* Go to next option.
			 */
			c = c2;
			while ( *c != '\0' && isspace(*c) )
				c++;
		}

		/* Get a raw socket so we can make request.
		 */
		sock = get_raw_socket();
		if ( ! deleteflag ) {
			if ( ioctl( sock, SIOCSHARP, &req ) < 0 )
				perror( "hipmap: couldn't SIOCSHARP" ),exit(1);
		}
		else {
			if ( ioctl( sock, SIOCDHARP, &req ) < 0 )
				perror( "hipmap: couldn't SIOCDHARP" );
			exit(1);
			/*NOTREACHED*/
		}
		close(sock);
	}
}

void
harp_getula( char *ascii_ula, u_char *ula )
{
	register int i;
	int	temp[6];

	if ( sscanf( ascii_ula, "%x:%x:%x:%x:%x:%x",
		&temp[0], &temp[1], &temp[2],
		&temp[3], &temp[4], &temp[5] ) != 6 )
		fprintf(stderr,"hipmap: malformed ULA: %s\n", ascii_ula ),exit(1);
	
	for (i=0; i<6; i++)
		if ( temp[i] < 0 || temp[i] > 255 )
			fprintf(stderr,"hipmap: malformed ULA: %s\n", ascii_ula ),
			exit(1);
		else
			ula[i] = (u_char) temp[i];
}

u_long harp_getI( char *ascii_I )
{
	u_long	I;

	if ( ascii_I[0] == '0' && ascii_I[1] == 'x' )
		ascii_I += 2;
	
	if ( sscanf( ascii_I, "%x", &I ) != 1 )
		fprintf(stderr,"hipmap: malformed switch address.\n"),exit(1);
	
	return I;
}





/*
 * This routine gets a raw socket descriptor that we can
 * use to send ioctl() requests to the interface.
 */
/* On Challenge systems, the first unit found is always hip0, but
 * on SN0, hwgraph/ioconfig numbering means that there may well
 * be gaps, so try hip0,hip1,...,hip31, in order, stopping at
 * the first success.
 */
static int
get_raw_socket()
{
	int	fd, i, j;
	struct sockaddr_raw	rawaddr;

	/* Create a raw socket bound to the interface.
	 */
	fd = socket( AF_RAW, SOCK_RAW, RAWPROTO_RAW );
	if (fd<0)
		perror("hipmap: couldn't get raw socket"),exit(1);
	rawaddr.sr_family = AF_RAW;
	rawaddr.sr_port = 0;
	bzero( rawaddr.sr_ifname, RAW_IFNAMSIZ );

	for (i = 0; i < 32; i++) {
	    sprintf (rawaddr.sr_ifname, "hip%d", i);
	    if (! (j = bind( fd, &rawaddr, sizeof(rawaddr) ) ) )
		break;
        }
	if ( j < 0 )
		perror("hipmap: couldn't bind socket"),exit(1);
	return fd;
}

harp_one_entry( int argc, char *argv[], int deleteflag )
{
	int	arg;
	struct harpreq req;
	struct sockaddr_in *sin = (struct sockaddr_in *) &req.harp_pa;
	int	sock;

	if ( argc < 3 && ! deleteflag || argc < 2)
		fprintf(stderr,usage,argv[0]),exit(1);
	
	arg = 1;
	req.harp_ula.sa_family = AF_UNSPEC;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = harp_dolookup( argv[arg++] );

	if ( ! sin->sin_addr.s_addr )
		return 1;

	if ( ! deleteflag ) {
		req.harp_swaddr = harp_getI( argv[arg++] );
		if ( argc > 3 )
		   harp_getula( argv[arg++], (u_char *) req.harp_ula.sa_data );
		else
		   bzero( (u_char *) req.harp_ula.sa_data, HIPPI_ULA_SIZE );
		req.harp_flags = HTF_PERM;

		printf("setting: %s %x:%x:%x:%x:%x:%x %08X\n",
			inet_ntoa( sin->sin_addr ),
			req.harp_ula.sa_data[0],
			req.harp_ula.sa_data[1],
			req.harp_ula.sa_data[2],
			req.harp_ula.sa_data[3],
			req.harp_ula.sa_data[4],
			req.harp_ula.sa_data[5],
			req.harp_swaddr
			);
		
		/* Get a raw socket so we can make request.
		 */
		sock = get_raw_socket();

		if ( ioctl( sock, SIOCSHARP, &req ) < 0 )
			perror( "hipmap: couldn't SIOCSHARP" ),exit(1);
		
		close( sock );
	}
	else {
		printf( "removing: %s\n", inet_ntoa( sin->sin_addr ) );

		/* Get a raw socket so we can make request.
		 */
		sock = get_raw_socket();

		if ( ioctl( sock, SIOCDHARP, &req ) < 0 )
			perror( "hipmap: couldn't SIOCSHARP" ),exit(1);
		
		close( sock );
	}

	return 0;
}

main(int argc, char *argv[] )
{
	int	c, errs = 0;
	int	flush=0, delflag=0, showall=0, filein=0;
	char	*filename = NULL;
	extern char *optarg;
	extern int optind, opterr;

	if ( geteuid() != 0 )
		fprintf( stderr, "%s: you must be superuser\n", argv[0] ),
		exit(1);

	if ( argc < 2 )
		fprintf(stderr,usage,argv[0]),exit(1);

	while ((c = getopt(argc, argv, "DdanNf:")) != EOF) switch (c) {
	case 'D': flush++;				break;
	case 'd': delflag++;				break;
	case 'a': showall++;				break;
	case 'n': nonames=1;				break;
	case 'f': filein++; filename = optarg;		break;
	case '?':
	default:  ++errs;
	}

	if (errs)
		fprintf(stderr,usage,argv[0]),exit(1);
	
	if ( showall )
		harp_all();
	else {
		if ( flush )
			harp_flush();
		
		if ( optind == argc && ! flush && ! filein )
			fprintf(stderr,usage,argv[0]),exit(1);
		else if ( filein )
			harp_file( filename, delflag );
		else if ( optind < argc )
			exit( harp_one_entry( argc-optind+1, &argv[optind-1],
				delflag ) );
	}

	exit(0);
}

