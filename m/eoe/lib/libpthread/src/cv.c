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

/* NOTES
 *
 * This module implements condition variables.
 */

#include "common.h"
#include "cvattr.h"
#include "delay.h"
#include "event.h"
#include "intr.h"
#include "mtx.h"
#include "pt.h"
#include "pthreadrt.h"
#include "sys.h"
#include "vp.h"
#include "cv.h"

#include <errno.h>
#include <pthread.h>
#include <sys/usync.h>

/*
 * definitions to catch and fix statically-initialized condition variables
 */
#define COND_INITIALIZED(cv)	((cv)->cv_q.next)
#define COND_INIT_PATCH(cv)	Q_INIT(&(cv)->cv_q)

static void cv_wakeup(register pt_t *);
static int cv_signal_pshared(pthread_cond_t *);
static int cv_broadcast_pshared(pthread_cond_t *);
static int cv_wait_pshared(pthread_cond_t *, pthread_mutex_t *);
static int cv_timedwait_pshared(pthread_cond_t *, pthread_mutex_t *,
				const timespec_t *);


int
pthread_cond_init(pthread_cond_t *cv, const pthread_condattr_t *attr)
{
	cv->cv_attr = attr ? *(pthread_condattr_t *)attr : cvattr_default;
	if (!cv->cv_attr.ca_pshared) {
		lock_init(&cv->cv_lock);
		Q_INIT(&cv->cv_q);
	}
	return (0);
}


int
pthread_cond_destroy(pthread_cond_t *cv)
{
	if (!cv->cv_attr.ca_pshared) {
		if (COND_INITIALIZED(cv) && !Q_EMPTY(&cv->cv_q)) {
			return (EBUSY);
		}

		/* Avoid destroying cv if library may be trying to lock it.
		 */
		VP_YIELD(intr_destroy_wait());
	}
	return (0);
}


int
pthread_cond_signal(pthread_cond_t *cv)
{
	pt_t	*wt;

	TRACE(T_CV, ("pthread_cond_signal(%#x)", cv));

	if (cv->cv_attr.ca_pshared)
		return (cv_signal_pshared(cv));

	sched_enter();
	lock_enter(&cv->cv_lock);

	if (!COND_INITIALIZED(cv)) {
		lock_leave(&cv->cv_lock);
		sched_leave();
		return (0);
	}

	if (wt = pt_dequeue(&cv->cv_q)) {
		wt->pt_wait = 0;
		wt->pt_state = PT_DISPATCH;
		lock_leave(&cv->cv_lock);

		sched_dispatch(wt);
	} else {
		lock_leave(&cv->cv_lock);
	}
	sched_leave();

	return (0);
}


static int
cv_signal_pshared(pthread_cond_t *cv)
{
	usync_arg_t	usarg;

	usarg.ua_version = USYNC_VERSION_2;
	usarg.ua_addr = (__uint64_t)cv;
	usarg.ua_flags = USYNC_FLAGS_NOPREPOST;
	while (pt_usync_cntl(USYNC_UNBLOCK, &usarg, FALSE) == -1) {
		if (oserror() != EINTR) {
			panic("cv_signal_pshared", "USYNC_UNBLOCK failed");
		}
	}
	return (0);
}


int
pthread_cond_broadcast(pthread_cond_t *cv)
{
	pt_t	*wt;	/* waiting thread */
	pt_t	*dt;	/* dispatched thread */

	TRACE(T_CV, ("pthread_cond_broadcast(%#x)", cv));

	if (cv->cv_attr.ca_pshared)
		return (cv_broadcast_pshared(cv));

	sched_enter();
	lock_enter(&cv->cv_lock);

	if (!COND_INITIALIZED(cv) || Q_EMPTY(&cv->cv_q)) {
		lock_leave(&cv->cv_lock);
		sched_leave();
		return (0);
	}

	/* Mark pts for dispatch and detach from cv q.
	 */
	for (wt = Q_HEAD(&cv->cv_q, pt_t*);
	     !Q_END(&cv->cv_q, wt); wt = Q_NEXT(wt, pt_t*)) {
		wt->pt_state = PT_DISPATCH;
	}
	wt = Q_HEAD(&cv->cv_q, pt_t*);
	Q_INIT(&cv->cv_q);
	lock_leave(&cv->cv_lock);

	/* Now, dispatch original members (not holding cv q lock).
	 */
	while (!Q_END(&cv->cv_q, wt)) {
		dt = wt;
		wt = Q_NEXT(wt, pt_t*);
		dt->pt_wait = 0;
		sched_dispatch(dt);
	}
	sched_leave();

	return (0);
}


static int
cv_broadcast_pshared(pthread_cond_t *cv)
{
	usync_arg_t	usarg;

	usarg.ua_version = USYNC_VERSION_2;
	usarg.ua_addr = (__uint64_t)cv;
	usarg.ua_flags = USYNC_FLAGS_NOPREPOST;
	while (pt_usync_cntl(USYNC_UNBLOCK_ALL, &usarg, FALSE) == -1) {
		if (oserror() != EINTR) {
			panic("cv_broadcast_pshared",
				"USYNC_UNBLOCK_ALL failed");
		}
	}
	return (0);
}


int
pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *mutex)
{
	register pt_t	*pt_self = PT;
	int		wait_result;

	TRACE(T_CV, ("pthread_cond_wait(%#x)", cv));

	/* Process pending interruptions.
	 */
	PT_INTR_PENDING(pt_self, PT_INTERRUPTS);

	if (cv->cv_attr.ca_pshared) {
		lock_leave(&pt_self->pt_lock);
		sched_leave();
		return (cv_wait_pshared(cv, mutex));
	}

	lock_enter(&cv->cv_lock);

	if (!COND_INITIALIZED(cv)) {
		COND_INIT_PATCH(cv);
	}

	pt_self->pt_state = PT_CVWAIT;
	pt_self->pt_sync = (pt_sync_t *)cv;
	lock_leave(&pt_self->pt_lock);

	pt_q_insert_tail(&cv->cv_q, pt_self);
	lock_leave(&cv->cv_lock);

	/*
	 * Can't unlock mutex until pt_lock is released -- might
	 * as well wait until cv_lock is released, too, so we
	 * aren't holding two interlocks.
	 */
	pthread_mutex_unlock(mutex);

	wait_result = sched_block(SCHED_READY);

	pthread_mutex_lock(mutex);

	if (wait_result == EINTR) {

		/* Check for deferred cancellation.
		 */
		if (pt_is_interrupted(pt_self)) {
			(void)intr_check(PT_INTERRUPTS);
		}
	}

	return (0);
}


static int
cv_wait_pshared(pthread_cond_t *cv, pthread_mutex_t *mutex)
{
	int		error = 0;
	usync_arg_t	usarg;

	if (!mutex->mtx_attr.ma_pshared) {
		return (EINVAL);
	}

	usarg.ua_version = USYNC_VERSION_2;
	usarg.ua_addr = (__uint64_t)cv;
	usarg.ua_policy = USYNC_POLICY_PRIORITY;
	usarg.ua_handoff = (__uint64_t)mutex;
	usarg.ua_flags = 0;
	if (pt_usync_cntl(USYNC_HANDOFF, &usarg, TRUE) == -1) {
		error = oserror();
	}

	pthread_mutex_lock(mutex);

	if (error == EINTR) {
		/*
		 * Check for deferred cancellation
		 */
		if (pt_is_interrupted(PT)) {
			(void)intr_check(PT_INTERRUPTS);
		}
	}

	return (0);
}


static void
cv_wakeup(register pt_t *pt)
{
	ASSERT(PT != pt);

	if (pt->pt_state == PT_CVTWAIT) {

		lock_enter(&pt->pt_lock);
		lock_enter(&pt->pt_sync_slock);

		if (pt->pt_state == PT_CVTWAIT) {

			Q_UNLINK(pt);
			lock_leave(&pt->pt_sync_slock);

			pt->pt_wait = ETIMEDOUT;
			pt->pt_state = PT_DISPATCH;
			lock_leave(&pt->pt_lock);

			sched_dispatch(pt);
			return;
		}

		lock_leave(&pt->pt_sync_slock);
		lock_leave(&pt->pt_lock);
	}

	return;
}


int
pthread_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *mutex,
		       const timespec_t *abstime)
{
	register pt_t	*pt_self = PT;
	timeout_t	timeout;
	int		wait_result;
	timespec_t	ts_now;
	int		e;

	TRACE(T_CV, ("pthread_cond_timedwait(%#x)", cv));

	pthread_testcancel();	/* catch pending cancel requests */

	if (e = libc_evt_start(0, 0)) {
		return (e);
	}

	if (!abstime || (unsigned long)(abstime->tv_nsec) >= 1000000000) {
		return (EINVAL);
	}

	if (!timeout_needed(&ts_now, abstime)) {
		pthread_mutex_unlock(mutex);
		/* POSIX rule: drop/reacquire for ETIMEDOUT */
		pthread_mutex_lock(mutex);
		return (ETIMEDOUT);
	}

	/* Process pending interruptions.
	 */
	PT_INTR_PENDING(pt_self, PT_INTERRUPTS);

	if (cv->cv_attr.ca_pshared) {
		lock_leave(&pt_self->pt_lock);
		sched_leave();
		return (cv_timedwait_pshared(cv, mutex, abstime));
	}

	lock_enter(&cv->cv_lock);

	if (!COND_INITIALIZED(cv)) {
		COND_INIT_PATCH(cv);
	}

	pt_self->pt_state = PT_CVTWAIT;
	pt_self->pt_sync = (pt_sync_t *)cv;
	lock_leave(&pt_self->pt_lock);

	pt_q_insert_tail(&cv->cv_q, pt_self);
	lock_leave(&cv->cv_lock);

	pthread_mutex_unlock(mutex);

	timeout_enqueue(&timeout, &ts_now, abstime, cv_wakeup);

	wait_result = sched_block(SCHED_READY);

	timeout_cancel(&timeout);

	pthread_mutex_lock(mutex);

	if (wait_result == ETIMEDOUT) {
		return (wait_result);
	}

	if (wait_result == EINTR) {

		/* Check for deferred cancellation.
		 */
		if (pt_is_interrupted(pt_self)) {
			(void)intr_check(PT_INTERRUPTS);
		}
	}

	return (0);
}


static int
cv_timedwait_pshared(pthread_cond_t *cv, pthread_mutex_t *mutex,
			const timespec_t *abstime)
{
	register int	error;
	usync_arg_t	usarg;

	if (!mutex->mtx_attr.ma_pshared) {
		return (EINVAL);
	}

	usarg.ua_version = USYNC_VERSION_2;
	usarg.ua_addr = (__uint64_t)cv;
	usarg.ua_policy = USYNC_POLICY_PRIORITY;
	usarg.ua_handoff = (__uint64_t)mutex;
	usarg.ua_flags = USYNC_FLAGS_TIMEOUT;
	usarg.ua_sec = (__uint64_t)abstime->tv_sec;
	usarg.ua_nsec = (__uint64_t)abstime->tv_nsec;
	error = 0;
	if (pt_usync_cntl(USYNC_HANDOFF, &usarg, TRUE) == -1) {
		error = oserror();
	}

	pthread_mutex_lock(mutex);

	if (error == ETIMEDOUT)
		return (ETIMEDOUT);

	if (error == EINTR) {
		/*
		 * Check for deferred cancellation
		 */
		if (pt_is_interrupted(PT)) {
			(void)intr_check(PT_INTERRUPTS);
		}
	}

	return (0);
}
