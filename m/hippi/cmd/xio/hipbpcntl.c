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
 * hipbpcntl.c
 * Bypass configuration extensions to hipcntl
 *
 */

#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/errno.h>
#include "sys/hippi.h"
#include "sys/hps_ext.h"
#include "sys/hippibp.h"
#include "hipcntl.h"

extern int errno;

struct cmd cmdbptbl[] = {

	"bpulp",	hipbpulp,	/* set bypass ULP number */
	"bpjobs",	hipbpjobs,	/* set bypass job limit */
	"bpports",	hipbpports,	/* set bypass port limit */
	"bpspages",	hipbpspages,	/* set bypass src page limit */
	"bpdpages",	hipbpdpages,	/* set bypass des page limit */
	"bpstatus",	hipbpstatus,	/* get bypass status of board */

	0,0
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

int
is_bp_cmd(char *st)
{
        int i;
	for (i = 0; cmdbptbl[i].name && strcmp(st, cmdbptbl[i].name); i++)
	    ;
	if(cmdbptbl[i].name) return i;
	else return -1;
}



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

void
hippbp_setconfig(int fd, struct hip_bp_config *bp_conf)
{
	int timeo = 0;

	for (;;) {
		if (ioctl(fd, HIPIOC_SET_BPCFG, bp_conf) == 0) {
			return;
		}

		switch (errno) {
		 case EBUSY:
			if (timeo++ < 10) {	/* should take < 5 secs */
				sginap(CLK_TCK/2);
				break;
			}
			/* else fall through */

		 default:
			perror("hipcntl: trouble setting bypass configuration");
			exit(1);
		}
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

	for (i = 0; i < HIPPIBP_MAX_JOBS; i++) { 
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

