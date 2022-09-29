/**************************************************************************
 *									  *
 * 		 Copyright (C) 1997 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "libmiser.h"


static char *qseg_opts[] = {
#define START	0
		"s",
#define END	1
		"e",
#define CPUS	2
		"c",
#define MEMORY	3
		"m",
		0
};


void
usage(const char *pname)
{
	fprintf(stderr, "Usage: %s -s srcq -d dstq "
			"[-o s=start,e=end,c=cpus,m=mem] "
			"[-f file]\n", pname);
	exit(1);
}


int
main(int argc, char **argv)
{
	int		opt;
	char *		subopt;
	char *		val;
	int		oflag	= 0;
	char *		file	= 0;
	char *		srcq	= 0;
	char *		dstq	= 0;
	miser_data_t *	req;
	miser_qseg_t *	qseg;

	if (!miser_init())
		exit(1);

	while ((opt = getopt(argc, argv, "s:d:f:o:")) != -1) {
		switch(opt) {
		case 's':
			srcq = optarg;
			break;
		case 'd':
			dstq = optarg;
			break;
		case 'f':
			if (oflag || file) usage(argv[0]);
			file = optarg;
			break;
		case 'o':
			if (file) usage(argv[0]);
			if (!oflag) {
				oflag = 1;
				if (!parse_start(PARSE_QMOV, 0))
					exit(1);
			}

			if (!(qseg = parse_qseg_start())) exit(1);
			subopt = optarg;
			while (*subopt != '\0') {
				switch(getsubopt(&subopt, qseg_opts, &val)) {
				case START:
					if (!fmt_str2time(val,
							&qseg->mq_stime))
						usage(argv[0]);
					break;
				case END:
					if (!fmt_str2time(val, 
							  &qseg->mq_etime))
						usage(argv[0]);
					break;
				case CPUS:
					qseg->mq_resources.mr_cpus = atol(val);
					break;
				case MEMORY:
					if (!fmt_str2bcnt(val,
						&qseg->mq_resources.mr_memory))
						usage(argv[0]);
					break;
				}
			}
			if (!parse_qseg_stop()) exit(1);
			break;
		default:
			usage(argv[0]);
		}
	}

	if (optind != argc || !srcq || !dstq || (!oflag && !file))
		usage(argv[0]);

	if (file)
		req = parse(PARSE_QMOV, file);
	else
		req = parse_stop();

	if (!req)
		exit(1);

	if (!miser_move(srcq, dstq, req))
		exit(1);

	print_move(stdout, req);
	return 1;
}







