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
#ifndef	_FS_FS_BHV_ID_H_
#define	_FS_FS_BHV_ID_H_
#ident	"$Id: fs_bhv_id.h,v 1.1 1999/05/14 20:13:13 lord Exp $"

/*
 * Types of vnode behaviors that are recognized for relocation.
 */
typedef enum {  VN_BHV_UNKNOWN,		/* not specified */
		VN_BHV_CFS,		/* CFS itself */
		VN_BHV_PR,		/* procfs */
		VN_BHV_XFS,		/* xfs */
		VN_BHV_END		/* housekeeping end-of-range */
} vn_bhv_t;

/*
 * Types of vfs behaviors that are recognized for relocation.
 */
typedef enum {  VFS_BHV_UNKNOWN,	/* not specified */
		VFS_BHV_CFS,		/* CFS itself */
		VFS_BHV_XFS,		/* xfs */
		VFS_BHV_END		/* housekeeping end-of-range */
} vfs_bhv_t;

#endif /* _FS_FS_BHV_ID_H_ */
