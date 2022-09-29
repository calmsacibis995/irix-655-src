/**************************************************************************
 *									  *
 * 		 Copyright (C) 1994-1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ident "$Id: exec_noncell.c,v 1.1 1999/05/14 20:13:13 lord Exp $"

/*
 * Contains non-cell versions of routines where there are cell and non-cell
 * specific routines
 */

#include <sys/types.h>
#include <sys/cred.h>
#include <sys/vnode.h>
#include <sys/dmi_kern.h>
#include <sys/fcntl.h>
/*
 *	Check if the file has DMAPI managed regions/events.  If so,
 *	generate a DMAPI read event for the entire file.  
 *
 *	Only a "read" event will be generated since check_dmapi_file
 *	is intended to be called only for gexec() and elfmap() files.
 *	In these cases, if the process later uses the mprotect() syscall
 *	to upgrade the page protection to include PROT_WRITE, the mapping
 *	type is changed to private.
 *
 *	Note that the VOP_FCNTL used here returns an error if the
 *	underlying file system is unaware of the F_DMAPI subfunction
 *	being used.  This causes no problems, since a non-zero return
 *	status is simply ignored.  Only in the case of a zero return status
 *	can we be sure that the VOP_FCNTL F_DMAPI subfunction
 *	DM_FCNTL_MAPEVENT is implemented for this file system, and then
 *	interpret the maprq.error field.
 */
/* ARGSUSED */
int
check_dmapi_file(vnode_t	*vp)
{
	int		error;
	dm_fcntl_t	dmfcntl;

	dmfcntl.dmfc_subfunc = DM_FCNTL_MAPEVENT;
	dmfcntl.u_fcntl.maprq.length = 0;	/* length = 0 for whole file */
	dmfcntl.u_fcntl.maprq.max_event = DM_EVENT_READ;

	VOP_FCNTL(vp, F_DMAPI, &dmfcntl, 0, (off_t)0, sys_cred, NULL, error);
	if (error == 0) {
		if ((error = dmfcntl.u_fcntl.maprq.error) != 0)
			return error;
	}
	return 0;
}
