#ifndef INOMAP_H
#define INOMAP_H

#ident "$Header: /proj/irix6.5m/isms/eoe/cmd/xfs/dump/common/RCS/inomap.h,v 1.7 1995/08/22 14:50:37 tap Exp $"

/* inomap.[hc] - inode map abstraction
 *
 * an inode map describes the inode numbers (inos) in a file system dump.
 * the map identifies which inos are in-use by the fs, which of those are
 * directories, and which are dumped.
 *
 * the map is represented as a list of map segments. a map segment is
 * a 64-bit starting ino and two 64-bit bitmaps. the bitmaps describe
 * the 64 inos beginning with the starting ino. two bits are available
 * for each ino.
 */

/* inomap
 * inomap_build - this function allocates and constructs an in-memory
 * representation of the bitmap. it prunes from the map inos of files not
 * changed since the last dump, inos not identified by the subtree list,
 * and directories not needed to represent a hierarchy containing
 * changed inodes. it handles hard links; a file linked to multiple
 * directory entries will not be pruned if at least one of those
 * directories has an ancestor in the subtree list.
 *
 * it returns by reference an array of startpoints in the non-directory
 * portion of the dump, as well as the count of dir and nondir inos
 * makred as present and to be dumped. A startpoint identifies a non-dir ino,
 * and a non-hole accumulated size position within that file. only very large
 * files will contain a startpoint; in all other cases the startpoints will
 * fall at file boundaries. returns BOOL_FALSE if error encountered (should
 * abort the dump; else returns BOOL_TRUE.
 */
extern bool_t inomap_build( jdm_fshandle_t *fshandlep,
			    intgen_t fsfd,
			    xfs_bstat_t *rootstatp,
			    bool_t last,
	      		    time_t lasttime,
			    bool_t resume,
	      		    time_t resumetime,
			    size_t resumerangecnt,
			    drange_t *resumerangep,
			    char *subtreebuf[],
			    intgen_t subtreecnt,
			    startpt_t startptp[],
	      		    size_t startptcnt );


/* inomap_skip - tell inomap about inodes to skip in the dump
 */
extern void inomap_skip( ino64_t ino );


/* inomap_writehdr - updates the write header with inomap-private info
 * to be communicated to the restore side
 */
extern void inomap_writehdr( content_inode_hdr_t *scwhdrp );


/* inomap_dump - dumps the map to media - content-abstraction-knowledgable
 *
 * returns error from media write op
 */
extern intgen_t inomap_dump( content_t *contentp );


/* inomap_restore - restores map from media - content-abstraction-knowledgable
 *
 * returns error from media read op
 */
extern intgen_t inomap_restore( content_t *contentp );


/* map state values
 */
#define MAP_INO_UNUSED	0       /* ino not in use by fs */
#define MAP_DIR_NOCHNG	1       /* dir, ino in use by fs, but not dumped */
#define MAP_NDR_NOCHNG	2       /* non-dir, ino in use by fs, but not dumped */
#define MAP_DIR_CHANGE	3       /* dir, changed since last dump */
#define MAP_NDR_CHANGE	4       /* non-dir, changed since last dump */
#define MAP_DIR_SUPPRT	5       /* dir, unchanged but needed for hierarchy */
#define MAP_RESERVED1	6       /* this state currently not used */
#define MAP_RESERVED2	7       /* this state currently not used */

/* inomap_state - returns the map state of the given ino.
 * highly optimized for monotonically increasing arguments to
 * successive calls. requires a pointer to a context block, obtained from
 * inomap_state_getcontext(), and released by inomap_state_freecontext().
 */
extern void *inomap_state_getcontext( void );
extern intgen_t inomap_state( void *contextp, ino64_t ino );
extern void inomap_state_freecontext( void *contextp );


/* inomap_iter_cb - will call the supplied function for each ino in
 * the map matching a state in the state mask. if the callback returns
 * FALSE, the iteration will be aborted and inomap_iter_cb() will
 * return FALSE. the state mask is constructed by OR'ing bits in bit
 * positions corresponding to the state values.
 */
extern bool_t inomap_iter_cb( void *contextp,
			      intgen_t statemask,
			      bool_t ( *funcp )( void *contextp,
					         ino64_t ino,
					         intgen_t state ));

#endif /* INOMAP_H */
