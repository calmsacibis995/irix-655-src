/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*  #ident	"@(#)cron:at.c	1.12" */
#ident	"$Revision: 1.25 $"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include "cron.h"
#include <locale.h>		/* i18n=internationalization */
#include <unistd.h>		/* i18n */
#include <sgi_nl.h>		/* i18n */
#include <msgs/uxsgicore.h>	/* i18n */
#include <stdlib.h>
#include <sys/capability.h>


#define TMPFILE		"_at"	/* prefix for temporary files	*/
#define ATMODE		06444	/* Mode for creating files in ATDIR.
Setuid bit on so that if an owner of a file gives that file 
away to someone else, the setuid bit will no longer be set.  
If this happens, atrun will not execute the file	*/
#define ROOT		0	/* user-id of super-user */
#define BUFSIZE		512	/* for copying files */
#define PIDSIZE         11      /* for PID as a string */
#define LINESIZE	130	/* for listing jobs */
#define	MAXTRYS		100	/* max trys to create at job file */
#define	RIGHT_NOW	2	/* execute at job right now */
#define TMPLINE_MAX	200

/* i18n messages (in irix/cmd/messages/uxsgicore/msgs.src) */
#define BADDATE		0 /* "bad date specification" 		*/
#define BADSHELL	1 /* "because your login shell isn't 
					/bin/sh, you can't use at"	*/
#define WARNSHELL	2 /* "warning: commands will be executed 	
					using /bin/sh\n"		*/
#define CANTCD		3 /* "can't change directory to the at directory"*/
#define CANTCHOWN	4 /* "can't change the owner of your job to you" */
#define CANTCREATE	5 /* "can't create a job for you"		*/
#define INVALIDUSER	6 /* "you are not a valid user (no entry in 
					/etc/passwd)"			*/
#define NOOPENDIR	7 /* "can't open the at directory"		*/
#define NOTALLOWED	8 /* "you are not authorized to use at.  Sorry." */
#define NOTHING		9 /* "nothing specified" */
#define NOPROTOTYPE	10 /* "no prototype" */
#define PIPEOPENFAIL	11 /* "pipe open failed" */
#define FORKFAIL	12 /* "fork failed" */
#define INVALIDQUEUE	13 /* "invalid queue specified: not lower case letter */
#define INCOMPATOPT	14 /* "incompatible options"*/
#define OUTOFMEM	15 /* "Out of memory" */
#define TOOLATE		16 /* "too late" */
#define QUEUEFULL	17 /* "queue full" */
#define CANTGETSTAT	18 /* "Can not get status of spooling directory for at*/
#define BADDATECONV	19 /* "bad date specification" */
#define IMPROPER	20 /* "job may not be executed at the proper time" */
#define NOCRON		21 /* "cron may not be running - call your system administrator " */
#define MSGQOPEN	22 /* "error in message queue open" */
#define MSGSENTERR	23 /* "error in message send" */
#define FILEOPENFAIL	24 /* "Cannot open %s" */

#define EXIT_WITH_1	1	/* for flag exit_1*/
#define DONT_EXIT	0	/* for flag exit_1*/

/*
	this data is used for parsing time
*/
#define	dysize(A)	(((A)%4) ? 365 : 366)
int	gmtflag = 0;
int	dflag = 0;
extern	char	*argp;
char	login[LLEN+1];
char	argpbuf[BUFSIZE];
char	pname[80];
char	pname1[80];
time_t	when, now, gtime();
struct	tm	*tp, at, rt;
int	mday[12] =
{
	31,38,31,
	30,31,30,
	31,31,30,
	31,30,31,
};
int	mtab[12] =
{
	0,   31,  59,
	90,  120, 151,
	181, 212, 243,
	273, 304, 334,
};
int     dmsize[12] = {
	31,28,31,30,31,30,31,31,30,31,30,31};

/* end of time parser */

short	jobtype = ATEVENT;		/* set to 1 if batch job */
char	*tfname;
char	*atpath;
struct	message msgbuf;
extern char *xmalloc();
extern int   per_errno;
extern void  exit();
time_t num();
char	now_flag=0;	/* 0=no "now", 1= "now", 2="rightnow" */
int	utc_flag = 0;
char 	*cbp;
time_t	timbuf;
char	filenamebuf[PATH_MAX+1]; 
char	fopt=0;

/* forward declarations */
void copy(/*char f_option*/);
static void sendmsg(char, char *);
static int cap_satgetid(void);
static void usages(void);

main(argc,argv)
char **argv;
{
	DIR *dir;
	struct dirent *dentry;
	struct passwd *pw;
	struct stat buf, st1, st2;
	int user,i,fd;
	void catch();
	unsigned int atdirlen;
	char *ptr,*job;
	char *pp, *atdir, *patdir;
	char *mkjobname(),*getuser();
	time_t t = 0;
	int  st = 1;
	char lopt=0, mopt=0, qopt=0, topt=0, ropt=0; /* options */
	char queuename='\0';
	char *ptrqueue;
	char compare=0;
	int c, len, errflg = 0;

	/* i18n */
	setlocale(LC_ALL,"");
	setcat("uxsgicore");
	setlabel("at");

	/* usage */
	if (argc < 2) {
		usages();
	}
	atpath = (sysconf(_SC_MAC) > 0) ? MACATDIR : ATDIR;
	pp = getuser((user=getuid()));
	if(pp == NULL) {
		if(per_errno == 2)
			atabort(BADSHELL, EXIT_WITH_1);
		else
			atabort(INVALIDUSER, EXIT_WITH_1);
	}
	strncpy(login, pp, (sizeof(login)-1));
	login[sizeof(login)-1] = 0;
	if (!allowed(login,ATALLOW,ATDENY)) atabort(NOTALLOWED, EXIT_WITH_1);


	/* analyze all options */

	while ((c = getopt(argc, argv, "f:q:t:mrl")) != -1) {
		switch (c) {
			case 'l':
				lopt ++;
				break;
			case 'm':
				mopt ++;
				break;
			case 'f':
				fopt ++;
				strncpy(filenamebuf, optarg, PATH_MAX);
				filenamebuf[PATH_MAX] = 0;
				break;
			case 't':
				topt = 1;
				cbp = optarg;
				if (topt_gtime((size_t)strlen(cbp))) {
					atabort(BADDATECONV, EXIT_WITH_1);
				}
				break;
			
			case 'r':
				ropt = 1;
				break;
			
			case 'q':
				qopt = 1;
				if ((strlen(optarg) != 1) || (*optarg < 'a') || 
					(*optarg > 'z')) {
					atabort(INVALIDQUEUE, EXIT_WITH_1);
					errflg++;
				}
				queuename = optarg[0];
				jobtype = (*optarg - 'a');
				break;
			default :
				errflg ++;
				break;
		}

	}

	if (errflg) {
		usages();
	}

	/* weed out incompatible options */
	if (((mopt || fopt || topt) && !(ropt) && !(lopt) )  
	 || (!(mopt || fopt || topt || lopt || qopt) && ropt )
	 || (!(mopt || fopt || topt ) && (lopt || qopt) && !ropt )
	 || (!(mopt || fopt || topt || lopt || qopt || ropt))) {
		st = optind; /* where time spec begins */
	}
	else	atabort(INCOMPATOPT, EXIT_WITH_1);

	/* -r option is always used by itself */
	if (ropt && !mopt && !fopt && !qopt && !topt && !lopt) {
		/* remove jobs that are specified */
		if (chdir(atpath)==-1) atabort(CANTCD, EXIT_WITH_1);
		for (i=2; i<argc; i++)
			if (stat(argv[i],&buf))
				_sgi_nl_error(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_jobdoesnotexist,"%s does not exist"),argv[i]);
			else if ((user!=buf.st_uid) && (user!=ROOT))
				_sgi_nl_error(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_youdontown,"you don't own %s"),argv[i]);
			else {
				sendmsg(DELETE,argv[i]);
				unlink(argv[i]);
			}
		exit(0); 
	}

	if ( lopt && !mopt && !topt && !fopt) {
		/* list jobs for user */
		if (chdir(atpath)==-1) atabort(CANTCD, EXIT_WITH_1);
		if ((argc==2) || (qopt)) {
			/* list all jobs for a user */
			atdirlen = strlen(atpath);
			if ((atdir = (char *)malloc(atdirlen + 1)) == NULL)
				atabort(OUTOFMEM, EXIT_WITH_1);
			strcpy(atdir, atpath);
			patdir= strrchr(atdir, '/');
			*patdir = '\0';
			if (stat(atpath,&st1) != 0 || stat(atdir,&st2) != 0)
				atabort(CANTGETSTAT, EXIT_WITH_1);
			if ((dir=opendir(atpath)) == NULL) atabort(NOOPENDIR, EXIT_WITH_1);
			for (;;) {
				if ((dentry = readdir(dir)) == NULL)
					break;
				if (dentry->d_ino==st1.st_ino || dentry->d_ino==st2.st_ino)
					continue;
				if (stat(dentry->d_name,&buf)) {
					unlink(dentry->d_name);
					continue; 
				}
				if ((user!=ROOT) && (buf.st_uid!=user))
					continue;
				ptr = dentry->d_name;
				if (((t=num(&ptr))==0) || (*ptr!='.'))
					continue;
				/* determine whether the queuename is in the */
				/*  job name: result in compare */
				ptrqueue = dentry->d_name;
				while (*ptrqueue != NULL) {
					if (*ptrqueue == '.') {
						ptrqueue++;
						if(*ptrqueue == queuename) 
							compare = 1;
						break;
					}
					ptrqueue++;
				}
				/* display either all jobs or only those that */
				/*  are associated with a given queue (option)*/
				if (!qopt || 
				   (qopt && compare)) {
				   if ((user==ROOT) && ((pw=getpwuid(buf.st_uid))!=NULL))
					printf("user = %s\t%s\t%s",pw->pw_name,dentry->d_name,asctime(localtime(&t)));
				   else	printf("%s\t%s",dentry->d_name,asctime(localtime(&t)));
				}
				compare = 0; /* reset for next job */
			}
			(void) closedir(dir);
		}
		else	/* list particular jobs for user */
			for (i=2; i<argc; i++) {
				ptr = argv[i];
				if (((t=num(&ptr))==0) || (*ptr!='.'))
					_sgi_nl_error(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_invalidjob,"invalid job name %s"),argv[i]);
				else if (stat(argv[i],&buf))
					_sgi_nl_error(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_jobdoesnotexist,"%s does not exist"),argv[i]);
				else if ((user!=buf.st_uid) && (user!=ROOT))
					_sgi_nl_error(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_youdontown,"you don't own %s"),argv[i]);
				else 
					printf("%s\t%s",argv[i],asctime(localtime(&t)));
			}
		exit(0);
	}

	/* figure out what time to run the job */

	if(argc == 1 && jobtype != BATCHEVENT)
		atabort(NOTHING, EXIT_WITH_1);
	time(&now);
	if(jobtype != BATCHEVENT) {	/* at job */
		argp = argpbuf;
		i = st;
                while((i < argc) && ((strlen(argp) + strlen(argv[i]))
                        < BUFSIZE))
		{
			strcat(argp,argv[i]);
			strcat(argp, " ");
			i++;
		}
		tp = localtime(&now);
		mday[1] = 28 + leap(tp->tm_year);
		if (!topt) yyparse();
		atime(&at, &rt);
		when = gtime(&at);
		if(!gmtflag) {
			if (topt) when = timbuf;
			when += timezone;
			if(localtime(&when)->tm_isdst)
			{
				when -= (time_t) (timezone - altzone);
			}
		}
		if (utc_flag) {
			if (when < now) when +=24 * 60 * 60; /* add a day */
		}
		if (now_flag == RIGHT_NOW) when = now;
	} else		/* batch job */
		when = now;
	if(when < now)	/* time has already past */
		atabort(TOOLATE, EXIT_WITH_1);

	len = strlen(atpath)+strlen(TMPFILE)+PIDSIZE+2;
	tfname = (char *)xmalloc(len);
	snprintf(tfname, len, "%s/%s.%d", atpath, TMPFILE, getpid());
	/* catch SIGINT, HUP, and QUIT signals */
	if (signal(SIGINT, catch) == SIG_IGN) signal(SIGINT,SIG_IGN);
	if (signal(SIGHUP, catch) == SIG_IGN) signal(SIGHUP,SIG_IGN);
	if (signal(SIGQUIT,catch) == SIG_IGN) signal(SIGQUIT,SIG_IGN);
	if (signal(SIGTERM,catch) == SIG_IGN) signal(SIGTERM,SIG_IGN);
	if((fd = open(tfname,O_CREAT|O_EXCL|O_WRONLY,ATMODE)) < 0)
		atabort(CANTCREATE, EXIT_WITH_1);
	if (chown(tfname,user,getgid())==-1) {
		unlink(tfname);
		atabort(CANTCHOWN, EXIT_WITH_1);
	}
	close(1); 	/* closing stdout */
	dup(fd);	/* duplicating the file descriptor */
	close(fd);
	snprintf(pname, sizeof(pname), "%s",PROTO);
	snprintf(pname1, sizeof(pname1), "%s.%c",PROTO,'a'+jobtype);
	copy(fopt);
	/* 
	 * with NFS v2 the link/unlink doesn't work with open files
	 * Close the file so that we'll be able to see it later. We won't
	 * need stdout again, so we're ok with it closed (bug 363028).
	 */
	close(1); 

	if (link(tfname,job=mkjobname(when,mopt))==-1) {
		unlink(tfname);
		atabort(CANTCREATE, EXIT_WITH_1);
	}
	unlink(tfname);
	sendmsg(ADD,strrchr(job,'/')+1);
	if(per_errno == 2)
		atabort(WARNSHELL, DONT_EXIT); 
	fprintf(stderr,"job %s at %.24s\n",strrchr(job,'/')+1,ctime(&when));
	
	if (when-t-CR_MINUTE < CR_HOUR) atabort(IMPROPER, DONT_EXIT);
	exit(0);
}

static void
usages(void)
{
	_sgi_nl_usage(SGINL_USAGE,"at",gettxt(_SGI_MMX_at_usage1,"at [-m] [-f file] [-q queuename] time [ date ] [ +increment ]"));
	_sgi_nl_usage(SGINL_USAGE,"at",gettxt(_SGI_MMX_at_usage2,"at -r job..."));
	_sgi_nl_usage(SGINL_USAGE,"at",gettxt(_SGI_MMX_at_usage3,"at -l [ job ... ]"));
	_sgi_nl_usage(SGINL_USAGE,"at",gettxt(_SGI_MMX_at_usage4,"at -l -q queuename..."));
	exit(2);
}


/***************/
find(elem,table,tabsize)
/***************/
char *elem,**table;
int tabsize;
{
	int i;

	for (i=0; i<strlen(elem); i++)
		elem[i] = tolower(elem[i]);
	for (i=0; i<tabsize; i++)
		if (strcmp(elem,table[i])==0) return(i);
		else if (strncmp(elem,table[i],3)==0) return(i);
	return(-1);
}


/****************/
char *mkjobname(t,m_option)
/****************/
time_t t; char m_option;
{
	int i;
	int satid = cap_satgetid();
	char *name;
	struct  stat buf;
	int len;

	name = (char *) xmalloc(TMPLINE_MAX);
	for (i=0;i < MAXTRYS;i++) {
		len = snprintf(name, TMPLINE_MAX, "%s/%ld.%c", atpath, t, 'a' + jobtype);
		if (m_option)
			if (len < (TMPLINE_MAX-1)) strcat(name, "m");
		if (satid != -1) {
			if (m_option) 
				snprintf((name + len +1), 
					(TMPLINE_MAX-len-1), "+%ld", (long) satid);
			else
				snprintf((name + len), 
					(TMPLINE_MAX-len), "+%ld", (long) satid);
		}
		if (stat(name,&buf)) 
			return(name);
		t += 1;
	}
	atabort(QUEUEFULL, EXIT_WITH_1);
}


/****************/
void
catch()
/****************/
{
	unlink(tfname);
	exit(1);
}


/****************/
atabort(msg_number, exit_1)
/****************/
/* this function displays all messages without arguments i18n'ly */ 
int msg_number; char exit_1;
{
	switch(msg_number) {
	case BADDATE:	 
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_baddate,"bad date specification"));
		break;
	case BADSHELL:	 
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_badshell,"because your login shell isn't /bin/sh, you can't use at"));
		break;
	case WARNSHELL: 
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_warnshell,"warning: commands will be executed using /bin/sh"));
		break;
	case CANTCD:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_cantcd,"can't change directory to the at directory"));
		break;
	case CANTCHOWN: 
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_cantchown,"can't change the owner of your job to you"));
		break;
	case CANTCREATE:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_cantcreate,"can't create a job for you"));
		break;
	case INVALIDUSER: 
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_invaliduser,"you are not a valid user (no entry in /etc/passwd)"));
		break;
	case NOOPENDIR:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_noopendir,"can't open the at directory"));
		break;
	case NOTALLOWED:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_notallowed,"you are not authorized to use at.  Sorry."));
		break;
	case NOTHING: 
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_nothing,"nothing specified"));
		break;
	case NOPROTOTYPE:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_noproto,"no prototype"));
		break;
	case PIPEOPENFAIL:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_pipefail,"pipe open failed"));
		break;
	case FORKFAIL: 
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_forkfail,"fork failed"));
		break;
	case INVALIDQUEUE:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_invalidqueue,"invalid queue specified: not lower case letter"));
		break;
	case INCOMPATOPT:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_incompatopt,"incompatible options"));
		break;
	case OUTOFMEM:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_outofmem,"Out of memory"));
		break;
	case TOOLATE:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_toolate,"too late"));
		break;
	case QUEUEFULL:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_queuefull,"queue full"));
		break;
	case CANTGETSTAT:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_cantgetstat,"Can not get status of spooling directory for at"));
		break;
	case BADDATECONV:	 
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_baddateconv,"bad date conversion"));
		break;
	case IMPROPER:	 
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_at_improper,"this job may not be executed at the proper time"));
		break;
	case FILEOPENFAIL:
		_sgi_nl_usage(SGINL_NOSYSERR,"at",gettxt(_SGI_MMX_CannotOpen,"Cannot open file %s"),filenamebuf);
		break;

	default:
		break;
	}
	
	if (exit_1) exit(1); 
}

yywrap()
{
	return 1;
}

yyerror()
{
	atabort(BADDATE, EXIT_WITH_1);
}

/*
 * add time structures logically
 */
atime(a, b)
register
struct tm *a, *b;
{
	if ((a->tm_sec += b->tm_sec) >= 60) {
		b->tm_min += a->tm_sec / 60;
		a->tm_sec %= 60;
	}
	if ((a->tm_min += b->tm_min) >= 60) {
		b->tm_hour += a->tm_min / 60;
		a->tm_min %= 60;
	}
	if ((a->tm_hour += b->tm_hour) >= 24) {
		b->tm_mday += a->tm_hour / 24;
		a->tm_hour %= 24;
	}
	a->tm_year += b->tm_year;
	if ((a->tm_mon += b->tm_mon) >= 12) {
		a->tm_year += a->tm_mon / 12;
		a->tm_mon %= 12;
	}
	a->tm_mday += b->tm_mday;
	while (a->tm_mday > mday[a->tm_mon]) {
		a->tm_mday -= mday[a->tm_mon++];
		if (a->tm_mon > 11) {
			a->tm_mon = 0;
			mday[1] = 28 + leap(++a->tm_year);
		}
	}
}

leap(year)
{
	return year % 4 == 0;
}

/*
 * return time from time structure
 */
time_t
gtime(tptr)
register
struct	tm *tptr;
{
	register i;
	long	tv;
	extern int dmsize[];

	tv = 0;
	for (i = 1970; i < tptr->tm_year+1900; i++)
		tv += dysize(i);
	if (dysize(tptr->tm_year) == 366 && tptr->tm_mon >= 2)
		++tv;
	for (i = 0; i < tptr->tm_mon; ++i)
		tv += dmsize[i];
	tv += tptr->tm_mday - 1;
	tv = 24 * tv + tptr->tm_hour;
	tv = 60 * tv + tptr->tm_min;
	tv = 60 * tv + tptr->tm_sec;
	return tv;
}

/*
 * make job file from proto + stdin
 */
void
copy(f_option)
char f_option;	/* whether the f option was selected on the cmd line */
{
	register c;
	register FILE *pfp;
	char	dirbuf[PATH_MAX];
	register char **ep;
	struct rlimit rlimtab;
	unsigned um;
	char *val;
	extern char **environ;
	uid_t euid, ruid;

	printf(": %s job\n",jobtype ? "batch" : "at");
	for (ep=environ; *ep; ep++) {
		if ( strchr(*ep,'\'')!=NULL )
			continue;
		if ((val=strchr(*ep,'='))==NULL)
			continue;
		*val++ = '\0';
		printf("export %s; %s='%s'\n",*ep,*ep,val);
		*--val = '=';
	}
	if((pfp = fopen(pname1,"r")) == NULL && (pfp=fopen(pname,"r"))==NULL)
		atabort(NOPROTOTYPE, EXIT_WITH_1);
	um = umask(0);
	while ((c = getc(pfp)) != EOF) {
		if (c != '$')
			putchar(c);
		else switch (c = getc(pfp)) {
		case EOF:
			goto out;
		case 'd':
			dirbuf[0] = NULL;

			/* save e uid */
			euid = geteuid();
			ruid = getuid();

			/* use real uid for operation */
			if (setreuid((uid_t)-1, ruid))
				atabort(NOTALLOWED, EXIT_WITH_1);

			if (getcwd(dirbuf, PATH_MAX) == NULL)
				atabort(NOTALLOWED, EXIT_WITH_1);

			/* revert back to saved e uid */
			if (setreuid((uid_t)-1, euid))
				atabort(NOTALLOWED, EXIT_WITH_1);

			printf("%s", dirbuf);

			break;
		case 'l':
			getrlimit(RLIMIT_FSIZE, &rlimtab);
			if (rlimtab.rlim_cur == RLIM_INFINITY)
				printf("unlimited");
			else
				printf("%llu", (unsigned long long) rlimtab.rlim_cur);

			break;
		case 'm':
			printf("%o", um);
			break;
		case '<':
			fclose (pfp);

			if (f_option) {
				/* save e uid */
				euid = geteuid();
				ruid = getuid();

				/* use real uid for file open */
				if (setreuid((uid_t)-1, ruid))
					atabort(NOTALLOWED, EXIT_WITH_1);

				if ((pfp = fopen(filenamebuf,"r")) == NULL)
					atabort(FILEOPENFAIL, EXIT_WITH_1);

				while ((c = getc(pfp)) != EOF)
					putchar(c);

				/* revert back to saved e uid */
				if (setreuid((uid_t)-1, euid))
					atabort(NOTALLOWED, EXIT_WITH_1);
			}
			else
				while ((c = getchar()) != EOF)
					 putchar(c);
			
			break;
		case 't':
			printf(":%lu", when);
			break;
		default:
			putchar(c);
		}
	}
out:
	/*
	 * Flush the output before we tell cron about this command file.
	 */
	fflush(stdout);
	fclose(pfp);
}

/****************/
static void
sendmsg(char action, char *fname)
/****************/
{

	static	int	msgfd = -2;
	struct	message	*pmsg;

	pmsg = &msgbuf;
	if(msgfd == -2)
		if((msgfd = open(FIFO,O_WRONLY|O_NDELAY)) < 0) {
			if(errno == ENXIO || errno == ENOENT)
				atabort(NOCRON, DONT_EXIT);
			else
				atabort(MSGQOPEN, DONT_EXIT);
			return;
		}
	pmsg->etype = AT;
	pmsg->action = action;
	strncpy(pmsg->fname,fname,FLEN);
	strncpy(pmsg->logname,login,LLEN);
	if(write(msgfd,pmsg,sizeof(struct message)) != sizeof(struct message))
		atabort(MSGSENTERR, DONT_EXIT);
}

/* This function translates the -t option time into timbuf */
/* This is very similar to gtime in the command "touch" */
int
topt_gtime(size_t len)
{
	register int i, y, t;
	int d, h, m;
	long nt;
	int c = 0;		/* entered century value */
	int s = 0;		/* entered seconds value */
	int point = 0;		/* relative byte position of decimal point */
	int ssdigits = 0;	/* count seconds digits include decimal pnt */
	int noyear = 0;		/* 0 means year is entered; 1 means none */

	tzset();

	/*
	 * mmddhhmm is a total of 8 bytes min
	 */
	if (len < 8)
		return(1);
	/* if (which) {		if '1'; means -t option */
		/*
		 * -t [[cc]yy]mmddhhmm[.ss] is a total of 15 bytes max
		 */
		if (len > 15)
			return(1);
		/*
		 *  Determine the decimal point position, if any.
		 */
		for (i=0; i<len; i++) {
			if (*(cbp + i) == '.') {
				point = i;
				break;
			}
		}
		/*
		 *  If there is a decimal point present,
		 *  AND:
		 *
		 *	the decimal point is positioned in bytes 0 thru 7;
		 *  OR
		 *	the the number of digits following the decimal point
		 *	is greater than two
		 *  OR
		 *	the the number of digits following the decimal point
		 *	is greater than two
		 *  OR
		 *	the the number of digits following the decimal point
		 *	is less than two
		 *  then,
		 *	error terminate.
		 *      
		 */
		/* the "+ 1" below means add one for decimal */
		if (point && ((point < 8) || ((len - (point + 1)) > 2) ||
				((len - (point + 1)) < 2)) )
		{
			return(1);
		}
		/*
		 * -t [[cc]yy]mmddhhmm.[ss] is greater than 12 bytes
		 * -t [yy]mmddhhmm.[ss]     is greater than 12 bytes
		 *
		 *  If there is no decimal present and the length is greater
		 *  than 12 bytes, then error terminate.
		 */
		if (!point && (len > 12))
			return(1);
		switch(len) {
		case 11:
			if (*(cbp + 8) != '.')
				return(1);
			break;
		case 13:
			if (*(cbp + 10) != '.')
				return(1);
			break;
		case 15:
			if (*(cbp + 12) != '.')
				return(1);
			break;
		}
		if (!point)
			ssdigits = 0;
		else
			ssdigits = ((len - point) + 1);
		if ((len - ssdigits) > 10) {
			/*
			 * -t ccyymmddhhmm is the input
			 */

			/* detemine c -- century number */
			c = gpair();

			/* detemine y -- year    number */
			y = gpair();
			if (y<0) {
				(void) time(&nt);
				y = localtime(&nt)->tm_year;
			}
			if ((c != 19) && ((y >= 69) && (y <= 99)))
				return(1);
			if ((c != 20) && ((y >= 0) && (y <= 68)))
				return(1);
			goto mm_next;
		}
		if ((len - ssdigits) > 8) {
			/*
			 * -t yymmddhhmm is the input
			 */
			/* detemine yy -- year    number */
			y = gpair();
			if (y<0) {
				(void) time(&nt);
				y = localtime(&nt)->tm_year;
			}
			if ((y >= 69) && (y <= 99))
				c = 19;			/* 19th century */
			if ((y >= 0) && (y <= 68))
				c = 20;			/* 20th century */
			goto mm_next;
		}
		if ((len - ssdigits) < 10) {
			/*
			 * -t mmddhhmm is the input
			 */
			noyear++;
		}
	/* } */
mm_next:
	t = gpair();
	if(t<1 || t>12)
		return(1);
	d = gpair();
	if(d<1 || d>31)
		return(1);
	h = gpair();
	if(h == 24) {
		h = 0;
		d++;
	}
	m = gpair();
	if(m<0 || m>59)
		return(1);
	/* } else { */
	/* realign ! aaa */
		/*
		 * There was a "-t" input.
		 * If there is a decimal get the seconds inout
		 */
		 if (point) {
			cbp++;		/* skip over decimal point */
			s = gpair();	/* get [ss] */
			if (s<0) {
				return(1);
			}
			if (!((s >= 0) && (s <= 61)))
				return(1);
		 }
		 if (noyear) {
			(void) time(&nt);
			y = localtime(&nt)->tm_year;
		 }
	/* } */
	if (*cbp == 'p')
		h += 12;
	if (h<0 || h>23)
		return(1);
	timbuf = 0;
	if (c && (c == 20))
		y += 2000;
	else
		y += 1900;
	for(i=1970; i<y; i++)
		timbuf += dysize(i);
	/* Leap year */
	if (dysize(y)==366 && t >= 3)
		timbuf += 1;
	while(--t)
		timbuf += dmsize[t-1];
	timbuf += (d-1);
	timbuf *= 24;
	timbuf += h;
	timbuf *= 60;
	timbuf += m;
	timbuf *= 60;
	timbuf += s;
	return(0);
}

/* This function is called by topt_gtime */
/* This is the same gpair function as in the command "touch" */
gpair()
{
	register int c, d;
	register char *cp;

	cp = cbp;
	if(*cp == 0)
		return(-1);
	c = (*cp++ - '0') * 10;
	if (c<0 || c>100)
		return(-1);
	if(*cp == 0)
		return(-1);
	if ((d = *cp++ - '0') < 0 || d > 9)
		return(-1);
	cbp = cp;
	return (c+d);
}

static int
cap_satgetid(void)
{
	cap_t ocap;
	const cap_value_t cv = CAP_AUDIT_CONTROL;
	int r;

	ocap = cap_acquire(1, &cv);
	r = satgetid();
	cap_surrender(ocap);
	return(r);
}
