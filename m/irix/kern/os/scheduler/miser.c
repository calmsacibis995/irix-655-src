/*
 * irix/kern/os/scheduler/miser.c
 *	Implements miser sysmp calls.
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

#include <sys/types.h>
#include <sys/sema.h>
#include <sys/errno.h>
#include <sys/syssgi.h>
#include <sys/miser_public.h>
#include <ksys/vproc.h>
#include <sys/cred.h>
#include <sys/kabi.h>
#include <sys/kmem.h>
#include <sys/capability.h>
#include <ksys/vfile.h>
#include <sys/vnode.h>
#include <sys/batch.h>
#include <sys/space.h>
#include <sys/uthread.h>
#include <ksys/fdt.h>
#include <sys/q.h>
#include <sys/idbgentry.h>
#include <sys/proc.h>
#include <sys/sysmp.h>
#include <ksys/vpag.h>
#include <ksys/vm_pool.h>
#include <sys/sysmacros.h>
#include <ksys/sthread.h>
#include <sys/cpumask.h>
#include <sys/cmn_err.h>
#include <sys/par.h>
#include <sys/kthread.h>
#include <sys/runq.h>
#include <sys/pda.h>
#include <sys/prctl.h>

#include "os/proc/pproc_private.h"      /* XXX bogus */

static miser_pid = -1;
static lock_t	miser_lock;
static lock_t   miser_batch_lock;
static sv_t	miser_sv;
static quanta_t	time_quantum;
static int	miser_thread = 0;
vm_pool_t	*miser_pool;

cpuset_t* 	cpusets;
int16_t*	cpu_to_cpuset;
int32_t		cpuset_size;
extern	int 	batch_freecpus;
extern 	int 	batch_oldcpus;

typedef enum miser_enum {
	MISER_NEEDRESPONSE,
	MISER_NORESPONSE
} miser_response_t;

typedef struct miser_q {
	struct q_element_s mq_queue;
	job_t *	mq_job;
	miser_response_t	mq_response;
	struct cred*	mq_cred;
	lock_t		mq_lock;
	sv_t		mq_sv;
	int		mq_error;
	int		mq_holds;
	int 		mq_lbolt;
	int		mq_alive;
	vpagg_t		*mq_vpagg;
	miser_request_t	mq_request;
} miser_queue_t;

static void miser_handle_exception(miser_queue_t *mq);

struct q_element_s	miser_queue;
struct q_element_s	miser_batch;

#define MISER ((miser_job_t *)mq->mq_request.mr_req_buffer.md_data)
#define SEG (MISER->mj_segments)
#define IS_SUBMIT(mq) ((mq)->mq_request.mr_req_buffer.md_request_type \
						== MISER_USER_JOB_SUBMIT)
	  

#define FIRST_MISER_QUEUE (miser_queue.qe_forw->qe_parent)
#define FIRST_BATCH_QUEUE (miser_batch.qe_forw->qe_parent)
#define BATCH(mq)	  (mq == (miser_queue_t*)miser_batch.qe_parent)
#define NEXT(mq)	  ((miser_queue_t*)(mq)->mq_queue.qe_forw->qe_parent)
static void 		miser_drop_request(miser_queue_t *mq);
static void 		release_miser(void *arg);
static void 		miser_return_error(int error);
static int 		miser_return(miser_queue_t *mq);
static miser_queue_t * 	miser_queue_get(enum miser_enum type);
static void 		miser_queue_release(miser_queue_t *mq);
static void 		miser_sched_preprocess(miser_queue_t *, pid_t);
static void 		miser_sched_postprocess(miser_queue_t *);
static void		miser_adjust_time(miser_queue_t *);
static int 		miser_cpuset_process(miser_data_t *, sysarg_t, sysarg_t);
static void		miser_time_lock_expire(miser_queue_t *);
miser_queue_t* 		miser_find_bid(bid_t bid);
extern int batch_memory;

#ifdef DEBUG
int miser_debug = 0;

#define MISER_DEBUG() if (miser_debug) \
	{ qprintf("cpu %d line %d\n", cpuid(), __LINE__); }
#else
#define MISER_DEBUG() ;
#endif

void
miser_init()
{
	int i;
	spinlock_init(&miser_lock, "miserq");
	spinlock_init(&miser_batch_lock, "miserb");
	sv_init(&miser_sv, SV_DEFAULT, "miserg");
	init_q_element(&miser_queue, (void *)&miser_queue);
	init_q_element(&miser_batch, (void *)&miser_batch);
	cpuset_size = maxcpus + 2;
	cpusets = kmem_zalloc(sizeof(cpuset_t) * cpuset_size, KM_SLEEP);
	cpu_to_cpuset = kmem_zalloc(sizeof(uint16_t) * maxcpus, KM_SLEEP);
	PRIMARY_IDLE = -1;

	for (i = 2; i < cpuset_size; i++)  {
		spinlock_init(&cpusets[i].cs_lock, "cpuset");
		cpusets[i].cs_idler = -2;
		cpusets[i].cs_mastercpu = CPU_NONE;
	}
	for (i = 0; i < maxcpus; i++)  {
		cpusets[i].cs_lbpri = -1;
		cpu_to_cpuset[i] = 1;
	}
	global_cpuset.cs_count = 1;
	global_cpuset.cs_mastercpu = master_procid;

	for (i=0; i<maxcpus; i++)
		if (cpu_enabled(i))
			CPUMASK_SETB(global_cpuset.cs_node_cpumask, i);
	global_cpuset.cs_nodemask = CNODEMASK_BOOTED_MASK;
	global_cpuset.cs_nodemaskptr = &global_cpuset.cs_nodemask;
}

void
miser_purge(void) 
{
	miser_queue_t *mq;

	int s = mutex_spinlock(&miser_batch_lock);
	for (mq = FIRST_BATCH_QUEUE; !BATCH(mq); mq = NEXT(mq)) {
		nested_spinlock(&mq->mq_lock);
		if (mq->mq_job &&  mq->mq_job->j_nthreads && 
			mq->mq_job->b_deadline < lbolt) {
			miser_time_lock_expire(mq);
		}
		nested_spinunlock(&mq->mq_lock);
	}
	mutex_spinunlock(&miser_batch_lock, s);

}	

int
miser_reset_job(sysarg_t arg)
{
	miser_request_t mr;
	miser_queue_t *mq;
	miser_job_t *mj;
	int s;
	int error = EINVAL;

	if (miser_pid != current_pid())
		return EPERM;
	if (copyin((caddr_t)(__psint_t) arg, &mr, sizeof(mr)))
		return EFAULT;

 	mj = (miser_job_t *) &mr.mr_req_buffer.md_data;

	s = mutex_spinlock(&batch_queue.b_lock);
	nested_spinlock(&miser_batch_lock);
	mq = miser_find_bid(mj->mj_bid);
	nested_spinlock(&mq->mq_lock);	
	if (!mq->mq_job) {
		nested_spinunlock(&mq->mq_lock);
		nested_spinunlock(&miser_batch_lock);
		mutex_spinunlock(&batch_queue.b_lock, s);
		qprintf("[miser_reset_job]: No job found\n");
		return error;
	} 

	rmq(&mq->mq_queue);
	job_nested_lock(mq->mq_job);
	if (mq->mq_job->b_time > lbolt) {
		rmq(&mq->mq_job->j_queue);
		bcopy(mr.mr_req_buffer.md_data, 
			mq->mq_request.mr_req_buffer.md_data,
				sizeof(mr.mr_req_buffer.md_data));
		
		mq->mq_lbolt = lbolt;	
		miser_adjust_time(mq);
		batch_set_params(mq->mq_job, SEG);
		batch_push_queue_locked(mq->mq_job);
		error = 0;
	}  
	q_insert_before(&miser_batch, &mq->mq_queue);

	job_nested_unlock(mq->mq_job);
	nested_spinunlock(&mq->mq_lock);
	nested_spinunlock(&miser_batch_lock);
	mutex_spinunlock(&batch_queue.b_lock, s);	

	return error;
}	
	
INLINE int
mq_is_okay(miser_queue_t *mq)
{
	do {
		nested_spinlock(&mq->mq_lock);
		if (mq->mq_holds == 1 && 
				mq->mq_response == MISER_NEEDRESPONSE) {
			rmq(&mq->mq_queue);
			mq->mq_holds--;
			nested_spinunlock(&mq->mq_lock);
			miser_queue_release(mq);
			mq = FIRST_MISER_QUEUE;
			continue;
		} else  {
			return 1;
		}
	} while(!q_empty(&miser_queue));
	return 0;
}

int
miser_get_request(sysarg_t arg)
{
	miser_queue_t *mq;
	int s;

	if (miser_pid == -1)
		return ESRCH;
	if (miser_pid != current_pid())
		return EBUSY;

	MISER_DEBUG();

	s = mutex_spinlock(&miser_lock);
	while (q_empty(&miser_queue) || !mq_is_okay(FIRST_MISER_QUEUE)) {
		if (sv_wait_sig(&miser_sv, PZERO + 1, &miser_lock, s) == -1)
			return EINTR;
		s = mutex_spinlock(&miser_lock);
	}

	mq = FIRST_MISER_QUEUE;
        if (copyout(&mq->mq_request,
                (caddr_t)(__psint_t)arg, sizeof(mq->mq_request)))
        {
                nested_spinunlock(&mq->mq_lock);
                miser_return_error(EFAULT);
                mutex_spinunlock(&miser_lock, s);
                return EFAULT;
        }
        nested_spinunlock(&mq->mq_lock);
        mutex_spinunlock(&miser_lock, s);
	MISER_DEBUG();
	return 0;
}

int
miser_respond(sysarg_t arg)
{
	int s;
	miser_queue_t *mq, *mqbid; 

	MISER_DEBUG();
        if (miser_pid != current_pid()) 
                return EBUSY;

	s = mutex_spinlock(&miser_lock);
	mq = FIRST_MISER_QUEUE;

	ASSERT(mq);
	nested_spinlock(&mq->mq_lock);
        if (mq->mq_response == MISER_NEEDRESPONSE &&
		mq->mq_holds == 1) {
                rmq(&mq->mq_queue);
                nested_spinunlock(&mq->mq_lock);
                mutex_spinunlock(&miser_lock, s);
                return EINVAL;
        }

	if (copyin((caddr_t)(__psint_t)arg,
	    &mq->mq_request, sizeof(mq->mq_request)))
	{
                nested_spinunlock(&mq->mq_lock);
		miser_return_error(EFAULT);
		mutex_spinunlock(&miser_lock, s);
		return EFAULT;
	}

	rmq(&mq->mq_queue);
	nested_spinunlock(&mq->mq_lock);
	mutex_spinunlock(&miser_lock, s);

	if (mq->mq_request.mr_req_buffer.md_request_type 
				== MISER_KERN_TIME_EXPIRE) {
		miser_handle_exception(mq);
	}

	if (mq->mq_request.mr_req_buffer.md_request_type
				== MISER_KERN_EXIT) {
		s = mutex_spinlock(&miser_batch_lock);
		mqbid = miser_find_bid(mq->mq_request.mr_bid);
		ASSERT(mq);
		rmq(&mqbid->mq_queue);
		mutex_spinunlock(&miser_batch_lock, s);
		PID_BATCH_LEAVE(mqbid->mq_request.mr_bid);
		miser_queue_release(mqbid);
	}
				
	return miser_return(mq);
}

void
miser_cleanup(vpagg_t *vpagg)
{
	pgno_t rss_limit;

	VPAG_GET_VM_RESOURCE_LIMITS(vpagg, &rss_limit);
	vm_pool_update_guaranteed_mem(miser_pool, -rss_limit);
}

miser_queue_t*
miser_find_bid(bid_t bid) 
{
	miser_queue_t *mq;
	ASSERT(spinlock_islocked(&miser_batch_lock));
	for (mq = FIRST_BATCH_QUEUE; !BATCH(mq); mq = NEXT(mq)) 
		if (mq->mq_request.mr_bid == bid) 
			return mq;
	return 0;
}	 

static void
miser_adjust_time(miser_queue_t *mq)
{
	int i = 0;
	for (i = 0; i < MISER->mj_count; i++) {
		SEG[i].ms_etime = SEG[i].ms_etime * HZ + mq->mq_lbolt;
	}
}

void 
miser_send_request_error(miser_queue_t *mq)
{
	vpgrp_t *vpgrp;

	if (mq->mq_request.mr_req_buffer.md_request_type
			== MISER_USER_JOB_SUBMIT) {
		prs_flagclr(&curuthread->ut_pproxy->prxy_sched, PRSBATCH);

		vpgrp = VPGRP_LOOKUP(mq->mq_request.mr_bid);
		if (vpgrp) {
			VPGRP_CLEARBATCH(vpgrp);
			VPGRP_RELE(vpgrp);
		}
		PID_BATCH_LEAVE(mq->mq_request.mr_bid);
		mq->mq_vpagg = 0;
	}
}

int
miser_send_request_scall(sysarg_t arg, sysarg_t file)
{
	int error = 0;
	int s;
	miser_queue_t *mq;

	vproc_t *vpr;
	vpgrp_t *vpgrp;
	vp_get_attr_t attr;

	MISER_DEBUG();


	vpr = VPROC_LOOKUP(current_pid());
	if (vpr == 0) 
		return ESRCH;
	VPROC_GET_ATTR(vpr, VGATTR_PGID, &attr);
	VPROC_RELE(vpr);
	ASSERT(!issplhi(getsr()));

	mq = miser_queue_get(MISER_NEEDRESPONSE);
	mq->mq_cred = get_current_cred();

	if (copyin((caddr_t)(__psint_t)arg,
	    &mq->mq_request.mr_req_buffer,
	    sizeof(mq->mq_request.mr_req_buffer)))
	{
		miser_queue_release(mq);
		MISER_DEBUG();
		return EFAULT;
	}

	switch (mq->mq_request.mr_req_buffer.md_request_type) 
	{	
		case MISER_CPUSET_CREATE:
		case MISER_CPUSET_DESTROY:
		case MISER_CPUSET_LIST_PROCS:
		case MISER_CPUSET_MOVE_PROCS:
		case MISER_CPUSET_ATTACH:
		case MISER_CPUSET_QUERY_CPUS:
		case MISER_CPUSET_QUERY_NAMES:
		case MISER_CPUSET_QUERY_CURRENT:
		error = miser_cpuset_process(&mq->mq_request.mr_req_buffer, arg,file);  
		miser_queue_release(mq);
		return error;
	}

	if (miser_pid == -1) {
		miser_queue_release(mq);
		MISER_DEBUG();
		return ESRCH;
	}

	if (mq->mq_request.mr_req_buffer.md_request_type
			 == MISER_USER_JOB_SUBMIT) {

		/* 
		 * Try to set the process group id to be the bid. 
		 */

		vpgrp = VPGRP_LOOKUP(attr.va_pgid);
		if (vpgrp) 
			error = VPGRP_SETBATCH(vpgrp);
		else {
			miser_queue_release(mq);
			return EFAULT;
		}

		if (error) {
			miser_queue_release(mq);
			VPGRP_RELE(vpgrp);
			return error;
		}	
        	prs_flagset(&curuthread->ut_pproxy->prxy_sched, PRSBATCH);
		VPGRP_RELE(vpgrp);

		miser_sched_preprocess(mq, attr.va_pgid);

		s = kt_lock(curthreadp);
		if (!cpuset_isglobal(curthreadp->k_cpuset) || 
				KT_ISMR(curthreadp)) 
			error = EBUSY;	
		kt_unlock(curthreadp, s);

		if (error) {
			miser_send_request_error(mq);
			miser_queue_release(mq);
			return error;
		}

		mq->mq_vpagg = vpag_miser_create(SEG->ms_resources.mr_memory);
		if (!mq->mq_vpagg) {
			miser_send_request_error(mq);
			miser_queue_release(mq);
			return ENOMEM;
		}

	}
	
	s = mutex_spinlock(&mq->mq_lock);

	miser_drop_request(mq);
	while (mq->mq_holds > 1) {
		if (sv_wait_sig(&mq->mq_sv, PZERO + 1, &mq->mq_lock, s) == -1) {
			int i = 1;
			s = mutex_spinlock(&miser_lock);
			nested_spinlock(&mq->mq_lock);
			i = --mq->mq_holds;
			nested_spinunlock(&mq->mq_lock);
			mutex_spinunlock(&miser_lock, s);
			if (!i) miser_send_request_error(mq);
			return EINTR;
		}	
		s = mutex_spinlock(&mq->mq_lock);
	}
	error = mq->mq_error;
	if (!(error = mq->mq_error)) {
		if (copyout(&mq->mq_request.mr_req_buffer,
			(caddr_t)(__psint_t)arg,
			sizeof(mq->mq_request.mr_req_buffer))) {
			error = EFAULT;
			/* 
			 * oh well, failed to copy the buffer,
			 * no reason not to go on... 
			 */
		}
	} else {
		mutex_spinunlock(&mq->mq_lock, s);
		miser_send_request_error(mq);
		miser_queue_release(mq);	
		/*
		 * message failed to be delivered, up to the
		 * daemon to figure out what to do. It already
		 * knows the message was failed. 
		 */
		return error;
	}

	if (mq->mq_request.mr_req_buffer.md_request_type 
						== MISER_USER_JOB_SUBMIT
		&& mq->mq_request.mr_req_buffer.md_error == MISER_ERR_OK)
	{ 
		ASSERT(issplhi(getsr()));
		kt_nested_lock(curthreadp);	

		batch_create();
		ASSERT(issplhi(getsr()));

		mq->mq_job = curuthread->ut_job;
        	curuthread->ut_job->b_bid = mq->mq_request.mr_bid;
		miser_adjust_time(mq);

		nested_spinlock(&miser_batch_lock);
		q_insert_before(&miser_batch, &mq->mq_queue);
		nested_spinunlock(&miser_batch_lock);

		job_nested_lock(mq->mq_job);
		mq->mq_job->b_vpagg = mq->mq_vpagg;
		kt_nested_unlock(curthreadp);
		batch_set_params(mq->mq_job, SEG);
		batch_push_queue(mq->mq_job);
		job_nested_unlock(mq->mq_job);
		mutex_spinunlock(&mq->mq_lock,s);


		user_resched(RESCHED_Y);
		return error;

	} else if (mq->mq_request.mr_req_buffer.md_request_type ==
						MISER_USER_JOB_KILL
		&& mq->mq_request.mr_req_buffer.md_error == MISER_ERR_OK) {
		bid_t *b = (bid_t*) mq->mq_request.mr_req_buffer.md_data;
		int s = mutex_spinlock(&miser_batch_lock);	
		miser_queue_t *mq2 = miser_find_bid(*b);
		if (mq2)
			mq2->mq_alive = 0;
		mutex_spinunlock(&miser_batch_lock, s);
	}	

	splx(s);
	miser_send_request_error(mq);
	miser_queue_release(mq);
	
	return error;
}

void
miser_send_request(pid_t bid, int type, caddr_t buffer, int length)
{
	miser_queue_t *mq;

	mq = miser_queue_get(MISER_NORESPONSE);
	mq->mq_request.mr_req_buffer.md_request_type = type;
	mq->mq_request.mr_bid = bid;
	bcopy(buffer, mq->mq_request.mr_req_buffer.md_data, length);
	miser_drop_request(mq);
}

int
miser_get_quantum(sysarg_t arg)
{
	MISER_DEBUG();
	if (copyout((caddr_t)&time_quantum,
	    (caddr_t)(__psint_t)arg,
	    sizeof(time_quantum)))
		return EFAULT;
	else
		return 0;
}

int 
miser_get_bids(sysarg_t arg) 
{
	miser_request_t mgr;
	miser_queue_t *mq;
	miser_bids_t *mb;
	int start = 1;
	int s = 0;
	MISER_DEBUG();

	if (miser_pid != current_pid())
		return EPERM;

	if (copyin((caddr_t)(__psint_t)arg,
		&mgr, sizeof(mgr)))
		return EFAULT;
	
	mb = (miser_bids_t*) &mgr.mr_req_buffer.md_data; 
	if (mb->mb_first)
		start = 0;
	s = mutex_spinlock(&miser_batch_lock); 
	for (mq = FIRST_BATCH_QUEUE; !BATCH(mq); mq = NEXT(mq)) {
		if (start && mq->mq_alive) {
			mb->mb_buffer[mb->mb_count] = MISER->mj_bid;
			mb->mb_count++;
		} else if (mb->mb_first == MISER->mj_bid)
			start = 1;
	}	
	mutex_spinunlock(&miser_batch_lock, s); 
	if (!start) 
		return EINVAL;
	if (copyout(&mgr, (caddr_t)(__psint_t)arg, sizeof(mgr)))
		return EFAULT;
	return 0;
}

int 
miser_get_job(sysarg_t arg)
{
	miser_request_t mgr;
	miser_queue_t *mq;
	miser_job_t *mj;	
	int s;
	int i = 0; 

	if (miser_pid != current_pid())
		return EPERM;
	
	if (copyin((caddr_t)(__psint_t)arg,
		&mgr, sizeof(mgr)))
		return EFAULT;
	
	mj = (miser_job_t*) &mgr.mr_req_buffer.md_data;
	s = mutex_spinlock(&miser_batch_lock);
	for (mq = FIRST_BATCH_QUEUE; !BATCH(mq); mq = NEXT(mq)) {
		if (mj->mj_bid == MISER->mj_bid) {
			nested_spinlock(&mq->mq_lock);
			bcopy(MISER, mj, sizeof(miser_job_t));
			nested_spinunlock(&mq->mq_lock);
			for (i = 0; i < MISER->mj_count; i++)
				mj->mj_segments[i].ms_etime =
					(MISER->mj_segments[i].ms_etime 
						- mq->mq_lbolt) / HZ;
			if (copyout(&mgr, (caddr_t)(__psint_t)arg,
				sizeof(mgr))) {
				mutex_spinunlock(&miser_batch_lock, s);
				return EFAULT;
			}
			mutex_spinunlock(&miser_batch_lock, s);
			return 0;
		}
	}
	mutex_spinunlock(&miser_batch_lock, s);
	return ESRCH;
}	

	
		
int
miser_set_quantum(sysarg_t arg)
{
	quanta_t quantum;
	int error;
	int s;
	extern int batchd_pri;

	MISER_DEBUG();
	if (!_CAP_CRABLE(get_current_cred(), CAP_SCHED_MGT))
		return EPERM;
	if (miser_pid != -1)
		return EBUSY;
	error = add_exit_callback(current_pid(), 0, release_miser, 0);
	if (error)
		return error;
	s = mutex_spinlock(&miser_lock);
	if (miser_pid == -1)
		miser_pid = current_pid();
	mutex_spinunlock(&miser_lock, s);

	if (!miser_thread) {
		miser_thread = 1;
		sthread_create("batchd", NULL, 4096, 0, batchd_pri, KT_PS,
				(st_func_t *)batchd, 0, 0, 0, 0);
	}
	/* start sched tick sthread right here */
	if (miser_pid != current_pid())
		return EBUSY;
	if (copyin((void *)(__psint_t)arg, (caddr_t)&quantum, sizeof(quantum)))
		return EFAULT;
	if (quantum <= 0 || time_quantum != 0 && quantum != time_quantum)
		return EINVAL;
	time_quantum = quantum;
	return 0;
}

int   
miser_set_resources(sysarg_t resource, sysarg_t arg) 
{
	int s, isolated = 0, i;
	miser_resources_t mr;

	if (!_CAP_CRABLE(get_current_cred(), CAP_SCHED_MGT))
		return EPERM;

	if (copyin((void*)(__psint_t) arg, (caddr_t)&mr, 
		sizeof(mr)))
		return EFAULT;

	if (resource == MPTS_MISER_CPU) {
			
		s = mutex_spinlock(&batch_isolatedlock);	

		for (i = 0; i < maxcpus; i++) 
			if (!cpu_enabled(i) || cpu_restricted(i))
				isolated++;

		if (mr.mr_cpus + isolated > maxcpus) {
			mutex_spinunlock(&batch_isolatedlock, s);
			return EINVAL; 
		}
		batch_init_cpus(mr.mr_cpus);
		mutex_spinunlock(&batch_isolatedlock, s);
	} else if (resource == MPTS_MISER_MEMORY) {
		if (miser_pool)
			return vm_pool_resize(miser_pool, btoc(mr.mr_memory));
		miser_pool = vm_pool_create(btoc(mr.mr_memory)); 
		if (!miser_pool) 
			return ENOMEM;
	}
	return 0;
}

int
miser_check_access(sysarg_t file, sysarg_t atype)
{
	vfile_t *fp;
	int error;
	int mode;
	miser_queue_t *mq;
	int s;

	MISER_DEBUG();
	if (q_empty(&miser_queue))
		return EINVAL;
	if (error = getf(file, &fp))
		return error;
	if (!VF_IS_VNODE(fp))
		return EACCES;
	mode = ((atype & (R_OK|W_OK|X_OK)) << 6);
	s = mutex_spinlock(&miser_lock);
	mq = FIRST_MISER_QUEUE;
	nested_spinlock(&mq->mq_lock);
	if (mq->mq_holds == 1) {

		nested_spinunlock(&mq->mq_lock);
		mutex_spinunlock(&miser_lock, s);
		return ESRCH;
	}

	nested_spinunlock(&mq->mq_lock);
	mutex_spinunlock(&miser_lock, s);
	VOP_ACCESS(VF_TO_VNODE(fp), mode, mq->mq_cred, error);
	return error;
}

/*
 * 	Any miser request that requires credentials needs to acquire
 *  them prior to getting to drop request, since drop request is not 
 *  a place you can go to sleep in.
 */
static void
miser_drop_request(miser_queue_t *mq)
{
	int s = mutex_spinlock(&miser_lock);
	if (q_empty(&miser_queue))
		sv_signal(&miser_sv);
	q_insert_before(&miser_queue, &mq->mq_queue);
	mutex_spinunlock(&miser_lock, s);
}

/*ARGSUSED*/
static void
release_miser(void *arg)
{
	time_quantum = 0;
	miser_pid = -1;
	batch_cpus = 0;
}

static void
miser_return_error(int error)
{

	int s = mutex_spinlock(&miser_lock);
	miser_queue_t *mq = FIRST_MISER_QUEUE;

	rmq(&mq->mq_queue);
	mutex_spinunlock(&miser_lock,s);

	mq->mq_error = error;
	(void)miser_return(mq);
}

static int
miser_return(miser_queue_t *mq)
{
	int s = mutex_spinlock(&mq->mq_lock);
	
	mq->mq_holds--;
	if (mq->mq_holds == 1) {
		miser_sched_postprocess(mq);
		sv_signal(&mq->mq_sv);
		mq->mq_error = 0;
		mutex_spinunlock(&mq->mq_lock, s);
		return 0;

	} else {
		miser_queue_release(mq);
		splx(s);
		return 0; 
	}
}

static miser_queue_t *
miser_queue_get(enum miser_enum type)
{
	miser_queue_t *mq = kmem_zalloc(sizeof(*mq), KM_SLEEP);
	spinlock_init(&mq->mq_lock, "miserq");
	sv_init(&mq->mq_sv, SV_DEFAULT, "miserq");
	init_q_element(&mq->mq_queue, mq);
	mq->mq_alive = 1;
	mq->mq_response = type;
	if (type == MISER_NEEDRESPONSE) {
		mq->mq_holds = 2;
		mq->mq_error = 0;
	} else {
		mq->mq_holds = 1;
	}
	return mq;
}

static void
miser_queue_release(miser_queue_t *mq)
{
	spinlock_destroy(&mq->mq_lock);
	sv_destroy(&mq->mq_sv);
	kmem_free(mq, sizeof(*mq));
}

static void 
miser_sched_preprocess(miser_queue_t *mq, pid_t pgid)
{
	mq->mq_request.mr_bid = pgid;
	PID_BATCH_JOIN(pgid);
}

static void
miser_sched_postprocess(miser_queue_t *mq)
{
	if (mq->mq_request.mr_req_buffer.md_request_type 
			== MISER_USER_JOB_SUBMIT)
		mq->mq_lbolt = lbolt;
}

static void
miser_to_weightless(miser_queue_t *mq)
{
	vpgrp_t *vpgrp = VPGRP_LOOKUP(mq->mq_request.mr_bid);
	if (vpgrp) {
		VPGRP_CLEARBATCH(vpgrp);
		VPGRP_RELE(vpgrp);
	}
	batch_to_weightless(mq->mq_job);
}

static void
miser_handle_exception(miser_queue_t *mq)
{
	ASSERT(SEG);
	if (SEG->ms_flags & MISER_EXCEPTION_WEIGHTLESS)
		miser_to_weightless(mq);
}


static void 
miser_time_lock_expire(miser_queue_t *mq)
{

	job_t *j = mq->mq_job;
	ASSERT(spinlock_islocked(&miser_batch_lock));
	if (--MISER->mj_count <= 0 && j) {
		job_nested_lock(j);

		if (MISER->mj_count >= 0)   {
			batch_release_resources(j);
			miser_send_request(MISER->mj_bid,
                                MISER_KERN_TIME_EXPIRE, (caddr_t)MISER,
                                        sizeof(*MISER));
		}	

		if (j->j_nthreads > 0)
			sv_broadcast(&j->j_sv);

		job_nested_unlock(j);
	} else {
		ASSERT(issplhi(getsr()));
		ASSERT(MISER->mj_count > 0);
		bcopy(SEG + 1, SEG, MISER->mj_count * sizeof(*SEG));
		job_nested_lock(j);
		batch_set_params(j, SEG);
		job_nested_unlock(j);
	}

}

void
miser_time_expire(job_t *j)
{
	miser_queue_t *mq;
	int s;

	s = mutex_spinlock(&miser_batch_lock);
	mq = miser_find_bid(j->b_bid);
	if (!mq) {
		mutex_spinunlock(&miser_batch_lock, s);
		return;
	}
	miser_time_lock_expire(mq);
	mutex_spinunlock(&miser_batch_lock, s);
}

void
miser_exit(job_t *j)
{
	
	miser_queue_t *mq;
	nested_spinlock(&miser_batch_lock);
	mq = miser_find_bid(j->b_bid);
	nested_spinlock(&mq->mq_lock);
	mq->mq_holds--;
	mq->mq_job = 0;
	nested_spinunlock(&mq->mq_lock);
	nested_spinunlock(&miser_batch_lock);
	batch_release_resources(j);
	miser_send_request(j->b_bid, MISER_KERN_EXIT, (caddr_t) &(j->b_bid), 
				sizeof (j->b_bid));
}

void 
miser_inform(int16_t flags)
{
	int s = mutex_spinlock(&miser_batch_lock);
	job_t *j = curuthread->ut_job;
	miser_queue_t *mq = miser_find_bid(j->b_bid);	

	ASSERT(mq);
	mutex_spinunlock(&miser_batch_lock, s);
	miser_send_request(MISER->mj_bid, flags, (caddr_t) MISER,
				sizeof(*MISER));
}

void 
idbg_mqb_disp(struct q_element_s *qe)
{
	struct miser_batch	*mb = (struct miser_batch*)qe;
	qprintf("	job 0x%x\n", ((miser_queue_t *)mb)->mq_job);
}

void 
idbg_vpagg(__psint_t x)
{
	vpagg_t *vpagg;

	if (x == -1L)
		return;

	vpagg = (vpagg_t *) x;

	qprintf("Vpagg 0x%x \n", vpagg);
	qprintf("paggid %d, refcnt %d \n", vpagg->vpag_paggid,
					 vpagg->vpag_refcnt);

	return;
}

void 
idbg_mq(__psint_t x)
{
        miser_queue_t* mq;
	miser_data_t* rq; 
        miser_seg_t* seg;

        if (x == -1L)
                return;

        mq = (miser_queue_t*) x;
	rq = &mq->mq_request.mr_req_buffer;

	qprintf("mq associated to job 0x%x holds %d \n", mq->mq_job, 
					mq->mq_holds);

	if (mq->mq_response == MISER_NEEDRESPONSE) 
		qprintf("Need response\n");
	else
		qprintf("Need no response\n");

	if (mq->mq_vpagg)
		idbg_vpagg((__psint_t) mq->mq_vpagg);

	if (rq->md_request_type == MISER_USER_JOB_SUBMIT) {
        	qprintf("Miser queue \n");
		qprintf("ID %d, bid %d, end %d nsegs %d\n start %d \n", 
			MISER->mj_queue, MISER->mj_bid, MISER->mj_etime, 
				MISER->mj_count, mq->mq_lbolt);

		for (seg = SEG; seg != SEG + MISER->mj_count; seg++) 
			 qprintf ("seg %x, time %d, end_time %d\n", seg, 
				seg->ms_rtime, seg->ms_etime);
	} else 
		qprintf("Miser command %d \n", rq->md_request_type);
}

void 
idbg_cpuset(__psint_t x)
{
	int start = 0; 
	int end = cpuset_size;
	int i, j;

	if (x != -1L) {
		start = (int) x;
		end = (int) x + 1;
	}

	for ( i = 0; i < maxcpus; i++) 	 
		qprintf("cpu_to_cpuset [%d] = %d \n", i, cpu_to_cpuset[i]);
	qprintf("\n");

	for ( j = start; j < end; j++)  {
		if (j > 1 && CPUMASK_IS_ZERO(cpusets[j].cs_cpus))
			continue;
		qprintf("CPUSET %d queue [%s] idler %d count %d lbpri %d master %d\n", 
					j, 
					&(cpusets[j].cs_name),
					cpusets[j].cs_idler,
					cpusets[j].cs_count,
					cpusets[j].cs_lbpri,
					cpusets[j].cs_mastercpu);
		qprintf("  CPUS:");
		for (i = 0; i < maxcpus; i++)  {
			if (CPUMASK_TSTB(cpusets[j].cs_cpus, i))
				qprintf(" %d", i);
		}
		qprintf("\n");		
		qprintf("  NODE CPUMASK:");
		for (i = 0; i < maxcpus; i++)  {
			if (CPUMASK_TSTB(cpusets[j].cs_node_cpumask, i))
				qprintf(" %d", i);
		}
		qprintf("\n");		
		qprintf("  NODEMASK:");
		if (j == 1 || cpusets[j].cs_nodemaskptr != &global_cpuset.cs_nodemask) {
			for (i = 0; i < maxnodes; i++)
				if (CNODEMASK_TSTB(cpusets[j].cs_nodemask, i))
					qprintf(" %d", i);
		} else {
			qprintf(" (uses global cpuset mask)"); 
		}
		qprintf("\n");		
	}
}

cpuset_t*
find_cpuset(id_type_t id, int* s) 
{
	int i;

	for (i = 2; i < cpuset_size; i++) {
		*s = mutex_spinlock(&cpusets[i].cs_lock);
		if (cpusets[i].cs_name == id)
			return &cpusets[i];
		mutex_spinunlock(&cpusets[i].cs_lock, *s);
	}
	return 0;
}
	
void 
set_cputocpuset(cpumask_t *cmask, int cid)
{	
	int i = 0;

	for (i = 0; i < maxcpus; i++)
		if (CPUMASK_TSTB(*cmask, i))
			cpu_to_cpuset[i] = cid;
}	

int
disjoint_set(cpumask_t in, cpumask_t of)
{
	CPUMASK_ANDM(in, of);
	return CPUMASK_IS_ZERO(in);
}

/* cpumask_to_nodemask
 * Create a node mask that represent the nodes of each cpu in the
 * a cpumask.
 *
 * NOTE: <nodemask> may point to a mask that is being used on other
 * cpus & is NOT interlocked. May sure this mask is never set to an 
 * invalid state. It is ok to set/clear bits over a period of time
 * as long as the value of a bit is valid for either the old or
 * new state. Specifically, DONT clear all bits, then reset the ones
 * that should stay set. If a bit should be set in both the old & new
 * state, it would be in an invalid "0" state for a short period of
 * time.
 */
void
cpumask_to_nodemask(cnodemask_t *nodemask, cpumask_t cpumask)
{
	int		cpu;
	cnodemask_t	tmpmask;

	CNODEMASK_CLRALL(tmpmask);
	for (cpu =0; cpu < maxcpus; cpu++)
		if (CPUMASK_TSTB(cpumask, cpu))
			CNODEMASK_SETB(tmpmask, pdaindr[cpu].pda->p_nodeid);
	*nodemask = tmpmask;
}

int 
restrict_cpuset(cpumask_t cmask)
{
	pda_t *npda;
	cpu_cookie_t was_running;	
	int i, s;
	for (i = 0; i < maxcpus; i++) {
		if (!CPUMASK_TSTB(cmask, i))
			continue;
		npda = pdaindr[i].pda;
		s = mutex_spinlock_spl(&npda->p_special, spl7);
		npda->p_flags &= ~PDAF_ENABLED;
		mutex_spinunlock(&npda->p_special, s);
		s = splhi();
		was_running = setmustrun(i);
		cpu_restrict(i, cpu_to_cpuset[cpuid()]);
		batch_isolate(i);
		migrate_timeouts(i, clock_processor);
		restoremustrun(was_running);
		splx(s);
	} 
	return 0;
}	

int 
unrestrict_cpuset(cpumask_t cmask)
{
	int i = 0;
	int s;
	pda_t *npda; 
	for (i = 0; i < maxcpus; i++) {
		if (!CPUMASK_TSTB(cmask, i))
			continue;
		npda = pdaindr[i].pda;
		s = mutex_spinlock_spl(&npda->p_special, spl7);
		npda->p_flags |= PDAF_ENABLED;
		cpu_unrestrict(i);
		batch_unisolate(i);
		mutex_spinunlock(&npda->p_special, s);
	}
	return 1;
}
cpuset_t *
check_cpuset(id_type_t id)
{	
	int i;

	for (i = 2; i < cpuset_size; i++) {
		if (cpusets[i].cs_name == id)
			return &cpusets[i];
	}
	return 0;
}
	
int 
miser_cpuset_create(miser_queue_cpuset_t* request, sysarg_t file)
{
	int s, error = EBUSY, cid;
	int i = 0;
	cpumask_t cmask;
	cpuset_t *cs, *tcs;
	vfile_t *fp = 0;
	vnode_t *vnode;
	cpuid_t mastercpu = CPU_NONE;
#ifndef LARGE_CPU_COUNT
	cmask = 0;
#endif


	if (!_CAP_CRABLE(get_current_cred(), CAP_SCHED_MGT))
		return EPERM;

	CPUMASK_CLRALL(cmask);
	if (request->mqc_count >= maxcpus)
		return EINVAL;
	
		
	for (; i < request->mqc_count; i++) {
		if (request->mqc_cpuid[i] < 0 || 
			request->mqc_cpuid[i] > maxcpus || 
			!cpu_enabled(request->mqc_cpuid[i]) ||
			(request->mqc_cpuid[i] == master_procid &&
			(request->mqc_flags & MISER_CPUSET_CPU_EXCLUSIVE)))
			return ENXIO;
		if (mastercpu == CPU_NONE)
			mastercpu = request->mqc_cpuid[i];
		CPUMASK_SETB(cmask, request->mqc_cpuid[i]);
	}	
	if (!(request->mqc_flags & MISER_CPUSET_KERN) 
		&& (request->mqc_queue <= maxcpus))
		return EINVAL;

	/* check if that cpu set already exists? */
	if (!(request->mqc_flags & MISER_CPUSET_KERN) && 
		(cs = find_cpuset(request->mqc_queue, &s))) {
		mutex_spinunlock(&cs->cs_lock, s);
		return EBUSY;
	}
	if (!(request->mqc_flags & MISER_CPUSET_KERN)) {
		if (getf(file, &fp))
			return EINVAL;
		
		if (!VF_IS_VNODE(fp))
			return EACCES;
		vnode = VF_TO_VNODE(fp);
		VN_HOLD(vnode);
	}  else
		vnode = 0;
		
	
	/* check to see if you can create cpu set */
	if (cs = find_cpuset(0, &s)) {
		nested_spinlock(&global_cpuset.cs_lock);
		nested_spinlock(&cpusets[0].cs_lock);
		if (tcs = check_cpuset(request->mqc_queue)) {
			nested_spinunlock(&global_cpuset.cs_lock);
			nested_spinunlock(&cpusets[0].cs_lock);

			/* 
			 * There is race condition existing between
			 * creating a cpuset via isolate and
			 * destroying it. This race is prevented
			 * by the isolate semaphore.
			 */
			if ((tcs->cs_flags & MISER_CPUSET_KERN) &&
				(request->mqc_flags & MISER_CPUSET_KERN))
				error = 0;
			mutex_spinunlock(&cs->cs_lock, s);
			return error;
		}	
		if (disjoint_set(cpusets[0].cs_cpus, cmask)){ 
			if (request->mqc_flags&(MISER_CPUSET_CPU_EXCLUSIVE|MISER_CPUSET_KERN)) { 
				nested_spinlock(&batch_isolatedlock);	
				if ((request->mqc_count + 
				     batch_isolated + batch_cpus > maxcpus) ||
				     !disjoint_set(global_cpuset.cs_cpus, cmask)) {
					nested_spinunlock(&batch_isolatedlock);
					nested_spinunlock(&cpusets[0].cs_lock);
					nested_spinunlock(&global_cpuset.cs_lock);
					mutex_spinunlock(&cs->cs_lock, s);
					if (vnode)
						VN_RELE(vnode);
					return EBUSY;
				}
				CPUMASK_SETM(global_cpuset.cs_cpus, cmask);
				batch_isolated += request->mqc_count;
				nested_spinunlock(&batch_isolatedlock);	
			}
			CPUMASK_SETM(cs->cs_cpus, cmask);
			CPUMASK_SETM(cpusets[0].cs_cpus, cmask);

			/*
			 * Now set up the nodemasks that specify where memory is allocated
			 * for threads using the cpuset & for the GLOBAL_CPUSET.
			 *	MISER_CPUSET_MEMORY_LOCAL
			 *		specified - use node that contain the cpus in the cpuset
			 *		else      - use whatever the GLOBAL_CPUSET can use
			 *	MISER_CPUSET_MEMORY_EXCLUSIVE
			 *		specified - delete cpus from GLOBAL_CPUSET cpus
			 *			    and recalculate the nodemask
			 */
			if (request->mqc_flags&MISER_CPUSET_MEMORY_LOCAL) {
				CPUMASK_SETM(cs->cs_node_cpumask, cmask);
				cpumask_to_nodemask(&cs->cs_nodemask, cmask);
				cs->cs_nodemaskptr = &cs->cs_nodemask;
			} else {
				cs->cs_nodemaskptr = &global_cpuset.cs_nodemask;
			}
			if (request->mqc_flags&MISER_CPUSET_MEMORY_EXCLUSIVE) {
				CPUMASK_CLRM(global_cpuset.cs_node_cpumask, cmask);
				cpumask_to_nodemask(&global_cpuset.cs_nodemask, global_cpuset.cs_node_cpumask);
			}

			ASSERT(cs->cs_idler == -2);
			cs->cs_name = request->mqc_queue;
			cs->cs_flags = request->mqc_flags;
			if (request->mqc_flags & MISER_CPUSET_KERN) 
				cid  = 1;

			else  {
				cid = cs - &cpusets[0];
			}
			cs->cs_mastercpu = mastercpu;
			cs->cs_idler = CPU_NONE;	
			cs->cs_file = vnode;
			set_cputocpuset(&cs->cs_cpus, cid);
			error = 0;
		}
		nested_spinunlock(&global_cpuset.cs_lock);
		nested_spinunlock(&cpusets[0].cs_lock);
		mutex_spinunlock(&cs->cs_lock, s);
	}

	if (!error && (request->mqc_flags & MISER_CPUSET_CPU_EXCLUSIVE))  
		restrict_cpuset(cmask);
	if (error && fp)  
		VN_RELE(VF_TO_VNODE(fp));
	return error;	
}

/*
 * detach a process from this cpuset
 */
static int miser_cpuset_detach_proc(proc_t *p, void *arg, int mode)
{
	int s;
	cpuset_t *cs = (cpuset_t *) arg;
	uthread_t *ut;
	kthread_t *kt;

	switch (mode) {
	case 0:
		break;
	case 1:
		/* find out if the proc is in the cpuset */
		uscan_hold(&p->p_proxy);
		uscan_forloop(&p->p_proxy, ut) {
			kt = UT_TO_KT(ut);
			s = kt_lock(kt);	
			if(cs == &cpusets[kt->k_cpuset]) {
				ASSERT(kt->k_cpuset>1);
				kt->k_cpuset 	= 1;
				kt->k_lastrun	= CPU_NONE;
				kt->k_flags	&= ~KT_HOLD;
				kt->k_flags	|= KT_NOAFF; 
				nested_spinlock(&cs->cs_lock);
				cs->cs_count--;
				nested_spinunlock(&cs->cs_lock);
			}
			kt_unlock(kt, s);
		}
		uscan_rele(&p->p_proxy);
		break;
	}
	return 0;

}

/*
 * find all the processes in the cpuset, and move they out of it.
 */
static int miser_cpuset_move_procs(miser_queue_cpuset_t *request)
{
	int error = ESRCH, s;
	cpuset_t *cs;

	if (!_CAP_CRABLE(get_current_cred(), CAP_SCHED_MGT))
		return EPERM;

	if (cs = find_cpuset(request->mqc_queue, &s)) {
		cs->cs_count++;
		mutex_spinunlock(&cs->cs_lock, s);
	} else  {
		return error;
	}
	/* now cs won't go away. Try to move all procs in the set. */ 
	procscan(miser_cpuset_detach_proc, cs);
	s = mutex_spinlock(&cs->cs_lock);
	cs->cs_count--;
	mutex_spinunlock(&cs->cs_lock, s);
	return 0;
}


int      
miser_cpuset_destroy(miser_queue_cpuset_t *request)
{
	int error = ESRCH, s;
	cpuset_t *cs;
	cpumask_t cmask, tmask;
	vnode_t *vnode = 0;

	if (!_CAP_CRABLE(get_current_cred(), CAP_SCHED_MGT))
		return EPERM;

	CPUMASK_CLRALL(tmask);
	CPUMASK_CLRALL(cmask);

	if (!(request->mqc_flags & MISER_CPUSET_KERN) &&  
		(request->mqc_queue <= maxcpus) && (request->mqc_queue < 1) ||
		(request->mqc_queue == 0))
		return EINVAL;

again:
	if (cs = find_cpuset(request->mqc_queue, &s)) {
		if (cs->cs_count == 0 || 
			(request->mqc_flags & MISER_CPUSET_KERN)) {
			if(!compare_and_swap_int(&cs->cs_idler, CPU_NONE, -2)){
				mutex_spinunlock(&cs->cs_lock, s);
				goto again;
			}
			nested_spinlock(&global_cpuset.cs_lock);
			if (!disjoint_set(global_cpuset.cs_cpus, cs->cs_cpus))
				CPUMASK_SETM(cmask, cs->cs_cpus);
			CPUMASK_CLRM(global_cpuset.cs_cpus, cs->cs_cpus);
			CPUMASK_SETM(global_cpuset.cs_node_cpumask, cs->cs_cpus);
			cpumask_to_nodemask(&global_cpuset.cs_nodemask, global_cpuset.cs_node_cpumask);
			CPUMASK_CLRM(cpusets[0].cs_cpus, cs->cs_cpus);
			CPUMASK_SETM(tmask, cs->cs_cpus);
			CPUMASK_CLRALL(cs->cs_cpus);
			CPUMASK_CLRALL(cs->cs_node_cpumask);
			CNODEMASK_CLRALL(cs->cs_nodemask);
			nested_spinunlock(&global_cpuset.cs_lock);
			cs->cs_name = 0;
			vnode = cs->cs_file;
			cs->cs_file = 0;
			set_cputocpuset(&tmask, 1);	
			error = 0;
		} else 
			error = EBUSY;
		mutex_spinunlock(&cs->cs_lock, s);
		if (CPUMASK_IS_NONZERO(cmask))
			unrestrict_cpuset(cmask);
		if (vnode)  
			VN_RELE(vnode);

	} else if (request->mqc_flags & MISER_CPUSET_KERN) { 
		if (CPUMASK_TSTB(global_cpuset.cs_cpus, request->mqc_queue))
			error = EPERM;
		else
			error = 0;
	} 
	return error;
}

int 
find_cpu_in_set(cpumask_t cs)
{
	int i = 0;
	for(; i < maxcpus; i++) 
		if (CPUMASK_TSTB(cs, i))
			return i;

	return -1;
}

int 
miser_cpuset_attach(miser_queue_cpuset_t* request)
{
	/* attach thread to the cpu set */
	int s, error = ESRCH;
	int index = 0; 
	cpuset_t *cs, *old_cs;
	
	int mode;	
        vpgrp_t *vpgrp;
	vproc_t *vpr;	
        int is_batch;
	vp_get_attr_t attr;


	vpr = VPROC_LOOKUP(current_pid());
	if (vpr == 0) 
		return ESRCH;
        VPROC_GET_ATTR(vpr, VGATTR_PGID, &attr);
        VPROC_RELE(vpr);
	vpgrp = VPGRP_LOOKUP(attr.va_pgid);
        if (vpgrp) {
                VPGRP_HOLD(vpgrp);
                VPGRP_GETATTR(vpgrp, NULL, NULL, &is_batch);
                if (is_batch) {
			VPROC_RELE(vpr);
                        return EPERM;
                }
                VPGRP_RELE(vpgrp);
        } else 
		return ESRCH;
		
	if (cs = find_cpuset(request->mqc_queue, &s)) {
		cs->cs_count++;
		mutex_spinunlock(&cs->cs_lock, s);
	} else 
		return error;

	mode = X_OK << 6;
	VOP_ACCESS(cs->cs_file, mode, get_current_cred(),
				 error);
	if (error) {
		s = mutex_spinlock(&cs->cs_lock);
		cs->cs_count--;
		mutex_spinunlock(&cs->cs_lock, s);
		return error;
        }
	s = kt_lock(curthreadp);

	/* Not sure what else should go here */
	if (KT_ISMR(curthreadp)) {
		nested_spinlock(&cs->cs_lock);
		cs->cs_count--;
		nested_spinunlock(&cs->cs_lock);
		kt_unlock(curthreadp, s);
		return EPERM;
	}
	index = cs->cs_idler;
	if (index == CPU_NONE) 
		index = find_cpu_in_set(cs->cs_cpus);			
	if (index == CPU_NONE) {
		kt_unlock(curthreadp, s);
		return EPERM;	
	}
	if(curthreadp->k_cpuset >1) { 
		/* we are going out of this cpuset to a new one.*/
		old_cs = &cpusets[curthreadp->k_cpuset];
		nested_spinlock(&old_cs->cs_lock);
		old_cs->cs_count--;
		nested_spinunlock(&old_cs->cs_lock);
	}
	curthreadp->k_cpuset = cs - &cpusets[0];
	ASSERT(index >= 0 && index < maxcpus);
	if (curthreadp->k_lastrun != CPU_NONE 
		&& CPUMASK_TSTB(cs->cs_cpus, curthreadp->k_lastrun))
		index = curthreadp->k_lastrun;
	curthreadp->k_lastrun = index;
	curthreadp->k_flags &= ~KT_NOAFF;
	curthreadp->k_flags |= KT_HOLD;
	ASSERT(CPUMASK_TSTB(cs->cs_cpus, index));
	ASSERT(issplhi(getsr()));
	ASSERT(KT_ISUTHREAD(curthreadp));
	ut_endrun(KT_TO_UT(curthreadp));
	putrunq(curthreadp, index);
	kt_nested_unlock(curthreadp);
	swtch(MUSTRUNCPU);
	splx(s);
	ASSERT(!issplhi(getsr()));

	return error;
}

int
miser_cpuset_query_cpus(miser_queue_cpuset_t* request, sysarg_t arg)
{
	int s, error = ESRCH, mode = 0;
	int index = 0, i;
	miser_request_t mr;
	miser_queue_cpuset_t *qr = 
			(miser_queue_cpuset_t*) mr.mr_req_buffer.md_data;
	cpuset_t *cs;
	
	if (cs = find_cpuset(request->mqc_queue, &s)) {
		cs->cs_count++;
		mutex_spinunlock(&cs->cs_lock, s);
	} else
		return error;

	error = 0;
	for (i = 0; i < maxcpus; i++)  {
		if (CPUMASK_TSTB(cs->cs_cpus, i)) {
			qr->mqc_cpuid[index] = i;
			index++;
		}
	}

	mode = R_OK << 6;
	VOP_ACCESS(cs->cs_file, mode, get_current_cred(),
				 error);
	if (error)  {
		s = mutex_spinlock(&cs->cs_lock);
		cs->cs_count--;
		mutex_spinunlock(&cs->cs_lock, s);
		return error;
	}

	qr->mqc_count = index;
	
	if (copyout(&mr.mr_req_buffer,
			(caddr_t)(__psint_t)arg,
			sizeof(mr.mr_req_buffer))) {
			error = EFAULT;
	}

	s = mutex_spinlock(&cs->cs_lock);
	cs->cs_count--;
	mutex_spinunlock(&cs->cs_lock, s);

	return error;
}

struct cs_procs {
	cpuset_t *cs;
	pid_t *pids;
	int   usermax;
	int   count;
};

/*
 * put the pid into buffer, if it is in the cpuset.
 */
static int 
miser_cpuset_list_proc(proc_t *p, void *arg, int mode)
{
	int s, error = 0;
	struct cs_procs *cs_proc = (struct cs_procs *) arg;
	cpuset_t *cs = cs_proc->cs;
	uthread_t *ut;
	kthread_t *kt;

	switch (mode) {
	case 0:
		break;
	case 1:
		/* find out if the proc is in the cpuset */
		uscan_hold(&p->p_proxy);
		if((ut = prxy_to_thread(&p->p_proxy)) == NULL) {
			uscan_rele(&p->p_proxy);
			break;
		}
		kt = UT_TO_KT(ut);
		s = kt_lock(kt);
		if(cs == &cpusets[kt->k_cpuset]) {
			ASSERT(kt->k_cpuset>1);
			if(cs_proc->pids == NULL) {
				/* just count the number of procs */
				cs_proc->count++;
			} else if(cs_proc->count >= cs_proc->usermax) {
				error = ENOMEM;
			} else {
				cs_proc->pids[cs_proc->count] = p->p_pid;
				cs_proc->count++;
			}
		}
		kt_unlock(kt, s);
		uscan_rele(&p->p_proxy);
		break;
	}
	return error;

}

/*
 * list all the processes in the cpuset.
 */
static int
miser_cpuset_list_procs(miser_cpuset_pid_t* request, sysarg_t arg)
{
	pid_t *first, *last;
	int s, error = ESRCH, mode = 0;
	miser_request_t mr;
	miser_cpuset_pid_t *mcp = 
			(miser_cpuset_pid_t *) mr.mr_req_buffer.md_data;
	cpuset_t *cs;
	struct cs_procs cs_proc;
	
	if (cs = find_cpuset(request->mcp_queue, &s)) {
		cs->cs_count++;
		mutex_spinunlock(&cs->cs_lock, s);
	} else
		return error;
	error = 0;
	mode = R_OK << 6;
	VOP_ACCESS(cs->cs_file, mode, get_current_cred(), error);
	if (error)  {
		s = mutex_spinlock(&cs->cs_lock);
		cs->cs_count--;
		mutex_spinunlock(&cs->cs_lock, s);
		return error;
	}
	last = (pid_t *)(&mr+1);
	first = (pid_t *)mcp->mcp_pids;
	last = first + (last - first);
	if((last - mcp->mcp_pids) < mcp->mcp_max_count)
		cs_proc.usermax = last - mcp->mcp_pids;
	else 
		cs_proc.usermax = request->mcp_max_count;
	cs_proc.cs = cs;
	cs_proc.pids= (pid_t *)mcp->mcp_pids; 
	cs_proc.count = 0;
	error = procscan(miser_cpuset_list_proc, &cs_proc);
	s = mutex_spinlock(&cs->cs_lock);
	cs->cs_count--;
	mutex_spinunlock(&cs->cs_lock, s);
	/* copy out */
	mcp->mcp_count = cs_proc.count;
	if (copyout(&mr.mr_req_buffer,
		(caddr_t)(__psint_t)arg,
		sizeof(mr.mr_req_buffer))) {
		error = EFAULT;
	}
	return error;
}
int 
miser_cpuset_query_names(sysarg_t arg)
{
	int error = 0;
	int index = 0, i;
	miser_request_t mr;
	miser_queue_names_t *qr = (miser_queue_names_t*) 
					mr.mr_req_buffer.md_data;

	for (i = 2; i < cpuset_size; i++) {
		if (cpusets[i].cs_name > 0) {
			qr->mqn_queues[index] = cpusets[i].cs_name;	
			index++;
		}
	}
	qr->mqn_count = index;

	if (copyout(&mr.mr_req_buffer,
		(caddr_t)(__psint_t)arg,
		sizeof(mr.mr_req_buffer))) {
		error = EFAULT;
	}
	return error;
}
	

/*
 * check if the thread can run on this cpu.
 * return 1 if:
 *  1. the cpu and the thread are in the same cpu set.
 *  2. OR the thread has permission to run on that set.
 */
int miser_cpuset_check_access(void *arg, int cpu)
{
	kthread_t 	*kt = (kthread_t *)arg;
	int 		s, mode, error = 0;
	int		cpuset = cpu_to_cpuset[cpu];
	cpuset_t 	*cs = &cpusets[cpuset];

	ASSERT(kt);
	/* the same cpuset */
	if(cpuset == kt->k_cpuset)
		return 1;

	/* hold this cpuset */
	s = mutex_spinlock(&cs->cs_lock);
	cs->cs_count++;
	mutex_spinunlock(&cs->cs_lock, s);
	/* check if this proc has permission to access the cpuset */
	if(cs->cs_file != NULL) {
		mode = X_OK << 6;
		VOP_ACCESS(cs->cs_file, mode, get_current_cred(), error);
	}
	s = mutex_spinlock(&cs->cs_lock);
	cs->cs_count--;
	mutex_spinunlock(&cs->cs_lock, s);
	return (error == 0);
}

int 
miser_cpuset_query_current(sysarg_t arg)
{
	int s, error = 0;
	int index = 0;
	miser_request_t mr;
	miser_queue_names_t *qr = (miser_queue_names_t*) 
					mr.mr_req_buffer.md_data;
	int cpuset;

	s = kt_lock(curthreadp);
	cpuset = curthreadp->k_cpuset;
	kt_unlock(curthreadp, s);
	ASSERT(cpuset >= 1 &&  cpuset < cpuset_size);
	if (cpusets[cpuset].cs_name > 0) {
		qr->mqn_queues[0] = cpusets[cpuset].cs_name;	
		index++;
	} else 
		error = ESRCH;

	qr->mqn_count = index;

	if (copyout(&mr.mr_req_buffer,
		(caddr_t)(__psint_t)arg,
		sizeof(mr.mr_req_buffer))) {
		error = EFAULT;
	}
	return error;
}
static int 
miser_cpuset_process(miser_data_t* req, sysarg_t arg, sysarg_t file)
{

	switch(req->md_request_type) {
	case MISER_CPUSET_CREATE:
		return miser_cpuset_create((miser_queue_cpuset_t*)
							req->md_data,
						 file);
	case MISER_CPUSET_DESTROY:
		return miser_cpuset_destroy((miser_queue_cpuset_t*)
							req->md_data);
	case MISER_CPUSET_LIST_PROCS:
		return miser_cpuset_list_procs((miser_cpuset_pid_t*)
						req->md_data, arg);
	case MISER_CPUSET_MOVE_PROCS:
		return miser_cpuset_move_procs((miser_queue_cpuset_t*)
							req->md_data);
	case MISER_CPUSET_ATTACH:
		return miser_cpuset_attach((miser_queue_cpuset_t*)
							req->md_data);
	case MISER_CPUSET_QUERY_CPUS:
		return miser_cpuset_query_cpus((miser_queue_cpuset_t*)
						req->md_data, arg);
	case MISER_CPUSET_QUERY_NAMES:
		return miser_cpuset_query_names(arg);
	case MISER_CPUSET_QUERY_CURRENT:
		return miser_cpuset_query_current(arg);
	} 
	
	return EINVAL;
}
	
cnodemask_t
get_effective_nodemask(kthread_t *kt)
{
	cnodemask_t     mask;
	CNODEMASK_CPY(mask, kt->k_nodemask);
	CNODEMASK_ANDM(mask, *cpusets[kt->k_cpuset].cs_nodemaskptr);
	return(mask);
}
