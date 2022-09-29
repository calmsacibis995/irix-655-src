/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uniq:uniq.c	1.4.1.2"
/*
 * Deal with duplicated lines in a file
 */

/* Modified to support EUC Multibyte/Big5-Sjis/Full multibyte */

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <locale.h>
#include <sgi_nl.h>
#include <msgs/uxsgicore.h>
#include <string.h>
#include <errno.h>
#include <sys/euc.h>
#include <i18n_capable.h>
#include <wchar.h>

int	fields = 0;
int	letters = 0;
int	linec;
char	mode = 0;
int	uniq;
char	*skip();

     /* Function Prototype */
void pline(register char buf[]);

main(argc, argv)
int argc;
char *argv[];
{
	static char b1[BUFSIZ], b2[BUFSIZ];
	FILE *temp = NULL;
	int keep_looping = 1;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxsgicore");
	(void)setlabel("UX:uniq");

	while(argc > 1 && keep_looping) {
		if (*argv[1] == '-') {
			switch(argv[1][1]) {
			    case 'c':
			    case 'd':
			    case 'u':
				    mode = argv[1][1];
				    break;
			    case 'f':
				    if (argv[2] && isdigit(argv[2][0])) {
					    fields = atoi(argv[2]);
				    } else {
					    _sgi_nl_error(SGINL_NOSYSERR,
							  "UX:uniq",
							  GETTXT(_MSG_MMX_uniq_missing_f_arg));
					    _sgi_nl_usage(SGINL_USAGE,
							  "UX:uniq",
							  GETTXT(_MSG_MMX_uniq_usage));
					    exit(1);
				    }
				    argc--;
				    argv++;
				    break;
			    case 's':
				    if (argv[2] && isdigit(argv[2][0])) {
					    letters = atoi(argv[2]);
				    } else {
					    _sgi_nl_error(SGINL_NOSYSERR,
							  "UX:uniq",
							  GETTXT(_MSG_MMX_uniq_missing_s_arg));
					    _sgi_nl_usage(SGINL_USAGE,
							  "UX:uniq",
							  GETTXT(_MSG_MMX_uniq_usage));
					    exit(1);
				    }
				    argc--;
				    argv++;
				    break;
			    case '-':
				    keep_looping = 0;
				    break;
			    case 0:	/* '-' alone */
				    temp = stdin;
				    break;
			    default:
				    if (isdigit(argv[1][1])) {
					    fields = atoi(&argv[1][1]);
				    } else {
					    wchar_t wTemp;
					    mbtowc(&wTemp, &argv[1][1], MB_CUR_MAX);
					    _sgi_nl_error(SGINL_NOSYSERR,
							  "UX:uniq",
							  GETTXT(_MSG_MMX_illoption),
							  wTemp);
					    _sgi_nl_usage(SGINL_USAGE,
							  "UX:uniq",
							  GETTXT(_MSG_MMX_uniq_usage));
					    exit(1);
				    }
			}
			argc--;
			argv++;
		} else if (*argv[1] == '+') {
			letters = atoi(&argv[1][1]);
			argc--;
			argv++;
		} else {
			break;
		}
	}

	if (!temp && argc > 1) {
		if ((temp = fopen(argv[1], "r")) == NULL) {
			_sgi_nl_error(SGINL_SYSERR2, "UX:uniq",
				      GETTXT(_MSG_MMX_CannotOpen),
				      argv[1]);
			exit(1);
		} else {
			fclose(temp);
			freopen(argv[1], "r", stdin);
		}
		argc--;
		argv++;
	}

	if (argc > 1 && freopen(argv[1], "w", stdout) == NULL) {
		_sgi_nl_error(SGINL_SYSERR2, "UX:uniq",
			      GETTXT(_MSG_MMX_CannotCreat),
			      argv[1]);
		exit(1);
	}

	if(gline(b1))
		exit(0);
	for(;;) {
		linec++;
		if(gline(b2)) {
			pline(b1);
			exit(0);
		}
		if(!equal(b1, b2)) {
			pline(b1);
			linec = 0;
			do {
				linec++;
				if(gline(b1)) {
					pline(b2);
					exit(0);
				}
			} while(equal(b1, b2));
			pline(b2);
			linec = 0;
		}
	}
}

gline(buf)
register char buf[];
{
	register c, nBytes = 1, nLen = -1;
	char *pTemp = buf;

	while((c = getchar()) != '\n') {
		if(c == EOF)
			return(1);
		if(nBytes++ < BUFSIZ)
			*buf++ = c;
	}
	*buf = 0;
	if (!I18N_SBCS_CODE)	{
		for(; ((nLen = mblen(pTemp, MB_CUR_MAX)) >= 0) 
			&& (pTemp < buf); pTemp += nLen);
		*pTemp = '\0';
	}
	return(0);
}

void pline(buf)
register char buf[];
{

	switch(mode) {

	case 'u':
		if(uniq) {
			uniq = 0;
			return;
		}
		break;

	case 'd':
		if(uniq) break;
		return;

	case 'c':
		printf("%4d ", linec);
	}
	uniq = 0;
	fputs(buf, stdout);
	putchar('\n');
}

equal(b1, b2)
register char b1[], b2[];
{
	register char c;

	b1 = skip(b1);
	b2 = skip(b2);
	while((c = *b1++) != 0)
		if(c != *b2++) return(0);
	if(*b2 != 0)
		return(0);
	uniq++;
	return(1);
}

char *
skip(s)
register char *s;
{
	register nf, nl;
	register int i, j, nLen = -1;

	nf = nl = 0;
	while(nf++ < fields) {
		while(isspace(*s))
			s++;
		while( !(isspace(*s) || *s == 0) )
			s++;
	}
	while(nl < letters && *s != 0) 
	{
	     if (I18N_SBCS_CODE)  {
			nl++;
			s++;
	     }
	     else  {
		nl++;
		s += mblen(s, MB_CUR_MAX);
	    }
	}
	return(s);
}
