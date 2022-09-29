/**************************************************************************
 *									  *
 *		 Copyright (C) 1998 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ifndef	_KSYS_SYSNAME_H_
#define	_KSYS_SYSNAME_H_
#ident	"$Id: sysname.h,v 1.1 1999/05/14 20:13:13 lord Exp $"

/*
 * Header defining semi-private data and interfaces (may be used in the 
 * kernel proper but not exported to third-party code) relating to system 
 * naming functions (hostname, domainname, etc.).
 */

#include <sys/sema.h>

extern mutex_t setname_lock;

#define SETNAME_LOCK()		mutex_lock(&setname_lock, PZERO)
#define SETNAME_UNLOCK()	mutex_unlock(&setname_lock)
#define SETNAME_MINE()          mutex_mine(&setname_lock)

#endif	/* _KSYS_SYSNAME_H_ */
