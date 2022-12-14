#ident "$Revision: 1.71 $"

/*
 * This file contains the implementation of the xfs_buf_log_item.
 * It contains the item operations used to manipulate the buf log
 * items as well as utility routines used by the buffer specific
 * transaction routines.
 */

#include <limits.h>
#ifdef SIM
#define _KERNEL 1
#endif
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/atomic_ops.h>
#include <sys/debug.h>
#ifdef SIM
#undef _KERNEL
#endif
#include <sys/vnode.h>
#include <sys/kmem.h>
#include <sys/errno.h>
#ifdef SIM
#include <bstring.h>
#include <stdio.h>
#else
#include <sys/systm.h>
#endif
#include <sys/ktrace.h>
#include <sys/cmn_err.h>
#include <sys/uuid.h>
#include "xfs_macros.h"
#include "xfs_types.h"
#include "xfs_inum.h"
#include "xfs_log.h"
#include "xfs_trans.h"
#include "xfs_buf_item.h"
#include "xfs_sb.h"
#include "xfs_dir.h"
#include "xfs_mount.h"
#include "xfs_trans_priv.h"
#include "xfs_rw.h" 
#include "xfs_bit.h"

#ifdef SIM
#include "sim.h"
#endif

#define	ROUNDUPNBWORD(x)	(((x) + (NBWORD - 1)) & ~(NBWORD - 1))

zone_t	*xfs_buf_item_zone;

#if 0
STATIC void	xfs_buf_item_set_bit(uint *, uint, uint);
#endif

#ifdef	XFS_TRANS_DEBUG
STATIC void
xfs_buf_item_log_debug(
	xfs_buf_log_item_t	*bip,
	uint			first,
	uint			last);

STATIC void
xfs_buf_item_log_check(
	xfs_buf_log_item_t	*bip);
#else
#define		xfs_buf_item_log_debug(x,y,z)
#define 	xfs_buf_item_log_check(x)
#endif

STATIC void	xfs_buf_error_relse(buf_t *bp);

/*
 * This returns the number of log iovecs needed to log the
 * given buf log item.
 *
 * It calculates this as 1 iovec for the buf log format structure
 * and 1 for each stretch of non-contiguous chunks to be logged.
 * Contiguous chunks are logged in a single iovec.
 *
 * If the XFS_BLI_STALE flag has been set, then log nothing.
 */
uint
xfs_buf_item_size(
	xfs_buf_log_item_t	*bip)
{
	uint	nvecs;
	int	next_bit;
	int	last_bit;

	ASSERT(bip->bli_refcount > 0);
	if (bip->bli_flags & XFS_BLI_STALE) {
		/*
		 * The buffer is stale, so all we need to log
		 * is the buf log format structure with the
		 * cancel flag in it.
		 */
		xfs_buf_item_trace("SIZE STALE", bip);
		ASSERT(bip->bli_format.blf_flags & XFS_BLI_CANCEL);
		return 1;
	}

	ASSERT(bip->bli_flags & XFS_BLI_LOGGED);
	nvecs = 1;
	last_bit = xfs_buf_item_next_bit(bip->bli_format.blf_data_map,
					 bip->bli_format.blf_map_size, 0);
	ASSERT(last_bit != -1);
	nvecs++;
	while (last_bit != -1) {	
		/*
		 * This takes the bit number to start looking from and
		 * returns the next set bit from there.  It returns -1
		 * if there are no more bits set or the start bit is
		 * beyond the end of the bitmap.
		 */
		next_bit = xfs_buf_item_next_bit(bip->bli_format.blf_data_map,
						 bip->bli_format.blf_map_size,
						 last_bit + 1);
		/*
		 * If we run out of bits, leave the loop,
		 * else if we find a new set of bits bump the number of vecs,
		 * else keep scanning the current set of bits.
		 */
		if (next_bit == -1) {
			last_bit = -1;
		} else if (next_bit != last_bit + 1) {
			last_bit = next_bit;	
			nvecs++;
		} else {
			last_bit++;
		}
	}

	xfs_buf_item_trace("SIZE NORM", bip);
	return nvecs;
}

/*
 * This is called to fill in the vector of log iovecs for the
 * given log buf item.  It fills the first entry with a buf log
 * format structure, and the rest point to contiguous chunks
 * within the buffer.
 */
void
xfs_buf_item_format(
	xfs_buf_log_item_t	*bip,
	xfs_log_iovec_t		*log_vector)
{
	uint		base_size;
	uint		nvecs;
	xfs_log_iovec_t	*vecp;
	buf_t		*bp;
	int		first_bit;
	int		last_bit;
	int		next_bit;
	uint		nbits;
	uint		buffer_offset;

	ASSERT(bip->bli_refcount > 0);
	ASSERT((bip->bli_flags & XFS_BLI_LOGGED) ||
	       (bip->bli_flags & XFS_BLI_STALE));
	bp = bip->bli_buf;
	ASSERT(BP_ISMAPPED(bp));
	vecp = log_vector;

	/*
	 * The size of the base structure is the size of the
	 * declared structure plus the space for the extra words
	 * of the bitmap.  We subtract one from the map size, because
	 * the first element of the bitmap is accounted for in the
	 * size of the base structure.
	 */
	base_size =
		(uint)(sizeof(xfs_buf_log_format_t) +
		       ((bip->bli_format.blf_map_size - 1) * sizeof(uint)));
	vecp->i_addr = (caddr_t)&bip->bli_format;
	vecp->i_len = base_size;
	vecp++;
	nvecs = 1;

	if (bip->bli_flags & XFS_BLI_STALE) {
		/*
		 * The buffer is stale, so all we need to log
		 * is the buf log format structure with the
		 * cancel flag in it.
		 */
		xfs_buf_item_trace("FORMAT STALE", bip);
		ASSERT(bip->bli_format.blf_flags & XFS_BLI_CANCEL);
		bip->bli_format.blf_size = nvecs;
		return;
	}

	/*
	 * Fill in an iovec for each set of contiguous chunks.
	 */
	first_bit = xfs_buf_item_next_bit(bip->bli_format.blf_data_map,
					 bip->bli_format.blf_map_size, 0);
	ASSERT(first_bit != -1);
	last_bit = first_bit;
	nbits = 1;
	for (;;) {
		/*
		 * This takes the bit number to start looking from and
		 * returns the next set bit from there.  It returns -1
		 * if there are no more bits set or the start bit is
		 * beyond the end of the bitmap.
		 */
		next_bit = xfs_buf_item_next_bit(bip->bli_format.blf_data_map,
						 bip->bli_format.blf_map_size,
						 (uint)last_bit + 1);
		/*
		 * If we run out of bits fill in the last iovec and get
		 * out of the loop.
		 * Else if we start a new set of bits then fill in the
		 * iovec for the series we were looking at and start
		 * counting the bits in the new one.
		 * Else we're still in the same set of bits so just
		 * keep counting and scanning.
		 */
		if (next_bit == -1) {
			buffer_offset = first_bit * XFS_BLI_CHUNK;
			vecp->i_addr = bp->b_un.b_addr + buffer_offset;
			vecp->i_len = nbits * XFS_BLI_CHUNK;
			nvecs++;
			break;
		} else if (next_bit != last_bit + 1) {
			buffer_offset = first_bit * XFS_BLI_CHUNK;
			vecp->i_addr = bp->b_un.b_addr + buffer_offset;
			vecp->i_len = nbits * XFS_BLI_CHUNK;
			nvecs++;
			vecp++;
			first_bit = next_bit;
			last_bit = next_bit;	
			nbits = 1;
		} else {
			last_bit++;
			nbits++;
		}
	}
	bip->bli_format.blf_size = nvecs;

	/*
	 * Check to make sure everything is consistent.
	 */
	xfs_buf_item_trace("FORMAT NORM", bip);
	xfs_buf_item_log_check(bip);
}

/*
 * This is called to pin the buffer associated with the buf log
 * item in memory so it cannot be written out.  Simply call bpin()
 * on the buffer to do this.
 */
void
xfs_buf_item_pin(
	xfs_buf_log_item_t	*bip)
{
	buf_t	*bp;

	bp = bip->bli_buf;
	ASSERT(bp->b_flags & B_BUSY);
	ASSERT(bip->bli_refcount > 0);
	ASSERT((bip->bli_flags & XFS_BLI_LOGGED) ||
	       (bip->bli_flags & XFS_BLI_STALE));
	xfs_buf_item_trace("PIN", bip);
	buftrace("XFS_PIN", bp);
	bpin(bp);
}


/*
 * This is called to unpin the buffer associated with the buf log
 * item which was previously pinned with a call to xfs_buf_item_pin().
 * Just call bunpin() on the buffer to do this.
 *
 * Also drop the reference to the buf item for the current transaction.
 * If the XFS_BLI_STALE flag is set and we are the last reference,
 * then free up the buf log item and unlock the buffer.
 */
void
xfs_buf_item_unpin(
	xfs_buf_log_item_t	*bip)
{
	xfs_mount_t	*mp;
	buf_t		*bp;
	int		refcount;
	SPLDECL(s);

	bp = bip->bli_buf;
	ASSERT(bp != NULL);
	ASSERT((xfs_buf_log_item_t*)(bp->b_fsprivate) == bip);
	ASSERT(bip->bli_refcount > 0);
	xfs_buf_item_trace("UNPIN", bip);
	buftrace("XFS_UNPIN", bp);

	refcount = atomicAddInt(&bip->bli_refcount, -1);
	mp = bip->bli_item.li_mountp;
	bunpin(bp);
	if ((refcount == 0) && (bip->bli_flags & XFS_BLI_STALE)) {
		ASSERT(valusema(&bp->b_lock) <= 0);
		ASSERT(!(bp->b_flags & B_DELWRI));
		ASSERT(bp->b_flags & B_STALE);
		ASSERT(bp->b_pincount == 0);
		ASSERT(bip->bli_format.blf_flags & XFS_BLI_CANCEL);
		xfs_buf_item_trace("UNPIN STALE", bip);
		buftrace("XFS_UNPIN STALE", bp);
		AIL_LOCK(mp,s);
		/*
		 * If we get called here because of an IO error, we may
		 * or may not have the item on the AIL. xfs_trans_delete_ail()
		 * will take care of that situation.
		 * xfs_trans_delete_ail() drops the AIL lock.
		 */
		xfs_trans_delete_ail(mp, (xfs_log_item_t *)bip, s);
		xfs_buf_item_relse(bp);
		ASSERT(bp->b_fsprivate == NULL);
		brelse(bp);
	}

}

/*
 * this is called from uncommit in the forced-shutdown path.
 * we need to check to see if the reference count on the log item
 * is going to drop to zero.  If so, unpin will free the log item
 * so we need to free the item's descriptor (that points to the item)
 * in the transaction.
 */
void
xfs_buf_item_unpin_remove(
	xfs_buf_log_item_t	*bip,
	xfs_trans_t		*tp)
{
	/* REFERENCED */
	buf_t		*bp;
	xfs_log_item_desc_t	*lidp;

	bp = bip->bli_buf;
	/*
	 * will xfs_buf_item_unpin() call xfs_buf_item_relse()?
	 */
	if (bip->bli_refcount == 1 && (bip->bli_flags & XFS_BLI_STALE)) {
		ASSERT(valusema(&bip->bli_buf->b_lock) <= 0);
		xfs_buf_item_trace("UNPIN REMOVE", bip);
		buftrace("XFS_UNPIN_REMOVE", bp);
		/*
		 * yes -- clear the xaction descriptor in-use flag
		 * and free the chunk if required.  We can safely
		 * do some work here and then call buf_item_unpin
		 * to do the rest because if the if is true, then
		 * we are holding the buffer locked so no one else
		 * will be able to bump up the refcount.
		 */
		lidp = xfs_trans_find_item(tp, (xfs_log_item_t *) bip);
		xfs_trans_free_item(tp, lidp);
		/*
		 * Since the transaction no longer refers to the buffer,
		 * the buffer should no longer refer to the transaction.
		 */
		bp->b_fsprivate2 = NULL;
	}

	xfs_buf_item_unpin(bip);

	return;
}

/*
 * This is called to attempt to lock the buffer associated with this
 * buf log item.  Don't sleep on the buffer lock.  If we can't get
 * the lock right away, return 0.  If we can get the lock, pull the
 * buffer from the free list, mark it busy, and return 1.
 */
uint
xfs_buf_item_trylock(
	xfs_buf_log_item_t	*bip)
{
	buf_t	*bp;

	bp = bip->bli_buf;

	if (bp->b_pincount > 0) {
		return XFS_ITEM_PINNED;
	}

	if (!cpsema(&bp->b_lock)) {
		return XFS_ITEM_LOCKED;
	}

	/*
	 * Remove the buffer from the free list.  Only do this
	 * if it's on the free list.  Private buffers like the
	 * superblock buffer are not.
	 */
	if (bp->av_forw != NULL) {
		notavail(bp);
	}

	ASSERT(!(bip->bli_flags & XFS_BLI_STALE));
	xfs_buf_item_trace("TRYLOCK SUCCESS", bip);
	return XFS_ITEM_SUCCESS;
}

/*
 * Release the buffer associated with the buf log item.
 * If there is no dirty logged data associated with the
 * buffer recorded in the buf log item, then free the
 * buf log item and remove the reference to it in the
 * buffer.
 *
 * This call ignores the recursion count.  It is only called
 * when the buffer should REALLY be unlocked, regardless
 * of the recursion count.
 *
 * If the XFS_BLI_HOLD flag is set in the buf log item, then
 * free the log item if necessary but do not unlock the buffer.
 * This is for support of xfs_trans_bhold(). Make sure the
 * XFS_BLI_HOLD field is cleared if we don't free the item.
 */
void
xfs_buf_item_unlock(
	xfs_buf_log_item_t	*bip)
{
	int	aborted;
	buf_t	*bp;
	uint	hold;

	bp = bip->bli_buf;
	buftrace("XFS_UNLOCK", bp);

	/*
	 * Clear the buffer's association with this transaction.
	 */
	bp->b_fsprivate2 = NULL;

	/*
	 * If this is a transaction abort, don't return early.
	 * Instead, allow the brelse to happen.
	 * Normally it would be done for stale (cancelled) buffers
	 * at unpin time, but we'll never go through the pin/unpin
	 * cycle if we abort inside commit.
	 */
	aborted = (bip->bli_item.li_flags & XFS_LI_ABORTED) != 0;

	/*
	 * If the buf item is marked stale, then don't do anything.
	 * We'll unlock the buffer and free the buf item when the
	 * buffer is unpinned for the last time.
	 */
	if (bip->bli_flags & XFS_BLI_STALE) {
		bip->bli_flags &= ~XFS_BLI_LOGGED;
		xfs_buf_item_trace("UNLOCK STALE", bip);
		ASSERT(bip->bli_format.blf_flags & XFS_BLI_CANCEL);
		if (!aborted)
			return;
	}

	/*
	 * Drop the transaction's reference to the log item if
	 * it was not logged as part of the transaction.  Otherwise
	 * we'll drop the reference in xfs_buf_item_unpin() when
	 * the transaction is really through with the buffer.
	 */
	if (!(bip->bli_flags & XFS_BLI_LOGGED)) {
		(void) atomicAddInt(&bip->bli_refcount, -1);
	} else {
		/*
		 * Clear the logged flag since this is per
		 * transaction state.
		 */
		bip->bli_flags &= ~XFS_BLI_LOGGED;
	}

	/*
	 * Before possibly freeing the buf item, determine if we should
	 * release the buffer at the end of this routine.
	 */
	hold = bip->bli_flags & XFS_BLI_HOLD;
	xfs_buf_item_trace("UNLOCK", bip);

	/*
	 * If the buf item isn't tracking any data, free it.
	 * Otherwise, if XFS_BLI_HOLD is set clear it.
	 */
	if (xfs_buf_item_bits(bip->bli_format.blf_data_map,
			      bip->bli_format.blf_map_size, 0) == 0) {
		xfs_buf_item_relse(bp);
	} else if (hold) {
		bip->bli_flags &= ~XFS_BLI_HOLD;
	}

	/*
	 * Release the buffer if XFS_BLI_HOLD was not set.
	 */
	if (!hold) {
		brelse(bp);
	}
}

/*
 * This is called to find out where the oldest active copy of the
 * buf log item in the on disk log resides now that the last log
 * write of it completed at the given lsn.
 * We always re-log all the dirty data in a buffer, so usually the
 * latest copy in the on disk log is the only one that matters.  For
 * those cases we simply return the given lsn.
 * 
 * The one exception to this is for buffers full of newly allocated
 * inodes.  These buffers are only relogged with the XFS_BLI_INODE_BUF
 * flag set, indicating that only the di_next_unlinked fields from the
 * inodes in the buffers will be replayed during recovery.  If the
 * original newly allocated inode images have not yet been flushed
 * when the buffer is so relogged, then we need to make sure that we
 * keep the old images in the 'active' portion of the log.  We do this
 * by returning the original lsn of that transaction here rather than
 * the current one.
 */
xfs_lsn_t
xfs_buf_item_committed(
	xfs_buf_log_item_t	*bip,
	xfs_lsn_t		lsn)
{
	xfs_buf_item_trace("COMMITTED", bip);
	if ((bip->bli_flags & XFS_BLI_INODE_ALLOC_BUF) &&
	    (bip->bli_item.li_lsn != 0)) {
		return bip->bli_item.li_lsn;
	}
	return (lsn);
}

/*
 * This is called when the transaction holding the buffer is aborted.
 * Just behave as if the transaction had been cancelled. If we're shutting down
 * and have aborted this transaction, we'll trap this buffer when it tries to
 * get written out.
 */
void
xfs_buf_item_abort(
	xfs_buf_log_item_t	*bip)
{
	buf_t 	*bp;

	bp = bip->bli_buf;
	buftrace("XFS_ABORT", bp);
	bp->b_flags &= ~(B_DELWRI|B_DONE);
	bp->b_flags |= B_STALE;
	xfs_buf_item_unlock(bip);
	return;
}

/*
 * This is called to asynchronously write the buffer associated with this
 * buf log item out to disk. The buffer will already have been locked by
 * a successful call to xfs_buf_item_trylock().  If the buffer still has
 * B_DELWRI set, then get it going out to disk with a call to bawrite().
 * If not, then just release the buffer.
 */
void
xfs_buf_item_push(
	xfs_buf_log_item_t	*bip)
{
	buf_t	*bp;

	ASSERT(!(bip->bli_flags & XFS_BLI_STALE));
	xfs_buf_item_trace("PUSH", bip);

	bp = bip->bli_buf;

	if (bp->b_flags & B_DELWRI) {
		xfs_bawrite(bip->bli_item.li_mountp, bp);
	} else {
		brelse(bp);
	}
}

/*
 * This is the ops vector shared by all buf log items.
 */
struct xfs_item_ops xfs_buf_item_ops = {
	(uint(*)(xfs_log_item_t*))xfs_buf_item_size,
	(void(*)(xfs_log_item_t*, xfs_log_iovec_t*))xfs_buf_item_format,
	(void(*)(xfs_log_item_t*))xfs_buf_item_pin,
	(void(*)(xfs_log_item_t*))xfs_buf_item_unpin,
	(void(*)(xfs_log_item_t*, xfs_trans_t *))xfs_buf_item_unpin_remove,
	(uint(*)(xfs_log_item_t*))xfs_buf_item_trylock,
	(void(*)(xfs_log_item_t*))xfs_buf_item_unlock,
	(xfs_lsn_t(*)(xfs_log_item_t*, xfs_lsn_t))xfs_buf_item_committed,
	(void(*)(xfs_log_item_t*))xfs_buf_item_push,
	(void(*)(xfs_log_item_t*))xfs_buf_item_abort,
	NULL
};


/*
 * Allocate a new buf log item to go with the given buffer.
 * Set the buffer's b_fsprivate field to point to the new
 * buf log item.  If there are other item's attached to the
 * buffer (see xfs_buf_attach_iodone() below), then put the
 * buf log item at the front.
 */
void
xfs_buf_item_init(
	buf_t		*bp,
	xfs_mount_t	*mp)
{
	xfs_log_item_t		*lip;
	xfs_buf_log_item_t	*bip;
	int			chunks;
	int			map_size;

	/*
	 * Check to see if there is already a buf log item for
	 * this buffer.  If there is, it is guaranteed to be
	 * the first.  If we do already have one, there is
	 * nothing to do here so return.
	 */
	if (bp->b_fsprivate3 != mp)
		bp->b_fsprivate3 = mp;
	if (bp->b_bdstrat == NULL)
		bp->b_bdstrat = xfs_bdstrat_cb;
	if (bp->b_fsprivate != NULL) {
		lip = (xfs_log_item_t *)bp->b_fsprivate;
		if (lip->li_type == XFS_LI_BUF) {
			return;
		}
	}
		
	/*
	 * chunks is the number of XFS_BLI_CHUNK size pieces
	 * the buffer can be divided into. Make sure not to
	 * truncate any pieces.  map_size is the size of the
	 * bitmap needed to describe the chunks of the buffer.
	 */
	chunks = (int)((bp->b_bcount + (XFS_BLI_CHUNK - 1)) >> XFS_BLI_SHIFT);
	map_size = (int)((chunks + NBWORD) >> BIT_TO_WORD_SHIFT);

	bip = (xfs_buf_log_item_t*)kmem_zone_zalloc(xfs_buf_item_zone,
						    KM_SLEEP);
	bip->bli_item.li_type = XFS_LI_BUF;
	bip->bli_item.li_ops = &xfs_buf_item_ops;
	bip->bli_item.li_mountp = mp;
	bip->bli_buf = bp;
	bip->bli_format.blf_type = XFS_LI_BUF;
	bip->bli_format.blf_blkno = (__int64_t)bp->b_blkno;
	bip->bli_format.blf_len = (ushort)BTOBB(bp->b_bcount);
	bip->bli_format.blf_map_size = map_size;
#ifdef XFS_BLI_TRACE
	bip->bli_trace = ktrace_alloc(XFS_BLI_TRACE_SIZE, 0);
#endif

#ifdef XFS_TRANS_DEBUG
	/*
	 * Allocate the arrays for tracking what needs to be logged
	 * and what our callers request to be logged.  bli_orig
	 * holds a copy of the original, clean buffer for comparison
	 * against, and bli_logged keeps a 1 bit flag per byte in
	 * the buffer to indicate which bytes the callers have asked
	 * to have logged.
	 */
	bip->bli_orig = (char *)kmem_alloc(bp->b_bcount, KM_SLEEP);
	bcopy(bp->b_un.b_addr, bip->bli_orig, bp->b_bcount);
	bip->bli_logged = (char *)kmem_zalloc(bp->b_bcount / NBBY, KM_SLEEP);
#endif

	/*
	 * Put the buf item into the list of items attached to the
	 * buffer at the front.
	 */
	if (bp->b_fsprivate != NULL) {
		bip->bli_item.li_bio_list = (xfs_log_item_t *)bp->b_fsprivate;
	}
	bp->b_fsprivate = bip;
}


/*
 * Mark bytes first through last inclusive as dirty in the buf
 * item's bitmap.
 */
void
xfs_buf_item_log(
	xfs_buf_log_item_t	*bip,
	uint			first,
	uint			last)
{
	uint		first_bit;
	uint		last_bit;
	uint		bits_to_set;
	uint		bits_set;
	uint		word_num;
	uint		*wordp;
	uint		bit;
	uint		end_bit;
	uint		mask;

	/*
	 * Mark the item as having some dirty data for
	 * quick reference in xfs_buf_item_dirty.
	 */
	bip->bli_flags |= XFS_BLI_DIRTY;

	/*
	 * Convert byte offsets to bit numbers.
	 */
	first_bit = first >> XFS_BLI_SHIFT;
	last_bit = last >> XFS_BLI_SHIFT;

	/*
	 * Calculate the total number of bits to be set.
	 */
	bits_to_set = last_bit - first_bit + 1;	

	/*
	 * Get a pointer to the first word in the bitmap
	 * to set a bit in.
	 */
	word_num = first_bit >> BIT_TO_WORD_SHIFT;
	wordp = &(bip->bli_format.blf_data_map[word_num]);

	/*
	 * Calculate the starting bit in the first word.
	 */
	bit = first_bit & (uint)(NBWORD - 1);

	/*
	 * First set any bits in the first word of our range.
	 * If it starts at bit 0 of the word, it will be
	 * set below rather than here.  That is what the variable
	 * bit tells us. The variable bits_set tracks the number
	 * of bits that have been set so far.  End_bit is the number
	 * of the last bit to be set in this word plus one.
	 */
	if (bit) {
		end_bit = MIN(bit + bits_to_set, (uint)NBWORD);
		mask = ((1 << (end_bit - bit)) - 1) << bit;
		*wordp |= mask;
		wordp++;
		bits_set = end_bit - bit;
	} else {
		bits_set = 0;
	}

	/*
	 * Now set bits a whole word at a time that are between
	 * first_bit and last_bit.
	 */
	while ((bits_to_set - bits_set) >= NBWORD) {
		*wordp |= 0xffffffff;
		bits_set += NBWORD;
		wordp++;
	}

	/*
	 * Finally, set any bits left to be set in one last partial word.
	 */
	end_bit = bits_to_set - bits_set;
	if (end_bit) {
		mask = (1 << end_bit) - 1;
		*wordp |= mask;
	}

	xfs_buf_item_log_debug(bip, first, last);
}

#ifdef XFS_TRANS_DEBUG
/*
 * This function uses an alternate strategy for tracking the bytes
 * that the user requests to be logged.  This can then be used
 * in conjunction with the bli_orig array in the buf log item to
 * catch bugs in our callers' code.
 *
 * We also double check the bits set in xfs_buf_item_log using a
 * simple algorithm to check that every byte is accounted for.
 */
STATIC void
xfs_buf_item_log_debug(
	xfs_buf_log_item_t	*bip,
	uint			first,
	uint			last)
{
	uint	x;
	uint	byte;
	uint	nbytes;
	uint	chunk_num;
	uint	word_num;
	uint	bit_num;
	uint	bit_set;
	uint	*wordp;

	ASSERT(bip->bli_logged != NULL);
	byte = first;
	nbytes = last - first + 1;
	bfset(bip->bli_logged, first, nbytes);
	for (x = 0; x < nbytes; x++) { 
		chunk_num = byte >> XFS_BLI_SHIFT;
		word_num = chunk_num >> BIT_TO_WORD_SHIFT;
		bit_num = chunk_num & (NBWORD - 1);
		wordp = &(bip->bli_format.blf_data_map[word_num]);
		bit_set = *wordp & (1 << bit_num);
		ASSERT(bit_set);
		byte++;
	}
}

/*
 * This function is called when we flush something into a buffer without
 * logging it.  This happens for things like inodes which are logged
 * separately from the buffer.
 */
void
xfs_buf_item_flush_log_debug(
	buf_t	*bp,
	uint	first,
	uint	last)
{
	xfs_buf_log_item_t	*bip;
	uint			nbytes;

	bip = (xfs_buf_log_item_t*)bp->b_fsprivate;
	if ((bip == NULL) || (bip->bli_item.li_type != XFS_LI_BUF)) {
		return;
	}

	ASSERT(bip->bli_logged != NULL);
	nbytes = last - first + 1;
	bfset(bip->bli_logged, first, nbytes);
}

/*
 * This function is called to verify that our caller's have logged
 * all the bytes that they changed.
 *
 * It does this by comparing the original copy of the buffer stored in
 * the buf log item's bli_orig array to the current copy of the buffer
 * and ensuring that all bytes which miscompare are set in the bli_logged
 * array of the buf log item.
 */
STATIC void
xfs_buf_item_log_check(
	xfs_buf_log_item_t	*bip)
{
	char	*orig;
	char	*buffer;
	int	x;
	buf_t	*bp;

	ASSERT(bip->bli_orig != NULL);
	ASSERT(bip->bli_logged != NULL);

	bp = bip->bli_buf;
	ASSERT(bp->b_bcount > 0);
	ASSERT(bp->b_un.b_addr != NULL);
	orig = bip->bli_orig;
	buffer = bp->b_un.b_addr;
	for (x = 0; x < bp->b_bcount; x++) {
		if (orig[x] != buffer[x] && !btst(bip->bli_logged, x))
			cmn_err(CE_PANIC,
	"xfs_buf_item_log_check bip %x buffer %x orig %x index %d",
				bip, bp, orig, x);
	}
}
#endif /* XFS_TRANS_DEBUG */

/*
 * Count the number of bits set in the bitmap starting with bit
 * start_bit.  Size is the size of the bitmap in words.
 *
 * Do the counting by mapping a byte value to the number of set
 * bits for that value using the xfs_countbit array, i.e.
 * xfs_countbit[0] == 0, xfs_countbit[1] == 1, xfs_countbit[2] == 1,
 * xfs_countbit[3] == 2, etc.
 */
int
xfs_buf_item_bits(
	uint	*map,
	uint	size,
	uint	start_bit)
{
	register int	bits;
	register char	*bytep;
	register char	*end_map;
	int		byte_bit;

	bits = 0;
	end_map = (char*)(map + size);
	bytep = (char*)(map + (start_bit & ~0x7));
	byte_bit = start_bit & 0x7;

	/*
	 * If the caller fell off the end of the map, return 0.
	 */
	if (bytep >= end_map) {
		return (0);
	}

	/*
	 * If start_bit is not byte aligned, then process the
	 * first byte separately.
	 */
	if (byte_bit != 0) {
		/*
		 * Shift off the bits we don't want to look at,
		 * before indexing into xfs_countbit.
		 */
		bits += xfs_countbit[(*bytep >> byte_bit)];
		bytep++;
	}

	/*
	 * Count the bits in each byte until the end of the bitmap.
	 */
	while (bytep < end_map) {
		bits += xfs_countbit[*bytep];
		bytep++;
	}

	return (bits);
}	/* xfs_buf_item_bits */
	
/*
 * Count the number of contiguous bits set in the bitmap starting with bit
 * start_bit.  Size is the size of the bitmap in words.
 *
 * Do the counting by mapping a byte value to the number of set
 * bits for that value using the xfs_countbit array, i.e.
 */
int
xfs_buf_item_contig_bits(
	uint	*map,
	uint	size,
	uint	start_bit)
{
	register int	bits;
	register uint	*wordp;
	register uint	cwordp;
	register uint	*end_map;
	int		word_bit;
	int		cnt;

	bits = 0;
	end_map = (uint *)(map + size);
	wordp = (uint *)(map + (start_bit >> 5));
	word_bit = start_bit & 0x1F;

	/*
	 * If the caller fell off the end of the map, return 0.
	 */
	if (wordp >= end_map) {
		return (0);
	}

	/*
	 * If start_bit is not byte aligned, then process just the
	 * relevant bits.
	 */
	if (word_bit != 0) {
		cwordp = *wordp >> word_bit;
	} else {
		cwordp = *wordp;
		word_bit = 0;
	}

	/*
	 * Count the bits in each byte until the end of the bitmap.
	 */
	while (wordp < end_map) {
		/*
		 * Cycle through bits left in word.  If the low bit is
		 * set, we've found a 'contingous' bit.
		 */
		for (cnt = (int)(NBPW*NBBY-word_bit); cnt > 0; cnt--) {
			if (cwordp & 0x1)
				bits++;
			else
				return bits;
			cwordp >>= 1;
		}

		/* Grab another word */
		wordp++;
		cwordp = *wordp;
		word_bit = 0;
	}

	return (bits);
}	/* xfs_buf_item_contig_bits */
	
/*
 * This takes the bit number to start looking from and
 * returns the next set bit from there.  It returns -1
 * if there are no more bits set or the start bit is
 * beyond the end of the bitmap.
 *
 * Size is the number of words, not bytes, in the bitmap.
 */
int
xfs_buf_item_next_bit(
	uint	*map,
	uint	size,
	uint	start_bit)
{
	int	next_bit;
	uint	*wordp;
	uint	*end_map;
	int	word_bit;
	uint	word;

	end_map = map + size;
	wordp = map + (start_bit >> BIT_TO_WORD_SHIFT);
	word_bit = start_bit & (int)(NBWORD - 1);

	/*
	 * If the caller has stepped beyond the end of the bitmap,
	 * return -1.
	 */
	if (wordp >= end_map) {
		return (-1);
	}

	next_bit = start_bit;

	/*
	 * If the start_bit does not start on a word boundary,
	 * check the remainder of the starting word first.
	 */
	if (word_bit != 0) {
		word = *wordp >> word_bit;
		while (word != 0) {
			if (word & 1) {
				return (next_bit);
			}
			word = word >> 1;
			next_bit++;	
		}
		/*
		 * Since we don't know how many bits we looked at before
		 * word became 0, just set next_bit to the start of the
		 * next word.
		 */
		wordp++;
		next_bit = (int)ROUNDUPNBWORD(start_bit); 
	}

	/*
	 * Do word at a time checking for bits until the end of the map.
	 */
	while (wordp < end_map) {
		/*
		 * If the current word is empty, skip it.
		 */
		if (*wordp == 0) {
			wordp++;
			next_bit += NBWORD;
			continue;
		}

		/*
		 * We know we've got a bit in this word, find it.
		 */
		word = *wordp;
		for (;;) {
			if (word & 1) {
				return (next_bit);
			}
			next_bit++;
			word = word >> 1;
		}
	}

	/*
	 * If there were no more bits in the bitmap, return -1.
	 */
	return (-1);
}

#if 0
/*
 * Set the specified bit in the given bitmap.
 */
/*ARGSUSED*/
STATIC void
xfs_buf_item_set_bit(
	uint	*map,
	uint	size,
	uint	bit)
{
	uint	*wordp;
	int	word_bit;

	wordp = map + (bit >> BIT_TO_WORD_SHIFT);
	word_bit = bit & (NBWORD - 1);

	*wordp |= 1 << word_bit;
}
#endif
		
/*
 * Return 1 if the buffer has some data that has been logged (at any
 * point, not just the current transaction) and 0 if not.
 */
uint
xfs_buf_item_dirty(
	xfs_buf_log_item_t	*bip)
{
	return (bip->bli_flags & XFS_BLI_DIRTY);
}

/*
 * This is called when the buf log item is no longer needed.  It should
 * free the buf log item associated with the given buffer and clear
 * the buffer's pointer to the buf log item.  If there are no more
 * items in the list, clear the b_iodone field of the buffer (see
 * xfs_buf_attach_iodone() below).
 */
void
xfs_buf_item_relse(
	buf_t	*bp)
{
	xfs_buf_log_item_t	*bip;

	buftrace("XFS_RELSE", bp);
	bip = (xfs_buf_log_item_t*)bp->b_fsprivate;
	bp->b_fsprivate = bip->bli_item.li_bio_list;
	if ((bp->b_fsprivate == NULL) && (bp->b_iodone != NULL)) {
		ASSERT((bp->b_flags & B_UNINITIAL) == 0);
		bp->b_iodone = NULL;
	}

#ifdef XFS_TRANS_DEBUG
	kmem_free(bip->bli_orig, bp->b_bcount);
	bip->bli_orig = NULL;
	kmem_free(bip->bli_logged, bp->b_bcount / NBBY);
	bip->bli_logged = NULL;
#endif /* XFS_TRANS_DEBUG */

#ifdef XFS_BLI_TRACE
	ktrace_free(bip->bli_trace);
#endif
	kmem_zone_free(xfs_buf_item_zone, bip);
}


/*
 * Add the given log item with it's callback to the list of callbacks
 * to be called when the buffer's I/O completes.  If it is not set
 * already, set the buffer's b_iodone() routine to be
 * xfs_buf_iodone_callbacks() and link the log item into the list of
 * items rooted at b_fsprivate.  Items are always added as the second
 * entry in the list if there is a first, because the buf item code
 * assumes that the buf log item is first.
 */
void
xfs_buf_attach_iodone(
	buf_t		*bp,
	void		(*cb)(buf_t *, xfs_log_item_t *),
	xfs_log_item_t	*lip)
{
	xfs_log_item_t	*head_lip;

	ASSERT(bp->b_flags & B_BUSY);
	ASSERT(valusema(&bp->b_lock) <= 0);

	lip->li_cb = cb;
	if (bp->b_fsprivate != NULL) {
		head_lip = (xfs_log_item_t *)bp->b_fsprivate;
		lip->li_bio_list = head_lip->li_bio_list;
		head_lip->li_bio_list = lip;
	} else {
		bp->b_fsprivate = lip;
	}

	ASSERT((bp->b_iodone == xfs_buf_iodone_callbacks) ||
	       (bp->b_iodone == NULL));
	if (bp->b_iodone == NULL) {
		bp->b_iodone = xfs_buf_iodone_callbacks;
	}
}

STATIC void
xfs_buf_do_callbacks(
	buf_t		*bp,
	xfs_log_item_t	*lip)
{
	xfs_log_item_t	*nlip;

	while (lip != NULL) {
		nlip = lip->li_bio_list;
		ASSERT(lip->li_cb != NULL);
		/*
		 * Clear the next pointer so we don't have any
		 * confusion if the item is added to another buf.
		 * Don't touch the log item after calling its
		 * callback, because it could have freed itself.
		 */
		lip->li_bio_list = NULL;
		lip->li_cb(bp, lip);
		lip = nlip;
	}
}
		       
/*
 * This is the iodone() function for buffers which have had callbacks
 * attached to them by xfs_buf_attach_iodone().  It should remove each
 * log item from the buffer's list and call the callback of each in turn.
 * When done, the buffer's fsprivate field is set to NULL and the buffer
 * is unlocked with a call to iodone().
 */
void
xfs_buf_iodone_callbacks(
	buf_t	*bp)
{
	xfs_log_item_t	*lip;
	static time_t	lasttime;
	static dev_t	lastdev;
	xfs_mount_t	*mp;

	ASSERT(bp->b_fsprivate != NULL);
	lip = (xfs_log_item_t *)bp->b_fsprivate;

	if (geterror(bp) != 0) {
		/*
		 * If we've already decided to shutdown the filesystem
		 * because of IO errors, there's no point in giving this
		 * a retry.
		 */ 
		mp = lip->li_mountp;
		if (XFS_FORCED_SHUTDOWN(mp)) {
			ASSERT(bp->b_edev == mp->m_dev);
			bp->b_flags |= B_STALE;
			bp->b_flags &= ~(B_DONE|B_DELWRI);
			buftrace("BUF_IODONE_CB", bp);
			xfs_buf_do_callbacks(bp, lip);
			bp->b_fsprivate = NULL;
			bp->b_iodone = NULL;

			/*
			 * XFS_SHUT flag gets set when we go thru the
			 * entire buffer cache and deliberately start
			 * throwing away delayed write buffers.
			 * Since there's no biowait done on those,
			 * we should just brelse them.
			 */
			if (bp->b_flags & B_XFS_SHUT) {
				bp->b_flags &= ~B_XFS_SHUT;
				brelse(bp);
			} else {
				biodone(bp);
			}
			
			return;
		}

		if ((bp->b_edev != lastdev) || ((lbolt - lasttime) > 500)) {
			prdev("XFS write error in file system meta-data "
			      "block 0x%x in %s",
			      (int)bp->b_edev, bp->b_blkno, 
			      mp->m_fsname);
			lasttime = lbolt;
		}
		lastdev = bp->b_edev;

		if (bp->b_flags & B_ASYNC) {
			/*
			 * If the write was asynchronous then noone will be
			 * looking for the error.  Clear the error state
			 * and write the buffer out again delayed write.
			 *
			 * XXXsup This is OK, so long as we catch these
			 * before we start the umount; we don't want these
			 * DELWRI metadata bufs to be hanging around.
			 */
			bp->b_error = 0;
			bp->b_flags &= ~(B_ERROR);
			if (!(bp->b_flags & B_STALE)) {
				bp->b_flags |= B_DELWRI | B_DONE;
				bp->b_start = lbolt;
			}
			ASSERT(bp->b_iodone);
			buftrace("BUF_IODONE ASYNC", bp);
			brelse(bp);
		} else {
			/*
			 * If the write of the buffer was not asynchronous,
			 * then we want to make sure to return the error
			 * to the caller of bwrite().  Because of this we
			 * cannot clear the B_ERROR state at this point.
			 * Instead we install a callback function that
			 * will be called when the buffer is released, and
			 * that routine will clear the error state and
			 * set the buffer to be written out again after
			 * some delay.
			 */
			/* We actually overwrite the existing b-relse
			   function at times, but we're gonna be shutting down
			   anyway. */
			bp->b_relse = xfs_buf_error_relse;
			bp->b_flags |= B_DONE;
			vsema(&bp->b_iodonesema);
		}
		return;
	}
#ifdef XFSERRORDEBUG
	buftrace("XFS BUFCB NOERR", bp);
#endif
	xfs_buf_do_callbacks(bp, lip);
	bp->b_fsprivate = NULL;
	bp->b_iodone = NULL;
	biodone(bp);
}

/*
 * This is a callback routine attached to a buffer which gets an error
 * when being written out synchronously. 
 */
STATIC void
xfs_buf_error_relse(
	buf_t	*bp)
{
	xfs_log_item_t 	*lip;
	xfs_mount_t	*mp;

	lip = (xfs_log_item_t *)bp->b_fsprivate;
	mp = (xfs_mount_t *)lip->li_mountp;
	ASSERT(bp->b_edev == mp->m_dev);

	bp->b_flags |= B_STALE|B_DONE;
	bp->b_flags &= ~(B_DELWRI|B_ERROR);
	bp->b_error = 0;
	buftrace("BUF_ERROR_RELSE", bp);
	if (! XFS_FORCED_SHUTDOWN(mp)) 		
		xfs_force_shutdown(mp, XFS_METADATA_IO_ERROR);
	/*
	 * We have to unpin the pinned buffers so do the
	 * callbacks.
	 */
	xfs_buf_do_callbacks(bp, lip);
	bp->b_fsprivate = NULL;
	bp->b_iodone = NULL;
	bp->b_relse = NULL;
	brelse(bp);
	return;

}


/*
 * This is the iodone() function for buffers which have been
 * logged.  It is called when they are eventually flushed out.
 * It should remove the buf item from the AIL, and free the buf item.
 * It is called by xfs_buf_iodone_callbacks() above which will take
 * care of cleaning up the buffer itself.
 */ 
/* ARGSUSED */
void
xfs_buf_iodone(
	buf_t			*bp,
	xfs_buf_log_item_t	*bip)
{
	struct xfs_mount	*mp;
	SPLDECL(s);

	ASSERT(bip->bli_buf == bp);

	mp = bip->bli_item.li_mountp;

	/*
	 * If we are forcibly shutting down, this may well be
	 * off the AIL already. That's because we simulate the
	 * log-committed callbacks to unpin these buffers. Or we may never
	 * have put this item on AIL because of the transaction was
	 * aborted forcibly. xfs_trans_delete_ail() takes care of these.
	 *
	 * Either way, AIL is useless if we're forcing a shutdown.
	 */
	AIL_LOCK(mp,s);
	/*
	 * xfs_trans_delete_ail() drops the AIL lock.
	 */
	xfs_trans_delete_ail(mp, (xfs_log_item_t *)bip, s);

#ifdef XFS_TRANS_DEBUG
	kmem_free(bip->bli_orig, bp->b_bcount);
	bip->bli_orig = NULL;
	kmem_free(bip->bli_logged, bp->b_bcount / NBBY);
	bip->bli_logged = NULL;
#endif /* XFS_TRANS_DEBUG */

#ifdef XFS_BLI_TRACE
	ktrace_free(bip->bli_trace);
#endif
	kmem_zone_free(xfs_buf_item_zone, bip);
}

#if defined(XFS_BLI_TRACE)
void
xfs_buf_item_trace(
	char			*id,
	xfs_buf_log_item_t	*bip)
{
	buf_t	*bp;
	ASSERT(bip->bli_trace != NULL);

	bp = bip->bli_buf;
	ktrace_enter(bip->bli_trace,
		     (void *)id,
		     (void *)bip->bli_buf,
		     (void *)((unsigned long)bip->bli_flags),
		     (void *)((unsigned long)bip->bli_recur),
		     (void *)((unsigned long)bip->bli_refcount),
		     (void *)bp->b_blkno,
		     (void *)((unsigned long)bp->b_bcount),
		     (void *)((unsigned long)(0xFFFFFFFF & (bp->b_flags >> 32))),
		     (void *)((unsigned long)(0xFFFFFFFF & bp->b_flags)),
		     (void *)bp->b_fsprivate,
		     (void *)bp->b_fsprivate2,
		     (void *)((unsigned long)bp->b_pincount),
		     (void *)bp->b_iodone,
		     (void *)((unsigned long)(valusema(&(bp->b_lock)))),
		     (void *)bip->bli_item.li_desc,
		     (void *)((unsigned long)bip->bli_item.li_flags));
}
#endif /* XFS_BLI_TRACE */


