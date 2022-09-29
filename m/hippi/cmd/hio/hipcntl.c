
/*
 * hipcntl.c
 *
 * Usage: hipctl [hippiN] {startup|shutdown|accept|reject|status|stimeo}
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include "sys/hippi.h"
#include "sys/hippidev.h"
#include "sys/hippibp.h"
#include <sys/errno.h>

#include "../../firm/ehip/ehip.firm"

extern int errno;

char *usage="Usage: %s [hippiN] <cmd> <parameters>\n" 
	"\t <cmd> must be one of:\n"
	"\t startup    (update firmware and start board) \n"
	"\t shutdown   (bring board down and reset it) \n" 
	"\t accept     (board starts accepting connections) \n"
	"\t reject     (board stops accepting connections) \n"
	"\t status     (get status of board) \n"
	"\t stimeo     (set source timeout value) \n"
        "\n"
	"\t bpulp      (set bypass ULP number) \n"
	"\t bpjobs     (set bypass job limit) \n"
	"\t bpports    (set bypass port limit) \n"
	"\t bpspages   (set bypass src page limit) \n"
	"\t bpdpages   (set bypass des page limit) \n"
	"\t bpstatus   (get bypass status of board) \n";

#ifndef lint
static char rcsid[] = "$Header: /isms/hippi/3.2/cmd/hio/RCS/hipcntl.c,v 1.12 1996/12/04 05:02:01 irene Exp $";
#endif

void	hipstartup( int, int, char ** );
void	hipshutdown( int, int, char ** );
void	hipdownload( int, int, char ** );
void	hipaccept( int, int, char ** );
void	hipreject( int, int, char ** );
void	hipstatus( int, int, char ** );
void	hipstimeo( int, int, char ** );

void    hipbpulp( int, int, char ** );
void    hipbpjobs( int, int, char ** );
void    hipbpports( int, int, char ** );
void    hipbpspages( int, int, char ** );
void    hipbpdpages( int, int, char ** );
void    hipbpstatus( int, int, char ** );

static struct cmd {
	char	*name;
	void	(*func)( int, int, char ** );
} cmdtbl[] = {

	"startup",	hipstartup,	/* update firmware and start board */
	"bringup",	hipstartup,	/* same as startup: backwards compat */
	"shutdown",	hipshutdown,	/* bring board down and reset it */
	"download",	hipdownload,	/* force update of firmware */
	"accept",	hipaccept,	/* board starts accepting connections*/
	"reject",	hipreject,	/* board stops accepting connections */
	"status",	hipstatus,	/* get status of board */
	"stimeo",	hipstimeo,	/* set source timeout value */

	"bpulp",	hipbpulp,	/* set bypass ULP number */
	"bpjobs",	hipbpjobs,	/* set bypass job limit */
	"bpports",	hipbpports,	/* set bypass port limit */
	"bpspages",	hipbpspages,	/* set bypass src page limit */
	"bpdpages",	hipbpdpages,	/* set bypass des page limit */
	"bpstatus",	hipbpstatus,	/* get bypass status of board */

	0,0
};

static char *hipstatnames[] = {
	"SRC connections      ",
	"SRC packets          ",
	"SRC rejects          ",
	"SRC seq errors (dm)  ",
	"SRC seq errors (cd)  ",
	"SRC seq errors (cs)  ",
	"SRC dsic lost        ",
	"SRC time outs        ",
	"SRC connects lost    ",
	"SRC parity errs      ",

	"",
	"",
	"",
	"",
	"",
	"",

	"DST connections      ",
	"DST packets          ",
	"DST rcv on bad ulp   ",
	"DST hippi-le drop    ",
	"DST llrc             ",
	"DST parity           ",
	"DST sequence err     ",
	"DST sync err         ",
	"DST illegal burst    ",
	"DST sdic lost        ",
	"DST null connections ",

	"",
	"",
	"",
	"",
	"",
	0
};

static char *hipbpstatnames[] = {
	"SRC descriptors       ",
	"SRC packets           ",
	"SRC bytes             ",
	"SRC desc err: ifield  ",
	"SRC desc err: bufx    ",
	"SRC desc err: opcode  ",
	"SRC desc err: addr    ",
	"",
	"",
	"",
	"",
	"",
	"",
	"DST descriptors       ",
	"DST packets           ",
	"DST bytes             ",
	"DST err: port disabled",
	"DST err: job disabled ",
	"DST err: no buffs     ",
	"DST err: inv bufx     ",
	"DST err: inv auth     ",
	"DST err: inv offset   ",
	"DST err: inv opcode   ",
	"DST err: inv version  ",
	"DST err: inv seq num  ",
	"",
	"",
	"",
	0
};

static int hipbpstatlens[] = {
	1,
	1,
	2,
	1,
	1,
	1,
	1,
	1,
	0,
	0,
	0,
	0,
	0,
	1,
	1,
	2,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	0,
	0,
	0
};


static char *hipflagnames[] = {
	"DSIC", "SDIC", "", "",
	"ACCEPTING", "DST.PKT", "DST.REQ", "",
	"SRC.REQ", "SRC.CON",
	NULL
};

/* global options. */
int quiet = 0;



static struct hip_bp_config *
hippbp_getconfig(int fd)
{
	static struct hip_bp_config bp_conf;

	if (ioctl(fd, HIPIOC_GET_BPCFG, &bp_conf) < 0) {
		perror("hipcntl: trouble getting bypass configuration");
		exit(1);
	}

	return &bp_conf;
}

static void
hippbp_setconfig(int fd, struct hip_bp_config *bp_conf)
{
	for (;;) {
		if (ioctl(fd, HIPIOC_SET_BPCFG, bp_conf) == 0) {
			return;
		}

		switch (errno) {
		 case EBUSY:
			sleep(1);
			break;

		 default:
			perror("hipcntl: trouble setting bypass configuration");
			exit(1);
		}
	}
}

void
hipstatus( int fd, int argc, char *argv[] )
{
	int	i;
	u_long	flags;
	hippi_stats_t	hipstats;

	if ( ioctl( fd, HIPIOC_GET_STATS, &hipstats ) < 0 ) {
		if ( errno == ENODEV )
			printf( "hipcntl: HIPPI board is down\n" ),exit(1);
		else
			perror( "hipcntl: couldn't get HIPPI statistics" ),exit(1);
	}
	
	flags = hipstats.hst_flags;
	
	printf( "FLAGS: " );
	i=0;
	while ( hipflagnames[i] ) {
		if ( flags & (1<<i) )
			printf( " %s ", hipflagnames[i] );
		i++;
	}
	printf( "\n" );

	i=0;
	while ( hipstatnames[i] ) {
		if ( *hipstatnames[i] != '\0' )
		  printf( "%s:\t%u\n", hipstatnames[i],
			 ( (u_long *) &hipstats.hst_s_conns )[i] );
		i++;
	}

}

void
hipdownload( int fd, int argc, char *argv[] )
{
	register int	i, j;
	struct hip_dwn *hdwn;
	u_char	op, *s, *d;
	u_long	v;

	i = ehip_maxaddr-ehip_minaddr;
	hdwn = (struct hip_dwn *) malloc( sizeof(struct hip_dwn) + i );

	hdwn->addr = 0;
	hdwn->len = i;
	hdwn->vers = ehip_vers;

	d = (u_char *) (hdwn+1);
	s = ehip_txt;
	while ( i > 0 ) {
		op = *s++;

		if (op < ehip_DZERO) {
			j = op-ehip_DDATA+1;
			while (j-- != 0) {
				*d++ = *s++;
				*d++ = *s++;
				*d++ = *s++;
				*d++ = *s++;
				i -= 4;
			}
		} else {
			if (op < ehip_DNOP) {
				j = op-ehip_DZERO+1;
				v = 0;
			} else {
				j = op-ehip_DNOP+1;
				v = 0x70400101;
			}
			while (j-- != 0) {
				*( (u_long *) d ) = v;
				d += 4;
				i -= 4;
			}
		}
	}

	if ( ioctl( fd, HIPPI_PGM_FLASH, hdwn ) < 0 ) {
		if ( errno == EINVAL )
			fprintf( stderr, "hipcntl: (device should"
			 " be shutdown before downloading firmware)\n" );
		perror( "hipcntl: problem programming flash" ),exit(1);
	}
}

void
hipstartup( int fd, int argc, char *argv[] )
{
	struct	hip_bp_config bp_conf;
	long	firmvers, drvrvers;

	/* Get version of firmware installed in HIPPI board */
	firmvers = ioctl( fd, HIPPI_GET_FIRMVERS );

	/* Get version of firmware the driver expects */
	drvrvers = ioctl( fd, HIPPI_GET_DRVRVERS );

	if ( argc > 0 && strcmp( argv[0], "force" ) == 0 ) {
		fprintf(stderr,"hipcntl: forcing bringup without firmware update.\n" );
	}
	else if ( firmvers == -1 || drvrvers == -1 ) {
		fprintf(stderr,"hipcntl: Error: couldn't get version numbers.\n" );
		fprintf(stderr,"Possible hipcntl/kernel-driver mismatch.\n" );
		fprintf(stderr,"Please autoconfig your system and reboot.\n" );

		exit(1);
	}
	else if ( firmvers == 0 ) {
		fprintf( stderr, "hipcntl: Warning: no firmware version #.  Not reprogramming.\n" );
		fprintf( stderr, "Probably means you are running HIPPI_DEBUG kernel.\n" );
		if ( drvrvers != ehip_vers ) {
			fprintf( stderr, "hipcntl: Double Warning: may be firmware driver mismatch if you force download.\n" );
		}
	}
	else {
		if ( drvrvers != ehip_vers ) {
			fprintf( stderr,"hipcntl: Error: mismatch between kernel driver and hipcntl.\n" );
			fprintf( stderr,"Cannot startup adapter.\n" );
			fprintf( stderr,"You probably need to autoconfig and reboot your system\n" );
			fprintf( stderr,"and/or remove any old copies of hipcntl(1m) on your system.\n" );
			exit(1);
		}
		if ( firmvers != ehip_vers )
			hipdownload( fd, 0, NULL );
	}

	if ( ioctl( fd, HIPPI_SETONOFF, 1 ) < 0 ) {
		if ( errno == EINVAL )
			fprintf( stderr,
			   "hipcntl: Error: board is already up.\n" ),exit(1);
		else
			perror( "hipcntl: trouble bringing up HIPPI" ),exit(1);
	}

/*
 * Since we've made it this far, the board must be up.
 * Time to configure the bypass with some default parameters...
 */

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

void
hipbpulp(int fd, int argc, char *argv[])
{
	struct hip_bp_config *bp_conf;
	int value;

	if (argc < 1 || (value = atoi(argv[0])) < 1) {
		fprintf(stderr, "Usage: hipcntl bpulp <value>\n");
		exit(1);
	}

	bp_conf = hippbp_getconfig(fd);

	bp_conf->ulp = value;

	hippbp_setconfig(fd, bp_conf);
}

void
hipbpjobs(int fd, int argc, char *argv[])
{
	struct hip_bp_config *bp_conf;
	int value;

	if (argc < 1 || (value = atoi(argv[0])) < 1) {
		fprintf(stderr, "Usage: hipcntl bpjobs <value>\n");
		exit(1);
	}

	bp_conf = hippbp_getconfig(fd);

	bp_conf->max_jobs = value;

	hippbp_setconfig(fd, bp_conf);
}

void
hipbpports(int fd, int argc, char *argv[])
{
	struct hip_bp_config *bp_conf;
	int value;

	if (argc < 1 || (value = atoi(argv[0])) < 1) {
		fprintf(stderr, "Usage: hipcntl bpports <value>\n");
		exit(1);
	}

	bp_conf = hippbp_getconfig(fd);

	bp_conf->max_portids = value;

	hippbp_setconfig(fd, bp_conf);
}

void
hipbpspages(int fd, int argc, char *argv[])
{
	struct hip_bp_config *bp_conf;
	int value;

	if (argc < 1 || (value = atoi(argv[0])) < 1) {
		fprintf(stderr, "Usage: hipcntl bpspages <value>\n");
		exit(1);
	}

	bp_conf = hippbp_getconfig(fd);

	bp_conf->max_sfm_pgs = value;

	hippbp_setconfig(fd, bp_conf);
}

void
hipbpdpages(int fd, int argc, char *argv[])
{
	struct hip_bp_config *bp_conf;
	int value;

	if (argc < 1 || (value = atoi(argv[0])) < 1) {
		fprintf(stderr, "Usage: hipcntl bpdpages <value>\n");
		exit(1);
	}

	bp_conf = hippbp_getconfig(fd);

	bp_conf->max_dfm_pgs = value;

	hippbp_setconfig(fd, bp_conf);
}

void
hipbpstatus(int fd, int argc, char *argv[])
{
	struct hip_bp_config *bp_config;
	hippibp_stats_t	hipstats;
	int i, j;
	long long tmp_64;
	int	*p_int = (int *) &tmp_64;

	if (ioctl(fd, HIPIOC_GET_BPSTATS, &hipstats) < 0) {
		if (errno == ENODEV) {
			fprintf(stderr, "hipcntl: HIPPI board is down or bypass is not configured\n");
			exit(1);
		} else {
			perror("hipcntl: couldn't get HIPPI bypass statistics");
			exit(1);
		}
	}

/* Hardwire the number of jobs to 8 for now because that's all that the firmware currently supports */

/*	for (i = 0; i < HIPPIBP_MAX_JOBS; i++) { */

	for (i = 0; i < 8; i++) {
		printf("Job %d: ", i);

		if ((hipstats.hst_bp_job_vec>>(31-i)) & 0x1) {
			printf("BUSY\n");
		} else {
			printf("IDLE\n");
		}
	}

	bp_config = hippbp_getconfig( fd );

	printf("\n");
	printf("Bypass ulp            :\t%d\n", bp_config->ulp);
	printf("Bypass max jobs       :\t%d\n", bp_config->max_jobs);
	printf("Bypass max ports      :\t%d\n", bp_config->max_portids);
	printf("Bypass max src pages  :\t%d\n", bp_config->max_sfm_pgs);
	printf("Bypass max dst pages  :\t%d\n", bp_config->max_dfm_pgs);
	printf("\n");

	for (i = 0, j = 0; hipbpstatnames[i]; i++, j++) {
		if (*hipbpstatnames[i] && hipbpstatlens[i] == 1) {
			printf("%s:\t%u\n", hipbpstatnames[i], ((u_long *)&hipstats.hst_s_bp_descs)[j]);
		}
		else  if(*hipbpstatnames[i] && hipbpstatlens[i] == 2) {
			*p_int	= ((u_long *)&hipstats.hst_s_bp_descs)[j++];
			*(p_int + 1) = ((u_long *)&hipstats.hst_s_bp_descs)[j];
			printf("%s:\t%lld\n", hipbpstatnames[i], tmp_64);
		}
	}
}

int
main(int argc, char *argv[])
{
	char n, device_name[64];
	int i, fd, unit=0, errs=0, arg=1;

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
		n = argv[arg][strlen(argv[arg]) - 1];

		if (n < '0' || n > '7') {
			fprintf(stderr, "%s: bad HIPPI unit number\n", argv[0]);
			exit(1);
		}

		unit = n - '0';
		arg++;
	}

	if (arg >= argc) {
		fprintf(stderr, usage, argv[0]);
		exit(1);
	}

	sprintf(device_name, "/dev/hippi%d", unit);

	fd = open(device_name, 0);

	if (fd < 0) {
		if( ! quiet ) {
			fprintf(stderr, "hipcntl: couldn't open HIPPI device %s: ", device_name);
			perror((char *)0);
			}
		exit(1);
	}

	for (i = 0; cmdtbl[i].name && strcmp(argv[arg], cmdtbl[i].name); i++)
		;

	arg++;

	if (cmdtbl[i].func) {
		(*cmdtbl[i].func)(fd, argc - arg, &argv[arg]);
		exit(0);
	} else {
		fprintf(stderr, usage, argv[0]);
		exit(1);
	}
}
