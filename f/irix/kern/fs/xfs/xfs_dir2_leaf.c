#ident "$Revision: 1.2 $"

/*
 * xfs_dir2_leaf.c
 * XFS directory version 2 implementation - single leaf form
 * see xfs_dir2_leaf.h for data structures.
 * These directories have multiple XFS_DIR2_DATA blocks and one
 * XFS_DIR2_LEAF1 block containing the hash table and freespace map.
 */

#ifdef SIM
#define _KERNEL 1
#endif
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/uuid.h>
#ifdef SIM
#undef _KERNEL
#endif
#include <sys/kmem.h>
#include <sys/errno.h>
#include <sys/vnode.h>
#include <sys/dirent.h>
#include <stddef.h>
#ifdef SIM
#include <bstring.h>
#else
#include <sys/systm.h>
#endif
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
#include "xfs_bmap_btree.h"
#include "xfs_bmap.h"
#include "xfs_attr_sf.h"
#include "xfs_dir_sf.h"
#include "xfs_dir2_sf.h"
#include "xfs_dinode.h"
#include "xfs_inode.h"
#include "xfs_da_btree.h"
#include "xfs_dir2_data.h"
#include "xfs_dir2_leaf.h"
#include "xfs_dir2_block.h"
#include "xfs_dir2_node.h"
#include "xfs_dir2_trace.h"
#include "xfs_error.h"
#include "xfs_bit.h"
#ifdef SIM
#include "sim.h"
#endif

/*
 * Local function declarations.
 */
#ifdef DEBUG
static void xfs_dir2_leaf_check(xfs_inode_t *dp, xfs_dabuf_t *bp);
#else
#define	xfs_dir2_leaf_check(dp, bp)
#endif
static int xfs_dir2_leaf_lookup_int(xfs_da_args_t *args, xfs_dabuf_t **lbpp,
				    int *indexp, xfs_dabuf_t **dbpp);

/*
 * Convert a block form directory to a leaf form directory.
 */
int						/* error */
xfs_dir2_block_to_leaf(
	xfs_da_args_t		*args,		/* operation arguments */
	xfs_dabuf_t		*dbp)		/* input block's buffer */
{
	xfs_dir2_data_off_t	*bestsp;	/* leaf's bestsp entries */
	xfs_dablk_t		blkno;		/* leaf block's bno */
	xfs_dir2_block_t	*block;		/* block structure */
	xfs_dir2_leaf_entry_t	*blp;		/* block's leaf entries */
	xfs_dir2_block_tail_t	*btp;		/* block's tail */
	xfs_inode_t		*dp;		/* incore directory inode */
	int			error;		/* error return code */
	xfs_dabuf_t		*lbp;		/* leaf block's buffer */
	xfs_dir2_db_t		ldb;		/* leaf block's bno */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	xfs_dir2_leaf_tail_t	*ltp;		/* leaf's tail */
	xfs_mount_t		*mp;		/* filesystem mount point */
	int			needlog;	/* need to log block header */
	int			needscan;	/* need to rescan bestfree */
	xfs_trans_t		*tp;		/* transaction pointer */

	xfs_dir2_trace_args_b("block_to_leaf", args, dbp);
	dp = args->dp;
	mp = dp->i_mount;
	tp = args->trans;
	/*
	 * Add the leaf block to the inode.
	 * This interface will only put blocks in the leaf/node range.
	 * Since that's empty now, we'll get the root (block 0 in range).
	 */
	if (error = xfs_da_grow_inode(args, &blkno)) {
#pragma mips_frequency_hint NEVER
		return error;
	}
	ldb = XFS_DIR2_DA_TO_DB(mp, blkno);
	ASSERT(ldb == XFS_DIR2_LEAF_FIRSTDB(mp));
	/*
	 * Initialize the leaf block, get a buffer for it.
	 */
	if (error = xfs_dir2_leaf_init(args, ldb, &lbp, XFS_DIR2_LEAF1_MAGIC)) {
#pragma mips_frequency_hint NEVER
		return error;
	}
	ASSERT(lbp != NULL);
	leaf = lbp->data;
	block = dbp->data;
	xfs_dir2_data_check(dp, dbp);
	btp = XFS_DIR2_BLOCK_TAIL_P(mp, block);
	blp = XFS_DIR2_BLOCK_LEAF_P(btp);
	/*
	 * Set the counts in the leaf header.
	 */
	leaf->hdr.count = btp->count;
	leaf->hdr.stale = btp->stale;
	/*
	 * Could compact these but I think we always do the conversion
	 * after squeezing out stale entries.
	 */
	bcopy(blp, leaf->ents, btp->count * sizeof(xfs_dir2_leaf_entry_t));
	xfs_dir2_leaf_log_ents(tp, lbp, 0, leaf->hdr.count - 1);
	needscan = 0;
	needlog = 1;
	/*
	 * Make the space formerly occupied by the leaf entries and block
	 * tail be free.
	 */
	xfs_dir2_data_make_free(tp, dbp,
		(xfs_dir2_data_aoff_t)((char *)blp - (char *)block),
		(xfs_dir2_data_aoff_t)((char *)block + mp->m_dirblksize -
				       (char *)blp),
		&needlog, &needscan);
	/*
	 * Fix up the block header, make it a data block.
	 */
	block->hdr.magic = XFS_DIR2_DATA_MAGIC;
	if (needscan)
		xfs_dir2_data_freescan(mp, (xfs_dir2_data_t *)block, &needlog,
			NULL);
	/*
	 * Set up leaf tail and bests table.
	 */
	ltp = XFS_DIR2_LEAF_TAIL_P(mp, leaf);
	ltp->bestcount = 1;
	bestsp = XFS_DIR2_LEAF_BESTS_P(ltp);
	bestsp[0] = block->hdr.bestfree[0].length;
	/*
	 * Log the data header and leaf bests table.
	 */
	if (needlog)
		xfs_dir2_data_log_header(tp, dbp);
	xfs_dir2_leaf_check(dp, lbp);
	xfs_dir2_data_check(dp, dbp);
	xfs_dir2_leaf_log_bests(tp, lbp, 0, 0);
	xfs_da_buf_done(lbp);
	return 0;
}

/*
 * Add an entry to a leaf form directory.
 */
int						/* error */
xfs_dir2_leaf_addname(
	xfs_da_args_t		*args)		/* operation arguments */
{
	xfs_dir2_data_off_t	*bestsp;	/* freespace table in leaf */
	int			compact;	/* need to compact leaves */
	xfs_dir2_data_t		*data;		/* data block structure */
	xfs_dabuf_t		*dbp;		/* data block buffer */
	xfs_dir2_data_entry_t	*dep;		/* data block entry */
	xfs_inode_t		*dp;		/* incore directory inode */
	xfs_dir2_data_unused_t	*dup;		/* data unused entry */
	int			error;		/* error return value */
	int			grown;		/* allocated new data block */
	int			highstale;	/* index of next stale leaf */
	int			i;		/* temporary, index */
	int			index;		/* leaf table position */
	xfs_dabuf_t		*lbp;		/* leaf's buffer */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	int			length;		/* length of new entry */
	xfs_dir2_leaf_entry_t	*lep;		/* leaf entry table pointer */
	int			lfloglow;	/* low leaf logging index */
	int			lfloghigh;	/* high leaf logging index */
	int			lowstale;	/* index of prev stale leaf */
	xfs_dir2_leaf_tail_t	*ltp;		/* leaf tail pointer */
	xfs_mount_t		*mp;		/* filesystem mount point */
	int			needbytes;	/* leaf block bytes needed */
	int			needlog;	/* need to log data header */
	int			needscan;	/* need to rescan data free */
	xfs_dir2_data_off_t	*tagp;		/* end of data entry */
	xfs_trans_t		*tp;		/* transaction pointer */
	xfs_dir2_db_t		use_block;	/* data block number */

	xfs_dir2_trace_args("leaf_addname", args);
	dp = args->dp;
	tp = args->trans;
	mp = dp->i_mount;
	/*
	 * Read the leaf block.
	 */
	error = xfs_da_read_buf(tp, dp, mp->m_dirleafblk, -1, &lbp,
		XFS_DATA_FORK);
	if (error) {
#pragma mips_frequency_hint NEVER
		return error;
	}
	ASSERT(lbp != NULL);
	/*
	 * Look up the entry by hash value and name.
	 * We know it's not there, our caller has already done a lookup.
	 * So the index is of the entry to insert in front of.
	 * But if there are dup hash values the index is of the first of those.
	 */
	index = xfs_dir2_leaf_search_hash(args, lbp);
	leaf = lbp->data;
	ltp = XFS_DIR2_LEAF_TAIL_P(mp, leaf);
	bestsp = XFS_DIR2_LEAF_BESTS_P(ltp);
	length = XFS_DIR2_DATA_ENTSIZE(args->namelen);
	/*
	 * See if there are any entries with the same hash value
	 * and space in their block for the new entry.
	 * This is good because it puts multiple same-hash value entries
	 * in a data block, improving the lookup of those entries.
	 */
	for (use_block = -1, lep = &leaf->ents[index];
	     index < leaf->hdr.count && lep->hashval == args->hashval;
	     index++, lep++) {
		if (lep->address == XFS_DIR2_NULL_DATAPTR)
			continue;
		i = XFS_DIR2_DATAPTR_TO_DB(mp, lep->address);
		ASSERT(i < ltp->bestcount);
		ASSERT(bestsp[i] != NULLDATAOFF);
		if (bestsp[i] >= length) {
			use_block = i;
			break;
		}
	}
	/*
	 * Didn't find a block yet, linear search all the data blocks.
	 */
	if (use_block == -1) {
		for (i = 0; i < ltp->bestcount; i++) {
			/*
			 * Remember a block we see that's missing.
			 */
			if (bestsp[i] == NULLDATAOFF && use_block == -1)
				use_block = i;
			else if (bestsp[i] >= length) {
				use_block = i;
				break;
			}
		}
	}
	/*
	 * How many bytes do we need in the leaf block?
	 */
	needbytes =
		(leaf->hdr.stale != 0 ? 0 : (uint)sizeof(leaf->ents[0])) +
		(use_block != -1 ? 0 : (uint)sizeof(leaf->bests[0]));
	/*
	 * Now kill use_block if it refers to a missing block, so we
	 * can use it as an indication of allocation needed.
	 */
	if (use_block != -1 && bestsp[use_block] == NULLDATAOFF)
		use_block = -1;
	/*
	 * If we don't have enough free bytes but we can make enough
	 * by compacting out stale entries, we'll do that.
	 */
	if ((char *)bestsp - (char *)&leaf->ents[leaf->hdr.count] < needbytes &&
	    leaf->hdr.stale > 1) {
#pragma mips_frequency_hint NEVER
		compact = 1;
	}
	/*
	 * Otherwise if we don't have enough free bytes we need to
	 * convert to node form.
	 */
	else if ((char *)bestsp - (char *)&leaf->ents[leaf->hdr.count] <
		 needbytes) {
#pragma mips_frequency_hint NEVER
		/*
		 * Just checking or no space reservation, give up.
		 */
		if (args->justcheck || args->total == 0) {
			xfs_da_brelse(tp, lbp);
			return XFS_ERROR(ENOSPC);
		}
		/*
		 * Convert to node form.
		 */
		error = xfs_dir2_leaf_to_node(args, lbp);
		xfs_da_buf_done(lbp);
		if (error)
			return error;
		/*
		 * Then add the new entry.
		 */
		return xfs_dir2_node_addname(args);
	}
	/*
	 * Otherwise it will fit without compaction.
	 */
	else
		compact = 0;
	/*
	 * If just checking, then it will fit unless we needed to allocate
	 * a new data block.
	 */
	if (args->justcheck) {
		xfs_da_brelse(tp, lbp);
		return use_block == -1 ? XFS_ERROR(ENOSPC) : 0;
	}
	/*
	 * If no allocations are allowed, return now before we've
	 * changed anything.
	 */
	if (args->total == 0 && use_block == -1) {
#pragma mips_frequency_hint NEVER
		xfs_da_brelse(tp, lbp);
		return XFS_ERROR(ENOSPC);
	}
	/*
	 * Need to compact the leaf entries, removing stale ones.
	 * Leave one stale entry behind - the one closest to our
	 * insertion index - and we'll shift that one to our insertion
	 * point later.
	 */
	if (compact) {
#pragma mips_frequency_hint NEVER
		xfs_dir2_leaf_compact_x1(lbp, &index, &lowstale, &highstale,
			&lfloglow, &lfloghigh);
	}
	/*
	 * There are stale entries, so we'll need log-low and log-high
	 * impossibly bad values later.
	 */
	else if (leaf->hdr.stale) {
		lfloglow = leaf->hdr.count;
		lfloghigh = -1;
	}
	/*
	 * If there was no data block space found, we need to allocate
	 * a new one.
	 */
	if (use_block == -1) {
#pragma mips_frequency_hint NEVER
		/*
		 * Add the new data block.
		 */
		if (error = xfs_dir2_grow_inode(args, XFS_DIR2_DATA_SPACE,
				&use_block)) {
			xfs_da_brelse(tp, lbp);
			return error;
		}
		/*
		 * Initialize the block.
		 */
		if (error = xfs_dir2_data_init(args, use_block, &dbp)) {
			xfs_da_brelse(tp, lbp);
			return error;
		}
		/*
		 * If we're adding a new data block on the end we need to
		 * extend the bests table.  Copy it up one entry.
		 */
		if (use_block >= ltp->bestcount) {
			bestsp--;
			ovbcopy(&bestsp[1], &bestsp[0],
				ltp->bestcount * sizeof(bestsp[0]));
			ltp->bestcount++;
			xfs_dir2_leaf_log_tail(tp, lbp);
			xfs_dir2_leaf_log_bests(tp, lbp, 0, ltp->bestcount - 1);
		}
		/*
		 * If we're filling in a previously empty block just log it.
		 */
		else
			xfs_dir2_leaf_log_bests(tp, lbp, use_block, use_block);
		data = dbp->data;
		bestsp[use_block] = data->hdr.bestfree[0].length;
		grown = 1;
	}
	/*
	 * Already had space in some data block.
	 * Just read that one in.
	 */
	else {
		if (error =
		    xfs_da_read_buf(tp, dp, XFS_DIR2_DB_TO_DA(mp, use_block),
			    -1, &dbp, XFS_DATA_FORK)) {
#pragma mips_frequency_hint NEVER
			xfs_da_brelse(tp, lbp);
			return error;
		}
		data = dbp->data;
		grown = 0;
	}
	xfs_dir2_data_check(dp, dbp);
	/*
	 * Point to the biggest freespace in our data block.
	 */
	dup = (xfs_dir2_data_unused_t *)
	      ((char *)data + data->hdr.bestfree[0].offset);
	ASSERT(dup->length >= length);
	needscan = needlog = 0;
	/*
	 * Mark the initial part of our freespace in use for the new entry.
	 */
	xfs_dir2_data_use_free(tp, dbp, dup,
		(xfs_dir2_data_aoff_t)((char *)dup - (char *)data), length,
		&needlog, &needscan);
	/*
	 * Initialize our new entry (at last).
	 */
	dep = (xfs_dir2_data_entry_t *)dup;
	dep->inumber = args->inumber;
	dep->namelen = args->namelen;
	bcopy(args->name, dep->name, dep->namelen);
	tagp = XFS_DIR2_DATA_ENTRY_TAG_P(dep);
	*tagp = (xfs_dir2_data_off_t)((char *)dep - (char *)data);
	/*
	 * Need to scan fix up the bestfree table.
	 */
	if (needscan)
		xfs_dir2_data_freescan(mp, data, &needlog, NULL);
	/*
	 * Need to log the data block's header.
	 */
	if (needlog)
		xfs_dir2_data_log_header(tp, dbp);
	xfs_dir2_data_log_entry(tp, dbp, dep);
	/*
	 * If the bests table needs to be changed, do it.
	 * Log the change unless we've already done that.
	 */
	if (bestsp[use_block] != data->hdr.bestfree[0].length) {
		bestsp[use_block] = data->hdr.bestfree[0].length;
		if (!grown)
			xfs_dir2_leaf_log_bests(tp, lbp, use_block, use_block);
	}
	/*
	 * Now we need to make room to insert the leaf entry.
	 * If there are no stale entries, we just insert a hole at index.
	 */
	if (leaf->hdr.stale == 0) {
		/*
		 * lep is still good as the index leaf entry.
		 */
		if (index < leaf->hdr.count)
			ovbcopy(lep, lep + 1,
				(leaf->hdr.count - index) * sizeof(*lep));
		/*
		 * Record low and high logging indices for the leaf.
		 */
		lfloglow = index;
		lfloghigh = leaf->hdr.count;
		leaf->hdr.count++;
	}
	/*
	 * There are stale entries.
	 * We will use one of them for the new entry.
	 * It's probably not at the right location, so we'll have to
	 * shift some up or down first.
	 */
	else {
		/*
		 * If we didn't compact before, we need to find the nearest
		 * stale entries before and after our insertion point.
		 */
		if (compact == 0) {
			/*
			 * Find the first stale entry before the insertion
			 * point, if any.
			 */
			for (lowstale = index - 1;
			     lowstale >= 0 &&
				leaf->ents[lowstale].address !=
				XFS_DIR2_NULL_DATAPTR;
			     lowstale--)
				continue;
			/*
			 * Find the next stale entry at or after the insertion
			 * point, if any.   Stop if we go so far that the
			 * lowstale entry would be better.
			 */
			for (highstale = index;
			     highstale < leaf->hdr.count &&
				leaf->ents[highstale].address !=
				XFS_DIR2_NULL_DATAPTR &&
				(lowstale < 0 ||
				 index - lowstale - 1 >= highstale - index);
			     highstale++)
				continue;
		}
		/*
		 * If the low one is better, use it.
		 */
		if (lowstale >= 0 &&
		    (highstale == leaf->hdr.count ||
		     index - lowstale - 1 < highstale - index)) {
			ASSERT(index - lowstale - 1 >= 0);
			ASSERT(leaf->ents[lowstale].address ==
			       XFS_DIR2_NULL_DATAPTR);
			/*
			 * Copy entries up to cover the stale entry
			 * and make room for the new entry.
			 */
			if (index - lowstale - 1 > 0)
				ovbcopy(&leaf->ents[lowstale + 1],
					&leaf->ents[lowstale],
					(index - lowstale - 1) * sizeof(*lep));
			lep = &leaf->ents[index - 1];
			lfloglow = MIN(lowstale, lfloglow);
			lfloghigh = MAX(index - 1, lfloghigh);
		}
		/*
		 * The high one is better, so use that one.
		 */
		else {
			ASSERT(highstale - index >= 0);
			ASSERT(leaf->ents[highstale].address ==
			       XFS_DIR2_NULL_DATAPTR);
			/*
			 * Copy entries down to copver the stale entry
			 * and make room for the new entry.
			 */
			if (highstale - index > 0)
				ovbcopy(&leaf->ents[index],
					&leaf->ents[index + 1],
					(highstale - index) * sizeof(*lep));
			lep = &leaf->ents[index];
			lfloglow = MIN(index, lfloglow);
			lfloghigh = MAX(highstale, lfloghigh);
		}
		leaf->hdr.stale--;
	}
	/*
	 * Fill in the new leaf entry.
	 */
	lep->hashval = args->hashval;
	lep->address = XFS_DIR2_DB_OFF_TO_DATAPTR(mp, use_block, *tagp);
	/*
	 * Log the leaf fields and give up the buffers.
	 */
	xfs_dir2_leaf_log_header(tp, lbp);
	xfs_dir2_leaf_log_ents(tp, lbp, lfloglow, lfloghigh);
	xfs_dir2_leaf_check(dp, lbp);
	xfs_da_buf_done(lbp);
	xfs_dir2_data_check(dp, dbp);
	xfs_da_buf_done(dbp);
	return 0;
}

#ifdef DEBUG
/*
 * Check the internal consistency of a leaf1 block.
 * Pop an assert if something is wrong.
 */
void
xfs_dir2_leaf_check(
	xfs_inode_t		*dp,		/* incore directory inode */
	xfs_dabuf_t		*bp)		/* leaf's buffer */
{
	int			i;		/* leaf index */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	xfs_dir2_leaf_tail_t	*ltp;		/* leaf tail pointer */
	xfs_mount_t		*mp;		/* filesystem mount point */
	int			stale;		/* count of stale leaves */

	leaf = bp->data;
	mp = dp->i_mount;
	ASSERT(leaf->hdr.info.magic == XFS_DIR2_LEAF1_MAGIC);
	/*
	 * This value is not restrictive enough.
	 * Should factor in the size of the bests table as well.
	 * We can deduce a value for that from di_size.
	 */
	ASSERT(leaf->hdr.count <= XFS_DIR2_MAX_LEAF_ENTS(mp));
	ltp = XFS_DIR2_LEAF_TAIL_P(mp, leaf);
	/*
	 * Leaves and bests don't overlap.
	 */
	ASSERT((char *)&leaf->ents[leaf->hdr.count] <=
	       (char *)XFS_DIR2_LEAF_BESTS_P(ltp));
	/*
	 * Check hash value order, count stale entries.
	 */
	for (i = stale = 0; i < leaf->hdr.count; i++) {
		if (i + 1 < leaf->hdr.count)
			ASSERT(leaf->ents[i].hashval <=
			       leaf->ents[i + 1].hashval);
		if (leaf->ents[i].address == XFS_DIR2_NULL_DATAPTR)
			stale++;
	}
	ASSERT(leaf->hdr.stale == stale);
}
#endif	/* DEBUG */

#if defined(XFS_REPAIR_SIM) || !defined(SIM)
/*
 * Compact out any stale entries in the leaf.
 * Log the header and changed leaf entries, if any.
 */
void
xfs_dir2_leaf_compact(
	xfs_da_args_t	*args,		/* operation arguments */
	xfs_dabuf_t	*bp)		/* leaf buffer */
{
	int		from;		/* source leaf index */
	xfs_dir2_leaf_t	*leaf;		/* leaf structure */
	int		loglow;		/* first leaf entry to log */
	int		to;		/* target leaf index */

	leaf = bp->data;
	if (leaf->hdr.stale == 0) {
#pragma mips_frequency_hint NEVER
		return;
	}
	/*
	 * Compress out the stale entries in place.
	 */
	for (from = to = 0, loglow = -1; from < leaf->hdr.count; from++) {
		if (leaf->ents[from].address == XFS_DIR2_NULL_DATAPTR)
			continue;
		/*
		 * Only actually copy the entries that are different.
		 */
		if (from > to) {
			if (loglow == -1)
				loglow = to;
			leaf->ents[to] = leaf->ents[from];
		}
		to++;
	}
	/*
	 * Update and log the header, log the leaf entries.
	 */
	ASSERT(leaf->hdr.stale == from - to);
	leaf->hdr.count -= leaf->hdr.stale;
	leaf->hdr.stale = 0;
	xfs_dir2_leaf_log_header(args->trans, bp);
	if (loglow != -1)
		xfs_dir2_leaf_log_ents(args->trans, bp, loglow, to - 1);
}
#endif /* XFS_REPAIR_SIM || !SIM */

/*
 * Compact the leaf entries, removing stale ones.
 * Leave one stale entry behind - the one closest to our
 * insertion index - and the caller will shift that one to our insertion
 * point later.
 * Return new insertion index, where the remaining stale entry is,
 * and leaf logging indices.
 */
void
xfs_dir2_leaf_compact_x1(
	xfs_dabuf_t	*bp,		/* leaf buffer */
	int		*indexp,	/* insertion index */
	int		*lowstalep,	/* out: stale entry before us */
	int		*highstalep,	/* out: stale entry after us */
	int		*lowlogp,	/* out: low log index */
	int		*highlogp)	/* out: high log index */
{
	int		from;		/* source copy index */
	int		highstale;	/* stale entry at/after index */
	int		index;		/* insertion index */
	int		keepstale;	/* source index of kept stale */
	xfs_dir2_leaf_t	*leaf;		/* leaf structure */
	int		lowstale;	/* stale entry before index */
	int		newindex;	/* new insertion index */
	int		to;		/* destination copy index */

	leaf = bp->data;
	ASSERT(leaf->hdr.stale > 1);
	index = *indexp;
	/*
	 * Find the first stale entry before our index, if any.
	 */
	for (lowstale = index - 1;
	     lowstale >= 0 &&
		leaf->ents[lowstale].address != XFS_DIR2_NULL_DATAPTR;
	     lowstale--)
		continue;
	/*
	 * Find the first stale entry at or after our index, if any.
	 * Stop if the answer would be worse than lowstale.
	 */
	for (highstale = index;
	     highstale < leaf->hdr.count &&
		leaf->ents[highstale].address != XFS_DIR2_NULL_DATAPTR &&
		(lowstale < 0 || index - lowstale > highstale - index);
	     highstale++)
		continue;
	/*
	 * Pick the better of lowstale and highstale.
	 */
	if (lowstale >= 0 &&
	    (highstale == leaf->hdr.count ||
	     index - lowstale <= highstale - index))
		keepstale = lowstale;
	else
		keepstale = highstale;
	/*
	 * Copy the entries in place, removing all the stale entries
	 * except keepstale.
	 */
	for (from = to = 0; from < leaf->hdr.count; from++) {
		/*
		 * Notice the new value of index.
		 */
		if (index == from)
			newindex = to;
		if (from != keepstale &&
		    leaf->ents[from].address == XFS_DIR2_NULL_DATAPTR) {
			if (from == to)
				*lowlogp = to;
			continue;
		}
		/*
		 * Record the new keepstale value for the insertion.
		 */
		if (from == keepstale)
			lowstale = highstale = to;
		/*
		 * Copy only the entries that have moved.
		 */
		if (from > to)
			leaf->ents[to] = leaf->ents[from];
		to++;
	}
	ASSERT(from > to);
	/*
	 * If the insertion point was past the last entry,
	 * set the new insertion point accordingly.
	 */
	if (index == from)
		newindex = to;
	*indexp = newindex;
	/*
	 * Adjust the leaf header values.
	 */
	leaf->hdr.count -= from - to;
	leaf->hdr.stale = 1;
	/*
	 * Remember the low/high stale value only in the "right"
	 * direction.
	 */
	if (lowstale >= newindex)
		lowstale = -1;
	else
		highstale = leaf->hdr.count;
	*highlogp = leaf->hdr.count - 1;
	*lowstalep = lowstale;
	*highstalep = highstale;
}

#ifndef SIM
/*
 * Getdents (readdir) for leaf and node directories.
 * This reads the data blocks only, so is the same for both forms.
 */
int						/* error */
xfs_dir2_leaf_getdents(
	xfs_trans_t		*tp,		/* transaction pointer */
	xfs_inode_t		*dp,		/* incore directory inode */
	uio_t			*uio,		/* I/O control & vectors */
	int			*eofp,		/* out: reached end of dir */
	dirent_t		*dbp,		/* caller's buffer */
	xfs_dir2_put_t		put)		/* ABI formatting routine */
{
	xfs_dabuf_t		*bp;		/* data block buffer */
	int			byteoff;	/* offset in current block */
	xfs_dir2_db_t		curdb;		/* db for current block */
	xfs_dir2_off_t		curoff;		/* current overall offset */
	xfs_dir2_data_t		*data;		/* data block structure */
	xfs_dir2_data_entry_t	*dep;		/* data entry */
	xfs_dir2_data_unused_t	*dup;		/* unused entry */
	int			eof;		/* reached end of directory */
	int			error;		/* error return value */
	int			i;		/* temporary loop index */
	int			j;		/* temporary loop index */
	int			length;		/* temporary length value */
	xfs_bmbt_irec_t		*map;		/* map vector for blocks */
	xfs_extlen_t		map_blocks;	/* number of fsbs in map */
	xfs_dablk_t		map_off;	/* last mapped file offset */
	int			map_size;	/* total entries in *map */
	int			map_valid;	/* valid entries in *map */
	xfs_mount_t		*mp;		/* filesystem mount point */
	xfs_dir2_off_t		newoff;		/* new curoff after new block */
	int			nmap;		/* mappings to ask xfs_bmapi */
	xfs_dir2_put_args_t	p;		/* formatting arg bundle */
	char			*ptr;		/* pointer to current data */
	int			ra_current;	/* number of read-ahead blks */
	int			ra_index;	/* *map index for read-ahead */
	int			ra_offset;	/* offset in map entry for ra */
	int			ra_want;	/* readahead count wanted */

	/*
	 * If the offset is at or past the largest allowed value,
	 * give up right away, return eof.
	 */
	if (uio->uio_offset >= XFS_DIR2_MAX_DATAPTR) {
#pragma mips_frequency_hint NEVER
		*eofp = 1;
		return 0;
	}
	mp = dp->i_mount;
	/*
	 * Setup formatting arguments.
	 */
	p.dbp = dbp;
	p.put = put;
	p.uio = uio;
	/*
	 * Set up to bmap a number of blocks based on the caller's
	 * buffer size, the directory block size, and the filesystem
	 * block size.
	 */
	map_size =
		howmany(uio->uio_resid + mp->m_dirblksize,
			mp->m_sb.sb_blocksize);
	map = kmem_alloc(map_size * sizeof(*map), KM_SLEEP);
	map_valid = ra_index = ra_offset = ra_current = map_blocks = 0;
	bp = NULL;
	eof = 1;
	/*
	 * Inside the loop we keep the main offset value as a byte offset
	 * in the directory file.
	 */
	curoff = XFS_DIR2_DATAPTR_TO_BYTE(mp, uio->uio_offset);
	/*
	 * Force this conversion through db so we truncate the offset
	 * down to get the start of the data block.
	 */
	map_off = XFS_DIR2_DB_TO_DA(mp, XFS_DIR2_BYTE_TO_DB(mp, curoff));
	/*
	 * Loop over directory entries until we reach the end offset.
	 * Get more blocks and readahead as necessary.
	 */
	while (curoff < XFS_DIR2_LEAF_OFFSET) {
		/*
		 * If we have no buffer, or we're off the end of the
		 * current buffer, need to get another one.
		 */
		if (!bp || ptr >= (char *)bp->data + mp->m_dirblksize) {
#pragma mips_frequency_hint NEVER
			/*
			 * If we have a buffer, we need to release it and
			 * take it out of the mapping.
			 */
			if (bp) {
				xfs_da_brelse(tp, bp);
				bp = NULL;
				map_blocks -= mp->m_dirblkfsbs;
				/*
				 * Loop to get rid of the extents for the
				 * directory block.
				 */
				for (i = mp->m_dirblkfsbs; i > 0; ) {
					j = MIN(map->br_blockcount, i);
					map->br_blockcount -= j;
					map->br_startblock += j;
					map->br_startoff += j;
					/*
					 * If mapping is done, pitch it from
					 * the table.
					 */
					if (!map->br_blockcount && --map_valid)
						ovbcopy(&map[1], &map[0],
							sizeof(map[0]) *
							map_valid);
					i -= j;
				}
			}
			/*
			 * Recalculate the readahead blocks wanted.
			 */
			ra_want = howmany(uio->uio_resid + mp->m_dirblksize,
					  mp->m_sb.sb_blocksize) - 1;
			/*
			 * If we don't have as many as we want, and we haven't
			 * run out of data blocks, get some more mappings.
			 */
			if (1 + ra_want > map_blocks &&
			    map_off <
			    XFS_DIR2_BYTE_TO_DA(mp, XFS_DIR2_LEAF_OFFSET)) {
				/*
				 * Get more bmaps, fill in after the ones
				 * we already have in the table.
				 */
				nmap = map_size - map_valid;
				error = xfs_bmapi(tp, dp,
					map_off,
					XFS_DIR2_BYTE_TO_DA(mp,
						XFS_DIR2_LEAF_OFFSET) - map_off,
					XFS_BMAPI_METADATA, NULL, 0,
					&map[map_valid], &nmap, NULL);
				/*
				 * Don't know if we should ignore this or
				 * try to return an error.
				 * The trouble with returning errors
				 * is that readdir will just stop without
				 * actually passing the error through.
				 */
				if (error)
					break;	/* XXX */
				/*
				 * If we got all the mappings we asked for,
				 * set the final map offset based on the
				 * last bmap value received.
				 * Otherwise, we've reached the end.
				 */
				if (nmap == map_size - map_valid)
					map_off =
					map[map_valid + nmap - 1].br_startoff +
					map[map_valid + nmap - 1].br_blockcount;
				else
					map_off =
						XFS_DIR2_BYTE_TO_DA(mp,
							XFS_DIR2_LEAF_OFFSET);
				/*
				 * Look for holes in the mapping, and
				 * eliminate them.  Count up the valid blocks.
				 */
				for (i = map_valid; i < map_valid + nmap; ) {
					if (map[i].br_startblock ==
					    HOLESTARTBLOCK) {
						nmap--;
						length = map_valid + nmap - i;
						if (length)
							ovbcopy(&map[i + 1],
								&map[i],
								sizeof(map[i]) *
								length);
					} else {
						map_blocks +=
							map[i].br_blockcount;
						i++;
					}
				}
				map_valid += nmap;
			}
			/*
			 * No valid mappings, so no more data blocks.
			 */
			if (!map_valid) {
				curoff = XFS_DIR2_DA_TO_BYTE(mp, map_off);
				break;
			}
			/*
			 * Read the directory block starting at the first
			 * mapping.
			 */
			curdb = XFS_DIR2_DA_TO_DB(mp, map->br_startoff);
			error = xfs_da_read_buf(tp, dp, map->br_startoff,
				map->br_blockcount >= mp->m_dirblkfsbs ?
				    XFS_FSB_TO_DADDR(mp, map->br_startblock) :
				    -1,
				&bp, XFS_DATA_FORK);
			/*
			 * Should just skip over the data block instead
			 * of giving up.
			 */
			if (error)
				break;	/* XXX */
			/*
			 * Adjust the current amount of read-ahead: we just
			 * read a block that was previously ra.
			 */
			if (ra_current)
				ra_current -= mp->m_dirblkfsbs;
			/*
			 * Do we need more readahead?
			 */
			for (ra_index = ra_offset = i = 0;
			     ra_want > ra_current && i < map_blocks;
			     i += mp->m_dirblkfsbs) {
				ASSERT(ra_index < map_valid);
				/*
				 * Read-ahead a contiguous directory block.
				 */
				if (i > ra_current &&
				    map[ra_index].br_blockcount >=
				    mp->m_dirblkfsbs) {
					baread(mp->m_ddev_targp,
						XFS_FSB_TO_DADDR(mp,
						   map[ra_index].br_startblock +
						   ra_offset),
						(int)BTOBB(mp->m_dirblksize));
					ra_current = i;
				}
				/*
				 * Read-ahead a non-contiguous directory block.
				 * This doesn't use our mapping, but this
				 * is a very rare case.
				 */
				else if (i > ra_current) {
					(void)xfs_da_reada_buf(tp, dp,
						map[ra_index].br_startoff +
						ra_offset, XFS_DATA_FORK);
					ra_current = i;
				}
				/*
				 * Advance offset through the mapping table.
				 */
				for (j = 0; j < mp->m_dirblkfsbs; j++) {
					/*
					 * The rest of this extent but not
					 * more than a dir block.
					 */
					length = MIN(mp->m_dirblkfsbs,
						map[ra_index].br_blockcount -
						ra_offset);
					j += length;
					ra_offset += length;
					/*
					 * Advance to the next mapping if
					 * this one is used up.
					 */
					if (ra_offset ==
					    map[ra_index].br_blockcount) {
						ra_offset = 0;
						ra_index++;
					}
				}
			}
			/*
			 * Having done a read, we need to set a new offset.
			 */
			newoff = XFS_DIR2_DB_OFF_TO_BYTE(mp, curdb, 0);
			/*
			 * Start of the current block.
			 */
			if (curoff < newoff)
				curoff = newoff;
			/*
			 * Make sure we're in the right block.
			 */
			else if (curoff > newoff)
				ASSERT(XFS_DIR2_BYTE_TO_DB(mp, curoff) ==
				       curdb);
			data = bp->data;
			xfs_dir2_data_check(dp, bp);
			/*
			 * Find our position in the block.
			 */
			ptr = (char *)&data->u;
			byteoff = XFS_DIR2_BYTE_TO_OFF(mp, curoff);
			/*
			 * Skip past the header.
			 */
			if (byteoff == 0)
				curoff += (uint)sizeof(data->hdr);
			/*
			 * Skip past entries until we reach our offset.
			 */
			else {
				while ((char *)ptr - (char *)data < byteoff) {
					dup = (xfs_dir2_data_unused_t *)ptr;
					if (dup->freetag ==
					    XFS_DIR2_DATA_FREE_TAG) {
						length = dup->length;
						ptr += length;
						continue;
					}
					dep = (xfs_dir2_data_entry_t *)ptr;
					length =
					    XFS_DIR2_DATA_ENTSIZE(dep->namelen);
					ptr += length;
				}
				/*
				 * Now set our real offset.
				 */
				curoff =
					XFS_DIR2_DB_OFF_TO_BYTE(mp,
						XFS_DIR2_BYTE_TO_DB(mp, curoff),
						(char *)ptr - (char *)data);
			}
		}
		/*
		 * We have a pointer to an entry.
		 * Is it a live one?
		 */
		dup = (xfs_dir2_data_unused_t *)ptr;
		/*
		 * No, it's unused, skip over it.
		 */
		if (dup->freetag == XFS_DIR2_DATA_FREE_TAG) {
			length = dup->length;
			ptr += length;
			curoff += length;
			continue;
		}
		/*
		 * Copy the entry into the putargs, and try formatting it.
		 */
		dep = (xfs_dir2_data_entry_t *)ptr;
		p.namelen = dep->namelen;
		length = XFS_DIR2_DATA_ENTSIZE(p.namelen);
		p.cook = XFS_DIR2_BYTE_TO_DATAPTR(mp, curoff + length);
#if XFS_BIG_FILESYSTEMS
		p.ino = dep->inumber + mp->m_inoadd;
#else
		p.ino = dep->inumber;
#endif
		p.name = (char *)dep->name;
		error = p.put(&p);
		/*
		 * Won't fit.  Return to caller.
		 */
		if (!p.done) {
#pragma mips_frequency_hint NEVER
			eof = 0;
			break;
		}
		/*
		 * Advance to next entry in the block.
		 */
		ptr += length;
		curoff += length;
	}
	/*
	 * All done.  Set output offset value to current offset.
	 */
	*eofp = eof;
	if (curoff > XFS_DIR2_DATAPTR_TO_BYTE(mp, XFS_DIR2_MAX_DATAPTR))
		uio->uio_offset = XFS_DIR2_MAX_DATAPTR;
	else
		uio->uio_offset = XFS_DIR2_BYTE_TO_DATAPTR(mp, curoff);
	kmem_free(map, map_size * sizeof(*map));
	if (bp)
		xfs_da_brelse(tp, bp);
	return error;
}
#endif	/* !SIM */

/*
 * Initialize a new leaf block, leaf1 or leafn magic accepted.
 */
int
xfs_dir2_leaf_init(
	xfs_da_args_t		*args,		/* operation arguments */
	xfs_dir2_db_t		bno,		/* directory block number */
	xfs_dabuf_t		**bpp,		/* out: leaf buffer */
	int			magic)		/* magic number for block */
{
	xfs_dabuf_t		*bp;		/* leaf buffer */
	xfs_inode_t		*dp;		/* incore directory inode */
	int			error;		/* error return code */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	xfs_dir2_leaf_tail_t	*ltp;		/* leaf tail structure */
	xfs_mount_t		*mp;		/* filesystem mount point */
	xfs_trans_t		*tp;		/* transaction pointer */

	dp = args->dp;
	ASSERT(dp != NULL);
	tp = args->trans;
	mp = dp->i_mount;
	ASSERT(bno >= XFS_DIR2_LEAF_FIRSTDB(mp) &&
	       bno < XFS_DIR2_FREE_FIRSTDB(mp));
	/*
	 * Get the buffer for the block.
	 */
	error = xfs_da_get_buf(tp, dp, XFS_DIR2_DB_TO_DA(mp, bno), -1, &bp,
		XFS_DATA_FORK);
	if (error) {
#pragma mips_frequency_hint NEVER
		return error;
	}
	ASSERT(bp != NULL);
	leaf = bp->data;
	/*
	 * Initialize the header.
	 */
	leaf->hdr.info.magic = magic;
	leaf->hdr.info.forw = leaf->hdr.info.back = 0;
	leaf->hdr.count = 0;
	leaf->hdr.stale = 0;
	xfs_dir2_leaf_log_header(tp, bp);
	/*
	 * If it's a leaf-format directory initialize the tail.
	 * In this case our caller has the real bests table to copy into
	 * the block.
	 */
	if (magic == XFS_DIR2_LEAF1_MAGIC) {
		ltp = XFS_DIR2_LEAF_TAIL_P(mp, leaf);
		ltp->bestcount = 0;
		xfs_dir2_leaf_log_tail(tp, bp);
	}
	*bpp = bp;
	return 0;
}

/*
 * Log the bests entries indicated from a leaf1 block.
 */
void
xfs_dir2_leaf_log_bests(
	xfs_trans_t		*tp,		/* transaction pointer */
	xfs_dabuf_t		*bp,		/* leaf buffer */
	int			first,		/* first entry to log */
	int			last)		/* last entry to log */
{
	xfs_dir2_data_off_t	*firstb;	/* pointer to first entry */
	xfs_dir2_data_off_t	*lastb;		/* pointer to last entry */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	xfs_dir2_leaf_tail_t	*ltp;		/* leaf tail structure */

	leaf = bp->data;
	ASSERT(leaf->hdr.info.magic == XFS_DIR2_LEAF1_MAGIC);
	ltp = XFS_DIR2_LEAF_TAIL_P(tp->t_mountp, leaf);
	firstb = XFS_DIR2_LEAF_BESTS_P(ltp) + first;
	lastb = XFS_DIR2_LEAF_BESTS_P(ltp) + last;
	xfs_da_log_buf(tp, bp, (uint)((char *)firstb - (char *)leaf),
		(uint)((char *)lastb - (char *)leaf + sizeof(*lastb) - 1));
}

/*
 * Log the leaf entries indicated from a leaf1 or leafn block.
 */
void
xfs_dir2_leaf_log_ents(
	xfs_trans_t		*tp,		/* transaction pointer */
	xfs_dabuf_t		*bp,		/* leaf buffer */
	int			first,		/* first entry to log */
	int			last)		/* last entry to log */
{
	xfs_dir2_leaf_entry_t	*firstlep;	/* pointer to first entry */
	xfs_dir2_leaf_entry_t	*lastlep;	/* pointer to last entry */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */

	leaf = bp->data;
	ASSERT(leaf->hdr.info.magic == XFS_DIR2_LEAF1_MAGIC ||
	       leaf->hdr.info.magic == XFS_DIR2_LEAFN_MAGIC);
	firstlep = &leaf->ents[first];
	lastlep = &leaf->ents[last];
	xfs_da_log_buf(tp, bp, (uint)((char *)firstlep - (char *)leaf),
		(uint)((char *)lastlep - (char *)leaf + sizeof(*lastlep) - 1));
}

/*
 * Log the header of the leaf1 or leafn block.
 */
void
xfs_dir2_leaf_log_header(
	xfs_trans_t		*tp,		/* transaction pointer */
	xfs_dabuf_t		*bp)		/* leaf buffer */
{
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */

	leaf = bp->data;
	ASSERT(leaf->hdr.info.magic == XFS_DIR2_LEAF1_MAGIC ||
	       leaf->hdr.info.magic == XFS_DIR2_LEAFN_MAGIC);
	xfs_da_log_buf(tp, bp, (uint)((char *)&leaf->hdr - (char *)leaf),
		(uint)(sizeof(leaf->hdr) - 1));
}

/*
 * Log the tail of the leaf1 block.
 */
void
xfs_dir2_leaf_log_tail(
	xfs_trans_t		*tp,		/* transaction pointer */
	xfs_dabuf_t		*bp)		/* leaf buffer */
{
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	xfs_dir2_leaf_tail_t	*ltp;		/* leaf tail structure */
	xfs_mount_t		*mp;		/* filesystem mount point */

	mp = tp->t_mountp;
	leaf = bp->data;
	ASSERT(leaf->hdr.info.magic == XFS_DIR2_LEAF1_MAGIC);
	ltp = XFS_DIR2_LEAF_TAIL_P(mp, leaf);
	xfs_da_log_buf(tp, bp, (uint)((char *)ltp - (char *)leaf),
		(uint)(mp->m_dirblksize - 1));
}

/*
 * Look up the entry referred to by args in the leaf format directory.
 * Most of the work is done by the xfs_dir2_leaf_lookup_int routine which
 * is also used by the node-format code.
 */
int
xfs_dir2_leaf_lookup(
	xfs_da_args_t		*args)		/* operation arguments */
{
	xfs_dabuf_t		*dbp;		/* data block buffer */
	xfs_dir2_data_entry_t	*dep;		/* data block entry */
	xfs_inode_t		*dp;		/* incore directory inode */
	int			error;		/* error return code */
	int			index;		/* found entry index */
	xfs_dabuf_t		*lbp;		/* leaf buffer */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	xfs_dir2_leaf_entry_t	*lep;		/* leaf entry */
	xfs_trans_t		*tp;		/* transaction pointer */

	xfs_dir2_trace_args("leaf_lookup", args);
	/*
	 * Look up name in the leaf block, returning both buffers and index.
	 */
	if (error = xfs_dir2_leaf_lookup_int(args, &lbp, &index, &dbp)) {
#pragma mips_frequency_hint NEVER
		return error;
	}
	tp = args->trans;
	dp = args->dp;
	xfs_dir2_leaf_check(dp, lbp);
	leaf = lbp->data;
	/*
	 * Get to the leaf entry and contained data entry address.
	 */
	lep = &leaf->ents[index];
	/*
	 * Point to the data entry.
	 */
	dep = (xfs_dir2_data_entry_t *)
	      ((char *)dbp->data +
	       XFS_DIR2_DATAPTR_TO_OFF(dp->i_mount, lep->address));
	/*
	 * Return the found inode number.
	 */
	args->inumber = dep->inumber;
	xfs_da_brelse(tp, dbp);
	xfs_da_brelse(tp, lbp);
	return XFS_ERROR(EEXIST);
}

/*
 * Look up name/hash in the leaf block.
 * Fill in indexp with the found index, and dbpp with the data buffer.
 * If not found dbpp will be NULL, and ENOENT comes back.
 * lbpp will always be filled in with the leaf buffer unless there's an error.
 */
static int					/* error */
xfs_dir2_leaf_lookup_int(
	xfs_da_args_t		*args,		/* operation arguments */
	xfs_dabuf_t		**lbpp,		/* out: leaf buffer */
	int			*indexp,	/* out: index in leaf block */
	xfs_dabuf_t		**dbpp)		/* out: data buffer */
{
	xfs_dir2_db_t		curdb;		/* current data block number */
	xfs_dabuf_t		*dbp;		/* data buffer */
	xfs_dir2_data_entry_t	*dep;		/* data entry */
	xfs_inode_t		*dp;		/* incore directory inode */
	int			error;		/* error return code */
	int			index;		/* index in leaf block */
	xfs_dabuf_t		*lbp;		/* leaf buffer */
	xfs_dir2_leaf_entry_t	*lep;		/* leaf entry */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	xfs_mount_t		*mp;		/* filesystem mount point */
	xfs_dir2_db_t		newdb;		/* new data block number */
	xfs_trans_t		*tp;		/* transaction pointer */

	dp = args->dp;
	tp = args->trans;
	mp = dp->i_mount;
	/*
	 * Read the leaf block into the buffer.
	 */
	if (error =
	    xfs_da_read_buf(tp, dp, mp->m_dirleafblk, -1, &lbp,
		    XFS_DATA_FORK)) {
#pragma mips_frequency_hint NEVER
		return error;
	}
	*lbpp = lbp;
	leaf = lbp->data;
	xfs_dir2_leaf_check(dp, lbp);
	/*
	 * Look for the first leaf entry with our hash value.
	 */
	index = xfs_dir2_leaf_search_hash(args, lbp);
	/*
	 * Loop over all the entries with the right hash value
	 * looking to match the name.
	 */
	for (lep = &leaf->ents[index], dbp = NULL, curdb = -1;
	     index < leaf->hdr.count && lep->hashval == args->hashval;
	     lep++, index++) {
		/*
		 * Skip over stale leaf entries.
		 */
		if (lep->address == XFS_DIR2_NULL_DATAPTR)
			continue;
		/*
		 * Get the new data block number.
		 */
		newdb = XFS_DIR2_DATAPTR_TO_DB(mp, lep->address);
		/*
		 * If it's not the same as the old data block number,
		 * need to pitch the old one and read the new one.
		 */
		if (newdb != curdb) {
			if (dbp)
				xfs_da_brelse(tp, dbp);
			if (error =
			    xfs_da_read_buf(tp, dp,
				    XFS_DIR2_DB_TO_DA(mp, newdb), -1, &dbp,
				    XFS_DATA_FORK)) {
#pragma mips_frequency_hint NEVER
				xfs_da_brelse(tp, lbp);
				return error;
			}
			xfs_dir2_data_check(dp, dbp);
			curdb = newdb;
		}
		/*
		 * Point to the data entry.
		 */
		dep = (xfs_dir2_data_entry_t *)
		      ((char *)dbp->data +
		       XFS_DIR2_DATAPTR_TO_OFF(mp, lep->address));
		/*
		 * If it matches then return it.
		 */
		if (dep->namelen == args->namelen &&
		    dep->name[0] == args->name[0] &&
		    bcmp(dep->name, args->name, args->namelen) == 0) {
			*dbpp = dbp;
			*indexp = index;
			return 0;
		}
	}
	/*
	 * No match found, return ENOENT.
	 */
	ASSERT(args->oknoent);
	if (dbp)
		xfs_da_brelse(tp, dbp);
	xfs_da_brelse(tp, lbp);
	return XFS_ERROR(ENOENT);
}

#if defined(XFS_REPAIR_SIM) || !defined(SIM)
/*
 * Remove an entry from a leaf format directory.
 */
int						/* error */
xfs_dir2_leaf_removename(
	xfs_da_args_t		*args)		/* operation arguments */
{
	xfs_dir2_data_off_t	*bestsp;	/* leaf block best freespace */
	xfs_dir2_data_t		*data;		/* data block structure */
	xfs_dir2_db_t		db;		/* data block number */
	xfs_dabuf_t		*dbp;		/* data block buffer */
	xfs_dir2_data_entry_t	*dep;		/* data entry structure */
	xfs_inode_t		*dp;		/* incore directory inode */
	int			error;		/* error return code */
	xfs_dir2_db_t		i;		/* temporary data block # */
	int			index;		/* index into leaf entries */
	xfs_dabuf_t		*lbp;		/* leaf buffer */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	xfs_dir2_leaf_entry_t	*lep;		/* leaf entry */
	xfs_dir2_leaf_tail_t	*ltp;		/* leaf tail structure */
	xfs_mount_t		*mp;		/* filesystem mount point */
	int			needlog;	/* need to log data header */
	int			needscan;	/* need to rescan data frees */
	xfs_dir2_data_off_t	oldbest;	/* old value of best free */
	xfs_trans_t		*tp;		/* transaction pointer */

	xfs_dir2_trace_args("leaf_removename", args);
	/*
	 * Lookup the leaf entry, get the leaf and data blocks read in.
	 */
	if (error = xfs_dir2_leaf_lookup_int(args, &lbp, &index, &dbp)) {
#pragma mips_frequency_hint NEVER
		return error;
	}
	dp = args->dp;
	tp = args->trans;
	mp = dp->i_mount;
	leaf = lbp->data;
	data = dbp->data;
	xfs_dir2_data_check(dp, dbp);
	/*
	 * Point to the leaf entry, use that to point to the data entry.
	 */
	lep = &leaf->ents[index];
	db = XFS_DIR2_DATAPTR_TO_DB(mp, lep->address);
	dep = (xfs_dir2_data_entry_t *)
	      ((char *)data + XFS_DIR2_DATAPTR_TO_OFF(mp, lep->address));
	needscan = needlog = 0;
	oldbest = data->hdr.bestfree[0].length;
	ltp = XFS_DIR2_LEAF_TAIL_P(mp, leaf);
	bestsp = XFS_DIR2_LEAF_BESTS_P(ltp);
	ASSERT(bestsp[db] == oldbest);
	/*
	 * Mark the former data entry unused.
	 */
	xfs_dir2_data_make_free(tp, dbp,
		(xfs_dir2_data_aoff_t)((char *)dep - (char *)data),
		XFS_DIR2_DATA_ENTSIZE(dep->namelen), &needlog, &needscan);
	/*
	 * We just mark the leaf entry stale by putting a null in it.
	 */
	leaf->hdr.stale++;
	xfs_dir2_leaf_log_header(tp, lbp);
	lep->address = XFS_DIR2_NULL_DATAPTR;
	xfs_dir2_leaf_log_ents(tp, lbp, index, index);
	/*
	 * Scan the freespace in the data block again if necessary,
	 * log the data block header if necessary.
	 */
	if (needscan)
		xfs_dir2_data_freescan(mp, data, &needlog, NULL);
	if (needlog)
		xfs_dir2_data_log_header(tp, dbp);
	/*
	 * If the longest freespace in the data block has changed,
	 * put the new value in the bests table and log that.
	 */
	if (data->hdr.bestfree[0].length != oldbest) {
		bestsp[db] = data->hdr.bestfree[0].length;
		xfs_dir2_leaf_log_bests(tp, lbp, db, db);
	}
	xfs_dir2_data_check(dp, dbp);
	/*
	 * If the data block is now empty then get rid of the data block.
	 */
	if (data->hdr.bestfree[0].length ==
	    mp->m_dirblksize - (uint)sizeof(data->hdr)) {
#pragma mips_frequency_hint NEVER
		ASSERT(db != mp->m_dirdatablk);
		if (error = xfs_dir2_shrink_inode(args, db, dbp)) {
			/*
			 * Nope, can't get rid of it because it caused
			 * allocation of a bmap btree block to do so.
			 * Just go on, returning success, leaving the
			 * empty block in place.
			 */
			if (error == ENOSPC && args->total == 0) {
				xfs_da_buf_done(dbp);
				error = 0;
			}
			xfs_dir2_leaf_check(dp, lbp);
			xfs_da_buf_done(lbp);
			return error;
		}
		dbp = NULL;
		/*
		 * If this is the last data block then compact the
		 * bests table by getting rid of entries.
		 */
		if (db == ltp->bestcount - 1) {
			/*
			 * Look for the last active entry (i).
			 */
			for (i = db - 1; i > 0; i--) {
				if (bestsp[i] != NULLDATAOFF)
					break;
			}
			/*
			 * Copy the table down so inactive entries at the
			 * end are removed.
			 */
			ovbcopy(bestsp, &bestsp[db - i],
				(ltp->bestcount - (db - i)) * sizeof(*bestsp));
			ltp->bestcount -= db - i;
			xfs_dir2_leaf_log_tail(tp, lbp);
			xfs_dir2_leaf_log_bests(tp, lbp, 0, ltp->bestcount - 1);
		} else
			bestsp[db] = NULLDATAOFF;
	}
	/*
	 * If the data block was not the first one, drop it.
	 */
	else if (db != mp->m_dirdatablk && dbp != NULL) {
		xfs_da_buf_done(dbp);
		dbp = NULL;
	}
	xfs_dir2_leaf_check(dp, lbp);
	/*
	 * See if we can convert to block form.
	 */
	return xfs_dir2_leaf_to_block(args, lbp, dbp);
}

/*
 * Replace the inode number in a leaf format directory entry.
 */
int						/* error */
xfs_dir2_leaf_replace(
	xfs_da_args_t		*args)		/* operation arguments */
{
	xfs_dabuf_t		*dbp;		/* data block buffer */
	xfs_dir2_data_entry_t	*dep;		/* data block entry */
	xfs_inode_t		*dp;		/* incore directory inode */
	int			error;		/* error return code */
	int			index;		/* index of leaf entry */
	xfs_dabuf_t		*lbp;		/* leaf buffer */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	xfs_dir2_leaf_entry_t	*lep;		/* leaf entry */
	xfs_trans_t		*tp;		/* transaction pointer */

	xfs_dir2_trace_args("leaf_replace", args);
	/*
	 * Look up the entry.
	 */
	if (error = xfs_dir2_leaf_lookup_int(args, &lbp, &index, &dbp)) {
#pragma mips_frequency_hint NEVER
		return error;
	}
	dp = args->dp;
	leaf = lbp->data;
	/*
	 * Point to the leaf entry, get data address from it.
	 */
	lep = &leaf->ents[index];
	/*
	 * Point to the data entry.
	 */
	dep = (xfs_dir2_data_entry_t *)
	      ((char *)dbp->data +
	       XFS_DIR2_DATAPTR_TO_OFF(dp->i_mount, lep->address));
	ASSERT(args->inumber != dep->inumber);
	/*
	 * Put the new inode number in, log it.
	 */
	dep->inumber = args->inumber;
	tp = args->trans;
	xfs_dir2_data_log_entry(tp, dbp, dep);
	xfs_da_buf_done(dbp);
	xfs_dir2_leaf_check(dp, lbp);
	xfs_da_brelse(tp, lbp);
	return 0;
}
#endif /* XFS_REPAIR_SIM || !SIM */

/*
 * Return index in the leaf block (lbp) which is either the first
 * one with this hash value, or if there are none, the insert point
 * for that hash value.
 */
int						/* index value */
xfs_dir2_leaf_search_hash(
	xfs_da_args_t		*args,		/* operation arguments */
	xfs_dabuf_t		*lbp)		/* leaf buffer */
{
	xfs_dahash_t		hash;		/* hash from this entry */
	xfs_dahash_t		hashwant;	/* hash value looking for */
	int			high;		/* high leaf index */
	int			low;		/* low leaf index */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	xfs_dir2_leaf_entry_t	*lep;		/* leaf entry */
	int			mid;		/* current leaf index */

	leaf = lbp->data;
#ifdef XFS_REPAIR_SIM
	if (leaf->hdr.count == 0)
		return 0;
#endif
	/*
	 * Note, the table cannot be empty, so we have to go through the loop.
	 * Binary search the leaf entries looking for our hash value.
	 */
	for (lep = leaf->ents, low = 0, high = leaf->hdr.count - 1,
		hashwant = args->hashval;
	     low <= high; ) {
		mid = (low + high) >> 1;
		if ((hash = lep[mid].hashval) == hashwant)
			break;
		if (hash < hashwant)
			low = mid + 1;
		else
			high = mid - 1;
	}
	/*
	 * Found one, back up through all the equal hash values.
	 */
	if (hash == hashwant) {
		while (mid > 0 && lep[mid - 1].hashval == hashwant) {
#pragma mips_frequency_hint NEVER
			mid--;
		}
	}
	/*
	 * Need to point to an entry higher than ours.
	 */
	else if (hash < hashwant)
		mid++;
	return mid;
}

#if defined(XFS_REPAIR_SIM) || !defined(SIM)
/*
 * Trim off a trailing data block.  We know it's empty since the leaf
 * freespace table says so.
 */
int						/* error */
xfs_dir2_leaf_trim_data(
	xfs_da_args_t		*args,		/* operation arguments */
	xfs_dabuf_t		*lbp,		/* leaf buffer */
	xfs_dir2_db_t		db)		/* data block number */
{
	xfs_dir2_data_off_t	*bestsp;	/* leaf bests table */
#ifdef DEBUG
	xfs_dir2_data_t		*data;		/* data block structure */
#endif
	xfs_dabuf_t		*dbp;		/* data block buffer */
	xfs_inode_t		*dp;		/* incore directory inode */
	int			error;		/* error return value */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	xfs_dir2_leaf_tail_t	*ltp;		/* leaf tail structure */
	xfs_mount_t		*mp;		/* filesystem mount point */
	xfs_trans_t		*tp;		/* transaction pointer */

	dp = args->dp;
	mp = dp->i_mount;
	tp = args->trans;
	/*
	 * Read the offending data block.  We need its buffer.
	 */
	if (error = xfs_da_read_buf(tp, dp, XFS_DIR2_DB_TO_DA(mp, db), -1, &dbp,
			XFS_DATA_FORK)) {
#pragma mips_frequency_hint NEVER
		return error;
	}
#ifdef DEBUG
	data = dbp->data;
	ASSERT(data->hdr.magic == XFS_DIR2_DATA_MAGIC);
#endif
	leaf = lbp->data;
	ltp = XFS_DIR2_LEAF_TAIL_P(mp, leaf);
	ASSERT(data->hdr.bestfree[0].length ==
	       mp->m_dirblksize - (uint)sizeof(data->hdr));
	ASSERT(db == ltp->bestcount - 1);
	/*
	 * Get rid of the data block.
	 */
	if (error = xfs_dir2_shrink_inode(args, db, dbp)) {
#pragma mips_frequency_hint NEVER
		ASSERT(error != ENOSPC);
		xfs_da_brelse(tp, dbp);
		return error;
	}
	/*
	 * Eliminate the last bests entry from the table.
	 */
	bestsp = XFS_DIR2_LEAF_BESTS_P(ltp);
	ltp->bestcount--;
	ovbcopy(&bestsp[0], &bestsp[1], ltp->bestcount * sizeof(*bestsp));
	xfs_dir2_leaf_log_tail(tp, lbp);
	xfs_dir2_leaf_log_bests(tp, lbp, 0, ltp->bestcount - 1);
	return 0;
}

/*
 * Convert node form directory to leaf form directory.
 * The root of the node form dir needs to already be a LEAFN block.
 * Just return if we can't do anything.
 */
int						/* error */
xfs_dir2_node_to_leaf(
	xfs_da_state_t		*state)		/* directory operation state */
{
	xfs_da_args_t		*args;		/* operation arguments */
	xfs_inode_t		*dp;		/* incore directory inode */
	int			error;		/* error return code */
	xfs_dabuf_t		*fbp;		/* buffer for freespace block */
	xfs_fileoff_t		fo;		/* freespace file offset */
	xfs_dir2_free_t		*free;		/* freespace structure */
	xfs_dabuf_t		*lbp;		/* buffer for leaf block */
	xfs_dir2_leaf_tail_t	*ltp;		/* tail of leaf structure */
	xfs_dir2_leaf_t		*leaf;		/* leaf structure */
	xfs_mount_t		*mp;		/* filesystem mount point */
	int			rval;		/* successful free trim? */
	xfs_trans_t		*tp;		/* transaction pointer */

	/*
	 * There's more than a leaf level in the btree, so there must
	 * be multiple leafn blocks.  Give up.
	 */
	if (state->path.active > 1)
		return 0;
	args = state->args;
	xfs_dir2_trace_args("node_to_leaf", args);
	mp = state->mp;
	dp = args->dp;
	tp = args->trans;
	/*
	 * Get the last offset in the file.
	 */
	if (error = xfs_bmap_last_offset(tp, dp, &fo, XFS_DATA_FORK)) {
#pragma mips_frequency_hint NEVER
		return error;
	}
	fo -= mp->m_dirblkfsbs;
	/*
	 * If there are freespace blocks other than the first one,
	 * take this opportunity to remove trailing empty freespace blocks
	 * that may have been left behind during no-space-reservation
	 * operations.
	 */
	while (fo > mp->m_dirfreeblk) {
		if (error = xfs_dir2_node_trim_free(args, fo, &rval)) {
#pragma mips_frequency_hint NEVER
			return error;
		}
		if (rval)
			fo -= mp->m_dirblkfsbs;
		else
			return 0;
	}
	/*
	 * Now find the block just before the freespace block.
	 */
	if (error = xfs_bmap_last_before(tp, dp, &fo, XFS_DATA_FORK)) {
#pragma mips_frequency_hint NEVER
		return error;
	}
	/*
	 * If it's not the single leaf block, give up.
	 */
	if (XFS_FSB_TO_B(mp, fo) > XFS_DIR2_LEAF_OFFSET + mp->m_dirblksize)
		return 0;
	lbp = state->path.blk[0].bp;
	leaf = lbp->data;
	ASSERT(leaf->hdr.info.magic == XFS_DIR2_LEAFN_MAGIC);
	/*
	 * Read the freespace block.
	 */
	if (error = xfs_da_read_buf(tp, dp, mp->m_dirfreeblk, -1, &fbp,
			XFS_DATA_FORK)) {
#pragma mips_frequency_hint NEVER
		return error;
	}
	free = fbp->data;
	ASSERT(free->hdr.magic == XFS_DIR2_FREE_MAGIC);
	ASSERT(free->hdr.firstdb == 0);
	/*
	 * Now see if the leafn and free data will fit in a leaf1.
	 * If not, release the buffer and give up.
	 */
	if ((uint)sizeof(leaf->hdr) +
	    (leaf->hdr.count - leaf->hdr.stale) * (uint)sizeof(leaf->ents[0]) +
	    free->hdr.nvalid * (uint)sizeof(leaf->bests[0]) +
	    (uint)sizeof(leaf->tail) >
	    mp->m_dirblksize) {
		xfs_da_brelse(tp, fbp);
		return 0;
	}
	/*
	 * If the leaf has any stale entries in it, compress them out.
	 * The compact routine will log the header.
	 */
	if (leaf->hdr.stale)
		xfs_dir2_leaf_compact(args, lbp);
	else
		xfs_dir2_leaf_log_header(tp, lbp);
	leaf->hdr.info.magic = XFS_DIR2_LEAF1_MAGIC;
	/*
	 * Set up the leaf tail from the freespace block.
	 */
	ltp = XFS_DIR2_LEAF_TAIL_P(mp, leaf);
	ltp->bestcount = free->hdr.nvalid;
	/*
	 * Set up the leaf bests table.
	 */
	bcopy(free->bests, XFS_DIR2_LEAF_BESTS_P(ltp),
		ltp->bestcount * sizeof(leaf->bests[0]));
	xfs_dir2_leaf_log_bests(tp, lbp, 0, ltp->bestcount - 1);
	xfs_dir2_leaf_log_tail(tp, lbp);
	xfs_dir2_leaf_check(dp, lbp);
	/*
	 * Get rid of the freespace block.
	 */
	error = xfs_dir2_shrink_inode(args, XFS_DIR2_FREE_FIRSTDB(mp), fbp);
	if (error) {
#pragma mips_frequency_hint NEVER
		/*
		 * This can't fail here because it can only happen when
		 * punching out the middle of an extent, and this is an
		 * isolated block.
		 */
		ASSERT(error != ENOSPC);
		return error;
	}
	fbp = NULL;
	/*
	 * Now see if we can convert the single-leaf directory
	 * down to a block form directory.
	 * This routine always kills the dabuf for the leaf, so
	 * eliminate it from the path.
	 */
	error = xfs_dir2_leaf_to_block(args, lbp, NULL);
	state->path.blk[0].bp = NULL;
	return error;
}
#endif /* XFS_REPAIR_SIM || !SIM */
