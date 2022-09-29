/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nl:nl.c	1.28.1.3"
/*	NLSID
*/
/*	Regular Expression handling changed by using		*/
/*	regcomp() family of regular expression matching		*/
/*	functions instead of compile() and step().		*/

/*	Modified to handle illegal character sequence in input	*/
/*	file generated in another locale.			*/

#include <stdio.h>	/* Include Standard Header File */
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>

#include <regex.h>
#include <limits.h>
#include <i18n_capable.h>
#include <widec.h>
#include <wchar.h>
#include <msgs/uxdfm.h>

#define EXPSIZ		512
#define	MAXWIDTH	100	/* max value used with '-w' option */


#ifdef u370
	int nbra, sed;	/* u370 - not used in nl.c, but extern in regexp.h */
#endif
	int width = 6;	/* Declare default width of number */
	char nbuf[MAXWIDTH + 1];	/* Declare bufsize used in convert/pad/cnt routines */
	char delim1 = '\\'; char delim2 = ':';	/* Default delimiters. */
	wchar_t wdelim1 = L'\\'; wchar_t wdelim2 = L':'; /*Default delimiters*/
	char pad = ' ';	/* Declare the default pad for numbers */
	char *s;	/* Declare the temp array for args */
	char s1[EXPSIZ];	/* Declare the conversion array */
	char format = 'n';	/* Declare the format of numbers to be rt just */
	int q = 2;	/* Initialize arg pointer to drop 1st 2 chars */
	int k;	/* Declare var for return of convert */
	int r;	/* Declare the arg array ptr for string args */

main(argc,argv)
int argc;
char *argv[];
{
	register char *p;
	wchar_t wp[BUFSIZ];
	char 	 mbTempBuf[BUFSIZ];
	char	 mbStoreTrunc[MB_LEN_MAX];
	char	 *pTruncChar;
	int	 nKeepFlg = 0;
	int 	 nLineFlg = 0;
	int 	 errcode;
	size_t	 errlen;
	char 	 *errstring;
	regex_t  hpreg, bpreg, fpreg;
	register char header = 'n';
	register char body = 't';
	register char footer = 'n';
	char line[BUFSIZ];
	char tempchr;	/* Temporary holding variable. */
	char swtch = 'n';
	char cntck = 'n';
	char type;
	int cnt;	/* line counter */
	int pass1 = 1;	/* First pass flag. 1=pass1, 0=additional passes. */
	char sep[EXPSIZ];
	char pat[EXPSIZ];
	int startcnt=1;
	int increment=1;
	int blank=1;
	int blankctr = 0;
	int c,lnt;
	char last;
	FILE *iptr=stdin;
	FILE *optr=stdout;
	sep[0] = '\t';
	sep[1] = '\0';

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:nl");

/*		DO WHILE THERE IS AN ARGUMENT
		CHECK ARG, SET IF GOOD, ERR IF BAD	*/
	
	while((c = getopt(argc, argv, "b:f:d:h:i:l:n:ps:v:w:")) != EOF)
	  switch(c){
		case 'h':
		  switch(*optarg){
			case 'n':
			  header = 'n';
			  break;
			case 't':
			  header = 't';
			  break;
			case 'a':
			  header = 'a';
			  break;
			  case 'p':
			  s=optarg;
			  q=1;
			    r=0;
			  while (s[q] != '\0'){
				  pat[r] = s[q];
				  r++;
				  q++;
			  }
			    pat[r] = '\0';
			  header = 'h';
			  if (pat[0] == '\0')
				regerr(GETTXT(_MSG_NL_NULL_REG_EXP));
			  errcode = regcomp( &hpreg, pat, REG_NOSUB | REG_NEWLINE); 
			  if (errcode)	{
				errlen = regerror(errcode, &hpreg, (char *) 0, (size_t) 0);
				errstring = (char *) malloc(errlen * sizeof(char));
				regerror(errcode, &hpreg, errstring, errlen);
			      	regerr(errstring);
			  }
			  break;
			default:
			    optmsg(optarg, GETTXT(_MSG_HEADER_ERROR));
		  }
		  break;
		  
		case 'b':
		  switch(*optarg){
			case 't':
			  body = 't';
			  break;
			case 'a':
			  body = 'a';
			  break;
			case 'n':
			  body = 'n';
			  break;
			case 'p':
			  s=optarg;
			  q=1;
			  r=0;
			  while (s[q] != '\0'){
				  pat[r] = s[q];
				  r++;
				  q++;
			  }
			  pat[r] = '\0';
			  body = 'b';
			  if (pat[0] == '\0')
				regerr(GETTXT(_MSG_NL_NULL_REG_EXP));
		  	  errcode = regcomp( &bpreg, pat, REG_NOSUB | REG_NEWLINE); 
		    	  if (errcode)	{
				errlen = regerror(errcode, &bpreg, (char *) 0, (size_t) 0);
				errstring = (char *) malloc(errlen * sizeof(char));
				regerror(errcode, &bpreg, errstring, errlen);
		      		regerr(errstring);
			  }
			  break;
			default:
			    optmsg(optarg, GETTXT(_MSG_BODY_ERROR));
		  }
		  break;

		case 'f':
		  switch(*optarg){
			case 'n':
			  footer = 'n';
			  break;
			case 't':
			  footer = 't';
			  break;
			case 'a':
			  footer = 'a';
			  break;
			case 'p':
			  s=optarg;
			  q=1;
			  r=0;
			  while (s[q] != '\0'){
				  pat[r] = s[q];
				  r++;
				  q++;
			  }
			  pat[r] = '\0';
			  footer = 'f';
			  if (pat[0] == '\0')
				regerr(GETTXT(_MSG_NL_NULL_REG_EXP));
			  errcode = regcomp( &fpreg, pat, REG_NOSUB | REG_NEWLINE); 
			  if (errcode)	{
				errlen = regerror(errcode, &fpreg, (char *) 0, (size_t) 0);
				errstring = (char *) malloc(errlen * sizeof(char));
				regerror(errcode, &fpreg, errstring, errlen);
			      	regerr(errstring);
			  }
			  break;
			default:
			    optmsg(optarg, GETTXTCAT(_MSG_FOOTER_ERROR));
		  }
		  break;
		  
		case 'p':
		  cntck = 'y';
		  break;
		case 'v':
		  startcnt = convert(optarg);
		  break;
		case 'i':
		  increment = convert(optarg);
		  break;
		case 'w':
		  width = convert(optarg);
		  if (width > MAXWIDTH)
		    width = MAXWIDTH;
		  break;
		case 'l':
		  blank = convert(optarg);
		  break;
		case 'n':
		  switch (*optarg) {
			case 'l':
			  if (optarg[1] == 'n')
			    format = 'l';
			  else
			    optmsg(& optarg[1], "");
			  break;
			case 'r':
			  if ( optarg[1] == 'n' || optarg[1] == 'z')
			    format = optarg[1];
			  else
			    optmsg(& optarg[1], "");
			  break;
			default:
			  optmsg(optarg, "");
			  break;
		  }
		  break;
		case 's':
		  s = optarg;
		  q = 0;
		  r = 0;
		  if(s != NULL)
		    while (s[q] != '\0') {

			    sep[r] = s[q];
			    r++;
			    q++;
		    }
		  sep[r] = '\0';
		  break;
		case 'd':
				/* I18NCAT_MB_WCHAR */
		  if (I18N_SBCS_CODE)	{
		  	tempchr = *optarg;
		  	delim1 = tempchr;
		  	tempchr = optarg[1];
		  	if(tempchr == '\0')break;
		  	delim2 = tempchr;
		  	if(optarg[2] != '\0')optmsg(& optarg[2],"");
		  }
		  else	{
			wchar_t woptarg[3];	/* Two Delimiter characters + One NULL character */

		  	mbstowcs( woptarg, optarg, 3 );
		  	wdelim1 = woptarg[0];
		  	if(woptarg[1] == L'\0') break;
		  	wdelim2 = woptarg[1];
		  	if(woptarg[2] != L'\0')  {
				char mbOpt[MB_LEN_MAX];
				int nCharLen;

				nCharLen = wctomb( mbOpt, woptarg[2] );
				mbOpt[nCharLen] = '\0';
				optmsg( mbOpt, "");
			}
		  }
		  break;
		default:
		{
		  char mb[2];
		  mb[0] = c; mb[1] = '\0';
		  optmsg( mb , "");
		}
	  }

	argc -= optind;
	argv = &argv[optind];
	
	/* only one file may be named */
	if(argc > 1){
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_ONLY_ONEFILE));
		exit(1);
	}

	/* Use stdin as input if filename not specified. */
	if(argc < 1 || !strcmp(argv[0], "-"))
	  iptr = stdin;
	else if ((iptr = fopen(argv[0],"r")) == NULL)  {
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_CANNOT_OPEN),
		     argv[0], strerror(errno));
		exit(1);
	}

	/* ON FIRST PASS ONLY, SET LINE COUNTER (cnt) = startcnt &
		SET DEFAULT BODY TYPE TO NUMBER ALL LINES.	*/
	if(pass1){
		cnt = startcnt; type = body; last = 'b'; pass1 = 0;
	}

/*		DO WHILE THERE IS INPUT
		CHECK TO SEE IF LINE IS NUMBERED,
		IF SO, CALCULATE NUM, PRINT NUM,
		THEN OUTPUT SEPERATOR CHAR AND LINE	*/

	while (( p = fgets(line,sizeof(line),iptr)) != NULL) {
	     if (I18N_SBCS_CODE)	{
		if (p[0] == delim1 && p[1] == delim2) {
			if (p[2] == delim1 && p[3] == delim2 && p[4]==delim1 
			    && p[5]==delim2 && p[6] == '\n') {
				if ( cntck != 'y')
				  cnt = startcnt;
				type = header;
				last = 'h';
				swtch = 'y';
			}
			else {
				if (p[2] == delim1 && p[3] == delim2 && 
				    p[4] == '\n') {
					if ( cntck != 'y' && last != 'h')
					  cnt = startcnt;
					type = body;
					last = 'b';
					swtch = 'y';
				}
				else if (p[2] == '\n') {
						if ( cntck != 'y' && last == 'f')
						  cnt = startcnt;
						type = footer;
						last = 'f';
						swtch = 'y';
				     }
			}
		}
	     }	 /* End of I18N_SBCS_CODE */
	     else	{
		if (nKeepFlg)	{
			    /* Processing for Truncated MB chars in lines longer than BUFSIZ bytes */
			int nLen = strlen(mbStoreTrunc); 
			memcpy( mbTempBuf, mbStoreTrunc, nLen );
			memcpy( &mbTempBuf[nLen], p, strlen(p)+1 );
			p = mbTempBuf;
			nKeepFlg = 0;
		}
		if ( ((mbstowcs(wp, p, strlen(p)+1)) == (size_t) -1 ) && (errno == EILSEQ))   {
			int nLen = 0, nBytes = 0, nChars = -1;

			    /* Processing for Truncated MB chars in lines 
			     * i)  longer than BUFSIZ bytes, or
			     * ii) containing illegal character sequence from another locale */
			pTruncChar = p;
			do  {
				pTruncChar += nLen;
				nChars++;
				nBytes += nLen;
				nLen = mblen(pTruncChar, MB_CUR_MAX);
			}  while (nLen > 0);

			if( (BUFSIZ - nBytes) >= MB_CUR_MAX)  {
				pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_NL_ILLEGAL_CHAR_SEQ));
				exit(1);
			}
			   /* pTruncChar points to bytes to save */
			memcpy(mbStoreTrunc, pTruncChar, strlen(pTruncChar)+1);
			nKeepFlg = 1;
			wp[nChars] = L'\0';
		}
	
		if (wp[0] == wdelim1 && wp[1] == wdelim2) {
			if (wp[2] == wdelim1 && wp[3] == wdelim2 && 
			   wp[4]==wdelim1 && wp[5]==wdelim2 && wp[6] == L'\n') {
				if ( cntck != 'y')
				  cnt = startcnt;
				type = header;
				last = 'h';
				swtch = 'y';
			}
			else {
				if (wp[2] == wdelim1 && wp[3] == wdelim2 && 
				    wp[4] == L'\n') {
					if ( cntck != 'y' && last != 'h')
					  cnt = startcnt;
					type = body;
					last = 'b';
					swtch = 'y';
				}
				else if (wp[2] == L'\n') {
						if ( cntck != 'y' && last == 'f')
						     cnt = startcnt;
						type = footer;
						last = 'f';
						swtch = 'y';
				     }
			}
		}
	     }
	        if (I18N_SBCS_CODE)	{
		  if (p[0] != '\n'){
			lnt = strlen(p);
			if(p[lnt-1] == '\n')
			  p[lnt-1] = NULL;
			else
			   nLineFlg = 1;
		  }
	        }
	        else  {
		  if (wp[0] != L'\n'){
			lnt = wcslen(wp);
			if(wp[lnt-1] == L'\n')
			  wp[lnt-1] = L'\0';
			else
			   nLineFlg = 1;
		  }
		}
		
		if (swtch == 'y') {
			swtch = 'n';
			fprintf(optr,"\n");
		}
		else {
			switch(type) {
			      case 'n':
				npad(width,sep);
				break;
			      case 't':
				if (I18N_SBCS_CODE) {
				   if (p[0] != '\n' && printable(line)) {
					pnum(cnt,sep);
					cnt+=increment;
				   }
				   else {
					npad(width,sep);
				   }
				}
				else  {
				   if (wp[0] != L'\n' && wprintable(wp)) {
					pnum(cnt,sep);
					cnt+=increment;
				   }
				   else npad(width,sep);
				}
				break;
			      case 'a':
				if (I18N_SBCS_CODE)  {
				   if (p[0] == '\n') {
					blankctr++;
					if (blank == blankctr) {
						blankctr = 0;
						pnum(cnt,sep);
						cnt+=increment;
					}
					else npad(width,sep);
				   }
				   else {
					blankctr = 0;
					pnum(cnt,sep);
					cnt+=increment;
				   }
				}
				else  {
				   if (wp[0] == L'\n') {
					blankctr++;
					if (blank == blankctr) {
						blankctr = 0;
						pnum(cnt,sep);
						cnt+=increment;
					}
					else npad(width,sep);
				   }
				   else {
					blankctr = 0;
					pnum(cnt,sep);
					cnt+=increment;
				   }
				}
				break;
			      case 'b':
				if (!regexec(&bpreg, p, (size_t) 0, NULL, 0)) {
					pnum(cnt,sep);
					cnt+=increment;
				}
				else {
					npad(width,sep);
				}
				break;
			      case 'h':
				if (!regexec(&hpreg, p, (size_t) 0, NULL, 0)) {
					pnum(cnt,sep);
					cnt+=increment;
				}
				else {
					npad(width,sep);
				}
				break;
			      case 'f':
				if (!regexec(&fpreg, p, (size_t) 0, NULL, 0)) {
					pnum(cnt,sep);
					cnt+=increment;
				}
				else {
					npad(width,sep);
				}
				break;
			}
			if (I18N_SBCS_CODE)  {
				if (p[0] != '\n' && !nLineFlg)
				  p[lnt-1] = '\n';
			}
			else {
				if (wp[0] != L'\n' && !nLineFlg)
				  wp[lnt-1] = L'\n';
			   	wcstombs(p, wp, (lnt+1) * MB_CUR_MAX );
			}
			fprintf(optr,"%s",p);
			if (nLineFlg)
				fprintf(optr, "\n");
		}
	}	/* Closing brace of "while". */
	fclose(iptr);
}

/* check whether a data area contains any printable wide-chars or not     */                            
/*  Parameter : The pointer to the data area containing the wide-chars.   */
/*  Return    : 1 if data area contains any printable chars, 0 otherwise. */
wprintable(ws)
wchar_t *ws;
{
	for (; *ws!= L'\0'; ws++)
	  if(iswprint(*ws))
	    return(1);
	return(0);
}

/* check if there is any printable character in this line */
printable(s)
char *s;
{
	for (;*s != '\0';s++)
	  if(isprint(*s))
	    return(1);
	return(0);
}

/*		REGEXP ERR ROUTINE		*/

regerr(c)
char *c;
{
pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_REGEXP_ERROR),c);
exit(1);
}

/*		CALCULATE NUMBER ROUTINE	*/

pnum(n,sep)
int	n;
char *	sep;
{
	register int	i;

	if (format == 'z') {
		pad = '0';
	}
	for ( i = 0; i < width; i++)
	  nbuf[i] = pad;
	num(n,width - 1);
	if (format == 'l') {
		while (nbuf[0]==' ') {
			for ( i = 0; i < width; i++)
			  nbuf[i] = nbuf[i+1];
			nbuf[width-1] = ' ';
		}
	}
	printf("%s%s",nbuf,sep);
}

/*		IF NUM > 10, THEN USE THIS CALCULATE ROUTINE		*/

num(v,p)
int v,p;
{
	if (v < 10)
		nbuf[p] = v + '0' ;
	else {
		nbuf[p] = (v % 10) + '0' ;
		if (p>0) num(v / 10,p - 1);
	}
}

/*		CONVERT ARG STRINGS TO STRING ARRAYS	*/

convert(argv)
char **argv;
{
	s = (char*)argv;
	q=0;
	r=0;
	while (s[q] != '\0') {
		if (s[q] >= '0' && s[q] <= '9')
		{
		s1[r] = s[q];
		r++;
		q++;
		}
		else
				{
				optmsg(& s[q], "");
				}
	}
	s1[r] = '\0';
	k = atoi(s1);
	return(k);
}

/*		CALCULATE NUM/TEXT SEPRATOR		*/

npad(width,sep)
	int	width;
	char *	sep;
{
	register int i;

	pad = ' ';
	for ( i = 0; i < width; i++)
		nbuf[i] = pad;
	printf("%s",nbuf);

	for(i=0; i < (int) strlen(sep); i++)
		printf(" ");
}
/* ------------------------------------------------------------- */
optmsg(option, whence)
char *option;
char *whence;
{
        if ( I18N_SBCS_CODE )
	    option[1] = '\0';
	else
	    option[mblen(option,MB_CUR_MAX)] = '\0';

        pfmt(stderr, MM_ERROR,
		PFMTTXT(_MSG_NL_ILLEGAL_OPT), whence, option);
	exit(1);
}
