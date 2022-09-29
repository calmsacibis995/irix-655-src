/**************************************************************************
 *									  *
 * 		 Copyright (C) 1995-1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/* This module implements thread facilities for apps/DSO code which is
 * required to work with either sprocs or pthreads.
 *
 * Scheme
 * Dummy pthread calls:
 *	One may use __multi_thread to decide at run time which thread calls
 *	to use; the default should be to use sproc calls.
 *	The dummy pthread calls are purely to permit a DSO to resolve symbols
 *	normally provided by libpthread - they will cause an error if executed
 *	at run time.
 *	These interfaces are preempted by libpthread if it is loaded.
 *
 * __multi_thread:
 *		- set at load time if pthreads is used
 *		- set at run time when the first sproc is created
 *	Pthreads is assumed to always be threaded and require protection.
 *	Sprocs and pthreads cannot exist in the same process.
 */

#ifdef __STDC__
	#pragma weak pthread_create =		__pt_dummy0
	#pragma weak pthread_exit =		__pt_dummy0
	#pragma weak pthread_testcancel =	__pt_dummy0
	#pragma weak pthread_self =		__pt_dummy1
	#pragma weak pthread_mutex_init =	__pt_dummy0
	#pragma weak pthread_mutex_destroy =	__pt_dummy0
	#pragma weak pthread_mutex_lock =	__pt_dummy0
	#pragma weak pthread_mutex_trylock =	__pt_dummy0
	#pragma weak pthread_mutex_unlock =	__pt_dummy0
	#pragma weak pthread_cond_init =	__pt_dummy0
	#pragma weak pthread_cond_destroy =	__pt_dummy0
	#pragma weak pthread_cond_wait =	__pt_dummy0
	#pragma weak pthread_cond_timedwait =	__pt_dummy0
	#pragma weak pthread_cond_signal =	__pt_dummy0
	#pragma weak pthread_cond_broadcast =	__pt_dummy0
	#pragma weak pthread_attr_init =	__pt_dummy0
	#pragma weak pthread_attr_destroy =	__pt_dummy0
	#pragma weak pthread_attr_setstacksize =	__pt_dummy0
	#pragma weak pthread_attr_setstackaddr =	__pt_dummy0
	#pragma weak pthread_attr_setdetachstate =	__pt_dummy0
#endif


#include "synonyms.h"
#include <sys/types.h>
#include <task.h>
#include <unistd.h>
#include <mplib.h>
#include "us.h"

unsigned
__pt_dummy0(void)
{
	return (0);
}

unsigned
__pt_dummy1(void)
{
	return (-1);
}

int
_mplib_get_thread_type(void)
{
	return (__multi_thread);
}

unsigned
_mplib_get_thread_id(void)
{
	extern unsigned pthread_self(void);

	return ((__multi_thread == MT_PTHREAD)
		? pthread_self() : (unsigned)get_pid());
}

