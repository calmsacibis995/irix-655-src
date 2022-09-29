#include <stdio.h>
#include <sys/types.h>

/*
 * The primary structure used to keep information for each
 * inode in use on the file system.
 */

typedef unsigned long index;	/* any index into one of the heaps, below */

typedef unsigned int str5_t;	/* result of str5pack of 5 chars into 32 bits */
#define U32_MAX UINT_MAX	/* max str5_t value: must be 32 bit unsigned */
#define STR5LEN 5		/* number of chars packed into str5_t type */

/*
 * Each hash entry contains a pointer to a group of 64 inodes starting 
 * on a 64 bit boundary. In case multiple groups share a hash entry - 
 * they are linked together.
 */

typedef struct {
	ino64_t startino;	/* 64 bit boundary for the group of inodes */
	index	nextptr;	/* Next group of inodes at the hash entry */
	index	inoptr [64];
} inm2_t;

typedef struct {
	ushort i_mode;		/* copied from disk inode structure */
	nlink_t i_nlink;	/*		""		    */
	ushort i_uid;		/*		""		    */
	ushort i_gid;		/*		""		    */
	off_t i_size;		/*		""		    */
	time_t i_atime;		/*		""		    */
	time_t i_mtime;		/*		""		    */
	time_t i_ctime;		/*		""		    */
	ulong i_gen;		/*		""		    */
	dev_t i_rdev;		/*		"" (di_u.di_dev)    */
	index i_xfenv;		/* offset in hp_fenv of files's "environment" string */
	nlink_t i_xnlink;	/* nmbr dir. links to this file seen so far */
	nlink_t i_xndent;	/* if (directory) number of entries therein */
	index i_xlinklist;	/* index into heap hp_link of links to this file */
	index i_xdentlist;	/* if (directory) index into heap hp_dent of struct dent's */
	ino_t i_xino;		/* my own inode number */
	size_t i_xnsubtree;	/* how many inodes below here in tree */
} inod_t;

/*
 * For those inodes that happen to be directories,
 * we keep the contents of the directory's entries
 * in the following structure.
 */

typedef struct {
	ino_t e_ino;		/* the inode number of this directory entry */
	index e_name;		/* index into hp_name of this directory entry's file name */
	index e_ddent;		/* index into hp_dent of parent dir's dent for this dir */
} dent_t;

/*
 * The hp_dndx heap holds an index into hp_dent, and a str5pack'd string.
 * The hp_dnd2 heap is holds a uniformly distributed subset of the str5_t's from hp_dndx.
 */

typedef struct {
	index dx_dent;
	str5_t dx_str5;
} dndx_t;

/*
 * This program primarily captures, stores, and manipulates all
 * the inode and directory information in a file system.
 *
 * To allow storage of this information in external store
 * for later use by another copy of this program, we avoid much use
 * of embedded pointers.  This is not easy, because the data being
 * managed would naturally form a large interwoven set of linked
 * structures.
 *
 * Instead all the dynamically allocated data is kept in a small, fixed
 * number of "heaps", that form a "heap set".  References between heaps
 * in a set are stored as indicies from the base of the referenced heap.
 * A different heap is used for each type of data, and it is implicit
 * from the type of data being referenced which heap the index
 * is relative to.  Sadly, this circumvents the compilers type checking.
 *
 * The current types of information kept in a heap set are:
 *
 *	inode numbers - info stored in two levels using hash algorithm
 *	inode contents
 *	directory entries
 *	links - the directory entries referring to an inode.
 *	strings - the filenames in directories
 *	index - secondary search index of indicies into hp_dent, ASCII sorted on e_name.
 *	fenv - per file "environment" - string values of additional properties.
 *
 * When creating a new heap set, some reasonable upper bound for the
 * size of each heap in the set is computed from the information in the
 * file system superblock, and memory is allocated for the entire heap
 * set before scanning the file system's inodes and directories.  By
 * reasonable, I mean only that (1) the entire set of heaps fits in the
 * virtual memory available to the process, and (2) each heap is big
 * enough to hold all information destined for it without growing.
 * This is fairly easy to do in a large VM address space system.
 *
 * Each heap is filled from the bottom up, and when writing a set
 * of heaps to an external file, only the lower portion of each
 * heap that was actually used is written to the file, since
 * physical disk space is not as cheap as unused virtual memory
 * address space.
 */

typedef union {
	char   *pchar;		/* for heaps hp_name and hp_fenv */
	dent_t *pdent;		/* for heap hp_dent */
	index  *pindex;		/* for heaps hp_inum and hp_link */
	inm2_t *pinm2;		/* for heaps hp_inm2 */
	inod_t *pinod;		/* for heap hp_inod */
	dndx_t *pdndx;		/* for heap hp_dndx */
	str5_t *pdnd2;		/* for heap hp_dnd2 */
} pun_t;			/* ... pointer union */

typedef struct {
	size_t hd_elsz;		/* size of each element */
	index  hd_next;		/* index of next element to allocate */
	index  hd_top;		/* index of first element too high to allocate */
	char   hd_mflg;		/* memory flag: see MMAP, MALC below */
} des_t;			/* ... heap descriptor */

#define MMAP 0		/* value of hd_mflg: means this heap is mmap'd */
#define MALC 1		/* value of hd_mflg: means this heap is malloc'd */
#define MMPA 2		/* value of hd_mflg; means this heap is mmap'd with autogrow */

typedef struct {
	struct { pun_t inumpun; des_t inumdes; } hx_inum;
	struct { pun_t inm2pun; des_t inm2des; } hx_inm2;
	struct { pun_t inodpun; des_t inoddes; } hx_inod;
	struct { pun_t dentpun; des_t dentdes; } hx_dent;
	struct { pun_t linkpun; des_t linkdes; } hx_link;
	struct { pun_t namepun; des_t namedes; } hx_name;
	struct { pun_t dndxpun; des_t dndxdes; } hx_dndx;
	struct { pun_t dnd2pun; des_t dnd2des; } hx_dnd2;
	struct { pun_t fenvpun; des_t fenvdes; } hx_fenv;
} heap_set;

extern heap_set mh;		/* ... the main memory heap */

#define hp_inum hx_inum.inumpun.pindex	/* for ea. hashindx index into hp_inm2 */
#define hp_inm2 hx_inm2.inm2pun.pinm2   /* for ea. group of 64 inodes , inm2_t */ 
#define hp_inod hx_inod.inodpun.pinod	/* for ea. allocated inode, its contents */
#define hp_dent hx_dent.dentpun.pdent	/* for ea. directory, list of its entries */
#define hp_link hx_link.linkpun.pindex	/* for ea. link to file, index into hp_dent of link */
#define hp_name hx_name.namepun.pchar	/* for ea. directory entry, its nul-term name string */
#define hp_dndx hx_dndx.dndxpun.pdndx	/* for fast searches, index to hp_dent in name order */
#define hp_dnd2 hx_dnd2.dnd2pun.pdnd2	/* for faster searches, index to the index hp_dndx */
#define hp_fenv hx_fenv.fenvpun.pchar	/* for ea. file, its "environment" (eg RCS rev) */

/*
 * Pseudo declarations:
 *
 * Except inside fsheap.c and when using the #define's below,
 * the declarations above behave just like the following:
 *
 * struct {
 *	index *hp_inum;		-- for ea. inode, index into hp_inm2
 *	inm2_t *hp_inm2;
 *	inod_t *hp_inod;	-- for ea. allocated inode, its contents
 *	dent_t *hp_dent;	-- for ea. directory, list of its entries
 *	index *hp_link;		-- for ea. link to file, index into hp_dent of link
 *	char *hp_name;		-- for ea. directory entry, its nul-term name string
 *	dndx_t *hp_dndx;	-- for fast searches, index to hp_dent in name order
 *	str5_t *hp_dnd2;	-- for faster searches, an index to the index hp_dndx
 *	char *hp_fenv;		-- for ea. file, its "environment" (eg RCS rev)
 * } mh;			-- ... the main memory heap
 */

/*
 * These various heaps reference each other as follows:
 *
 * inum ==> inod
 *		The contents of hp_inum map inode numbers to hp_inod's inod_t struct.
 *
 * inod ==> dent, link, fenv
 *		Each inod_t in hp_inod refers to its "file environment" in hp_fenv.
 *		For each directory, indicies in hp_inod refer to its entries and links.
 *
 * dent ==> inum, name, dent
 *		For each directory entry, hp_dent maps to the entry inum.
 *		For each directory entry, hp_dent maps to the string name in the entry.
 *		For each directory entry, hp_dent maps to that dirs entry in its parent.
 *
 * dndx ==> dent
 *		hp_dndx contains a list of references into hp_dent, sorted by name
 *		hp_dndx contains (not "maps to") the str5pack of each string in hp_name
 *
 * dnd2
 *		hp_dnd2 contains no direct references to other heaps
 *		Instead, it has 1 page (4K) of evenly distributed str5pack'd
 *		strings from hp_dndx - to aid in searching hp_dndx.
 *
 * link ==> dent
 *		For every file, the list of indicies into hp_dent of links to the file.
 *
 */

/*
 * Firewall:
 *	The above complex data structures are mostly
 *	invisible to all the code in fsdump.c, etal..
 * Except that:
 *	1) All heap references (except (2) - next) are of the form:
 *	   &mh.hp_inum, for one of hp_inum, hp_inod, ..., hp_fenv.
 *	2) The PtoD macro defined in fsheap.c is used only
 *	   in the heap*(){} heap management routines in fsheap.c.
 *	   This macro converts the usual "&mh.hp_inum" form into a pointer
 *	   to the heap descriptor members {hd_elsz, hd_next, hd_top}
 *	   that are used only inside the heap*() routines.
 *	3) Otherwise the internal structure (a struct of a list of
 *	   7 structs, each a union and a struct) of mh
 *	   is not visible in the files fsdump.c, etal..
 *	4) The dent_t and inod_t structures, and declarations prior
 *	   thereto, are widely visible in the files fsdump.c, etal..
 */

/*
 * Since we cannot directly address a heapset when it is
 * stored in a file, and since we do not allocate additional
 * items from heaps in a file, we only need to temporarilly
 * keep track of each heaps size and offset when moving
 * heaps to/from files:
 */

struct fh {
	off_t fh_inum_off;
	off_t fh_inm2_off;
	off_t fh_inod_off;
	off_t fh_dent_off;
	off_t fh_link_off;
	off_t fh_name_off;
	off_t fh_dndx_off;
	off_t fh_dnd2_off;
	off_t fh_fenv_off;
	size_t fh_inum_sz;
	size_t fh_inm2_sz;
	size_t fh_inod_sz;
	size_t fh_dent_sz;
	size_t fh_link_sz;
	size_t fh_name_sz;
	size_t fh_dndx_sz;
	size_t fh_dnd2_sz;
	size_t fh_fenv_sz;
	size_t fh_inum_top;
	size_t fh_inm2_top;
	size_t fh_inod_top;
	size_t fh_dent_top;
	size_t fh_link_top;
	size_t fh_name_top;
	size_t fh_dndx_top;
	size_t fh_dnd2_top;
	size_t fh_fenv_top;
};

extern struct fh fh;

/*
 * We define the following macros to access these structures.
 *
 *  PINO:
 *	return a pointer to the inod_t for inode n in heapset s
 *  PENT:
 *	for ea. directory, return pointer to n-th entry in that directory
 *  PLNK:
 *	for ea. file, return pointer to n-th directory entry linking to it.
 *  PNAM:
 *	converts inode name offset into inode name string (char *).
 *  PENV:
 *	converts inode pointer into file environment string (char *).
 *  PDDE:
 *	converts dent ptr into dent ptr of its parent directory's reference.
 */


/*#define PINO(s,n) ( (s).hp_inod + (((s).hp_inum)[n]) ) */

#define PNM1(s, n) ( (s).hp_inum + HASHFUNC(n))

#define PENT(s,p,n) ( (s).hp_dent + (p)->i_xdentlist + (n) )

#define PLNK(s,p,n) ( (s).hp_dent + (s).hp_link [ (p)->i_xlinklist + (n) ] )

#define PNAM(s,ep) ( (s).hp_name + (ep)->e_name )

#define PENV(s,ip) ( (ip)->i_xfenv == (index)0 ? NULL : (s).hp_fenv + (ip)->i_xfenv )

#define PDDE(s,ep) ( (s).hp_dent + (ep)->e_ddent )

/* index *PNM1(heap_set *, ino64_t); */
inm2_t *PNM2(heap_set *, ino64_t);
inod_t *PINO(heap_set *, ino64_t);
void PADD(heap_set *, index, ino64_t);

/*
 * The following macros are used to help display the
 * size originally allocated and actually used of
 * each heap.
 */

#define A(h) heapnsize((void **)(h))
#define B(h) heapntop((void **)(h))
#define C(h) heapbsize((void **)(h))
#define percent(A,B) (int)((100.0 * (double)(A)) / (double)(B))

void heapinit (void **, index, size_t);
void heapfinit (void **, char *, off_t, off_t, size_t, size_t);
index heapalloc (void **, index);
index heaprealloc (void **, index, index, index);
index xheaprealloc (void **, index, index, index);
void heapshrink (void **, index);
size_t heapbsize (void **);
index heapnsize (void **);
index heapntop (void **);

str5_t str5pack (char *);
char *str5unpack (str5_t);
char *path (dent_t *);
char *pathcomp (char *);

extern index primeno;
extern ino64_t rootino;

/* index hashfunc(ino64_t); */

#define HASHFUNC(i) ((index)((i & ~(ino64_t)63) % primeno))

