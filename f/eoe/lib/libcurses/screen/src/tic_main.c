/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/tic_main.c	1.12"
/*********************************************************************
*                         COPYRIGHT NOTICE                           *
**********************************************************************
*        This software is copyright (C) 1982 by Pavel Curtis         *
*                                                                    *
*        Permission is granted to reproduce and distribute           *
*        this file by any means so long as no fee is charged         *
*        above a nominal handling fee and so long as this            *
*        notice is always included in the copies.                    *
*                                                                    *
*        Other rights are reserved except as explicitly granted      *
*        by written permission of the author.                        *
*                Pavel Curtis                                        *
*                Computer Science Dept.                              *
*                405 Upson Hall                                      *
*                Cornell University                                  *
*                Ithaca, NY 14853                                    *
*                                                                    *
*                Ph- (607) 256-4934                                  *
*                                                                    *
*                Pavel.Cornell@Udel-Relay   (ARPAnet)                *
*                decvax!cornell!pavel       (UUCPnet)                *
*********************************************************************/

/*
 *	comp_main.c --- Main program for terminfo compiler
 *
 *  $Log: tic_main.c,v $
 *  Revision 1.5  1998/09/18 19:47:22  sherwood
 *  SJIS feature for 6.5.2f
 *
 *  Revision 1.7  1998/09/16 02:31:55  rkm
 *  i18n message cleanup
 *
 *  Revision 1.5  1998/01/30 02:42:28  ktill
 *  merged HCL changes
 *
 *  Revision 1.4.18.2  1998/01/09 06:07:49  himanshu
 *  modified for comment ending '/' in file header.
 *
 * Revision 1.4.18.1  1997/10/31  08:46:01  scm
 * I18N changes for SJIS/BIG5 support.
 *
 * Revision 1.4.18.2  1997/10/04  19:22:03  rajkr
 * New Coding and Cataloguing Changes
 *
 * Revision 1.4  1993/09/09  00:03:01  igehy
 * Converted to 64-bit.
 *
 * Revision 1.3  1993/08/04  22:29:34  wtk
 * Fullwarn of libcurses
 *
 * Revision 1.2  1991/12/09  16:33:40  daveh
 * Merge in symlink capability from IRIX,
 *
 * Revision 2.1  82/10/25  14:45:37 pavel
 * Added Copyright Notice
 *
 * Revision 2.0  82/10/24  15:16:37  pavel
 * Beta-one Test Release
 *
 * Revision 1.3  82/08/23  22:29:36  pavel
 * The REAL Alpha-one Release Version
 *
 * Revision 1.2  82/08/19  19:09:49  pavel
 * Alpha Test Release One
 *
 * Revision 1.1  82/08/12  18:36:55  pavel
 * Initial revision
 *
 *
 */


#define EXTERN		/* EXTERN=extern in other .c files */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include "curses_inc.h"
#include "compiler.h"


#include <locale.h>
#include <nl_types.h>
#include <i18n_capable.h>
#include <msgs/uxeoe.h>
nl_catd catd;

static 	void init(void);

char	*source_file = "./terminfo.src";
char	*destination = SRCDIR;
char	check_only = 0;
char	*progname;

#ifdef _SGI_SOURCE
int symlinks;
#endif
char	*usage_string = NULL; 


main (int argc, char *argv[])
{
	int	i;
	int	argflag = FALSE;

	(void) setlocale(LC_ALL, "");

	catd = catopen("uxeoe", 0);

#ifdef _SGI_SOURCE
	usage_string = CATGETS(catd, _MSG_TIC_USAGE_SGI);
#else
	usage_string = CATGETS(catd, _MSG_TIC_USAGE);
#endif	/* If sgi source */


	debug_level = 0;
	progname = argv[0];

	umask(022);

	for (i=1; i < argc; i++)
	{
	    if (argv[i][0] == '-')
	    {
		switch (argv[i][1])
		{
		    case 'c':
			check_only = 1;
			break;

		    case 'v':
			debug_level = argv[i][2]  ?  atoi(&argv[i][2])  :  1;
			break;
#ifdef _SGI_SOURCE
		    case 's':
			/* use symbolic links instead of hard ones */
			symlinks = 1;
			break;
#endif
		    default:
			fprintf(stderr,
				CATGETS(catd, _MSG_TIC_UNKNOWN_OPT),
				argv[0], progname, usage_string);
			catclose(catd);
			exit(1);
		}
	    }
	    else if (argflag)
	    {
		fprintf(stderr, 
			CATGETS(catd, _MSG_TIC_TOOMANYFILES),
			argv[0], usage_string);
		catclose(catd);
		exit(1);
	    }
	    else
	    {
		argflag = TRUE;
		source_file = argv[i];
	    }
	}

	init();
	make_hash_table();
	compile();

	catclose(catd);

	exit(0);
}

/*
 *	init()
 *
 *	Miscellaneous initializations
 *
 *	Open source file as standard input
 *	Change directory to working terminfo directory.
 *
 */

static void
init(void)
{
	char		*env = getenv("TERMINFO");

	start_time = time((long *) 0);

	curr_line = 0;

	if (freopen(source_file, "r", stdin) == NULL)
	{
	    fprintf(stderr, 
		    CATGETS(catd, _MSG_TIC_CANT_OPEN), 
		    progname, source_file);
		catclose(catd);
	    exit(1);
	}

	if (env && *env)
	    destination = env;
	if (check_only) {
		DEBUG(1,CATGETS(catd, _MSG_TIC_WWD),destination);
	} else {
		DEBUG(1,CATGETS(catd, _MSG_TIC_WORKING_IN),
		      destination);
	}

	if (access(destination, 7) < 0)
	{
	    fprintf(stderr, 
		    CATGETS(catd, _MSG_TIC_PERM_DENIED),
		    progname, destination);
		catclose(catd);
	    exit(1);
	}

	if (chdir(destination) < 0)
	{
	    fprintf(stderr, 
		    CATGETS(catd, _MSG_TIC_NOTDIR), 
		    progname, destination);
		catclose(catd);
	    exit(1);
	}

}

/*
 *
 *	check_dir(dirletter)
 *
 *	Check for access rights to the destination directory.
 *	Create any directories which don't exist.
 *
 */

void
check_dir(char dirletter)
{
	struct stat	statbuf;
	static char	dirnames[128];
	static char	dir[2] = " ";

	if (dirnames[dirletter] == 0)
	{
	    dir[0] = dirletter;
	    if (stat(dir, &statbuf) < 0)
	    {
		if (mkdir(dir, 0755) < 0)
		    syserr_abort(CATGETS(catd, _MSG_TIC_BADMKDIR), dir);
		dirnames[dirletter] = 1;
	    }
	    else if (access(dir, 7) < 0)
	    {
		fprintf(stderr,	CATGETS(catd, _MSG_TIC_PERM_DENIED2),
			progname, destination, dir);
					
		perror(dir);
		catclose(catd);
		exit(1);
	    }
	    else if ((statbuf.st_mode & S_IFMT) != S_IFDIR)
	    {
		fprintf(stderr,	
			CATGETS(catd, _MSG_TIC_NOTDIR),
			progname, destination, dir);
					
		perror(dir);
		catclose(catd);
		exit(1);
	    }
	}
}


/*
 * Date: 24 Sep 1997 (i18n by HCL)
 * Creating problem in IRIX 6.3 environment 
 * The function mkdir() is already defined in libc.
 * It must not be redefined here.
 * This function has been commented out using the #if 0 directive.
 */
#if 0
#include <curses.h>
#if (defined(SYSV) || defined(USG)) && !defined(SIGPOLL)
/*
 *	mkdir(dirname, mode)
 *
 *	forks and execs the mkdir program to create the given directory
 *
 */

int
mkdir(dirname, mode)
#ifdef __STDC__
const
#endif
char	*dirname;
int mode;
{
    int	fork_rtn;
    int	status;

    fork_rtn = fork();

    switch (fork_rtn)
	{
	case 0:		/* Child */
	    (void) execl("/bin/mkdir", "mkdir", dirname, (char*)0);
	    _exit(1);

	case -1:	/* Error */
	    fprintf(stderr, CATGETS(catd, _MSG_TIC_FORKFAILED),
				progname);
	    exit(1);

	default:
	    (void) wait(&status);
	    if ((status != 0) || (chmod(dirname, mode) == -1))
		return -1;
	    return 0;
	}
}
#endif
#endif
