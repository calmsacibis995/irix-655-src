/**************************************************************************
 *									  *
 * 		 Copyright (C) 1998 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ifndef	_KSYS_KCALLOUT_H_
#define	_KSYS_KCALLOUT_H_	1
#ident "$Id: kcallout.h,v 1.1 1999/05/14 20:13:13 lord Exp $"

#include <ksys/kqueue.h>

/*
 * A callout list is a kqueue_t linked list with listhead:
 */
typedef kqueuehead_t kcallouthead_t;

#if defined(_KERNEL) && !defined(_MASTER_C)
#include <strings.h>
#include <sys/debug.h>
#include <sys/kmem.h>

/*
 * This file contains definitions for manipulating callout lists.
 * Callout lists are derived from kqueue's. A callout list entry
 * is a request to call a specified function with 6 arguments:
 * (four void *, plus optionally the address of argument block and
 * its size). The arguments are allocated as part of the callout entry
 * and this is freed after the callout function has been dispatched.
 * Hence, a callout entry is a variable-sized object.
 *
 * It is the responsibility of the caller to provide serialization -
 * these routines are not MP-safe. Typically the user will embed the
 * callout list in an object whose semantics provides this synchronization.
 *
 * The set of kernel callout primitives provided is:
 *	kcallout_init
 *		Initialize a callout listhead.
 *	kcallout_create
 *		Create a callout entry.
 *	kcallout_destroy
 *		Destroy a callout entry.
 *	kcallout_remove
 *		Remove a callout entry from its list.
 *	kcallout_isempty
 *		Return whether a callout list is empty.
 *	kcallout_enter
 *		Enter a previously created callout entry into a given list.
 *	kcallout_request
 *		Create a callout entry and entry in given list.
 *	kcallout_first
 *		Return the first callout entry in a given list.
 *	kcallout_next
 *		Return the next callout entry.
 *	kcallout_end
 *		Returns the end of list (marker).
 *	kcallout_cancel
 *		Remove and destroy a callout entry.
 *	kcallout_dispatch
 *		Dispatch and then remove/destroy a callout entry.
 *	kcallout_cancel_first
 *		Remove and destroy the first callout of a list.
 *	kcallout_dispatch_first
 *		ispatch, remove and destroy the first callout of a list.
 * 	kcallout_cancel_all
 *		Remove and destroy all callouts in a list.
 * 	kcallout_dispatch_all
 *		Dispatch, remove and destroy all callouts in a list.
 *      kcallout_dispatch_until_stop
 *              Dispatch, remove and destroy callouts in a list, stopping
 *              after dispatching one with a stop indication.
 */

/*
 * This is the type of a kernel callout function. It takes 4 annonymous
 * void * arguments plus an address, size pair.
 */
typedef void kcalloutfunc_t(void *arg0, void *arg1, void *arg2, void *arg3,
			    caddr_t argv, size_t argvsz);

/*
 * Each callout request is:
 */
typedef struct kcallout {
	kqueue_t	kc_queue;	/* queue link - must be first */
        int             kc_stop;        /* stop dispatch after this entry. */
	kcalloutfunc_t	*kc_func;	/* callout function */
	void		*kc_arg0;	/* first fixed argument */
	void		*kc_arg1;	/* second fixed argument */
	void		*kc_arg2;	/* third fixed argument */
	void		*kc_arg3;	/* fourth fixed argument */
	size_t		kc_argvsz;	/* size of optional argument block */
	char		kc_argv[1];	/* start of optional argument block */
} kcallout_t;


/*
 * Initializer for a callout list.
 */
static __inline void
kcallout_init(
	kcallouthead_t	*klist)
{
	kqueue_init(klist);
}

/*
 * Allocate a callout request entry and copy-in the details. The callout
 * entry is returned to the caller to be entered into a list (or destroyed).
 * Note that this may sleep for memory.
 */
static __inline kcallout_t *
kcallout_create(
	kcalloutfunc_t	*func,
	void		*arg0,
	void		*arg1,
	void		*arg2,
	void		*arg3,
	caddr_t		argv,
	size_t		argvsz,
        int             stop)
{
	kcallout_t	*kcp;

	kcp = kmem_alloc(offsetof(kcallout_t, kc_argv) + argvsz, KM_SLEEP); 
	kqueue_null(&kcp->kc_queue);
	kcp->kc_stop = stop;
	kcp->kc_func = func;
	kcp->kc_arg0 = arg0;
	kcp->kc_arg1 = arg1;
	kcp->kc_arg2 = arg2;
	kcp->kc_arg3 = arg3;
	kcp->kc_argvsz = argvsz;
	if (argvsz > 0)
		bcopy(argv, kcp->kc_argv, argvsz);

	return kcp;
}

static __inline void
kcallout_destroy(
	kcallout_t	*kcp)
{
	ASSERT(kqueue_isnull(&kcp->kc_queue));
	kmem_free(kcp, offsetof(kcallout_t, kc_argv) + kcp->kc_argvsz);
}

static __inline void
kcallout_remove(
	kcallout_t	*kcp)
{
	ASSERT(!kqueue_isnull(&kcp->kc_queue));
	kqueue_remove(&kcp->kc_queue);
	kqueue_null(&kcp->kc_queue);
}

static __inline int
kcallout_isempty(
	kcallouthead_t	*klist)
{
	return kqueue_isempty(klist);
}

static __inline void
kcallout_enter(
	kcallouthead_t	*klist,
	kcallout_t	*kcp)
{
	kqueue_enter_last(klist, &kcp->kc_queue);
}

/*
 * Create a callout request and queue it. This function should be
 * used when the calling thread can tolerate sleeping for memory,
 * and should not be used if the caller is using a spinlock to protect
 * the callout list.
 */ 
static __inline void
kcallout_request(
	kcallouthead_t	*klist,
	kcalloutfunc_t	*func,
	void		*arg0,
	void		*arg1,
	void		*arg2,
	void		*arg3,
	caddr_t		argv,
	size_t		argvsz)
{
	/* Create entry */
	kcallout_t *kcp = kcallout_create(func, arg0, arg1, arg2, arg3,
						argv, argvsz, 0);

	/* Enter on queue */
	kcallout_enter(klist, kcp);
}

/*
 * The following (kcallout_first, kcallout_next, kcallout_isend) provide 
 * iteration over a callout list. The generic for loop being:
 *	for (kcp =  kcallout_first(klist);
 *	     kcp != kcallout_end(klist);
 *	     kcp =  kcallout_next(kcp)) {
 *		...
 *	}
 */
static __inline kcallout_t *
kcallout_first(
	kcallouthead_t  *klist)
{
	return (kcallout_t *) kqueue_first(klist);
}

static __inline kcallout_t *
kcallout_end(
	kcallouthead_t  *klist)
{
	return  (kcallout_t *) kqueue_end(klist);
}

static __inline kcallout_t *
kcallout_next(
	kcallout_t  *kcp)
{
	return (kcallout_t *) kqueue_next(&kcp->kc_queue);
}



static __inline void
kcallout_cancel(
	kcallout_t	*kcp)
{
	kcallout_remove(kcp);
	kcallout_destroy(kcp);
}

static __inline void
kcallout_dispatch(
	kcallout_t	*kcp)
{
	(*kcp->kc_func)(kcp->kc_arg0, kcp->kc_arg1, kcp->kc_arg2, kcp->kc_arg3,
			kcp->kc_argvsz > 0 ? kcp->kc_argv : NULL,
			kcp->kc_argvsz);
}

static __inline int
kcallout_cancel_first(
	kcallouthead_t	*klist)
{
	if (kcallout_isempty(klist))
		return 0;
	else {
		kcallout_cancel(kcallout_first(klist)); 
		return 1;
	}
}

static __inline int
kcallout_dispatch_first(
	kcallouthead_t	*klist)
{
	if (kcallout_isempty(klist))
		return 0;
	else {
		kcallout_dispatch(kcallout_first(klist)); 
		kcallout_cancel(kcallout_first(klist));
		return 1;
	}
}

static __inline int
kcallout_cancel_all(
	kcallouthead_t	*klist)
{
	int	i = 0;

	while (kcallout_cancel_first(klist))
		i++;
	return i;
}

static __inline int
kcallout_dispatch_all(
	kcallouthead_t	*klist)
{
	int	i = 0;
	while (kcallout_dispatch_first(klist))
		i++;
	return i;
}

static __inline int
kcallout_dispatch_until_stop(
	kcallouthead_t	*klist)
{
	int     stop = 0;

	while (!(kcallout_isempty(klist) || stop)) {
		kcallout_t	*kcp = kcallout_first(klist);

		kcallout_dispatch(kcp);
		stop = kcp->kc_stop;
		kcallout_cancel(kcp);
	}
	return stop;
}

#endif /* KERNEL */
#endif	/* _KSYS_KCALLOUT_H_ */
