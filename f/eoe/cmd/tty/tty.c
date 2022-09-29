/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)tty:tty.c	1.3"	*/
#ident	"$Header: /proj/irix6.5f/isms/eoe/cmd/tty/RCS/tty.c,v 1.5 1998/09/18 19:47:22 sherwood Exp $"

/*
** Type tty name
*/

#include	<stdio.h>
#include	<sys/stermio.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<msgs/uxue.abi.h>

char	*ttyname();

extern int	optind;
int		lflg;
int		sflg;

main(argc, argv)
char **argv;
{
	register char *p;
	register int	i;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxue.abi");
	(void)setlabel("UX:tty");

	while((i = getopt(argc, argv, "ls")) != EOF)
		switch(i) {
		case 'l':
			lflg = 1;
			break;
		case 's':
			sflg = 1;
			break;
		case '?':
			/* I18NCAT_PGM_MSG */
			pfmt(stderr, MM_ACTION, PFMTTXT(_MSG_TTY_USAGE));
			exit(2);
		}
	p = ttyname(0);
	if(!sflg){
		if (p)
			puts(p);
		else
			/* I18NCAT_PGM_MSG */
			pfmt(stdout, MM_NOSTD, PFMTTXT(_MSG_TTY_NOT_A_TTY));
	}
	if(lflg) {
		if((i = ioctl(0, STWLINE, 0)) == -1)
			/* I18NCAT_PGM_MSG */
			pfmt(stdout, MM_NOSTD, 
				PFMTTXT(_MSG_TTY_NOT_ON_AN_ACTIVE_SYNC_LINE));
		else
			/* I18NCAT_PGM_MSG */
			pfmt(stdout, MM_NOSTD, PFMTTXT(_MSG_TTY_SYNCHRONOUS_LINE), i);
	}
	exit(p? 0: 1);
}
