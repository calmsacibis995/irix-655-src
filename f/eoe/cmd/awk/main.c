/*
 *	$Id: main.c,v 1.16 1998/09/18 19:47:22 sherwood Exp $
 */

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)awk:main.c	2.15"

/***********************I18N File Header******************************
File                  : main.c

Compatibility Options : No support/Improper support/ EUC single byte/
                        EUC multibyte/Sjis-Big5/Full multibyte/Unicode

Old Compatibility     : EUC single byte/EUC Multibyte/ Big5-Sjis

Present Compatibility : EUC single byte/EUC Multibyte/ Big5-Sjis

Type of Application (Normal/Important/Critical) : Critical 

Optimization Level (EUC & Single byte/Single byte/Not optimized)
                      : Not optimized

Change History        : 14 July 1997         HCL
			cataloging modifications are done. 
Change History        : 16 Sept 1997         HCL
			Use of 'getwidth()' function is removed.
Change History        :	25th October 1997    HCL
		        Commented out 'XPG4' related code which invokes
		        'pawk' command if environment variable '_XPG' is
			set to '1'.
Change History        :	13 Jan 1998    HCL
        * VSC BUG FIX : for failed assertion#(25) on
            Baseline version, in VSC .
************************End of I18N File Header**********************/

#define DEBUG
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <locale.h>
#include <sys/euc.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>
#include <i18n_capable.h>
#include <msgs/uxawk.h>
#include <getwidth.h>
eucwidth_t WW;

#define	CMDCLASS	"UX:"	/* Command classification */

/* SGI CHANGE: I presume we want the locale stuff */
#include <locale.h>

#include "awk.h"
#include "y.tab.h"

char	*version;

int	dbg	= 0;
uchar	*cmdname;	/* gets argv[0] for error messages */
extern	FILE	*yyin;	/* lex input file */
uchar	*lexprog;	/* points to program argument if it exists */
extern	int errorflag;	/* non-zero if any syntax errors; set by yyerror */
int	compile_time = 2;	/* for error printing: */
				/* 2 = cmdline, 1 = compile, 0 = running */

uchar	*pfile[20];	/* program filenames from -f's */
int	npfile = 0;	/* number of filenames */
int	curpfile = 0;	/* current filename */

extern const char badopen[];
char *getenv(), *xpg;

main(argc, argv, envp)
	int argc;
	uchar *argv[], *envp[];
{
	uchar *fs = NULL;
	char label[MAXLABEL+1];	/* Space for the catalogue label */
	extern void fpecatch();

        if( (xpg = getenv("_XPG")) != NULL && atoi(xpg) > 0) {
                execve("/usr/bin/pawk", argv, envp);
		fprintf(stderr,GETTXT(_MSG_NAWK_GOTXPG));
                exit(1);
        }


	/* I18NCAT_SETLOCALE */
	SETLOCALE(LC_ALL, "");
	{getwidth(&WW); WW._eucw2++; WW._eucw3++;}
	cmdname = ((cmdname = (uchar*) strrchr ((char*) argv[0], '/')) ?
		++cmdname : argv[0]);
	(void)strcpy(label, CMDCLASS);
	(void)strncat(label, (char*) cmdname, (MAXLABEL - sizeof(CMDCLASS) - 1));
	(void)setcat("uxawk");
	(void)setlabel(label);
	/* I18NCAT_PGM_MSG */
	version = GETTXT(_MSG_NAWK_VER_OCT_11_1989);
 	if (argc == 1) {
		/* I18NCAT_PGM_MSG */
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_NAWK_INCORRECT_USAGE));
		pfmt(stderr, MM_ACTION,
		   	PFMTTXT(_MSG_NAWK_USAGE_MSG),
			cmdname);
		exit(1);
	}
	signal(SIGFPE, fpecatch);
	yyin = NULL;
	syminit();
	while (argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		if (strcmp((char*) argv[1], "--") == 0) {	/* explicit end of args */
			argc--;
			argv++;
			break;
		}
		switch (argv[1][1]) {
		case 'f':	/* next argument is program filename */
			argc--;
			argv++;
			if (argc <= 1)
				/* I18NCAT_PGM_MSG */
				error(MM_ERROR,CATNUM_DEFMSG(_MSG_NAWK_NO_PROG_FILE_NAME));
			pfile[npfile++] = argv[1];
			break;
		case 'F':	/* set field separator */
			if (argv[1][2] != 0) {	/* arg is -Fsomething */
				if (argv[1][2] == 't' && argv[1][3] == 0)	/* wart: t=>\t */
					fs = (uchar *) "\t";
				else if (argv[1][2] != 0)
					fs = &argv[1][2];
			} else {		/* arg is -F something */
				argc--; argv++;
				if (argc > 1 && argv[1][0] == 't' && argv[1][1] == 0)	/* wart: t=>\t */
					fs = (uchar *) "\t";
				else if (argc > 1 && argv[1][0] != 0)
					fs = &argv[1][0];
			}
			if (fs == NULL || *fs == '\0')
				/* I18NCAT_PGM_MSG */
				error(MM_WARNING, CATNUM_DEFMSG(_MSG_NAWK_FS_EMPTY));
			break;
		case 'v':	/* -v a=1 to be done NOW.  one -v for each */
			if (argv[1][2] == '\0' && --argc > 1 && isclvar((++argv)[1]))
				setclvar(argv[1]);
			break;
		case 'd':
			dbg = atoi(&argv[1][2]);
			if (dbg == 0)
				dbg = 1;
			pfmt(stdout, (MM_INFO | MM_NOGET), "%s %s\n",
				cmdname, version);
			break;
		default:
			/* I18NCAT_PGM_MSG */
			pfmt(stderr, MM_WARNING,
				PFMTTXT(_MSG_NAWK_UNKNOWN_OPTION),argv[1]);
			break;
		}
		argc--;
		argv++;
	}
	/* argv[1] is now the first argument */
	if (npfile == 0) {	/* no -f; first argument is program */
		if (argc <= 1)
			/* I18NCAT_PGM_MSG */
			error(MM_ERROR,CATNUM_DEFMSG(_MSG_NAWK_NO_PROG_GIVEN));
		dprintf( ("program = |%s|\n", argv[1]) );
		lexprog = argv[1];
		argc--;
		argv++;
	}
	compile_time = 1;
	argv[0] = cmdname;	/* put prog name at front of arglist */
	dprintf( ("argc=%d, argv[0]=%s\n", argc, argv[0]) );

	/* SGI HACK: Lots of programs assume that nawk performs the
	 * leading set of variable initializations before it starts
	 * parsing the program (probably because the old nawk did this).
	 * The Awk book implies this behavior is wrong, but the book 
	 * is kind of ambiguous about exactly when the first file is
	 * opened, and a number of our scripts assume that the initialization
	 * has occurred before the BEGIN clause is executed.
	 */
	while (argc > 1) {
		if (!isclvar(argv[1]))
			break;
		setclvar(argv[1]);
		argc--;
		argv++;
	}
	argv[0] = cmdname;

	arginit(argc, argv);
	envinit(envp);
	yyparse();
	if (fs)
		*FS = tostring(qstring(fs, '\0'));
	dprintf( ("errorflag=%d\n", errorflag) );
	if (errorflag == 0) {
		compile_time = 0;
		run(winner);
	} else
		bracecheck();
	exit(errorflag);
}

pgetc()		/* get program character */
{
	int c;

	for (;;) {
		if (yyin == NULL) {
			if (curpfile >= npfile)
				return EOF;
			if ((yyin = fopen((char *) pfile[curpfile], "r")) == NULL)
				error(MM_ERROR, badopen,
					pfile[curpfile], strerror(errno));
		}
		if ((c = getc(yyin)) != EOF)
			return c;
		yyin = NULL;
		curpfile++;
	}
}
