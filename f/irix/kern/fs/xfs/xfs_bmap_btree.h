#ifndef _FS_XFS_BMAP_BTREE_H
#define	_FS_XFS_BMAP_BTREE_H

#ident "$Revision: 1.39 $"

#define	XFS_BMAP_MAGIC	0x424d4150	/* 'BMAP' */

struct buf;
struct xfs_btree_cur;
struct xfs_btree_lblock;
struct xfs_mount;
struct xfs_inode;

/*
 * Bmap root header, on-disk form only.
 */
typedef struct xfs_bmdr_block
{
	__uint16_t	bb_level;	/* 0 is a leaf */
	__uint16_t	bb_numrecs;	/* current # of data records */
} xfs_bmdr_block_t;

/*
 * Bmap btree record and extent descriptor.
 * For 32-bit kernels,
 *  l0:31 is an extent flag (value 1 indicates non-normal).
 *  l0:0-30 and l1:9-31 are startoff.
 *  l1:0-8, l2:0-31, and l3:21-31 are startblock.
 *  l3:0-20 are blockcount.
 * For 64-bit kernels,
 *  l0:63 is an extent flag (value 1 indicates non-normal).
 *  l0:9-62 are startoff.
 *  l0:0-8 and l1:21-63 are startblock.
 *  l1:0-20 are blockcount.
 */
#define	BMBT_TOTAL_BITLEN	128	/* 128 bits, 16 bytes */
#define	BMBT_EXNTFLAG_BITOFF	0
#define	BMBT_EXNTFLAG_BITLEN	1
#define	BMBT_STARTOFF_BITOFF	(BMBT_EXNTFLAG_BITOFF + BMBT_EXNTFLAG_BITLEN)
#define	BMBT_STARTOFF_BITLEN	54
#define	BMBT_STARTBLOCK_BITOFF	(BMBT_STARTOFF_BITOFF + BMBT_STARTOFF_BITLEN)
#define	BMBT_STARTBLOCK_BITLEN	52
#define	BMBT_BLOCKCOUNT_BITOFF	\
	(BMBT_STARTBLOCK_BITOFF + BMBT_STARTBLOCK_BITLEN)
#define	BMBT_BLOCKCOUNT_BITLEN	(BMBT_TOTAL_BITLEN - BMBT_BLOCKCOUNT_BITOFF)

#define	BMBT_USE_64	(_MIPS_SIM == _ABI64 || _MIPS_SIM == _ABIN32)

typedef struct xfs_bmbt_rec_32
{
	__uint32_t		l0, l1, l2, l3;
} xfs_bmbt_rec_32_t;
typedef struct xfs_bmbt_rec_64
{
	__uint64_t		l0, l1;
} xfs_bmbt_rec_64_t;

#if BMBT_USE_64
typedef	__uint64_t	xfs_bmbt_rec_base_t;	/* use this for casts */
typedef xfs_bmbt_rec_64_t xfs_bmbt_rec_t, xfs_bmdr_rec_t;
#else	/* !BMBT_USE_64 */
typedef	__uint32_t	xfs_bmbt_rec_base_t;	/* use this for casts */
typedef xfs_bmbt_rec_32_t xfs_bmbt_rec_t, xfs_bmdr_rec_t;
#endif	/* BMBT_USE_64 */

/*
 * Values and macros for delayed-allocation startblock fields.
 */
#define	STARTBLOCKVALBITS	17
#define	STARTBLOCKMASKBITS	(15 + XFS_BIG_FILESYSTEMS * 20)
#define	DSTARTBLOCKMASKBITS	(15 + 20)
#define	STARTBLOCKMASK		\
	(((((xfs_fsblock_t)1) << STARTBLOCKMASKBITS) - 1) << STARTBLOCKVALBITS)
#define	DSTARTBLOCKMASK		\
	(((((xfs_dfsbno_t)1) << DSTARTBLOCKMASKBITS) - 1) << STARTBLOCKVALBITS)
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_ISNULLSTARTBLOCK)
int isnullstartblock(xfs_fsblock_t x);
#define	ISNULLSTARTBLOCK(x)	isnullstartblock(x)
#else
#define	ISNULLSTARTBLOCK(x)	(((x) & STARTBLOCKMASK) == STARTBLOCKMASK)
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_ISNULLDSTARTBLOCK)
int isnulldstartblock(xfs_dfsbno_t x);
#define	ISNULLDSTARTBLOCK(x)	isnulldstartblock(x)
#else
#define	ISNULLDSTARTBLOCK(x)	(((x) & DSTARTBLOCKMASK) == DSTARTBLOCKMASK)
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_NULLSTARTBLOCK)
xfs_fsblock_t nullstartblock(int k);
#define	NULLSTARTBLOCK(k)	nullstartblock(k)
#else
#define	NULLSTARTBLOCK(k)	\
	((ASSERT(k < (1 << STARTBLOCKVALBITS))), (STARTBLOCKMASK | (k)))
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_STARTBLOCKVAL)
xfs_filblks_t startblockval(xfs_fsblock_t x);
#define	STARTBLOCKVAL(x)	startblockval(x)
#else
#define	STARTBLOCKVAL(x)	((xfs_filblks_t)((x) & ~STARTBLOCKMASK))
#endif

/*
 * Possible extent formats.
 */
typedef	enum {
	XFS_EXTFMT_NOSTATE = 0,
	XFS_EXTFMT_HASSTATE
} xfs_exntfmt_t;

/*
 * Possible extent states.
 */
typedef	enum {
	XFS_EXT_NORM, XFS_EXT_UNWRITTEN,
	XFS_EXT_DMAPI_OFFLINE
} xfs_exntst_t;

/*
 * Extent state and extent format macros.
 */
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_EXTFMT_INODE )
xfs_exntfmt_t xfs_extfmt_inode(struct xfs_inode *ip);
#define	XFS_EXTFMT_INODE(x)	xfs_extfmt_inode(x)
#else
#define	XFS_EXTFMT_INODE(x) \
  (XFS_SB_VERSION_HASEXTFLGBIT(&((x)->i_mount->m_sb)) ? \
	XFS_EXTFMT_HASSTATE : XFS_EXTFMT_NOSTATE)
#endif
#define	ISUNWRITTEN(x)		((x) == XFS_EXT_UNWRITTEN)

/*
 * Incore version of above.
 */
typedef struct xfs_bmbt_irec
{
	xfs_fileoff_t	br_startoff;	/* starting file offset */
	xfs_fsblock_t	br_startblock;	/* starting block number */
	xfs_filblks_t	br_blockcount;	/* number of blocks */
	xfs_exntst_t	br_state;	/* extent state */
} xfs_bmbt_irec_t;

/*
 * Key structure for non-leaf levels of the tree.
 */
typedef struct xfs_bmbt_key
{
	xfs_dfiloff_t	br_startoff;	/* starting file offset */
} xfs_bmbt_key_t, xfs_bmdr_key_t;

typedef xfs_dfsbno_t xfs_bmbt_ptr_t, xfs_bmdr_ptr_t;	/* btree pointer type */
					/* btree block header type */
typedef	struct xfs_btree_lblock xfs_bmbt_block_t;

#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BUF_TO_BMBT_BLOCK)
xfs_bmbt_block_t *xfs_buf_to_bmbt_block(struct buf *bp);
#define	XFS_BUF_TO_BMBT_BLOCK(bp)		xfs_buf_to_bmbt_block(bp)
#else
#define	XFS_BUF_TO_BMBT_BLOCK(bp) ((xfs_bmbt_block_t *)((bp)->b_un.b_addr))
#endif

#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_RBLOCK_DSIZE)
int xfs_bmap_rblock_dsize(int lev, struct xfs_btree_cur *cur);
#define	XFS_BMAP_RBLOCK_DSIZE(lev,cur)		xfs_bmap_rblock_dsize(lev,cur)
#else
#define	XFS_BMAP_RBLOCK_DSIZE(lev,cur) ((cur)->bc_private.b.forksize)
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_RBLOCK_ISIZE)
int xfs_bmap_rblock_isize(int lev, struct xfs_btree_cur *cur);
#define	XFS_BMAP_RBLOCK_ISIZE(lev,cur)		xfs_bmap_rblock_isize(lev,cur)
#else
#define	XFS_BMAP_RBLOCK_ISIZE(lev,cur) \
	((int)XFS_IFORK_PTR((cur)->bc_private.b.ip, \
			    (cur)->bc_private.b.whichfork)->if_broot_bytes)
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_IBLOCK_SIZE)
int xfs_bmap_iblock_size(int lev, struct xfs_btree_cur *cur);
#define	XFS_BMAP_IBLOCK_SIZE(lev,cur) 		xfs_bmap_iblock_size(lev,cur)
#else
#define	XFS_BMAP_IBLOCK_SIZE(lev,cur) (1 << (cur)->bc_blocklog)
#endif

#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BLOCK_DSIZE)
int xfs_bmap_block_dsize(int lev, struct xfs_btree_cur *cur);
#define	XFS_BMAP_BLOCK_DSIZE(lev,cur)		xfs_bmap_block_dsize(lev,cur)
#else
#define	XFS_BMAP_BLOCK_DSIZE(lev,cur) \
	((lev) == (cur)->bc_nlevels - 1 ? \
		XFS_BMAP_RBLOCK_DSIZE(lev,cur) : \
		XFS_BMAP_IBLOCK_SIZE(lev,cur))
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BLOCK_ISIZE)
int xfs_bmap_block_isize(int lev, struct xfs_btree_cur *cur);
#define	XFS_BMAP_BLOCK_ISIZE(lev,cur)		xfs_bmap_block_isize(lev,cur)
#else
#define	XFS_BMAP_BLOCK_ISIZE(lev,cur) \
	((lev) == (cur)->bc_nlevels - 1 ? \
		XFS_BMAP_RBLOCK_ISIZE(lev,cur) : \
		XFS_BMAP_IBLOCK_SIZE(lev,cur))
#endif

#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BLOCK_DMAXRECS)
int xfs_bmap_block_dmaxrecs(int lev, struct xfs_btree_cur *cur);
#define	XFS_BMAP_BLOCK_DMAXRECS(lev,cur)	xfs_bmap_block_dmaxrecs(lev,cur)
#else
#define	XFS_BMAP_BLOCK_DMAXRECS(lev,cur) \
	((lev) == (cur)->bc_nlevels - 1 ? \
		XFS_BTREE_BLOCK_MAXRECS(XFS_BMAP_RBLOCK_DSIZE(lev,cur), \
			xfs_bmdr, (lev) == 0) : \
		((cur)->bc_mp->m_bmap_dmxr[(lev) != 0]))
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BLOCK_IMAXRECS)
int xfs_bmap_block_imaxrecs(int lev, struct xfs_btree_cur *cur);
#define	XFS_BMAP_BLOCK_IMAXRECS(lev,cur)	xfs_bmap_block_imaxrecs(lev,cur)
#else
#define	XFS_BMAP_BLOCK_IMAXRECS(lev,cur) \
	((lev) == (cur)->bc_nlevels - 1 ? \
		XFS_BTREE_BLOCK_MAXRECS(XFS_BMAP_RBLOCK_ISIZE(lev,cur), \
			xfs_bmbt, (lev) == 0) : \
		((cur)->bc_mp->m_bmap_dmxr[(lev) != 0]))
#endif

#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BLOCK_DMINRECS)
int xfs_bmap_block_dminrecs(int lev, struct xfs_btree_cur *cur);
#define	XFS_BMAP_BLOCK_DMINRECS(lev,cur)	xfs_bmap_block_dminrecs(lev,cur)
#else
#define	XFS_BMAP_BLOCK_DMINRECS(lev,cur) \
	((lev) == (cur)->bc_nlevels - 1 ? \
		XFS_BTREE_BLOCK_MINRECS(XFS_BMAP_RBLOCK_DSIZE(lev,cur), \
			xfs_bmdr, (lev) == 0) : \
		((cur)->bc_mp->m_bmap_dmnr[(lev) != 0]))
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BLOCK_IMINRECS)
int xfs_bmap_block_iminrecs(int lev, struct xfs_btree_cur *cur);
#define	XFS_BMAP_BLOCK_IMINRECS(lev,cur)	xfs_bmap_block_iminrecs(lev,cur)
#else
#define	XFS_BMAP_BLOCK_IMINRECS(lev,cur) \
	((lev) == (cur)->bc_nlevels - 1 ? \
		XFS_BTREE_BLOCK_MINRECS(XFS_BMAP_RBLOCK_ISIZE(lev,cur), \
			xfs_bmbt, (lev) == 0) : \
		((cur)->bc_mp->m_bmap_dmnr[(lev) != 0]))
#endif

#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_REC_DADDR)
xfs_bmbt_rec_t *
xfs_bmap_rec_daddr(xfs_bmbt_block_t *bb, int i, struct xfs_btree_cur *cur);
#define	XFS_BMAP_REC_DADDR(bb,i,cur)		xfs_bmap_rec_daddr(bb,i,cur)
#else
#define	XFS_BMAP_REC_DADDR(bb,i,cur) \
	XFS_BTREE_REC_ADDR(XFS_BMAP_BLOCK_DSIZE((bb)->bb_level,cur), xfs_bmbt, \
			   bb, i, XFS_BMAP_BLOCK_DMAXRECS((bb)->bb_level, cur))
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_REC_IADDR)
xfs_bmbt_rec_t *
xfs_bmap_rec_iaddr(xfs_bmbt_block_t *bb, int i, struct xfs_btree_cur *cur);
#define	XFS_BMAP_REC_IADDR(bb,i,cur)		xfs_bmap_rec_iaddr(bb,i,cur)
#else
#define	XFS_BMAP_REC_IADDR(bb,i,cur) \
	XFS_BTREE_REC_ADDR(XFS_BMAP_BLOCK_ISIZE((bb)->bb_level,cur), xfs_bmbt, \
			   bb, i, XFS_BMAP_BLOCK_IMAXRECS((bb)->bb_level, cur))
#endif

#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_KEY_DADDR)
xfs_bmbt_key_t *
xfs_bmap_key_daddr(xfs_bmbt_block_t *bb, int i, struct xfs_btree_cur *cur);
#define	XFS_BMAP_KEY_DADDR(bb,i,cur)		xfs_bmap_key_daddr(bb,i,cur)
#else
#define	XFS_BMAP_KEY_DADDR(bb,i,cur) \
	XFS_BTREE_KEY_ADDR(XFS_BMAP_BLOCK_DSIZE((bb)->bb_level,cur), xfs_bmbt, \
			   bb, i, XFS_BMAP_BLOCK_DMAXRECS((bb)->bb_level, cur))
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_KEY_IADDR)
xfs_bmbt_key_t *
xfs_bmap_key_iaddr(xfs_bmbt_block_t *bb, int i, struct xfs_btree_cur *cur);
#define	XFS_BMAP_KEY_IADDR(bb,i,cur)		xfs_bmap_key_iaddr(bb,i,cur)
#else
#define	XFS_BMAP_KEY_IADDR(bb,i,cur) \
	XFS_BTREE_KEY_ADDR(XFS_BMAP_BLOCK_ISIZE((bb)->bb_level,cur), xfs_bmbt, \
			   bb, i, XFS_BMAP_BLOCK_IMAXRECS((bb)->bb_level, cur))
#endif

#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_PTR_DADDR)
xfs_bmbt_ptr_t *
xfs_bmap_ptr_daddr(xfs_bmbt_block_t *bb, int i, struct xfs_btree_cur *cur);
#define	XFS_BMAP_PTR_DADDR(bb,i,cur)		xfs_bmap_ptr_daddr(bb,i,cur)
#else
#define	XFS_BMAP_PTR_DADDR(bb,i,cur) \
	XFS_BTREE_PTR_ADDR(XFS_BMAP_BLOCK_DSIZE((bb)->bb_level,cur), xfs_bmbt, \
			   bb, i, XFS_BMAP_BLOCK_DMAXRECS((bb)->bb_level, cur))
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_PTR_IADDR)
xfs_bmbt_ptr_t *
xfs_bmap_ptr_iaddr(xfs_bmbt_block_t *bb, int i, struct xfs_btree_cur *cur);
#define	XFS_BMAP_PTR_IADDR(bb,i,cur)		xfs_bmap_ptr_iaddr(bb,i,cur)
#else
#define	XFS_BMAP_PTR_IADDR(bb,i,cur) \
	XFS_BTREE_PTR_ADDR(XFS_BMAP_BLOCK_ISIZE((bb)->bb_level,cur), xfs_bmbt, \
			   bb, i, XFS_BMAP_BLOCK_IMAXRECS((bb)->bb_level, cur))
#endif

/*
 * These are to be used when we know the size of the block and
 * we don't have a cursor.
 */
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BROOT_REC_ADDR)
xfs_bmbt_rec_t *xfs_bmap_broot_rec_addr(xfs_bmbt_block_t *bb, int i, int sz);
#define	XFS_BMAP_BROOT_REC_ADDR(bb,i,sz)	xfs_bmap_broot_rec_addr(bb,i,sz)
#else
#define	XFS_BMAP_BROOT_REC_ADDR(bb,i,sz) \
	XFS_BTREE_REC_ADDR(sz,xfs_bmbt,bb,i,XFS_BMAP_BROOT_MAXRECS(sz))
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BROOT_KEY_ADDR)
xfs_bmbt_key_t *xfs_bmap_broot_key_addr(xfs_bmbt_block_t *bb, int i, int sz);
#define	XFS_BMAP_BROOT_KEY_ADDR(bb,i,sz)	xfs_bmap_broot_key_addr(bb,i,sz)
#else
#define	XFS_BMAP_BROOT_KEY_ADDR(bb,i,sz) \
	XFS_BTREE_KEY_ADDR(sz,xfs_bmbt,bb,i,XFS_BMAP_BROOT_MAXRECS(sz))
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BROOT_PTR_ADDR)
xfs_bmbt_ptr_t *xfs_bmap_broot_ptr_addr(xfs_bmbt_block_t *bb, int i, int sz);
#define XFS_BMAP_BROOT_PTR_ADDR(bb,i,sz)	xfs_bmap_broot_ptr_addr(bb,i,sz)
#else
#define XFS_BMAP_BROOT_PTR_ADDR(bb,i,sz) \
	XFS_BTREE_PTR_ADDR(sz,xfs_bmbt,bb,i,XFS_BMAP_BROOT_MAXRECS(sz))
#endif

#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BROOT_NUMRECS)
int xfs_bmap_broot_numrecs(xfs_bmdr_block_t *bb);
#define	XFS_BMAP_BROOT_NUMRECS(bb)		xfs_bmap_broot_numrecs(bb)
#else
#define	XFS_BMAP_BROOT_NUMRECS(bb) ((bb)->bb_numrecs)
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BROOT_MAXRECS)
int xfs_bmap_broot_maxrecs(int sz);
#define	XFS_BMAP_BROOT_MAXRECS(sz)		xfs_bmap_broot_maxrecs(sz)
#else
#define	XFS_BMAP_BROOT_MAXRECS(sz) XFS_BTREE_BLOCK_MAXRECS(sz,xfs_bmbt,0)
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BROOT_SPACE_CALC)
int xfs_bmap_broot_space_calc(int nrecs);
#define	XFS_BMAP_BROOT_SPACE_CALC(nrecs)	xfs_bmap_broot_space_calc(nrecs)
#else
#define	XFS_BMAP_BROOT_SPACE_CALC(nrecs) \
	((int)(sizeof(xfs_bmbt_block_t) + \
	       ((nrecs) * (sizeof(xfs_bmbt_key_t) + sizeof(xfs_bmbt_ptr_t)))))
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_BROOT_SPACE)
int xfs_bmap_broot_space(xfs_bmdr_block_t *bb);
#define	XFS_BMAP_BROOT_SPACE(bb)		xfs_bmap_broot_space(bb)
#else
#define	XFS_BMAP_BROOT_SPACE(bb) XFS_BMAP_BROOT_SPACE_CALC((bb)->bb_numrecs)
#endif
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMDR_SPACE_CALC)
int xfs_bmdr_space_calc(int nrecs);
#define	XFS_BMDR_SPACE_CALC(nrecs)		xfs_bmdr_space_calc(nrecs)
#else
#define	XFS_BMDR_SPACE_CALC(nrecs)	\
	((int)(sizeof(xfs_bmdr_block_t) + \
	       ((nrecs) * (sizeof(xfs_bmbt_key_t) + sizeof(xfs_bmbt_ptr_t)))))
#endif

/*
 * Maximum number of bmap btree levels.
 */
#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BM_MAXLEVELS)
int xfs_bm_maxlevels(struct xfs_mount *mp, int w);
#define	XFS_BM_MAXLEVELS(mp,w)			xfs_bm_maxlevels(mp,w)
#else
#define	XFS_BM_MAXLEVELS(mp,w)		((mp)->m_bm_maxlevels[w])
#endif

#if XFS_WANT_FUNCS || (XFS_WANT_SPACE && XFSSO_XFS_BMAP_SANITY_CHECK)
int xfs_bmap_sanity_check(struct xfs_mount *mp, xfs_bmbt_block_t *bb,
	int level);
#define	XFS_BMAP_SANITY_CHECK(mp,bb,level)	\
	xfs_bmap_sanity_check(mp,bb,level)
#else
#define	XFS_BMAP_SANITY_CHECK(mp,bb,level)	\
	((bb)->bb_magic == XFS_BMAP_MAGIC && \
	 (bb)->bb_level == level && \
	 (bb)->bb_numrecs > 0 && \
	 (bb)->bb_numrecs <= (mp)->m_bmap_dmxr[(level) != 0])
#endif

/*
 * Trace buffer entry types.
 */
#define	XFS_BMBT_KTRACE_ARGBI	1
#define	XFS_BMBT_KTRACE_ARGBII	2
#define	XFS_BMBT_KTRACE_ARGFFFI	3
#define	XFS_BMBT_KTRACE_ARGI	4
#define	XFS_BMBT_KTRACE_ARGIFK	5
#define	XFS_BMBT_KTRACE_ARGIFR	6
#define	XFS_BMBT_KTRACE_ARGIK	7
#define	XFS_BMBT_KTRACE_CUR	8

#define	XFS_BMBT_TRACE_SIZE	4096	/* size of global trace buffer */     
#define	XFS_BMBT_KTRACE_SIZE	32	/* size of per-inode trace buffer */

#if defined(XFS_ALL_TRACE)
#define	XFS_BMBT_TRACE
#endif

#if !defined(DEBUG) || defined(SIM)
#undef XFS_BMBT_TRACE
#endif


/*
 * Prototypes for xfs_bmap.c to call.
 */

void
xfs_bmdr_to_bmbt(
	xfs_bmdr_block_t *,
	int,
	xfs_bmbt_block_t *,
	int);

int
xfs_bmbt_decrement(
	struct xfs_btree_cur *,
	int,
	int *);

int
xfs_bmbt_delete(
	struct xfs_btree_cur *,
	int,
	int *);	       

void
xfs_bmbt_get_all(
	xfs_bmbt_rec_t	*r,
	xfs_bmbt_irec_t	*s);

xfs_bmbt_block_t *
xfs_bmbt_get_block(
	struct xfs_btree_cur	*cur,
	int			level,
	struct buf		**bpp);

xfs_filblks_t
xfs_bmbt_get_blockcount(
	xfs_bmbt_rec_t	*r);

xfs_fsblock_t
xfs_bmbt_get_startblock(
	xfs_bmbt_rec_t	*r);

xfs_fileoff_t
xfs_bmbt_get_startoff(
	xfs_bmbt_rec_t	*r);

xfs_exntst_t
xfs_bmbt_get_state(
	xfs_bmbt_rec_t	*r);

int
xfs_bmbt_increment(
	struct xfs_btree_cur *,
	int,
	int *);

int
xfs_bmbt_insert(
	struct xfs_btree_cur *,
	int *);	       

int
xfs_bmbt_insert_many(
	struct xfs_btree_cur *,
	int,
	xfs_bmbt_rec_t *,
	int *);	       

void
xfs_bmbt_log_block(
	struct xfs_btree_cur *,
	struct buf *,
	int);

void
xfs_bmbt_log_recs(
	struct xfs_btree_cur *,
	struct buf *,
	int,
	int);

int
xfs_bmbt_lookup_eq(
	struct xfs_btree_cur *,
	xfs_fileoff_t,
	xfs_fsblock_t,
	xfs_filblks_t,
	int *);

int
xfs_bmbt_lookup_ge(
	struct xfs_btree_cur *,
	xfs_fileoff_t,
	xfs_fsblock_t,
	xfs_filblks_t,
	int *);

int
xfs_bmbt_lookup_le(
	struct xfs_btree_cur *,
	xfs_fileoff_t,
	xfs_fsblock_t,
	xfs_filblks_t,
	int *);

/*
 * Give the bmap btree a new root block.  Copy the old broot contents
 * down into a real block and make the broot point to it.
 */
int						/* error */
xfs_bmbt_newroot(
	struct xfs_btree_cur	*cur,		/* btree cursor */
	int			*logflags,	/* logging flags for inode */
	int			*stat);		/* return status - 0 fail */

void
xfs_bmbt_set_all(
	xfs_bmbt_rec_t	*r,
	xfs_bmbt_irec_t	*s);

void
xfs_bmbt_set_allf(
	xfs_bmbt_rec_t	*r,
	xfs_fileoff_t	o,
	xfs_fsblock_t	b,
	xfs_filblks_t	c,
	xfs_exntst_t	v);

void
xfs_bmbt_set_blockcount(
	xfs_bmbt_rec_t	*r,
	xfs_filblks_t	v);

void
xfs_bmbt_set_startblock(
	xfs_bmbt_rec_t	*r,
	xfs_fsblock_t	v);

void
xfs_bmbt_set_startoff(
	xfs_bmbt_rec_t	*r,
	xfs_fileoff_t	v);

void
xfs_bmbt_set_state(
	xfs_bmbt_rec_t	*r,
	xfs_exntst_t	v);

void
xfs_bmbt_to_bmdr(
	xfs_bmbt_block_t *,
	int,
	xfs_bmdr_block_t *,
	int);

int
xfs_bmbt_update(
	struct xfs_btree_cur *,
	xfs_fileoff_t,
	xfs_fsblock_t,
	xfs_filblks_t,
	xfs_exntst_t);

#ifdef XFSDEBUG
/* 
 * Get the data from the pointed-to record.
 */
int
xfs_bmbt_get_rec(
	struct xfs_btree_cur *,
	xfs_fileoff_t *,
	xfs_fsblock_t *,
	xfs_filblks_t *,
	xfs_exntst_t *,
	int *);
#endif


/*
 * Search an extent list for the extent which includes block
 * bno.
 */
xfs_bmbt_rec_t *
xfs_bmap_do_search_extents(
        xfs_bmbt_rec_t *,
        xfs_extnum_t,
        xfs_extnum_t,
        xfs_fileoff_t,
        int *,
        xfs_extnum_t *,
        xfs_bmbt_irec_t	*,
        xfs_bmbt_irec_t	*);


#endif	/* _FS_XFS_BMAP_BTREE_H */
