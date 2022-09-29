#ident	"$Revision: 1.55 $"

/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1989, Silicon Graphics, Inc.               *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/

/*
//  NAME
//	man - print entries from the on-line reference manuals; find manual
//	entries by keyword
*/

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <ctype.h>
#include <math.h>
#include <curses.h>
#include <term.h>
#include <termio.h>
#include <errno.h>
#include <malloc.h>
#include <locale.h>
#include "path.h"

#include <nl_types.h>
#include <widec.h>
#include <regex.h>
#include <i18n_capable.h>
#include <msgs/uxeoe.h>
nl_catd catd=0;
/*
//  Imported routines without include file declarations
*/
extern char *		glob2regex(char *, const char *);

/*
//  Routines
*/
extern int	main(int argc, char * argv[]);
static int	parsePaths(char * pathstr, char * patharray[]);
/* The code is changed to use recomp function instead of regcmp and wsrecompile .
   So the prototypes of following two functions have been changed .
*/
static void	looksection(char *section, regex_t * manregexpr,
				regex_t *anymanregexpr, char * manpaths[]);
static int	findman(char * rootpath, regex_t * name, regex_t *anyname,
				int pagetype);
static void	launch(char * name);
static void	deferlaunch(char *name, int match);
static void	schedulelaunch(void);
static void	apropos(int argc, char * argv[], char * manpaths[]);
static void	whatis(int argc, char * argv[], char * manpaths[]);
static unsigned int isRegEx(const char * str);
static unsigned int filetype(char * file);
static void	initterm(void);
static void	prompt(char * str);
static void	set_tty (void);
static void	reset_tty (void);
static void	waitonchildren(void);
static char *	strcopylower(char * strout, char * strin, char terminator);
static void	clearprompt(void);
static void	resetexit(void);
static void	waitexit(void);

static wchar_t *wstrcopylower(wchar_t * wstrout, wchar_t * wstrin, wchar_t wterminator);
char promptstr[BUFSIZ];
wchar_t wcyes,wcno,wcquit;
char yesch,noch,quitch;

#define	DEFAULTMANPATHS "/usr/share/catman:/usr/share/man:/usr/catman:/usr/man"

/*
 * sections are one of ALLSECTIONS plus any name <= 3 chars and beginning
 * with a [1-8]
 * This defines the search order
 * Section 'D' is the kernel man pages for the Device Driver Writer's Guide.
 */
char *Allsections[] = {
	"1", "n", "l", "6", "8", "2", "3", "4", "5", "7", "p", "o", "D", 0
};

/* Suffixes to man pages that manregexpr will accept */
char *suf = "(_(32)*(64)*(att)*(bsd)*(mips)*(sun)*)*\\..*";

char *books[] = {
    "local",
    "u_man",
    "a_man",
    "p_man",
    "g_man",
    0
};
#define	DEFAULTPAGER "ul -b | more -s -f"
#define DEFAULTTROFFDISPLAY "lp"
#define DEFAULTTROFFCMD "psroff -t"
#define DEFAULTNROFFCMD "nroff"

char *manprinter = "/usr/lib/print/manprint";
static char * macropackage = "/usr/lib/tmac/tmac.an";
static char * filtercmd = "col";
static char * htmlfiltercmd = "/usr/sbin/html2term";

static char * pager;
static char * troffdisplay;
static char * nroffdisplay;
static char * troffcmd;
static char * nroffcmd;
static char * manfmtcmd;
static char * awfcmd = "awf -man";
int displayenv;

#define	GUESS	0
#define	CAT	1
#define	MAN	2

#define	WHATIS	"whatis"

#define	BUFSIZE 4096
#define MANDIRCNT 200	/* 1/2 this is max dirs in manpath */

#define	USAGE "Usage:  man [-cdwWtpr] [-M manpath] [-T macropackage] [section] title [...]\n        man -k keyword [...]\n        man -f file [...]\n"

#define	PACKMAGIC	017436
#define	COMPRESSMAGIC	0x1f9d
#define	GZIPMAGIC	0x1f8b

#define NOTREGULARFILE  0
#define	PACKFILE	1
#define	COMPRESSFILE	2
#define	NROFFFILE	3
#define	CATFILE		4
#define	GZIPFILE	5
#define GZIPHTML	6

#define GZIPFILEOFFSET  10

char *		catpathregexpr;

unsigned	aproposflag, whatisflag;
unsigned	troffflag, macroflag;
unsigned	whereisflag, directpathflag;
unsigned	whatisdbflag;
unsigned	printcmdflag;
unsigned	catflag;
unsigned	isregexflag;
unsigned	useregexflag;


unsigned	found;
unsigned	pagecount;
int		ttyoutput;

struct launchlist {
	struct launchlist	*next;		/* Next to launch */
	char			*newpath;	/* Path to launch */
} LaunchList[2];
struct launchlist *CurLaunchList[2];

extern int
main(int argc, char * argv[])
{
    regex_t *		manregexpr;
    regex_t *		anymanregexpr;
    char *		manpaths[MANDIRCNT];
    register char	*section;
    register char **	sect;
    register char *	envmp;
    register unsigned	i;
    register int 	o;
    extern char *	optarg;
    extern int 		optind;
    char*		cmdname;

    char ystr[MB_LEN_MAX],nstr[MB_LEN_MAX],qstr[MB_LEN_MAX];

    (void) setlocale(LC_ALL, "");
    
    catd=catopen("uxeoe",0);

    strcpy(promptstr, CATGETS(catd, _MSG_MAN_PROMPT));
    strcpy(ystr, CATGETS(catd, _MSG_MAN_YESCHAR));
    strcpy(nstr, CATGETS(catd, _MSG_MAN_NOCHAR));
    strcpy(qstr, CATGETS(catd, _MSG_MAN_QUITCHAR));

    if (I18N_SBCS_CODE)
    {
	yesch=ystr[0]; noch=nstr[0]; quitch=qstr[0];
    }
    else
    {
	mbtowc(&wcyes, ystr, strlen(ystr));
	mbtowc(&wcno, nstr, strlen(nstr));
	mbtowc(&wcquit, qstr, strlen(qstr));
    }

    sigset(SIGINT, waitexit);	/* Make sure all children have terminated */

    initterm();			/* All this just to do a highlighted prompt */

    /* Set up defaults search paths */
    parsePaths(DEFAULTMANPATHS, manpaths);

    if ((envmp = getenv("MANPATH")) != NULL) {
	if (parsePaths(envmp, manpaths) == 0) {
	    parsePaths(DEFAULTMANPATHS, manpaths);
	}
    }

    /* Initialize globals */
    pager = getenv("MANPAGER");
    if (pager == 0 || pager[0] == '\0') pager = getenv("PAGER");
    if (pager == 0 || pager[0] == '\0') pager = DEFAULTPAGER;

    troffdisplay = getenv("TCAT");
    if (troffdisplay == 0) troffdisplay = DEFAULTTROFFDISPLAY;
    else displayenv = 1;

    nroffdisplay = getenv("NCAT");
    if (nroffdisplay == 0) nroffdisplay = troffdisplay;
    else displayenv = 1;

    troffcmd = getenv("TROFF");
    if (troffcmd == 0) troffcmd = DEFAULTTROFFCMD;

    nroffcmd = getenv("NROFF");
    if (nroffcmd == 0) nroffcmd = DEFAULTNROFFCMD;

	/* if set, this is the *full* command we us to format unformatted
	 * man pages.  Otherwise we have to add tbl, eqn, and a way to
	 * specify arguments to nroff, etc.  For exampe, this allows
	 * us to use things Henry Spencer's "awf" simply.
	*/
    manfmtcmd = getenv("MANFMTCMD");

    /* Parse args */
    cmdname = pathbasename(argv[0]);
    if (strcmp(cmdname, "apropos") == 0) {
	apropos(argc-1, argv+1, manpaths);
	catclose(catd);
	exit(0);
    }

    if (strcmp(cmdname, "whatis") == 0) {
	whatis(argc-1, argv+1, manpaths);
	catclose(catd);
	exit(0);
    }

    if (argc <= 1) {
	(void) fprintf(stderr, CATGETS(catd, _MSG_MAN_USAGESTR));
	catclose(catd);
	exit(2);
    }


    while ((o = getopt(argc, argv, "cdfkM:prtT:wW")) != -1) {
	switch (o) {
	case 'c':	/* 'cat' output to standard out (from BSD-Reno) */
	    catflag++;
	    filtercmd = NULL;
	    break;

	case 'd':	/* direct path */
	    directpathflag++;
	    parsePaths(".", manpaths);
	    break;

	case 'f':	/* whatis */
	    whatisflag++;
	    break;

	case 'k':	/* apropos */
	    aproposflag++;
	    break;

	case 'w':	/* whereis */
	    whereisflag++;
	    break;

	case 't':	/* troff it */
	    troffflag++;
	    ttyoutput = FALSE;
	    break;

	case 'p':	/* print but don't execute resulting command */
	    printcmdflag++;
	    break;

	case 'r':	/* use regular expression instead of glob style match */
	    useregexflag++;
	    break;

	case 'T':	/* use macro package */
	    macroflag++;
	    macropackage = optarg;
	    break;

	case 'M':	/* man path */
	    directpathflag = FALSE;
	    if (parsePaths(optarg, manpaths) == 0) {
		parsePaths(DEFAULTMANPATHS, manpaths);
	    }
	    break;

	case 'W':	/* create whatis database */
	    whatisdbflag++;
#ifdef TESTING
	    filtercmd = "./getcatNAME"; /* assuming internationalized version in current dir */
#else
	    filtercmd = "/usr/lib/getcatNAME";
#endif
	    break;

	case '?':
	    (void) fprintf(stderr, CATGETS(catd, _MSG_MAN_USAGESTR));
	    catclose(catd);
	    exit(2);
	}
    }
    
    CurLaunchList[0] = &LaunchList[0];
    CurLaunchList[1] = &LaunchList[1];

    if (aproposflag) {
	apropos(argc-optind, &argv[optind], manpaths);
	catclose(catd);
	exit(0);
    }

    if (whatisflag) {
	whatis(argc-optind, &argv[optind], manpaths);
	catclose(catd);
	exit(0);
    }

    /*
    //  Parse section number/name
    */
    if (argc-optind < 1) {
	(void) fprintf(stderr, CATGETS(catd, _MSG_MAN_USAGESTR));
	catclose(catd);
	exit(2);
    }
    section = "*";

    switch(argv[optind][0]) {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
	if (strlen(argv[optind]) <= 3) {
	    section = argv[optind];
	    optind++;
	}
	break;
    case 'D':
	if(!argv[optind][1] || (isdigit(argv[optind][1]) && !argv[optind][2])) {
		/* because there is only one section, but titles say D3, etc. */
	    section = "D";
	    optind++;
	}
	break;
    case 'l':	/* local */
	if (optind < argc && strcmp("local", argv[optind]) == 0) {
	    section = "l";
	    optind++;
	}
	break;
    case 'n':	/* new */
	if (optind < argc && strcmp("new", argv[optind]) == 0) {
	    section = "n";
	    optind++;
	}
	break;
    case 'o':	/* old */
	if (optind < argc && strcmp("old", argv[optind]) == 0) {
	    section = "o";
	    optind++;
	}
	break;
    case 'p':	/* public */
	if (optind < argc && strcmp("public", argv[optind]) == 0) {
	    section = "p";
	    optind++;
	}
	break;
    }

    /*
    //	Parse titles, keywords, and filenames
    */
    if (argc-optind < 1) {
	(void) fprintf(stderr, CATGETS(catd, _MSG_MAN_USAGESTR));
	catclose(catd);
	exit(2);
    }
    pagecount = 0;
    for (i=optind; i < argc; i++) {
	char 	matchname[PATH_MAX+1];
	char	anymatchname[PATH_MAX+1];
	char 	pattern[PATH_MAX+1];
	wchar_t wmatchname[PATH_MAX+1];
	wchar_t	wanymatchname[PATH_MAX+1];
	wchar_t	wargv[PATH_MAX+1];

	if (useregexflag) {
	    strcpy(matchname, argv[i]);

       if (I18N_SBCS_CODE)
	   strcopylower(anymatchname, argv[i], '\0');
       else
       {
	   mbstowcs(wargv, argv[i], strlen(argv[i])+1);
           wstrcopylower(wanymatchname, wargv, L'\0');
	   wcstombs(anymatchname, wanymatchname, PATH_MAX);
       }

	} else {
	    glob2regex(matchname, argv[i]);
	    if (!directpathflag)
	        if (I18N_SBCS_CODE)
		    strcopylower(anymatchname, matchname, '\0');
	        else
		{
	           mbstowcs(wmatchname, matchname, strlen(matchname)+1);
                   wstrcopylower(wanymatchname, wmatchname, L'\0');
	           wcstombs(anymatchname, wanymatchname, PATH_MAX);
		}

	    else
	    	anymatchname[0] = '\0';
	}
	isregexflag = isRegEx(matchname);
	found = FALSE;

        manregexpr=(regex_t *)malloc(sizeof(regex_t));
	if (manregexpr == NULL ) {
	    (void) fprintf(stderr, CATGETS(catd, _MSG_MAN_MALLOC_FAIL));
	    catclose(catd);
 	    exit(1);
        }
        anymanregexpr=(regex_t *)malloc(sizeof(regex_t));
	if (anymanregexpr == NULL ) {
	    (void) fprintf(stderr, CATGETS(catd, _MSG_MAN_MALLOC_FAIL));
	    catclose(catd);
 	    exit(1);
        }

	if (directpathflag) {
	    char* basename = pathbasename(matchname);
	    sprintf(pattern,"^%s$",basename);
	    if (regcomp(manregexpr, pattern, REG_NOSUB|REG_EXTENDED)) {
		(void) fprintf(stderr, CATGETS(catd, _MSG_MAN_BAD_PAGE), argv[i]);
		free(basename);
	        catclose(catd);
		exit(1);
	    }
	    free(basename);
	    basename = pathbasename(anymatchname); 
	    sprintf(pattern,"^%s$",basename);
	    if (regcomp(anymanregexpr, pattern, REG_NOSUB|REG_EXTENDED)) {
		(void) fprintf(stderr, CATGETS(catd, _MSG_MAN_BAD_PAGE), argv[i]);
		free(basename);
	        catclose(catd);
		exit(1);
	    }
	    free(basename);
	} else {
	    sprintf(pattern,"^%s%s",matchname,suf);
	    if (regcomp(manregexpr, pattern, REG_NOSUB|REG_EXTENDED)) {
		(void) fprintf(stderr, CATGETS(catd, _MSG_MAN_BAD_PAGE), argv[i]);
	        catclose(catd);
		exit(1);
	    }
	    sprintf(pattern,"^%s%s",anymatchname,suf);
	    if (regcomp(anymanregexpr, pattern, REG_NOSUB|REG_EXTENDED)) {
		(void) fprintf(stderr, CATGETS(catd, _MSG_MAN_BAD_PAGE), argv[i]);
	        catclose(catd);
		exit(1);
	    }
        }

	if (directpathflag) {
	    char* dirname = pathdirname(argv[i]);
	    if (dirname[0] == '\0') {
		findman(".", manregexpr, anymanregexpr, GUESS);
	    } else {
		findman(dirname, manregexpr, anymanregexpr, GUESS);
	    }
	    schedulelaunch();
	    free(dirname);
	} else
	if (*section == '*') {
	    for (sect=Allsections; *sect; sect++) {
		looksection(*sect, manregexpr, anymanregexpr, manpaths);
		schedulelaunch();
	    }
	} else {
	    looksection(section, manregexpr, anymanregexpr, manpaths);
	    schedulelaunch();
	}
	if (!found) {
	    waitonchildren();
	    (void) printf(CATGETS(catd, _MSG_MAN_NO_ENTRY), argv[i]);
	}
    }
    waitonchildren();

    catclose(catd);
    return(0);	/* exit */
}

int
sel(struct dirent *dp)
{
	if ((strncmp(dp->d_name, "cat", 3) == 0) ||
	    (strncmp(dp->d_name, "man", 3) == 0))
		return 1;
	return 0;
}

#define strcopy(s1,s2,s1end)		\
    {					\
        register char * s2end;		\
	s1end = s1; s2end = s2;		\
	while (*s1end++ = *s2end++) ; 	\
	s1end--;			\
    }

static void
looksection(char *section, regex_t * manregexpr, regex_t *anymanregexpr,
							char * manpaths[])
{
    register char **	mp;
    register char **	b;
    register char *	spmpend;
    register char *	spbend;
    register char *	sptend;
    char		searchpath[PATH_MAX+1];
    struct dirent *dp, **dlist;
    int			ptype, i, j, nent;
    int			*flist;
    wchar_t wd_name[BUFSIZ];

    for (mp=manpaths; *mp; mp++) {
	strcopy(searchpath, *mp, spmpend);
	*spmpend++ = '/';
	*spmpend = '\0';

	/*  Search SYS V style first */
	for (b=books; *b; b++) {
	    strcopy(spmpend, *b, spbend);
	    *spbend++ = '/';
	    *spbend = '\0';

	    if ((nent = scandir(searchpath, &dlist, sel, alphasort)) < 0)
		    continue;
	    if (nent == 0) {
		    free(dlist);
		    continue;
	    }
	    
	    flist = calloc(nent, sizeof(int));
	    for (i = 0; i < nent; i++) {
		    dp = dlist[i];
                    if (I18N_SBCS_CODE)
	            {
		        if ((strlen(dp->d_name) < (3 + strlen(section))) ||
			        strncmp(&dp->d_name[3], section, strlen(section)) != 0)
			        continue;
	            }
	            else
	            {
			wchar_t wsection[BUFSIZ];
                       
			mbstowcs(wd_name, dp->d_name, strlen(dp->d_name)+1);
			mbstowcs(wsection, section, strlen(section)+1);
		        if ((wcslen(wd_name) < (3 + wcslen(wsection))) ||
			        wcsncmp(&wd_name[3], wsection, wcslen(wsection)) != 0)
			        continue;
	            }
		    /*
		     * If this is a 'man' directory, scan for the
		     * corresponding 'cat' directory -
		     * if we already found the man page there
		     * then ignore this dir
		     * Since we sorted - 'cat' comes before 'man'
		     * Always search 'man' dirs if we're building databases
		     */
		    ptype = CAT;
		    if ((strncmp(dp->d_name, "man", 3) == 0) &&
				    !(isregexflag || whereisflag || whatisdbflag)) {
			    int ignore = 0;
			    for (j = 0; j < nent; j++) {
				    if (flist[j] && strcmp(&dp->d_name[3],
						    &dlist[j]->d_name[3]) == 0) {
					    ignore = 1;
					    break;
				    }
			    }
			    if (ignore)
				    continue;
			    ptype = MAN;
		    }
		    strcopy(spbend, dp->d_name, sptend);
		    if (findman(searchpath, manregexpr, anymanregexpr, ptype) != -1)
			    flist[i] = 1;
	    }
	    free(flist);
	    for (i = 0; i < nent; i++)
		    free(dlist[i]);
	    free(dlist);
	}

	/*  Search BSD style */
	strcopy(searchpath, *mp, spmpend);
	*spmpend++ = '/';
	*spmpend = '\0';
	if ((nent = scandir(searchpath, &dlist, sel, alphasort)) < 0)
		continue;
	if (nent == 0) {
		free(dlist);
		continue;
	}
	
	flist = calloc(nent, sizeof(int));
	for (i = 0; i < nent; i++) {
		dp = dlist[i];
		if(I18N_SBCS_CODE)
		{
		    if ((strlen(dp->d_name) < (3 + strlen(section))) ||
			    strncmp(&dp->d_name[3], section, strlen(section)) != 0)
			    continue;
		}
		else
		{
                    wchar_t wsection[BUFSIZ];

		    mbstowcs(wsection, section, strlen(section)+1);
		    mbstowcs(wd_name, dp->d_name, strlen(dp->d_name)+1);
                       
		    if ((wcslen(wd_name) < (3 + wcslen(wsection))) ||
			    wcsncmp(&wd_name[3], wsection, wcslen(wsection)) != 0)
			    continue;
		}
		/*
		 * If this is a 'man' directory, scan for the corresponding
		 * 'cat' directory - if we already found the man page there
		 * then ignore this dir
		 * Since we sorted - 'cat' comes before 'man'
		 * Always search 'man' dirs if we're building databases
		 */
		ptype = CAT;
		if ((strncmp(dp->d_name, "man", 3) == 0) &&
				!(isregexflag || whereisflag || whatisdbflag)) {
			int ignore = 0;
			for (j = 0; j < nent; j++) {
				if (flist[j] && strcmp(&dp->d_name[3],
						&dlist[j]->d_name[3]) == 0) {
					ignore = 1;
					break;
				}
			}
			if (ignore)
				continue;
			ptype = MAN;
		}
		strcopy(spmpend, dp->d_name, sptend);
	        if (findman(searchpath, manregexpr, anymanregexpr, ptype) != -1)
			flist[i] = 1;
	}
	free(flist);
	for (i = 0; i < nent; i++)
		free(dlist[i]);
	free(dlist);
    }
}

static unsigned int
isRegEx(const char * str)
{
    static char * regexpchars = ".+*^$(){}[]";
    static wchar_t wregexpchars[20] = L".+*^$(){}[]";
    wchar_t wstr[BUFSIZ];

    if (I18N_SBCS_CODE)
    {
        return (strcspn(str,regexpchars) != strlen(str));
    }
    else
    {
	mbstowcs(wstr, str, strlen(str)+1);
        return (wscspn(wstr,wregexpchars) != wcslen(wstr));
    }
}
	

static int
parsePaths(char * pathstr, char * patharray[])
{
    register int	count;
    register char *	ps;
    int  		addpath;
    char *		langname = setlocale(LC_MESSAGES, NULL);
   
    if ((langname == NULL) || !strcmp(langname, "C"))
        addpath = 0;
    else
        addpath = 1;
 
    count = 0;
    ps = pathstr;
    while(count < MANDIRCNT/2 && (patharray[count] = strtok(ps, ":"))) {
        if (addpath) {
            patharray[count + 1] = patharray[count];
            patharray[count] = malloc(strlen(patharray[count + 1]) + 
			       strlen(langname) + 2);
            sprintf(patharray[count], "%s/%s", patharray[count + 1], langname);
	    count++;
        }
	count++;
	if(count >= MANDIRCNT/2)
		fprintf(stderr, CATGETS(catd, _MSG_MAN_DIR_CNT),
			MANDIRCNT/2);
	ps = NULL;
    }
    return(count);
}

/******************************************************************************/
/* Purpose    : To copy a wide character string to other string in lower case */
/* Parameters : wstrout     - string in which to copy                         */
/*              wstrin      - string from which to copy                       */
/*              wterminator - a terminator for the input string               */
/* Return Value : pointer to output string is returned                        */
/******************************************************************************/

static wchar_t *
wstrcopylower(wchar_t * wstrout, wchar_t * wstrin, wchar_t wterminator)
{
    register wchar_t *	wstroutp = wstrout;
    register wchar_t *	wstrinp = wstrin;

	/* the first *wstrinp check is redundant when terminator==\0, but 
	 * is needed in case of trashed whatis files when terminator==\n;
	 * see bug 453603.
	*/
    for (; *wstrinp && *wstrinp != wterminator; wstroutp++, wstrinp++) {
	*wstroutp = (iswupper(*wstrinp)) ? towlower(*wstrinp) : *wstrinp ;
    }
    *wstroutp = L'\0';
    return(wstrout);
}

static char *
strcopylower(char * strout, char * strin, char terminator)
{
    register char *	stroutp = strout;
    register char *	strinp = strin;

	/* the first *strinp check is redundant when terminator==\0, but 
	 * is needed in case of trashed whatis files when terminator==\n;
	 * see bug 453603.
	*/
    for (; *strinp && *strinp != terminator; stroutp++, strinp++) {
	*stroutp = (isupper(*strinp)) ? _tolower(*strinp) : *strinp;
    }
    *stroutp = '\0';
    return(strout);
}

static int
findman(char * rootpath, regex_t * nameexpr, regex_t *anynameexpr, int pagetype)
{
    char		newpath[PATH_MAX+1];
    char		matchname[PATH_MAX+1];
    struct stat		filestats[1];
    register char *	rpend;
    register char *	npend;
    register DIR *	dirptr;
    register struct dirent *	direntptr;
    register int	foundcount = 0;
    register int	match;
    wchar_t		wmatchname[PATH_MAX+1];
    wchar_t		wd_name[PATH_MAX+1];
    
    if ((dirptr = opendir(rootpath)) == NULL) {
	return (-1);
    }

    while ((direntptr = readdir(dirptr)) != NULL) {
    	/*
	 * Try to match complete first, then match any.
	 */
        char *rcscheck;
	match = -1;
	strcpy(matchname, direntptr->d_name);
	if (regexec(nameexpr, matchname, 0, NULL, 0) == 0)
		match = 0;
	else if (useregexflag || !directpathflag) {
	    if(I18N_SBCS_CODE)
		(void) strcopylower(matchname, direntptr->d_name, '\0');
            else {
                mbstowcs(wd_name, direntptr->d_name, strlen(direntptr->d_name)+1);
		wstrcopylower(wmatchname, wd_name, L'\0');
		wcstombs(matchname, wmatchname, PATH_MAX+1);
            }
	        if (regexec(anynameexpr, matchname, 0, NULL, 0) == 0)
			match = 1;
	}

	if (match >= 0) {
	    	strcopy(newpath, rootpath, rpend);
		*rpend++ = '/';
		strcopy(rpend, direntptr->d_name, npend);

		/* Ignore . and .., and RCS files (...,v) */
		rcscheck = strrchr(direntptr->d_name, ',');
		if((rcscheck != NULL) && strcmp(rcscheck, ",v"))
			rcscheck = NULL;
	    
		if ((*direntptr->d_name != '.') && (rcscheck == NULL)) {
			if (whereisflag) {
			    (void) printf("%s\n", newpath);
			} else {
			    deferlaunch(newpath, match);
			}
		}
		found = TRUE;
		foundcount++;
	}

	/*
	//  If entry could be a subdirectory (doesn't have a period in its
	//  name), check if it is a directory and search it if it is.
	//  Avoid RCS directories to speed searches against man page
	//  source trees.
	*/
	if (!directpathflag && strchr(direntptr->d_name,'.') == NULL &&
	    strcmp(direntptr->d_name, "RCS")) {
	    strcopy(newpath, rootpath, rpend);
	    *rpend++ = '/';
	    strcopy(rpend, direntptr->d_name, npend);
	    stat(newpath, filestats);
	    if (filestats->st_mode & S_IFDIR) {
		findman(newpath, nameexpr, anynameexpr, pagetype);
	    }
	}
    }
    closedir(dirptr);

    return((foundcount) ? foundcount: -1);
}


static void
waitonchildren(void)
{
    auto int 		waitflag;

    for(;;) {
	if (wait(&waitflag) == -1) {
		if (errno == ECHILD)
			return;
	}
    }
#ifdef NEVER
    if (wait(&waitflag) == -1) {
	if (errno == ECHILD) {	/* If no children to wait on */
	    return;
	} else {
	    perror("wait");
	    catclose(catd);
	    exit(1);
	}
    }

    if(WIFSIGNALED(waitflag)){
	kill(0, waitflag.w_stopval);
    }
#endif
}

/* fork and exec to see if cmd exists in search path; designed to
 * work only with cmds that work as filters and work with no args
*/
forkandexeclp(char *cmd)
{
    int pid, wpid, i;
    wait_t status;
    char *args[11];

    if((pid=fork()) == -1)
	return 0;
    if(pid) {
	while((wpid = waitpid(pid, &status.w_status, 0)) != pid && wpid != -1)
		;
	if(wpid != pid)
	    return 0;	/* something's weird, act old way */
	if(status.w_termsig)
	     return 0;	/* strange; possibly resources; act old way */
	if((status.w_retcode & 0xff) == 0xff)
	     return -1;	/* it isn't there */
	/* any non-ff code take as success */
	return 1;
    }
    /* close fd 0; all of the cmds we are interested in work as
     * filters, reading from stdin, and work with no args;
     * close 1 also, in case, like *eqn output is generated immediately;
     * close 2 in case it generates usage messages.
     * it would be nice to use popen to handle strings with args,
     * but it is too hard to tell if exec fails; can't just use
     * first part of cmd, becauase cmds like psroff will happily
     * generate empty print jobs...; if more than 10 args, assume
     * they know what they are doing in specifying their old
     * filter and bomb out also */
    args[0] = strtok(cmd, " ");
    for(i=1; i<11 && (args[i]=strtok(NULL, " ")); i++)
	;
    if(i == 11)
    {
	catclose(catd);
	exit(0);
    }
    close(0);
    close(1);
    close(2);
    execvp(args[0], args);
    catclose(catd);
    exit(0xff);
    /*NOTREACHED*/
}


/* check to see if the formatting commands exist; also check for
 * tbl, and the appropriate *eqn command.  Too many customers just
 * won't read the man page...  I'm tired of seeing and answering this
 * question, as are most of support, I suspect.
 * return 1 on success, -1 on failure; 0 if fork fails, so we try
 * again on later man pages, if any; this essentially means that
 * the older style message about formatters "not found" message will
 * be printed.  Better than bogusly not trying and printing incorrect
 * no DWB message.  Normally called only once per roff cmd (i.e.,
 * once for nroff or troff, per invocation of the man cmd
*/
chkexist(char *roffcmd, char *eqncmd)
{
    int ret;
    if((ret = forkandexeclp(roffcmd)) <= 0)
	return ret;
    if((ret = forkandexeclp(eqncmd)) <= 0)
	return ret;
    if((ret = forkandexeclp("tbl")) <= 0)
	return ret;
    return 1;
}


static void
launch(char * name)
{
    register char	c;
    register unsigned	ft;
    register int	pid;
    char 		cmd[ARG_MAX];
    int skipit = 0;

    if ((ft = filetype(name)) == -1) return;

    switch (ft) {
    case CATFILE:
	(void) sprintf(cmd, "cat %s ", name);
	break;
    case PACKFILE:
	(void) sprintf(cmd, "pcat %s ", name);
	break;
    case COMPRESSFILE:
	(void) sprintf(cmd, "zcat %s ", name);
	break;
    case GZIPFILE:
    case GZIPHTML:
	(void) sprintf(cmd, "gzip -dc %s ", name);
	break;
    case NROFFFILE:
	if (troffflag) {
	    static chktfmtexist;
	     if(!chktfmtexist)
		    chktfmtexist = chkexist(troffcmd, "eqn");
	    if(chktfmtexist == -1)
		    skipit = 1;
	    (void)sprintf(cmd, "tbl %s | eqn /usr/pub/eqnchar - | %s -i %s",
		    name, troffcmd, macropackage);
	} else {
	    static chknfmtexist;
awfit:
		 if(manfmtcmd)
			(void)sprintf(cmd, "%s %s", manfmtcmd, name);
		 else {
			 if(!chknfmtexist) {
				if((chknfmtexist = chkexist(nroffcmd, "neqn")) == -1) {
					/* if no nroff, look for awf, and if it's there,
					 * use it for this and the other unformatted man pages.
					 * awf is Henry Spencer's awk based formatter */
					if((chknfmtexist = forkandexeclp(awfcmd)) != -1) {
						fprintf(stderr,CATGETS(catd, _MSG_MAN_TEXT_FORMATTER));
						manfmtcmd = awfcmd;
						goto awfit;
					}
				}
			}
			if(chknfmtexist == -1)
				skipit = 1;
			(void)sprintf(cmd, "tbl -TX %s | neqn /usr/pub/eqnchar - | %s -i %s ",
				name, nroffcmd, macropackage);
		}
	}
	break;
    case NOTREGULARFILE:
    default:
	fprintf(stderr, CATGETS(catd, _MSG_MAN_NOT_PRINTABLE), name);
	return;	 /* No reason to launch if we can't handle it */
    }

    if (!catflag && filtercmd && !(ft == NROFFFILE && troffflag)) {
	strcat(cmd, "| ");
	if (ft == GZIPHTML)
	    strcat(cmd, htmlfiltercmd);
	else
	    strcat(cmd, filtercmd);
    }

    if (ttyoutput && !(whatisdbflag || whereisflag || catflag)) {
	strcat(cmd, "| ");
	strcat(cmd, pager);
    } else if (troffflag) {
	strcat(cmd, "| ");
	if (ft == NROFFFILE) {
	    strcat(cmd, troffdisplay); 
	} else {
	    /*
	    ** If source wasn't ROFF format, it must be preprocessed nroff
	    ** output which a troffdisplay might not be able to handle like
	    ** in the case that troff is used instead of psroff so DVI output
	    ** is generated and sent to 'lpr -t'.
	    ** If /usr/lib/print/manprint exists, use it rather than lp to
	    ** do the printing.  This allows somewhat better translation
	    ** to the 'best' format for the printer.
	    */
	    if(!displayenv && forkandexeclp(manprinter) == 1)
		strcat(cmd, manprinter);
	    else
		strcat(cmd, nroffdisplay);
	}
    }

    /* Wait until previous man page has finished before we display next page */
    waitonchildren();

    if (ttyoutput && pagecount && !(whatisdbflag || whereisflag || catflag) &&
	    !printcmdflag) {
	sigset(SIGINT, resetexit);
	prompt(promptstr);

	set_tty();
	fflush(stdin);
	if (I18N_SBCS_CODE)
	{
	    do {
	        c = tolower(fgetc(stdin));
	    } while (c!=' ' && c!='\n' && c!=yesch && c!=noch && c!=quitch); 

	    reset_tty();
	    sigset(SIGINT, waitexit);
	    clearprompt();
	    if (c == noch || c == quitch) {
	        catclose(catd);
	        exit(0);
	    }
	}
        else
	{
	    wchar_t wc ;

	    do {
	        wc = towlower(fgetwc(stdin));
	    } while (wc!=L' ' && wc!=L'\n' && wc!=wcyes && wc!=wcno && wc!=wcquit); 

	    reset_tty();
	    sigset(SIGINT, waitexit);
	    clearprompt();
	    if (wc == wcno || wc == wcquit) {
	        catclose(catd);
	        exit(0);
	    }
        }
    }

    if (ft == NROFFFILE) {
	static nodwb = 0;
	if(skipit) {
	    if(!nodwb) {
		nodwb = 1;
		fprintf(stderr,CATGETS(catd, _MSG_MAN_NOT_INSTALLED));
	    }

	    if(!printcmdflag) {
		fprintf(stderr, CATGETS(catd, _MSG_MAN_SKIPPING), name);
		return; /* No reason to launch if we can't handle it */
	    }
	}
	if(ttyoutput && !printcmdflag) {
	    printf(CATGETS(catd, _MSG_MAN_REFORMATTING));
	    fflush(stdout);
	}
    }
    pid = fork();	/* Fork to permit one page look ahead */
    if(pid == 0) {
	/* Child process */
	    if (!printcmdflag) {
		(void) system(cmd);
	    } else {
		(void) puts(cmd);
	    }
	    catclose(catd);
	    exit(0); 
    } else if (pid < 0) {
	    perror("fork failed");
	    catclose(catd);
	    exit(1);
    }
}

/*
 * These routines are used to defer lanuch of a file based on the
 * exact or case-insensitive matching of the pathname.
 */
static void
deferlaunch(char *path, int match)
{
	struct launchlist *lp;
	
	lp = (struct launchlist *)malloc(sizeof(*lp));

	if (!lp)
		return;		/* arf! */
	lp->next = 0;
	lp->newpath = strdup(path);
	if (!lp->newpath) {
		free(lp);
		return;		/* arf! */
	}

	/*
	 * Move the pointer forward
	 */	
	CurLaunchList[match]->next = lp;
	CurLaunchList[match] = lp;
	return;
}

/*
 * Schedule the launch.
 */
static void
schedulelaunch(void)
{
	int match;
	struct launchlist *lp;
	struct launchlist *nlp;
	
	for (match = 0; match <= 1; match++) {
		for (lp = LaunchList[match].next; lp; lp = nlp) {
#ifdef notdef
			fprintf(stderr, CATGETS(catd, _MSG_MAN_LAUNCH), lp->newpath);
#endif
			launch(lp->newpath);
			pagecount++;
			nlp = lp->next;
			free(lp->newpath);
			free(lp);
		}
		LaunchList[match].next = 0;
		CurLaunchList[match] = &LaunchList[match];
	}
}

static void
helpwhatis(void)
{
	(void) fprintf(stderr, CATGETS(catd, _MSG_MAN_NO_DATABASE), WHATIS, WHATIS);
	catclose(catd);
	exit(1);
}

static void
apropos(int argc, char * argv[], char * manpaths[])
{
    register unsigned	i;
    char		buf[BUFSIZE];
    char		printbuf[BUFSIZE];
    char		file[PATH_MAX+1];
    regex_t *		aproposregexp;
    register char **	mp;
    int			foundwhatis = FALSE;

    if (argc == 0) {
	(void) fprintf(stderr, CATGETS(catd, _MSG_MAN_APROPOS_WHAT));
	catclose(catd);
	exit(1);
    }

    for (i=0; i < argc; i++) {
	char 	 matchname[PATH_MAX+1];
	wchar_t wmatchname[PATH_MAX+1];
	wchar_t wargv[BUFSIZ];

	if (useregexflag) {
	    if(I18N_SBCS_CODE)
	        strcopylower(matchname, argv[i], '\0');
            else
	    {
		mbstowcs(wargv, argv[i], strlen(argv[i])+1);
	        wstrcopylower(wmatchname, wargv, L'\0');
		wcstombs(matchname, wmatchname, 4096);
            }
	} else {
	    glob2regex(matchname, argv[i]);
	    if(I18N_SBCS_CODE)
	    {
	        strcopylower(matchname, matchname, '\0');
		    /* Make apropos expression case insensitive */
            }
	    else
	    {
		mbstowcs(wmatchname, matchname, strlen(matchname)+1);
	        wstrcopylower(wmatchname, wmatchname, '\0');
		wcstombs(matchname, wmatchname, 4096);
		    /* Make apropos expression case insensitive */
            }
	}
        aproposregexp=(regex_t *)malloc(sizeof(regex_t));
	if (aproposregexp == NULL ) {
	    (void) fprintf(stderr, CATGETS(catd, _MSG_MAN_MALLOC_FAIL));
	    catclose(catd);
 	    exit(1);
        }
	if (regcomp(aproposregexp, matchname, REG_NOSUB|REG_EXTENDED)) {
	    (void) fprintf(stderr, CATGETS(catd, _MSG_MAN_BAD_APR_EXPR), argv[i]);
	    catclose(catd);
	    exit(1);
	}

	for (mp=manpaths; *mp; mp++) {
	    (void) sprintf(file, "%s/%s", *mp, WHATIS);

	    if (freopen(file, "r", stdin) != NULL) {
	    
		while (fgets(printbuf, sizeof printbuf, stdin) != NULL) {
			/* only if we open it, and read at least one line; errors
			 * running makewhatis leave an empty file behind as a
			 * signal to not rerun it from /etc/init.d/configmsg */
		    wchar_t wbuf[BUFSIZ],wprintbuf[BUFSIZ];
		    if(!foundwhatis) foundwhatis = TRUE;
		    if(I18N_SBCS_CODE)
		        (void) strcopylower(buf, printbuf, '\n');
                    else
		    {
			mbstowcs(wprintbuf, printbuf, strlen(printbuf)+1);
		        wstrcopylower(wbuf, wprintbuf, '\n');
			wcstombs(buf, wbuf, BUFSIZ);
                    }
	            if (regexec(aproposregexp, buf, 0, NULL, 0) == 0) {
			printf("%s", printbuf);
			found = TRUE;
		    }
		}
	    }
	}

	if (!foundwhatis)
		helpwhatis();

	if (!found) {
	    (void) printf(CATGETS(catd, _MSG_MAN_NO_APR_ENTRY), argv[i]);
	}
    }
}


static void
whatis(int argc, char * argv[], char * manpaths[])
{
    register unsigned	i;
    char		buf[BUFSIZE];
    char		printbuf[BUFSIZE];
    char		file[PATH_MAX+1];
    regex_t *		whatisregexp;
    register char **	mp;
    int			foundwhatis = FALSE;

    if (argc == 0) {
	(void) fprintf(stderr, CATGETS(catd, _MSG_MAN_WHATIS_WHAT));
	catclose(catd);
	exit(1);
    }

    for (i=0; i < argc; i++) {
	char*	basename;
	char	matchname[PATH_MAX+1];
	char	pattern[PATH_MAX+1];
	wchar_t wmatchname[PATH_MAX+1];
	wchar_t wargv[BUFSIZ];
	if (useregexflag) {
	    if(I18N_SBCS_CODE)
	        strcopylower(matchname, argv[i], '\0');
            else
	    {
		mbstowcs(wargv, argv[i], strlen(argv[i])+1);
	        wstrcopylower(wmatchname, wargv, L'\0');
		wcstombs(matchname, wmatchname, 4096);
            }
	} else {
	    glob2regex(matchname, argv[i]);
	    if(I18N_SBCS_CODE)
	    {
	        strcopylower(matchname, matchname, '\0');
		    /* Make whatis expression case insensitive */
            }
	    else
	    {
		mbstowcs(wmatchname, matchname, strlen(matchname)+1);
	        wstrcopylower(wmatchname, wmatchname, '\0');
		wcstombs(matchname, wmatchname, 4096);
		    /* Make whatis expression case insensitive */
            }
	}
	basename = pathbasename(matchname);
	sprintf(pattern,"^(.*, )*%s",basename);
        whatisregexp=(regex_t *)malloc(sizeof(regex_t));
	if (whatisregexp == NULL ) {
	    (void) fprintf(stderr, CATGETS(catd, _MSG_MAN_MALLOC_FAIL));
	    catclose(catd);
 	    exit(1);
        }
	if (regcomp(whatisregexp, pattern, REG_NOSUB|REG_EXTENDED)) {
	    (void) fprintf(stderr, CATGETS(catd, _MSG_MAN_BAD_WHATIS_EXPR), argv[i]);
	    catclose(catd);
	    exit(1);
	}
	free(basename);

	for (mp=manpaths; *mp; mp++) {
	    (void) sprintf(file, "%s/%s", *mp, WHATIS);

	    if (freopen(file, "r", stdin) != NULL) {
		foundwhatis = TRUE;
	
		while (fgets(printbuf, sizeof printbuf, stdin) != NULL) {
		    wchar_t wbuf[BUFSIZ],wprintbuf[BUFSIZ];
		    if(I18N_SBCS_CODE)
		        (void) strcopylower(buf, printbuf, '\n');
                    else
		    {
			mbstowcs(wprintbuf, printbuf, strlen(printbuf)+1);
		        wstrcopylower(wbuf, wprintbuf, '\n');
			wcstombs(buf, wbuf, BUFSIZ);
                    }
	            if (regexec(whatisregexp, buf, 0, NULL, 0) == 0) {
			printf("%s", printbuf);
			found = TRUE;
		    }
		}
	    }
	}

	if (!foundwhatis)
		helpwhatis();

	if (!found) {
	    (void) printf(CATGETS(catd, _MSG_MAN_NO_APR_ENTRY), argv[i]);
	}
    }
}


#define	FBSZ		512

static unsigned int
filetype(char * file)
{
    union {
	short	magicnumber;	/* Guarantees short alignment of fbuf */
	char    fbuf[FBSZ];
    }		header;
    register int     	fbsz;
    register int	ifd;
    register unsigned int ft;
    register unsigned int i;
    struct stat		statbuf;

    ifd = open(file, O_RDONLY);
    if (ifd < 0) {
	perror(file);
	return(-1);
    }

    if (fstat(ifd, &statbuf) < 0) {
        perror(file);
	return(-1);
    }

    if (statbuf.st_mode & S_IFREG) {
	ft = NOTREGULARFILE;

	fbsz = read(ifd, header.fbuf, FBSZ);
	if (fbsz > 3) {
	    if (header.magicnumber == PACKMAGIC) {
		ft = PACKFILE;
	    } else
	    if (header.magicnumber == COMPRESSMAGIC) {
		ft = COMPRESSFILE;
	    } else
	    if (header.magicnumber == GZIPMAGIC) {
		if(strstr(header.fbuf+GZIPFILEOFFSET, ".html") != NULL)
		    ft = GZIPHTML;
		else
		    ft = GZIPFILE;
	    } else {
		/*
		** NROFF filetyping heuristic -- look for a line beginning
		** with what may be an NROFF command (regexp "^[.'][A-Za-z'\\]")
		*/
		for (i=0; i<fbsz; i++) {
		    if ((i == 0 || header.fbuf[i-1] == '\n')
			&& ((header.fbuf[i] == '.') || (header.fbuf[i] == '\''))
			&& (isalpha(header.fbuf[i+1]) ||
			    (header.fbuf[i+1] == '\\') ||
			    (header.fbuf[i+1] == '\'')
			)
		    ) {
			ft = NROFFFILE;
			break;
		    }
		}
	    }
	    if (ft == NOTREGULARFILE) {
		/*
		** If we haven't validated any other file type, try validating
		** an ASCII formatted CATFILE by making sure all of the
		** characters are printable.  This will reduce the chance that
		** things like executables get printed.
		*/
		int printable = TRUE;
		    if (I18N_SBCS_CODE)
		        for (i=0; i<fbsz && printable; i++) {
		            printable = isprint(header.fbuf[i])
				        || isspace(header.fbuf[i])
				        || header.fbuf[i] == '\b';
		        }
                    else
		    {
			wchar_t wc;

                        /* 
			   Here as 512 bytes are read the last character could have been half
			   read . So mbtowc will give error to that character . At tha it is
			   checked whether those are last MB_CUR_MAX bytes . If yes then the
			   string is assumed to be printable . Here read has been used to read 
			   from the file . It does not terminate the array with '\0' . So
			   mbtowc might or might not give an error . So it is terminated with
			   '\0' for multibyte portion . 
                        */

                        header.fbuf[fbsz-1]='\0';
		        for (i=0; i<fbsz-1 && printable; ) {
			    if((mbtowc(&wc, &header.fbuf[i], MB_CUR_MAX)) == -1)
			    {
				if (i<=(fbsz-MB_CUR_MAX))
				    printable=FALSE;
				break;
                            }
			    i+=mblen(&header.fbuf[i], MB_CUR_MAX);
		            printable = iswprint(wc)
				        || iswspace(wc)
				        || wc == L'\b';
		        }
		    }
		if (printable) {
		    ft = CATFILE;
		}
	    }
	}
    } else {
	ft = NOTREGULARFILE;
    }
    (void) close(ifd);

    return(ft);
}

#define stty(fd,argp)	ioctl(fd,TCSETAW,argp)
static struct termio 	otty, ntty;

static char * standoutmodeenter;
static char * standoutmodeexit;
static char * cleartoeol;
 
static void
initterm(void)
{
    register char *term = getenv("TERM");
    int setuperr;

    ttyoutput = isatty(fileno(stdout));

    if (ttyoutput) {
	ioctl(fileno(stdout), TCGETA, &otty);
	(void) setupterm (term, fileno(stdout), &setuperr);
    } else
	setuperr = 0;

    if (setuperr > 0) {
	reset_shell_mode();
	standoutmodeenter = tigetstr("smso");
	standoutmodeexit = tigetstr("rmso");
	cleartoeol = tigetstr("el");
    } else {
	standoutmodeenter = "";
	standoutmodeexit = "";
	cleartoeol = "";
    }	
}
    
static void
prompt (char *str)
{
    sighold(SIGINT);
    sighold(SIGTERM);
    sighold(SIGHUP);
    if (standoutmodeenter && standoutmodeexit) {
	putp(standoutmodeenter);
    }
    (void) printf(str);
    if (standoutmodeenter && standoutmodeexit) {
	putp(standoutmodeexit);
    }
    sigrelse(SIGINT);
    sigrelse(SIGTERM);
    sigrelse(SIGHUP);
}

static void
set_tty (void)  /* "cbreak" mode */
{
	ntty = otty;
	ntty.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL);
	ntty.c_lflag &= ~ICANON;
	ntty.c_lflag |= ISIG;
	ntty.c_cflag &= ~(CSIZE|PARENB);
	ntty.c_cflag |= CS8;
	ntty.c_iflag &= (ICRNL|ISTRIP);
	ntty.c_cc[VMIN] = ntty.c_cc[VTIME] = 1;
	stty(fileno(stderr), &ntty);
}

static void
reset_tty (void)
{
    stty(fileno(stderr), &otty);
}


static void
clearprompt(void)
{
    if (cleartoeol != '\0') {
        putchar('\r');
	putp(cleartoeol);
        fflush(stdout);
    }
}


static void
resetexit(void)
{
    reset_tty();
    clearprompt();
    waitexit();
}

static void
waitexit(void)
{
    while(wait(0) > 0 || errno == EINTR)
	;
    catclose(catd);
    exit(0);
}
