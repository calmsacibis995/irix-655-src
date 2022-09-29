/*
 * hiptest.c
 *
 * hiptest [-I<i-field>]  [-D/dev/hippi[0-3] ] [maxsize [n] ]
 *
 * Test HIPPI-FP interface data integrity.  Great loop-around test.
 *
 * Copyright 1994 Silicon Graphics, Inc.  All rights reserved.
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/errno.h>

#include "sys/hippi.h"

#define DEVICE_FILE	"/dev/hippi0"

extern char *optarg;
extern int optind, opterr;

extern int errno;

#define MIN(x,y)	((x)<(y) ? (x) : (y) )
#define MAX(x,y)	((x)>(y) ? (x) : (y) )

int mode = 0;
/* mode values */
#define LOOPBACK 0
#define SEND	 1
#define RECV	 2

u_int	len,
	num_pkts = 64;
u_int	*buf1, *buf2;
int	child;
char	*device_name = DEVICE_FILE;
u_int	ifield = -1,
	seed;
int	ulp = 0x89;
int	d1size = 8;
int	fast = 0;
int	verbose = 0;
int	count_pattern = 0;
int	fast_kill = 0;
int	static_len = 0;


static char *src_err_name[] =
{ "no error", "SRC sequence error", "SRC lost DSIC",
  "SRC timeout", "SRC lost connect",
  "SRC connect rejection", "HIPPI interface shutdown",
  "SRC parity error"};
 
static char *dst_err_name[] =
{ "DST Parity Error", "DST LLRC Error", "DST Sequence Error",
  "DST Sync Error", "DST Illegal Burst", "DST SDIC loss",
  "DST Framing Error", "DST lost linkrdy in packet",
  "DST No Packet Signal in Connection", 
  "DST Data Received with no READY's sent",
  "DST No Burst in Packet",
  "DST Illegal State Transition"};
#define MAX_DST_ERR_NAMES 12

/* called due to SIGALARM-- times out reads */
void
rdtimeo( int sig )
{
    /* This will also cause read to fail with EINTR */
    fprintf(stderr, "hiptest: read time-out\n" );
}


void
send_loop() {
    int i, j, len2,
	fd_wr, retv;

    fd_wr = open( device_name, O_WRONLY );
    if ( fd_wr < 0 )
	perror( "hiptest(SRC): couldn't open hippi device" ),exit(1);
		  
    /* Set ULP */
    if ( ioctl( fd_wr, HIPIOC_BIND_ULP, ulp ) < 0 )
	perror( "hiptest: couldn't bind fd_wr to ULP" ),exit(1);

    /* Set I-field */
    if ( ioctl( fd_wr, HIPIOCW_I, ifield ) < 0 )
	perror( "hiptest(SRC): couldn't set I-field" );

    /* Set D1 Size header */
    if ( ioctl( fd_wr, HIPIOCW_D1_SIZE, d1size ) < 0 )
	perror( "hiptest(SRC): couldn't set D1_SIZE hdr" );

    /* if fast, init check buffer just once, not every pkt.
     * Note that we loose the per packet count on data.
     */
    if ( fast ) {
	seed++;
	srand48( seed );
	for (j=0; j<len/4; j++)
	    if (count_pattern || (mode != LOOPBACK))
		buf1[ j ] = j;
	    else
		buf1[ j ] = mrand48();
    }
		
    for (i=0; i<num_pkts; i++) {
	if (static_len)
	    len2 = len;
	else {
	    seed++;
	    srand48( seed );
	    do {
		len2 = (lrand48()%(len-7)) & ~7;
	    } while ( len2 < 8 || len2 < d1size );
	}

	/* initialize the check buffer for each pkt if not fast */
	if ( ! fast )
	    for (j=d1size/4; j<len2/4; j++)
		if (count_pattern || (mode != LOOPBACK))
		    buf1[ j ] = (i<<24) | ( j - d1size/4 );
		else
		    buf1[ j ] = mrand48();
			
	if ( verbose )
	    printf("hiptest(SRC): #%d transmitting len %d\n",
		   i, len2+8 );
			
	retv = write( fd_wr, buf1, len2 );
	if ( retv != len2 ) {
	    fprintf( stderr, "hiptest(SRC): %d: write return value: %d\n",
		     i, retv );
	    perror( "hiptest(SRC): trouble writing" );
	    if ( retv < 0 && errno == EIO ) {
		retv = ioctl( fd_wr, HIPIOCW_ERR );
		if ( retv < 0 )
		    perror( "hiptest(SRC): trouble doing HIPIOCW_ERR" );
		else
		    fprintf( stderr,
			     "hiptest(SRC): HIPIOCW_ERR: %d: %s\n",
			     retv, src_err_name[ retv ] );
	    }
	    exit(1);
	}

	if (mode != LOOPBACK) {
	    if ( verbose )
		printf("hiptest(SRC): #%d sent len %d\n",
		       i, len2+8 );
	    else {
		printf(".");
		fflush( stdout );
		if ( (i & 63) == 63 || i == num_pkts-1 )
		    printf("\nhiptest(SRC): sent %d\n",
			   i+1 );
	    }
	}
    }
}


void
recv_loop(int fd_rd) {
    int i, j, len2, retv;

    signal( SIGALRM, rdtimeo );

    /* We've already done the open for the read only device so just set
       everything up. */
    
    /* Set ULP */
    if ( ioctl( fd_rd, HIPIOC_BIND_ULP, ulp ) < 0 ) {
	perror( "hiptest(DST): couldn't bind fd_rd to ULP" );
	kill( child, SIGINT );
	exit(1);
    }

    /* If we're not loopback no randoms are necessary. */
    if (fast && (mode == LOOPBACK)) {
	seed++;
	srand48( seed );
    }

    for (i=0; i<num_pkts; i++) {

	/* If we're not loopback we have to get the lengths from the
	   FP header. */
	if (mode == LOOPBACK) {
	    if (static_len)
		len2 = len;
	    else {
		seed++;
		srand48( seed );
		do {
		    len2 = (lrand48()%(len-7)) & ~7;
		} while ( len2 < 8 || len2 < d1size );
	    }
	}

	if ( ! fast )
	    for (j=0; j<(len2-d1size)/4; j++)
		if (count_pattern|| (mode != LOOPBACK))
		    buf1[ j ] = (i<<24) | j;
		else
		    buf1[ j ] = mrand48();

	if (mode == LOOPBACK)
	    alarm( 10 );

	retv = read( fd_rd, buf2, 1024 ); /* should be header */
	if ( retv < 0 ) {
	    fprintf( stderr, "hiptest(DST): %d: ", i );
	    perror( "trouble reading header" );
	    if ( errno == EIO ) {
		retv = ioctl( fd_rd, HIPIOCR_ERRS );
		fprintf( stderr,
			 "HIPPI DST errs: 0x%x\n", retv );
		j=0;
		while ( retv > 0 ) {
		    if (j == MAX_DST_ERR_NAMES) {
			fprintf(stderr,"invalid dst "
				"error code\n");
			break;
		    }
		    if ( (retv&1) )
			fprintf( stderr, "\t%s\n",
				 dst_err_name[j] );
		    retv = (u_int)retv >> 1;
		    j++;
		}
					
	    }
	    kill( child, SIGINT );
	    exit(1);
	}

	/* If we're not loopback we have to get the values from the
	   FP header. */
	if (mode != LOOPBACK) {
	    hippi_fp_t *fp_hdr = (hippi_fp_t *)buf2;

	    d1size = ((fp_hdr->hfp_d1d2off >> 3) & 0xff) * 8;
	    len2 = d1size + fp_hdr->hfp_d2size;
	}

	if ( retv != 8+d1size ) {
	    fprintf( stderr, "hiptest(DST): %d: header is %d long!?\n",
		     i, retv );
	    kill( child, SIGINT );
	    exit(1);
	}
	if ( verbose > 1 ) {
	    printf( "d1 area:\n" );
	    for (j=0; j<retv/sizeof(u_int); j++)
		printf("%2d: %08X\n", j, buf2[j] );
	}

	/* If D2 body to read */
	if ( len2 > d1size ) {

	    bzero( buf2, len );

	    alarm( 10 );

	    retv = read( fd_rd, buf2, len-8 );
	    if ( retv < 0 ) {
		fprintf( stderr," hiptest(DST): %d: ", i );
		perror( "trouble reading body" );
		if ( errno == EIO ) {
		    retv = ioctl( fd_rd, HIPIOCR_ERRS );
		    fprintf( stderr,
			     "HIPPI DST errs: 0x%x\n", retv );
		    j = 0;
		    while ( retv > 0 ) {
			if (j == MAX_DST_ERR_NAMES) {
			    fprintf(stderr,"invalid dst "
				    "error code\n");
			    break;
			}
			if ( (retv&1) )
			    fprintf( stderr, "\t%s\n",
				     dst_err_name[j] );
			retv = (u_int)retv >> 1;
			j++;
		    }
		}
		kill( child, SIGINT );
		exit(1);
	    }
			
	    if ( len2-d1size != retv ) {
		fprintf( stderr, "hiptest(DST): %d: length error: retv=%d len2=%d\n",
			 i, retv, len2 );
		kill( child, SIGINT );
		exit(1);
	    }
			
	    if (!fast && (mode == LOOPBACK)) 
		for (j=0; j<retv/4; j++)
		    if ( buf1[ j ] != buf2[ j ] ) {
			int	k;

			if(fast_kill)
			    kill( child, SIGINT );
			fprintf( stderr, "hiptest(DST): data integrity error at offset %08X\n", j*4 );
			fprintf( stderr, "hiptest(DST): packet %d: expecting %08X  got %08X\n",
				 i, buf1[ j ], buf2[ j ] );
			fprintf( stderr, "hiptest(DST): virtual address = %x, len = %d\n",
				 (int) &buf2[j], len2 );
					
			printf( "offset\t\texpect\t\tgot\t\txor\n" );
			for (k=MAX(0,j-5); k<MIN(j+5,retv/4); k++)
			    printf( "%08X\t%08X\t%08X\t%08X\n",
				    k*4, buf1[ k ],
				    buf2[ k ],
				    buf1[k]^buf2[k] );
			if(!fast_kill)
			    kill( child, SIGINT );
			exit(1);
		    }
	}
			
	if ( verbose )
	    printf("hiptest(DST): #%d received len %d\n",
		   i, len2+8 );
	else {
	    printf(".");
	    fflush( stdout );
	    if ( (i & 63) == 63 || i == num_pkts-1 )
		printf("\nhiptest(DST): received %d\n",
		       i+1 );
	}
    }
}


main(int argc, char *argv[] )
{
    int			fd_rd,
			max_platform_len,
			c, errs = 0;
    hippi_stats_t	hipstats;
    char		*usage = "Usage: %s [-I <i-field>]  [-D /dev/hippi[0-N] ] [-r] [-s] [maxsize [npkts] ]\n";



    while ((c = getopt(argc, argv, "lkvI:D:u:fH:n:crs")) != EOF) switch (c) {
    case 'I': ifield = strtoul( optarg, NULL, 16 );	break;
    case 'D': device_name = optarg;			break;
    case 'r': mode = RECV;				break;
    case 's': mode = SEND;				break;

	/* UNDOCUMENTED OPTIONS: */
    case 'u': ulp = strtoul( optarg, NULL, 16 );	break;
    case 'H': d1size = atoi( optarg );			break;
    case 'f': fast++;					break;
    case 'n': num_pkts = atoi( optarg );		break;
    case 'v': verbose++;				break;
    case 'c': count_pattern++;				break;
    case 'k': fast_kill++;				break;
    case 'l': static_len++;				break;

    case '?':
    default:  ++errs;
    }

    if ( errs )
	fprintf(stderr,usage,argv[0]),exit(1);
	
    fd_rd = open( device_name, O_RDONLY );
    if ( fd_rd < 0 ) {
	perror( "hiptest: couldn't open hippi device for reading" );
	exit(1);
    }
		
    /* verify which platform we're on */
    if ( ioctl( fd_rd, HIPIOC_GET_STATS, &hipstats ) < 0 ) {
	if ( errno == ENODEV )
	    fprintf(stderr,
		    "%s: HIPPI board is down\n", argv[0]), exit(1);
	else
	    perror("hipcntl: ioctl HIPIOC_GET_STATS failed"),exit(1);
    }

    /* XIO has a maximum write length of 16mb and HIO had a max of 2mb so set
       the max length appropriately. */
    max_platform_len = ((hipstats.hst_flags & HST_FORMAT_ID_MASK) == HST_XIO) ?
	(0x1000000 + 8) : (0x200000+8);

    /* init len to maximum platform will allow. */
    len = max_platform_len;

    if ( argc > optind )
	len = strtoul(argv[optind++], 0 , 0);
    if ( argc > optind )
	num_pkts = strtoul( argv[optind++], 0 , 0);
	
    if (len > max_platform_len) {
	fprintf(stderr, "%s: invalid maxsize for this platform: %d\n",
		argv[0], len);
	exit(1);
    }
	
    if ( len < d1size || len < 16 || (len & 7)) {
	fprintf(stderr, "%s: invalid maxsize %d\n", argv[0], len );
	fprintf(stderr,usage,argv[0]);
	exit(1);
    }

    if ( num_pkts < 1 ) {
	fprintf(stderr, "%s: invalid npkts %d\n", argv[0], num_pkts );
	fprintf(stderr,usage,argv[0]);
	exit(1);
    }

    if ( ulp < 0 || ulp > 0xff ) {
	fprintf(stderr, "%s: invalid ulp (0x%x)\n", argv[0], ulp );
	fprintf(stderr,usage,argv[0]);
	exit(1);
    }

    if ( d1size < 0 || d1size > 1016 ) {
	fprintf(stderr, "%s: invalid d1size (%d)\n", argv[0], d1size );
	fprintf(stderr,usage,argv[0]);
	exit(1);
    }

    if (( ifield == -1) && (mode != RECV)) {
	fprintf(stderr, "You must choose an I-Field to run %s.\n", argv[0]);
	fprintf(stderr, "Look at /usr/etc/hippi.imap for valid I-Field values.\n");
	fprintf(stderr,usage,argv[0]);
	exit(1);
    }
		

    buf1 = memalign( 8, len );
    buf2 = memalign( 8, len );

    printf( "hiptest: %s:\n", device_name);

    if (mode == LOOPBACK)
	printf("LOOPBACK mode\n");
    else if (mode == SEND)
	printf("SEND mode\n");
    else
	printf("RECEIVE mode\n");

    if (mode == RECV) {
	printf("reading %d packets...\n", num_pkts);
    }
    else if (static_len) {
	printf( "sending %d packets, size %d, to I-field 0x%08X\n",
		num_pkts, len, ifield );
    }
    else {
	printf( "sending %d packets, size range [%d..%d], to I-field 0x%08X\n",
		num_pkts, d1size == 0 ? 16 : d1size+8, len, ifield );
    }
	
    seed= time(0);


    if (mode == SEND)
	send_loop();
    else if (mode == RECV)
	recv_loop(fd_rd);
    else { /* We're in loopback mode. */
	if ( (child = fork()) == 0 ) {/* Child process -- sends */
	    sleep(2);	/* XXX: give reader time to bind */
	    send_loop();
	}
	else /* Parent process -- receives */
	    recv_loop(fd_rd);
    }

    if (mode == LOOPBACK)
	printf( "\nhiptest: %s: Successfully transferred %d HIPPI packets.\n",
		device_name, num_pkts );
    else if (mode == SEND)
	printf( "\nhiptest: %s: Successfully sent %d HIPPI packets.\n",
		device_name, num_pkts );
    else /* RECV mode */
	printf( "\nhiptest: %s: Successfully received %d HIPPI packets.\n",
		device_name, num_pkts );
	
    exit(0);
}

