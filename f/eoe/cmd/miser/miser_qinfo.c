/*
 * eoe/cmd/miser/miser_qinfo.c
 *	The miser_qinfo command is used to retrieve information about 
 * the free resources of a queue, the names of all miser queues, and to 
 * query all jobs currently scheduled against a particular queue.
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


#include "libmiser.h"


/* Structure to hold miser queue id list */
typedef struct mqlist {
        uint16_t	mq_count;
        id_type_t*	mq_qid;
} mqlist_t;


/* Structure to hold miser job id list */
typedef struct mjlist {
        uint16_t	mj_count;
        bid_t*		mj_jid;
} mjlist_t;


/*
 * usage
 *      Print an usage message with a brief description of each possible
 * option and quit.
 */
void
usage(const char* pname)
{
	fprintf(stderr,
		"\nUsage: %s -Q | -q qname [-j] | -a\n\n"
                "Valid Arguments:\n"
		"\t-Q\t\tList all defined miser queues.\n"
		"\t-q qname\tList current status of the queue.\n"
		"\t-q qname -j\tList all submitted jobids in the queue.\n"
		"\t-a\t\tList all queues and submitted jobs.\n\n",
			pname);
        exit(1);

} /* usage */


/*
 * qinfo_qname
 *     Query and list all miser queue names configured. 
 */
int
qinfo_qname()
{
	int 		i;		/* Loop index */
	char*		q_name;		/* Pointer to a qname string */
	id_type_t*	qid_ptr;	/* Pointer to queue id */

	miser_queue_names_t* mq;	/* Hold incoming miser qname list */

	/* Get a list of configured qnames */
	mq = miser_get_qnames();

	if ((mq != NULL) && (mq->mqn_count > 0))
		printf("\nMiser Queue(s):\n");
	else {  
		printf("\nmiser_qinfo: Failed to get Miser Queues.\n");
		return 0;
	}       

	/* Point to the first qid in the list found */
	qid_ptr = mq->mqn_queues; 

	for (i=0; i < mq->mqn_count; i++) {

		/* Convert qid to qname, incr qid_ptr to point next */
		q_name = (char *)miser_qstr(*qid_ptr++); 
                        
		printf("   %s\n", q_name);
	}       

	printf("\n");

	return 1;	/* Success */

} /* qinfo_qname */


/*
 * qinfo_qjid
 *     Query and list all miser submitted jobids in the specified queue.
 */
int
qinfo_qjid(char* q_name, int start)
{
	int		i;		/* Loop index */
	bid_t*		jid_ptr;	/* Pointer to job id */ 
	id_type_t	qid;		/* Hold queue id */
	miser_bids_t*	mj;		/* Hold incoming miser jid list */

	/* Convert qname to qid */
	qid = miser_qid(q_name);

	/* Get a list of submitted jids for the qid specified */
	mj = miser_get_jids(qid, start);

	if ((mj != NULL) && (mj->mb_count > 0))
		printf("\nSubmitted Job(s) in Queue [%s]:\n", q_name);
	else {
		printf("\nNo Submitted Job in Queue [%s].\n", q_name);
		return 0;
	}

	/* Point to the first jid in the list found */
	jid_ptr = mj->mb_buffer;

	/* Print and incr jid to point to the next */
	for (i=0; i < mj->mb_count; i++)
		printf("   %d\n", *jid_ptr++);

	printf("\n");

	return 1; 	/* Success */

} /* qinfo_qjid */


/*
 * qinfo_qjsched
 *     Query and list all miser submitted jobids and their schedules 
 * in the miser queues.
 */
int
qinfo_qjsched(int start)
{
	int		i, j;		/* Loop index */
	char*		q_name;		/* Pointer to qname */
	bid_t*		jid_ptr;	/* Pointer to job id */
	id_type_t*	qid_ptr;	/* Pointer to queue id */
	miser_seg_t*	seg_ptr;	/* Pointer to seg */
        mqlist_t	mql;		/* Hold local miser qid list */
        mjlist_t	mjl;		/* Hold local miser jid list */

	miser_queue_names_t* mq;	/* Hold incoming miser qname list */
	miser_bids_t*        mj;	/* Hold incoming miser jid list */
	miser_schedule_t*    ms;	/* Hold incoming miser job sched  */

	/* Get a list of configured qnames */
	mq = miser_get_qnames();

	if ((mq != NULL) && (mq->mqn_count > 0)) {
		printf("\nSubmitted Job(s) in Miser Queue(s):\n");
		printf("-----------------------------------\n");
	}
	else {
		printf("\nmiser_qinfo: Failed to get Miser Queues.\n");
		return 0;
	}

	/* Initialize and allocate local memory for qid list */
	mql.mq_count = mq->mqn_count;
	mql.mq_qid = malloc(sizeof(id_type_t) * mql.mq_count);

	/* Point to the first qid in the list */
	qid_ptr = mql.mq_qid;

	/* Load qids in local memory */
	for (i=0; i < mql.mq_count; i++)
		*mql.mq_qid++ = mq->mqn_queues[i];

	/* Collect job ids for each qid */
	for (i=0; i < mql.mq_count; i++) {

		/* Convert qid to qname */
		q_name = (char *)miser_qstr(*qid_ptr);

		printf("\nQueue [%s]:\n\n", q_name);

		/* Get a list of submitted jids for specified qid  */
		/* Incr ptr to point to next qid at next iteration */
		mj = miser_get_jids(*qid_ptr++, start);

		if ((mj == NULL) || (mj->mb_count == 0)) {
			printf("   No Submitted Job Found.\n\n");
			continue;
		}

		/* Initialize and allocate local memory for jid list */
		mjl.mj_count = mj->mb_count;
		mjl.mj_jid = malloc(sizeof(bid_t) * mjl.mj_count);

		/* Point to the first jid in the list */
		jid_ptr = mjl.mj_jid;

		/* Load jids in local memory */
		for (j=0; j < mjl.mj_count; j++)
			*mjl.mj_jid++ = mj->mb_buffer[j];

		/* Collect job schedule for each job id */
		for (j=0; j < mjl.mj_count; j++) {

			/* Get a job sched for specified jid */
			/* Incr ptr to point to next jid */
			ms = miser_get_jsched(*jid_ptr++, start);

			if ((ms == NULL) || (ms->ms_count == 0))
				continue; /* jid doesn't exist anymore */

			if (j==0) {
				printf(" JOBID   CPU  MEM     START TIME     "
				"    END TIME        USER       COMMAND\n"
				"-------- --- ----- ----------------- "
				"----------------- -------- ---------------\n"); 			}

			/* Point to the first seg in the list */
			seg_ptr = (miser_seg_t *)ms->ms_buffer;

			printf("%8d ", ms->ms_job);

			miser_print_job_status(seg_ptr);
			printf(" ");

			miser_print_job_desc(ms->ms_job); 
			printf("\n");

		} /* for 2 */

		free(mjl.mj_jid); /* Free allocated memory for jid list */

	} /* for 1 */

	printf("\n");

	free(mql.mq_qid); /* Free allocated memory for qid list */

	return 1;	/* Success */

} /* qinfo_qjsched */


int
main(int argc, char **argv)
{
        int	c;		/* Command line option character */
	int	start = 0;	/* Argument variable */
        char*	q_name;		/* Pointer to qname */
        int	qjsched = 0;	/* '-a' flag */
        int	jid = 0;	/* '-j' flag */
        int	qname = 0;	/* '-Q' flag */
        int	qstat = 0;	/* '-q' flag */

	/* Initialize miser */
	if (!miser_init()) exit(1);

	/* Parse command line arguments and set appropriate Flags */
        while ((c = getopt(argc, argv, "ahjQq:s")) != -1) {
                switch(c) {

		/* get all queues, submitted jobs, and their schedules */
                case 'a':
                        qjsched = 1;
                        break;

		/* get job ids for the specified queue */
                case 'j':
                        jid = 1;
                        break;

		/* get list of configured queue names */
		case 'Q':
			qname = 1;
			break;

		/* get queue statistics for the specified queue */
		case 'q':
			qstat = 1;
			q_name = optarg;
			break;

		/* specified start time for query - not published */
                case 's':
                        start = atoi(optarg);
                        break;

		/* print usage for anything else */
		default:
			usage(argv[0]);
		}
	}

	/* Check for valid combination of arguments requested */
	if ((qjsched + qname + (jid || qstat) != 1) ||
		((jid == 1) && (qstat == 0)))
		usage(argv[0]);

	/* Argument: '-Q' (get list of configured queue names) */
	if (qname) 
		return qinfo_qname();

	/* Argument: '-q qname' (get queue statistics for specified queue) */
	else if ((qstat == 1) && (jid == 0))
		return miser_get_qstat(q_name, start);

	/* Argument: '-q qname -j' (get job ids for the specified queue) */
	else if ((qstat == 1) && (jid == 1))
		return qinfo_qjid(q_name, start);

	/* Argument: '-a' (get all queues, submitted jobs, and schedules) */
	else if (qjsched)
		return qinfo_qjsched(start);

	/* Undefined - Print usage message */
	else
		usage(argv[0]);

	return 1;	/* Success */

} /* main */
