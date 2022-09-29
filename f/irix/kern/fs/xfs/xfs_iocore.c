#ident "$Revision: 1.2 $"

#ifdef SIM
#define _KERNEL 1
#endif
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/cred.h>
#include <sys/sysmacros.h>
#include <sys/pfdat.h>
#include <sys/uuid.h>
#include <sys/major.h>
#include <sys/grio.h>
#include <sys/pda.h>
#include <sys/dmi_kern.h>
#ifdef SIM
#undef _KERNEL
#endif
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/errno.h> 
#include <sys/fcntl.h>
#include <sys/var.h>
#ifdef SIM
#include <bstring.h>
#include <stdio.h>
#else
#include <sys/conf.h>
#include <sys/systm.h>
#endif
#include <sys/kmem.h>
#include <sys/sema.h>
#include <ksys/vfile.h>
#include <sys/flock.h>
#include <sys/fs_subr.h>
#include <sys/dmi.h>
#include <sys/dmi_kern.h>
#include <sys/schedctl.h>
#include <sys/atomic_ops.h>
#include <sys/ktrace.h>
#include <sys/sysinfo.h>
#include <sys/ksa.h>
#include <ksys/sthread.h>
#include "xfs_macros.h"
#include "xfs_types.h"
#include "xfs_inum.h"
#include "xfs_log.h"
#include "xfs_trans.h"
#include "xfs_sb.h"
#include "xfs_ag.h"
#include "xfs_dir.h"
#include "xfs_dir2.h"
#include "xfs_mount.h"
#include "xfs_alloc_btree.h"
#include "xfs_bmap_btree.h"
#include "xfs_ialloc_btree.h"
#include "xfs_itable.h"
#include "xfs_btree.h"
#include "xfs_alloc.h"
#include "xfs_bmap.h"
#include "xfs_ialloc.h"
#include "xfs_attr_sf.h"
#include "xfs_dir_sf.h"
#include "xfs_dir2_sf.h"
#include "xfs_dinode.h"
#include "xfs_inode_item.h"
#include "xfs_inode.h"
#include "xfs_error.h"
#include "xfs_bit.h"
#include "xfs_rw.h"
#include "xfs_quota.h"
#include "xfs_trans_space.h"
#include "xfs_dmapi.h"
#include <limits.h>

#ifdef SIM
#include "sim.h"
#endif

/* ARGSUSED */
static int
xfs_rsync_fn(
	xfs_inode_t	*ip,
	int		ioflag,
	off_t		start,
	off_t		end)
{
	xfs_mount_t	*mp = ip->i_mount;
	int		error = 0;

	if (ioflag & IO_SYNC) {
		xfs_ilock(ip, XFS_ILOCK_SHARED);
		xfs_iflock(ip);
		error = xfs_iflush(ip, XFS_IFLUSH_SYNC);
		xfs_iunlock(ip, XFS_ILOCK_SHARED);
		return error;
	} else {
		if (ioflag & IO_DSYNC) {
			xfs_log_force(mp, (xfs_lsn_t)0,
					XFS_LOG_FORCE | XFS_LOG_SYNC );
		}
	}

	return error;
}


static xfs_fsize_t
xfs_size_fn(
	xfs_inode_t	*ip)
{
	return (ip->i_d.di_size);
}

static xfs_fsize_t
xfs_setsize_fn(
	xfs_inode_t	*ip,
	xfs_fsize_t	newsize)
{
	xfs_fsize_t	isize;

	xfs_ilock(ip, XFS_ILOCK_EXCL);
	if (newsize  > ip->i_d.di_size) {
		ip->i_d.di_size = newsize;
		ip->i_update_core = 1;
		ip->i_update_size = 1;
		isize = newsize;
	} else {
		isize = ip->i_d.di_size;
	}
	xfs_iunlock(ip, XFS_ILOCK_EXCL);

	return isize;
}

#ifndef SIM
int
xfs_frlock2(
	bhv_desc_t	*bdp,
	int		cmd,
	flock_t		*flockp,
	int		flag,
	off_t		offset,
	vrwlock_t	vrwlock,
	cred_t		*credp,
	int		ioflag)
{
	xfs_inode_t	*ip;
	int		dolock, error;

	ASSERT(BHV_IS_XFS(bdp));

	vn_trace_entry(BHV_TO_VNODE(bdp), "xfs_frlock2",
			(inst_t *)__return_address);
	ip = XFS_BHVTOI(bdp);

	dolock = (vrwlock == VRWLOCK_NONE);
	if (dolock) {
		xfs_ilock(ip, XFS_IOLOCK_EXCL);
		vrwlock = VRWLOCK_WRITE;
	}

	ASSERT(vrwlock == VRWLOCK_READ ? ismrlocked(&ip->i_iolock, MR_ACCESS) :
		ismrlocked(&ip->i_iolock, MR_UPDATE));

	error = fs_frlock2(bdp, cmd, flockp, flag, offset, vrwlock,
			credp, ioflag);
	if (dolock)
		xfs_iunlock(ip, XFS_IOLOCK_EXCL);
	return error;
}

static int
xfs_checklock(
	bhv_desc_t	*bdp,
	vnode_t		*vp,
	int		iomode,
	off_t		offset,
	off_t		len,
	int		fmode,
	cred_t		*cr,
	flid_t		*fl,
	vrwlock_t	vrwlock,
	int		ioflag)
{
	struct flock	bf;
	int		cmd, error;

	if (!fl)
		return 0;
	bf.l_type = (iomode & FWRITE) ? F_WRLCK : F_RDLCK;
	bf.l_whence = 0;
	bf.l_start = offset;
	bf.l_len = len;
	bf.l_pid = fl->fl_pid;
	bf.l_sysid = fl->fl_sysid;

	if (fmode & (FNDELAY|FNONBLOCK))
		cmd = F_CHKLK;
	else
		cmd = F_CHKLKW;

	VN_BHV_READ_LOCK(&vp->v_bh);       
	error = xfs_frlock2(bdp, cmd, &bf, 0, offset, vrwlock, 
							cr, ioflag);
	VN_BHV_READ_UNLOCK(&vp->v_bh);     

	if (!error && bf.l_type != F_UNLCK)
		error = EAGAIN;

	return error;
}
#endif

xfs_ioops_t	xfs_iocore_xfs = {
#ifndef SIM
	(xfs_dio_write_t) xfs_dio_write,
	(xfs_dio_read_t) xfs_dio_read,
	(xfs_strat_write_t) xfs_strat_write,
#endif
	(xfs_bmapi_t) xfs_bmapi,
	(xfs_bmap_eof_t) xfs_bmap_eof,
	(xfs_rsync_t) xfs_rsync_fn,
	(xfs_lck_map_shared_t) xfs_ilock_map_shared,
	(xfs_lock_t) xfs_ilock,
#ifndef SIM
	(xfs_lock_demote_t) xfs_ilock_demote,
#endif
	(xfs_lock_nowait_t) xfs_ilock_nowait,
	(xfs_unlk_t) xfs_iunlock,
	(xfs_chgtime_t) xfs_ichgtime,
	(xfs_size_t) xfs_size_fn,
	(xfs_setsize_t) xfs_setsize_fn,
	(xfs_lastbyte_t) xfs_file_last_byte,
#ifndef SIM
	(xfs_checklock_t) xfs_checklock
#endif
};

void
xfs_iocore_inode_init(
	xfs_inode_t	*ip)
{
	xfs_iocore_t	*io = &ip->i_iocore;
	xfs_mount_t	*mp = ip->i_mount;

	io->io_mount = mp;
	io->io_lock = &ip->i_lock;
	io->io_iolock = &ip->i_iolock;
	mutex_init(&io->io_rlock, MUTEX_DEFAULT, "xfs_rlock");

	xfs_iocore_reset(io);

	io->io_obj = (void *)ip;

	io->io_flags = XFS_IOCORE_ISXFS;
	if (ip->i_d.di_flags & XFS_DIFLAG_REALTIME) {
		io->io_flags |= XFS_IOCORE_RT;
	}

	io->io_dmevmask = ip->i_d.di_dmevmask;
	io->io_dmstate = ip->i_d.di_dmstate;
}

void
xfs_iocore_reset(
	xfs_iocore_t	*io)
{
	xfs_mount_t	*mp = io->io_mount;

	/*
	 * initialize read/write io sizes
	 */
	ASSERT(mp->m_readio_log <= 0xff);
	ASSERT(mp->m_writeio_log <= 0xff);

	io->io_readio_log = (uchar_t) mp->m_readio_log;
	io->io_writeio_log = (uchar_t) mp->m_writeio_log;
	io->io_max_io_log = (uchar_t) mp->m_writeio_log;
	io->io_readio_blocks = mp->m_readio_blocks;
	io->io_writeio_blocks = mp->m_writeio_blocks;
}

void
xfs_iocore_destroy(
	xfs_iocore_t	*io)
{
	mutex_destroy(&io->io_rlock);
}

