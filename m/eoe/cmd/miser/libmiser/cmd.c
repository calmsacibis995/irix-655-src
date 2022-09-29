/*
 * eoe/cmd/miser/libmiser/cmd.c
 *	Library functions that implement miser commands.
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


#include <sys/sysmp.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include "libmiser.h"


time_t       time_quantum;
miser_data_t miser_data;


/*
 * miser_get_qnames
 *	Returns a pointer to miser_queue_names_t containing a list of
 * configured qids and a count.  Returns NULL in case of error.
 */
miser_queue_names_t*
miser_get_qnames(void)
{
	id_type_t *first, *last;
	miser_queue_names_t *mqn = (miser_queue_names_t*)miser_data.md_data;

	/* Point to qname memory location in the structure */
	last  = (id_type_t *) (&miser_data + 1);
	first = (id_type_t *) mqn->mqn_queues;
	last  = first + (last - first);

	/* Build the request */
	mqn->mqn_count = last - mqn->mqn_queues;

	/* Set request type flag for the sysmp call */
	miser_data.md_request_type = MISER_ADMIN_QUEUE_QUERY_NAMES;

	/* Make the sysmp call to get data requested */
	if (sysmp(MP_MISER_SENDREQUEST, &miser_data)) {
		merror("Failed to contact miser: %s", strerror(errno));
		return NULL;
	}

	/* Print miser error, if set, and return NULL */
	if (miser_data.md_error) {
		merror("miser query failed: %s", 
		       miser_error(miser_data.md_error));
		return NULL;
	}

	/* Return a pointer to the structure with data requested */
	return mqn;

} /* miser_get_qnames */


/*
 * miser_get_jids
 *	Returns a pointer to miser_bids_t containing a list of
 * active bids and a count for the specified qid.  Returns NULL
 * in case of error.
 */
miser_bids_t* 
miser_get_jids(id_type_t qid, int start)
{
	bid_t *first, *last;
        miser_bids_t *jq = (miser_bids_t *)miser_data.md_data;

	/* Point to qname memory location in the structure */
	last  = (bid_t *)(&miser_data + 1);
	first = (bid_t *)jq->mb_buffer;
	last  = first + (last - first);

	/* Build the request */
	jq->mb_first = start;
	jq->mb_queue = qid;
	jq->mb_count = last - jq->mb_buffer;

	/* Set request type flag for the sysmp call */
        miser_data.md_request_type = MISER_ADMIN_QUEUE_QUERY_JOB;

	/* Make the sysmp call to get data requested */
	if (sysmp(MP_MISER_SENDREQUEST, &miser_data)) {
		merror("Failed to contact miser: %s\n", strerror(errno));
		return NULL;
	}

	/* Print miser error, if set, and return NULL */
	if (miser_data.md_error) {
		merror("miser query failed: %s\n", 
			miser_error(miser_data.md_error));
		return NULL;
	}

	/* Return a pointer to the structure with data requested */
	return jq;

} /* miser_get_jids */


/*
 * miser_get_jsched
 *	Returns a pointer to miser_schedule_t containing a schedule for
 *	the specified jid.  Returns NULL in case of error.
 */
miser_schedule_t*
miser_get_jsched(bid_t jid, int start)
{
	miser_seg_t *first, *last;
	miser_schedule_t *js = (miser_schedule_t *)miser_data.md_data;

	/* Point to job schedule memory location in the structure */
	last  = (miser_seg_t *)(&miser_data + 1);
	first = (miser_seg_t *)js->ms_buffer;
	last  = first + (last - first);

	/* Build the request */
        js->ms_first = start;
        js->ms_job   = jid;
        js->ms_count = last - js->ms_buffer;

	/* Set request type flag for the sysmp call */
        miser_data.md_request_type = MISER_ADMIN_JOB_QUERY_SCHEDULE;

	/* Make the sysmp call to get data requested */
        if (sysmp(MP_MISER_SENDREQUEST, &miser_data)) {
                merror("Could not contact miser: %s\n", strerror(errno));
                return NULL;
        }

	/* Print miser error, if set, and return NULL */
        if (miser_data.md_error) {

		/* Print all error messages except when jid not found */ 
		if (miser_data.md_error != MISER_ERR_JOB_ID_NOT_FOUND)
			merror("miser query failed: %s\n",
				miser_error(miser_data.md_error));

                return NULL;
        }

        return js;

} /* miser_get_jsched */


/*
 * miser_get_qstat
 *	Prints a time segments and resource available during that period
 * for a specified miser queue.
 */
int 
miser_get_qstat(char* queue, int realstart)
{
        miser_resources_t *buffer, *current;
	int remaining = 0, qsize = 0, nextstart = 0, start = 0, count = 0;

        miser_queue_resources_t *qr = 
                (miser_queue_resources_t *)miser_data.md_data;
        miser_data.md_request_type = MISER_ADMIN_QUEUE_QUERY_RESOURCES;

	qsize =  (miser_resources_t *)(&miser_data + 1) - 
			(miser_resources_t *)qr->mqr_buffer;

	qr->mqr_queue = miser_qid(queue);
	qr->mqr_start = realstart != 0 ? realstart/time_quantum : 0;
	qr->mqr_count = qsize; 

	printf ("Queue: %s with quantum %d secs \n", queue, miser_quantum());
	printf (" %10s %12s %8s\n", "Time","CPU", "Memory"); 	
	printf ("%s %2s %5s\n", "-------------------", "-----","--------");  

	do {
		if (sysmp(MP_MISER_SENDREQUEST, &miser_data)) {
			merror("Could not contact miser: %s\n", 
				strerror(errno));
			return 0;
		}

		if (miser_data.md_error) {
			merror("miser_qinfo failed: %s\n",
			       miser_error(miser_data.md_error));
			return 0;
		}

		if (qr->mqr_count > qsize)   { 
			if (remaining == 0) { 
				remaining = count = qr->mqr_count;
				start = nextstart = qr->mqr_start;
				qr->mqr_count = qsize;
				buffer = malloc(sizeof(miser_resources_t) * 
					count);
				memset(buffer, 0, count);
				current = &buffer[0];
			}

			if (qsize > remaining) {
				qr->mqr_count = remaining;
				remaining = 0;	

			} else {
				qr->mqr_count = qsize;
				remaining -= qsize;
				nextstart += qsize;
			}
		}	
		else { /* qr->mqr_count <= qsize */
			count = qsize;
			buffer = malloc(sizeof(miser_resources_t) * count);
			current = &buffer[0];
			start = qr->mqr_start;
		}

		qr->mqr_start = nextstart;
		memcpy(current, qr->mqr_buffer, 
			qsize * sizeof(miser_resources_t));
		current += qr->mqr_count;

	} while (remaining > 0);

	miser_print_block(count, buffer, current, start);
	free(buffer);

	return 1;

} /* miser_get_qstat */


/*
 * miser_get_prpsinfo
 *      Fills the prpsinfo_t structure with information on the pid requested.
 * Returns 0 in case of error.
 */
int
miser_get_prpsinfo(int pid, prpsinfo_t *pinfo)
{
        int     procfd;
        char    pname[PRCOMSIZ];

        sprintf(pname, "%s%010ld", _PATH_PROCFSPI, pid);

        if ((procfd = open(pname, O_RDONLY)) == -1)
                return 0;

	if ((ioctl(procfd, PIOCPSINFO, pinfo)) == -1) 
		return 0;

	close(procfd);

        return 1;	/* Success */

} /* miser_get_prpsinfo */


/*
 * miser_submit
 *	Submit a job to a miser queue.
 */
int
miser_submit(char *qname, miser_data_t *req)
{
        miser_job_t* job = (miser_job_t *) req->md_data;

        req->md_request_type = MISER_USER_JOB_SUBMIT;
        job->mj_queue = miser_qid(qname);

        if (setpgid(0, 0) == -1) {

                merror_hdr("submit failed: ");

                if (errno == EINVAL)
                        merror("terminal does not support job control.");

                else if (errno == EPERM)
                        merror("process is a session leader.");

                else
                        merror("internal miser error.");

                return 0;
        }

        if (sysmp(MP_MISER_SENDREQUEST, req)) {

                if (errno == ESRCH)
			merror("submit failed: could not contact miser.");

                else if (errno == EFAULT)
                        merror("submit failed: args could not be processed.");

                else if (errno == EBUSY)
                        merror("submit failed: either process belongs to "
				"a cpuset or is mustrun.");

                else if (errno == ENOMEM)
                        merror("submit failed: not enough memory.");

                else
                        merror("submit failed: %s", strerror(errno));
                return 0;
        }

        if (req->md_error) {
                merror("submit failed: %s", miser_error(req->md_error));
                return 0;
        }

        return 1;

} /* miser_submit */


/*
 * miser_move
 *	Move a block of resources from one queue to another.
 */
int
miser_move(char *srcq, char *dstq, miser_data_t *req)
{
        miser_move_t* move = (miser_move_t *) req->md_data;

        req->md_request_type = MISER_ADMIN_QUEUE_MODIFY_MOVE;

        move->mm_from = miser_qid(srcq);
        move->mm_to   = miser_qid(dstq);

        if (sysmp(MP_MISER_SENDREQUEST, req)) {
                merror("move failed: could not contact miser");
                return 0;
        }

        if (req->md_error) {
                merror("move failed: %s", miser_error(req->md_error));
                return 0;
        }

        return 1;

} /* miser_move */
