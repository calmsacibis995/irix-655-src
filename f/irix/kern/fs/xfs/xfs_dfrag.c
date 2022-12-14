#ident "$Id: xfs_dfrag.c,v 1.5 1999/05/14 20:13:13 lord Exp $"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/sema.h>
#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/cred.h>

#include <sys/immu.h>
#include <sys/time.h>
#include <sys/kabi.h>
#include <ksys/vfile.h>
#include <ksys/fdt.h>
#include <ksys/cell_config.h>
#include <sys/vfs.h>
#include <sys/syssgi.h>
#include <sys/mac_label.h>
#include <sys/capability.h>
#include <sys/uuid.h>
#include <sys/hwgraph.h>
#include <sys/mode.h>

#include "xfs_macros.h"
#include "xfs_types.h"
#include "xfs_inum.h"
#include "xfs_log.h"
#include "xfs_trans.h"
#include "xfs_sb.h"

#include "xfs_dir.h"
#include "xfs_dir2.h"
#include "xfs_mount.h"
#include "xfs_ag.h"
#include "xfs_alloc_btree.h"
#include "xfs_bmap_btree.h"
#include "xfs_ialloc_btree.h"
#include "xfs_btree.h"
#include "xfs_attr_sf.h"
#include "xfs_dir_sf.h"
#include "xfs_dir2_sf.h"
#include "xfs_dinode.h"
#include "xfs_inode_item.h"
#include "xfs_inode.h"
#include "xfs_ialloc.h"
#include "xfs_itable.h"
#include "xfs_dfrag.h"
#include "xfs_error.h"
#include "xfs_cxfs.h"

extern void xfs_lock_inodes (xfs_inode_t **, int, int, uint);
	
/*
 * Syssgi interface for swapext
 */
int
xfs_swapext(
	xfs_swapext_t   *sxp)
{
	xfs_swapext_t 	sx;
        xfs_inode_t     *ip, *tip, *ips[2];
	xfs_trans_t     *tp;
	xfs_mount_t     *mp;
	xfs_bstat_t	*sbp;
	struct vfile	*fp, *tfp;
	vnode_t 	*vp, *tvp;
        bhv_desc_t      *bdp, *tbdp;
        vn_bhv_head_t   *bhp, *tbhp;
	xfs_fsize_t	last_byte;
	cred_t		*credp = get_current_cred();
	uint		lock_flags;
	int		ilf_fields, tilf_fields;
	int		error = 0;
	xfs_ifork_t	tempif, *ifp, *tifp;
	__uint64_t	tmp;
	__uint64_t	cxfs_val;

	if (copyin(sxp, &sx, sizeof sx))
		return XFS_ERROR(EFAULT);

	/* Pull information for the target fd */
	if (error = getf(sx.sx_fdtarget, &fp))
		return XFS_ERROR(error);

	if (!VF_IS_VNODE(fp) || VF_TO_VNODE(fp)->v_type != VREG)
		return XFS_ERROR(EINVAL);

	vp = VF_TO_VNODE(fp);
	bhp = VN_BHV_HEAD(vp);
	VN_BHV_READ_LOCK(bhp);
	bdp = vn_bhv_lookup(bhp, &xfs_vnodeops);
	if (bdp == NULL) {
		VN_BHV_READ_UNLOCK(bhp);
		return XFS_ERROR(EBADF);
	} else {
		ip = XFS_BHVTOI(bdp);
		VN_BHV_READ_UNLOCK(bhp);
	}

	/* Pull information for the tmp fd */
	if (error = getf(sx.sx_fdtmp, &tfp))
		return XFS_ERROR(error);

	if (!VF_IS_VNODE(tfp) || VF_TO_VNODE(tfp)->v_type != VREG)
		return XFS_ERROR(EINVAL);

	tvp = VF_TO_VNODE(tfp);
	tbhp = VN_BHV_HEAD(tvp);
	VN_BHV_READ_LOCK(tbhp);
	tbdp = vn_bhv_lookup(tbhp, &xfs_vnodeops);
	if (tbdp == NULL) {
		VN_BHV_READ_UNLOCK(tbhp);
		return XFS_ERROR(EBADF);
	} else {
		tip = XFS_BHVTOI(tbdp);
		VN_BHV_READ_UNLOCK(tbhp);
	}

	if (ip->i_ino == tip->i_ino) {
		return XFS_ERROR(EINVAL);
	}

	mp = ip->i_mount;

	sbp = &sx.sx_stat;

	if (XFS_FORCED_SHUTDOWN(mp))
		return XFS_ERROR(EIO);

	CELL_ONLY(cxfs_val = cfs_start_defrag(vp));

	/* quit if either is the swap file */
	if (vp->v_flag & VISSWAP && vp->v_type == VREG)
		return XFS_ERROR(EACCES);
	if (tvp->v_flag & VISSWAP && tvp->v_type == VREG)
		return XFS_ERROR(EACCES);

	/* Lock in i_ino order */
	if (ip->i_ino < tip->i_ino) {
		ips[0] = ip;
		ips[1] = tip;
	} else {
		ips[0] = tip;
		ips[1] = ip;
	}
	lock_flags = XFS_ILOCK_EXCL | XFS_IOLOCK_EXCL;
	xfs_lock_inodes(ips, 2, 0, lock_flags);

	/* Check permissions */
        if (error = _MAC_XFS_IACCESS(ip, MACWRITE, credp)) {
		goto error0;
	}
        if (error = _MAC_XFS_IACCESS(tip, MACWRITE, credp)) {
		goto error0;
	}
	if ((credp->cr_uid != ip->i_d.di_uid) &&
	    (error = xfs_iaccess(ip, IWRITE, credp)) &&
	    !cap_able_cred(credp, CAP_FOWNER)) {
		goto error0;
	}	
	if ((credp->cr_uid != tip->i_d.di_uid) &&
	    (error = xfs_iaccess(tip, IWRITE, credp)) &&
	    !cap_able_cred(credp, CAP_FOWNER)) {
		goto error0;
	}	

	/* Verify both files are either real-time or non-realtime */
	if ((ip->i_d.di_flags & XFS_DIFLAG_REALTIME) !=
	    (tip->i_d.di_flags & XFS_DIFLAG_REALTIME)) {
		error = XFS_ERROR(EINVAL);
		goto error0;
	}

	/* Should never get a local format */
	if (ip->i_d.di_format == XFS_DINODE_FMT_LOCAL ||
	    tip->i_d.di_format == XFS_DINODE_FMT_LOCAL) {
		error = XFS_ERROR(EINVAL);
		goto error0;
	}

	/* Verify O_DIRECT for ftmp */
	if (tvp->v_pgcnt != 0 || tvp->v_buf != 0) {
		error = XFS_ERROR(EINVAL);
		goto error0;
	}

	/* Verify all data are being swapped */
	if (sx.sx_offset != 0 || 
	    sx.sx_length != ip->i_d.di_size ||
	    sx.sx_length != tip->i_d.di_size) {
		error = XFS_ERROR(EFAULT);
		goto error0;
	}
		
	/* 
	 * This version does not know how to distinguish
	 * the attribute fork blocks from the data fork
	 * blocks in the di_nblocks value.  Code needs to
	 * be written to walk and count the data fork blocks
	 * and indirect blocks.  Until then,  we do not 
	 * support swapping files that have attributes.
	 */
	if ( ((XFS_IFORK_Q(ip) != 0) && (ip->i_d.di_anextents > 0)) ||
	     ((XFS_IFORK_Q(tip) != 0) && (tip->i_d.di_anextents > 0)) ) {
		error = XFS_ERROR(ENOTSUP);
		goto error0;
	}

	/* 
	 * Compare the current change & modify times with that 
	 * passed in.  If they differ, we abort this swap.
	 * This is the mechanism used to ensure the calling
	 * process that the file was not changed out from
	 * under it.
	 */
	if ((sbp->bs_ctime.tv_sec != ip->i_d.di_ctime.t_sec) ||
	    (sbp->bs_ctime.tv_nsec != ip->i_d.di_ctime.t_nsec) ||
	    (sbp->bs_mtime.tv_sec != ip->i_d.di_mtime.t_sec) ||
	    (sbp->bs_mtime.tv_nsec != ip->i_d.di_mtime.t_nsec)) {
		error = XFS_ERROR(EBUSY);
		goto error0;
	}

	/* We need to fail if the file is memory mapped, we also need to
	 * prevent it from getting mapped before we have tossed the existing
	 * pages. By setting VREMAPPING here we force a pas_vfault to go to
	 * the filesystem for pages. Once we have tossed all existing pages
	 * we can clear VREMAPPING as the page fault will have no option but
	 * to go to the filesystem for pages. By making the page fault call
	 * VOP_READ (or write in the case of autogrow) they block on the iolock
	 * until we have switched the extents.
	 */
	VN_FLAGSET(vp, VREMAPPING);
	if (VN_MAPPED(vp)) {
		error = XFS_ERROR(EBUSY);
		VN_FLAGCLR(vp, VREMAPPING);
		goto error0;
	}

	xfs_iunlock(ip, XFS_ILOCK_EXCL);
	xfs_iunlock(tip, XFS_ILOCK_EXCL);

	/* 
	 * There is a race condition here since we gave up the
	 * ilock.  However, the data fork will not change since
	 * we have the iolock (locked for truncation too) so we 
	 * are safe.  We don't really care if non-io related
	 * fields change.
	 */

	last_byte = xfs_file_last_byte(ip);
	VOP_TOSS_PAGES(vp, 0, last_byte - 1, FI_REMAPF);
	VN_FLAGCLR(vp, VREMAPPING);

	tp = xfs_trans_alloc(mp, XFS_TRANS_SWAPEXT);
	if (error = xfs_trans_reserve(tp, 0,
				     XFS_ICHANGE_LOG_RES(mp), 0,
				     0, 0)) {
		xfs_trans_cancel(tp, 0);
		return error;
	}
	xfs_lock_inodes(ips, 2, 0, XFS_ILOCK_EXCL);

	/* 
	 * Swap the data forks of the inodes 
	 */
	ifp = &ip->i_df;
	tifp = &tip->i_df;
	tempif = *ifp;	/* struct copy */
	*ifp = *tifp;	/* struct copy */
	*tifp = tempif;	/* struct copy */

	/* 
	 * Fix the on-disk inode values
	 * 
	 * The di_nblocks value includes data blocks, attribute
	 * blocks and indirect blocks.  The only reason I can
	 * copy this value like I'm doing is that I've already
	 * checked that no attribute blocks are involved with
	 * these two inodes.  To handle attributes, I need to
	 * walk one of the forks to count all blocks and re-do
	 * calculation.
	 */
	tmp = (__uint64_t)ip->i_d.di_nblocks;
	ip->i_d.di_nblocks = tip->i_d.di_nblocks;
	tip->i_d.di_nblocks = tmp;

	tmp = (__uint64_t) ip->i_d.di_nextents;
	ip->i_d.di_nextents = tip->i_d.di_nextents;
	tip->i_d.di_nextents = tmp;

	tmp = (__uint64_t) ip->i_d.di_format;
	ip->i_d.di_format = tip->i_d.di_format;
	tip->i_d.di_format = tmp;

	ilf_fields = XFS_ILOG_CORE;

	switch(ip->i_d.di_format) {
	case XFS_DINODE_FMT_EXTENTS:
		/* If the extents fit in the inode, fix the 
		 * pointer.  Otherwise it's already NULL or 
		 * pointing to the extent.
		 */
		if (ip->i_d.di_nextents <= XFS_INLINE_EXTS) {
			ifp->if_u1.if_extents = 
				ifp->if_u2.if_inline_ext;
		}
		ilf_fields |= XFS_ILOG_DEXT;
		break;
	case XFS_DINODE_FMT_BTREE:
		ilf_fields |= XFS_ILOG_DBROOT;
		break;
	}
	
	tilf_fields = XFS_ILOG_CORE;

	switch(tip->i_d.di_format) {
	case XFS_DINODE_FMT_EXTENTS:
		/* If the extents fit in the inode, fix the 
		 * pointer.  Otherwise it's already NULL or 
		 * pointing to the extent.
		 */
		if (tip->i_d.di_nextents <= XFS_INLINE_EXTS) {
			tifp->if_u1.if_extents = 
				tifp->if_u2.if_inline_ext;
		}
		tilf_fields |= XFS_ILOG_DEXT;
		break;
	case XFS_DINODE_FMT_BTREE:
		tilf_fields |= XFS_ILOG_DBROOT;
		break;
	}

	/*
	 * Increment vnode ref counts since xfs_trans_commit &
	 * xfs_trans_cancel will both unlock the inodes and
	 * decrement the associated ref counts.
	 */
	VN_HOLD(vp);
	VN_HOLD(tvp);

	xfs_trans_ijoin(tp, ip, lock_flags);
	xfs_trans_ijoin(tp, tip, lock_flags);

	xfs_trans_log_inode(tp, ip,  ilf_fields);
	xfs_trans_log_inode(tp, tip, tilf_fields);

        /*
         * If this is a synchronous mount, make sure that the
         * transaction goes to disk before returning to the user.
         */
        if (mp->m_flags & XFS_MOUNT_WSYNC) {
                xfs_trans_set_sync(tp);
        }

        error = xfs_trans_commit(tp, XFS_TRANS_SWAPEXT, NULL);

	CELL_ONLY(cfs_end_defrag(vp, cxfs_val));
	return error;

 error0:
	CELL_ONLY(cfs_end_defrag(vp, cxfs_val));
	xfs_iunlock(ip,  lock_flags);
	xfs_iunlock(tip, lock_flags);
	return error;
}
