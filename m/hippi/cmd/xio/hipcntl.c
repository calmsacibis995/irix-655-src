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
 * hipcntl.c
 *
 * Usage: hipctl [hippiN] {startup|shutdown|accept|reject|status|stimeo}
 *
 */

#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/errno.h>
#include "fwvers.h"
#include "sys/hippi.h"
#include "sys/hps_ext.h"
#include "sys/hippibp.h"
#include "hipcntl.h"
#include "lincprom.h"

/* see hipcntl.h for the definition */
#include DST_FW_FILE 
#include SRC_FW_FILE
#include DST_PROM_FILE
#include SRC_PROM_FILE

#ifdef HIP_DEBUG
int hip_debug = 5;
#endif

extern int errno;
int quiet = 0;

char device_name[64];
char bpdevice_name[64];

char *usage="Usage: %s [hippiN] <cmd> <parameters>\n" 
	"\t <cmd> must be one of:\n"
	"\t startup    (update firmware and start board) \n"
	"\t shutdown   (bring board down and reset it) \n" 
	"\t accept     (board starts accepting connections) \n"
	"\t reject     (board stops accepting connections) \n"
	"\t status     (get status of board) \n"
	"\t stimeo     (set source timeout value) \n"
	"\t getmac     (get MAC address) \n"
	"\t versions   (get version info)\n"
	"\t loopback   (put board in electrical loopback mode)\n"
        "\n"
	"\t bpulp      (set bypass ULP number) \n"
	"\t bpjobs     (set bypass job limit) \n"
	"\t bpports    (set bypass port limit) \n"
	"\t bpspages   (set bypass src page limit) \n"
	"\t bpdpages   (set bypass des page limit) \n"
	"\t bpstatus   (get bypass status of board) \n";


static struct cmd cmdtbl[] = {

	"startup",	hipstartup,	/* update firmware and start board */
	"bringup",	hipstartup,	/* same as startup: backwards compat */
	"shutdown",	hipshutdown,	/* bring board down and reset it */
	"download",	hipdownload,	/* force update of firmware */
	"accept",	hipaccept,	/* board starts accepting connections*/
	"reject",	hipreject,	/* board stops accepting connections */
	"status",	hipstatus,	/* get status of board */
	"stimeo",	hipstimeo,	/* set source timeout value */
	"getmac",	hipgetmac,	/* get MAC address */
	"versions",	hipgetversions, /* get version info */
	"loopback",	hiploopback,    /* put in loopback mode */
	/* Deliberately make prog_MAC dis-similar to getmac - we don't
	 * want anyone guessing at this one. */
	"prog_MAC",	hipsetmac,	/* get MAC address */
	0,0
};

extern struct cmd cmdbptbl[];

static char *hipstatnames[] = {
	"SRC connections           ", /* Line 0 */
	"SRC packets               ",
	"SRC rejects               ",
	"SRC xmit retry            ",
	"",
	"SRC glink reset           ",
	"SRC glink lost            ",
	"SRC time outs             ",
	"SRC connects lost         ",
	"SRC parity errs           ", /* Line 9 */
	"",
	"",
	"",
	"",
	"",                           /* Line 14 = HIPSRC_NB_BYTE_IDX */
	"SRC number bytes sent     ", /* Line 15 */

	"DST connections           ", /* Line 16 */
	"DST packets               ",
	"DST rcv on bad ulp        ",
	"DST hippi-le drop         ",
	"DST llrc                  ",
	"DST parity                ",
	"DST frame/state err       ",
	"DST flag err              ",
	"DST illegal burst         ",
	"DST link rdy lost in pkt  ",
	"DST null connections      ", /* Line 26 */
	"DST ready errors          ", 
	"DST bad packet starts     ",
	"",
	"",			      /* Line 30 = HIPDST_NB_BYTE_IDX */
	"DST number bytes received ", /* Line 31 */
	0
};

static char *hipflagnames[] = {
	"", "", "LOOPBACK", "",
	"ACCEPTING", "DST.PKT", "DST.REQ", "",
	"SRC.REQ", "SRC.CON", "", "",
	"", "", "", "",
	"DST.LNK_RDY", "DST.FSYNC", "DST.OH8SYNC", "DST.SIG_DET", 
	"", "", "", "",
	"SRC.LNK_RDY", "", "", "",
	NULL
};

void
hipstatus( int fd, int argc, char *argv[] )
{
	int     	    i;
	u_long  	    flags;
	unsigned long long  no_bytes;
	hippi_stats_t	    hipstats;
	int chars_print;

	if ( ioctl( fd, HIPIOC_GET_STATS, &hipstats ) < 0 ) {
		if ( errno == ENODEV )
			fprintf(stderr,
				"hipcntl: HIPPI board is down\n" ),exit(1);
		else
			perror( "hipcntl: couldn't get HIPPI statistics" ),exit(1);
	}
	
	flags = hipstats.hst_flags;
	
	if (flags & HST_XIO) 
	    printf("Origin HIPPI Serial XIO Adapter\n");
	else {
	    printf("Unknown status struct format code: %d\n", 
		   flags>>HST_FORMAT_ID_SHIFT);
	    exit(1);
	}
	printf( "FLAGS:\t" );
	i=0;
	chars_print = 0;
	while ( hipflagnames[i] ) {

		if ( flags & (1<<i) )
			chars_print += printf( " %s ", hipflagnames[i] );
		if (chars_print > 60) {
		        chars_print = 0;
			printf("\n\t");
		}
		i++;
	}
	printf( "\n" );
	i=0;
	while ( hipstatnames[i] ) {
		if ( i == HIPSRC_NB_BYTE_IDX )
		  {
		    no_bytes  = ( (u_long *) &hipstats.hst_s_conns )[i] ;
		    no_bytes  = no_bytes<<32;
		    i++;

		    no_bytes |= ( (u_long *) &hipstats.hst_s_conns )[i] ;
		    printf( "%s:\t%llu\n", hipstatnames[i], no_bytes);
		  }
		else 	if ( i == HIPDST_NB_BYTE_IDX )
		  {
		    no_bytes  = ( (u_long *) &hipstats.hst_s_conns )[i] ;
		    no_bytes  = no_bytes<<32;
		    i++;
		    no_bytes |= ( (u_long *) &hipstats.hst_s_conns )[i] ;
		    printf( "%s:\t%llu\n", hipstatnames[i], no_bytes);
		  }
		else  if ( *hipstatnames[i] != '\0' )
		  printf( "%s:\t%u\n", hipstatnames[i],
			 ( (u_long *) &hipstats.hst_s_conns )[i] );
		i++;
	}

}

void
erase_sectors(u_int offset,int  fd, int sflag, uint32_t size)
{
	int	         lowsector = -1, highsector = -1;
	u_int	         sectormask, i;

	/* Erase the correct sectors in the EEPROM.
	 */
	lowsector = offset/HIP_PROM_SECTOR_SIZE;
	highsector = (offset+size-1)/HIP_PROM_SECTOR_SIZE;
	if ( lowsector == highsector ) {
	    dprintf(2, ("Erasing Sector %d...", lowsector) );
	}
	else {
	    dprintf(2, ("Erasing Sectors [%d..%d]...",
		  lowsector, highsector) );
	}
	fflush( stdout );
	sectormask = 0;
	for (i=lowsector; i<=highsector; i++)
	  sectormask |= (1<<i);
	sectormask |= (sflag << 24);
	
	
#ifndef DEBUG
	if ( ioctl( fd, HIPPI_ERASE_FLASH, sectormask ) < 0 )
	  perror("trouble with HIPPI_ERASE_FLASH,"
	         "(might need to shutdown device)\n" ), exit(1);
#endif
}	

void
flash(int fd, u_long offset, uint32_t size, caddr_t data_p, int sflag, 
      int clear_reset)
{
	caddr_t	         firm_data2;
	hip_flash_req_t	 flasharg;
	uint32_t         i = 0;

	flasharg.offset      = offset;
	flasharg.len         = size;
	flasharg.data        = (uint64_t) data_p;
	flasharg.is_src      = sflag;
	flasharg.clear_reset = clear_reset;

#ifndef DEBUG
	if ( ioctl( fd, HIPPI_PGM_FLASH, &flasharg ) < 0 )
	    perror( "HIPPI_PGM_FLASH failed" ),exit(1);
#endif

	
	if( clear_reset ){
	    /* Verify that the data was written correctly.
	     */
	    firm_data2 = (caddr_t)malloc( size );
	
	    flasharg.offset      = offset;
	    flasharg.len         = size;
	    flasharg.data        = (uint64_t) firm_data2;
	    flasharg.is_src      = sflag;
#ifndef DEBUG
	    if ( ioctl( fd, HIPPI_GET_FLASH, &flasharg ) < 0 )
		perror( "HIPPI_GET_FLASH failed" ),exit(1);
#endif
	    for (i=0; i<size; i++)
		if ( data_p[i] != firm_data2[i] ) {
		    fprintf( stderr, "Verify failed:  offset 0x%x  "
			    "expected 0x%x  got 0x%x\n", offset+i,
			    data_p[i], firm_data2[i] );
		    break;
		}

	    free( firm_data2 );
	
	    if ( i >= size ) {
		dprintf(2, ("Verify passed.\n") ); 
	    }
	    else {
		dprintf(2, ("Verify !! FAILED !!.\n") );
		exit(1);
	    }
	}
}


void
write_lincprom(int fd, u_int start_addr, u_int entry, u_int size, int sflag, 
	       u_long offset)
{
	uint32_t         firm_size = size;
	
	dprintf(2, ("hipcntl: Download lincprom ") );
	if( sflag )
	  { dprintf(2, ("source: \n")); }
	else
	  { dprintf(2, ("destination: \n") ); }
	dprintf(5, ("\tStart Addr:\t0x%x\n",    start_addr) );
	dprintf(5, ("\tEntry pt:\t0x%x\n",      entry) );
	dprintf(5, ("\tSize:\t\t0x%x\n",        size) );
	dprintf(5, ("\tBurn location:\t0x%x\n", offset+HIP_PROM_BASE) );

	if ( offset+firm_size > HIP_PROM_SECTOR_SIZE )
	  fprintf( stderr,
"You'll run past end of sector 0 and clobber the MAC address!!\n" ),
	    exit(1);

	/* Erase the correct sectors in the EEPROM. */
	erase_sectors(offset, fd, sflag, size);
	
	dprintf(2, ("\nProgramming lincprom ...") );
	fflush( stdout );
	
	if(sflag)
	  flash(fd, offset, firm_size, (caddr_t) lincprom_src_txt, sflag, 1);
	else
	  flash(fd, offset, firm_size, (caddr_t) lincprom_dst_txt, sflag, 1);
}

void
write_lincfw(int fd, lincprom_fhdr_t  *Fhdr, caddr_t firm_data, int sflag, 
	      u_long offset )
{
	uint32_t	 cksum;	
	caddr_t	         firm_data2;
	hip_flash_req_t	 flasharg;
	u_int	         sectormask;
	int	         lowsector = -1, highsector = -1, i;
	
	dprintf(2, ("hipcntl: Download Firmware  ") );
	if( sflag ) {
	    dprintf(2, ("source: \n") );
	}
	else {
	    dprintf(2, ("destination: \n") );
	}
	dprintf(5, ("\tStart Addr:\t0x%x\n",    Fhdr->start_addr) );
	dprintf(5, ("\tEntry pt:\t0x%x\n",      Fhdr->entry) );
	dprintf(5, ("\tSize:\t\t0x%x\n",        Fhdr->size) );
	dprintf(5, ("\tBurn location:\t0x%x\n", offset+HIP_PROM_BASE) );
	
	/* Compute the checksum for the firmware and
	 * the Fhdr.
	 */
	cksum = 0;
	Fhdr->cksum = cksum;
	for (i=0; i<sizeof(lincprom_fhdr_t)/sizeof(uint32_t); i++) {
	  uint32_t d = ((uint32_t *)Fhdr)[i];
	  cksum += (d & 0xffff);
	  cksum += (d >> 16);
	  cksum = (cksum>>16) + (cksum&0xffff);
	}
	for (i=0; i<Fhdr->size/sizeof(uint32_t); i++) {
	  uint32_t d = ((uint32_t *)firm_data)[i];
	  cksum += (d & 0xffff);
	  cksum += (d >> 16);
	  cksum = (cksum>>16) + (cksum&0xffff);
	}
	
	cksum = (cksum>>16) + (cksum&0xffff);
	cksum = (cksum>>16) + (cksum&0xffff);
	
	Fhdr->cksum = cksum ^ 0xffff;
	
	/* Now make sure FHDR is included in calculations of
	 * firmware size.
	 */
	if ( (offset+Fhdr->size+sizeof(lincprom_fhdr_t))  > HIP_PROM_SIZE )
	  fprintf( stderr, "You'll run past end of EEPROM!!\n" ),
	    exit(1);
	
	/* Erase the correct sectors in the EEPROM. */
	erase_sectors(offset, fd,  sflag, Fhdr->size );
	
	dprintf(2, ("\nProgramming flash...") );
	fflush( stdout );
	
	/* Program the FHDR first.*/
	flash(fd, offset, sizeof(lincprom_fhdr_t),(caddr_t) Fhdr, sflag, 0);

	/* Programm the firmware */
	offset += sizeof(lincprom_fhdr_t);
	flash(fd, offset, Fhdr->size, firm_data, sflag, 1);
}


/* Downloads both the DST_FW_STR and the SRC_FW_STR file contents to the board */
void
hipdownload( int fd, int argc, char *argv[] )
{
	register int	 i, j;
	int              sflag;
	u_long           offset = 0;
	lincprom_fhdr_t  Fhdr;
	hippi_stats_t	 statbuf;


	if ( ( ioctl( fd, HIPIOC_GET_STATS, &statbuf ) == 0 ) ||
	   ( ( errno != ENODEV)))
		fprintf ( stderr,
		         "Interface is up, cannot program flash.\n" ),exit(1);
	
	/* Create and insert an FHDR for this firmware
	 * image.  This header is read by LINCPROM to
	 * know where the firmware should go in SDRAM.
	 */
	bzero( &Fhdr, sizeof(Fhdr) );

	/* Download Source direction image */
	sflag     = 1;
        offset    = hippi_src_offset;

	Fhdr.magic         = LINCPROM_FHDR_MAGIC;
	Fhdr.start_addr    = hippi_src_start_addr;
	Fhdr.size          = hippi_src_size;
	Fhdr.entry         = hippi_src_entry_addr;
	Fhdr.lincprom_vers = lincprom_src_vers ;  
	Fhdr.firmware_vers = hippi_src_vers;
	
        write_lincfw(  fd, &Fhdr,(caddr_t) hippi_src_txt, sflag, offset);
	write_lincprom(fd, lincprom_src_start_addr, lincprom_src_entry_addr,
		       lincprom_src_size, sflag, lincprom_src_offset);

	/* Download Destination direction image */
	sflag     = 0;
        offset    = hippi_dst_offset;

	Fhdr.magic         = LINCPROM_FHDR_MAGIC;
	Fhdr.start_addr    = hippi_dst_start_addr;
	Fhdr.size          = hippi_dst_size;
	Fhdr.entry         = hippi_dst_entry_addr;
	Fhdr.lincprom_vers = lincprom_dst_vers;  
	Fhdr.firmware_vers = hippi_dst_vers;
	
        write_lincfw(  fd, &Fhdr,(caddr_t) hippi_dst_txt, sflag, offset);
	write_lincprom(fd, lincprom_dst_start_addr, lincprom_dst_entry_addr, 
		       lincprom_dst_size, sflag, lincprom_dst_offset);
}


/* Campatability matrix:           0 older 1 newer                 
 * |   |FW |This|                                 
 * |   | in| FW | status/what to do                               
 * |drv| HW|    |
 * +---+---+----+------------------------------------------------
 * | 0 | 0 | 0  | OK/nothing                                      
 * | 1 | 1 | 1  | dito
 *
 * | 1 | 0 | 0  | wrong Kernel/ autoconfig for coherent state     
 * | 0 | 1 | 1  | dito
 *
 * | 0 | 1 | 0  | Usr switched card/ dl fw 
 * | 1 | 0 | 1  | dito
 *
 * | 0 | 0 | 1  | wrong utility/ inst it again (downrev the utility)
 * | 1 | 1 | 0  | dito                         (uprev the utility)
 */


void
hipstartup( int fd, int argc, char *argv[] )
{
	struct 	hip_bp_config bp_conf;
	uint_t	hw_dst_firmvers, hw_src_firmvers, drvrvers;
	int	timeo=0, on_tries=0, up_tries = 0;

	/* Get source and destination
	 * version of firmware installed in HIPPI board */
	hw_src_firmvers = (uint_t)ioctl( fd, HIPPI_GET_SRCVERS );
	hw_dst_firmvers = (uint_t)ioctl( fd, HIPPI_GET_DSTVERS );

	/* Get version of firmware the driver expects */
	drvrvers = (uint_t)ioctl( fd, HIPPI_GET_DRVRVERS );

	if ( argc > 0 && strcmp( argv[0], "force" ) == 0 ) {
		fprintf(stderr,
		  "hipcntl: forcing bringup without firmware update.\n" );
	}
	else if ( hw_dst_firmvers == -1 ||
		  hw_src_firmvers == -1 || drvrvers == -1 ) {
		fprintf(stderr,
			"hipcntl: Error: couldn't get version numbers.\n" );
		fprintf(stderr,
			"Possible hipcntl/kernel-driver mismatch.\n" );
		fprintf(stderr,
			"Please autoconfig your system and reboot.\n" );
		exit(1);
	}
	else if ( drvrvers != hippi_dstvers.whole ) {
		/* parts.is_src is set in hippi_srcvers, but not in drvrvers */
		fprintf( stderr,"hipcntl: Error: Version mismatch between kernel driver and hipcntl.\n" );
		fprintf( stderr,"Cannot startup adapter.\n" );
		fprintf( stderr,"You probably need to autoconfig and reboot your system\n" );
		fprintf( stderr,"and/or remove any old copies of hipcntl(1m) on your system.\n" );
		exit(1);
	}
	else if ( hw_dst_firmvers == 0 || hw_src_firmvers == 0  ) {
	        printf ("No firmware version #, downloading new firmware...\n");
		hipdownload( fd, 0, NULL );
		/* pause to allow LINCs to finish startup sequence
		 * before we try to bring the board up.
		 */
		printf ("Firmware download complete, restarting adapter.\n");
	}
	else if ( hippi_srcvers.whole != hw_src_firmvers || 
		  hippi_dstvers.whole != hw_dst_firmvers) {
		printf ("Firmware is downrev, downloading new firmware...\n");
		hipdownload( fd, 0, NULL );
		/* pause to allow LINCs to finish startup sequence
		 * before we try to bring the board up.
		 */
		printf ("Firmware download complete, restarting adapter.\n");
	}

	/* first time try just enough to not get the warning */
	timeo = HPS_QUIET_INIT_TRIES;
	while ( ioctl( fd, HIPPI_SETONOFF, 1 ) < 0 ) {
		switch (errno) {
		  case EINVAL:
			fprintf( stderr,
			   "hipcntl: Error: board is already up.\n" );
			exit(1);

		  case EBUSY:
			if (++up_tries < timeo) {	
				sginap(HPS_INIT_SLEEP_INTVL);
				break;
			}
			/* else board didn't come up so reset and try again */
			if (on_tries++ < 1) {
				up_tries = 0;
				/* if it doesn't come up after reset driver
				should print the warning so add one more try */
				timeo++;
				if (ioctl(fd, HIPPI_SETONOFF, 0) >= 0)
					break;
			}
			/* didn't come up at all */
			errno = EIO;

		  default:
			perror( "hipcntl: trouble bringing up HIPPI" );
			exit(1);
		}
	}

/*
 * Since we've made it this far, the board must be up.
 * Time to configure the bypass with some default parameters...
 */

#ifndef DEBUG
	fd = open(bpdevice_name, 0);

	if (fd < 0) {
	    fprintf(stderr, "hipcntl: couldn't open HIPPI device %s: ", bpdevice_name);
	    perror((char *)0);
	    exit(1);
	}
#endif

	bp_conf.ulp         = 144;
	bp_conf.max_jobs    = 8;
	bp_conf.max_portids = 1024;
	bp_conf.max_sfm_pgs = 1088;
	bp_conf.max_dfm_pgs = 1024;
	bp_conf.max_ddq_pgs = 1;

	hippbp_setconfig(fd, &bp_conf); 
}

void
hipshutdown(int fd, int argc, char *argv[])
{
	if (ioctl(fd, HIPPI_SETONOFF, 0) < 0) {
		perror("hipcntl: trouble shutting down HIPPI");
		exit(1);
	}
}

void
hipaccept(int fd, int argc, char *argv[])
{
	if (ioctl(fd, HIPIOC_ACCEPT_FLAG, 1) < 0) {
		perror("hipcntl: couldn't set HIPPI accept flag");
		exit(1);
	}
}

void
hipreject(int fd, int argc, char *argv[])
{
	if (ioctl(fd, HIPIOC_ACCEPT_FLAG, 0) < 0) {
		perror("hipcntl: couldn't set HIPPI accept flag");
		exit(1);
	}
}

void
hipstimeo(int fd, int argc, char *argv[])
{
	int value;

	if (argc < 1 || (value = atoi(argv[0])) < 1) {
		fprintf(stderr, "Usage: hipcntl stimeo <value>\n");
		exit(1);
	}

	if (ioctl(fd, HIPIOC_STIMEO, value) < 0) {
		perror("hipcntl: couldn't set src timeout");
		exit(1);
	}
}


int
is_hip_cmd(char *st)
{
        int i;
	for (i = 0; cmdtbl[i].name && strcmp(st, cmdtbl[i].name); i++)
	    ;
	if(cmdtbl[i].name) return i;
	else return -1;
}

int
main(int argc, char *argv[])
{
	int i, fd, unit=0, errs=0, arg=1, bp;

        while ( arg<argc && argv[arg][0] == '-' ) {
                i=1;
                while ( argv[arg][i] ) {
                        switch( argv[arg][i] ) {
                        case 'q':
                                quiet++;
                                break;
                        default:
                                fprintf( stderr,"%s: unknown option: -%c\n",
                                        argv[0], argv[arg][i] );
                                break;
                        }
                        i++;
                }
                arg++;
        }

	if (arg < argc && strncmp(argv[arg], "hip", 3) == 0) {
		char *num_start;
		char *cur;

		num_start = &argv[arg][3];

		if ((*num_start == 'p') && (*(num_start + 1) == 'i'))
			num_start += 2;
		
		cur = num_start;
		while (isdigit(*cur)) {
			cur++;
		}

		if ((cur == num_start) | (*cur != '\0')) {
			fprintf(stderr, "%s: bad HIPPI unit name: %s\n",
				argv[0], argv[arg]);
			exit(1);
		}

		unit = atoi(num_start);

		if (unit >= MAX_HIP_DEVS) {
			fprintf(stderr, "%s: %s is greater than max HIPPI "
				"devices\n", argv[0], argv[arg]);
			exit(1);
		}

		arg++;

	}

	if (arg >= argc) {
		fprintf(stderr, usage, argv[0]);
		exit(1);
	}

	sprintf(bpdevice_name, "/dev/hippibp%d", unit);

	if ( (i = is_hip_cmd(argv[arg])) != -1) {
	    bp = 0;
	    sprintf(device_name, "/dev/hippi%d", unit);
	}
	else if ((i = is_bp_cmd(argv[arg])) != -1) {
	    bp = 1;
	    sprintf(device_name, "/dev/hippibp%d", unit);
	}
	else {
	    fprintf(stderr, usage, argv[0]);
	    exit(1);
	}

	fd = open(device_name, 0);

	if (fd < 0) {
	    if (!quiet) {
		fprintf(stderr, "hipcntl: couldn't open HIPPI device %s: ", device_name);
		perror((char *)0);
	    }
	    exit(1);
	}

	arg++;

	if (bp  && cmdbptbl[i].func) {
		(*cmdbptbl[i].func)(fd, argc - arg, &argv[arg]);
		close(fd);
		exit(0);
	    }

	else  if (cmdtbl[i].func) {
		(*cmdtbl[i].func)(fd, argc - arg, &argv[arg]);
		close(fd);
		exit(0);
	    }
	
	fprintf(stderr, usage, argv[0]);
	exit(1);
}

int
parse_macaddr( char *s, u_char macaddr[] )
{
	int	i = 0;
	int	byte;

	while ( *s != '\0' && i < 6 ) {
		if ( ! isxdigit( *s ) )
			break;
		byte = strtol( s, &s, 16 );
		if ( byte < 0 || byte > 255 )
			break;
		macaddr[i] = byte;
		if ( *s == ':' )
			s++;
		i++;
	}

	return ( i == 6 && *s == '\0' ) ? 0 : -1;
}

/* 
 * syntax is
 *   hipcntl prog_MAC 08:00:69:xx:xx:xx [force]
 * !! DO NOT DOCUMENT THIS SYNTAX !!
 * THIS IS FOR MANUFACTURING USE ONLY.
 */

void
hipsetmac( int fd, int argc, char *argv[] )
{
    u_char mac_addr[6];
    u_char omac_addr[6];

    if ( argc < 1) {
	fprintf (stderr,
		 "hipcntl: insufficient number of arguments.\n");
	exit(1);
    }

    if (parse_macaddr (argv[0], mac_addr)) {
	fprintf (stderr,
		 "hipcntl: bad MAC address string.\n");
	exit(1);
    }

    /* SGI's MAC addresses begin with 08:00:69: */
    if ((mac_addr[0] != 8) || (mac_addr[1] != 0) || (mac_addr[2] != 0x69)) {
	fprintf (stderr,
		 "hipcntl: invalid MAC address\n");
	exit(1);
    }

    dprintf (5, ("Programming MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
		 mac_addr[0], mac_addr[1], mac_addr[2], 
		 mac_addr[3], mac_addr[4], mac_addr[5]));

    if ( ioctl( fd, HIPPI_GET_MACADDR, omac_addr ) < 0 ) {
	perror ("hipcntl: couldn't get HIPPI MAC address");
	exit(1);
    }

    /* If there's already a MAC address and it's not the same one,
       protest.
     */
    if ( (!bcmp (omac_addr, mac_addr, 3) )  &&
         ( bcmp (omac_addr, mac_addr, 6) ) ) {
	if ( ( argc < 2) || ( strcmp( argv[1], "force" ) != 0 ) ) {
	    fprintf (stderr,
		     "HIPPI adapter already has a MAC address programmed.\n");
	    exit(1);
	}
    }

    if ( ioctl( fd, HIPPI_SET_MACADDR, mac_addr ) < 0 ) {
	perror ("hipcntl: couldn't set HIPPI MAC address");
	exit(1);
    }

}

void
hipgetmac( int fd, int argc, char *argv[] )
{
    char mac_addr[6];

    if ( ioctl( fd, HIPPI_GET_MACADDR, mac_addr ) < 0 ) {
	perror ("hipcntl: couldn't get HIPPI MAC address");
	exit(1);
    }

    printf ("%02x:%02x:%02x:%02x:%02x:%02x\n",
	    mac_addr[0], mac_addr[1], mac_addr[2],
	    mac_addr[3], mac_addr[4], mac_addr[5]);
}

void
hipgetversions( int fd, int argc, char *argv[] )
{
	hippi_linc_fwvers_t	hw_dst_firmvers, hw_src_firmvers, drvrvers;

	/* Versions of actual firmware on the card. */
	hw_src_firmvers.whole = (uint_t)ioctl( fd, HIPPI_GET_SRCVERS );
	hw_dst_firmvers.whole = (uint_t)ioctl( fd, HIPPI_GET_DSTVERS );

	/* Get version of firmware the driver expects */
	drvrvers.whole = (uint_t)ioctl( fd, HIPPI_GET_DRVRVERS );

	printf ("Driver compiled with firmware version %d.%d\n",
		drvrvers.parts.major, drvrvers.parts.minor);
	printf ("hipcntl compiled with firmware version %d.%d\n",
		HIP_VERSION_MAJOR, HIP_VERSION_MINOR);
	if (hw_src_firmvers.parts.is_src != 1)
	    printf ("Adapter source EEPROM does not appear to hold src fw!!");
	else  
	    printf ("Adapter src EEPROM holds version %d.%d\n",
		   hw_src_firmvers.parts.major, hw_src_firmvers.parts.minor);

	if (hw_dst_firmvers.parts.is_src != 0)
	    printf ("Adapter dst EEPROM does not appear to hold dst fw!!");
	else  
	    printf ("Adapter dst EEPROM holds version %d.%d\n",
		   hw_dst_firmvers.parts.major, hw_dst_firmvers.parts.minor);
}

void
hiploopback( int fd, int argc, char *argv[] )
{
    if (ioctl( fd, HIPPI_SET_LOOPBACK ) < 0) {
	perror( "hipcntl: couldn't set loopback mode" );
	fprintf( stderr,
		"Make sure that the board has been shut down first.\n");
	exit(1);
    }
}
