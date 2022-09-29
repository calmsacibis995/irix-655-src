/*
 * eoe/cmd/miser/miser_submit.c
 * 	This file implements an user command which is used to submit a batch
 * job to run under miser reservation scheme.
 */

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


#include <errno.h>
#include "libmiser.h"


static char *seg_opts[] = {

#define TIME	0
		"t",
#define CPUS	1
		"c",
#define MEMORY	2
		"m",
#define STATIC	3
		"static",
		0

}; /* seg_opts */


/*
 * usage
 *      Print an usage message with a brief description of each possible
 * option and quit.
 */
void
usage(const char* pname)
{
	fprintf(stderr, 
		"\nUsage: %s -q qname -o c=cpus,m=mem,t=time[,static] command\n"
		"       %s -q qname -f file command\n\n"
		"Valid Arguments:\n"
		"\t-q\tQueue name in which to schedule the job to.\n"
		"\t-o\tSpecify job's resource requirements in command line.\n"
		"\t-f\tFilename containing job's resource requirements.\n\n"
		, pname, pname);
	exit(1);

} /* usage */


/*
 * parse_subopt
 *	Parse job's resource requirements specified after '-o' option.
 */
void
parse_subopt(char* subopt, const char* pname)
{
	char*        val;	/* Resource value from the command line */
	miser_seg_t* seg;	/* Pointer to resource segment structure */

	if (!(seg = parse_jseg_start())) exit(1);

	while (*subopt != '\0') {

		switch(getsubopt(&subopt, seg_opts, &val)) {

		case CPUS:
			seg->ms_resources.mr_cpus = atol(val);
			break;

		case TIME:
			if (!fmt_str2time(val, &seg->ms_rtime))
				usage(pname);
			break;

		case MEMORY:
			if (!fmt_str2bcnt(val, &seg->ms_resources.mr_memory))
				usage(pname);
			break;

		case STATIC:
			seg->ms_flags &= ~MISER_SEG_DESCR_FLAG_NORMAL;
			seg->ms_flags |= MISER_SEG_DESCR_FLAG_STATIC;
			break;

		default:
			usage(pname);
		}

	} /* while */

	if (!parse_jseg_stop()) exit(1);

} /* parse_subopt */


int
main(int argc, char **argv)
{
	int   opt;
	int   oflag = 0;
	char* queue = 0;
	char* file  = 0;

	miser_data_t* req;	/* Pointer to miser_data structure */
	miser_job_t*  job;	/* Pointer to miser_job structure */ 

	/* Initialize miser */
	if (!miser_init()) exit(1);

	/* Parse arguments - commandline */
	while ((opt = getopt(argc, argv, "q:f:o:")) != -1) {

		switch(opt) {

		case 'q':
			queue = optarg;
			break;

		case 'f':
			if (oflag || file) usage(argv[0]);
			file = optarg;
			break;

		case 'o':
			if (file) usage(argv[0]);

			if (!oflag) {
				oflag = 1;
				if (!parse_start(PARSE_JSUB, 0))
					exit(1);
			}

			parse_subopt(optarg, argv[0]);
			break;

		default:
			usage(argv[0]);
		}

	} /* while */

	if (optind >= argc || !queue || (!oflag && !file))
		usage(argv[0]);

	/* Parse contents of the job resource configuration file */
	if (file)
		req = parse(PARSE_JSUB, file);
	else
		req = parse_stop();

	/* Job submit argument file parsing failed - exit */
	if (!req) return 0;

	/* Send submit, job reservation request to the kernel */
	if (!miser_submit(queue, req))
		return 0;

	job = (miser_job_t *) req->md_data; 

        printf("\n\nMiser Job Successfully Scheduled:\n\n");

        printf("  JOBID  CPU  MEM   DURATION     START TIME         "
		"END TIME      MLT PRI OPT\n"
		"-------- --- ----- ---------- ----------------- "
		"----------------- --- --- ---\n");

	printf("%8d ", job->mj_bid);

	job->mj_segments[0].ms_etime = job->mj_etime;

	/* Print job schedule */
        miser_print_job_sched(&job->mj_segments[0]);

        printf("\n");
	fflush(NULL);

	/* Execute command - blocking */
	execvp(argv[optind], &argv[optind] );

	/* Returned from execvp - must have failed, print error message */
	merror("miser_submit: Failed to exec process: %s", strerror(errno));

	return 0;	/* Failed */

} /* main */
