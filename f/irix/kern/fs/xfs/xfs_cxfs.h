/**************************************************************************
 *									  *
 * 		 Copyright (C) 1997, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

#ifndef __FS_XFS_XFS_CXFS_H__
#define __FS_XFS_XFS_CXFS_H__

#ident "$Revision: 1.3 $"

/*
 * xfs_cxfs.h -- Interface cxfs presents to non-cell xfs code
 *
 * This header specifies the interface that cxfs V1 code presents to the 
 * non-cellular parts of xfs.  When the specfied routines are not present,
 * stubs will be provided.
 */

struct xfs_inode;
struct xfs_mount;
struct xfs_args;
struct mounta;
struct vfs;
struct vfsops;
struct vnode;
struct buf;

/*
 * Array mount routines.  Stubs provided for the non-CELL case.
 */
extern void cxfs_arrinit(void); /* Initialization for array mount logic. */
extern int cxfs_mount(	        /* For any specia mount handling. */
		struct xfs_mount    *mp,
                struct xfs_args     *ap,
		dev_t		    dev,
		int	            *client);
extern void cxfs_unmount(       /* For any special unmount handling. */
		struct xfs_mount    *mp);

/*
 * Other cxfs routines.  Stubs provided in non-CELL case.
 */
extern void cxfs_inode_quiesce(             /* Quiesce new inode for vfs */
		struct xfs_inode    *ip);   /* relocation. */
extern int cxfs_inode_qset(                 /* Set quiesce flag on inode. */
		struct xfs_inode    *ip);  
extern int cxfs_remount_server(             /* Modify mount parameters.  This */
                struct xfs_mount    *mp,    /* may result in vfs relocation. */
                struct mounta       *uap,   /* There are separate implementa- */
                struct xfs_args     *ap);   /* tions for arrays and ssi as */
                                            /* well as a stub for non-CELL. */

extern struct xfs_mount *get_cxfs_mountp(struct vfs *);

extern void cxfs_strat_complete_buf(struct buf *);

extern __uint64_t cfs_start_defrag(
		struct vnode		*vp);
extern void	cfs_end_defrag(
		struct vnode    	*vp,
		__uint64_t		handle);
		
#endif /* __FS_XFS_XFS_CXFS_H__ */
