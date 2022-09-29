/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Revision: 1.13 $"
/**************************************************************************
 ***			C r o n t a b . c				***
 **************************************************************************

	date:	7/2/82
	description:	This program implements crontab (see cron(1)).
			This program should be set-uid to root.
	files:
		/usr/lib/cron drwxr-xr-x root sys
		/usr/lib/cron/cron.allow -rw-r--r-- root sys
		/usr/lib/cron/cron.deny -rw-r--r-- root sys

 **************************************************************************/


#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <getopt.h>
#include "cron.h"

#define TMPFILE		"_cron"		/* prefix for tmp file */
#define CRMODE		0444	/* mode for creating crontabs */

#define BADCREATE	"can't create '%s' in the crontab directory: %s"
#define BADOPEN		"can't open crontab file: %s: %s"
#define BADSHELL	"because your login shell isn't /bin/sh, you can't use cron"
#define BADUSAGE	"proper usage is one of:\n" \
			"\tcrontab [file]\n" \
			"\tcrontab -l [username]\n" \
			"\tcrontab -r [username]\n" \
			"\tcrontab -e [username]\n" \
			"(Only root can access other people's crontabs)"
#define INVALIDFILE	"can't open %s: %s"
#define INVALIDUSER	"%s not a valid user (no entry in /etc/passwd)"
#define NOTALLOWED	"you are not authorized to use cron. Sorry."
#define BADPRIV		"only root is authorized to access other people's crontabs. Sorry."
#define EOLN		"unexpected end of line"
#define UNEXPECT	"unexpected character found in line"
#define OUTOFBOUND	"number out of bounds"
#define SYNTAX		"the crontab file had syntax errors in it; no change was made"
#define NOLINES		"no lines in crontab file - ignoring. use crontab -r to remove a crontab"
#define BADREAD		"error reading your crontab file %s: %s"

extern int per_errno;
extern char *xmalloc();
#ifdef sgi
int err,cursor;
static void catch();
static void sendmsg(char, char *);
static char *cf_create(char *, char *);
#else
int err,cursor,catch();
#endif
char *cf,*tnam,line[CTLINESIZE];
struct message msgbuf;
char edtemp[5+13+1];

static void crabort(const char * msg, ...);

main(argc,argv)
char **argv;
{
	int c, rflag, lflag, eflag, errflg;
	char login[UNAMESIZE],*getuser();
	char *crontab_file = NULL;
	char *pp;
	FILE *fp, *tmpfp;
	struct stat stbuf;
	time_t omodtime;
	char *editor, *getenv();
	char buf[BUFSIZ];
	pid_t pid;
	uid_t ruid;
	int stat_loc;

	rflag = 0;
	lflag = 0;
	eflag = 0;
	errflg = 0;
	while ((c=getopt(argc, argv, "elr")) != EOF)
		switch (c) {
			case 'e':
				argc--;
				if (lflag || rflag)
					errflg++;
				else
					eflag++;
				break;
			case 'l':
				argc--;
				if (rflag || eflag)
					errflg++;
				else
					lflag++;
				break;
			case 'r':
				argc--;
				if (lflag || eflag)
					errflg++;
				else
					rflag++;
				break;
			case '?':
				errflg++;
				break;
		}

	ruid = getuid();
	/*
	 * after processing -[l|e|r] we have either:
	 *	argc == 2 (a file name was given)
	 *	argc == 1 (no arguments after command name)
	 * In the argc == 1 case we either read stdin OR
	 * the default user crontab (if -[l|e|r] was given)
	 */
	if (errflg || argc > 2)
		crabort(BADUSAGE);

	if ((pp = getuser(ruid)) == NULL) {
		if (per_errno==2)
			crabort(BADSHELL);
		else
			crabort(INVALIDUSER, "you are"); 
	}
	strcpy(login,pp);
	if (!allowed(login,CRONALLOW,CRONDENY))
		crabort(NOTALLOWED);

	if (argc == 2) {	/* argument given: username or filename */
		if (lflag || rflag || eflag) {

			argv++;		/* skip the option flag it */

			if (getpwnam(argv[1]) == NULL)	/* invalid username */
				crabort(INVALIDUSER, argv[1]);

			/* valid username: are we allowed? */
			if (ruid == 0 || strcmp(argv[1], login) == 0)
				crontab_file = argv[1];
			else
				crabort(BADPRIV);

		} else if (access(argv[1], F_OK) != 0)
			/* No [l|r|e] flag and invalid filename */
			crabort(INVALIDFILE, argv[1], strerror(errno));
	}
	/* target crontab file */
	if (crontab_file == NULL) /* not set, use default: real username */
		crontab_file = login;

	cf = cf_create(crontab_file, "");
	if (rflag) {
		unlink(cf);
		sendmsg(DELETE,crontab_file);
		exit(0);
	}
	if (lflag) {
		if((fp = fopen(cf,"r")) == NULL)
			crabort(BADOPEN, cf, strerror(errno));
		while(fgets(line,CTLINESIZE,fp) != NULL)
			fputs(line,stdout);
		fclose(fp);
		exit(0);
	}

	if (eflag) {
                if((fp = fopen(cf,"r")) == NULL) {
                        if(errno != ENOENT)
                                crabort(BADOPEN, cf, strerror(errno));
                }
                (void)strcpy(edtemp, "/tmp/crontabXXXXXX");
                (void)mktemp(edtemp);
                /*
                 * Fork off a child with user's permissions,
                 * to edit the crontab file
                 */
                if ((pid = fork()) == (pid_t)-1)
                        crabort("fork failed");
                if (pid == 0) {         /* child process */
                        /* give up super-user privileges. */
                        setuid(ruid);
                        if((tmpfp = fopen(edtemp,"w")) == NULL)
                                crabort("can't create temporary file");
                        if(fp != NULL) {
                                /*
                                 * Copy user's crontab file to temporary file.                                 */
                                while(fgets(line,CTLINESIZE,fp) != NULL) {
                                        fputs(line,tmpfp);
                                        if(ferror(tmpfp)) {
                                                fclose(fp);
                                                fclose(tmpfp);
                                                crabort("write error on temporary file");
                                        }
                                }
                                if (ferror(fp)) {
                                        fclose(fp);
                                        fclose(tmpfp);
                                        crabort(BADREAD, cf, strerror(errno));
                                }
                                fclose(fp);
			}
                        if(fclose(tmpfp) == EOF)
                                crabort("write error on temporary file");
                        if(stat(edtemp, &stbuf) < 0)
                                crabort("can't stat temporary file");
                        omodtime = stbuf.st_mtime;
                        editor = getenv("VISUAL");
                        if (editor == NULL)
                                editor = getenv("EDITOR");
                        if (editor == NULL)
                                editor = "vi";
                        (void)sprintf(buf, "%s %s", editor, edtemp);
                        if (system(buf) == 0) {
                                /* sanity checks */
                                if((tmpfp = fopen(edtemp, "r")) == NULL)
                                        crabort("can't open temporary file");
                                if(fstat(fileno(tmpfp), &stbuf) < 0)
                                        crabort("can't stat temporary file");
                                if(stbuf.st_size == 0)
                                        crabort("temporary file empty");
                                if(omodtime == stbuf.st_mtime) {
                                        (void)unlink(edtemp);
                                        fprintf(stderr,
                                            "The crontab file was not changed.\n");
                                        exit(1);
                                }
                                exit(0);
                        } else {
                                /*
                                 * Couldn't run editor.
                                 */
                                (void)unlink(edtemp);
                                exit(1);
                        }
                }
                wait(&stat_loc);
                if ((stat_loc & 0xFF00) != 0)
 			exit(1);
                if ((tmpfp = fopen(edtemp,"r")) == NULL)
                        crabort("can't open temporary file\n");
                copycron(tmpfp);
                (void)unlink(edtemp);
	} else {
		if (argc==1) 
			copycron(stdin);
		else if (access(argv[1],04) || (fp=fopen(argv[1],"r"))==NULL) 
			crabort(BADOPEN, argv[1], strerror(errno));
		else copycron(fp);
	}
	sendmsg(ADD,crontab_file);
	exit(0);
}


/******************/
copycron(fp)
/******************/
FILE *fp;
{
#ifdef sgi
	FILE *tfp;
	int newlines = 0;
#else
	FILE *tfp,*fdopen();
#endif
	char pid[6];
	int t;

	sprintf(pid,"%-5d",getpid());
	tnam = cf_create(TMPFILE, pid);
	/* catch SIGINT, SIGHUP, SIGQUIT signals */
	if (signal(SIGINT,catch) == SIG_IGN) signal(SIGINT,SIG_IGN);
	if (signal(SIGHUP,catch) == SIG_IGN) signal(SIGHUP,SIG_IGN);
	if (signal(SIGQUIT,catch) == SIG_IGN) signal(SIGQUIT,SIG_IGN);
	if (signal(SIGTERM,catch) == SIG_IGN) signal(SIGTERM,SIG_IGN);
	if ((t=creat(tnam,CRMODE))==-1) crabort(BADCREATE,tnam,strerror(errno));
	if ((tfp=fdopen(t,"w"))==NULL) {
		unlink(tnam);
		crabort(BADCREATE,tnam,strerror(errno)); 
	}
	err=0;	/* if errors found, err set to 1 */
	while (fgets(line,CTLINESIZE,fp) != NULL) {
		cursor=0;
		while(line[cursor] == ' ' || line[cursor] == '\t')
			cursor++;
		if(line[cursor] == '#' || line[cursor] == '\n')
			goto cont;
		if (next_field(0,59)) continue;
		if (next_field(0,23)) continue;
		if (next_field(1,31)) continue;
		if (next_field(1,12)) continue;
		if (next_field(0,06)) continue;
		if (line[++cursor] == '\0') {
			cerror(EOLN);
			continue; 
		}
cont:
		if (fputs(line,tfp) == EOF) {
			unlink(tnam);
			crabort(BADCREATE,tnam,strerror(errno)); 
		}
		newlines++;
	}
	fclose(fp);
	fclose(tfp);
	if (!err) {
		if (newlines == 0) {
			unlink(tnam);
			crabort(NOLINES); 
		}
		/* make file tfp the new crontab */
		if (rename(tnam,cf)==-1) {
			unlink(tnam);
			crabort(BADCREATE,cf,strerror(errno)); 
		} 
	} else {
		unlink(tnam);
		crabort(SYNTAX);
	}
	unlink(tnam);
}


/*****************/
next_field(lower,upper)
/*****************/
int lower,upper;
{
	int num,num2;

	while ((line[cursor]==' ') || (line[cursor]=='\t')) cursor++;
	if (line[cursor] == '\0') {
		cerror(EOLN);
		return(1); 
	}
	if (line[cursor] == '*') {
		cursor++;
		if ((line[cursor]!=' ') && (line[cursor]!='\t')) {
			cerror(UNEXPECT);
			return(1); 
		}
		return(0); 
	}
	while (TRUE) {
		if (!isdigit(line[cursor])) {
			cerror(UNEXPECT);
			return(1); 
		}
		num = 0;
		do { 
			num = num*10 + (line[cursor]-'0'); 
		}			while (isdigit(line[++cursor]));
		if ((num<lower) || (num>upper)) {
			cerror(OUTOFBOUND);
			return(1); 
		}
		if (line[cursor]=='-') {
			if (!isdigit(line[++cursor])) {
				cerror(UNEXPECT);
				return(1); 
			}
			num2 = 0;
			do { 
				num2 = num2*10 + (line[cursor]-'0'); 
			}				while (isdigit(line[++cursor]));
			if ((num2<lower) || (num2>upper)) {
				cerror(OUTOFBOUND);
				return(1); 
			}
		}
		if ((line[cursor]==' ') || (line[cursor]=='\t')) break;
		if (line[cursor]=='\0') {
			cerror(EOLN);
			return(1); 
		}
		if (line[cursor++]!=',') {
			cerror(UNEXPECT);
			return(1); 
		}
	}
	return(0);
}


/**********/
cerror(msg)
/**********/
char *msg;
{
	fprintf(stderr,"%scrontab: error on previous line; %s\n",line,msg);
	err=1;
}


/**********/
#ifdef sgi
static void
#endif
catch()
/**********/
{
	unlink(tnam);
	exit(1);
}


/**********/
static void crabort(const char * msg, ...)
/**********/
{
	va_list ap;
	int sverrno;

	if (strcmp(edtemp, "") != 0) {
                sverrno = errno;
                (void)unlink(edtemp);
                errno = sverrno;
        }

	if (tnam != NULL) {
                sverrno = errno;
                (void)unlink(tnam);
                errno = sverrno;
        }
	va_start(ap, msg);
        (void)fprintf(stderr, "crontab: ");
        (void)vfprintf(stderr, msg, ap);
        va_end(ap);
        (void)fprintf(stderr, "\n");

	exit(1);
        /* NOTREACHED */
}

/***********/
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
				fprintf(stderr,"cron may not be running - call your system administrator\n");
			else
				fprintf(stderr,"at: error in message queue open\n");
			return;
		}
	pmsg->etype = CRON;
	pmsg->action = action;
	strncpy(pmsg->fname,fname,FLEN);
	if(write(msgfd,pmsg,sizeof(struct message)) != sizeof(struct message))
		fprintf(stderr,"at: error in message send\n");
}

char *
cf_create(char *name, char *suffix)
{
	char *root = (sysconf(_SC_MAC) > 0) ? MACCRONDIR : CRONDIR;
	char *cp = xmalloc(strlen(root) + strlen(name) + strlen(suffix) + 2);

	sprintf(cp, "%s/%s%s", root, name, suffix);
	return (cp);
}
