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

#ifndef	_KSYS_FSC_NOTIFY_H_
#define	_KSYS_FSC_NOTIFY_H_

#ident	"$Id: fsc_notify.h,v 1.2 1999/05/25 20:01:08 clk Exp $"

/*
 * ksys/fsc_notify.h -- Notification macros for cell-specifc fs code
 *
 * This header defines notifcation macros which file-systems may use
 * to notify cell-specific file-system code (i.e. cfs) that the unerlying
 * file system may have changed data necessitating invalidation or update
 * data cached on other cells.
 *
 * Use of most of these macros will only be necessary when the file 
 * system in question requests special cfs services such as name and 
 * attribute caching.  The requirements for using these macros are
 * detailed below.
 *
 * When a non-cell-capable kernel is compiled, no code is generated
 * for these macros.  When a cell-capable kernel is compiled but no
 * cell-specific code is linked in, the cost of these macros should be
 * very low.
 */
 
#include <sys/vfs.h>
#include <sys/vnode.h>

#if CELL_CAPABLE

#include <ksys/cell_config.h>

struct bhv_desc;

/*
 * FSC_NOTIFY_LINK_REMOVED is used to indicate that a link to a given
 * file or directory has been removed.  It must be used if either of
 * the following conditions is true:
 *  
 *      o The last link to a file or directory is being removed
 *      o Name or attribute caching is in efect. 
 *
 * Currently, FSC_NOTIFY_LINK_REMOVED generates a VOP_LINK_REMOVED.
 * This will be temporary, although direct use of VOP_LINK_REMOVED
 * will probably be supported until the next major.
 */
#define FSC_NOTIFY_LINK_REMOVED(vp, dvp, last) \
	CELL_ONLY(VOP_LINK_REMOVED(vp, dvp, last))

/*
 * FSC_NOTIFY_NAME_CHANGED is used to note that a file name has been
 * changed.  It is only required when name caching is in effect.  
 * Note that when file system does a name change by removing a link 
 * and then adding one (such as efs does), FSC_NOTIFY_LINK_REMOVED
 * will be invoked when the link is revoved and that since that is 
 * being done, FSC_NOTIFY_LINK_CHANGED is not required because the 
 * FSC_NOTIFY_LINK_REMOVED is adequate notification.
 */
#define FSC_NOTIFY_NAME_CHANGED(vp) \
	CELL_ONLY(cfs_notify_name_changed(vp))

extern void cfs_notify_name_changed(vnode_t *);

/*
 * FSC_NOTIFY_FRLOCK_WAIT_{PREPARE,DONE} are used by reclockGet
 * to call into the dsvn layer if we're going to sleep and
 * need to release/acquire tokens.
 */

extern void cfs_dsvn_frlock_wait_prepare(struct bhv_desc *, vrwlock_t, int);
extern void cfs_dsvn_frlock_wait_done(struct bhv_desc *, vrwlock_t, int);

#define FSC_NOTIFY_FRLOCK_WAIT_PREPARE(bhv, vrwlck, ioflag) \
	CELL_ONLY(cfs_dsvn_frlock_wait_prepare(bhv, vrwlck, ioflag))

#define FSC_NOTIFY_FRLOCK_WAIT_DONE(bhv, vrwlck, ioflag) \
	CELL_ONLY(cfs_dsvn_frlock_wait_done(bhv, vrwlck, ioflag))

#else /* CELL_CAPABLE */

#define FSC_NOTIFY_LINK_REMOVED(vp, dvp, last) 
#define FSC_NOTIFY_NAME_CHANGED(vp)

#define FSC_NOTIFY_FRLOCK_WAIT_PREPARE(bhv, vrwlck, ioflag)
#define FSC_NOTIFY_FRLOCK_WAIT_DONE(bhv, vrwlck, ioflag)

#endif /* CELL_CAPABLE */

#endif /* _KSYS_FSC_NOTIFY_H_ */


