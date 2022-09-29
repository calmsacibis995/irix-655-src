#ident "$Revision: 1.1 $"

/*
 * Implementation of CXFS RPC log items -- needed to implement
 * atomic rpc's for CXFS
 */

#ifdef SIM
#define _KERNEL 1
#endif

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/uuid.h>
#include <sys/debug.h>

#ifdef SIM
#undef _KERNEL
#endif

#include "xfs_macros.h"
#include "xfs_types.h"
#include "xfs_inum.h"
#include "xfs_log.h"
#include "xfs_trans.h"
#include "xfs_sb.h"
#include "xfs_dir.h"
#include "xfs_mount.h"
#include "xfs_rpc_item.h"

zone_t *xfs_rpcli_zone;		/* rpc log item zone */

/*
 * returns the number of iovecs needed to log the rpc.
 *
 * the number is always 2 -- one for the log format structure
 * and the second to log the return results
 */
/* ARGSUSED */
uint
xfs_rpc_item_size(xfs_rpc_log_item_t *rpcip)
{
	return 2;
}

/*
 * Called to fill in the the vector of log iovecs with
 * the data to be logged.  The first vector will be filled
 * with the log format structure, the second with the return
 * results.
 */
void
xfs_rpc_item_format(xfs_rpc_log_item_t *rpcip, xfs_log_iovec_t *logvec)
{
	xfs_log_iovec_t *vec = logvec;

	rpcip->rpcli_format.rlf_type = XFS_LI_RPC;

	/*
	 * first logvec contains everything but the data union
	 */
	vec->i_addr = (caddr_t) &rpcip->rpcli_format;
	vec->i_len = sizeof(cxfs_rpc_log_format_t);
	vec++;

	/*
	 * second logvec contains the contents of the data union
	 */
	vec->i_addr = (caddr_t) &rpcip->rpcli_data.rpcli_base;
	vec->i_len = rpcip->rpcli_format.rlf_size;
}

/*
 * Pin the rpc item to memory.  The rpc return results cannot 
 * be flushed back to the client until the rpc has been first
 * logged to disk or until the rpc is safely replicated in the
 * shadow logs.  The log pin/unpin calls and the shadow log
 * pin/unpin calls use the same fundamental field (a 64-bit
 * bitmask) with the disk being the low bit (bit 0).
 *
 * pin clears all bits.  The bits get set when the data has
 * the corresponding shadow or disk.
 */
void
xfs_rpc_item_pin(xfs_rpc_log_item_t *rpcip)
{
	int s;

	s = mutex_spinlock(&rpcip->rpcli_lock);
	rpcip->rpcli_pinmask = 0;
	mutex_spinunlock(&rpcip->rpcli_lock, s);

	return;
}

/*
 * Unpin the rpc item.  Drop the disk-based pin because the
 * item has hit the disk (sets the low-order bit high).
 */
void
xfs_rpc_item_unpin(xfs_rpc_log_item_t *rpcip)
{
	int s;

	s = mutex_spinlock(&rpcip->rpcli_lock);
	rpcip->rpcli_pinmask |= 0x1;
	mutex_spinunlock(&rpcip->rpcli_lock, s);

	return;
}

/*
 * Same as above for aborting dirty transactions (typically just
 * before shutting down the filesystem).
 */
void
xfs_rpc_item_unpin_remove(xfs_rpc_log_item_t *rpcip)
{
	int s;

	s = mutex_spinlock(&rpcip->rpcli_lock);
	rpcip->rpcli_pinmask |= 0x1;
	mutex_spinunlock(&rpcip->rpcli_lock, s);

	return;
}

/*
 * Non-blocking lock routine for tail-pushing.  I *think* we
 * can make this a spinlock that actually waits a split-second
 * since no one should be holding the rpc item lock for very long.
 * So this routine actually does nothing but return success since
 * we know we're going to be able to lock the item in the push routine.
 */
/* ARGSUSED */
uint
xfs_rpc_item_trylock(xfs_rpc_log_item_t *rpcip)
{
	return XFS_ITEM_SUCCESS;
}

/*
 * Unlock the item after the transaction has committed to
 * the incore log buffer.  This routine does nothing since
 * there is no long-term locking on the rpc/rpc-item.  There
 * is one rpc-item per rpc and all rpc's are single-threaded.
 */
/* ARGSUSED */
void
xfs_rpc_item_unlock(xfs_rpc_log_item_t *rpcip)
{
	return;
}

/*
 * return the lsn that the transaction system should
 * think was last committed for the rpc.  For rpc's,
 * it's the given lsn (see buffer items for a case
 * where we don't use the given lsn).
 */
/* ARGSUSED */
xfs_lsn_t
xfs_rpc_item_committed( xfs_rpc_log_item_t *rpcip, xfs_lsn_t lsn)
{
	return lsn;
}

/*
 * abort - not used right now since it's not called.
 */
/* ARGSUSED */
void
xfs_rpc_item_abort(xfs_rpc_log_item_t *rpcip)
{
	return;
}

/*
 * push - rpc the logged info back to the client
 * so we can pull the item from the AIL.  We don't want
 * make it a synchronous RPC since we don't want to wait
 * too long here.  However, for simplicity's sake, I'm
 * willing to do that for now since we expect this to be
 * a rarely (if ever) executed code path.
 */
/* ARGSUSED */
void
xfs_rpc_item_push(xfs_rpc_log_item_t *rpcip)
{
	/*
	 * XXX - do nothing right now.  Later this will
	 * have to lock the rpc log item, snapshot the
	 * results into a buffer, drop the lock, and send
	 * the buffer.  After the rpc is received by the
	 * client, then pull the item from AIL.  Either that,
	 * or let the async data transmission completion callout
	 * pull the item from the AIL.
	 */
	return;
}

/* the pushbuf op is NULL */

/*
 * committing - does nothing since the rpc is in and of itself not
 * a metadata object that another rpc will see depend on.
 */
/* ARGSUSED */
void
xfs_rpc_item_committing( xfs_rpc_log_item_t *rpcip, xfs_lsn_t lsn)
{
	return;
}

struct xfs_item_ops xfs_rpc_item_ops = {
	(uint(*)(xfs_log_item_t*))xfs_rpc_item_size,
	(void(*)(xfs_log_item_t*, xfs_log_iovec_t*))xfs_rpc_item_format,
	(void(*)(xfs_log_item_t*))xfs_rpc_item_pin,
	(void(*)(xfs_log_item_t*))xfs_rpc_item_unpin,
	(void(*)(xfs_log_item_t*, xfs_trans_t*))xfs_rpc_item_unpin_remove,
	(uint(*)(xfs_log_item_t*))xfs_rpc_item_trylock,
	(void(*)(xfs_log_item_t*))xfs_rpc_item_unlock,
	(xfs_lsn_t(*)(xfs_log_item_t*, xfs_lsn_t))xfs_rpc_item_committed,
	(void(*)(xfs_log_item_t*))xfs_rpc_item_push,
	(void(*)(xfs_log_item_t*))xfs_rpc_item_abort,
	NULL,
	(void(*)(xfs_log_item_t*, xfs_lsn_t))xfs_rpc_item_committing
};

xfs_rpc_log_item_t *
xfs_rpc_item_init(
	xfs_mount_t	*mp,
	uint		rpctype,
	cell_t		caller,
	struct chandle	*handle,
	uuid_t		caller_uuid)
{
	xfs_rpc_log_item_t *rpcip;

	rpcip = kmem_zone_alloc(xfs_rpcli_zone, KM_SLEEP);

	/*
	 * initialize base fields in embedded XFS log item
	 */
	rpcip->rpcli_item.li_type = XFS_LI_RPC;
	rpcip->rpcli_item.li_ops = &xfs_rpc_item_ops;
	rpcip->rpcli_item.li_mountp = mp;

	spinlock_init(&rpcip->rpcli_lock, "XFS rpc log item");
	rpcip->rpcli_handle = handle;	/* XXX might need to dup them instead */
	rpcip->rpcli_cell = caller;

	rpcip->rpcli_format.rlf_op = rpctype;
	rpcip->rpcli_format.rlf_cellid = caller_uuid;

	/*
	 * XXX rcc - probably more here ...
	 */

	return rpcip;
}

/*
 * XXX rcc - tear an rpc log item down
 */
/* ARGSUSED */
void
xfs_rpc_item_destroy(xfs_rpc_log_item_t *rpcip)
{
	return;
}
