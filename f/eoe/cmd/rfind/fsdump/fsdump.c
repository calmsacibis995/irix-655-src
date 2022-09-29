/*
 * fsdump -- create fsdump file from contents of a file system
 *
 * The fsdump command reads the inodes and directories of a file system
 * to create an fsdump file, and the command rfindd searches and displays
 * the information in an fsdump file.  The fsdump command knows enough about the
 * disk format of a file system to be able to access and interpret the superblock,
 * and to be able to find and interpret all the inodes in the file system.
 * Unlike the ill-fated AT&T ff(1) command, fsdump does not know about the
 * disk layout of data blocks or directories.  It uses ordinary opendir, ...
 * to read directories.  The rfindd command knows about the fsdump file format
 * shared with the fsdump command, but knows nothing about file system
 * disk formats.
 */

#include <math.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/immu.h>
#include <sys/syssgi.h>
#include <sys/sysmacros.h>
#define NDEBUG
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <values.h>
#include <sys/fs/efs.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <sys/mman.h>
#include <limits.h>
#include <malloc.h>
#include <signal.h>
#include <sys/statfs.h>
#include <pwd.h>
#include "fsdump.h"
#include "fenv.h"
#include "xfsio.h"
#include <sys/time.h>
#include <sys/uuid.h>
#include <sys/fs/xfs_fsops.h>
#include <sys/fs/xfs_itable.h>
#include "jdm.h"

#include <errno.h>
extern int errno;

#include <sys/stat.h>
struct stat statbuf;

#include <mntent.h>
struct mntlist {
	struct mntent	mnt;
	struct mntlist *next;
} *mnthead;

#include <sys/utsname.h>
struct utsname un;

#include <sys/times.h>
#include <sys/resource.h>
struct tms tmsbuf;

heap_set mh;				/* ... the main memory heap */
struct fh fh;					/* file image of heap */

int fsd;			/* descriptor filesystem is open on */
union {
	struct	efs fs;
#if lint
	char	block[BLOCKSIZE];
#else
	char	block[BBTOB(BTOBB(sizeof(struct efs)))];
#endif
} sblock;
struct efs *fs = &sblock.fs;

extern void error(char *s);	/* user provided error printer */
extern void wipedir (inod_t *);		/* wipe out stale directory info */
extern size_t xscandir (ino_t, index);	/* recursive directory scanner */
extern char *mountdev(char *);
extern int readb (int, char *, int, int);
extern char *findrawpath (char *);

long nitotal;			/* Number of inodes in file system */
long niinuse;			/* Number of inodes currently in use in file system */
index primeno;		/* Prime number derived as a function of inodes in use */
ino64_t rootino;
bool_t isxfs = BOOL_FALSE;

typedef enum { quiet = 0, showerrors = 1 } t_eflag;
t_eflag ErrorFlag;
typedef enum {
               newdump,        /* not merging prior fsdump, brand new ino */
               added,          /* inode was allocated */
               deleted,        /* inode was deallocated */
               changed,        /* inode changed in key way */
        } ino_action;          /* ... what's happened to this ino */
ino_action action;



extern int dndxcmp (const dndx_t *, const dndx_t *);
#ifndef NDEBUG
extern void check_hp_nuls();
#endif
extern void dump();
extern int opendumpfile();
extern int restore(int, int);
extern void mhinit();
extern void inumloop(int);
extern void fenvloop();
extern void fenvcat (inod_t *q1, char *oldfenv);
extern void symlinkfenvtreewalk (ino_t dirino);
extern void rcsfenvtreewalk (ino_t dirino);
extern void qsumfenvtreewalk (ino_t dirino);
extern void initqsumflag();
extern void dumpqsumflag();
extern void warn (char *s, char *t);
extern FILE *fdup (const FILE *fp, const char *type);
#define BUFLEN       1024
extern void genprimenum();
extern bool_t isinxfs ();
extern void gethash();
extern bool_t getinocnt ();
extern void cmpefsnodes (inod_t *p, struct efs_dinode *di, ino64_t ino);
extern void cmpxfsnodes (struct xfs_bstat_t *xp);
extern void markunused (ino64_t from, ino64_t to);

char *dumpfile = NULL;		/* name of file to dump or and/or merge with */
char *logfile = NULL;		/* name of file to write log messages to */
time_t dumpfiledate;		/* dumpfile mtime: used by time_to_garbage_collect() */
off_t dumpfilesize;		/* dumpfile size in bytes */
char filenamebuf[PATH_MAX];	/* for: dumpfilelock, newdumpfilename, symlink reads */
char *tmpfile_to_unlink = NULL;	/* pointer to tmp file to unlink if error */
char *lockfile_to_unlink = NULL;	/* pointer to lock file to unlink if error */
FILE *tmpfile_to_fclose = NULL;	/* FILE * to tmp file to close if error */
char curdir[PATH_MAX];		/* need to restore directory before referencing dumpfile */
char Pathname[PATH_MAX];	/* path to xscandir's current depth */
FILE *ndf_fp;			/* stream new dumpfile is open on */
FILE *real_stderr = NULL;	/* we dup stderr to here, then reopen stderr on the log file */
char *Mntpt;			/* full path of mount point of file system to dump */
int rfindd_uid;			/* uid of user "rfindd" */

typedef enum { FALSE, TRUE } BOOLEAN;
BOOLEAN domerge;		/* TRUE if merging disk scan results into prior fsdump file */
BOOLEAN update_dirs = FALSE;	/* TRUE if any inode changed, and need to update directories */

#define A_HOUR	3600L		/* an hour full of seconds */
#define ThreeHours (3*A_HOUR)
time_t StartTime;		/* seconds from system boot to invocation of fsdump */
int quantum = 0;		/* number of minutes between each scheduled dump */
int maxrss = 0;			/* bytes limit on max resident set size in main mem */
time_t inodescantime;		/* how many seconds it took to scan inodes */

char fsdumpline[512];

BOOLEAN takeoutgarbage;		/* TRUE if rebuilding heaps from scratch to collect garbage */
void dualscandir(char *, ino_t); /* Merge old fenv back into newly built heaps */
heap_set mhold, mhsave;	/* Only used by dualscandir if takeoutgarbage TRUE */

void Usage(){
	error("Usage: fsdump [-L<logfile>] [-M<minutes>] [-Q] [-R] [-U<maxrss>] -F<dumpfile> mount-point");
	/* NOTREACHED */
}

#ifdef SGI_CONST
static struct pageconst pageconst;

/*
 * These defines have been added based on the use of
 * the following pagesize-dependent macros:
 *
 *	btoc, ctob
 */
#define	_PAGESZ		pageconst.p_pagesz
#define	PNUMSHFT	pageconst.p_pnumshft
#define	BPCSHIFT	pageconst.p_pnumshft

#endif /* SGI_CONST */

int
getunum (char *name)
{
        register i;
        struct  passwd  *pw;

        i = -1;
	if( ((pw = getpwnam (name)) != (struct passwd *)NULL) && pw != (struct passwd *)EOF )
		i = pw->pw_uid;
        return(i);
}


/*
 * A file is considered ok_to_unlink if:
 *  1) its apparent mode is writable by owner,
 *  2) we ("rfindd") can't write it because we don't own it,
 *  3) we apparently can write its containing directory,
 *     because we own the directory, and that is owner writable.
 * 
 * For examples:
 *  * We don't consider a read-only file ok_to_unlink, even
 *    though we probably could unlink it if we tried.
 *  * We do consider a file on a read-only mount ok_to_unlink,
 *    even though the unlink will no doubt fail.
 * 
 * The intention is to identify those cases where we failed
 * to open a file for writing because the file had the wrong
 * owner, but where it is reasonable to force the matter
 * (by unlinking and trying the open again) because the file
 * is marked writable and we do own and have write permissions
 * on the directory.
 *
 * Older fsdump's, prior to fixes for bugs 448392/448393, lacked
 * the setuid("rfindd") call, and if run by root or from root's
 * crontab, left the data, log and index files owned by root.
 * But the files were owner-writable and in a directory,
 * /var/rfindd, owned by rfindd and owner-writable.
 */

int ok_to_unlink (const char *file) {
    extern char *dirname (char *);
    char *tmpfilename = strdup (file);

    int ret =
	stat (file, &statbuf) == 0 &&		/* can stat file */
	statbuf.st_uid != rfindd_uid &&		/* rfindd not own file */
	(statbuf.st_mode & S_IWUSR) != 0 &&	/* file user writable */
	stat (dirname (file), &statbuf) == 0 &&	/* can stat dir */
	statbuf.st_uid == rfindd_uid &&		/* rfindd does own dir */
	(statbuf.st_mode & S_IWUSR) != 0	/* dir user writable */
    ;

    free (tmpfilename);
    return ret;
}


main (int argc, char *argv[]) {
	extern int optind;
	extern char *optarg;
	int Qflg = 0, Rflg = 0;
	register inod_t *p;	/* pointer to current struct inod_t */
	register ino64_t inump;
	int i, j;
	struct efs_dinode *di, *enddi, *cginobuf;
	int fd;			/* file descriptor open on old dumpfile */
	int cg;
	size_t segsz;
	char *efs_block_dev;	/* "/dev/..." path of block device */
	char *efs_raw_dev;	/* "/dev/r..." path of raw device */
	double begin, end;	/* times begin and end main loops */
	double xbegin, xend;
	double BEGIN, END;	/* times for TOTAL fsdump */
        struct statfs statfs_buf;
	extern void lockdumpfile();

        /* insist on being real uid root */
        if (getuid() != 0) {
            char *msg = "not root\n";
            write (2, msg, strlen (msg));
            _exit(1);
        }

	/* BEWARE: executing with root privileges until open file sys */

	if (time(&StartTime) == (time_t) -1) {
		(void) fprintf(stderr, "fsdump: time() %s\n", sys_errlist[errno]);
		exit(2);
	}

	BEGIN = times(&tmsbuf);	/* Stopwatch on: begin doing fsdump */

	while ((i = getopt(argc, argv, "F:L:M:QRU:")) != EOF) {
		switch (i) {
		case 'F':			/* dumpfile name */
			dumpfile = optarg;
			break;
		case 'L':			/* logfile name */
			logfile = optarg;
			break;
		case 'M':			/* minutes in allowed quantum */
			quantum = atoi (optarg);
			break;
		case 'R':			/* collect RCS fenv */
			Rflg++;
			break;
		case 'Q':			/* quicksum data corruption check */
			Qflg++;
			break;
		case 'U':			/* limit memoryuse to xx kbytes */
			maxrss = atoi (optarg) * 1024;
			break;
		default:
			Usage();
		}
	}

	getwd(curdir);				/* do getwd() BEFORE findrawpath does chdir */

	if (dumpfile == NULL)
		Usage();
	if (optind + 1 != argc)
		error ("Must specify one mount-point");
	Mntpt = argv[optind];
	if ((efs_block_dev = mountdev(Mntpt)) == NULL)
		error ("FileSys not in /etc/mtab; consider editing fslist (see fsdump(1M)) to remove this FileSys");
	if ((efs_raw_dev = findrawpath (efs_block_dev)) == NULL)	/* does chdir's */
		error (efs_block_dev);

	sync();				/* so that raw disk reads see latest changes */

	if ((fsd = open (efs_block_dev, O_RDONLY)) < 0)
		error(efs_block_dev);

	umask (022);

	rfindd_uid = getunum ("rfindd");
	if (rfindd_uid == -1)
		error ("Unable to find uid for rfindd");

	setuid (rfindd_uid);  	/* RELAX: relinquish root privileges */
				/* Only vestige of root priviledges: fsd open on rawdev */

	lockdumpfile();

	if (chdir(curdir) < 0)		/* check we can do this even without root perms */
		error (curdir);

#ifdef SGI_CONST
	if (syssgi(SGI_CONST, SGICONST_PAGESZ, &pageconst,
				sizeof(pageconst), 0) == -1) {
		perror("syssgi: pagesz");
		exit(1);
	}	
#endif /* SGI_CONST */

	if (maxrss > 0) {
		struct rlimit rlimit;

		if (getrlimit (RLIMIT_RSS, &rlimit) < 0)
			error ("getrlimit");
		rlimit.rlim_cur = maxrss;
		if (setrlimit (RLIMIT_RSS, &rlimit) < 0)
			error ("setrlimit");
	}

	if (logfile != NULL) {
		char buf[100];		/* build date string here */

		if (freopen (logfile, "a", stdout) == 0) {
		    if (ok_to_unlink (logfile)) {
			(void) unlink (logfile);
			if (freopen (logfile, "a", stdout) == 0)
				error (logfile);
		    }
		}

		real_stderr = fdup (stderr, "a");
		if (freopen (logfile, "a", stderr) == 0)
			error (logfile);

		setvbuf (stdout, NULL, _IOLBF, 0);
		setvbuf (stderr, NULL, _IOLBF, 0);

		cftime (buf, "", &StartTime);
		puts ("");
		puts (buf);
	}

	/*
	 * EFS File System dependent code: Part 1 of 2.
	 *
	 * These lines must accomplish the following:
	 *
	 *  1) file system super block read and magic checked.
	 *  2) file system dependent structure "fs" filled in for use
	 *	here and by Part 2 of the EFS dependent code, below.
	 *  3) EFS dependent space "cginobuf" allocated to read in cylinder group of inodes
	 *  4) "nitotal" set to the total number of inodes in file system
	 *  5) "niinuse" set to the number of allocated inodes in file system
	 * << niinuse is padded by 10% to handle transients and such >>
	 */

	if (isxfs = isinxfs()) {
         	if (statfs( Mntpt, &statfs_buf, sizeof statfs_buf, 0) == 0)
		{
                        niinuse = statfs_buf.f_files - statfs_buf.f_ffree;
		} else {
			error("stat xfs");
		}
		if (stat (Mntpt, &statbuf) == 0) {
			rootino = statbuf.st_ino;
		} else {
			error ("stat root");
		}
		nitotal = (nitotal/2048 + 1) * 2048; 
	} else {
		lseek(fsd, (long) EFS_SUPERBOFF, 0);
		if(read(fsd,(void *)fs,BBTOB(BTOBB(sizeof(*fs))))!=BBTOB(BTOBB(sizeof(*fs))))
			error("read superblock");
		if (!IS_EFS_MAGIC(fs->fs_magic)) 
			error ("not efs superblock");
		EFS_SETUP_SUPERB(fs);
		if ((cginobuf = (struct efs_dinode *)malloc(
			EFS_INOPBB * fs->fs_cgisize * sizeof (struct efs_dinode)
			)) == NULL)
			error ("no memory");
		cg = EFS_ITOCG(fs, 2);
		if (cg == 0)
			rootino = 2;
		else
			rootino = cg * fs->fs_ipcg;
		if (cg >= fs->fs_ncg)
			return 0;
		nitotal = fs->fs_ipcg * fs->fs_ncg;
		niinuse = nitotal - fs->fs_tinode;
	}

	printf ("Number of inodes total %d; allocated %d\n", nitotal, niinuse);
        niinuse = (int) ((double)1.1 * (double)niinuse);
        if (niinuse > nitotal) niinuse = nitotal;
	genprimenum();

	fd = opendumpfile();

	uname (&un);			/* Both dump() and restore() need un set */
	if (time_to_garbage_collect())
		takeoutgarbage = TRUE;
	else
		takeoutgarbage = FALSE;;

/*** Workaround for a bug in irix where it picked an address some margin
 *** above the heap. Typically 0xx8000000 bytes which left 128Mb for the
 *** heap. So incase we have a fairly large filesystem where the size of
 *** the previous dumpfile is more 128Mb, mmaping the old one and creating
 *** a new one may hit this limit. 
 *** In that case malloc a big buffer and free it immediately. Irix will
 *** then mmap files above that buffer - giving us a large enough heap.
 ***/
	if (dumpfilesize > (128 * 1024 * 1024)) {
		char *trialptr = (char *)malloc(300 * 1024 * 1024);
        	if ( trialptr == NULL ) {
                        printf("Large malloc failed\n");
		} else {
			free((char *)trialptr);
		}
	}

	if (takeoutgarbage == TRUE || restore(fd, 0) < 0) {
		domerge = FALSE;
		mhinit();
	} else {
		domerge = TRUE;
	}

	/*
	 * Above code set "ino" to first valid inode number (2, most likely).
	 * Now that our memory heap mh is setup, we convert "ino" to a
	 * pointer "inump" into mh.hp_inum, in anticipation of our scan
	 * of all the inodes below.
	 */

	/* Need to somehow mark the current time, so that rfindd -sync can know
	 * that it only needs wait until the time that the available fsdump was
	 * begun is later than the time rfindd -sync began waiting. XXX */

	begin = times(&tmsbuf);		/* Stopwatch on: for time to read inodes */

	/*
	 * EFS File System dependent code: Part 2 of 2.
	 *
	 * For each cylinder group
	 *	Readb in all the inodes in that group
	 *	For each inode in that cylinder group
	 *		Update the memory heap
	 *
	 * After all inodes read in, close file descriptor to raw device
	 * and free the EFS dependent "cginobuf" space used to read inodes into.
	 */

	if (isxfs) {
	        jdm_fshandle_t *fshandlep;
	
	        xfs_bstat_t *buf;
		ino64_t lastino = 0;
		ino64_t expectedino = 1;
	        intgen_t buflen;
	
	        fshandlep = jdm_getfshandle(Mntpt);
	        if (!fshandlep) {
	                printf("Unable to get a filesystem handle for %s: %s\n",
	                                Mntpt, strerror(errno));
	                error("xfs handle error");
	        }
	
	        /* allocate a buffer
	         */
	        buf = ( xfs_bstat_t * )calloc( BUFLEN, sizeof( xfs_bstat_t ));
	        assert( buf );

	        while (! syssgi(SGI_FS_BULKSTAT, fsd, &lastino, BUFLEN, buf, &buflen)){
	                xfs_bstat_t *xp;
	                xfs_bstat_t *endp;
	
	                if ( buflen == 0 ) {
	                        free( ( void * )buf );
	                        break;
	                }
	                for ( xp = buf, endp = buf + buflen ; xp < endp ; xp++) {
				if (xp->bs_ino > expectedino) {
					/*  All entries in between to be marked
					   deleted if earlier in used   */
					markunused(expectedino, xp->bs_ino); 
				}
				cmpxfsnodes(xp); 
				expectedino = xp->bs_ino + 1; 
	                }
	        }
	} else {
		inump = rootino;
		for (; cg < fs->fs_ncg; cg++) {
			if(readb(fsd,(char *)cginobuf,EFS_CGIMIN(fs,cg),fs->fs_cgisize)!=fs->fs_cgisize)
				error("readb");
			enddi = cginobuf + EFS_INOPBB * fs->fs_cgisize;
			di = cginobuf + (cg == 0 ? 2 : 0);

			for (; di < enddi; inump++, di++) {
				p = PINO(&mh,inump);
				cmpefsnodes(p, di, inump);
			} /* end of loop over inodes in a cylinder group */
		} /* end of loop over cylinder groups */

		free((void *)cginobuf);
		close (fsd);			/* last vestige of setuid root perms is gone */
	}

	end = times(&tmsbuf);		/* Stopwatch off: done reading inodes */
	printf ("%.2f seconds to read inodes\n", (end - begin) / HZ);
	inodescantime = (end - begin) / HZ;

	if (update_dirs == TRUE) {
		begin = times(&tmsbuf);		/* Stopwatch on: for time to scan directories */

		(void) chdir (Mntpt);
		(void) strcpy (Pathname, Mntpt);
		(void) xscandir (rootino, (index)0);

		end = times(&tmsbuf);		/* Stopwatch off: done scanning directories */
		printf ("%.2f seconds to scan directories\n", (end - begin) / HZ);
	}

	if (update_dirs == TRUE) {
		index ndentold, ndentnew;

		/*
		 * Update hp_dndx: the secondary sort index for hp_dent, sorted by name.
		 *
		 * We could just realloc a clean hp_dndx and fully qsort it.
		 * But since it is faster to qsort an almost sorted array,
		 * we try to preserve the existing array, sort of ... :-).
		 *
		 * There are 4 possibilities covered by the following code:
		 *
		 * 1) We are not merging with an old fsdump from a file, and must
		 *    build hp_dndx from scratch.  In this case, ndentold == 0,
		 *    ndentnew is the number of entries in hp_dent.  We must init
		 *    the entire hp_dndx, alloc it (to mark it all in use), and qsort it.
		 * 2) There are fewer hp_dent entries allocated now than in the fsdump file
		 *    (ndentnew < ndentold).  So the old hp_dndx has some hp_dent offsets
		 *    in it that are TOO big for the current hp_dent.  This case only happens
		 *    if we have garbage collected hp_dent - in which case our old sort
		 *    is useless, since all index values into hp_dent changed.  Redo it,
		 *    by shrinking hp_dndx and re-qsorting from scratch.
		 * 3) There are just the same number of hp_dent entries as before.
		 *    Just qsort it again, which should be quick.
		 * 4) There are more hp_dent entries now than in the fsdump file.
		 *    This is the usual case.  Use heapalloc to get some more hp_dndx
		 *    entries, fill them with the new hp_dent index values between
		 *    ndentold and ndentnew, and qsort the mess.
		 */

		begin = times(&tmsbuf);		/* Stopwatch on: for time pack names */
		ndentold = heapnsize ((void **)(&mh.hp_dndx));
		ndentnew = heapnsize ((void **)(&mh.hp_dent));
		if (ndentold == 0)
			heapinit ((void **)(&mh.hp_dndx), ndentnew, sizeof(*mh.hp_dndx));
		if (ndentnew > ndentold) {
			(void) heapalloc ((void **)(&mh.hp_dndx), ndentnew - ndentold);
		} else if (ndentnew < ndentold) {
			heapshrink ((void **)(&mh.hp_dndx), ndentnew);
			ndentold = 0;
		}
		for (i = ndentold; i < ndentnew; i++)
			mh.hp_dndx[i].dx_dent  = i;
		/*
		 * Redo all the dx_str5 elements -- they may be stale
		 * if any hp_dent entries have been deleted or reallocated
		 */
		for (i = 0; i < ndentnew; i++)
			mh.hp_dndx[i].dx_str5 = str5pack (PNAM (mh, mh.hp_dent + mh.hp_dndx[i].dx_dent));

		end = times(&tmsbuf);		/* Stopwatch off: done packing names */
		printf ("%.2f seconds to pack names\n", (end - begin) / HZ);

		begin = times(&tmsbuf);		/* Stopwatch on: for time qsort hp_dndx */
		qsort ((void *)(mh.hp_dndx), ndentnew, sizeof(*mh.hp_dndx), dndxcmp);
		end = times(&tmsbuf);		/* Stopwatch off: done qsorting hp_dndx */
		printf ("%.2f seconds to qsort hp_dndx\n", (end - begin) / HZ);

		/*
		 * Update hp_dnd2: the index to the index hp_dndx.
		 *
		 * To build hp_dnd2, we consider hp_dndx to be 1K equal
		 * size segments, and put into hp_dnd2 the str5packed name
		 * of the first hp_dndx member in each segment.
		 *
		 * This gives us a table of names that fits on 1 page,
		 * and that is evenly distributed over the name space.
		 *
		 * The first 10 probes, more or less, of a binary search
		 * for some name can be done on hp_dnd2, which should help
		 * us avoid up to 9 page faults.
		 */
#ifdef SGI_CONST
#if !defined(_PAGESZ)
		segsz = ((ndentnew * sizeof(*mh.hp_dnd2)) / pagesize);
		if (segsz < 1) segsz = 1;
		if (ndentold == 0)
			heapinit ((void **)(&mh.hp_dnd2), pagesize/sizeof(*mh.hp_dnd2), sizeof(*mh.hp_dnd2));

		for (i=0, j=0; i < ndentnew && j < pagesize/sizeof(*mh.hp_dnd2); i += segsz, j++) {
			mh.hp_dnd2[j] = mh.hp_dndx[i].dx_str5;
			assert (j == 0 || mh.hp_dnd2[j-1] <= mh.hp_dnd2[j]);
		}
#else
		segsz = ((ndentnew * sizeof(*mh.hp_dnd2)) >> BPCSHIFT);
		if (segsz < 1) segsz = 1;
		if (ndentold == 0)
			heapinit ((void **)(&mh.hp_dnd2), NBPC/sizeof(*mh.hp_dnd2), sizeof(*mh.hp_dnd2));

		for (i=0, j=0; i < ndentnew && j < NBPC/sizeof(*mh.hp_dnd2); i += segsz, j++) {
			mh.hp_dnd2[j] = mh.hp_dndx[i].dx_str5;
			assert (j == 0 || mh.hp_dnd2[j-1] <= mh.hp_dnd2[j]);
		}
#endif
#else /* SGI_CONST */
                segsz = ((ndentnew * sizeof(*mh.hp_dnd2)) >> BPCSHIFT);
                if (segsz < 1) segsz = 1;
                if (ndentold == 0)
                        heapinit ((void **)(&mh.hp_dnd2), NBPC/sizeof(*mh.hp_dnd2), sizeof(*mh.hp_dnd2));

                for (i=0, j=0; i < ndentnew && j < NBPC/sizeof(*mh.hp_dnd2);
i += segsz, j++) {
                        mh.hp_dnd2[j] = mh.hp_dndx[i].dx_str5;
                        assert (j == 0 || mh.hp_dnd2[j-1] <= mh.hp_dnd2[j]);
                }
#endif /* SGI_CONST */

		if (j > heapnsize ((void **)(&mh.hp_dnd2)))
			(void) heapalloc ((void **)(&mh.hp_dnd2), j - heapnsize((void **)(&mh.hp_dnd2)));
		else if (j < heapnsize ((void **)(&mh.hp_dnd2)))
			(void) heapshrink ((void **)(&mh.hp_dnd2), j);

	}

	if (takeoutgarbage == TRUE) {
		/*
		 * In this case, we have chosen to ignore a possible existing
		 * fsdump file, and build a new one from scratch, in order to
		 * get rid of the wasted space that builds up in our heaps.
		 * All well and good - except that the existing fsdump file
		 * (if any) has some valuable file environment "fenv" information,
		 * such as RCS top-of-trunk delta numbers.  We don't want to
		 * have to recompute all that, so we map in the old fsdump,
		 * and ask dualscandir to walk the old and new trees in parallel
		 * copying over the fenv of any files that didn't change.
		 */
		mhsave = mh;
		begin = times(&tmsbuf);		/* Stopwatch on: for time to collect garbage */
		if (restore(fd, 1) == 0) {		/* restore old fsdump into mh */
			mhold = mh;
			mh = mhsave;

			dualscandir ("/", rootino);

			end = times(&tmsbuf);	/* Stopwatch off: done collecting garbage */
			printf ("%.2f seconds to collect garbage\n", (end - begin) / HZ);
		}
	}

        if (TRUE) {
		/*
		 * reread any new or changed symlinks
		 */

		begin = times(&tmsbuf);	/* Stopwatch on: for time to set SYMLINK fenv */
		(void) chdir (Mntpt);
		(void) strcpy (Pathname, Mntpt);
		symlinkfenvtreewalk ( rootino );
		end = times(&tmsbuf);	/* Stopwatch off: done setting SYMLINK fenv */
		printf ("%.2f seconds to set SYMLINK fenv\n", (end - begin) / HZ);
	}

	if (Rflg) {
		/*
		 * Now update the RCS *,v info.
		 * If we are out of time in our quantum - this call returns immediately.
		 */

		begin = times(&tmsbuf);	/* Stopwatch on: for time to set RCS fenv */
		(void) chdir (Mntpt);
		(void) strcpy (Pathname, Mntpt);
		rcsfenvtreewalk ( rootino );
		end = times(&tmsbuf);	/* Stopwatch off: done setting RCS fenv */
		printf ("%.2f seconds to set RCS fenv\n", (end - begin) / HZ);
	}

	if (Qflg) {
		/*
		 * Spend all (if any) the time remaining in our quantum checking
		 * quicksum's (QSUM).  We are looking for file system corruption.
		 */

		begin = times(&tmsbuf);	/* Stopwatch on: for time to check corruption */
		(void) chdir (Mntpt);
		(void) strcpy (Pathname, Mntpt);

		initqsumflag();
		qsumfenvtreewalk ( rootino );
		dumpqsumflag();

		end = times(&tmsbuf);	/* Stopwatch off: done checking corruption */
		printf ("%.2f seconds checking for data corruption\n", (end - begin) / HZ);
	}

#if 0
	printf("inum %d/%d (%d%%) inm2 %d/%d (%d%%) inod %d/%d (%d%%) dent %d/%d (%d%%)\nlink %d/%d (%d%%) name %d/%d (%d%%)\ndndx %d/%d (%d%%) dnd2 %d/%d (%d%%) fenv %d/%d (%d%%)\n",
		A(&mh.hp_inum), B(&mh.hp_inum), percent (A(&mh.hp_inum), B(&mh.hp_inum)),
		A(&mh.hp_inm2), B(&mh.hp_inm2), percent (A(&mh.hp_inm2), B(&mh.hp_inm2)),
		A(&mh.hp_inod), B(&mh.hp_inod), percent (A(&mh.hp_inod), B(&mh.hp_inod)),
		A(&mh.hp_dent), B(&mh.hp_dent), percent (A(&mh.hp_dent), B(&mh.hp_dent)),
		A(&mh.hp_link), B(&mh.hp_link), percent (A(&mh.hp_link), B(&mh.hp_link)),
		A(&mh.hp_name), B(&mh.hp_name), percent (A(&mh.hp_name), B(&mh.hp_name)),
		A(&mh.hp_dndx), B(&mh.hp_dndx), percent (A(&mh.hp_dndx), B(&mh.hp_dndx)),
		A(&mh.hp_dnd2), B(&mh.hp_dnd2), percent (A(&mh.hp_dnd2), B(&mh.hp_dnd2)),
		A(&mh.hp_fenv), B(&mh.hp_fenv), percent (A(&mh.hp_fenv), B(&mh.hp_fenv))
	);
#endif

	printf("Total memory used: %.2f MBytes\n",
		(double)(C(&mh.hp_inum) + C(&mh.hp_inm2) +C(&mh.hp_inod) + C(&mh.hp_dent) + C(&mh.hp_link) + C(&mh.hp_name) + C(&mh.hp_dndx) + C(&mh.hp_dnd2) + C(&mh.hp_fenv)) / (double)(1024*1024) );

#if 0
	inumloop (nitotal);
#endif
#if 0
	fenvloop();
#endif

#ifndef NDEBUG
	check_hp_nuls();
#endif

	begin = times(&tmsbuf);	/* Stopwatch on: for time to dodump */
	dump();
	end = times(&tmsbuf);	/* Stopwatch off: done doing dump */
	printf ("%.2f seconds to do dump\n", (end - begin) / HZ);

	END = times(&tmsbuf);	/* Stopwatch off: done doing fsdump */
	printf ("%.2f seconds TOTAL fsdump time\n", (END - BEGIN) / HZ);

	exit (0);
	/* NOTREACHED */
}


void error (char *msg) {
	int saverr = errno;

	if (tmpfile_to_unlink != NULL) (void) unlink (tmpfile_to_unlink);
	if (lockfile_to_unlink != NULL) (void) unlink (lockfile_to_unlink);
	if (tmpfile_to_fclose != NULL) fclose (tmpfile_to_fclose);
	fprintf(stderr,"fsdump failed:: FileSys <%s> DumpFile <%s> Msg <%s> errno <%s>\n",
					Mntpt, dumpfile, msg, strerror(saverr));
	if (real_stderr != NULL)
		fprintf(real_stderr,
			"fsdump failed:: FileSys <%s> DumpFile <%s> Msg <%s> errno <%s>\n",
					Mntpt, dumpfile, msg, strerror(saverr));
	exit (1);
	/* NOTREACHED */
}

void warn (char *s, char *t) {

	if (errno)
		fprintf (stderr, "warning: %s: %s: %s\n", s, t, strerror(errno));
	else
		fprintf (stderr, "warning: %s: %s\n", s, t);
	errno = 0;
}

/*
 * Create the "mh" memory heap set from scratch,
 * do this if there was no dumpfile to restore mh from.
 */

void mhinit() {
	/*
	 * Allocate space for mh heapset, using the following upper bounds.
	 *  hp_inum:
	 * 	for each possible entry in the hash bucket : hp_inum has 1
	 *	offset into hp_inm2;
	 * hp_inm2:
	 *	for each actual group of 64 inodes: hp_inm2 has one struct 
	 *	inm2_t - they may be chained, for an allocated inode - 1 
	 *	index into hp_inod; Assume an average 4 groups at each hashkey.
	 *	(for each possible inode: hp_inum has 1 offset into hp_inod)
	 *  hp_inod:
	 *	for each actual inode: hp_inod has one struct inod_t
	 *  hp_dent:
	 *	for each directory entry: hp_dent has one struct dent
	 *	assume on average < 1.5 directory entries (links) per file
	 *  hp_link:
	 *	for each directory entry: hp_link has one offset into hp_dent
	 *	assume on average < 2.5 directory entries (links) per file
	 *	(there are more links than dents because we waste link space
	 *	 every time we have to reallocate to grow a link list)
	 *  hp_name:
	 *	assume average strlen filename < 16    (basename, not pathname)
	 *  hp_dndx and hp_dnd2:
	 *	initialize these later, when we know how many enties in hp_dent
	 *  hp_fenv:
	 *	Since this is kept LAST in heap set, it is easier to grow it later.
	 */

	heapinit ((void **)(&mh.hp_inum), primeno, sizeof(*mh.hp_inum));
	heapinit ((void **)(&mh.hp_inm2), primeno*4, sizeof(*mh.hp_inm2));
	heapinit ((void **)(&mh.hp_inod), niinuse, sizeof(*mh.hp_inod)); 
	heapinit ((void **)(&mh.hp_dent), 3*niinuse/2, sizeof(*mh.hp_dent));
	heapinit ((void **)(&mh.hp_link), 5*niinuse/2, sizeof(*mh.hp_link));
	heapinit ((void **)(&mh.hp_name), 16*niinuse, sizeof(*mh.hp_name));
	heapinit ((void **)(&mh.hp_fenv), 1, sizeof(*mh.hp_fenv));

	/*
	 * The entire hp_inum array is directly indexed by the hashkey 
	 * number, rather than being allocated using heapalloc().
	 * So we immediately allocate it all, telling the dump()
	 * routine that the entire hp_inum heap should be dumped.
	 */

	(void) heapalloc ((void **)(&mh.hp_inum), primeno);

	/*
	 * Allocate the first element of all the other heaps, and
	 * then forget that element - so that a zero offset into
	 * that heap can be used as a pseudo-null, meaning not set.
	 */
	(void) heapalloc ((void **)(&mh.hp_inm2), 1);
	mh.hp_inm2->startino = -1;
	(void) heapalloc ((void **)(&mh.hp_inod), 1);
	(void) heapalloc ((void **)(&mh.hp_dent), 1);
	(void) heapalloc ((void **)(&mh.hp_link), 1);
	(void) heapalloc ((void **)(&mh.hp_name), 1);
	(void) heapalloc ((void **)(&mh.hp_fenv), 1);
}

void wipedir (inod_t *p) {
	nlink_t i, j, k;

	/*
	 * For changed or deleted directories - blow away stale data.
	 *
	 * Find the hp_link entries that reference this directory's entries,
	 * and zero them out, by moving the rest of the link list down
	 * over the top of the entry being removed.
	 *
	 * Occasional messy work for modest gain.
	 * They are used in the path routine to walk back up the tree,
	 * which is needed to print pathnames from the -inum, -ncheck
	 * and -name options when doing fast searches.
	 *
	 * Also zero out each of the hp_dent entries for this directory.
	 * When we can do fast name lookups using hp_dndx, it will be
	 * nice not to be finding phantom (garbage) hp_dent entries.
	 */

#if 0
printf("wipedir: ino %d <%s>\n", p->i_xino, path(PENT(mh,p,0)));
#endif
	for (i=0; i < p->i_xndent; i++) {
		dent_t *dp = PENT (mh, p, i);
		inod_t *q = PINO (&mh, dp->e_ino);

		for (j=0,k=0; j < q->i_xnlink; j++) {
			if (mh.hp_link[q->i_xlinklist+j] != p->i_xdentlist+i) {
				mh.hp_link[q->i_xlinklist+k] =
					mh.hp_link[q->i_xlinklist+j];
				k++;
			}
		}
		q->i_xnlink = k;

		dp->e_ino = (ino_t)0;
		dp->e_name = (index)0;
		dp->e_ddent = (index)0;
	}
	p->i_xndent = 0;
	p->i_xdentlist = (index)0;
}

size_t xscandir (ino_t dirino, index ddent) {
	DIR *dirp;
	struct dirent *dp;
	inod_t *p, *q;
	dent_t *ep;
	index sz;
	int i;
	char *c1, *endofname, *fname;
	int namelen;
	size_t xnsubtree = 0;


	/*
	 * We scan each directory three times:
	 *	First scan to detect the mode going to zero on entry
	 *		without touching the directory.  (If the directory
	 *		had been touched, as unlink/rmdir do, then we already
	 *		did wipedir on this directory and its i_xndent == 0).
	 *	Second scan to stash all its entries (if need be)
	 *	Third scan to recurse into any subdirectories.
	 */

	p = PINO(&mh,dirino);
	if (p->i_mode == 0) {
		fprintf(stderr,"warning: xscandir zero mode inode %lld <%s>\n",dirino,Pathname);
		return 0 ;
	}
	if (p->i_xnlink == 0 && (dirino != rootino || domerge == TRUE))
		fprintf(stderr,"warning: xscandir zero nlink inode %lld <%s>\n",dirino,Pathname);

	assert (PINO (&mh, rootino) -> i_mode);

	for (i=0; i < p->i_xndent; i++) {
		ep = PENT (mh, p, i);
		q = PINO(&mh,ep->e_ino);
		if (q->i_mode == 0) {
			/*
			 * Looks like fsck zero'd out a sick inode, which
			 * likely means that fsck also deleted this directory
			 * entry without touching the directory.
			 *
			 * Wipe out what we think we know about this directory, and
			 * (since p->i_xndent will be set to 0) force a rescan of it.
			 *
			 * This all has the serious problem that if fsck blasts
			 * an inode and deletes its links (without touching the
			 * directory, of course) and if then the inode gets reused
			 * before fsdump runs once, then we will never notice that
			 * we have extra links (names) for the reincarnated inode
			 * until we garbage collect the database at 3:00 am.
			 */
			wipedir (p);
			break;
		}
	}

	if (p->i_xndent == 0) {
#if 0
		printf("opendir(%s)\n",Pathname);
#endif
		if ((dirp = opendir (Pathname)) == NULL) {
#if 0
			warn ("opendir", Pathname);
#endif
			return 0;
		}

		/*
		 * Second scan - since directory changed, redo all its entries
		 */

		while ((dp = readdir(dirp)) != NULL) {
			if (dp->d_ino == 0) continue;
			assert (dp->d_ino < nitotal);
			if (dp->d_name[0] == '.' && dp->d_name[1] == 0) {
				if (dirino != dp->d_ino)
					break;
			}
			q = PINO(&mh,dp->d_ino);

			/*
			 * If file entered directory after our inode scan, skip it.
			 * We don't have the matching inode data to go with it.
			 */
			if (q->i_mode == 0) {
				/* XXX Hack: mutate our info for this directory
				 * so that next time fsdump runs, it will think
				 * this directory changed, and wipedir/re-scan it.
				 * Be sure to change one of the fields that
				 * lib/fchanged.c looks at, so ptools will
				 * know that this directory is out of date.
				 * Future idea: try stat'ing the file q (d_name)
				 * right here, and building an inode and such
				 * to match what we find, at least in the case
				 * that the stat yields an inode == d_ino.
				 */
				p->i_ctime++;
#if 0
				fprintf(stderr,"will rescan <%s> for <%s> next pass\n", Pathname, dp->d_name);
#endif
				continue;
			}

			/* stash another entry <ino,name> for this directory */
			p->i_xdentlist = heaprealloc ((void **)(&mh.hp_dent), p->i_xdentlist, p->i_xndent, 1);
			ep = PENT(mh, p, p->i_xndent++);
			ep->e_ino = dp->d_ino;
			sz = (index)(strlen (dp->d_name) + 1);
			ep->e_name = heapalloc ((void **)(&mh.hp_name), sz);
			(void) strcpy (PNAM(mh,ep), dp->d_name);
			ep->e_ddent = ddent;

			/* and stash this dent's offset in ref'd file's linklist */
			q->i_xlinklist = heaprealloc ((void **)(&mh.hp_link), q->i_xlinklist, q->i_xnlink, 1);
			mh.hp_link [ q->i_xlinklist + q->i_xnlink++ ] = ep - mh.hp_dent;
		}
		closedir(dirp);
	}

	/*
	 * Third scan - Recurse to all subdirectories
	 */

	namelen = strlen (Pathname);
	c1 = Pathname + namelen;
	if (*(c1-1) =='/') --c1;
	endofname = c1;

	for (i=0; i < p->i_xndent; i++) {

		ep = PENT (mh, p, i);
		q = PINO(&mh,ep->e_ino);

		/* Check ddent for each entry: they moved if my parent realloc'd dentlist. */
		if (ep->e_ddent != ddent)
			ep->e_ddent = ddent;

		fname = PNAM (mh, ep);
		if (fname[0] == '.'
		    && (fname[1] == '\0'
			|| fname[1] == '.'
			    && fname[2] == '\0'))
				continue;

		if ( ! S_ISDIR(q->i_mode) )
			continue;

		c1 = endofname;
		*c1++ = '/';
		if (namelen + 1 + strlen(fname) + 1 >= PATH_MAX) {
			*c1 = '\0';
			fprintf(stderr,"warning: pathname too long : skipping <%s%s>\n", Pathname, fname);
			continue;
		} 
		(void) strcpy (c1, fname);

		if (ismounted (Pathname))
			continue;
		xnsubtree += xscandir (ep->e_ino, ep - mh.hp_dent);
	}
	*endofname = 0;
	p->i_xnsubtree = xnsubtree + p->i_xndent;
	return p->i_xnsubtree;
}

int dndxcmp (register const dndx_t *a, register const dndx_t *b) {
	if (a->dx_str5 < b->dx_str5)
		return -1;
	if (a->dx_str5 > b->dx_str5)
		return 1;
	return strcmp (PNAM (mh, mh.hp_dent+a->dx_dent), PNAM (mh, mh.hp_dent+b->dx_dent));
}

#ifndef NDEBUG
void check_hp_nuls(){
	register int *ip;
	register char *cp;

	/*
	 * Checks that the first element of each heap
	 * really contains all zeros, like a good pseudo-nul should.
	 */

	(void) chdir (curdir);
	for (ip = (int *)(mh.hp_inum); ip < (int *)(mh.hp_inum + 1); ip++)
		assert (*ip == 0);
	for (ip = (int *)(mh.hp_inm2); ip < (int *)(mh.hp_inm2 + 1); ip++)
		assert (*ip == 0);
	for (ip = (int *)(mh.hp_inod); ip < (int *)(mh.hp_inod + 1); ip++)
		assert (*ip == 0);
	for (ip = (int *)(mh.hp_dent); ip < (int *)(mh.hp_dent + 1); ip++)
		assert (*ip == 0);
	for (ip = (int *)(mh.hp_link); ip < (int *)(mh.hp_link + 1); ip++)
		assert (*ip == 0);
	for (cp = (char *)(mh.hp_name); cp < (char *)(mh.hp_name + 1); cp++)
		assert (*cp == 0);
	/* dont check hp_dndx or dnd2 - their first elements are not pseudo-nuls */
	for (cp = (char *)(mh.hp_fenv); cp < (char *)(mh.hp_fenv + 1); cp++)
		assert (*cp == 0);
}
#endif

#include <ndbm.h>

void updatedbm (char *mnt, char *dump) {
	datum key, content;
	DBM *db;

	/* Update the fsdump.{dir,pag} ndbm files that map Mntpt's to dumpfile names. */

	if ((db = dbm_open ("fsdump", O_CREAT|O_RDWR, 0644)) == NULL) {
	    if (ok_to_unlink ("fsdump.dir")) {
		(void) unlink ("fsdump.dir");
		(void) unlink ("fsdump.pag");
		db = dbm_open ("fsdump", O_CREAT|O_RDWR, 0644);
	    }
	}

	if (db == NULL)
		error ("Cannot create/update fsdump.{dir,pag} file");

	signal (SIGHUP, SIG_IGN);
	signal (SIGINT, SIG_IGN);
	signal (SIGPIPE, SIG_IGN);
	signal (SIGTERM, SIG_IGN);

	for (key = dbm_firstkey(db); key.dptr != NULL; key = dbm_nextkey(db)) {
		content = dbm_fetch (db, key);
		if (stat (content.dptr, &statbuf) < 0 && errno == ENOENT)
			dbm_delete (db, key);
	}

	key.dptr = mnt;
	key.dsize = strlen(mnt) + 1;
	content = dbm_fetch (db, key);

	if (content.dptr == NULL || strcmp (content.dptr, dump) != 0) {
		content.dptr = dump;
		content.dsize = strlen (dump) + 1;
		if (dbm_store (db, key, content, DBM_REPLACE) < 0)
			error ("Cannot store into fsdump.{dir,pag} file");
	}
	dbm_close (db);
}

void dosig() {
	if (tmpfile_to_unlink != NULL) (void) unlink (tmpfile_to_unlink);
	if (lockfile_to_unlink != NULL) (void) unlink (lockfile_to_unlink);
	fprintf (stderr, "interrupted\n");
	exit (1);
}

void dump() {
	off_t heap_header_offset, heap_inum_offset;
	struct utimbuf utbuf;
	struct statfs buffs;
	int cmask, i;

	/*
	 * Setup a tmp file for the new fsdump file that we are to construct.
	 * Put it in the same directory as the specified "-F<dumpfile>" so
	 * that we can use rename to move it into place, if all goes well.
	 */

	(void) chdir(curdir);
	if (dumpfile[0] == '/')
		(void) strcpy (filenamebuf, dumpfile);
	else
		(void) strcat (strcat (strcpy (filenamebuf, curdir), "/"), dumpfile);
	(void) strcpy (strrchr (filenamebuf, '/') + 1, "fsdump.tmp.XXXXXX");


	if ((i = mkstemp (filenamebuf)) < 0)
		error (filenamebuf);
	tmpfile_to_unlink = strdup (filenamebuf);
	if ((ndf_fp = fdopen (i, "w")) == NULL)
		error(filenamebuf);
	tmpfile_to_fclose = ndf_fp;
	umask (cmask = umask(0));
	chmod (filenamebuf, 0644 & ~cmask);

	fputs ("fsdump format level 3\n", ndf_fp);
	fprintf (ndf_fp, "%.*s %.*s %.*s %.*s %.*s\n",
		sizeof(un.sysname), un.sysname,
		sizeof(un.nodename), un.nodename,
		sizeof(un.release), un.release,
		sizeof(un.version), un.version,
		sizeof(un.machine), un.machine);
	fputs (ctime (&StartTime), ndf_fp);
	if (stat (Mntpt, &statbuf) < 0)
		error ("Cannot stat FileSys mount point");
	fprintf (ndf_fp,"Mount point: %s device %d\n", Mntpt, statbuf.st_dev);

	fprintf (ndf_fp,"Total Inodes %ld\n", nitotal);
	fprintf (ndf_fp,"Root inode %lld\n", rootino);
	fprintf (ndf_fp,"Prime number used %ld\n", primeno);

	/*
	 * We write the heap header into the dumpfile twice,
	 * once just to figure out how big the header is,
	 * and then again to write out the correct values.
	 * This is because the rest of this header includes
	 * offsets to the data that is packed in right afterward.
	 */

	/* Once for practice: */

        heap_header_offset = ftell (ndf_fp);
	fh.fh_inum_sz = heapbsize((void **)(&mh.hp_inum));
	fh.fh_inum_top = ctob(btoc(fh.fh_inum_off + fh.fh_inum_sz) + 1);

        fh.fh_inm2_off = fh.fh_inum_top;
        fh.fh_inm2_sz = heapbsize((void **)(&mh.hp_inm2));
        fh.fh_inm2_top = ctob(btoc(fh.fh_inm2_off + fh.fh_inm2_sz) + 1);

	fh.fh_inod_off = fh.fh_inm2_top;
	fh.fh_inod_sz = heapbsize((void **)(&mh.hp_inod));
	fh.fh_inod_top = ctob(btoc(fh.fh_inod_off + fh.fh_inod_sz) + 1);

	fh.fh_dent_off = fh.fh_inod_top;
	fh.fh_dent_sz = heapbsize((void **)(&mh.hp_dent));
	fh.fh_dent_top = ctob(btoc(fh.fh_dent_off + fh.fh_dent_sz) + 1);

	fh.fh_link_off = fh.fh_dent_top;
	fh.fh_link_sz = heapbsize((void **)(&mh.hp_link));
	fh.fh_link_top = ctob(btoc(fh.fh_link_off + fh.fh_link_sz) + 1);

	fh.fh_name_off = fh.fh_link_top;
	fh.fh_name_sz = heapbsize((void **)(&mh.hp_name));
	fh.fh_name_top = ctob(btoc(fh.fh_name_off + fh.fh_name_sz) + 1);

	fh.fh_dndx_off = fh.fh_name_top;
	fh.fh_dndx_sz = heapbsize((void **)(&mh.hp_dndx));
	fh.fh_dndx_top = ctob(btoc(fh.fh_dndx_off + fh.fh_dndx_sz) + 1);

	fh.fh_dnd2_off = fh.fh_dndx_top;
	fh.fh_dnd2_sz = heapbsize((void **)(&mh.hp_dnd2));
	fh.fh_dnd2_top = ctob(btoc(fh.fh_dnd2_off + fh.fh_dnd2_sz) + 1);

	fh.fh_fenv_off = fh.fh_dnd2_top;
	fh.fh_fenv_sz = heapbsize((void **)(&mh.hp_fenv));
	fh.fh_fenv_top = ctob(btoc(fh.fh_fenv_off + fh.fh_fenv_sz) + 1);

	fprintf(ndf_fp, "inum offset: %16lld size %8u\n", fh.fh_inum_off, fh.fh_inum_sz);
	fprintf(ndf_fp, "inm2 offset: %16lld size %8u\n", fh.fh_inm2_off, fh.fh_inm2_sz);
	fprintf(ndf_fp, "inod offset: %16lld size %8u\n", fh.fh_inod_off, fh.fh_inod_sz);
	fprintf(ndf_fp, "dent offset: %16lld size %8u\n", fh.fh_dent_off, fh.fh_dent_sz);
	fprintf(ndf_fp, "link offset: %16lld size %8u\n", fh.fh_link_off, fh.fh_link_sz);
	fprintf(ndf_fp, "name offset: %16lld size %8u\n", fh.fh_name_off, fh.fh_name_sz);
	fprintf(ndf_fp, "dndx offset: %16lld size %8u\n", fh.fh_dndx_off, fh.fh_dndx_sz);
	fprintf(ndf_fp, "dnd2 offset: %16lld size %8u\n", fh.fh_dnd2_off, fh.fh_dnd2_sz);
	fprintf(ndf_fp, "fenv offset: %16lld size %8u\n", fh.fh_fenv_off, fh.fh_fenv_sz);

	while ((heap_inum_offset = ftell (ndf_fp)) & 03)
		fprintf(ndf_fp,"\n");			/* go up to next word boundary */

	if (fstatfs (fileno(ndf_fp), &buffs, sizeof(buffs), 0) < 0)
		error ("fstatfs");
#define vBTOBB(bsize,bytes) (((unsigned long)(bytes) + bsize - 1) / bsize )
	if (buffs.f_bfree < vBTOBB (buffs.f_bsize, fh.fh_fenv_off + fh.fh_fenv_sz)) {
		errno = ENOSPC;
		error ("Cannot write out new dumpfile");
	}

	/* And once for real: */

	fseek (ndf_fp, heap_header_offset, SEEK_SET);

        fh.fh_inum_off = heap_inum_offset;
	fh.fh_inum_sz = heapbsize((void **)(&mh.hp_inum));
	fh.fh_inum_top = ctob(btoc(fh.fh_inum_off + fh.fh_inum_sz) + 1);

        fh.fh_inm2_off = fh.fh_inum_top;
        fh.fh_inm2_sz = heapbsize((void **)(&mh.hp_inm2));
        fh.fh_inm2_top = ctob(btoc(fh.fh_inm2_off + fh.fh_inm2_sz) + 1);

        fh.fh_inod_off = fh.fh_inm2_top;
	fh.fh_inod_sz = heapbsize((void **)(&mh.hp_inod));
	fh.fh_inod_top = ctob(btoc(fh.fh_inod_off + fh.fh_inod_sz) + 1);

	fh.fh_dent_off = fh.fh_inod_top;
	fh.fh_dent_sz = heapbsize((void **)(&mh.hp_dent));
	fh.fh_dent_top = ctob(btoc(fh.fh_dent_off + fh.fh_dent_sz) + 1);

	fh.fh_link_off = fh.fh_dent_top;
	fh.fh_link_sz = heapbsize((void **)(&mh.hp_link));
	fh.fh_link_top = ctob(btoc(fh.fh_link_off + fh.fh_link_sz) + 1);

	fh.fh_name_off = fh.fh_link_top;
	fh.fh_name_sz = heapbsize((void **)(&mh.hp_name));
	fh.fh_name_top = ctob(btoc(fh.fh_name_off + fh.fh_name_sz) + 1);

	fh.fh_dndx_off = fh.fh_name_top;
	fh.fh_dndx_sz = heapbsize((void **)(&mh.hp_dndx));
	fh.fh_dndx_top = ctob(btoc(fh.fh_dndx_off + fh.fh_dndx_sz) + 1);

	fh.fh_dnd2_off = fh.fh_dndx_top;
	fh.fh_dnd2_sz = heapbsize((void **)(&mh.hp_dnd2));
	fh.fh_dnd2_top = ctob(btoc(fh.fh_dnd2_off + fh.fh_dnd2_sz) + 1);

	fh.fh_fenv_off = fh.fh_dnd2_top;
	fh.fh_fenv_sz = heapbsize((void **)(&mh.hp_fenv));
	fh.fh_fenv_top = ctob(btoc(fh.fh_fenv_off + fh.fh_fenv_sz) + 1);

	fprintf(ndf_fp, "inum offset: %16lld size %8u\n", fh.fh_inum_off, fh.fh_inum_sz);
	fprintf(ndf_fp, "inm2 offset: %16lld size %8u\n", fh.fh_inm2_off, fh.fh_inm2_sz);
	fprintf(ndf_fp, "inod offset: %16lld size %8u\n", fh.fh_inod_off, fh.fh_inod_sz);
	fprintf(ndf_fp, "dent offset: %16lld size %8u\n", fh.fh_dent_off, fh.fh_dent_sz);
	fprintf(ndf_fp, "link offset: %16lld size %8u\n", fh.fh_link_off, fh.fh_link_sz);
	fprintf(ndf_fp, "name offset: %16lld size %8u\n", fh.fh_name_off, fh.fh_name_sz);
	fprintf(ndf_fp, "dndx offset: %16lld size %8u\n", fh.fh_dndx_off, fh.fh_dndx_sz);
	fprintf(ndf_fp, "dnd2 offset: %16lld size %8u\n", fh.fh_dnd2_off, fh.fh_dnd2_sz);
	fprintf(ndf_fp, "fenv offset: %16lld size %8u\n", fh.fh_fenv_off, fh.fh_fenv_sz);

	while ((heap_inum_offset = ftell (ndf_fp)) & 03)
		fprintf(ndf_fp,"\n");			/* go up to next word boundary */

	/*
	 * Done writing out the header.
	 * Now write out each heap in the set.
	 */

	assert (fh.fh_inum_off == ftell(ndf_fp));
	fwrite ((void *)mh.hp_inum, 1, fh.fh_inum_sz, ndf_fp);
	fseek (ndf_fp, fh.fh_inum_top, SEEK_SET);

        assert (fh.fh_inm2_off == ftell(ndf_fp));
        fwrite ((void *)mh.hp_inm2, 1, fh.fh_inm2_sz, ndf_fp);
        fseek (ndf_fp, fh.fh_inm2_top, SEEK_SET);

	assert (fh.fh_inod_off == ctob(btoc(ftell(ndf_fp))));
	fwrite ((void *)mh.hp_inod, 1, fh.fh_inod_sz, ndf_fp);
	fseek (ndf_fp, fh.fh_inod_top, SEEK_SET);

	assert (fh.fh_dent_off == ctob(btoc(ftell(ndf_fp))));
	fwrite ((void *)mh.hp_dent, 1, fh.fh_dent_sz, ndf_fp);
	fseek (ndf_fp, fh.fh_dent_top, SEEK_SET);

	assert (fh.fh_link_off == ctob(btoc(ftell(ndf_fp))));
	fwrite ((void *)mh.hp_link, 1, fh.fh_link_sz, ndf_fp);
	fseek (ndf_fp, fh.fh_link_top, SEEK_SET);

	assert (fh.fh_name_off == ctob(btoc(ftell(ndf_fp))));
	fwrite ((void *)mh.hp_name, 1, fh.fh_name_sz, ndf_fp);
	fseek (ndf_fp, fh.fh_name_top, SEEK_SET);

	assert (fh.fh_dndx_off == ctob(btoc(ftell(ndf_fp))));
	fwrite ((void *)mh.hp_dndx, 1, fh.fh_dndx_sz, ndf_fp);
	fseek (ndf_fp, fh.fh_dndx_top, SEEK_SET);

	assert (fh.fh_dnd2_off == ctob(btoc(ftell(ndf_fp))));
	fwrite ((void *)mh.hp_dnd2, 1, fh.fh_dnd2_sz, ndf_fp);
	fseek (ndf_fp, fh.fh_dnd2_top, SEEK_SET);

	assert (fh.fh_fenv_off == ctob(btoc(ftell(ndf_fp))));
	fwrite ((void *)mh.hp_fenv, 1, fh.fh_fenv_sz, ndf_fp);
	if (fh.fh_fenv_top > ftell (ndf_fp)) {
		fseek (ndf_fp, fh.fh_fenv_top - 1, SEEK_SET);
		fwrite ("", 1, 1, ndf_fp);
	}

	fflush (ndf_fp);
	if (ferror (ndf_fp)) error ("writing dumpfile");
	fclose (ndf_fp);

	(void) chdir(curdir);

	utbuf.actime = utbuf.modtime = StartTime;
	if (utime (filenamebuf, &utbuf) != 0)
		error ("utime filenamebuf");

	updatedbm (Mntpt, dumpfile);
	if (rename (filenamebuf, dumpfile) != 0)
		error ("rename");
	if (lockfile_to_unlink != NULL) (void) unlink (lockfile_to_unlink);
}


int opendumpfile(){
	int fd;			/* file descriptor to open on old dumpfile */

	(void) chdir (curdir);
	if ((fd = open (dumpfile, O_RDWR)) < 0) {
		if (errno == ENOENT)
			return -1;
		if (ok_to_unlink (dumpfile) && unlink (dumpfile) == 0)
			return -1;
		error ("cannot open dumpfile");
	}
	if (fstat (fd, &statbuf) < 0)
		error ("fstat census");
	dumpfilesize = statbuf.st_size;
	dumpfiledate = statbuf.st_mtime;
	return fd;
}

void lockdumpfile() {
	int fd;

	/*
	 * Setup a lock file for the new fsdump file that we are to construct.
	 * Put it in the same directory as the specified "-F<dumpfile>"
	 * Don't use dumpfile itself - might not be created yet.
	 */

	if (dumpfile[0] == '/')
		(void) strcpy (filenamebuf, dumpfile);
	else
		(void) strcat (strcat (strcpy (filenamebuf, curdir), "/"), dumpfile);

	strcat (filenamebuf, ".lock");

	signal (SIGHUP, dosig);
	signal (SIGINT, dosig);
	signal (SIGPIPE, dosig);
	signal (SIGTERM, dosig);

	if ((fd = creat (filenamebuf, 0600)) < 0) {
		(void) unlink (filenamebuf);
		if ((fd = creat (filenamebuf, 0600)) < 0) {
			error(filenamebuf);
		}
	}
	lockfile_to_unlink = strdup (filenamebuf);
	if (lockf (fd, F_TLOCK, 0) < 0) {
		warn ("dump file already locked", dumpfile);
		exit(1);
	}
	fsync(fd);
}


void get_fsdump_line(char *buf) {
	static char *cur_ptr;
	char *line_end;
	int len = 0;

	if (buf) {
		cur_ptr = buf;
	}
	line_end = strchr(cur_ptr, '\n');
	if (line_end) {
		len = line_end - cur_ptr;
		strncpy(fsdumpline, cur_ptr, len);
	}
	fsdumpline[len] = '\0';
	cur_ptr = ++line_end;
}

/*
 * Load the heap set that is in dumpfile into memory heap set mh,
 * using file heap set header fh along the way.  The main contents of
 * the heap set in dumpfile is simply mmap'd into our memory space.
 */

int restore(int fd, int ro_only) {
	void *buf;		/* memory mapped buffer for old dumpfile */
	char *lp;		/* scans header lines of buf */
	int flvl;		/* format level seen in buf */
	char unnnbuf[sizeof(un.nodename)];
	char mntptbuf[PATH_MAX];
	dev_t dev;
	long totalinum;
	ino64_t rootinum;
	index primenum;

	if (fd == -1) return -1;
	if (dumpfilesize == 0) {
		fprintf(stderr, "dumpfile <%s> zero length: will rebuild from scratch\n", dumpfile);
		return -1;
	}
	if (ro_only == 1) {
	buf = mmap((void *)0, dumpfilesize,(PROT_READ), MAP_SHARED, fd, 0);
	} else {
	buf = mmap((void *)0, dumpfilesize,(PROT_READ|PROT_WRITE), MAP_PRIVATE|MAP_AUTOGROW|MAP_AUTORESRV, fd, 0);
	}
	if ((int)buf == -1)
		error ("mmap");

	get_fsdump_line(buf);
	if (sscanf (fsdumpline, "fsdump format level %d", &flvl) != 1)
		error ("fsdumpfile line 1");
	if (flvl < 3) {
		fprintf(stderr, "obsolete dumpfile <%s> level %d; will rebuild from scratch\n", dumpfile, flvl);
		return -1;
	}	
	if (flvl != 3)
		error ("Only fsdump level 3 recognized");

	get_fsdump_line(NULL);
	if (sscanf (fsdumpline, "%*s %s %*s %*s %*s", unnnbuf) != 1)
		error ("fsdumpfile line 2");
	if (
	    strncmp (unnnbuf, un.nodename, sizeof(un.nodename)) != 0
	)
		fprintf(stderr,"warning: using dumpfile <%s> from uname -n node <%s> on node <%s>\n", dumpfile, unnnbuf, un.nodename);

	get_fsdump_line(NULL);
	if (sscanf (fsdumpline, "%*s %*s %*s %*s %*s") != 0)		/* ctime */
		error ("fsdumpfile line 3");
	/* should look at restored fsdump date and see if older XXX */

	get_fsdump_line(NULL);
	if (sscanf (fsdumpline, "Mount point: %s device %d", mntptbuf, &dev) != 2)
		error ("fsdumpfile line 4");
#if 0
	/* XXX if filesystem moved/expanded, these errors fire, to no advantage */
	if (strcmp (mntptbuf, Mntpt) != 0)
		error ("not same mount point");
	if (stat (Mntpt, &statbuf) < 0)
		error (Mntpt);
	if (statbuf.st_dev != dev)
		error ("not same dev");
#endif
	get_fsdump_line(NULL);

	if (sscanf (fsdumpline, "Total Inodes %ld", &totalinum) != 1)
		error ("fsdumpfile line 5");
        get_fsdump_line(NULL);
	if (sscanf (fsdumpline, "Root inode %lld", &rootinum) != 1)
		error ("fsdumpfile line 6");
	if (rootinum != rootino) {
		 return -1;
	}
        get_fsdump_line(NULL);
	if (sscanf (fsdumpline, "Prime number used %ld", &primenum) != 1)
		error ("fsdumpfile line 7");
	if (primenum != primeno) {
                fprintf(stderr, "dumpfile <%s> with different primeno; will rebuild from scratch\n", dumpfile);
                return -1;
	/*	error ("not same prime num"); */
	}

        get_fsdump_line(NULL);
	if(sscanf(fsdumpline,"inum offset: %lld size %8u", &fh.fh_inum_off, &fh.fh_inum_sz)!=2)
		error ("fsdumpfile line 8");

        get_fsdump_line(NULL);
        if(sscanf(fsdumpline,"inm2 offset: %lld size %8u", &fh.fh_inm2_off, &fh.fh_inm2_sz)!=2)
                error ("fsdumpfile line 9");

        get_fsdump_line(NULL);
	if(sscanf(fsdumpline,"inod offset: %lld size %u", &fh.fh_inod_off, &fh.fh_inod_sz)!=2)
		error ("fsdumpfile line 10");

        get_fsdump_line(NULL);
	if(sscanf(fsdumpline,"dent offset: %lld size %u", &fh.fh_dent_off, &fh.fh_dent_sz)!=2)
		error ("fsdumpfile line 11");

        get_fsdump_line(NULL);
	if(sscanf(fsdumpline,"link offset: %lld size %u", &fh.fh_link_off, &fh.fh_link_sz)!=2)
		error ("fsdumpfile line 12");

        get_fsdump_line(NULL);
	if(sscanf(fsdumpline,"name offset: %lld size %u", &fh.fh_name_off, &fh.fh_name_sz)!=2)
		error ("fsdumpfile line 13");

        get_fsdump_line(NULL);
	if(sscanf(fsdumpline,"dndx offset: %lld size %u", &fh.fh_dndx_off, &fh.fh_dndx_sz)!=2)
		error ("fsdumpfile line 14");

        get_fsdump_line(NULL);
	if(sscanf(fsdumpline,"dnd2 offset: %lld size %u", &fh.fh_dnd2_off, &fh.fh_dnd2_sz)!=2)
		error ("fsdumpfile line 15");

        get_fsdump_line(NULL);
	if(sscanf(fsdumpline,"fenv offset: %lld size %u", &fh.fh_fenv_off, &fh.fh_fenv_sz)!=2)
		error ("fsdumpfile line 16");

	heapfinit ((void **)(&mh.hp_inum),
		buf, fh.fh_inum_off, fh.fh_inm2_off, sizeof(*mh.hp_inum), fh.fh_inum_sz);
        heapfinit ((void **)(&mh.hp_inm2),
                buf, fh.fh_inm2_off, fh.fh_inod_off, sizeof(*mh.hp_inm2), fh.fh_inm2_sz);
	heapfinit ((void **)(&mh.hp_inod),
		buf, fh.fh_inod_off, fh.fh_dent_off, sizeof(*mh.hp_inod), fh.fh_inod_sz);
	heapfinit ((void **)(&mh.hp_dent),
		buf, fh.fh_dent_off, fh.fh_link_off, sizeof(*mh.hp_dent), fh.fh_dent_sz);
	heapfinit ((void **)(&mh.hp_link),
		buf, fh.fh_link_off, fh.fh_name_off, sizeof(*mh.hp_link), fh.fh_link_sz);
	heapfinit ((void **)(&mh.hp_name),
		buf, fh.fh_name_off, fh.fh_dndx_off, sizeof(*mh.hp_name), fh.fh_name_sz);
	heapfinit ((void **)(&mh.hp_dndx),
		buf, fh.fh_dndx_off, fh.fh_dnd2_off, sizeof(*mh.hp_dndx), fh.fh_dndx_sz);
	heapfinit ((void **)(&mh.hp_dnd2),
		buf, fh.fh_dnd2_off, fh.fh_fenv_off, sizeof(*mh.hp_dnd2), fh.fh_dnd2_sz);
	heapfinit ((void **)(&mh.hp_fenv),
		buf, fh.fh_fenv_off, dumpfilesize, sizeof(*mh.hp_fenv), fh.fh_fenv_sz);

#if 0
	printf("inod %d/%d (%d%%) dent %d/%d (%d%%)\nlink %d/%d (%d%%) name %d/%d (%d%%)\ndndx %d/%d (%d%%) fenv %d/%d (%d%%)\n",
		A(&mh.hp_inod), B(&mh.hp_inod), percent (A(&mh.hp_inod), B(&mh.hp_inod)),
		A(&mh.hp_dent), B(&mh.hp_dent), percent (A(&mh.hp_dent), B(&mh.hp_dent)),
		A(&mh.hp_link), B(&mh.hp_link), percent (A(&mh.hp_link), B(&mh.hp_link)),
		A(&mh.hp_name), B(&mh.hp_name), percent (A(&mh.hp_name), B(&mh.hp_name)),
		A(&mh.hp_dndx), B(&mh.hp_dndx), percent (A(&mh.hp_dndx), B(&mh.hp_dndx)),
		A(&mh.hp_fenv), B(&mh.hp_fenv), percent (A(&mh.hp_fenv), B(&mh.hp_fenv))
	);
#endif
	return 0;
}

time_to_garbage_collect() {
	struct tm *tmp;
	int today, dumpday;
	time_t Now3hrsago, dumpdate3hrsago;

	/*
	 * Collect garbage in heaps the first dump after 3am.
	 * That is: if the date of the dump file is not today's date,
	 *	    then it is time to collect garbage.
	 * Adjust times by ThreeHours to check for 3am
	 * instead of midnight -- to get quietest time period.
	 */

	Now3hrsago = StartTime - ThreeHours;
	tmp = localtime (&Now3hrsago);
	today = tmp->tm_yday;

	dumpdate3hrsago = dumpfiledate - ThreeHours;
	tmp = localtime (&dumpdate3hrsago);
	dumpday = tmp->tm_yday;

	if (today != dumpday) {
#if 1
		printf("Collecting garbage.\n");
#endif
		return 1;
	} else {
#if 1
		printf("Not collecting garbage.\n");
#endif
		return 0;
	}
}

void dualscandir (char *workdir, ino_t d) {
	char pathname[PATH_MAX];
	dent_t *ep1base, *ep2base;
	char *n1p, *n2p;
	inod_t *p1, *p2, *q1, *q2;
	int i, j;

#if 0
	printf ("dualscandir(%s,%d)\n",workdir, d);
#endif
	p1 = PINO(&mh,d);
	assert (S_ISDIR(p1->i_mode));

	p2 = PINO(&mhold,d);
	assert (S_ISDIR(p2->i_mode));

	ep1base = PENT (mh, p1, 0);
	ep2base = PENT (mhold, p2, 0);

	for (i=0; i < p1->i_xndent; i++) {

		ino_t ino = ep1base[i].e_ino;
		if (ino == ep2base[i].e_ino) {
			j = i;
			goto same_ino;
		} else {
			for (j=0; j < p2->i_xndent; j++) {
				if (ino == ep2base[j].e_ino)
					goto same_ino;
			}
			goto no_match;
		}
		/* NOTREACHED */
	same_ino:
		q1 = PINO(&mh,ino);
		q2 = PINO(&mhold,ino);

		if (S_ISDIR (q1->i_mode)) {

			if ( ! S_ISDIR (q2->i_mode))
				goto no_match;
			n1p = PNAM (mh, ep1base+i);
			if (n1p[0] == '.'
			    && (n1p[1] == '\0'
				|| n1p[1] == '.'
				    && n1p[2] == '\0')) {
				goto skipping;
			}
			n2p = PNAM (mhold, ep2base+j);
			if (*n1p != *n2p)
				goto no_match;
			if (strcmp (n1p, n2p) != 0)
				goto no_match;

			/* found 2 directories with same pathname */
			if (workdir[0] == '/' && workdir[1] == 0)
				strcpy(pathname,"/");
			else
				strcat(strcpy(pathname,workdir),"/");
			strcat (pathname, n1p);
			if (ismounted (pathname))
				goto skipping;
			dualscandir (pathname, ino);
			continue;

		} else /* if (!S_ISDIR(q1->i_mode)) */ {

			if (
			    q1->i_mtime != q2->i_mtime
			 ||
			    q1->i_ctime != q2->i_ctime
			 ||
			    q1->i_size != q2->i_size
			 ||
			    q1->i_gen != q2->i_gen
			 ||
			    (q1->i_mode) & S_IFMT != (q2->i_mode) & S_IFMT
			) {
				goto no_match;
			}

			switch ((q1->i_mode) & S_IFMT) {
			  case S_IFSOCK:
			  case S_IFCHR:
			  case S_IFBLK:
				if (q1->i_rdev != q2->i_rdev)
					goto no_match;
			}

			n1p = PNAM (mh, ep1base+i);
			n2p = PNAM (mhold, ep2base+j);
			if (*n1p != *n2p)
				goto no_match;
			if (strcmp (n1p, n2p) != 0)
				goto no_match;

			/* Found same file, unchanged: copy over hard earned fenv */
			if (q2->i_xfenv != (index)0)
				fenvcat (q1, mhold.hp_fenv + q2->i_xfenv);
			continue;
		}
		/* NOTREACHED */

	no_match:
#if 0
		printf ("added or changed: %s/%s\n", workdir, PNAM(mh,ep1base+i));
#endif
		continue;
		/* NOTREACHED */

	skipping:				/* quietly skip ".", ".." and mount pts */
		continue;
	} /* end for loop on directory */
}

/*
 * buildmnttab - borrowed from cmd/find/find.c
 */
void buildmnttab()
{
	FILE *mnttabp;
	struct mntent *mnt;
	struct mntlist *mntlistp, *newmntlist;
	static int bagmtab;

	if (bagmtab)
		return;
	if ( (mnttabp = setmntent(MOUNTED, "r")) == NULL ) {
		/* cant't use mtab, have to stat file */
		bagmtab = 1;
		return;
	}

	while ( (mnt = getmntent(mnttabp)) != 0 ) {
		newmntlist = (struct mntlist *)malloc(sizeof(struct mntlist));
		if ( newmntlist == NULL ) {
			fprintf(stderr,"buildmnttab: malloc failed\n");
			goto end;
		}
		newmntlist->mnt.mnt_dir = strdup (mnt->mnt_dir);
		newmntlist->mnt.mnt_type = strdup (mnt->mnt_type);
		newmntlist->mnt.mnt_freq = mnt->mnt_freq;
		newmntlist->mnt.mnt_passno = mnt->mnt_passno;
		newmntlist->mnt.mnt_fsname = strdup(mnt->mnt_fsname);

		if ( mnthead == 0 ) {
			mnthead = newmntlist;
			mntlistp = newmntlist;
			continue;
		}
		mntlistp->next = newmntlist;
		mntlistp = newmntlist;	
	}
	if (mnthead == 0) {
		fprintf(stderr,"buildmnttab: empty mtab\n");
		bagmtab = 1;
		goto end;
	}
	mntlistp->next = 0;
end:
	endmntent(mnttabp);
	return;
}

ismounted(name)
char *name;
{
	struct mntlist *mntp;

	if ( mnthead == 0 )
		buildmnttab();
	mntp = mnthead;
	if ( mntp == 0 )
		return(0);

	do {
		if ( strcmp(mntp->mnt.mnt_dir, name) == 0 )
			return (1);
		mntp = mntp->next;
	} while ( mntp != 0 ) ;
	return(0);
}

char *mountdev(char *dir){
	struct mntlist *mntp;

	if ( mnthead == 0 )
		buildmnttab();
	mntp = mnthead;
	if ( mntp == 0 )
		return(0);

	do {
		/* XXX when checking mountdevs, should also look for mnt_type == "efs" */
		if ( strcmp(mntp->mnt.mnt_dir, dir) == 0 )
			return (mntp->mnt.mnt_fsname);
		mntp = mntp->next;
	} while ( mntp != 0 ) ;
	return(0);
}

bool_t
isinxfs()
{
	intgen_t fd;
        xfs_fsop_geom_t geo;
        intgen_t rval;

        rval = syssgi( SGI_XFS_FSOPERATIONS,
		       fsd,
                       XFS_FS_GEOMETRY,
                       (void *)0, &geo );
        if ( rval < 0 ) {
                return BOOL_FALSE;
        } else {
/*                return BOOL_TRUE; */
		return getinocnt();
        }
}

bool_t
getinocnt ()
{
        int             count;
        int             i;
	ino64_t		firstino = 0;
        ino64_t         last;
        int             nent = 1;
        xfs_inogrp_t    *t;

        t = malloc(nent * sizeof(*t));
        last = 0;
        while (syssgi(SGI_FS_INUMBERS, fsd, &last, nent, t, &count) == 0) {
                if (count == 0) {
                        return BOOL_TRUE;
		}
		nitotal = t[count-1].xi_startino + t[count-1].xi_alloccount;
        }
	return BOOL_FALSE;
}


void
cmpefsnodes (inod_t *p, struct efs_dinode *di, ino64_t inump)
{
	/*
	 * Test as efficiently as possible for the 2 common
	 * cases: inode never allocated and inode not changed.
	 * Together these 2 cases cover >99% of the inodes,
	 * and this is the key inner loop.  Well - it was ...
	 * Now dndxcmp(), in the qsort of hp_dndx, is the
	 * main user of application CPU cycles.
	 */
	
	if (p->i_mode == 0) {
	        if (di->di_mode == 0)
	                return;       /* never allocated */
	} else if (
	    di->di_mode != 0
	 &&
	    p->i_mtime == di->di_mtime
	 &&
	    p->i_ctime == di->di_ctime
	 &&
	    p->i_size == di->di_size
	 &&
	    p->i_gen == di->di_gen
	 &&
	    p->i_xino == inump
	 &&
	    ((p->i_mode) & S_IFMT) == ((di->di_mode) & S_IFMT)
	) {
	        switch ((p->i_mode) & S_IFMT) {
	        case S_IFSOCK:
	        case S_IFCHR:
	        case S_IFBLK:
	                if (p->i_rdev == di->di_u.di_dev.ndev)
	                        goto inode_not_changed;
	                break;
	        default:
	        inode_not_changed:
	                /*
	                 * We don't really need to test for "!=" before doing
	                 * '=' assignments - except for performance reasons:
	                 * it allows us to avoid copy-on-write page faulting
	                 * in many of the pages underneath mmapped hp_inod.
	                 */
	
	                if (p->i_mode != di->di_mode)
	                        p->i_mode = di->di_mode;
	                if (p->i_nlink != di->di_nlink)
	                        p->i_nlink = di->di_nlink;
	                if (p->i_uid != di->di_uid)
	                        p->i_uid = di->di_uid;
	                if (p->i_gid != di->di_gid)
	                        p->i_gid = di->di_gid;
	                if (p->i_atime != di->di_atime)
	                        p->i_atime = di->di_atime;
	                if (S_ISREG(p->i_mode) || S_ISDIR(p->i_mode)) {
	                        if (p->i_rdev != di->di_numextents)
	                                p->i_rdev = di->di_numextents;
	                }
	                return;
	        }
	}
	
	/*
	 * Now methodically consider all the rare cases.
	 *
	 * If fsdump is run often enough - say every few minutes - then
	 * many of its runs will find ALL 100% of the inodes handled by
	 * the 2 common cases above.  On those runs, there is no need to
	 * do the updating below of hp_dent (in xscandir) or of hp_dndx
	 * (see the qsort(..dndxcmp..)).  Of course, its not worth much
	 * to optimize for the case that the file system was lightly
	 * loaded - but its easy to do here: we set a flag update_dirs
	 * whenever any inodes get this far, and we only do the hp_dent
	 * and hp_dndx updates if update_dirs is set.
	 */
	
	update_dirs = TRUE;
	
	if (domerge == FALSE) {
	        assert (di->di_mode != 0);
	        action = newdump;
	} else {
	        if (p->i_mode == 0) {
	                assert (di->di_mode != 0);
	                action = added;
	        } else if (di->di_mode == 0)
	                        action = deleted;
	                else
	                        action = changed;
	}
	
	if (action == changed || action == deleted) {
	        p->i_xino = inump;
	        p->i_xfenv = (index)0;
	
	        if (S_ISDIR (p->i_mode))
	                wipedir (p);
	
	        if (action == deleted) {
			inm2_t *inm2;
	                p->i_mode = (ushort)0;
	                p->i_xino = (ino_t)0;
			inm2 = PNM2 (&mh, inump & ~63);
			if (inm2) 
				inm2->inoptr[inump & 63] = (index)0;	
	                return;
	        }
	} else {
		inm2_t *inm2;
		index newino;
	        assert (action == added || action == newdump);
	        newino = heapalloc ((void **)(&mh.hp_inod), 1);
	        p = mh.hp_inod + newino;
	        p->i_xino = inump;
		PADD(&mh, newino, inump);
	}
	
	
	assert (action == changed || action == added || action == newdump);
	
	p->i_mode = di->di_mode;
	p->i_nlink = di->di_nlink;
	p->i_uid = di->di_uid;
	p->i_gid = di->di_gid;
	p->i_size = di->di_size;
	p->i_atime = di->di_atime;
	p->i_mtime = di->di_mtime;
	p->i_ctime = di->di_ctime;
	p->i_gen = di->di_gen;
	switch ((p->i_mode) & S_IFMT) {
	  case S_IFSOCK:
	  case S_IFCHR:
	  case S_IFBLK:
	        p->i_rdev = di->di_u.di_dev.ndev;
	        break;
	  case S_IFREG:
	  case S_IFDIR:
	        p->i_rdev = di->di_numextents;
	}
}

void
cmpxfsnodes (xfs_bstat_t *xp)
{
	ino64_t inump = xp->bs_ino;
	inod_t *p = PINO(&mh, inump);

        /*
         * Test as efficiently as possible for the 2 common
         * cases: inode never allocated and inode not changed.
         * Together these 2 cases cover >99% of the inodes,
         * and this is the key inner loop.  Well - it was ...
         * Now dndxcmp(), in the qsort of hp_dndx, is the
         * main user of application CPU cycles.
         */

        if (p->i_mode != 0 		/* inode was previously used */
					/* Check if unchanged */
	 &&
            p->i_mtime == xp->bs_mtime.tv_sec
         &&
            p->i_ctime == xp->bs_ctime.tv_sec
         &&
            p->i_size == xp->bs_size
         &&
            p->i_gen == xp->bs_gen
         &&
            p->i_xino == xp->bs_ino
         &&
            ((p->i_mode) & S_IFMT) == ((xp->bs_mode) & S_IFMT)
        ) {
                switch ((p->i_mode) & S_IFMT) {
                case S_IFSOCK:
                case S_IFCHR:
                case S_IFBLK:
                        if (p->i_rdev == xp->bs_rdev)
                                goto inode_not_changed;
                        break;
                default:
                inode_not_changed:
                        /*
                         * We don't really need to test for "!=" before doing
                         * '=' assignments - except for performance reasons:
                         * it allows us to avoid copy-on-write page faulting
                         * in many of the pages underneath mmapped hp_inod.
                         */

                        if (p->i_mode != xp->bs_mode)
                                p->i_mode = xp->bs_mode;
                        if (p->i_nlink != xp->bs_nlink)
                                p->i_nlink = xp->bs_nlink;
                        if (p->i_uid != xp->bs_uid)
                                p->i_uid = xp->bs_uid;
                        if (p->i_gid != xp->bs_gid)
                                p->i_gid = xp->bs_gid;
                        if (p->i_atime != xp->bs_atime.tv_sec)
                                p->i_atime = xp->bs_atime.tv_sec;
                        if (S_ISREG(p->i_mode) || S_ISDIR(p->i_mode)) {
                                if (p->i_rdev != xp->bs_extents)
                                        p->i_rdev = xp->bs_extents;
                        }
                        return;
                }
        }

        /*
         * Now methodically consider all the rare cases.
         *
         * If fsdump is run often enough - say every few minutes - then
         * many of its runs will find ALL 100% of the inodes handled by
         * the 2 common cases above.  On those runs, there is no need to
         * do the updating below of hp_dent (in xscandir) or of hp_dndx
         * (see the qsort(..dndxcmp..)).  Of course, its not worth much
         * to optimize for the case that the file system was lightly
         * loaded - but its easy to do here: we set a flag update_dirs
         * whenever any inodes get this far, and we only do the hp_dent
         * and hp_dndx updates if update_dirs is set.
         */

        update_dirs = TRUE;

        if (domerge == FALSE) {
                action = newdump;
        } else {
                if (p->i_mode == 0) {
                        action = added;
                } else {
			action = changed;
		}
	}

        if (action == changed)  {
                p->i_xino = xp->bs_ino;
                p->i_xfenv = (index)0;

                if (S_ISDIR (p->i_mode))
                        wipedir (p);
        } else {
                inm2_t *inm2;
                index newino;
                assert (action == added || action == newdump);
                newino = heapalloc ((void **)(&mh.hp_inod), 1);
                p = mh.hp_inod + newino;
                p->i_xino = inump;
                PADD(&mh, newino, inump);
        }


        assert (action == changed || action == added || action == newdump);

        p->i_mode = xp->bs_mode;
        p->i_nlink = xp->bs_nlink;
        p->i_uid = xp->bs_uid;
        p->i_gid = xp->bs_gid;
        p->i_size = xp->bs_size;
        p->i_atime = xp->bs_atime.tv_sec;
        p->i_mtime = xp->bs_mtime.tv_sec;
        p->i_ctime = xp->bs_ctime.tv_sec;
        p->i_gen = xp->bs_gen;
        switch ((p->i_mode) & S_IFMT) {
          case S_IFSOCK:
          case S_IFCHR:
          case S_IFBLK:
                p->i_rdev = xp->bs_rdev;
                break;
          case S_IFREG:
          case S_IFDIR:
                p->i_rdev = xp->bs_extents;
        }
}

void
markunused (ino64_t from, ino64_t to)
{
	index inump; 
	inod_t *p;
	ino64_t ft;
	inm2_t *inm2;

	for ( ft = from; ft < to ; ft++) {
		if (inm2 = PNM2 (&mh, ft & ~63)) {
			if (inump = inm2->inoptr[ft & 63]) {
				(mh.hp_inod + inump)->i_mode = (ushort)0;
				(mh.hp_inod + inump)->i_xino = (ino_t)0;
				inm2->inoptr[ft & 63] = (index)0;
			}
		}
	} 
}

void
genprimenum()
{
	char	*p;
	ino64_t	i,j;

	int hashpwr = ((log(niinuse)/M_LN2)/2 - 1);

	ino64_t hashino = 2 << hashpwr;

	if (hashino < 2) {
		primeno = 2;
		return;
	}

	p = malloc(hashino);

	for (i = 0; i < hashino; i++)
		p[i] = '0';

	for (i = 2; i < hashino; i++)
		if (p[i] == '0') {
			for (j = 2; j * i < hashino; j++)
				p[j*i] = '1';
		}

	for (i=hashino; i > 0; i--)
		if (p[i] == '0') {
			primeno = i;
			free(p);
			return;
		}
}

void
gethash ()
{
        int             count;
        int             i;
        ino64_t         firstino = 0;
        ino64_t         last;
        int             nent = 1;
        xfs_inogrp_t    *t;
        int             *hashtb;
        unsigned int    hkey;

        hashtb = malloc(primeno * sizeof(int));
        for (i=0; i < primeno; i++)
                hashtb[i] = 0;
        t = malloc(nent * sizeof(*t));
        last = 0;
        while (syssgi(SGI_FS_INUMBERS, fsd, &last, nent, t, &count) == 0) {
                if (count == 0) {
                        break;
                }
/*              hkey = (unsigned int)hashfunc(t[count-1].xi_startino & 0xffffffc0); */
                hkey = (unsigned int)hashfunc(t[count-1].xi_startino & ~63);
                hashtb[hkey]++;
        }
        for(i = 0; i < primeno; i++)
                printf ("For index %10d count %10d\n", i, hashtb[i]);
}
