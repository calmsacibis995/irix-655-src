/* from tar.c       4.19 (Berkeley) 9/22/83 */
#ident "tar/tar.c: $Revision: 1.124 $"

/*
 *  Modified 1/84 by Dave Yost
 *   - At end of archive, tar now tries two more reads to find end of file
 *     so that mag tape is properly positioned at the beginning of the next
 *     tape file.
 *   - Recognize premature end of file as such.  These last two
 *     called for changes to getdir(), readtape(), passtape(), endtape(),
 *     and the places where they are called.
 *   - Add 'e' option to continue after tape read errors.
 *   - Add 'X' option, which is like 'x' except it compares
 *     against another tree and links to there on files from tape
 *     which are identical to their counterparts in the specified tree.
 *   - Add 'C' option to compare the tape against the file system.
 *   - Changed the linkbuf struct, putting the filename string at the
 *     end and allocating just enough space for it, instead of making
 *     it a fixed-size array which is hoped to be big enough.
 *   - Replaced some of the rflag, tflag, etc. variables with a 'work'
 *     variable that is set to the type of work to be done.
 *   - Enforce blocking factor limit of 20.  If tar can't extract it, why
 *     let it write it?
 *   - The 'cannot link' message now says what it cannot link to.
 *   - The 'x filename' message printed when extracting now holds back the
 *     newline until the file is finished extracting so you can know when
 *     it is done if you are only extracting one file and want to kill it.
 *   - Print out group owner %-2d instead of %d.
 *   - Changed default device to the logical device /dev/tar instead of
 *     /dev/rmt0.  Let'em make the appropriate device for their system.
 *   - Complain and exit if tarfile specified with 'f' option is in /dev
 *     and either doesn't exist or is a regular file.
 *   - Put in some ifdefs so this source will work on more vanilla systems.
 *   - Buffer stderr and flush it after each write.  (4.2 line buffering not
 *     used because it reverts to block buffering when tar output log is
 *     redirected to a file and because it is not portable.)
 *   - Clean up file names before writing them on the directory block:
 *     no leading '//' or './' garbage.
 *   - If a filename on tape has a leading './', pretend it doesn't.
 *   - Added 'R' option to ignore leading '/' when extracting.
 *   - Fixed bug: tar got mixed up on what its parent directory was if you
 *     tried to archive a directory starting with '/'.
 *   - Added 'U' option to unlink before each creat.
 *   - Fix bug where file descriptor was not freed up when file name too
 *     long in putfile in the IF_REG case.
 *   - Tar passes lint now.
 *
 *  Modified 7/84 by Bob Toxen of Silicon Graphics, Inc.
 *
 *  1. Tar CAN NOW BACK UP CHAR/BLOCK DEVICES AND NAMED PIPES!!!
 *  2. Renamed '-o' option to '-d'.
 *  3. Added   '-o' option to not do CHOWN or CHGRP on extracted files.
 *  4. Default blocking factor for our cartridge tape is now 400.
 *  5. Fix bug so Tar will cope with pwd returning excessively long path.
 *  6. Added   '-V' option to allow variably sized last block to avoid large
 *     write for last block for hugely blocked tapes.
 *  7. Can now have huge blocking factors.
 *  8. Fixed bug whereby Tar hung on device and named pipe files (see #1).
 *  9. Fixed bug where Tar stripped set-U/Gid & sticky bit from modes.
 * 10. Fixed bug whereby a compiler warning about NULL being redefined.
 * 11. Changed default archive file back to /dev/rmt1.
 * 12. Added support for System III and System V.
 * 13. Jazzed up conditional code & added comments.
 * 14. Allow specifying all drives from 1 to 9, don't have to have 4.2BSD MT.
 * 15. In verbose mode tells when symbolic links are changed to hard links.
 * 16. Files in directories are written to tape alphabetically!!!!!
 * 17. Fixed bug whereby if "/" is to be backed up tar tried to open ""
 *     which was interpreted as "." on V7 but is illegal on System III & V.
 * 18. Can now handle multiple tape volumes. If -e (continue on errors)
 *     is NOT specified then Tar will consider any error return to mean
 *     end of tape. Otherwise any error except EIO will mean end of tape.
 * 19. Don't try to read past end of tape if OLDMAGTAPE.
 * 20. If file name "-" is on command line then read list of files from
 *     standard input (thanks to Steve Hartwell).
 * 21. Added -a to preserve access time on files read (10/10/84).
 */

/*
 * Tape Archival Program
 */

#define	OLDMAGTAPE
#define	CART
#define	FASTDIE

#define DEVTAR  "/dev/tape"
#define DEVTAR2 "/dev/tape1"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <utime.h>
#include <unistd.h>
#include <bstring.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <pwd.h>			/* POSIX */
#include <grp.h>			/* POSIX */
#include <locale.h>
#include <nl_types.h>
#include <msgs/uxeoe.h>

/*
 * SGI: getbsize is unconditionally part of tar, so we need this.
 */
#include <sys/mtio.h>
#include <sys/tpsc.h>	/* for audio turn off */
#include <tar.h>
#ifdef	TRUSTEDIRIX
#include <sys/mac.h>
#include <sys/acl.h>
#endif  /* TRUSTEDIRIX */


#define	YES 1
#define	NO  0
#undef	MIN
#define	MIN(a,b) (((a) < (b))? (a): (b))

#define	NBLOCK	20	/* default blocking factor */

#ifdef	CART
#define	NBLOCKC	400	/* default SGI cartridge blocking factor */
#else
#define	NBLOCKC	20	/* default blocking factor */
#endif
#define	DFUDGE	20	/* fudge factor on growing directories */

#define S_PERMMASK (~S_IFMT)	/* mask for permission part of modes */

#define	TBLOCK	512	/* size of a tape block */

#define	FCHR	0020000
#define FDIR	0040000
#define	FBLK	0060000
#define	FIFO	0010000

/* even for SVR3 must keep NAMSPACE 100 and not MAXPATHLEN because TAR
 * tape block size written in stone (TBLOCK)
 */
#define	NAMSPACE	100	/* total storage, including terminating NULL */
#define NAMSIZ		(NAMSPACE-1)	/* max # of 'usable' chars */

#define	PREFIXSPACE	156		/* POSIX: prefix space including NULL */
#define	PREFIXSIZ	(PREFIXSPACE-1)

/*
 * Added for xfs: Block size for large file transfers
 */

#define BIGBLOCK ((long long)1<<((sizeof(long)*CHAR_BIT)-1))

union hblock {
	char dummy[TBLOCK];
	struct header {
		char name[NAMSPACE];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char chksum[8];
		char linkflag;
		char linkname[NAMSPACE];
		char rdev[12];
	} dbuf;
	struct pheader {				/* POSIX */
		char name[NAMSPACE];			/* POSIX */
		char mode[8];				/* POSIX */
		char uid[8];				/* POSIX */
		char gid[8];				/* POSIX */
		char size[12];				/* POSIX */
		char mtime[12];				/* POSIX */
		char chksum[8];				/* POSIX */
		char typeflag;				/* POSIX */
		char linkname[NAMSPACE];		/* POSIX */
		char magic[6];				/* POSIX */
		char version[2];			/* POSIX */
		char uname[32];				/* POSIX */
		char gname[32];				/* POSIX */
		char devmajor[8];			/* POSIX */
		char devminor[8];			/* POSIX */
		char prefix[PREFIXSPACE];		/* POSIX */
	} pdbuf;					/* POSIX */
};
/* #define linkflag typeflag */

struct linkbuf {
	struct	linkbuf *nextp;
	dev_t	devnum;
	ino64_t	inum;
	short	count;
	char	pathname[1];	/* actually alloced larger */
};

union	hblock dblock;
union	hblock *tbuf;
char	*dblockname;		/* pointer to name */
char	*dblocklnname;		/* pointer to link name*/
char	*dblocksymname;		/* pointer to symbolic linkname*/
struct	linkbuf *ihead;
struct	stat64 stbuf;
#ifdef  TRUSTEDIRIX
#define TIRIX_PREFIX     "/tmp/TRUSTEDIRIX-label-data."
#define ATTRFILE         "attribute"
char    tirix_file[NAMSIZ];
char 	*g_attrbuf;
char    g_lname[256];			/* to go with stbuf */
char    g_aclstr[256];			/* to go with stbuf */
mac_t	g_lp;		 		/* to go with g_lname */
acl_t	g_aclp; 
void    attr_from_tape(void);
void	clear_gacl(void);
void	clear_glabel(void);
void    clear_attr(void);
#endif  /* TRUSTEDIRIX */


/*
 *  The kind of work we are going to do is determined
 *  by the value of the 'work' variable.  Other aspects
 *  of the work are influenced by other flags.
 */
typedef void (*funcaddr_t)(char **);       /* Pointer to a function */
funcaddr_t work;
#define NOWORK  (funcaddr_t)0

int	vflag;
int	cflag;
int	mflag;
int	fflag;
int	dflag;		/* don't backup directory files			*/
int	pflag;
int	wflag;
int	Lflag;
int	Bflag;
int	Kflag;		/* Added for xfs */
int	Eflag;		/* SGI: Exclude any non-local files/dirs (including symlinks
	* to remote, if L also specified.  This includes dirs given on the
	* command line.  Used primarily for full system backups */
int	Fflag;		/* undocumented 4.2 flag */
int Nflag;	/* extract only if it doesn't exist */
int	continflag;	/* continue on read errs */
char	lnflag;		/* -X  link from linkdir if identical */
int	Rflag;		/* ignore leading '/' when extracting */
int     Uflag;  	/* unlink before creating each file */
int	Vflag;		/* SGI: Variable blocking */
int	oflag;		/* SGI: don't chown files			*/
int	bflag;		/* SGI: user set blocking			*/
int	Dflag;		/* SGI: don't backup device/pipe files		*/
int	aflag;		/* SGI: restore access time			*/
int	Pflag;		/* SGI: use POSIX header format */
int	Sflag;		/* SGI: 3.3.* and prior chown behavior		*/
#ifdef  TRUSTEDIRIX
int     Mflag;          /* store/expect phantom label files             */
#endif  /* TRUSTEDIRIX */

/* the max filename length for POSIX format allows 255 chars plus one
 * terminating NULL char (NAMSPACE+PREFIXSPACE).  In the non-POSIX 
 * format, the only filename array is NAMSPACE (100) bytes, which
 * must store the NULL too.  Therefore, in the POSIX format maxnamlen
 * == NAMSPACE+PREFIXSIZ, otherwise it is NAMSIZ, where PREFIXSIZ and
 * NAMSIZ are defined to be one less than their *SPACE counterparts. */
int	maxnamlen = NAMSPACE + PREFIXSIZ;	/* default to POSIX */

int	infrompipe;

int	mt;
int	term;
int	chksum;
int	recno;
int	first;
int	linkerrok;
char	*linkdir;	/* link from this directory if identical	*/
int	freemem = 1;
int	nblock;
int	isatape = 0;	/* output to a device (as opposed to pipe or file).
	used to decide whether we should check for certain types of tape
	errors, and also multi-vol archive stuff. */
int	debug;		/* activate with -q				*/

off64_t	low;
off64_t	high;

FILE	*tfile;
char	tname[] = "/tmp/tarXXXXXX";
char	*usefile;
char	*defaultdev = DEVTAR;
char    *defdev2 = DEVTAR2;

int atstrcmp(char **a, char **b);
void backtape(void);
int bread(int fd, char *buf, int size, char *eot);
off64_t bsrch(char *s, int n, off64_t l, off64_t h);
int bufcmp(register char *cp1, register char *cp2, register int num);
int bwrite(int fd, char *buf, int size, char *eot);
int checkdir(register char *name);
int checkf(char *name, mode_t mode, int howmuch);
int checksum(void);
int checkupdate(char *arg);
int checkw(int c, char *name);
void chkandfixaudio(int mt);
void choose(int *pairp, struct stat64 *st);
int cmp(char *b, char *s, int n);
void cmpname(char type, int do_nl);
int cmprd(int ifile, char *buf, long num);
void cond_unlink(char *name);
int dirpart(char *str);
void docompare(char *argv[]);
void done(int n);
void dorep(char *argv[]);
void dotable(char *argv[]);
void doxtract(char *argv[]);
void endread(void);
int endtape(void);
gid_t findgid(struct pheader *hp);
char *findgname(gid_t gid);
uid_t finduid(struct pheader *hp);
char *finduname(uid_t uid);
void flushtape(void);
int getbsize(int fd);
int getdir(void);
int gotit(register char **list, register char *name);
char *getlink(struct pheader *hp);
char *getname(struct pheader *hp);
void longt(register struct stat64 *st, char *name);
off64_t lookup(char *s);
int myio(int (*fn)(), int fd, char *ptr, int n, char *eot);
void onhup(void);
void onintr(void);
void onquit(void);
int openmt(char *tape, int writing);
static char parseaction(char *cp);
void passtape(void);
void pmode(register struct stat64 *st);
int prefix(register char *s1, register char *s2);
void premature_eof(void);
void putempty(void);
void putfile(char *longname, char *shortname, char *parent);
int putname(struct pheader *hp, char *wholename);
int readtape(char *buffer);
int response(void);
int signedchecksum(void);
void terror(int isread, int exval);
void tomodes(register struct stat64 *sp);
void usage(void);
void writetape(char *buffer);
void zeroendtbuf(void);

#ifdef RMT
int	rmtopen(char *, mode_t);
int	rmtcreat(char *, mode_t);
int	rmtclose(int);
int	rmtlseek(int, off_t, int);
int	rmtread(int, char *, size_t);
int	rmtwrite(int, char *, size_t);
int	rmtioctl(int, int, ...);
#define ioctl rmtioctl	/* all ioctls are done on device only,
	reduces ifdefs. Olson */
#endif /* RMT */



char	action = 0;		/* this char holds what tar will do */
int	minusok = 0; 		/* only allow '-' (piped filelist) w/ r,u,& c */
int	imroot = 0;		/* is tar running as superuser? */
static mode_t Oumask = 0;	/* old umask value */
nl_catd catd;

main(int argc, char *argv[])
{
	char stderrbuf[BUFSIZ];
	char    *usefil1;
	char *cp;

	(void) setlocale(LC_ALL, "");
	catd = catopen("uxeoe", 0);

#ifdef  TRUSTEDIRIX
        sprintf(tirix_file, "%s%d", TIRIX_PREFIX, getpid());
#endif  /* TRUSTEDIRIX */
	usefil1 = DEVTAR2;
	Pflag++;	/* by default use POSIX format */

	setbuf(stderr, stderrbuf);
	if (geteuid() == (uid_t)0)
		imroot++;

	if (argc < 2)
		usage();

	tfile = NULL;
	argv[argc] = 0;			/* may be illegal! */
	argv++;			/* bump over progname */
	cp = *argv++;

	/* skip leading '-' only if preceeds a real option */
	if ((*cp == '-') && (cp[1] != '\0')) {
		if (debug)
			printf(CATGETS(catd,_MSG_TAR_SKIPPING_FORMAT_C),*cp);  
		cp++;
	}

	action = parseaction(cp);

	switch(action) {	/* get the operation for tar to perform */

	case 'u':	/* update archive */
		mktemp(tname);
		if ((tfile = fopen(tname, "w")) == NULL) {
			fprintf(stderr,
			 CATGETS(catd,_MSG_TAR_TAR_CANNOT_CREATE_TEMPORARY),  
			 tname);
			done(1);
		}
		fprintf(tfile, "!!!!!/!/!/!/!/!/!/! 000\n");
		
		/* FALL THRU */

	case 'r':	/* append files to end of archive */
		work = dorep;
		minusok++;
#ifdef  TRUSTEDIRIX
                Mflag = -1;
#endif  /* TRUSTEDIRIX */
                break;

	case 'X':	/* extract files from archive with linking */
		if (*argv == 0) {
			fprintf(stderr,
		CATGETS(catd,_MSG_TAR_TAR_LINKDIR_MUST_BE));  
			usage();
		}
		linkdir = *argv++;
		lnflag++;
#ifdef  TRUSTEDIRIX
                Mflag = -1;
#endif  /* TRUSTEDIRIX */
		/* FALL THRU */

	case 'x':	/* extract files from archive */
		work = doxtract;
		break;


	case 't':	/* list file names */
		work = dotable;
		break;

	case 'c':	/* create a new archive */
		cflag++;
		work = dorep;
		minusok++;
		break;

	case 'C':	/* Compare files */
		work = docompare;
#ifdef  TRUSTEDIRIX
                Mflag = -1;
#endif  /* TRUSTEDIRIX */
		break;

	default:	/* can never happen now that parseaction checks */
		fprintf(stderr,CATGETS(catd,_MSG_TAR_FIRST_FLAG_MUST_BE));  
		usage();
	}

	if (debug)
		fprintf(stderr,CATGETS(catd,_MSG_TAR_ACTION_FORMAT_C),action);  
			
	/* Now fetch remaining opts and/or parms.  */
	for (; *cp; cp++)

		switch (*cp) {

		case 'r':
		case 'u':
		case 'x':
		case 'X':
		case 't':
		case 'c':
		case 'C':	break;  /* already seen by parseaction() */

		case 'f':
			if (*argv == 0) {
				fprintf(stderr,
			CATGETS(catd,_MSG_TAR_TAR_TAPEFILE_MUST_BE));  
				usage();
			}
			usefile = *argv++;
			fflag++;
			break;

		case 'b':	/* this case must come after 'f'; mandated
				 * by parseaction() and the man page */

			if (*argv == 0) {
				fprintf(stderr,
			CATGETS(catd,_MSG_TAR_TAR_BLOCKSIZE_MUST));  
				usage();
			}
			nblock = atoi(*argv);
			if (nblock <= 0) {
				fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_INVALID_BLOCKSIZE));  
				done(1);
			}
			bflag++;
			argv++;
			break;

		case 'e':
			continflag++;
			break;

		case 'd':
			dflag++;
			Dflag++;
			break;

		case 'p':
			pflag++;
			break;

		case 'v':
			vflag++;
			break;

		case 'w':
			wflag++;
			break;

		case 'R':
			Rflag++;
			break;

		case 'U':
			Uflag++;
			break;
#ifdef  TRUSTEDIRIX
                case 'M':
			if (!sysconf(_SC_MAC) && !sysconf(_SC_ACL)) {
                                fprintf(stderr,
				"tar: 'M' option may only be specified with Trusted Systems\n");
				usage();
			}
                        if (Mflag < 0) {
                                fprintf(stderr,
        			CATGETS(catd,_MSG_TAR_TAR_M_OPTION_MAY_ONLY));
				usage();  
                        }
                        Mflag = 1;
                        break;
#endif  /* TRUSTEDIRIX */
		case 'm':
			mflag++;

			break;

		case 'a':
			aflag++;
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			fprintf( stderr,
			CATGETS(catd,_MSG_TAR_TAR_USE_F_TAPEFILE), DEVTAR );  
			usage();
			/* NOTREACHED */

		case 'l':
			linkerrok++;
			break;

		case 'H':
		case 'h':
			Lflag = 0;
			break;
		
		case 'L':
			Lflag++;
			break;

		case 'B':
			Bflag++;
			break;

		case 'E':
			Eflag++;
			break;

		case 'F':
			Fflag++;
			break;

		case 'N':
			Nflag++;
			break;

		case 'K':
			if (!Pflag) {
				fprintf(stderr,
				   CATGETS(catd,_MSG_TAR_K_AND_O_FLAGS_ARE_INCOMPATIBLE));  
				usage();
			}
			Kflag++;
			break;

		case 'V':		/* SGI: Variable blocking	*/
			Vflag++;
			break;

		case 'o':		/* SGI (pre 5.2): don't chown	*/
			if (Sflag) {
				fprintf(stderr,
				   CATGETS(catd,_MSG_TAR_O_AND_S_FLAGS_ARE_INCOMPATIBLE));  
				usage();
			}
			oflag++;
			break;

		case 'S':	/* SGI (pre SVR4): chown to tape user	*/
			if (oflag) {
				fprintf(stderr,
				   CATGETS(catd,_MSG_TAR_O_AND_S_FLAGS_ARE_INCOMPATIBLE));  
				usage();
			}
			Sflag++;
			if (pathconf(NULL,_PC_CHOWN_RESTRICTED) == 1) {
				fprintf(stderr,
					CATGETS(catd,_MSG_TAR_TTWARNING_RESTRICTED));  
				fprintf(stderr,
					CATGETS(catd,_MSG_TAR_WONT_ALLOW_S_OPTION));  
				fflush(stderr);
			}
			break;

		case 'D':		/* SGI: don't back devs/pipes	*/
			Dflag++;
			break;

		case 'q':		/* SGI: bump up debugging level	*/
			debug++;
			break;

		case 'O':		/* SGI: write pre-POSIX format */
			if (Kflag) {
				fprintf(stderr,
				   CATGETS(catd,_MSG_TAR_K_AND_O_FLAGS_ARE_INCOMPATIBLE));  
				usage();
			}
			Pflag = 0;
			maxnamlen = NAMSIZ;
			break;

		default:
			fprintf(stderr,
				CATGETS(catd,_MSG_TAR_TAR_FORMAT_C_UNKNOWN),*cp);  
			usage();
		}
	if (!usefile) {
		/* we've had an RFE on this for along time, and svr4EA5 does it,
		 * so we'll add it.  I'm not adding the whole default table stuff
		 * EA5 has though! 'f' option overrides. Tell them about it with 'q'
		 */
		usefile = getenv("TAPE");
		if(usefile) {
			if(debug)
				fprintf(stderr, CATGETS(catd,_MSG_TAR_USE_TAPE_AS_ARCHIVE), usefile);  
		}
		else
			usefile = defaultdev;
	}

	if (!strncmp("/dev/", usefile, 5)) {
		struct stat64 stbuf;

		if (stat64(usefile, &stbuf) < 0
		  || (stbuf.st_mode & S_IFMT) == S_IFREG) {
			if (!fflag) {
    
			    if (stat64(usefil1, &stbuf) < 0)
				{
				/* 	IFREG check appears to be here so we don't output to
				/dev/tape if 'f' not given, and it happens to be a file. */
				/*  Also added invalid blocksize in error message -- hack
				    over a hack!!
				*/
				fprintf(stderr,
					CATGETS(catd,_MSG_TAR_TAR_ARCHIVE_FILE_FORMAT),   
					usefile);

				done(1);

			    }
			    else {
				usefile = usefil1;

			    }
			}
			else {
				/* 	IFREG check is here so we don't output to a file in
				* /dev even if 'f' given, and it happens to be a file. */
				/*  Also added invalid blocksize in error message -- hack
				    over a hack!!
				*/
				fprintf(stderr,
    					CATGETS(catd,_MSG_TAR_TAR_ARCHIVE_FILE_FORMAT),   
					usefile);
				done(1);
			}
		}
		isatape = 1;
	}

	if (work == dorep) {
		mt = openmt(usefile, 1);
		dorep(argv);
	}
	else if (work != NOWORK) {
		mt = openmt(usefile, 0);
		(*work) (argv);
	} else
		usage();
	done(0);
	/* NOTREACHED */
	return 0;
}

void
usage(void)
{
	fprintf(stderr, 
		CATGETS(catd,_MSG_TAR_USAGE_TAR_RUXXTCCACDEFHLMOSPQVWLUBDRVOKFB));  
	done(1);
}

openmt(char *tape, int writing)
{
	int pgsize;

	if (strcmp(tape, "-") == 0) {
		/*
		 * Read from standard input or write to standard output.
		 */
		if (writing) {
			if (cflag == 0) {
				fprintf(stderr,
			 		CATGETS(catd,_MSG_TAR_TAR_CAN_ONLY_CREATE));  
				done(1);
			}
			mt = dup(1);
			if (!Vflag)
				nblock = 1;
		} else {
			infrompipe = 1;
			mt = dup(0);
			if (!Vflag)
				nblock = 1;
		}
	} else {
		/*
		 * Use file or tape on local machine.
		 */
		if (writing) {
			int saverr = 0;
			mt = -1;
			/* If -c then truncate regular files! */
#ifdef RMT
			if (cflag) {
				mt = rmtcreat(usefile, 0666);
				if(mt == -1) saverr = errno;
			}
			if(mt < 0)
				mt = rmtopen(usefile, cflag?1:2);
#else
			if (cflag) {
				mt = creat(usefile, 0666);
				if(mt == -1) saverr = errno;
			}
			if (mt < 0)
				mt = open(usefile, cflag?1:2);
#endif /* RMT */
			if(mt == -1 && saverr && errno == ENOENT) errno = saverr;
#ifdef RMT
		} else 
			mt = rmtopen(tape, 0);
#else
		} else 
			mt = open(tape, 0);
#endif /* RMT */
		if (mt <0) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_FORMAT),tape,strerror(errno));  
			done(1);
		}
		else
			chkandfixaudio(mt);
	}
	if (!bflag || (nblock == 0) )
		nblock = getbsize(mt);
	if((pgsize=getpagesize())<=0) {
		fprintf(stderr, CATGETS(catd,_MSG_TAR_ILLEGAL_GETPAGESIZE), pgsize);  
		done(1);
	}
	pgsize--;	/* make it a mask */
	tbuf = (union hblock *)malloc((unsigned int)(nblock*TBLOCK+pgsize));
	if (tbuf == NULL) {
		fprintf(stderr,
		    CATGETS(catd,_MSG_TAR_TAR_BLOCK_SIZE_FORMAT),  
		    nblock * TBLOCK);
		done(1);
	}
	/* align to page boundary for best performance */
	tbuf = (union hblock *)(((unsigned int)tbuf+pgsize)&~pgsize);
	return(mt);
}

/*
 * Try to figure out the block size using a mag tape ioctl.  Sketch:
 *	if (using standard input/output archive)
 *		dup stdin or stdout;
 *	else if (open() fails && not creating a named archive)
 *		complain and die;
 *	if (get block size via ioctl() fails)
 *		return default block size;
 *	else
 *		return ioctl() block size;
 */
int
getbsize(int fd)
{
	register int bsize;

	bsize = 0;
	if (fd >= 0) {
		auto int blksize;

		if (ioctl(fd, MTIOCGETBLKSIZE, &blksize) == 0)
			bsize = blksize;
	}
	if (bsize <= 0)
		bsize = NBLOCK;
	/*	The stat() in main doesn't set isatape for remote
		tapes, because it always fails for remote devices as of 1/89
		(even if you use rmtstat().  So, if the ioctl above succeeded,
		we are clearly dealing with a tape, and we set isatape.  This
		also catches the case where the 'f' option was given, and
		the pathname didn't start with /dev.  Olson, 1/89. */
	else
		isatape = 1;
	return bsize;
}

void
dorep(char *argv[])
{
	char wdir[MAXPATHLEN];
	int anywritten = 0;

	if (cflag && tfile != NULL)
		usage();
#ifndef	FASTDIE
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, onintr);
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, onhup);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, onquit);
#endif	/* FASTDIE */

	if (!cflag) {		/* 'r' mode */
		while (getdir()) {
			passtape();
			if (term) {
				if (debug)
					fprintf(stderr,CATGETS(catd,_MSG_TAR_EXITING_FROM_DOREP));  
				done(0);
			}
		}
		backtape();
		if (tfile != NULL) {	/* 'u' mode */
			char buf[200];

			sprintf(buf,
"sort +0 -1 +1nr %s -o %s; awk '$1 != prev {print; prev=$1}' %s >%sX; mv %sX %s",
				tname, tname, tname, tname, tname, tname);
			fflush(tfile);
			system(buf);
			freopen(tname, "r", tfile);
			fstat64(fileno(tfile), &stbuf);
			if ((stbuf.st_mode & S_IFMT) != S_IFREG)
				stbuf.st_size = 0;
			high = stbuf.st_size;
		}
	}

				/*
				 * Modified by Bob to do Steve Hartwell's
				 * enhancement of reading list of files to
				 * backup from stdin if file name is "-".
				 */
	(void) getwd(wdir);
	while (*argv && ! term) {
		register char *cp, *cp2;
		char tempdir[MAXPATHLEN], *parent;
		char stdinfname[MAXPATHLEN+2], *filearg;

		if (!strcmp(*argv, "-C") && argv[1]) {
			argv++;
			if (chdir(*argv) < 0)
				fprintf(stderr,CATGETS(catd,_MSG_TAR_FORMAT_S_FORMAT_S_1),*argv,  
					strerror(errno));
			else
				(void) getwd(wdir);
			argv++;
			continue;
		}
		if (!strcmp(*argv,"-")) {
			if (gets(stdinfname) == NULL) {
				argv++;
				continue;
			}
			filearg = stdinfname;
		} else
			filearg = *argv++;
		cp2 = 0;
		/* deal with trailing /'s in names, such as
		 * as tar cv /usr/  Otherwise we pass an emptry
		 * string to putfile as 2nd arg, producing bogus errors. */
		for (cp = &filearg[strlen(filearg)-1]; cp>filearg && *cp=='/'; cp--)
			*cp = '\0';
		for (cp = filearg; *cp; cp++)
			if (*cp == '/')
				cp2 = cp;
		if (cp2) {
			*cp2 = '\0';
			if (chdir(filearg[0] ? filearg : "/") < 0) {
				fprintf(stderr,CATGETS(catd,_MSG_TAR_FORMAT_S_FORMAT_S_1),filearg,  
					strerror(errno));
				continue;
			}
			parent = getwd(tempdir);
			*cp2 = '/';
			if(cp2[1])
				cp2++;
		} else {
			cp2 = filearg;
			parent = wdir;
		}
		putfile(filearg, cp2, parent);
		anywritten = 1;
		if (chdir(wdir) < 0) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_CANNOT_CHANGE_BACK));  
			fprintf(stderr,CATGETS(catd,_MSG_TAR_FORMAT_S_FORMAT_S_1),wdir,strerror(errno));  
			fflush(stderr);
		}
	}
	if(anywritten) {	/* this is so a mistaken c option when they
		meant t or C won't destroy an archive (if no files were
		specified). */
		putempty();

		if(nblock>1)	
			putempty();
		if ( Pflag )
			zeroendtbuf();
		flushtape();
	}
	if (linkerrok == 0)
		return;
	for (; ihead != NULL; ihead = ihead->nextp) {
		if (ihead->count == 0)
			continue;
		fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_MISSING_LINKS_TO), ihead->pathname);  
		fflush(stderr);
	}
}

int
endtape(void)
{

	if (!continflag && dblock.dbuf.name[0] == '\0')
		return YES;

	/*
	 * Assume constant expression in if causes dead code elimination.
	 */
	if(TBLOCK % sizeof(int)) {
		register char *cp;
		register char *endp;

		endp = &dblock.dummy[TBLOCK];
		for (cp = dblock.dummy; cp < endp; cp++)
			if (*cp != '\0')
				return NO;
	}
	else {
 		register int *ip;
 		register int *endp;

 		endp = (int *) &dblock.dummy[TBLOCK];
 		for (ip = (int *) dblock.dummy; ip < endp; ip++)
 			if (*ip != 0)
 				return NO;
	}
	return YES;
}

int
getdir(void)
{
	register struct stat64 *sp;
	register int continuing;
	int i, firstdir=1, isposix = 0, signedsum;
	static hadsignedsum = 0;
	char cbuf[sizeof(dblock.dbuf.chksum)];

	continuing = NO;
	for (;;) {
		int cnt;
		if ((cnt = readtape((char *) &dblock)) < 0) {
			fflush(stdout);
			if (!continflag) {
				fprintf(stderr,
			       CATGETS(catd,_MSG_TAR_TAR_TAPE_ERROR_READING));  
				done(2);
			}
			if (!continuing) {
				fprintf(stderr,
			       CATGETS(catd,_MSG_TAR_TAR_TAPE_ERROR_READING_CONT));  
				fflush(stderr);
				continuing = YES;
			} else {
				fprintf(stderr, ".");
				fflush(stderr);
			}
			continue;
		}
		if (cnt == 0)
			premature_eof();
		if (endtape() && !continuing)
			/*      if endtape and continuing, do NOT
			 *	return NO, since it is common to find
			 *	blocks of nulls in binary files on the
			 *	backup.  Keep going until we re-sync,
			 *	we get a read error, or reach EOT/EOF */
			return NO;
		sp = &stbuf;
		if ( strcmp(dblock.pdbuf.magic, TMAGIC) == 0 ) {
			isposix = 1;
			sp->st_mode = strtol(dblock.pdbuf.mode, 0, 8);
			sp->st_uid = finduid(&dblock.pdbuf);
			sp->st_gid = findgid(&dblock.pdbuf);

			sp->st_size = 0;	/* default val */
			switch ( dblock.pdbuf.typeflag ) {
			case '3':	/* CHR */
				sp->st_mode |= FCHR;
				sp->st_rdev = makedev(
					strtol(dblock.pdbuf.devmajor, 0, 8),
					strtol(dblock.pdbuf.devminor, 0, 8));
				break;
			case '4':	/* BLK */
				sp->st_mode |= FBLK;
				sp->st_rdev = makedev(
					strtol(dblock.pdbuf.devmajor, 0, 8),
					strtol(dblock.pdbuf.devminor, 0, 8));
				break;
			case '5':	/* DIR */
				sp->st_mode |= FDIR;
				break;
			case '6':	/* FIFO */
				sp->st_mode |= FIFO;
				sp->st_rdev = makedev(
					strtol(dblock.pdbuf.devmajor, 0, 8),
					strtol(dblock.pdbuf.devminor, 0, 8));
				break;
			default:
				sp->st_size = (long)strtoul(dblock.pdbuf.size, 0, 8);
				break;
			}
			sp->st_mtime = strtol(dblock.pdbuf.mtime, 0, 8);
			chksum = strtol(dblock.pdbuf.chksum, 0, 8);
		} else { /* not POSIX */
			isposix = 0;

                	sscanf(dblock.dbuf.mode, "%o", &i);
                	sp->st_mode = i;
                	sscanf(dblock.dbuf.uid, "%o", &i);
                	sp->st_uid = i;
                	sscanf(dblock.dbuf.gid, "%o", &i);
                	sp->st_gid = i;
                	sscanf(dblock.dbuf.size, "%llo", &(sp->st_size));
                	sscanf(dblock.dbuf.mtime, "%lo", &sp->st_mtime);
                	sscanf(dblock.dbuf.chksum, "%o", &chksum);
			sscanf(dblock.dbuf.rdev, "%o", &sp->st_rdev);
			switch ((int)(sp->st_mode & ~07777)) {
		  	case S_IFCHR:
		  	case S_IFBLK:
		  	case S_IFIFO:
				sp->st_size = 0;
				break;
		  	default:
				if (dblock.dbuf.name[
				    strlen(dblock.dbuf.name)] == '/')
					sp->st_size = 0;
			}

		} /* not POSIX */

		if(firstdir) /* preserve for byteswap chk */
			strncpy(cbuf, dblock.dbuf.chksum, sizeof(cbuf));
		if (chksum == (i=checksum())) {
			if (continuing) {
				fprintf(stderr, CATGETS(catd,_MSG_TAR_OK));  
				fflush(stderr);
			}
			break;
		}
		else {
			signedsum = signedchecksum();
			if(chksum == signedsum) {
				if(!hadsignedsum) {
					fprintf(stderr,
					 CATGETS(catd,_MSG_TAR_TAR_ARCHIVE_APPARENTLY));  
					hadsignedsum = 1;
				}
				break;
			}
		}

		if (!continflag) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_DIRECTORY_CHECKSUM11));  

			if(firstdir) {
				/* check to see if byte-swapped; if so print message,
			 	 * then fall through to normal message, in case of
			 	 * scripts that know about the error message. */
				swab(cbuf, dblock.dbuf.chksum, sizeof(cbuf));
				sscanf(dblock.dbuf.chksum, "%o", &chksum);
				if(chksum == i)
					fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_THIS_APPEARS_TO_ARCHIVE));  
				else if(chksum == signedsum)
					fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_THIS_APPEARS_TO));
			}
			done(2);
		} else {
			fflush(stdout);
			if (!continuing) {
				fprintf(stderr,
				CATGETS(catd,_MSG_TAR_TAR_DIRECTORY_CHECKSUM));  
				fflush(stderr);
				continuing = YES;
			} else {
				fprintf(stderr, ".");
				fflush(stderr);
			}
		}
		firstdir = 0;
	}
	{
		register char *cp;
		register char *lcp;

		if ( isposix )  {
			cp = getname(&dblock.pdbuf);
			lcp = getlink(&dblock.pdbuf);
		}
		else {
			cp = &dblock.dbuf.name[0];
			lcp = &dblock.dbuf.linkname[0];
		}


		if (Rflag > 2) {
			/* strip directory from filename:
				first, find end of directory specification
				if found, change directory to ./
			*/
			
			char *op;
			char *np;
			np = op = cp;
			while (*op) {
				if (*op == '/')
				np = op + 1;
				op++;
			}
			if (np != cp) {
				op = cp;
				*op++ = '.';
				*op++ = '/';
				strcpy(op,np);
			}
			
			np = op = lcp;
			while (*op) {
				if (*op == '/')
					np = op + 1;
				op++;
			}
			if (np != lcp) {
				op = lcp;
				*op++ = '.';
				*op++ = '/';
				strcpy(op,np);
			}
		}
		else for ( ;; ) {
			if (cp[0] == '.' && cp[1] == '/') {
				if (cp[2] == '\0')
					break;
				cp += 2;
			} else if (Rflag && cp[0] == '/') {
				if (cp[1] == '\0')
					break;
				cp++;
			} else
				break;
		}
		dblockname = cp;
		dblocksymname = lcp;

		/* Rflag applies even if archive doesn't have full pathnames.
		 * this will be a nop for hardlinks (because of the way tar works
		 * they will be relative also), but *DOES* have an effect for
		 * symlinks.  Since this requires doubling R, and that's a new
		 * feature, I think this is worth keeping, even on relative
		 * archives.  */
		if(Rflag) /* if R given, need to make links relative also */
			while(lcp[0] == '/' && lcp[1])
				lcp++;
		dblocklnname = lcp;
		if(Rflag>1)	/* symlinks also? */
			dblocksymname = lcp;
	}
	if (tfile != NULL)
		fprintf(tfile, "%s %s\n", dblockname, dblock.dbuf.mtime);
	return YES;
}

void
passtape(void)
{
	off64_t blocks;
	char buf[TBLOCK];

	switch ( dblock.pdbuf.typeflag ) {		/* POSIX */
	case '1': case '2': case '3': case '4': case '5': case '6':
		return;
	}
	
	if (stbuf.st_size < 0)
		blocks = -stbuf.st_size*BIGBLOCK;
	else
		blocks = stbuf.st_size;
	blocks += TBLOCK-1;
	blocks /= TBLOCK;

	while (blocks-- > 0) {
		int cnt;
		if ((cnt = readtape(buf)) < 0) {
			fflush(stdout);
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_TAPE_READ_ERROR1));  
			if (continflag)
				break;
			done(2);
		}
		if (cnt == 0)
			premature_eof();
	}
}

void
putfile(char *longname, char *shortname, char *parent)
{
	int infile = 0;
	off_t leftover;
	int bigflag=0;
	off64_t blocks;
	char buf[TBLOCK];
	register char *cp;
	dirent64_t *dp;
	DIR *dirp;
	int i;
	int j;
	char newparent[NAMSPACE+PREFIXSPACE+64];    /* +PREFIXSPACE POSIX */
	extern int errno;
	int special;
	unsigned int dirents;	/* number of entries in directory */
	char **dirnms;
	struct statvfs vfs;
	extern int atstrcmp();
#ifdef  TRUSTEDIRIX
	mac_t flabel;		/* label of the file */
        char *flabelstr;        /* string representation of label */
	acl_t acl;		/* acl of the file */
	char *aclstr = NULL;	/* string representation of acl */
#endif  /* TRUSTEDIRIX */

	/* strip off trailing slashes */
	for (cp = &longname[strlen(longname)]; *--cp == '/'; *cp = '\0')
		if (cp == longname)
			break;
	/* strip leading ./ or extra leading slashes */
	for (;;)
		if (longname[0] == '.' && longname[1] == '/') {
			longname += 2;
			while (*longname == '/')
				longname++;
		} else if (longname[0] == '/' && longname[1] == '/')
			longname++;
		else
			break;

	if ( Pflag && strlen(longname) > maxnamlen ) {
		fprintf(stderr,
			CATGETS(catd,_MSG_TAR_TAR1_NAME_TOO_LONG),  
			strlen(longname),maxnamlen,longname);
		return;
	}

#ifdef	S_IFLNK
	if (!Lflag)

		i = lstat64(shortname, &stbuf);
	else
#endif	/* S_IFLNK */
		i = stat64(shortname, &stbuf);

	if (i < 0) {
		switch (errno) {
		case EACCES:
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_CANNOT_ACCESS),  
			  longname);
			break;
		case ENOENT:
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_NO_SUCH),  
			    longname);
			break;
		default:
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_CANNOT_STAT_FILE),  
			  longname);
			break;
		}
		fflush(stderr);
		return;
	}

	if(Eflag && !S_ISLNK(stbuf.st_mode) && statvfs(shortname, &vfs)==0 && 
		!(vfs.f_flag&ST_LOCAL)) {
		if(vflag > 1)
			fprintf(stderr, CATGETS(catd,_MSG_TAR_FORMAT_S_SKIPPED_NOT), longname);  
		return;
	}

	/*
	 * Added for xfs
	 */
	if ( stbuf.st_size > LONG_MAX && !Kflag ){
		fprintf(stderr,CATGETS(catd,_MSG_TAR_K_OPTION_NOT_SPECIFIED),  
			longname);
		fflush(stderr);
		return;
	}
	if ( stbuf.st_size > LONG_MAX && Kflag ){
		fprintf(stderr,CATGETS(catd,_MSG_TAR_WARNING_INCLUSION_OF),longname);  
		fflush(stderr);
	}

	if (tfile != NULL && checkupdate(longname) == 0)
		return;
	if (checkw('r', longname) == 0)
		return;
	if (Fflag && checkf(shortname, stbuf.st_mode, Fflag) == 0)
		return;
#ifdef	TRUSTEDIRIX
	if (Mflag > 0) {
		FILE *fp;		/* descriptor for phantom file */
		struct stat64 sbuf;	/* to check on stat file */

		/* 
		 * Skip directories named "attribute" owned by root.
		 */
		if ( (strcmp(shortname, ATTRFILE) == 0) &&
		    S_ISDIR(stbuf.st_mode) && (stbuf.st_uid == 0) )
		    return;
		/*
		 * In truth, getlabel should never fail unless the preceeding
		 * stat() failed.
		 */
	 	if ((flabel = mac_get_file(shortname)) == NULL) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_CANNOT_GET_FILE_LABEL), longname);
			fflush(stderr);
			return;
		}
	 	if ((flabelstr = mac_to_text(flabel, (size_t *) NULL)) == NULL) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_CANNOT_GET_LABEL_NAME), longname);
			fflush(stderr);
			mac_free(flabel);
			return;
		}
		
                mac_free(flabel);
                aclstr = NULL;

                if ((acl = acl_get_file(shortname,  ACL_TYPE_ACCESS)) != (acl_t) NULL) {
                        if (acl->acl_cnt > NACLBASE) {
                                if ((aclstr = acl_to_text(acl, (size_t *) NULL)) == (char *) NULL) {
                                	fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_CANNOT_GET_ACL), longname);
                                        fflush(stderr);
                                        acl_free(acl);
                                        mac_free(flabelstr);
                                        return;
                                }
                        }
                        acl_free(acl);
                }

		if (stat64(tirix_file, &sbuf) >= 0) {
			fprintf(stderr,
			    CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_LABEL), tirix_file);  
			fflush(stderr);
			mac_free(flabelstr);
			if (aclstr != NULL) acl_free(aclstr);
			return;
		}
		if ((fp = fopen(tirix_file, "w")) == NULL) {
			fprintf(stderr,
			    CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_CANNOT_CREATE_LABEL), tirix_file);
			fflush(stderr);
			mac_free(flabelstr);
			if (aclstr != NULL) acl_free(aclstr);
			return;
		}
		fchmod(fp, 0600);

		fprintf(fp, "mac=%s\n", flabelstr);
		mac_free(flabelstr);

		if (aclstr != NULL) {
			fprintf(fp, "acl=%s\n", aclstr);
			acl_free(aclstr);
		}
		fclose(fp);
		/*
		 * Get clever here.
		 * Turn off Mflag, then call putfile recursivly to
		 * dump this on the tape. Save stbuf as this function is not
		 * (Yetch!) reentrant.
		 */
		sbuf = stbuf;
		Mflag = 0;

		putfile(tirix_file, tirix_file, "/");

		unlink(tirix_file);
		stbuf = sbuf;
		Mflag = 1;
	}
#endif	/* TRUSTEDIRIX */

	switch ((int)(stbuf.st_mode & S_IFMT)) {

	case S_IFCHR:
		special = FCHR;
		break;
	case S_IFBLK:
		special = FBLK;
		break;
#ifdef	S_IFIFO
	case S_IFIFO:
		special = FIFO;
		break;
#endif /* S_IFIFO */
	default:
		special = 0;
	}

	if ( Pflag )
		bzero((char *)&dblock, TBLOCK);
	switch ((int)(stbuf.st_mode & S_IFMT)) {
	case S_IFDIR:
		for (i = 0, cp = buf; *cp++ = longname[i++];)
			continue;
		/* add a trailing '/' if it doesn't already have one */
		if (cp[-2] != '/') {
			cp[-1] = '/';
			cp[0] = '\0';
		} else
			--cp;
		if (!dflag) {
			if ((cp - buf) > maxnamlen) {
			    fprintf(stderr,
				CATGETS(catd,_MSG_TAR_TAR2_FORMAT_S_FILE),  
				    buf,(cp - buf),maxnamlen);
			    fflush(stderr);
			    return;
			}
			stbuf.st_size = 0;
			tomodes(&stbuf);
			if ( Pflag ) {
				if ( putname(&dblock.pdbuf, buf) ) {
			    		fprintf(stderr,
						CATGETS(catd,_MSG_TAR_TAR3_FORMAT_S_FILE),  
				    		buf,strlen(buf),maxnamlen);
					fflush(stderr);
					return;
				}
				dblock.pdbuf.typeflag = '5';
				sprintf(dblock.dbuf.chksum, "%06o", checksum());
			} else {
				strncpy(dblock.dbuf.name, buf, sizeof(dblock.dbuf.name));
				sprintf(dblock.dbuf.chksum, "%6o", checksum());
			}

			writetape((char *)&dblock);
		}
		sprintf(newparent, "%s/%s", parent, shortname);
		if (chdir(shortname) < 0) {
			fprintf(stderr,CATGETS(catd,_MSG_TAR_FORMAT_S_FORMAT_S_1),shortname,strerror(errno));  
			return;
		}
		if ((dirp = opendir(".")) == NULL) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_DIRECTORY),  
			    longname);
			fflush(stderr);
			if (chdir(parent) < 0) {
				fprintf(stderr, CATGETS(catd,_MSG_TAR_CANNOT_CHANGE_BACK));  
				fprintf(stderr,CATGETS(catd,_MSG_TAR_FORMAT_S_FORMAT_S_1),parent,  
					strerror(errno));
				fflush(stderr);
			}
			return;
		}
		dirents = 0;
		while ((dp = readdir64(dirp)) != NULL) {
			if (dp->d_ino == 0)
				continue;
			if (!strcmp(".", dp->d_name) ||
			  !strcmp("..", dp->d_name))
				continue;
			dirents++;
		}
		if (!dirents)
			goto cleanup;
		closedir(dirp);
		dirents += DFUDGE;
		dirnms = (char **) calloc(dirents, sizeof (char *));
		if (dirnms == NULL) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_OUT_OF_MEMORY));  
			fflush(stderr);
			goto cleanup;
		}
		if ((dirp = opendir(".")) == NULL) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_DIRECTORY),  
			  longname);
			fflush(stderr);
			return;
		}
		j = 0;
		while ((dp = readdir64(dirp)) != NULL && !term) {
			if (dp->d_ino == 0)
				continue;
			if (!strcmp(".", dp->d_name) ||
			    !strcmp("..", dp->d_name))
				continue;
			if (j >= dirents) {
				fprintf(stderr,CATGETS(catd,_MSG_TAR_TAR_DIRECTORY_CHANGED));  
				fflush(stderr);
				break;
			}
			dirnms[j] = malloc(strlen(dp->d_name)+1);
			if (dirnms[j] == NULL) {
				fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_OUT_OF_MEMORY));  
				fflush(stderr);
				goto cleanup;
			}
			strcpy(dirnms[j++], dp->d_name);
		}
		if (j != dirents-DFUDGE) {
			fprintf(stderr,CATGETS(catd,_MSG_TAR_TAR_DIRECTORY_CHANGED));  
			fflush(stderr);
		}
		dirents = j;
		qsort((char *) dirnms, dirents, sizeof (*dirnms), atstrcmp);
		for (j=0; j<dirents; j++) {
			strcpy(cp, dirnms[j]);
			putfile(buf, cp, newparent);
			free(dirnms[j]);
		}
		free(dirnms);

cleanup:
		closedir(dirp);
		if (chdir(parent) < 0) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_CANNOT_CHANGE_BACK));  
			fprintf(stderr,CATGETS(catd,_MSG_TAR_FORMAT_S_FORMAT_S_1),parent,strerror(errno));  
			fflush(stderr);
		}
		break;
#ifdef	S_IFLNK

	case S_IFLNK:
		tomodes(&stbuf);
		if ( Pflag ) {
			if ( putname(&dblock.pdbuf, longname) ) {
				fprintf(stderr,
					CATGETS(catd,_MSG_TAR_TAR4_FORMAT_S_TOTAL),  
				longname,strlen(longname),maxnamlen,NAMSIZ);
				fflush(stderr);
				return;
			}
		} else {
			if(strlen(longname) > NAMSIZ) {
				fprintf(stderr,
					CATGETS(catd,_MSG_TAR_TAR5_FORMAT_S_FILE),  
					longname,strlen(longname),NAMSIZ);
				fflush(stderr);
				return;
			}
			strcpy(dblock.dbuf.name, longname);
		}
		if (stbuf.st_size > NAMSIZ) {	/* st_size doesn't count NULL */
			fprintf(stderr,
    				CATGETS(catd,_MSG_TAR_TAR6_FORMAT_S_DESTINATION),  
			    longname,(int)stbuf.st_size,NAMSIZ);
			fflush(stderr);
			return;
		}
		i = readlink(shortname, dblock.dbuf.linkname, NAMSIZ);
		if (i < 0) {
			fprintf(stderr,CATGETS(catd,_MSG_TAR_FORMAT_S_FORMAT_S_1),longname,strerror(errno));  
			return;
		}
		if ( Pflag ) {
			dblock.pdbuf.linkname[i] = '\0';
			dblock.pdbuf.typeflag = '2';
		} else {
			dblock.dbuf.linkname[i] = '\0';
			dblock.dbuf.linkflag = '2';
		}
		if (vflag) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_A_FORMAT_S), longname);  
			fprintf(stderr, CATGETS(catd,_MSG_TAR_SYMBOLIC_LINK_TO_FORMAT_1),  
			    dblock.dbuf.linkname);
			fflush(stderr);
		}

		if ( Pflag ) {
			sprintf(dblock.pdbuf.size, "%011o", 0);
			sprintf(dblock.pdbuf.chksum, "%06o", checksum());
		} else {
			sprintf(dblock.dbuf.size, "%11lo", 0);
			sprintf(dblock.dbuf.chksum, "%6o", checksum());
		}
		writetape((char *)&dblock);
		break;
#endif	/* S_IFLNK */

	case S_IFCHR:
	case S_IFBLK:
#ifdef	S_IFIFO
	case S_IFIFO:
#endif /* S_IFIFO */
		if (Dflag) {
			fprintf(stderr,
			  CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_IS_NOT_REGULAR),  
			  longname);
			fflush(stderr);
			break;
		}
		stbuf.st_size = 0;
		
		/* Fall Through */

	case S_IFREG:
		if (!special && (infile = open(shortname, 0)) < 0) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_CANNOT_OPEN_FILE),  
			  longname);
			fflush(stderr);
			return;
		}
		tomodes(&stbuf);
		if ( Pflag ) {
			if ( !special )
				dblock.pdbuf.typeflag = '0';
			if ( putname(&dblock.pdbuf, longname) ) {
				fprintf(stderr,
	CATGETS(catd,_MSG_TAR_TAR7_FORMAT_S_TOTAL),  
	    			    longname,strlen(longname),maxnamlen,NAMSIZ);
				fflush(stderr);
				if (!special)
					close(infile);
				return;
			}
		} else {
			if ( strlen(longname) > NAMSIZ ) {
				fprintf(stderr,
					CATGETS(catd,_MSG_TAR_TAR8_FORMAT_S_FILE),  
			    		longname,strlen(longname),NAMSIZ);
				fflush(stderr);
				if (!special)
					close(infile);
				return;
			}
			strcpy(dblock.dbuf.name, longname);
		}
		if (stbuf.st_nlink > 1) {
			struct linkbuf *lp;
			int found = 0;

			for (lp = ihead; lp != NULL; lp = lp->nextp)
				if (lp->inum == stbuf.st_ino &&
				    lp->devnum == stbuf.st_dev) {
					found++;
					break;
				}
			if (found) {
				if ( Pflag ) {
					if ( strlen(lp->pathname) > NAMSIZ ) {
						fprintf(stderr,
							CATGETS(catd,_MSG_TAR_TAR8_FORMAT_S_LINK),  
							longname, lp->pathname,
							strlen(lp->pathname),
							NAMSIZ);
						fflush(stderr);
						if (!special)
							close(infile);
						return;
					}
					strcpy(dblock.pdbuf.linkname,
							lp->pathname);
					dblock.pdbuf.typeflag = '1';
					sprintf(dblock.pdbuf.size,
							"%011o", 0);
					sprintf(dblock.pdbuf.chksum,
							"%06o", checksum());
				} else {
					strcpy(dblock.dbuf.linkname,
							lp->pathname);
					dblock.dbuf.linkflag = '1';
					sprintf(dblock.dbuf.chksum,
							"%6o", checksum());
				}
				writetape( (char *) &dblock);
				if (vflag) {
					fprintf(stderr, CATGETS(catd,_MSG_TAR_A_FORMAT_S), longname);  
					fprintf(stderr, CATGETS(catd,_MSG_TAR_LINK_TO_FORMAT_S),  
					    lp->pathname);
					fflush(stderr);
				}
				lp->count--;
				if (!special)
					close(infile);
				return;
			}
			lp = (struct linkbuf *)
			     malloc((unsigned)
				     (strlen(longname) + sizeof *lp));
			if (lp == NULL) {
				if (freemem) {
					fprintf(stderr,
						CATGETS(catd,_MSG_TAR_TAR_OUT_OF_MEMORY_LINK));  
					fflush(stderr);
					freemem = 0;
				}
			} else {
				lp->nextp = ihead;
				ihead = lp;
				lp->inum = stbuf.st_ino;
				lp->devnum = stbuf.st_dev;
				lp->count = (short)stbuf.st_nlink - 1;
				strcpy(lp->pathname, longname);
			}
		}
			/* size was set to 0 for specials */
		blocks = (stbuf.st_size + (TBLOCK-1)) / TBLOCK;
		if (vflag) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_A_FORMAT_S), longname);  
			switch (special) {
			  case S_IFCHR:
			  case S_IFBLK:
				fprintf(stderr,CATGETS(catd,_MSG_TAR_FORMAT_S_SPECIAL),  
				  special == S_IFCHR ? CATGETS(catd,_MSG_TAR_CHAR) : CATGETS(catd,_MSG_TAR_BLOCK),  
				  major((int) stbuf.st_rdev),
				  minor((int) stbuf.st_rdev));
				break;
			  case S_IFIFO:
				fprintf(stderr,CATGETS(catd,_MSG_TAR_PIPE_1));  
				break;
			  default:
				fprintf(stderr, CATGETS(catd,_MSG_TAR_FORMAT_LLD_BLOCK_FORMAT),  
				    blocks,blocks!=1?CATGETS(catd,_MSG_TAR_S):"");
			}
			fflush(stderr);
		}

		/* Added for xfs support */
		if ( stbuf.st_size >= BIGBLOCK ){
			sprintf(dblock.pdbuf.size, "%011o ", (long)(-stbuf.st_size/BIGBLOCK));
			sprintf(dblock.pdbuf.mtime, "%011o ", stbuf.st_mtime);
			leftover = stbuf.st_size%BIGBLOCK;
			blocks = ((stbuf.st_size - leftover)  + (TBLOCK-1)) / TBLOCK;
			bigflag++;
		}

		if ( Pflag )
			sprintf(dblock.pdbuf.chksum, "%06o", checksum());
		else
			sprintf(dblock.dbuf.chksum, "%6o", checksum());
		writetape((char *)&dblock);

				/*
				 * special files are 0 bytes long
				 */
		if (!special) {
			int err;
			struct utimbuf utimbuf;
			i=1;
			while (i > 0 && blocks > 0) {
				i = read(infile, buf, TBLOCK);
				writetape(buf);
				blocks--;
			}
			err = errno;
			if (blocks != 0 || i < 0 || (!bigflag && i == 0)) {
				if (i < 0)
				    fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_ERROR_READING_FORMAT),  
					longname, strerror(err));
				else
				    fprintf(stderr,
					CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_FILE_CHANGED),  
					longname);
				fflush(stderr);
				while (--blocks >=  0)
					putempty();
			}
			if ( bigflag-- ){
				sprintf(dblock.pdbuf.size, "%011o ", (int) leftover);
				sprintf(dblock.pdbuf.mtime, "%011o ", stbuf.st_mtime);
				sprintf(dblock.pdbuf.chksum,"%06o", checksum());
				writetape((char *)&dblock);
				blocks = (leftover + (TBLOCK-1))/TBLOCK;
				while ((i = read(infile, buf, TBLOCK)) > 0
				    && blocks > 0) {
					writetape(buf);
					blocks--;
				}
				err = errno;
				if (blocks != 0 || i != 0) {
					if (i < 0)
						fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_ERROR_READING_FORMAT),  
						    longname, strerror(err));
					else
						fprintf(stderr,
						    CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_FILE_CHANGED),  
						    longname);
					fflush(stderr);
					while (--blocks >=  0)
						putempty();
				}
			}
			close(infile);
			/* assumes order of members in stat struct */
			if (aflag) {
				utimbuf.actime = stbuf.st_atime;
				utimbuf.modtime = stbuf.st_mtime;
				utime(shortname, &utimbuf);
			}
		}
		break;

	default:
		fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_IS_NOT),  
		    longname);
		fflush(stderr);
		break;
	}
}


/* unlink a file, but first test to see if it is a directory if
 * we are the superuser.  Otherwise mess up dir trees...
 * Don't report errors, unless debug set; subsequent extracts fail
 * with the 'right' error message.
*/
void
cond_unlink(char *name)
{
	if(imroot) {
		struct stat64 dst;
		if(lstat64(name, &dst))
			return;	/* doesn't exist */
		if(S_ISDIR(dst.st_mode)) {
			if(debug)
			    fprintf(stderr,
				CATGETS(catd,_MSG_TAR_REQUEST_TO_UNLINK_DIR),  
				name);
			return;
		}
	}
	if(unlink(name) && debug)
		fprintf(stderr, CATGETS(catd,_MSG_TAR_CANT_UNLINK_FORMAT), name, strerror(errno));  
}


/* print error message on tape i/o error; if exval != 0, exit.
 * check to see if error caused by no tape present, to give a
 * more helpful error to the user.
*/
void
terror(int isread, int exval)
{
	struct mtget mtg;
	int saverrno = errno;
	fprintf(stderr,CATGETS(catd,_MSG_TAR_TAR_TAPE_FORMAT_S_ERROR), isread?CATGETS(catd,_MSG_TAR_READ):CATGETS(catd,_MSG_TAR_WRITE));  

	if(isatape && ioctl(mt, MTIOCGET, &mtg) == 0 && !(mtg.mt_dposn&MT_ONL))
		fprintf(stderr,CATGETS(catd,_MSG_TAR_NO_TAPE_IN_DRIVE));  
	else if(saverrno == EROFS)	/* so we don't get readonly filesystem */
		fprintf(stderr,CATGETS(catd,_MSG_TAR_WRITE_PROTECTED_TAPE));  
	else
		fprintf(stderr,CATGETS(catd,_MSG_TAR_FORMAT_S4), strerror(saverrno));  

	if(exval)
		done(exval);
}


void
doxtract(char *argv[])
{
	off64_t blocks, bytes, tempbytes, tempblocks;
	int Bigflag=0;
	char buf[TBLOCK];
	int ifile;
	int ofile;
	char cmpfile[200 + NAMSPACE];
	char *fcmpfile;
	char tmpfile[200 + NAMSPACE];
	int same;
	mode_t special;
	int typeflag, isposix = 0;
	int once = 1;
#ifdef TRUSTEDIRIX
	int attrset = 0;
#endif /* TRUSTEDIRIX */

	/* reading stdin for file list valid only during archiving */
	if (*argv && (!strcmp(*argv,"-")) && !minusok) {
		fprintf(stderr,
		CATGETS(catd,_MSG_TAR_TAR_READ_FILELIST));  
		usage();
	}

	if (lnflag) {
		strcpy(cmpfile, linkdir);
		fcmpfile = &cmpfile[strlen(cmpfile)];
		strcpy(fcmpfile, "/");
		fcmpfile++;
	}
	while (getdir()) {
		int res;
		if ((! *dblockname) && (Rflag > 2)) {
			/* skip directories if RRR */
			passtape();
			continue;
		}
#ifdef  TRUSTEDIRIX
                if (Mflag > 0) {
                        if ( (strncmp(dblockname, TIRIX_PREFIX,
                            strlen(TIRIX_PREFIX)) == 0) ||
                            (strncmp(dblockname, &(TIRIX_PREFIX[1]),
                            strlen(TIRIX_PREFIX) - 1) == 0) ) {
                                attr_from_tape();
                                continue;
                        }
                }
#endif  /* TRUSTEDIRIX */
		if (*argv && !gotit(argv, dblockname)) {
			if(vflag>1)
					fprintf(stderr, CATGETS(catd,_MSG_TAR_P_NOT_IN_LIST_SKIP), dblockname);  
			passtape();
			continue;
		}
		if (checkw('x', dblockname) == 0) {
			passtape();
			continue;
		}
		if (Pflag && once) {
			if (!strncmp(dblock.pdbuf.magic, "ustar", 5)) {
				if (geteuid() == (uid_t) 0) {
					pflag = 1;
				} else {
					Oumask = umask(0); 	/* get file creation mask */
					(void) umask(Oumask);
				}
				once = 0;
			} else {
				if (geteuid() == (uid_t) 0) {
					pflag = 1;
				}
				if (!pflag) {
					Oumask = umask(0); 	/* get file creation mask */
					(void) umask(Oumask);
				}
				once = 0;
			}
		}
		if (Nflag) {
			struct stat s;
			if(!stat(dblockname, &s)) {
				if(vflag)
					fprintf(stderr, CATGETS(catd,_MSG_TAR_N_SKIP_FORMAT_S), dblockname);  
				passtape();
				continue;
			}
		}
		if (Fflag) {
			char *s;

			if ((s = strrchr(dblockname, '/')) == 0)
				s = dblockname;
			else
				s++;
			if (checkf(s, stbuf.st_mode, Fflag) == 0) {
				passtape();
				continue;
			}
		}

		if ((res = checkdir(dblockname)) == -1) {
			fprintf(stderr,CATGETS(catd,_MSG_TAR_TAR_EXTRACT_FAILED),  
				dblockname);
			done(2);
		} else if (res)		/* it's a dir and made; next hdr */
			continue;

		if ( strcmp(dblock.pdbuf.magic, "ustar") == 0 )
			isposix = 1;
		if ( isposix )
			typeflag = dblock.pdbuf.typeflag;
		else
			typeflag = dblock.dbuf.linkflag;
		if (typeflag == '2') {
#ifndef S_IFLNK
			goto linkit;
#else	/* S_IFLNK */
			cond_unlink(dblockname);
			if (symlink(dblocksymname, dblockname)<0) {
				fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_SYMBOLIC),  
				    dblockname);
				fflush(stderr);
				continue;
			}
			if (vflag) {
#ifdef  TRUSTEDIRIX
                                if (Mflag > 0)
                                        fprintf(stderr,
                                            CATGETS(catd,_MSG_TAR_X_FORMAT_S_SYMBOLIC_1),  
                                            dblockname, dblocksymname,
					    g_lname);
                                else
#endif  /* TRUSTEDIRIX */
				fprintf(stderr, CATGETS(catd,_MSG_TAR_X_FORMAT_S_SYMBOLIC),  
				    dblockname, dblocksymname);
				fflush(stderr);
			}
			/* see below for chown explanation */
			if (Sflag || (!oflag && imroot))
				lchown(dblockname, stbuf.st_uid, stbuf.st_gid);
#ifdef  TRUSTEDIRIX
                        if (Mflag > 0) {
				attrset = 0;
                                if (g_lp != NULL) {
		 	 		if (mac_set_file(dblockname, g_lp) == -1)
                                                fprintf(stderr,
                                        	  "tar: cannot set %s to label \"%s\"\n",
						  dblockname, g_lname);
                                        /*
                                         * Having used the label once it
                                         * must be discarded so as not to be
                                         * incorrectly used again.
                                         */
					attrset = 1;
                                        clear_glabel();
                                } 
				if (g_aclp != NULL) {
		  			if (acl_set_file(dblockname,
					  ACL_TYPE_ACCESS, g_aclp) == -1) {
						fprintf(stderr, 
						  "tar: cannot set %s to acl %s\n", 
						  dblockname, g_aclstr);
					}
					attrset = 1;
					clear_gacl();
				}
				if (attrset == 0) {
                                	fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_NO_LABEL_FOR_FORMAT),  
                                    	dblockname);
                        	}
                        }
#endif  /* TRUSTEDIRIX */
			continue;
#endif	/* S_IFLNK */
		} else if (typeflag == '1') { /* XXX POSIX */
#ifndef S_IFLNK
linkit:
#endif	/* S_IFLNK */
			cond_unlink(dblockname);
			if (link(dblocklnname, dblockname) < 0) {
				fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_CANNOT_LINK_1),  
				    dblocklnname, dblockname);
				fflush(stderr);
				continue;
			}
			if (vflag) {
				fprintf(stderr, CATGETS(catd,_MSG_TAR_FORMAT_S_FORMAT_S_TO),  
				  dblockname,
				  typeflag == '2'
				    ? CATGETS(catd,_MSG_TAR_SYMLINK_CHANGED_TO)  
				    : CATGETS(catd,_MSG_TAR_LINKED),  
				  dblocklnname);
				fflush(stderr);
			}
			continue;
		}
		ifile = -1;
		same = YES;
		if (lnflag) {
			static char tmpf[] = "TarXXXXXX";
			strcpy(fcmpfile, dblockname);
			if ((ifile = open(cmpfile, 0)) >= 0) {
			    tmpfile[0] = '\0';
			    strncat(tmpfile, dblockname,
				     dirpart(dblockname));
			    strcat(tmpfile, tmpf);
			    mktemp(tmpfile);
			}
		}
		if (ifile >= 0) {
			ofile = creat(tmpfile, (int) stbuf.st_mode & 0xfff);
		} else {
			if (Uflag)
				cond_unlink(dblockname);
			switch ((int)(stbuf.st_mode & S_IFMT)) {
			  case S_IFCHR:
			  case S_IFBLK:
			  case S_IFIFO:
				special = stbuf.st_mode & S_IFMT;
				break;
			  default:
				special = 0;
			}
			if (special) {
				ofile = mknod(dblockname,
				  (int) (stbuf.st_mode & S_PERMMASK) | special,
				  stbuf.st_rdev);
			} else if (dblockname[strlen(dblockname)] != '/')
					ofile = creat(dblockname,
					    (int) stbuf.st_mode & S_PERMMASK);
		}
		if (ofile < 0) {
			fprintf(stderr,CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_CANNOT),  
				dblockname, special ? CATGETS(catd,_MSG_TAR_MKNOD) : CATGETS(catd,_MSG_TAR_CREATE),  
				strerror(errno));
			fflush(stderr);
			passtape();
			continue;
		}

		/*
		 * Added for xfs
		 */
		if (stbuf.st_size < 0){
			tempblocks = blocks = ((tempbytes = bytes = -stbuf.st_size*BIGBLOCK) + TBLOCK-1)/TBLOCK;
			Bigflag = 1;
		}
		else
			blocks = ((bytes = stbuf.st_size) + TBLOCK-1)/TBLOCK;
#ifndef IGNORE_SCCSID
		if (ifile >= 0) {
			struct stat64 scstbuf;

			if (fstat64(ifile, &scstbuf) < 0 || bytes != scstbuf.st_size)
				same = NO;
		}
#endif /* IGNORE_SCCSID */
		if (vflag && !Bigflag) {
			switch ((int)special) {
			  case 0:
				fprintf(stderr,
				    CATGETS(catd,_MSG_TAR_X_FORMAT_S_FORMAT_LLD),  
				    dblockname, bytes, blocks, blocks!=1?CATGETS(catd,_MSG_TAR_S):"");
				break;
			  case S_IFCHR:
				fprintf(stderr, CATGETS(catd,_MSG_TAR_X_FORMAT_S_CHAR_SPECIAL),  
				  dblockname,
				  major(stbuf.st_rdev),
				  minor(stbuf.st_rdev));
				  break;
			  case S_IFBLK:
				fprintf(stderr, CATGETS(catd,_MSG_TAR_X_FORMAT_S_CHAR_SPECIAL),  
				  dblockname,
				  major(stbuf.st_rdev),
				  minor(stbuf.st_rdev));
				  break;
#ifdef	S_IFIFO
			  case S_IFIFO:
				fprintf(stderr, CATGETS(catd,_MSG_TAR_X_FORMAT_S_PIPE), dblockname);  
				break;
#endif /* S_IFIFO */
			  default:
				fprintf(stderr,
					CATGETS(catd,_MSG_TAR_UNRECOGNIZED_CASE_FORMAT),  
					special);
			}
#ifdef  TRUSTEDIRIX
                        if (Mflag >0)
                                fprintf(stderr, CATGETS(catd,_MSG_TAR_T_FORMAT_S), g_lname ? g_lname : CATGETS(catd,_MSG_TAR_QUESTION));  
#endif  /* TRUSTEDIRIX */
			fflush(stderr);
		}
		while (blocks-- > 0) {
			int nw;

			if ((nw = readtape(buf)) < 0) {
				fflush(stdout);
				if (vflag)
					fprintf(stderr, CATGETS(catd,_MSG_TAR_NEWLINE));
				if (!continflag)
					terror(1, 2);
				fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_OMITTING_DATA_BLOCK));  
				fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_DISCARD_FILE_FORMAT),  
						dblockname);
				fflush(stderr);
				continue;
			}
			if (nw == 0) {
				if (vflag)
					fprintf(stderr, CATGETS(catd,_MSG_TAR_NEWLINE));
				premature_eof();
			}
			nw = MIN(bytes, TBLOCK);
			if (write(ofile, buf, nw) < nw) {
				if (vflag)
					fprintf(stderr, CATGETS(catd,_MSG_TAR_NEWLINE));
				fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_HELP),  
				    dblockname);
				done(2);
			}
#ifndef IGNORE_SCCSID
			if (ifile >= 0 && same)
				same = cmprd(ifile, buf, (long)nw);
#endif /* IGNORE_SCCSID */
			bytes -= TBLOCK;
		}

		/*
		 * Added for xfs
		 */
		if (Bigflag){
			Bigflag = 0;
			getdir();
			blocks = ((bytes = stbuf.st_size) + TBLOCK-1)/TBLOCK;
			tempbytes += bytes;
			tempblocks += blocks;
			while (blocks-- > 0) {
				int nw;

				if ((nw = readtape(buf)) < 0) {
					fflush(stdout);
					if (vflag)
						fprintf(stderr, CATGETS(catd,_MSG_TAR_NEWLINE));
					if (!continflag)
						terror(1, 2);
					fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_OMITTING_DATA_BLOCK));  
					fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_DISCARD_FILE_FORMAT),  
					    dblockname);
					fflush(stderr);
					continue;
				}
				if (nw == 0) {
					if (vflag)
						fprintf(stderr, CATGETS(catd,_MSG_TAR_NEWLINE));
					premature_eof();
				}
				nw = MIN(bytes, TBLOCK);
				if (write(ofile, buf, nw) < nw) {
					if (vflag)
						fprintf(stderr, CATGETS(catd,_MSG_TAR_NEWLINE));
					fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_HELP),  
					    dblockname);
					done(2);
				}
#ifndef IGNORE_SCCSID
				if (ifile >= 0 && same)
					same = cmprd(ifile, buf, (long)nw);
#endif /* IGNORE_SCCSID */
				bytes -= TBLOCK;
			}
			if (vflag) {
				fprintf(stderr,
				    CATGETS(catd,_MSG_TAR_X_FORMAT_S_FORMAT_LLD),  
				    dblockname, tempbytes, tempblocks, tempblocks!=1?CATGETS(catd,_MSG_TAR_S):"");
			}
		}

		if (!special)
			close(ofile);
		if (ifile >= 0) {
#ifdef IGNORE_SCCSID
			same = cmpsccsid(tmpfile, cmpfile);
#endif /* IGNORE_SCCSID */
			close(ifile);
			cond_unlink(dblockname);
			if (vflag) {
				fprintf(stderr, CATGETS(catd,_MSG_TAR_FORMAT_S2), same ? CATGETS(catd,_MSG_TAR_SAME) : CATGETS(catd,_MSG_TAR_NEW));  
				fflush(stderr);
			}
			if (link(same ? cmpfile : tmpfile, dblockname) < 0) {
				fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_CANNOT_LINK),  
				  dblockname);
				fflush(stderr);
				if (same && link(tmpfile, dblockname) < 0) {
					fprintf(stderr,
					  CATGETS(catd,_MSG_TAR_TAR_FORMAT_S_CANNOT_LINK),  
					  dblockname);
					fflush(stderr);
				}
			}
			unlink(tmpfile);
		} else if (vflag) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_NEWLINE));
			fflush(stderr);
		}
		if (ifile < 0 || !same) {
			if (mflag == 0) {
				time_t tv[2];

				tv[0] = time((time_t *)0);
				tv[1] = stbuf.st_mtime;
				utime(dblockname, (struct utimbuf *)tv);
			}
			if (pflag)	/* symlinks don't get down here?? */
				chmod(dblockname,(int)stbuf.st_mode&S_PERMMASK);
			/* POSIX tar chown behavior:
			 * Sflag acts exactly like 3.3* and prior--chown 
			 *	to tape owner.
			 * Else, do as SVR4 does:
			 * - if user specified oflag, no chown regardless.
			 * - if no oflag and not root, no chown regardless.
			 * - if no oflag and root: 
			 *   + if posix hdr, try stored uname (pdbuf.uname)
			 *   + else use old-style stored uid string (dbuf.uid)
			 *	: these last two cases are same in our tar
			 *	cuz getdir() set {u/g}id as per header type
			 */
			if (Sflag || (!oflag && imroot))
				chown(dblockname, stbuf.st_uid, stbuf.st_gid);
		}
#ifdef  TRUSTEDIRIX
                if (Mflag > 0) {
			attrset = 0;
                        if (g_lp != NULL) {
		 		if (mac_set_file(dblockname, g_lp) == -1)
                                        fprintf(stderr,
                                		CATGETS(catd,_MSG_TAR_TAR_CANNOT_SET_FORMAT_1),  
                                            dblockname, g_lname);
                                /*
                                 * Having used the label once it
                                 * must be discarded so as not to be
                                 * incorrectly used again.
                                 */
				attrset = 1;
                                clear_glabel();
                        } 
			if (g_aclp != NULL) {
		 		if (acl_set_file(dblockname, ACL_TYPE_ACCESS,
					  g_aclp) == -1) {
					fprintf(stderr,
					CATGETS(catd,_MSG_TAR_TAR_CANNOT_SET_FORMAT),  
					dblockname, g_aclstr);
				}
				attrset = 1;
				clear_gacl();
			}
			if (attrset == 0) {
                                fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_NO_LABEL_FOR_FORMAT),  
                                    dblockname);
                        }
                }
#endif  /* TRUSTEDIRIX */
	}
	endread();
}

/* having this a separate routine makes docompare cleaner */
void
cmpname(char type, int do_nl)
{
	printf(CATGETS(catd,_MSG_TAR_FORMAT_C), type);  
	if (vflag)
		longt(&stbuf,dblockname);
	printf(do_nl?CATGETS(catd,_MSG_TAR_FORMAT_S4):CATGETS(catd,_MSG_TAR_FORMAT_S1), dblockname);  
}


void
docompare(char *argv[])
{
	off64_t blocks, bytes, tempbytes;
	mode_t	smode;
	int	Bigflag=0,doagain=0;
	char buf[TBLOCK];
	char typinfo;
	int ifile;
	int same;
	int typeflag, isposix = 0;
	struct stat64 st;

	/* reading stdin for file list valid only during archiving */
	if (*argv && (!strcmp(*argv,"-")) && !minusok) {
		fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_READ_FILELIST));  
		usage();
	}

	while (getdir()) {
		if (*argv && !gotit(argv, dblockname)) {
			passtape();
			continue;
		}
		if ( strcmp(dblock.pdbuf.magic, "ustar") == 0 )
			isposix = 1;
		if ( isposix )
			typeflag = dblock.pdbuf.typeflag;
		else
			typeflag = dblock.dbuf.linkflag;

		/* check if it is a hard (1) or soft(2) link */
		if (typeflag == '1' || typeflag == '2') { 
			cmpname(typeflag=='1' ? 'L' : 'S', 0);
			printf(CATGETS(catd,_MSG_TAR_LINKED_TO_FORMAT_S1),  
				typeflag=='1' ? dblocklnname : dblocksymname);
			continue;
		}
		smode = stbuf.st_mode & S_IFMT;
		if(smode && smode != S_IFREG) {
			switch ((int)smode) {
			  case S_IFCHR:
				typinfo = 'C';
				break;
			  case S_IFBLK:
				typinfo = 'B';
				break;
			  case S_IFIFO:
				typinfo = 'P';
				break;
			  case S_IFDIR:
				typinfo = 'D';
				break;
			}
			cmpname(typinfo, 1);
			continue;
		}

		/* 
			IFMT bits not kept for old non-posix extension tar dirs... 
			The posix tar should use the above switch statement since
			stbuf.st_mode for posix tar should mark directories 
			(in getdir routine).
		*/
		if(dblockname[strlen(dblockname)-1] == '/') {
			cmpname('D', 1);
			continue;
		}

		if ((ifile = open(dblockname, 0)) < 0) {
			cmpname((stat64(dblockname, &st) >= 0) ? '?' : '>', 1);
			passtape();
			continue;
		}

		same = YES;

		/*
		 * Added for xfs
		 */
		if (stbuf.st_size < 0){
			blocks = ((tempbytes = bytes = -stbuf.st_size*BIGBLOCK) + TBLOCK-1)/TBLOCK;
			Bigflag = 1;
		}
		else
			blocks = ((bytes = stbuf.st_size) + TBLOCK-1)/TBLOCK;

		if (fstat64(ifile, &st) < 0 || (bytes != st.st_size && !Bigflag))
			same = NO;

		if (same){
			doagain = 1;
			while(doagain--){
				while (blocks-- > 0) {
					int nw;
					if ((nw = readtape(buf)) < 0) {
						fflush(stdout);
						if (!continflag)
							terror(1, 2);
						fprintf(stderr,
						    CATGETS(catd,_MSG_TAR_TAR_OMITTING_DATA_BLOCK));  
						fprintf(stderr,
						    CATGETS(catd,_MSG_TAR_TAR_DISCARD_FILE_FORMAT),  
						    dblockname);
						fflush(stderr);
						continue;
					}
					if (nw == 0)
						premature_eof();
					nw = MIN(bytes, TBLOCK);
					if (ifile >= 0 && same)
						same = cmprd(ifile, buf, (long)nw);
					bytes -= TBLOCK;
				}
				if (Bigflag){
					Bigflag = 0;
					getdir();
					if (same){
						blocks = ((bytes = stbuf.st_size) + TBLOCK-1)/TBLOCK;
						stbuf.st_size += tempbytes;
						doagain++;
					}
					else
						passtape();
				}
			}
		}


		else
			passtape();
		close(ifile);
		cmpname(same ? '=' : '!' , 1);
	}
	endread();
}

void
dotable(char *argv[])
{
	int typeflag, bigflag = 0;
	off64_t tmpsize,tmpsiz2;

	/* reading stdin for file list valid only during archiving */
	if (*argv && (!strcmp(*argv,"-")) && !minusok) {
		fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_READ_FILELIST));  
		usage();
	}

	while (getdir()) {
#ifdef  TRUSTEDIRIX
                if (Mflag > 0) {
                        if (strncmp(dblockname, TIRIX_PREFIX,
                            strlen(TIRIX_PREFIX)) == 0) {
                                attr_from_tape();
                                continue;
                        }
                        if (vflag)
                                printf(CATGETS(catd,_MSG_TAR_FORMAT_ST), g_lname ? g_lname : CATGETS(catd,_MSG_TAR_QUESTION));  
                }
#endif  /* TRUSTEDIRIX */
		if (*argv && !gotit(argv, dblockname)) {
			passtape();
			continue;
		}

		if (Nflag) {
			struct stat s;
			if(!stat(dblockname, &s)) {
				passtape();
				continue;
			}
		}

		/*
		 * Added for xfs
		 */
		if (stbuf.st_size < 0 ){
			tmpsize= -stbuf.st_size*BIGBLOCK;
			passtape();
			getdir();
			tmpsiz2 = stbuf.st_size;
			stbuf.st_size += tmpsize;
			bigflag++;
		}
		if (vflag)
			longt(&stbuf,dblockname);
		printf(CATGETS(catd,_MSG_TAR_FORMAT_S), dblockname);  
		if ( strcmp(dblock.pdbuf.magic, "ustar") == 0 )
			typeflag = dblock.pdbuf.typeflag;
		else
			typeflag = dblock.dbuf.linkflag;

		switch (typeflag) {
		  case '1':
			printf(CATGETS(catd,_MSG_TAR_LINKED_TO_FORMAT_S), dblocklnname);  
			break;
		  case '2':
			printf(CATGETS(catd,_MSG_TAR_SYMBOLIC_LINK_TO_FORMAT), dblocksymname);  
		}
		printf(CATGETS(catd,_MSG_TAR_NEWLINE));
		if (bigflag){
			stbuf.st_size = tmpsiz2;
			bigflag=0;
		}
		passtape();
	}
	endread();
}

void
putempty(void)
{
	static char zerobuf[TBLOCK];

	writetape(zerobuf);
}


void
longt(struct stat64 *st, char	*name)
{
	register char *cp;
	char info[32];
	pmode(st);
	printf(CATGETS(catd,_MSG_TAR_FORMAT_3D_FORMAT_2D), st->st_uid, st->st_gid);  
	switch ((int)(st->st_mode & S_IFMT)) {
	case 0:
		/* AIX tar puts the IFREG bit in their st_mode, bug #316112 */
	case S_IFREG:
		if (name[strlen(name)-1] != '/') {
			sprintf(info, CATGETS(catd,_MSG_TAR_FORMAT_13lld), st->st_size);  

			break;
		}
		/* fall through */
	case S_IFDIR:
		strcpy(info, CATGETS(catd,_MSG_TAR_DIR));
		break;
	case S_IFCHR:
		sprintf(info, CATGETS(catd,_MSG_TAR_IFCHR),  
			major(st->st_rdev), minor(st->st_rdev));
		break;
	case S_IFBLK:
		sprintf(info, CATGETS(catd,_MSG_TAR_IFBLK),  
			major(st->st_rdev), minor(st->st_rdev));
		break;
	case S_IFIFO:
		strcpy(info, CATGETS(catd,_MSG_TAR_PIPE));  
		break;
	default:
#ifdef DEBUG
		fprintf(stderr, CATGETS(catd,_MSG_TAR_DEBUG_UNRECOGNIZED));  
		fflush(stderr);
#endif /* DEBUG */
		strcpy(info, CATGETS(catd,_MSG_TAR_QUESTION));
		break;
	}
	cp = ctime(&st->st_mtime);
	printf(CATGETS(catd,_MSG_TAR_FORMAT_13S_FORMAT), info, cp+4, cp+20);  
}

#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000
int	m1[] = { 1, ROWN, 'r', '-' };
int	m2[] = { 1, WOWN, 'w', '-' };
int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int	m4[] = { 1, RGRP, 'r', '-' };
int	m5[] = { 1, WGRP, 'w', '-' };
int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int	m7[] = { 1, ROTH, 'r', '-' };
int	m8[] = { 1, WOTH, 'w', '-' };
int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

void
pmode(struct stat64 *st)
{
	register int **mp;

	for (mp = &m[0]; mp < &m[9];)
		choose(*mp++, st);
}

void
choose(int *pairp, struct stat64 *st)
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n>=0 && (st->st_mode&*ap++)==0)
		ap++;
	printf("%c", *ap);  
}

checkdir(char *name)
{
	register char *cp;
	extern int errno;

	if (dblock.pdbuf.typeflag != '5') {
		/*
		 * Quick check for existence of directory.
		 */
		if ((cp = strrchr(name, '/')) == 0)
			return (0);
		*cp = '\0';
	} else if ((cp = strrchr(name, '/')) == 0) {
		if (access(name, 0) < 0) {
			if(mkdir(name, 0777) < 0) {
				goto mkdir_error;
			}
			return(1);
		}
		return(0);
	}
	if (access(name, 0) >= 0) {
		*cp = '/';
		return (cp[1] == '\0');	/* return (lastchar  == '/') */
	}
	*cp = '/';

	/*
	 * No luck, try to make all directories in path.
	 * Assume "/" exists. Otherwise we will test "" which fails on
	 * System V, breaking tar.
	 */
	for (cp = name+1; *cp; cp++) {
		if (*cp != '/')
			continue;
		*cp = '\0';
		if (access(name, 0) < 0) {
			errno = 0;
			if (mkdir(name, 0777) < 0) {	/* umask controls */
mkdir_error:
				fflush(stdout);
				fprintf(stderr,CATGETS(catd,_MSG_TAR_TARCANT_MKDIR));  
				if (errno)
					fprintf(stderr,CATGETS(catd,_MSG_TAR_FORMAT_S_FORMAT_S_1),name,  
						strerror(errno));
				else
					fprintf(stderr,CATGETS(catd,_MSG_TAR_FORMAT_S4),name);  
				*cp = '/';
					/*
					 * Nonzero to suppress creation of
					 * zero-length file preventing
					 * subsequent mkdir by hand after
					 * fixing problem.
					 */
				return -1;
			}
				/* .../me/ */
			if (pflag && cp[1] == '\0')
				chmod(name, (int)stbuf.st_mode & S_PERMMASK);

			/* see doxtract() for chown explanation */
			if (Sflag || (!oflag && imroot))
				chown(name, stbuf.st_uid, stbuf.st_gid);
#ifdef  TRUSTEDIRIX
                        if (Mflag > 0 && (g_lp != NULL)) {
		 		if (mac_set_file(name, g_lp) == -1)
					fprintf(stderr,
					CATGETS(catd,_MSG_TAR_TAR_CANNOT_SET_FORMAT_1),
					  name, g_lname);
				clear_glabel();
                        } 
			if (Mflag > 0 && (g_aclp != NULL)) {
		 		if (acl_set_file(dblockname, ACL_TYPE_ACCESS,
					  g_aclp) == -1) {
					fprintf(stderr,
					CATGETS(catd,_MSG_TAR_TAR_CANNOT_SET_FORMAT),  
					dblockname, g_aclstr);
				}
				clear_gacl();
			}
#endif  /* TRUSTEDIRIX */
		}
		*cp = '/';
	}
	if (((dblock.pdbuf.typeflag == '5') || (cp[-1]=='/'))
	    && access(name, 0) < 0) {
		if (mkdir(name, 0777) < 0) {
			goto mkdir_error;
		}
		return(1);
	}
	return (cp[-1]=='/');	/* TRUE if only directory (*?/)		*/
}

void
onintr()
{
	signal(SIGINT, SIG_IGN);
	term++;
}

void
onquit()
{
	signal(SIGQUIT, SIG_IGN);
	term++;
}

void
onhup()
{
	signal(SIGHUP, SIG_IGN);
	term++;
}


void
tomodes(struct stat64 *sp)
{
    	static int uid_overflow = 0;
	register char *cp;
	int i;

	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		*cp = '\0';
	switch ((int)(sp->st_mode & S_IFMT)) {
	  case S_IFCHR:
		i = FCHR;
		if ( Pflag )
			dblock.pdbuf.typeflag = '3';
		break;
	  case S_IFBLK:
		i = FBLK;
		if ( Pflag )
			dblock.pdbuf.typeflag = '4';
		break;
#ifdef	S_IFIFO
	  case S_IFIFO:
		i = FIFO;
		if ( Pflag )
			dblock.pdbuf.typeflag = '6';
		break;
#endif /* S_IFIFO */
	  default:
		i = 0;
		break;
	}
	if (i)
		sp->st_size = 0;
	if ( Pflag ) {
		sprintf(dblock.pdbuf.devmajor, "%06o ",
		    i ? major(sp->st_rdev) : 0);
		sprintf(dblock.pdbuf.devminor, "%06o ",
		    i ? minor(sp->st_rdev) : 0);
		sprintf(dblock.pdbuf.mode,"%06o ",(sp->st_mode&S_PERMMASK) | i);
		/* N.B. We don't print a warning for uid overflow (as we do
		 * in pre-POSIX mode (below) because we're still writing the
		 * uname, so chances are we'll be okay at extraction time.
		 */
		sprintf(dblock.pdbuf.uid, "%06o ", 
			((sp->st_uid > 0777777) ? UID_NOBODY : sp->st_uid));
		sprintf(dblock.pdbuf.gid, "%06o ",
			((sp->st_gid > 0777777) ? GID_NOBODY : sp->st_gid));
		sprintf(dblock.pdbuf.size, "%011o ", (long)sp->st_size);
		sprintf(dblock.pdbuf.mtime, "%011o ", sp->st_mtime);
		sprintf(dblock.pdbuf.magic, "%s", TMAGIC);
		sprintf(dblock.pdbuf.version, "%2s", TVERSION);
		sprintf(dblock.pdbuf.uname, "%s", finduname(sp->st_uid));
		sprintf(dblock.pdbuf.gname, "%s", findgname(sp->st_gid));
		return;
	}
	sprintf(dblock.dbuf.mode, "%6o ", (sp->st_mode & S_PERMMASK) | i);
	sprintf(dblock.dbuf.uid, "%6o ", 
			((sp->st_uid > 0777777) ? UID_NOBODY : sp->st_uid));
	sprintf(dblock.dbuf.gid, "%6o ", 
			((sp->st_gid > 0777777) ? GID_NOBODY : sp->st_gid));
	if (!uid_overflow && (sp->st_uid > 0777777 || sp->st_gid > 0777777))
	{
	    	uid_overflow = 1;
		fprintf(stderr, 
			CATGETS(catd,_MSG_TAR_WARNING_FILES_HAVE));
	}
	  
	/* Each sprintf writes the NUL one past the buffer boundary,
	 * but that should be okay (even for rdev).
	 */
	sprintf(dblock.dbuf.size, "%11lo ", (unsigned long)sp->st_size);
	sprintf(dblock.dbuf.mtime, "%11lo ", sp->st_mtime);
	sprintf(dblock.dbuf.rdev, "%11o ", (unsigned int) sp->st_rdev);
}

int
checksum(void)
{
	register i;	/* still signed? */
	register unsigned char *cp;

	for (cp = (unsigned char *)dblock.dbuf.chksum;
	     cp < (unsigned char *)&dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)];
		 cp++)
		*cp = ' ';
	i = 0;
	for (cp = (unsigned char *)dblock.dummy;
		cp < (unsigned char *)&dblock.dummy[TBLOCK]; cp++)
		i += *cp;
	return (i);
}

/* check with signed chars.  POSIX calls for unsigned chars, but
 * some systems use signed, because that is the default for their
 * chars.  Can show up as a problem if filenames have high bit set
*/
int
signedchecksum(void)
{
	register i;
	register signed char *cp;

	for (cp = (signed char *)dblock.dbuf.chksum;
	     cp < (signed char *)&dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)];
		 cp++)
		*cp = ' ';
	i = 0;
	for (cp = (signed char *)dblock.dummy;
		cp < (signed char *)&dblock.dummy[TBLOCK]; cp++)
		i += *cp;
	return (i);
}

int
checkw(int c, char *name)
{
	if (!wflag)
		return (1);
	printf(CATGETS(catd,_MSG_TAR_FORMAT_C), c);  
	if (vflag)
		longt(&stbuf, name);
	printf(CATGETS(catd,_MSG_TAR_FORMAT_S3), name);  
	return (response() == 'y');
}

int
response(void)
{
	char c;
	int i;

	c = getchar();
	if (c != '\n')
		while ((i=getchar()) != '\n' && i != EOF)
			continue;
	else
		c = 'n';
	return (c);
}

int
checkf(char *name, mode_t mode, int howmuch)
{
	int l;

	if ((mode & S_IFMT) == S_IFDIR)
		return (strcmp(name, "SCCS") != 0);
	if ((l = strlen(name)) < 3)
		return (1);
	if (howmuch > 1 && name[l-2] == '.' && name[l-1] == 'o')
		return (0);
	if (strcmp(name, "core") == 0 ||
	    strcmp(name, "errs") == 0 ||
	    (howmuch > 1 && strcmp(name, "a.out") == 0))
		return (0);
	/* SHOULD CHECK IF IT IS EXECUTABLE */
	return (1);
}

int
checkupdate(arg)
	char *arg;
{
	char name[100];
	long mtime;
	off64_t seekp;
	off64_t lookup();

	rewind(tfile);
	if ((seekp = lookup(arg)) < 0)
		return (1);
	fseek64(tfile, seekp, 0);
	fscanf(tfile, "%s %lo", name, &mtime);
	return (stbuf.st_mtime > mtime);
}

void
done(int n)
{
	if (debug)
		fprintf(stderr,CATGETS(catd,_MSG_TAR_EXITING_FROM_DONE_N),n);  
	fflush(stderr);
	fflush(stdin);
#ifdef RMT
	rmtclose(mt);
#endif /* RMT */
	unlink(tname);
	exit(n);
}

gotit(char **list, char *name)
{
	for (; *list; ++list)
		if (prefix(*list, name))
			return YES;
	return NO;
}

prefix(char *s1, char *s2)
{
	while (*s1)
		if (*s1++ != *s2++)
			return (0);
	if (*s2)
		return (*s2 == '/');
	return (1);
}

#define	N	200
int	njab;

off64_t
lookup(char *s)
{
	register i;
	off64_t a;

	for (i=0; s[i]; i++)
		if (s[i] == ' ')
			break;
	a = bsrch(s, i, low, high);
	return (a);
}

off64_t
bsrch(char *s, int n, off64_t l, off64_t h)
{
	register i, j;
	char b[N];
	off64_t m, m1;

	njab = 0;

loop:
	if (l >= h)
		return (-1L);
	m = l + (h-l)/2 - N/2;
	if (m < l)
		m = l;
	fseek64(tfile, m, 0);
	fread(b, 1, N, tfile);
	njab++;
	for (i=0; i<N; i++) {
		if (b[i] == '\n')
			break;
		m++;
	}
	if (m >= h)
		return (-1L);
	m1 = m;
	j = i;
	for (i++; i<N; i++) {
		m1++;
		if (b[i] == '\n')
			break;
	}
	i = cmp(b+j, s, n);
	if (i < 0) {
		h = m;
		goto loop;
	}
	if (i > 0) {
		l = m1;
		goto loop;
	}
	return (m);
}

cmp(char *b, char *s, int n)
{
	register i;

	if (b[0] != '\n') {
		if (debug)
			fprintf(stderr,CATGETS(catd,_MSG_TAR_EXITING_FROM_CMP_B0),b[0]);  
		exit(2);
	}
	for (i=0; i<n; i++) {
		if (b[i+1] > s[i])
			return (-1);
		if (b[i+1] < s[i])
			return (1);
	}
	return (b[i+1] == ' '? 0 : -1);
}

void
endread(void)
{
	register int cnt;

	if (infrompipe) {
		while ((cnt = readtape(NULL)) > 0)
			;
		if (cnt == -1) {
			fflush(stdout);
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_PIPE_READ_ERROR));  
			fflush(stderr);
		}
	}
#ifndef	OLDMAGTAPE

	/* try to find end of file.  Give up after two tries */
	if ((cnt = readtape((char *) NULL)) != 0
	  && (cnt < 0 || readtape((char *) NULL) < 0)) {
		fflush(stdout);
		fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_TAPE_READ_ERROR));  
		fflush(stderr);
	}
#endif	/* OLDMAGTAPE */
}

void
premature_eof(void)
{
	fflush(stdout);
	fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_TAPE_PREMATURE));  
	done(2);
}

/* SGI: Variable blocking */
readtape(char *buffer)
{
	register int i;
	static int haderr;

	if (recno >= nblock || first == 0 || buffer == (char *) NULL) {
		if ((i = bread(mt, (char *) tbuf,
		  TBLOCK*(nblock?nblock:NBLOCK), buffer)) < 0) {
			fflush(stdout);
			terror(1, 0);
			fflush(stderr);
			if (continflag && !haderr) {
				haderr = 1;
				return -1;
			}
			done(3);
		}
		haderr = 0;
		if (i == 0)
			return 0;
		if (buffer == (char *) NULL)
			return i;
		if ((i % TBLOCK) != 0) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_TAPE_BLOCKSIZE));  
			done(3);
		}
		i /= TBLOCK;
		if (first == 0) {
			if (i != nblock && i != 1) {
				fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_BLOCKSIZE_FORMAT), i);  
				fflush(stderr);
				nblock = i;
			}
		}
		nblock = i;
		recno = 0;
	}
	first = 1;
	bcopy((char *)&tbuf[recno++], buffer, TBLOCK);
	return (TBLOCK);
}

void
writetape(char *buffer)
{
	first = 1;
	if (nblock == 0)
		nblock = 1;
	if (recno >= nblock) {
		if (bwrite(mt, (char *) tbuf,
		  TBLOCK*nblock, buffer) != TBLOCK*nblock)
			terror(0, 2);
		recno = 0;
	}
	bcopy(buffer, (char *)&tbuf[recno++], TBLOCK);
	if (recno >= nblock) {
		if (bwrite(mt, (char *) tbuf,
		  TBLOCK*nblock, buffer) != TBLOCK*nblock)
			terror(0, 2);
		recno = 0;
	}
}

/* if eot == NULL -> don't change tapes */
myio(int (*fn)(), int fd, char *ptr, int n, char *eot)
{
	int	i;
	int	tty;
	char	what;
	static int firsttape = 1;
#ifdef RMT
	int isread = (fn == rmtread);
#else
	int isread = (fn == read);
#endif /* RMT */

	i = (*fn)(fd, ptr, n);
	if (i != n && eot != NULL) {
		fflush(stdout);
		if (debug) {
			fprintf(stderr,CATGETS(catd,_MSG_TAR_MYIO_FORMAT_S_WANTED),  
			    isread ? CATGETS(catd,_MSG_TAR_READ) : CATGETS(catd,_MSG_TAR_WRITE), n, i);
			if (i < 0) {
				fprintf(stderr,CATGETS(catd,_MSG_TAR_TERRNO_IS_FORMAT_D),errno,  
					strerror(errno));
			}
		}
		fflush(stderr);
		if (!isatape)
			return i;	/* return short count if not a device */
		if ( i == -1 && errno != ENOSPC )
		{
			terror(isread, 0);
			if(firsttape)	 {
				/* if failed at start of first tape, or
				 * in 'middle' of any tape, then quit,
				 * else prompt for new tape.  I.e.,
				 * always give up unless we just had
				 * a tape change, with no OK i/o. */
				if (debug)
					fprintf(stderr,
					    CATGETS(catd,_MSG_TAR_EXITING_MYIO_FIRSTTAPE),  
					    firsttape);
				done(2);
			}
		}
		else if ( i > 0 ) {
			/* short read/write is OK */
			return(i);
		}
		else if(firsttape==1)	/* if failed at start of first tape with
			ENOSPC then don't prompt for new tape; return 0 to print
			premature EOF for reads; or perror msg for writes. */
			return isread ? 0 : -1;
		else if(!i) {
		    /* 9 track, and DAT multivolume; this is what cpio has
		     * always allowed, and tar and bru now do starting with
		     * IRIX 4.0, since filemarks are now always written at the
		     * end of a tape for 9 track, DAT, and any new tape devices.
		     * Writes from tpsc.c will return 0 on first write after
		     * EW/EOM is encountered, and therefore reads will
		     * return 0 when the FM is encountered. We talked about
		     * adding an mtstat check for EOT or EW, but that seems
		     * like overkill.  May still get -1 and ENOSPC on
			 * multivol writes due to deferred error reporting on
			 * SCSI tapes */
		    if (debug)
			    fprintf(stderr, CATGETS(catd,_MSG_TAR_IO_RETURNED_0_BYTES));  
		}
		/* else -1 and ENOSPC, which is 'normal' for QIC and 8mm
		 * tapes when doing multivolume the older SGI way. */
			
		if ((tty = open("/dev/tty", 2)) < 0 && !isatty(tty = 0)) {
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_CANNOT_PROMPT_FOR));  
			done(2);
		}
#ifdef RMT
		rmtclose(mt);
#else
		close(mt);
#endif /* RMT */
		for (;;) {
			/* !*!*!*! NOTE NOTE NOTE NOTE NOTE NOTE !*!*!*!
			 * This prompt is known to the GUI backup and restore
			 * tool.  If it is changed or i18n'ed, the GUI tool 
			 * must be changed to match
			 * (usrenv/sysadm/lib/libbackuprestore/
			 * is where the source lives as of 7/96)
			*/
			static char prompt[] =
			    "\007Change tape and press the Enter key:";

			/* Prompt the user to replace the tape. */
			fflush(stdout);
			write(tty, prompt, sizeof(prompt));
			do {
				errno = 0;
				if (read(tty, &what, 1) != 1) {
					if (errno > 0)
						fprintf(stderr,
						    CATGETS(catd,_MSG_TAR_TAR_NO_NEW_TAPE_FORMAT),  
						    strerror(errno));
					else {
						fprintf(stderr,
						    CATGETS(catd,_MSG_TAR_TAR_NO_NEW_TAPE));  
					}
					fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_TAPE_FORMAT_S_ERROR),  
					    isread ? CATGETS(catd,_MSG_TAR_READ) : CATGETS(catd,_MSG_TAR_WRITE));
					done(2);
				}
				if (debug > 1) {
					fprintf(stderr,CATGETS(catd,_MSG_TAR_READ_FORMAT_C), what);  
					fflush(stderr);
				}
			} while (what != '\n');

			/* Try to open the device with the new tape in it. */
			if (!strcmp(usefile,"-"))
				mt = dup(isread ? 0 : 1);
			else {
#ifdef RMT
				if (isread) {
					if (work == dorep)
						mt = rmtopen(usefile,2);
					else
						mt = rmtopen(usefile,0);
				}
				else {
					mt = rmtopen(usefile,1);
					if (mt < 0)
						mt = rmtcreat(usefile,0666);
				}
#else
				if (isread)
					mt = open(usefile,0);
				else {
					mt = open(usefile,1);
					if (mt < 0)
						mt = creat(usefile,0666);
				}
#endif /* RMT */
			}
			if (mt < 0) {
				fprintf(stderr,CATGETS(catd,_MSG_TAR_TAR_CANT_FORMAT_S),  
				  isread ? CATGETS(catd,_MSG_TAR_OPEN) : CATGETS(catd,_MSG_TAR_CREAT));  
				fprintf(stderr,CATGETS(catd,_MSG_TAR_FORMAT_S_FORMAT_S_1),usefile,  
					strerror(errno));
				fflush(stderr);
			} else {
				chkandfixaudio(mt);
				firsttape = 0;	/* we are at start of a tape */
				if((i = myio(fn, mt, ptr, n, eot)) > 0)
					break;
				/* else very first i/o fails; write protected
				 * when writing, or some similar error; reprompt,
				 * rather than giving up in middle of multivol
				 * backup set */
				if(debug) 
					fprintf(stderr, CATGETS(catd,_MSG_TAR_FIRST_FORMAT_S_ON_NEW),  
						isread?CATGETS(catd,_MSG_TAR_READ):CATGETS(catd,_MSG_TAR_WRITE));
			}
		}
		if (tty > 0)
			(void) close(tty);
	}
	if(i == n)
		firsttape = -1;	/* we're in 'middle' of a tape */
	return i;
}

void
backtape(void)
{
	static int mtdev = 1;
	static struct mtop mtop1 = {MTBSR, 1};
	struct mtget mtget;

	if (mtdev == 1)	/* verify that it's a tape */
		mtdev = ioctl(mt, MTIOCGET, &mtget);
	if (mtdev == 0) {
		/* a change was made at one point to use MTAFILE first, so
		 * we got an error if drive didn't support overwrite. The 
		 * problem with that idea is that it can *never* work reliably,
		 * since the two putempty() records could be split over a block
		 * boundary, or not (except at nblock==1, and then it works reliably
		 * if you did bsr 2.  Better to simply rely on getting an error on
		 * the next write, if the drive doesn't support overrite. */
		if (ioctl(mt, MTIOCTOP, &mtop1) < 0)
			goto backerr;
	}
	else
#ifdef RMT
		rmtlseek(mt, (off_t) -TBLOCK*nblock, 1);
#else
		lseek(mt, (off_t) -TBLOCK*nblock, 1);
#endif /* RMT */
	recno--;
	return;
backerr:
	fprintf(stderr,CATGETS(catd,_MSG_TAR_TAR_CAN_NOT_APPEND));  
	done(4);
}

/*
 * zero the remaining blocks in the tbuf
 * keeps POSIX happy
 */
void
zeroendtbuf(void)
{
	while ( recno < nblock )
		bzero(&tbuf[recno++], TBLOCK);
}

/* SGI: Variable blocking */
void
flushtape(void)
{
	int amt;
	char oink[3];

	amt = TBLOCK*(Vflag?recno:nblock);

	if (recno || !Vflag)
	{
#ifdef RMT
		if (myio(rmtwrite, mt, (char *) tbuf,
#else
		if (myio(write, mt, (char *) tbuf,
#endif /* RMT */
		  amt, oink) != amt) {
			terror(0, 2);
		}
	}
}

/* if eot == NULL -> don't switch tapes on error */
bread(int fd, char *buf, int size, char *eot)
{
	int count;
	static int lastread = 0;

	if (!Bflag && !isatape)
#ifdef RMT
		return (myio(rmtread, fd, buf, size, eot));
#else
		return (myio(read, fd, buf, size, eot));
#endif /* RMT */
	for (count = 0; count < size; count += lastread) {
		if (lastread < 0) {
			if (count > 0)
				return (count);
			return (lastread);
		}
#ifdef RMT
		lastread = myio(rmtread, fd, buf, size - count, eot);
#else
		lastread = myio(read, fd, buf, size - count, eot);
#endif /* RMT */
		if (debug)
			if (debug > 1 || lastread <= 0)
				fprintf(stderr,
			    	  CATGETS(catd,_MSG_TAR_BREADFD_FORMAT_DBUF),  
				  fd, buf, size - count, eot, lastread);
		if (lastread <= 0)
			if (lastread < 0)
				return lastread;
			else
				return count;
		buf += lastread;
	}
	return (count);
}

/* if eot == NULL -> don't switch tapes on error */
bwrite(int fd, char *buf, int size, char *eot)
{
	int count;
	static int lastwrite = 0;

	if (!isatape)
#ifdef RMT
		return (myio(rmtwrite, fd, buf, size, eot));
#else
		return (myio(write, fd, buf, size, eot));
#endif /* RMT */
	for (count = 0; count < size; count += lastwrite) {
#ifdef RMT
		lastwrite = myio(rmtwrite, fd, buf, size - count, eot);
#else
		lastwrite = myio(write, fd, buf, size - count, eot);
#endif /* RMT */
		if (debug)
			if (debug > 1 || lastwrite <= 0)
				fprintf(stderr,
			   	  CATGETS(catd,_MSG_TAR_BWRITEFD_FORMAT_DBUF),  
				  fd, buf, size - count, eot, lastwrite);
		if (lastwrite <= 0)
			if (lastwrite < 0)
				return lastwrite;
			else
				return count;
		buf += lastwrite;
	}
	return (count);
}


/*  Compare the next 'nw' characters of ifile with buf.
 *   return 1 if same, else 0
 */
cmprd(int ifile, char *buf, long num)
{
	register int nr;
	char ibuf[BUFSIZ];

	for (; (nr = MIN(num, BUFSIZ)) > 0; num -= nr) {
		if (read(ifile, ibuf, nr) < nr)
			return NO;
		if (bufcmp(buf, ibuf, nr))
			return NO;
	}
	return YES;
}


bufcmp(char *cp1, char *cp2, int num)
{
	if (num <= 0)
		return 0;
	do
		if (*cp1++ != *cp2++)
			return *--cp2 - *--cp1;
	while (--num);
	return 0;
}

dirpart(char *str)
{
	register char *cp;

	if (cp = strrchr(str, '/'))
		return cp - str + 1;
	else
		return 0;
}

#ifdef IGNORE_SCCSID

cmpsccsid(char *f1, char *f2)
{
	char buf[1000];
	static char tmplate[] = "/var/tmp/cmpXXXXXX";
	static char tmpbuf[sizeof tmplate];
	static char *tmpfile;
	int retval;

	if (!tmpfile) {
		tmpfile = tmpbuf;
		strcpy(tmpfile, tmplate);
		mktemp(tmpfile);
	}
	sprintf(buf, "grep -v '(#)' '%s' > '%s' ;\
grep -v '(#)' '%s' | cmp -s - '%s'", f1, tmpfile, f2, tmpfile);
	retval = !system(buf);
	unlink(tmpfile);
	return retval;
}
#endif /* IGNORE_SCCSID */


atstrcmp(char **a, char **b)
{
	int	i;

	i = strcmp(*a,*b);
	return i;
}

/* ------------------------------------------------------------------------ */
/* these functions are more or less borrowed from pax */
#define	UNAMELEN	31	/* make this 31 because we need a NULL */
#define	GNAMELEN	31	/* make this 31 because we need a NULL */

uid_t saveuid = -993;	/* presumed to be an invalid uid */
gid_t savegid = -993;	/* presumed to be an invalid gid */
char saveuname[UNAMELEN];
char savegname[GNAMELEN];

char *
finduname(uid_t uid)
{
	struct passwd *pw;

	if ( uid != saveuid ) {
		saveuid = uid;
		if ( pw = getpwuid(uid) ) {
			strncpy(saveuname, pw->pw_name, UNAMELEN);
		} else
			sprintf(saveuname,"unknown%d",uid);
	}
	return saveuname;
}

char *
findgname(gid_t gid)
{
	struct group *gp;

	if ( gid != savegid ) {
		savegid = gid;
		if ( gp = getgrgid(gid) ) {
			strncpy(savegname, gp->gr_name, GNAMELEN);
		} else
			sprintf(savegname,"unknown%d",gid);
	}
	return savegname;
}

uid_t
finduid(struct pheader *hp)
{
	struct passwd *pw;

	if ( strncmp(hp->uname, saveuname, UNAMELEN) == 0 )
		return saveuid;

	strncpy(saveuname, hp->uname, UNAMELEN);
	if ( pw = getpwnam(hp->uname) ) {
		saveuid = pw->pw_uid;
	} else {
		saveuid = strtol(hp->uid, 0, 8);
	}
	return saveuid;
}

gid_t
findgid(struct pheader *hp)
{
	struct group *gp;

	if ( strncmp(hp->gname, savegname, GNAMELEN) == 0 )
		return savegid;

	strncpy(savegname, hp->gname, GNAMELEN);
	if ( gp = getgrnam(hp->gname) ) {
		savegid = gp->gr_gid;
	} else {
		savegid = strtol(hp->gid, 0, 8);
	}
	return savegid;
}
/* ------------------------------------------------------------------------ */

/*
 * Copy 'wholename' into header block, posix style:
 * 	if 'wholename' > 100 break up into prefix and name
 * Returns -1 if name too long
 * Returns 0 on success
 */
int
putname(struct pheader *hp, char *wholename)
{
	int len = strlen(wholename);

	if ( len > maxnamlen )
		return -1;

	if ( len > NAMSIZ ) {    /* if shortname-format, must count NULL */
		char *cp = wholename;
		cp += (PREFIXSIZ > len) ? len : PREFIXSIZ;
smore:
		while ( cp > wholename && *cp != '/' ) /* find a '/' */
			cp--;
		if ( cp == wholename ) /* no '/' found */
			return -1;
		if ( strlen(cp) > NAMSIZ ) /* "a/x<99chars>x" */
			return -1;
		if ( *(cp+1) == '\0' ) { /* BSD tar dirs end with '/' */
			--cp;
			goto smore;
		}
		strncpy(hp->prefix, wholename, cp - wholename);
		strncpy(hp->name, cp+1, len - (cp - wholename) - 1);
	} else {
		strncpy(hp->name, wholename, sizeof(hp->name));
	}
	return 0;
}

/*
 * Retrieve 'wholename' from header block, posix style:
 *	concatenate prefix '/' name
 */
char *
getname(struct pheader *hp)
{
	static char wholename[NAMSPACE + PREFIXSPACE+1];

	wholename[0] = '\0';
	if ( hp->prefix[0] != '\0' ) {
		strcpy(wholename, hp->prefix);
		strcat(wholename, "/");
	}
	strncat(wholename, hp->name,100);
	wholename[NAMSPACE + PREFIXSPACE] = '\0';

	return wholename;
}

char *
getlink(struct pheader *hp)
{
        static char wholelink[NAMSPACE + 1];

	wholelink[0] = '\0';
        strncat(wholelink, hp->linkname,NAMSPACE);
        wholelink[NAMSPACE] = '\0';
        return wholelink;
}


/* parseaction: looks at opt string: extracts action, disallowing duplicate
 * actions, also disallows duplicates of options that take args. */

static char
parseaction(char *cp)
{
char c;
char action = 0x00;
int  bseen = 0;
int  fseen = 0;

	while(c = *cp++) 
		switch (c) {

	case '\n' 	:
	case ' '  	:	break;

	case 'c'	:
	case 'C'	:
	case 'r'	:
	case 't'	:
	case 'u'	:
	case 'x'	:
	case 'X'	:
				if (action) goto duperr;
				action = c;
				continue;

	case 'f'	:	if (fseen) goto dferror;
				fseen = 1;
				continue;

	case 'b'	:	if (bseen) goto dberror;
				bseen = 1;
				continue;

	default		:	continue;
	};

	if (!action)
	{
		fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_NO_FUNCTION_SPECIFIER));  
		usage();
	}
	return (action);

duperr:
	fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_DUPLICATE_FUNCTION), action, c);  
	usage();

dberror:
	fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_DUPLICATE_OPTION));  
	usage();

dferror:
	fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_DUPLICATE_OPTION_F));  
	usage();
	/* NOTREACHED */
}

/* check to see if it's a drive, and is in audio mode; if it is, then
 * try to fix it, and also notify them.  Otherwise we can write in
 * audio mode, but they won't be able to get the data back
*/
void
chkandfixaudio(int mt)
{
	static struct mtop mtop = {MTAUD, 0};
	struct mtget mtget;
	unsigned status;

	if(ioctl(mt, MTIOCGET, &mtget))
		return;
	status = mtget.mt_dsreg | ((unsigned)mtget.mt_erreg<<16);
	if(status & CT_AUDIO) {
		fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_WARNING_DRIVE_WAS));  
		if(ioctl(mt, MTIOCTOP, &mtop) < 0)
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_WARNING_UNABLE));  
	}
}


#ifdef	TRUSTEDIRIX

void
attr_from_tape(void)
{
	int red = 0;
	long blocks;
	char *attrstrp, *bufp;
	char buf[400];

	clear_attr();
	blocks = stbuf.st_size;
	blocks += TBLOCK-1;
	blocks /= TBLOCK;
	g_attrbuf = malloc(blocks * TBLOCK + 1);
	g_attrbuf[0] = '\0';

	while (blocks-- > 0) {
		int cnt;
		if ((cnt = readtape(g_attrbuf + (red++ * TBLOCK))) < 0) {
			fflush(stdout);
			fprintf(stderr, CATGETS(catd,_MSG_TAR_TAR_TAPE_READ_ERROR1));  
			if (continflag)
				break;
			done(2);
		}
		if (cnt == 0)
			premature_eof();
	}
	g_attrbuf[stbuf.st_size] = '\0';
	attrstrp = g_attrbuf;
	bufp = buf;
	while (sscanf(attrstrp, "%s", buf) != EOF) {
		if (!strncmp(bufp, "mac=", 4)) {
			strncpy(g_lname, bufp + 4, sizeof(g_lname)-4);
		 	g_lp = mac_from_text(g_lname);
		} else
		if (!strncmp(bufp, "acl=", 4)) {
			strncpy(g_aclstr, bufp + 4, sizeof(g_aclstr)-4);
		 	g_aclp = acl_from_text(g_aclstr);
		}
		attrstrp += strlen(buf) + 1;
	}
}

void
clear_attr(void)
{
	if (g_attrbuf != NULL)
		free(g_attrbuf);
}

void
clear_glabel(void)
{
	if (g_lp)
		mac_free(g_lp);
	g_lname[0] = '\0';
}

void
clear_gacl(void)
{
	if (g_aclp)
		acl_free(g_aclp);
	g_aclstr[0] = '\0';
}
#endif	/* TRUSTEDIRIX */
