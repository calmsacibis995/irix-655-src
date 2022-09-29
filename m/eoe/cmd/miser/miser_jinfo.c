/*
 * eoe/cmd/miser/miser_jinfo.c
 *	This file implements an user command to query schedule information 
 * on a job submitted under miser reservation scheme.
 */

/**************************************************************************
 *									  *
 * 		 Copyright (C) 1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/


#include <sys/sysmp.h>
#include <errno.h>
#include "libmiser.h"


/*
 * usage
 *      Print an usage message with a brief description of each possible
 * option and quit.
 */
void
usage(const char* pname)
{
	fprintf(stderr,
		"\nUsage: %s -j jid [-d]\n\n"
                "Valid Arguments:\n"
                "\t-j jid\tPrint job status for the requested jobid.\n"
                "\t-d\tPrint job description (owner, command) for the "
			"requested jid.\n\n",
                        pname);
        exit(1);

} /* usage */


/*
 * jinfo_desc
 *	Print job description including the username of the job owner and 
 * the command name executing.
 */
void
jinfo_desc(bid_t jid, int start)
{ 
	miser_seg_t*      seg_ptr;	/* Pointer to miser resource segment */
	miser_schedule_t* ms;		/* Hold incoming miser job schedule */

	/* Get a job schedule for specified jid */
	if ((ms = miser_get_jsched(jid, start)) == NULL) {
		printf("miser_jinfo: Failed to find job [%d].\n\n", jid);
		exit(1);
	}

	/* Point to the first seg in the list */
	seg_ptr = (miser_seg_t *)ms->ms_buffer;

	printf("\n  JOBID  CPU  MEM     START TIME     "
		"    END TIME        USER       COMMAND\n"
		"-------- --- ----- ----------------- "
		"----------------- -------- ---------------\n");

	printf("%8d ", jid);

	miser_print_job_status(seg_ptr);
	printf(" ");

	miser_print_job_desc(jid);
	printf("\n\n");

} /* jinfo_desc */


/*
 * jinfo_status
 *      Print miser job status including resources, start/end times etc.
 */
void
jinfo_status(bid_t jid, int start)
{ 
	miser_seg_t*      seg_ptr;	/* Pointer to miser resource segment */
	miser_schedule_t* ms;		/* Hold incoming miser job schedule */

	/* Get a job schedule for specified jid */
	if ((ms = miser_get_jsched(jid, start)) == NULL) {
		printf("miser_jinfo: Failed to find job [%d].\n\n", jid);
		exit(1);
	}

	/* Point to the first seg in the list */
	seg_ptr = (miser_seg_t *)ms->ms_buffer;

        printf("\n  JOBID  CPU  MEM   DURATION     START TIME"
		"         END TIME      MLT PRI OPT\n"
		"-------- --- ----- ---------- ----------------- "
		"----------------- --- --- ---\n");

	printf("%8d ", jid);

	miser_print_job_sched(seg_ptr);

	printf("\n");

} /* jinfo_status */


int
main(int argc, char ** argv)
{
	int c;		/* Command line option character */ 
	int desc = 0;	/* Job description '-d' flag */
	int start = 0;	/* Search start segment '-s' flag */
	int status = 0;	/* Job status '-j' flag */
	bid_t jid = -1;	/* Job id to find information on */

	/* Initialize miser */
	if (!miser_init()) exit(1);

	/* Parse command line arguments and set appropriate Flags */
	while ((c = getopt(argc, argv, "dj:s:")) != -1) {

		switch (c) {
		case 'd':	/* job description (submitter, command) */
			desc = 1;
			break;

		case 'j':	/* job status display */
			status = 1;
			jid = atoi(optarg);
			break;

		case 's':	/* not documented - search start segment */
			start = atoi(optarg);
			break;

		default:
			usage(argv[0]);

		} /* switch */

	} /* while */

	if (jid < 0)
		usage(argv[0]);

	if (desc)
		jinfo_desc(jid, start);

	else if (status)
		jinfo_status(jid, start);

	return 0;

} /* main */
