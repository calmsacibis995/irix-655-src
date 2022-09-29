/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)wc:wc.c	1.5"	*/
#ident	"$Header: /proj/irix6.5f/isms/eoe/cmd/wc/RCS/wc.c,v 1.18 1998/09/18 19:47:22 sherwood Exp $"
/*
**	wc -- word and line count
*/

#include	<stdio.h>
#include	<stdlib.h>	/* for i18n */
#include	<wctype.h>	/* for i18n */
#include	<ctype.h>	/* for isgraph def */
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<string.h>
#include	<limits.h>	/* for i18n */
#include	<msgs/uxcore.h>
#include	<i18n_capable.h>

#define		OPTBUFSIZ	20

char		b[BUFSIZ];
int		mbBUFLMT;

FILE *fptr = stdin;
long long	wordct;
long long	twordct=0;
long long	linect;
long long	tlinect=0;
long long	charct;
long long	tcharct=0;
long long	bytect;
long long	tbytect=0;

extern	int errno;
extern	char *sys_errlist[];

#define STR_CLONE(A, B)	{ if (A) free(A); A = strdup(B); }

void wcp(char *, long long, long long, long long, long long);
void nomem(void);

int
main(
	int	argc,
	char	**argv
	)
{
	char	*p1, *p2;
	unsigned int c;
	int	i, token;
	int	status = 0;
	char	*wd;	/* this string contains all wc options */
	char	*begin_wd, *tmp_wd;
	char	buffer[OPTBUFSIZ];	/* to store options */
	int	index;
	int	charlength;	/* i18n */
	wchar_t	wchar_c;	/* i18n */
	wchar_t	*pwc;
	long long	char_index;
	int	byte_cnt;
	int	begin_wdsz;
	int	mb_offset;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:wc");

	/* should put this AFTER setlocale() */
	mbBUFLMT = sizeof(b) - MB_CUR_MAX;

#ifdef DEBUG_WORDC
	if ( mbBUFLMT < MB_CUR_MAX )
		printf("too small size of buffer: b\n");
#endif
	wd = strdup("lwc");	/* default is: newlines, words, bytes */
	if(!wd) nomem();

	/* case where there is only one "grouped" option like -wc, -wlc etc.. */
	/* or case where there are separate options */
	index = 1;
	if ( (argc>=2) && (*argv[index]=='-') ) {
		STR_CLONE(wd, "");
		if(!wd) nomem();
		begin_wd = wd;
		begin_wdsz = (strlen(wd) + 1);
		while (index < argc) {	
	    		if (*argv[index]=='-') {
				++argv[index];
				if (*argv[index]!='-') {
					/*
					 *  See if this string will overflow
					 *  the current malloc'ed string space.
					 *  If so, then calloc another string
					 *  space which will be able to hold
					 *  the old string plus the new one and
					 *  then copy the old and concatenate
					 *  the new one.
					 */
					if((strlen(argv[index]) + 
						strlen(begin_wd)) > 
							(begin_wdsz - 1))
					{
						tmp_wd =
						  (char *)calloc((size_t)1,
						  (size_t)(begin_wdsz + 
						  strlen(argv[index]) + 2));

						if(!tmp_wd) {
							free(begin_wd);
							nomem();
						}
						begin_wdsz = (begin_wdsz + 
						    strlen(argv[index]) + 2);
						wd = tmp_wd;
						strcat(tmp_wd , begin_wd);
						free(begin_wd); /* free old */
						begin_wd = wd;
					}
					strcat(wd , argv[index]);
					wd = begin_wd;
				}
				else {	/* -- terminates the options */
					argc-=index;
					argv+=index;
					if (!strlen(wd)) {
						STR_CLONE(wd, "lwc");
					}
					if(!wd) nomem();
					begin_wdsz = (strlen(wd) + 1);
					begin_wd = wd;
					break; 
				}
	    		}
			if ( (index+1) < argc ) {
	    			if (*argv[index+1]!='-') {
					argc-=index;
					argv+=index; 
					break;
	    			}
	    			else index++;
			}
			else {
				argc-=index;
				argv+=index;
				break;
			}
		}
	}

	/* usr strstr to find what options are in wd and reorder them */
	/* (the "newlines, words, chars/bytes" order always holds) */
	if (strlen(wd) < OPTBUFSIZ) {
		strcpy(buffer, wd);
		STR_CLONE(wd, "lwc");
		if(!wd) nomem();
		*wd = *(wd+1) = *(wd+2) = 0;

		/* buffer contains only option letters, but not MB chars such as file name */
		if (strstr(buffer,"l"))  strcat(wd,"l");
		if (strstr(buffer,"w"))  strcat(wd,"w");
		if (strstr(buffer,"c"))  strcat(wd,"c");/* c or m, not both */
		else if (strstr(buffer,"m"))  strcat(wd,"m");
	}
	else STR_CLONE(wd, "lwc");
	
	if(!wd) nomem();

	i = 1;
	do { /* for each file given as argument */
		if(argc>1 && (fptr=fopen(argv[i], "r")) == NULL) {
			pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_CANNOT_OPEN),
				argv[i], strerror(errno));
			status = 2;
			continue;
		}
		p1 = p2 = b; /* beginning of buffer */
		linect = 0;
		wordct = 0;
		charct = 0;
		token = 0;
		bytect = 0;
		mb_offset = 0;

		for(;;) {	/* for each character */

			if( p1 >= p2 || mb_offset ) { /* done for each page buffer */
				p1 = b;
				if ( feof(fptr) && !mb_offset )	/* EOF */
					break;
				c = fread( p1 + mb_offset, 1, BUFSIZ - mb_offset, fptr );
				if ( c <= 0 && !mb_offset )
					break;
				bytect += (c > 0)? c : 0;
				p2 = p1 + mb_offset + c;	/* end of buffer */
				mb_offset = 0;
			}

			if ( !(I18N_SBCS_CODE) ) {	/* MBCS CODE (should include EUC) */
 
				byte_cnt = mbtowc(&wchar_c, p1, (size_t) MB_CUR_MAX);
				if (byte_cnt <= 0) {
					if ( b + mbBUFLMT <= p1  &&  p1 <= p2 ) {
						mb_offset = p2 - p1;
						memcpy( b, p1, mb_offset ); /* mbBUFLMT must be > MB_CUR_MAX */
					}
					else {	/* if you need consistency with SBCS locale, 
						   it can count charct when byte_cnt == 0. 
						*/
						p1++;		/* skip the NULL */
					}
				}
				else {	charct ++;
					p1 += byte_cnt; /* move to the next char*/
					if(iswgraph(wchar_c)) {
						if(!token) {
							wordct++;
							token++;
						}
						continue;
					}
					if(wchar_c == L'\n')
						linect++;
					else if(!iswspace(wchar_c) && 
							wchar_c != L'\t')
						continue;
					token = 0;
				}
			}
			else { 	c = *p1++;
				charct ++;
				if(isgraph(c)) {
					if(!token) {
						wordct++;
						token++;
					}
					continue;
                        	}
				if(c == '\n') linect++;
				else if(!isspace(c)) continue;
				token = 0;
			}
		} /* "for each character" loop */
#ifdef sgi
		if (ferror(fptr)) {
			if (argc > 1)
				pfmt(stderr, MM_WARNING, PFMTTXT(_MSG_READ_FAILED_WITH_ARGV),
					argv[i], sys_errlist[errno]);
			else
				pfmt(stderr, MM_WARNING, PFMTTXT(_MSG_READ_FAILED),
					sys_errlist[errno]);
			fclose(fptr);
			status = 2;
			continue;
		}
#endif

		/* print lines, words, chars */
		wcp(wd, bytect, charct, wordct, linect);
		if(argc>1) {
			printf(" %s\n", argv[i]);
		}
		else
			printf("\n");
		fclose(fptr);
		tlinect += linect;
		twordct += wordct;
		tcharct += charct;
		tbytect += bytect;
	} while(++i<argc);
	if(argc > 2) {
		wcp(wd, tbytect, tcharct, twordct, tlinect);
		pfmt(stdout, MM_NOSTD, PFMTTXT(_MSG_WC_TOTAL));
	}
	if(wd) free(wd);
	exit(status);
}


void
wcp(
	char		*wd,
	long long	bytect,
	long long	charct,
	long long	wordct,
	long long	linect
	)
{
	char	*wdp = wd;

	while(*wdp) {
	switch(*wdp++) {
		case 'l':
			printf("%14lld", linect);
			break;

		case 'w':
			printf("%14lld", wordct);
			break;

		case 'c':
			printf("%14lld", bytect);
			break;

		case 'm':
			printf("%14lld", charct);
			break;

		default:
			pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_INCORRECT_USAGE));
			pfmt(stderr, MM_ACTION,PFMTTXT(_MSG_WC_USAGE));
			exit(2);
		}
	}
}

void
nomem()
{
	fprintf(stderr, "wc: ");
	pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_OUT_OF_MEMORY));
	exit(2);
}
