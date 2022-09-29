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
 * This module implements the low level simple locks.
 *
 * Key points:
	* On an MP these locks spin.
	* The holder of the lock is the VP; a VP must not change pthreads
	  while holding a simple lock.
 */

#include "common.h"
#include "asm.h"
#include "mtx.h"
#include "pt.h"
#include "vp.h"

#include <errno.h>
#include <pthread.h>
#include <sys/usync.h>
#include <mutex.h>
#include <unistd.h>

extern int __usync_cntl(int, void*);

/* Simple locks.
 *
 * Simple locks are the most primitive locks.
 *	The lockwd field is 0 if the lock is unlocked, otherwise it is one
 *	less than the number of waiters for the lock (i.e, if a thread holds
 *	the lock but nobody is waiting, lockwd is 1).
 *	If lockers find that the lock is already held, they either
 *	block immediately (UP or bound) or spin (MP and not bound).
 *	
 *	Unlockers decrement lockwd, waking up one waiter if necessary.
 *	
 *	Using locks based purely on spinning and back-off can cause
 *	priority inversions.
 */

/*  lock_tryenter(slock)
 *
 * Attempt to acquire lock; report success or failure.
 */
int
lock_tryenter(slock_t *slock)
{
	ASSERT(sched_entered());
	return (cmp0_and_swap(&slock->sl_lockwd, (void*)1uL));
}


/* lock_enter(slock)
 * 
 * Acquire the lock.
 * If it is already held either busy wait or block in the kernel
 * using a priority-ordered usync obj.
 */ 
void
lock_enter(slock_t *slock) 
{
	pt_t	*pt_self;

	ASSERT(sched_entered());

	if (cmp0_and_swap(&slock->sl_lockwd, (void*)1uL)) {
		return;
	}

	pt_self = PT;

	if (sched_proc_count == 1 || pt_self && pt_self->pt_system) {

		/* Add to waiter count and block until lock is released.
		 */
		if (test_then_add(&slock->sl_lockwd, 1) > 0) {
			usync_arg_t usarg;
			usarg.ua_version = USYNC_VERSION_2;
			usarg.ua_addr = (__uint64_t) slock;
			usarg.ua_policy = USYNC_POLICY_PRIORITY;
			usarg.ua_flags = 0;
			if (__usync_cntl(USYNC_INTR_BLOCK, &usarg) == -1) {
				panic("lock_enter", "USYNC_INTR_BLOCK failed");
			}
		} 
	} else {

		int spin = 1000;
		static struct timespec nano = { 0, 1000 };
		extern int __nanosleep(const struct timespec *,
					struct timespec *);

		for (;;) {

			if (cmp0_and_swap(&slock->sl_lockwd, (void*)1uL)) {
				break;
			}
			if (spin--) {
				continue;
			}
			spin = 1000;

			/* Sleep.
			 * Avoid a cancellation point, use libc.
			 */
			(void)__nanosleep(&nano, 0);
		}
	}
	ASSERT(slock->sl_lockwd >= 1);
	return;
}


/* lock_leave(slock)
 * 
 * If anybody is waiting for the lock, wake one of them up.
 */
void
lock_leave(slock_t *slock)
{
	ASSERT(sched_entered());

	if (test_then_add(&slock->sl_lockwd, -1) > 1) {
		usync_arg_t	usarg;
		usarg.ua_version = USYNC_VERSION_2;
		usarg.ua_addr = (__uint64_t)slock;
		usarg.ua_flags = 0;
		if (__usync_cntl(USYNC_UNBLOCK, &usarg) == -1) {
			panic("lock_leave", "USYNC_UNBLOCK failed");
		}
	} 

	ASSERT((long)slock->sl_lockwd >= 0);
}


int
lock_held(slock_t *slock)
{
	return (slock->sl_lockwd > 0);
}
