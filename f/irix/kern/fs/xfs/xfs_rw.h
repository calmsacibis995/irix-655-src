#ifndef	_XFS_RW_H
#define	_XFS_RW_H

#ident "$Revision: 1.43 $"

struct bhv_desc;
struct bdevsw;
struct bmapval;
struct buf;
struct cred;
struct flid;
struct uio;
struct vnode;
struct xfs_inode;
struct xfs_iocore;
struct xfs_mount;
struct xfs_trans;
struct xfs_dio;
struct pm;

/*
 * used for mmap i/o page lockdown code
 */
typedef struct xfs_uaccmap {
	uvaddr_t		xfs_uacstart;
	__psunsigned_t		xfs_uaclen;
} xfs_uaccmap_t;

/*
 * Maximum count of bmaps used by read and write paths.
 */
#define	XFS_MAX_RW_NBMAPS	4

/*
 * Counts of readahead buffers to use based on physical memory size.
 * None of these should be more than XFS_MAX_RW_NBMAPS.
 */
#define	XFS_RW_NREADAHEAD_16MB	2
#define	XFS_RW_NREADAHEAD_32MB	3
#define	XFS_RW_NREADAHEAD_K32	4
#define	XFS_RW_NREADAHEAD_K64	4

/*
 * Maximum size of a buffer that we\'ll map.  Making this
 * too big will degrade performance due to the number of
 * pages which need to be gathered.  Making it too small
 * will prevent us from doing large I/O\'s to hardware that
 * needs it.
 *
 * This is currently set to 512 KB.
 */
#define	XFS_MAX_BMAP_LEN_BB	1024
#define	XFS_MAX_BMAP_LEN_BYTES	524288

/*
 * Convert the given file system block to a disk block.
 * We have to treat it differently based on whether the
 * file is a real time file or not, because the bmap code
 * does.
 */
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_FSB_TO_DB)
daddr_t xfs_fsb_to_db(struct xfs_inode *ip, xfs_fsblock_t fsb);
#define	XFS_FSB_TO_DB(ip,fsb)	xfs_fsb_to_db(ip,fsb)
#else
#define	XFS_FSB_TO_DB(ip,fsb) \
		(((ip)->i_d.di_flags & XFS_DIFLAG_REALTIME) ? \
		 (daddr_t)XFS_FSB_TO_BB((ip)->i_mount, (fsb)) : \
		 XFS_FSB_TO_DADDR((ip)->i_mount, (fsb)))
#endif

#define XFS_FSB_TO_DB_IO(io,fsb) \
		(((io)->io_flags & XFS_IOCORE_RT) ? \
		 XFS_FSB_TO_BB((io)->io_mount, (fsb)) : \
		 XFS_FSB_TO_DADDR((io)->io_mount, (fsb)))

#define	xfs_bdwrite(mp, bp) \
          { ((bp)->b_vp == NULL) ? (bp)->b_bdstrat = xfs_bdstrat_cb: 0; \
		    (bp)->b_fsprivate3 = (mp); bdwrite(bp);}
#define	xfs_bawrite(mp, bp) \
	  { ((bp)->b_vp == NULL) ? (bp)->b_bdstrat = xfs_bdstrat_cb: 0; \
		    (bp)->b_fsprivate3 = (mp); bawrite(bp);}
/*
 * Defines for the trace mechanisms in xfs_rw.c.
 */
#define	XFS_RW_KTRACE_SIZE	64
#define	XFS_STRAT_KTRACE_SIZE	64
#define	XFS_STRAT_GTRACE_SIZE	512

#define	XFS_READ_ENTER		1
#define	XFS_WRITE_ENTER		2
#define XFS_IOMAP_READ_ENTER	3
#define	XFS_IOMAP_WRITE_ENTER	4
#define	XFS_IOMAP_READ_MAP	5
#define	XFS_IOMAP_WRITE_MAP	6
#define	XFS_IOMAP_WRITE_NOSPACE	7
#define	XFS_ITRUNC_START	8
#define	XFS_ITRUNC_FINISH1	9
#define	XFS_ITRUNC_FINISH2	10
#define	XFS_CTRUNC1		11
#define	XFS_CTRUNC2		12
#define	XFS_CTRUNC3		13
#define	XFS_CTRUNC4		14
#define	XFS_CTRUNC5		15
#define	XFS_CTRUNC6		16     
#define	XFS_BUNMAPI		17
#define	XFS_INVAL_CACHED	18

#define	XFS_STRAT_ENTER		1
#define	XFS_STRAT_FAST		2
#define	XFS_STRAT_SUB		3
#define	XFS_STRAT_UNINT		4
#define	XFS_STRAT_UNINT_DONE	5
#define	XFS_STRAT_UNINT_CMPL	6

#if defined(XFS_ALL_TRACE)
#define	XFS_RW_TRACE
#define	XFS_STRAT_TRACE
#endif

#if !defined(DEBUG) || defined(SIM)
#undef XFS_RW_TRACE
#undef XFS_STRAT_TRACE
#endif

/*
 * Prototypes for functions in xfs_rw.c.
 */
int
xfs_read(struct bhv_desc	*bdp,
	 struct uio		*uiop,
	 int			ioflag,
	 struct cred		*credp,
	 struct flid		*fl);

int
xfs_vop_readbuf(bhv_desc_t 	*bdp,
		off_t		offset,
		ssize_t		len,
		int		ioflags,
		struct cred	*creds,
		struct flid	*fl,
		struct buf	**rbuf,
		int		*pboff,
		int		*pbsize);

int
xfs_write_clear_setuid(
	struct xfs_inode	*ip);

int
xfs_write(struct bhv_desc	*bdp,
	  struct uio		*uiop,
	  int			ioflag,
	  struct cred		*credp,
	  struct flid		*fl);

int 
xfs_bwrite(
	struct xfs_mount 	*mp,
	struct buf		*bp);
int
xfsbdstrat(
	struct xfs_mount 	*mp,
	struct buf		*bp);

void
xfs_strategy(struct bhv_desc	*bdp,
	     struct buf		*bp);

void
xfs_strat_write_iodone(struct buf *bp);

int
xfs_bmap(struct bhv_desc	*bdp,
	 off_t			offset,
	 ssize_t		count,
	 int			flags,
	 struct cred		*credp,
	 struct bmapval		*bmapp,
	 int			*nbmaps);

int
xfs_zero_eof(vnode_t		*vp,
	     struct xfs_iocore	*io,
	     off_t		offset,
	     xfs_fsize_t	isize,
	     struct cred	*credp,
	     struct pm		*pmp);

void
xfs_inval_cached_pages(
	struct vnode		*vp,
	struct xfs_iocore	*io,
	off_t			offset,
	off_t			len,
	void			*dio);

void
xfs_refcache_insert(
	struct xfs_inode	*ip);

void
xfs_refcache_purge_ip(
	struct xfs_inode	*ip);

void
xfs_refcache_purge_mp(
	struct xfs_mount	*mp);

void
xfs_refcache_purge_some(void);

int
xfs_bioerror(struct buf *b);

#ifndef SIM
/*
 * XFS I/O core functions
 */

struct vnmap;

extern int xfs_dio_read(struct xfs_dio *);
extern int xfs_dio_write(struct xfs_dio *);
extern int xfs_read_core(bhv_desc_t *, struct xfs_iocore *, uio_t *, int,
                        struct cred *, struct flid *, int,
			struct vnmap *, int, const uint,
			xfs_uaccmap_t *, xfs_fsize_t, int,
			vrwlock_t *);
extern int xfs_diostrat(struct buf *);
extern int xfs_diordwr(bhv_desc_t *, struct xfs_iocore *, uio_t *, int,
                        struct cred *, uint64_t, off_t *, size_t *);
extern int xfs_strat_read(struct xfs_iocore *, struct buf *);
extern int xfs_strat_write(struct xfs_iocore *, struct buf *);
extern int xfs_strat_write_core(struct xfs_iocore *, struct buf *, int);
extern int xfs_iomap_read(struct xfs_iocore *, off_t, size_t, struct bmapval *,
                        int *, struct pm *, int *, unsigned int);
extern int xfs_bioerror_relse(struct buf *);
extern void xfs_strat_core(struct xfs_iocore *, struct buf *);
extern int xfs_write_file(bhv_desc_t *, struct xfs_iocore *, uio_t *, int,
			 struct cred *, xfs_lsn_t *, struct vnmap *, int,
			 const uint, xfs_uaccmap_t *);
extern int xfs_strat_write_unwritten(struct xfs_iocore *, struct buf *);
extern int xfs_check_mapped_io(struct vnode *, uio_t *, struct vnmap**,
			int *, int *, int *, xfs_fsize_t *, xfs_uaccmap_t **);
extern int xfs_is_nested_locking_enabled(void);
extern void xfs_enable_nested_locking(void);
extern void xfs_disable_nested_locking(void);
#endif


void
xfs_xfsd_list_evict(bhv_desc_t *bdp);

/*
 * Needed by xfs_rw.c
 */
void
xfs_rwlock(
	bhv_desc_t	*bdp,
	vrwlock_t	write_lock);

void
xfs_rwlockf(
	bhv_desc_t	*bdp,
	vrwlock_t	write_lock,
	int		flags);

void
xfs_rwunlock(
	bhv_desc_t	*bdp,
	vrwlock_t	write_lock);

void
xfs_rwunlockf(
	bhv_desc_t	*bdp,
	vrwlock_t	write_lock,
	int		flags);

int
xfs_read_buf(
	struct xfs_mount *mp,
	buftarg_t	 *target,
        daddr_t 	 blkno,
        int              len,
        uint             flags,
	struct buf	 **bpp);

int
xfs_bdstrat_cb(struct buf *bp);

void
xfs_ioerror_alert(
	char 			*func,
	struct xfs_mount	*mp,
	dev_t			dev,
	daddr_t			blkno);
	  
#endif /* _XFS_RW_H */
