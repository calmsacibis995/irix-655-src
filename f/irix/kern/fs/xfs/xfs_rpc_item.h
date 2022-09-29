
/*
 * CXFS RPC Log Format - data structures for logging RPC ops
 */

typedef uuid_t cxfs_rpcid_t;	/* XXX rcc - will have to be moved later */
struct chandle;

/*
 * base logging data for all ops (so far)
 *
 *	- log anticipated errno/return result code
 *	- the vattr and size can be determined from the inode
 *		core logged with the transaction.  The extent
 *		list modifications, if any, can be determined after
 *		the truncate operation is rolled forwards or immediately
 *		if it's a grow operation.
 *	- the fsid/inode number is logged so that it will can sent back to
 *		the client if the log is wrapped before the rpc returns.
 *		that way, recovery knows that an rpc for that inode
 *		was in progress and committed.  the fsid/inum should
 *		be enough to let recovery find the correct inode
 *		during recovery.
 *
 * note:  may also need to log something about the tokens requested.
 */
typedef struct cxfs_base_results {
	int			rlf_result;	/* returned errno */
	xfs_ino_t		rlf_inum;	/* inode created/operated on */
	uuid_t			rlf_fsid;	/* unique filesystem id */
} cxfs_base_results_t;

/*
 * actual structure that goes into the log.  Size matters here since
 * every byte in the structure gets written to disk.  This structure
 * really ought to be packed (no holes).
 */
typedef struct cxfs_rpc_log_format {
	unsigned short		rlf_type;	/* rpc log item type */
	unsigned short		rlf_size;	/* used portion of rpcli_data */
	uint			rlf_op;		/* rpc operation being logged */
	uuid_t			rlf_cellid;	/* callers UUID */
	cxfs_rpcid_t		rlf_rpcid;	/* unique rpc id */
} cxfs_rpc_log_format_t;

/*
 * this item is attached to the xtinfo structure to log data during the
 * rpc.  The log item is attached to the transaction when xfs_trans_rpc_join()
 * is called and added into the AIL when the transaction commit record
 * hits the on-disk log.  Once the principal transaction for an RPC commits,
 * a flag is set preventing further logging activity from happening.  The
 * item still needs to be present so that the refcount can be decremented
 * when the RPC returns to the client.  The refcount is also dropped when
 * the item is removed from the AIL.  The item is deleted when the refcount
 * hits zero.
 *
 * Pinning - the item is pinned in memory once the rpc is committed to
 * the incore log buffer (iclogbufs).  The item cannot be unpinned
 * until either the item has been committed to disk or the item
 * is safely replicated on all of the shadow logs (to guard against
 * multiple failures of both the primary servers and one or more shadows).
 * The rpc info in the item cannot be flushed back to the client until
 * the item is unpinned.  The pinmask is a 64-bit mask - one for each
 * shadow and one bit (bit 0) for the disk.  The value of the shadow
 * portion of the pinmask is a mount field in the xfs mount structure
 * that is set by CXFS.
 * 
 * A note on locking.  There should be little if no contention on this
 * structure since every rpc gets its own rpc log item.  The only contention
 * is between the executing rpc and the shadow system (if stuff gets flushed
 * to the client) or the transaction system and the shadow log system after
 * the rpc is committed.  In either case, the locking is necessary only
 * for very short-term mutual exclusion necessary to move the item or
 * set related fields atomically, etc.  Hence this is protected only by
 * a spinlock.
 */
typedef struct xfs_rpc_log_item {
	xfs_log_item_t		rpcli_item;	/* embedded generic log item */
	lock_t			rpcli_lock;	/* primary short-term lock */
	uint			rpcli_flags;	/* flags */
	int			rpcli_refcount;	/* reference count */
	struct chandle		*rpcli_handle;	/* rpc handle of caller */
	cell_t			rpcli_cell;	/* caller's cell ID */
	xfs_mount_t		*rcpli_mp;	/* XFS fs mount structure */
	uint64_t		rpcli_pinmask;	/* for pinning in-core */
	cxfs_rpc_log_format_t	rpcli_format;	/* actual data logged */
	union {					/* operation specific data */
		cxfs_base_results_t	rpcli_base;
	} rpcli_data;
#ifdef DEBUG
	struct ktrace		*rpcli_trace;	/* ktrace buffer */
#endif
} xfs_rpc_log_item_t;

/*
 * RPC log item flags
 */

#define XFS_RPCLI_DONE		/* p-xaction committed, no more logging */

/*
 * logged RPC operations - note:  not logging fsync.  I think it's
 * effectively idempotent.  Also not logging bmap since I'm almost
 * certain it's idempotent.
 */

#define	RPCLI_OP_CREATE		1
#define RPCLI_OP_SETATTR	2
#define RPCLI_OP_REMOVE		3
#define RPCLI_OP_RENAME		4
#define RPCLI_OP_RMDIR		5
#define RPCLI_OP_LINK		6
#define RPCLI_OP_SYMLINK	7
#define RPCLI_OP_ALLOCSTORE	8
#define RPCLI_OP_ATTR_SET	9
#define RPCLI_OP_ATTR_REMOVE	10
#define RPCLI_OP_FCNTL		11	/* for F_FSSETXATTR */
#define RPCLI_OP_CHANGESP	12	/* ALLOCSP/RESVP/FREESP/UNRESVP fcntl */

