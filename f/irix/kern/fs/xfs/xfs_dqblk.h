#ifndef _XFS_DQBLK_H_
#define _XFS_DQBLK_H_

#ident "$Revision: 1.2 $"

/*
 * The ondisk form of a dquot structure.
 */
#define XFS_DQUOT_MAGIC	 	0x4451	 	/* 'DQ' */
#define XFS_DQUOT_VERSION	(u_int8_t)0x01	/* latest version number */

/* 
 * This is the main portion of the on-disk representation of quota 
 * information for a user. This is the q_core of the xfs_dquot_t that
 * is kept in kernel memory. We pad this with some more expansion room
 * to construct the on disk structure.
 */
typedef struct	xfs_disk_dquot {
/*16*/	u_int16_t	d_magic;	/* dquot magic = XFS_DQUOT_MAGIC */
/*8 */	u_int8_t	d_version;	/* dquot version */
/*8 */	u_int8_t	d_flags;	/* XFS_DQ_USER/DQ_PROJ */
/*32*/	xfs_dqid_t	d_id;		/* user id or proj id */
/*64*/	xfs_qcnt_t	d_blk_hardlimit;/* absolute limit on disk blks */
/*64*/	xfs_qcnt_t	d_blk_softlimit;/* preferred limit on disk blks */
/*64*/	xfs_qcnt_t	d_ino_hardlimit;/* maximum # allocated inodes */
/*64*/	xfs_qcnt_t	d_ino_softlimit;/* preferred inode limit */
/*64*/	xfs_qcnt_t	d_bcount;	/* disk blocks owned by the user */
/*64*/	xfs_qcnt_t	d_icount;	/* inodes owned by the user */
/*32*/	__int32_t	d_itimer;	/* zero if within inode limits if not, 
					   this is when we refuse service */
/*32*/	__int32_t	d_btimer;	/* similar to above; for disk blocks */
/*16*/	xfs_qwarncnt_t  d_iwarns;       /* warnings issued wrt num inodes */
/*16*/	xfs_qwarncnt_t  d_bwarns;       /* warnings issued wrt disk blocks */
/*32*/	__int32_t	d_pad0;		/* 64 bit align */
/*64*/	xfs_qcnt_t	d_rtb_hardlimit;/* absolute limit on realtime blks */
/*64*/	xfs_qcnt_t	d_rtb_softlimit;/* preferred limit on RT disk blks */
/*64*/	xfs_qcnt_t	d_rtbcount;	/* realtime blocks owned */
/*32*/	__int32_t	d_rtbtimer;	/* similar to above; for RT disk blocks */
/*16*/	xfs_qwarncnt_t  d_rtbwarns;     /* warnings issued wrt RT disk blocks */
/*16*/	__uint16_t	d_pad;
} xfs_disk_dquot_t;

/*
 * This is what goes on disk. This is separated from the xfs_disk_dquot because
 * carrying the unnecessary padding would be a waste of memory.
 */
typedef struct xfs_dqblk {
	xfs_disk_dquot_t  dd_diskdq;	/* portion that lives incore as well */
	char              dd_fill[32];	/* filling for posterity */
} xfs_dqblk_t;

/*
 * flags for q_flags field in the dquot.
 */
#define XFS_DQ_USER	 	0x0001		/* a user quota */
#define XFS_DQ_PROJ	 	0x0002		/* a project quota */

#define XFS_DQ_FLOCKED		0x0008		/* flush lock taken */
#define XFS_DQ_DIRTY		0x0010		/* dquot is dirty */
#define XFS_DQ_WANT		0x0020		/* for lookup/reclaim race */
#define XFS_DQ_INACTIVE		0x0040		/* dq off mplist & hashlist */
#define XFS_DQ_MARKER		0x0080		/* sentinel */

/*
 * In the worst case, when both user and proj quotas on,
 * we can have a max of three dquots changing in a single transaction.
 */
#define XFS_DQUOT_LOGRES(mp)	(sizeof(xfs_disk_dquot_t) * 3)

#endif
