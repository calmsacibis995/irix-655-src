/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)paste:paste.c	1.4.1.2"	*/
#ident	"$Revision: 1.9 $"
#
/* paste: concatenate corresponding lines of each file in parallel. Release 1.4 */
/*	(-s option: serial concatenation like old (127's) paste command */
# include <stdio.h>	/* make :  cc paste.c  */
# include <locale.h>
# include <pfmt.h>
# include <string.h>
# include <errno.h>
# include <unistd.h>
# include <i18n_capable.h>
# include <msgs/uxdfm.h>
# define WRUB  L'\177'

# define MAXOPNF 12  	/* maximal no. of open files (not with -s option) */
# define MAXLINE BUFSIZ  	/* maximal line length */
#define RUB  '\177'
	char del[MAXLINE] = {"\t"};

static const char badopen[] = PFMTTXT(_MSG_CANNOT_OPEN);
static int exitcode = 0;

main(argc, argv)
int argc;
char ** argv;
{
	int i, j, k, eofcount, nfiles, maxline, glue;
	int delcount = { 1 } ;
	int onefile  = { 0 } ;
	register int c ;
	char outbuf[MAXLINE], l, t;
	register char *p;
	FILE *inptr[MAXOPNF];

 	int delw;
	int mdelc;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:paste");


	maxline = MAXLINE -2;
 
	while((c = getopt(argc, argv, ":sd:")) != EOF)
	  switch(c){
		case 's': 
		  onefile++;
		  break;
		case 'd': 
		  if (I18N_SBCS_CODE)
		     delcount = move(optarg, &del[0]);
		  else
		     delcount = wmove(optarg, &del[0]);
		  break;
		case ':':
		  pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_NO_DELIM));
		  usage();		  
		case '?':
		  pfmt(stderr, MM_ERROR,
		       PFMTTXT(_MSG_ILLEGAL_OPT_C), c);
		  usage();
	  }/* end options */

	argc -= optind;
	argv = &argv[optind];
 
	if ( ! onefile) {	/* not -s option: parallel line merging */
		nfiles = 0;
		for (i = 0; argc >0 && i < MAXOPNF; i++) {
			if (!strcmp(argv[i], "-")) {
				inptr[nfiles] = stdin;
			} else inptr[nfiles] = fopen(argv[i], "r");
			if (inptr[nfiles] == NULL) {
				diag(badopen, argv[i], strerror(errno));
				exit(1);
			}
			else nfiles++;
			argc--;
		}
		if (argc > 0) diag(PFMTTXT(_MSG_TOO_MANY_FILES), MAXOPNF);
  
		do {
			p = &outbuf[0];
			eofcount = 0;
			j = k = 0;
			for (i = 0; i < nfiles; i++) {
				while((c = getc(inptr[i])) != '\n' && c != EOF)   {
					if (++j <= maxline) *p++ = c ;
					else {
					if ( !(I18N_SBCS_CODE)) {
						/* For MultiByte Code Sets			    */
						/* The approach used in this portion should be used */
						/* carefully for locales other than C,EUC,SJIS,BIG5 */
						int counter = 0,len;
						*p = 0;
						p--;
						for (; (len=mblen(p, MB_CUR_MAX)) < 0; p--);
						p += len;
					}
					diag(PFMTTXT(_MSG_LINE_LONG));
					}
				}
				if ( (l = del[k]) != RUB) *p++ = l;
				if ( !(I18N_SBCS_CODE))  {
					delw = mblen( &del[k], MB_CUR_MAX );
					if (delw > 1)	{
						mdelc = delw;
						while (--mdelc)
							*p++ = del[++k];
					}
				}
				k = (k + 1) % delcount;
				if( c == EOF) eofcount++;
			}
			if (l != RUB) {
				if ( !(I18N_SBCS_CODE))
				   	p -= (--delw);
				*--p = '\n';
			} else
				*p = '\n';
			*++p = 0;
			if (eofcount < nfiles) fputs(outbuf, stdout);
		}while (eofcount < nfiles);
  
	} else {	/* -s option: serial file pasting (old 127 paste command) */
		k = 0;
		t = 0;
		for (i = 0; i < argc; i++) {
			p = &outbuf[0];
			glue = 0;
			j = 0;
			if (argv[i][0] == '-') {
				inptr[0] = stdin;
			} else inptr[0] = fopen(argv[i], "r");
			if (inptr[0] == NULL) {
				diag(badopen, argv[i], strerror(errno));
				continue;
			}
	  
			while((c = getc(inptr[0])) != EOF)   {
				if (j >= maxline) {
					t = *--p;
					*++p = 0;
					fputs(outbuf, stdout);
					p = &outbuf[0];
					j = 0;
				}
				if (glue) {
					glue = 0;
					l = del[k];
					if (l != RUB) {
						*p++ = l ;
						t = l ;
						j++;
						if ( !(I18N_SBCS_CODE))  {
							delw = mblen( &del[k], MB_CUR_MAX );
							if (delw > 1)  {
								mdelc = delw;
								while (--mdelc) {
									*p++ = del[++k];
									j++;
								}
							}
						}
					}
					k = (k + 1) % delcount;
				}
				if(c != '\n') {
					*p++ = c;
					t = c;
					j++;
				} else glue++;
			}
			if (t != '\n') {
				*p++ = '\n';
				j++;
			}
			if (j > 0) {
				*p = 0;
				fputs(outbuf, stdout);
			}
		}
	}
	exit(exitcode);
}

diag(s,a1, a2)
char *s,*a1, *a2;
{
	pfmt(stderr, MM_ERROR, s, a1, a2);
	exitcode = 1;
}

/******************************************************************************/
/*									      */
/* Purpose : The function copies characters from a given data area to another */
/*	     and returns the characters transferred. It treats "\0", "\t",    */
/*	     "\n" as single characters '\0', '\t' & '\n' .		      */
/* Parameters : The pointer to the data area from where the characters are to */
/* 	        be copied and the pointer to the data area to which they are  */
/*	 	to be copied.					              */
/* Return Values : No of characters transferred. 			      */
/*                 The data area to which the characters are to be copied     */
/*		   contains the copied characters. 		      	      */
/*									      */
/******************************************************************************/

wmove(from, to)
char *from, *to;
{
	int i, len = strlen(from) + 1;
	wchar_t wc, wto, *wptr, wfrom[MAXLINE];
	char *to_begin ;

	to_begin = to;
	mbstowcs( wfrom, from, len );

	wptr = wfrom;
	i = 0;
	do {
		wc = *wptr++;
		if (wc != L'\\') wto = wc;
		else { 
			wc = *wptr++;
			switch (wc) {
				case L'0' : wto = WRUB;
						break;
				case L't' : wto = L'\t';
						break;
				case L'n' : wto = L'\n';
						break;
				default   : wto = wc;
						break;
			}
		}
		to += wctomb( to, wto );
	} while (wc != L'\0');
	i=strlen(to_begin);
	return(i);
}
/***************************end of wmove()*************************************/

move(from, to)
char *from, *to;
{
int c, i;
	i = 0;
	do {
		c = *from++;
		i++;
		if (c != '\\') *to++ = c;
		else { c = *from++;
			switch (c) {
				case '0' : *to++ = RUB;
						break;
				case 't' : *to++ = '\t';
						break;
				case 'n' : *to++ = '\n';
						break;
				default  : *to++ = c;
						break;
			}
		}
	} while (c) ;
return(--i);
}


usage()
{
	pfmt(stderr, MM_ACTION,
		PFMTTXT(_MSG_PASTE_USAGE));
	exit(1);
}
