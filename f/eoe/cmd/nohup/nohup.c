/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)nohup:nohup.c	1.5" */
#ident	"$Header: /proj/irix6.5f/isms/eoe/cmd/nohup/RCS/nohup.c,v 1.7 1998/09/18 19:47:22 sherwood Exp $"

/* Modified to support EUC multibyte/Sjis-Big5/Full multibyte	*/

#include	<stdio.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<string.h>
#include	<signal.h>
#include	<sys/stat.h>
#include	<msgs/uxue.h>

char	nout[100] = "nohup.out";
char	*getenv();

static char badopen[] = PFMTTXT(_MSG_CANNOT_OPEN_FORMAT);

main(argc, argv)
char **argv;
{
	char	*home;
	FILE *temp;
	int	err;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxue");
	(void)setlabel("UX:nohup");

	if(argc < 2) {
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_INCORRECT_USAGE));
		pfmt(stderr, MM_ACTION, PFMTTXT(_MSG_NOHUP_USAGE));
		exit(127);
	}
	if(strcmp(argv[1],"--")==0) {
		++argv;
		--argc;
	}
	argv[argc] = 0;
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	if(isatty(1)) {
		if( (temp = fopen(nout, "a")) == NULL) {
			if((home=getenv("HOME")) == NULL) {
				pfmt(stderr, MM_ERROR, badopen, nout,
					strerror(errno));
				exit(127);
			}
			strcpy(nout,home);
			strcat(nout,"/nohup.out");
			if(freopen(nout, "a", stdout) == NULL) {
				pfmt(stderr, MM_ERROR, badopen, nout,
					strerror(errno));
				exit(127);
			}
		}
		else {
			fclose(temp);
			freopen(nout, "a", stdout);
		}
		pfmt(stderr, MM_INFO, PFMTTXT(_MSG_SENDING_OUTPUT_TO_NOUT_FILE), nout);
	}
	chmod(nout,S_IRUSR|S_IWUSR);
	if(isatty(2)) {
		close(2);
		dup(1);
	}
	execvp(argv[1], &argv[1]);
	err = errno;

	/* It failed, so print an error */
	freopen("/dev/tty", "w", stderr);
	pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_ERR_MSG_FOR_EXECVP), argv[1], strerror(err));
	exit(err == ENOENT ? 127 : 126);
}

