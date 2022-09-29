/*
 * main.C
 * 	Central file for miser daemon. Initiates the miser scheduler.
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
#include <sys/param.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include "miser_private.h"
#include "miser_debug.h"
#include "libmiser.h"

extern char*	    optarg;
extern miser_time_t now;

id_type_t 	system_id;
time_t 		time_quantum;
quanta_t 	duration;
miser_queue* 	system_pool;
queuelist* 	active_queues;
policy_list 	policies;
miser_request_t	req;
jobschedule	job_schedule_queue;

error_t         G_error;                // Global miser error
bool 		G_verbose;		// Flag to toggle verbose mode
ncpus_t 	G_maxcpus;		// Max cpus under miser control
memory_t 	G_maxmemory;		// Max memory under miser control
miser_resources_t G_resource;

void *start_request = (void *)req.mr_req_buffer.md_data;
void *end_request   = (void *)(&req + 1);


/*
 * usage
 *      Print an usage message with a brief description of each possible 
 * option and quit the daemon.  Messages goes to stderr and SYSLOG.
 */
void
usage(const char *pname)
{
	merror("\nUsage: %s [-vd] (-c maxCPUs -m maxMemory -f configFile | C)\n" 	"\nValid options are:\n"
	"\t-v\tVerbose, prints additional output.\n"
	"\t-d\tPrints debug output, does not relinquish terminal.\n"
	"\t-m\tThe maximum amount of Memory that miser can use.\n"
	"\t-c\tThe maximum number of CPUs that miser can use.\n"
	"\t-f\tSpecifies the location of the configuration file.\n"
	"\t-C\tClear all miser resource reservations and exit.\n",
		pname);
	exit(1);

} /* usage */


static void
detachFromTTY(void)
{ 
	switch (fork()) {
		case 0:
			break;

		case -1:
			merror("Could not fork: %s", strerror(errno));
				exit(1);

		default:
			exit(0);
	}

	(void) setsid();

	openlog("miser", LOG_PID|LOG_CONS, LOG_DAEMON);

} /* detachFromTTY */
		

/*
 * handle_exception
 *	Act on a job based on flag set by the kernel.
 */
static error_t 
handle_exception(miser_job_t* job, int reason)
{
	char timebuf[30];	// Hold current time string
	char* reason_str;	// Point to the reason string

        if (reason == MISER_KERN_TIME_EXPIRE)
                reason_str = "Time ran out.";

        else if (reason == MISER_KERN_MEMORY_OVERRUN)
                reason_str = "Memory overrun.";

	else 
		reason_str = "Unknown.";	

	// Check flag - set in kernel and take appropriate action. 
	if (job->mj_segments->ms_flags & MISER_EXCEPTION_TERMINATE) {

		// Kill the job
		if (kill(-job->mj_bid, SIGKILL) == -1) {
			merror("miser could not kill job %d: %s",
				job->mj_bid, strerror(errno));
			return MISER_ERROR;
		}

		// Write an error message with a reason 
		curr_time_str(timebuf);
		msyslog(LOG_INFO, "Killing job %d at %s - %s", job->mj_bid, 
			timebuf, reason_str);

		return MISER_ERR_OK;
	} 

	// Job priority reclassified as weightless 
	else if (job->mj_segments->ms_flags & MISER_EXCEPTION_WEIGHTLESS) {

		return MISER_ERR_OK;
	} 

	else {	// Unknown exception flag
		merror("Exception %x not implemented.", 
			job->mj_segments->ms_flags);

		return MISER_ERROR;
	}

} /* handle_exception */


/*
 * write_init_mesg
 *	Write message indicating miser initialization 	
 */
static int 
write_init_mesg(char *config_file, int cpus, memory_t memory)
{
	char pathbuf[MAXPATHLEN];	// Hold resolved path for config file
	char timebuf[30];	 	// Hold current time string

	// Verify miser config file location, return 0 if invalid
	if (!realpath(config_file, pathbuf)) {
		merror("miser: %s - %s", config_file, strerror(errno));
		return 0;
	}

	curr_time_str(timebuf);
	msyslog(LOG_INFO,"[%s] Starting miser (CONFIG: %s CPU: %d MEM: %4.0b)",
		timebuf, pathbuf, cpus, (float) memory);

	return 1;

} /* write_init_mesg */


/*
 * clear_resources
 * 	Free all reserved miser resources in kernel and exit miser daemon
 */
void
clear_resources() 
{
	miser_request_t	mreq;
	miser_bids_t*	mb;
	bid_t*		bid;	

	// Initialize miser_request_t struct
	memset(&mreq, 0, sizeof(mreq));

	// Reset time_quantum to 1
	time_quantum = 1;

	// Reset time_quantum in kernel
	if (sysmp(MP_MISER_SETRESOURCE, MPTS_MISER_QUANTUM, &time_quantum)) {
		merror("miser: Failed to set time quantum %s", strerror(errno));
		exit(1);
	}

	// Find if active miser job(s) exist in kernel
	if (sysmp(MP_MISER_GETRESOURCE, MPTS_MISER_BIDS, &mreq) == -1) {
		merror ("miser: Getting miser bids failed");
		exit(1);
	}

	// Load bids from result buffer
	mb = (miser_bids_t*) &mreq.mr_req_buffer.md_data;

	// Print active miser jobids and fail clear resources 
	if (mb->mb_count) {
		merror("miser: Processes are still active");

		for (bid = mb->mb_buffer; bid != mb->mb_buffer + mb->mb_count; 
			bid++) {
			merror("miser: miser jobid = %d", *bid);
		}	

		merror("Could not free resources.\n"
			"Please restart miser and wait for jobs to terminate\n"
			"or restart miser and kill all the jobs, and then\n"
			"restart miser with the -C flag.");

		exit(1);
	}

	// Reinitialize miser resource to 0
	G_resource.mr_cpus   = 0;
	G_resource.mr_memory = 0;

	// Free miser cpus in kernel
	if (sysmp(MP_MISER_SETRESOURCE, MPTS_MISER_CPU, &G_resource)) {
		merror("miser: set maxcpus failed: %s", strerror(errno));
		exit(1);	
	}

	// Free miser memory in kernel
	if (sysmp(MP_MISER_SETRESOURCE, MPTS_MISER_MEMORY, &G_resource)) {
		merror("miser: set maxmemory failed: %s", strerror(errno));
		exit(1);
	}

	msyslog(LOG_INFO, "Successfully freed all miser resources");

	exit(0);

} /* clear_resources */


int 
main(int argc, char** argv)
{
        int	opt;		// Hold command line option char 
	int	tcpus = 0;	// Total cpus requested in command line
	int	clear = 0;	// Flag to indicate clear resource requested
	char*	filename = 0;	// Hold config file path

	// Initialize Global variables
	G_maxcpus 	= 0;
	G_maxmemory	= 0;	
	G_maxcpus	= sysmp(MP_NPROCS);
	G_syslog	= true;

	// Parse command line options and set appropriate flags
	while ((opt = getopt(argc, argv, "c:Cdf:m:v")) != -1) { 

		switch(opt) {

		case 'c':	// max cpus 
			tcpus = atoi(optarg); 
			break;

		case 'C':	// clear resource 
			clear = 1;
			break;

		case 'd':	// retain TTY
			G_syslog = false;
			break;

		case 'f':	// config file
			filename = optarg;
			break;

		case 'm':	// max memory
			if (!fmt_str2bcnt(optarg, &G_maxmemory))
				usage(argv[0]);
			break;

		case 'v':	// extra output
			G_verbose = true;
			break;	

		default:
			usage(argv[0]);

		} // switch 

	} // while

	// Send log information to SYSLOG and detach from the terminal 
	if (G_syslog)
		detachFromTTY();

	// Free all previous miser resource reservations and exit 
	if (clear) 
		clear_resources();

	// Config filename missing - exit 
	if (!filename) {
		merror("miser: no miser config file specified.");
		usage(argv[0]);
	}
	
	// Requested cpus is greater than total available - exit
	if (tcpus > G_maxcpus)  {
		merror("miser: cpus requested (%d) > cpus available (%d).",
			tcpus, G_maxcpus);
		usage(argv[0]);
	}

	// Insert policy in the policies list
	copy(miser_policy_begin, miser_policy_end, back_inserter(policies));

	//
	if (!write_init_mesg(filename, tcpus, G_maxmemory)) {
		merror("miser: Fatal error. Aborting.");	
		exit(1);
	}

	system_id = miser_qid("system");
	G_maxcpus = tcpus;

	error ret_val = miser_reset(filename, active_queues);
	setnow();

	if (ret_val != MISER_ERR_OK) {
		merror("miser: Failed to initialize miser using file \"%s\"",
			filename);
		exit(1);
	}

	miser_rebuild();
	
	// Initialize miser resources with requested values
	G_resource.mr_cpus   = G_maxcpus;
	G_resource.mr_memory = G_maxmemory;	

	// Initialize miser cpus in kernel
	if (sysmp(MP_MISER_SETRESOURCE, MPTS_MISER_CPU, &G_resource)) {
		merror("miser: set maxcpus failed: %s", strerror(errno));
		exit(1);	
	}

	// Initialize miser memory in kernel
	if (sysmp(MP_MISER_SETRESOURCE, MPTS_MISER_MEMORY, &G_resource)) {
		merror("miser: set maxmemory failed: %s", strerror(errno));

		// Unset miser cpus in kernel since memory setting failed
 		if (sysmp(MP_MISER_SETRESOURCE, MPTS_MISER_CPU, &G_resource)) {
               		merror("miser: could not unset cpus failed: %s", 
					strerror(errno));
		}

		exit(1);
	}

	// The miser initialization completed successfully
	msyslog(LOG_INFO,"Miser is ready to accept jobs.");

	if (G_syslog)  {
		int fd = open("/dev/null", O_RDWR);
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
	}

	/* Loop for ever and attend to events */
        for (;;) {
                if (sysmp(MP_MISER_GETREQUEST, &req)) {
			merror("miser: get request failed");
			exit(1);
		}

		G_error = MISER_ERR_OK;		/* reinitialize */

                STRING_PRINT("---------- MAIN LOOP ----------");
		setnow();
		SYMBOL_PRINT(now);

                switch (req.mr_req_buffer.md_request_type) {

                case MISER_USER_JOB_SUBMIT:
		{
                        id_type_t queue_id;

                	STRING_PRINT("MISER_USER_JOB_SUBMIT");

                        miser_job_t* job
				= (miser_job_t*) req.mr_req_buffer.md_data;

                        memcpy(&queue_id, &job->mj_queue, sizeof(queue_id));

                        error_t error = job_schedule(queue_id, req.mr_bid,
			       job->mj_segments, job->mj_segments+job->mj_count,
			       &job->mj_etime);

			for (int i = 0; i < job->mj_count; i++) 
				SYMBOL_PRINT(job->mj_segments[i]);

			setnow();
			SYMBOL_PRINT(now);

                        req.mr_req_buffer.md_error = error;
			job->mj_bid = req.mr_bid;
                        break;
		}

		case MISER_USER_JOB_KILL:
		{
                	STRING_PRINT("MISER_USER_JOB_KILL");
			bid_t *bid = (bid_t*) req.mr_req_buffer.md_data;
			req.mr_req_buffer.md_error = job_deschedule(*bid);
			break;
		}

                case MISER_KERN_EXIT:
		{
                	STRING_PRINT("MISER_KERN_EXIT");
                        bid_t bid = *(bid_t *)req.mr_req_buffer.md_data;
                        req.mr_req_buffer.md_error = job_deschedule(bid);
                        break;
		}

                case MISER_ADMIN_QUEUE_MODIFY_MOVE:
		{
                	STRING_PRINT("MISER_ADMIN_QUEUE_MODIFY_MOVE");
                        miser_move_t* move
				= (miser_move_t*)req.mr_req_buffer.md_data;
                        req.mr_req_buffer.md_error
				= queueid_move_block(move->mm_from,
						     move->mm_to,
						     move->mm_qsegs,
						     move->mm_qsegs
							+ move->mm_count);
                        break;
		}

                case MISER_ADMIN_QUEUE_QUERY_JOB:
		{
                	STRING_PRINT("MISER_ADMIN_QUEUE_QUERY_JOB");
                        miser_bids_t* bids
				= (miser_bids_t*) req.mr_req_buffer.md_data;
                        id_type_t queue_id;
                        memcpy(&queue_id, &bids->mb_queue, sizeof(queue_id));
			bid_t* end = queue_query_jobs(queue_id,
                                                      bids->mb_first,
                                                      bids->mb_buffer,
                                                      bids->mb_buffer
                                                            + bids->mb_count);
			bids->mb_count = end - bids->mb_buffer;
                        req.mr_req_buffer.md_error = MISER_ERR_OK;
                        break;
		}

                case MISER_ADMIN_QUEUE_QUERY_RESOURCES:
		{
                	STRING_PRINT("MISER_ADMIN_QUEUE_QUERY_RESOURCES");
                        miser_queue_resources_t* qrsc
				= (miser_queue_resources_t*)
					req.mr_req_buffer.md_data;
                        id_type_t queue_id;
                        memcpy(&queue_id, &qrsc->mqr_queue, sizeof(queue_id));
                        req.mr_req_buffer.md_error
				= queue_query_resources(queue_id, qrsc);
							
                        break;
		}

		case MISER_ADMIN_QUEUE_QUERY_NAMES:
		{
                	STRING_PRINT("MISER_ADMIN_QUEUE_QUERY_NAMES");
			miser_queue_names_t *mqn = (miser_queue_names_t*)
					req.mr_req_buffer.md_data;
			id_type_t *end = mqn->mqn_queues + mqn->mqn_count;
			req.mr_req_buffer.md_error 
				= queue_query_names(mqn->mqn_queues, end);
			mqn->mqn_count = end - mqn->mqn_queues;
			break;
		}

                case MISER_ADMIN_JOB_QUERY_SCHEDULE:
		{
                	STRING_PRINT("MISER_ADMIN_JOB_QUERY_SCHEDULE");
                        miser_schedule_t* qsched
				= (miser_schedule_t*) req.mr_req_buffer.md_data;
			miser_seg_t *end
				= qsched->ms_buffer + qsched->ms_count;

                        req.mr_req_buffer.md_error
				= job_query_schedule(qsched->ms_job,
				qsched->ms_first, qsched->ms_buffer, end);

			qsched->ms_count = end - qsched->ms_buffer;
                        break;
		}

                case MISER_ADMIN_QUEUE_MODIFY_RESET:
		{
                	STRING_PRINT("MISER_ADMIN_QUEUE_MODIFY_RESET");
                        char* fname = req.mr_req_buffer.md_data;
                        queuelist* new_queue = 0;
                        req.mr_req_buffer.md_error
				= miser_reset(fname, new_queue);
                        if (req.mr_req_buffer.md_error == MISER_ERR_OK)
				req.mr_req_buffer.md_error
				     = queue_rebuild_from_queues(new_queue);
			write_init_mesg(fname, G_maxcpus, G_maxmemory);
                        break;
		}

                case MISER_ADMIN_TERMINATE:
		{
                	STRING_PRINT("MISER_ADMIN_TERMINATE");
                        exit(1);
                        break;
		}

		case MISER_KERN_TIME_EXPIRE:
		{
                	STRING_PRINT("MISER_KERN_TIME_EXPIRE");
			STRING_PRINT("[main] Killing a batch job");

			miser_job_t* job
                                = (miser_job_t*) req.mr_req_buffer.md_data;
			req.mr_req_buffer.md_error = handle_exception(job,
							MISER_KERN_TIME_EXPIRE);
			break;
		}

		case MISER_KERN_MEMORY_OVERRUN:
		{
                	STRING_PRINT("MISER_KERN_MEMORY_OVERRUN");
			miser_job_t* job 
				= (miser_job_t*) req.mr_req_buffer.md_data;
			req.mr_req_buffer.md_error = handle_exception(job,
						MISER_KERN_MEMORY_OVERRUN);
			break;
		}

                default:
                        // no idea but an error has occured at this point!
                        merror ("Unhandled message %d",
				req.mr_req_buffer.md_request_type);
			req.mr_req_buffer.md_error = MISER_ERR_BAD_TYPE;

                } /* switch */

		STRING_PRINT(miser_error((error_t)req.mr_req_buffer.md_error));

                if (sysmp(MP_MISER_RESPOND, &req) == -1) {

			int req_type = req.mr_req_buffer.md_request_type;

			if (errno != EINVAL)  {
				merror("miser: reponse failed");
				exit(1);
			}

			if (req_type == MISER_USER_JOB_SUBMIT) {

				if (req.mr_req_buffer.md_error !=
						 MISER_ERR_OK) 
					continue;

				if (job_deschedule(req.mr_bid) == 
							MISER_ERR_OK) 
					continue;
			}
		}

	} /* for */

} /* main */
