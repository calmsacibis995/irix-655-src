/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)sed:sed0.c	1.10"	*/
#ident	"$Header: /proj/irix6.5f/isms/eoe/cmd/sed/RCS/sed0.c,v 1.15 1998/10/28 22:42:44 danc Exp $"

#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <limits.h>
#include "sed.h"
#include <errno.h>
#include <widec.h>
#include <locale.h>
#include <nl_types.h>
#include <msgs/uxeoe.h>
#include <i18n_capable.h>
FILE	*fin;
FILE    *fcode[12];
struct savestr_s lastre;
struct savestr_s address_string;
char    sseof;
char	*recur;
char    *reend;
char    *hend;
union reptr     *ptrend;
int     eflag;
char    linebuf[LBSIZE+1];
int     gflag;
int     nlno;
#define	LOCMAXNFILES	12
char    fname[LOCMAXNFILES][NAME_MAX+1];
int     nfiles;
union reptr ptrspace[PTRSIZE];
union reptr *rep;
char    *cp;
char    respace[RESIZE];
struct label ltab[LABSIZE];
struct label    *lab;
struct label    *labend;
int     depth;
int     eargc;
char    **eargv;
union reptr     **cmpend[DEPTH];
char    *badp;
char    bad;
char    compfl;
int	xpg4command = 0;

#define CCEOF	22

struct label    *labtab = ltab;

char    *CGMES;
char    *TMMES;
char    *LTL;
char    *AD0MES;
char    *AD1MES;
char	*TOOBIG;
char    *bracket = NULL;
static int     nLen=0;
wchar_t	   *wBracket = NULL;
wchar_t	   *wCporg,*wCptr;
wchar_t    wSseof;
nl_catd ncCatd;
extern errno;

main(argc, argv)
char    *argv[];
{
	int	c;
	extern char *optarg;
	extern int optind;
	extern int opterr;
	extern int optopt;
	int	error = 0;

	(void) setlocale(LC_ALL,"");
        ncCatd = catopen("uxeoe",0);	
	CGMES = CATGETS(ncCatd,_MSG_SED_SED_COMMAND_GARBLED); 
	TMMES = CATGETS(ncCatd,_MSG_SED_TOO_MUCH_TEXT_FORMAT);
	LTL = CATGETS(ncCatd,_MSG_SED_LABEL_TOO_LONG_FORMAT);
	AD0MES = CATGETS(ncCatd,_MSG_SED_NO_ADDRESSES_ALLOWED);
	AD1MES = CATGETS(ncCatd,_MSG_SED_ONLY_ONE_ADDRESS_ALLOWED);
	TOOBIG = CATGETS(ncCatd,_MSG_SED_SUFFIX_TOO_LARGE_512);

	initsavestr(&lastre);
	initsavestr(&address_string);

	badp = &bad;
	aptr = abuf;
	lab = labtab + 1;       /* 0 reserved for end-pointer */
	rep = ptrspace;
	rep->r1.ad1lno = LNO_NONE;
	rep->r1.ad2lno = LNO_NONE;
	rep->r1.re1lno = LNO_NONE;
	recur = respace;
	lbend = &linebuf[LBSIZE];
	hend = &holdsp[LBSIZE];
	lcomend = &genbuf[71];
	ptrend = &ptrspace[PTRSIZE];
	reend = &respace[RESIZE-1];
	labend = &labtab[LABSIZE];
	lnum = 0;
	pending = 0;
	depth = 0;
	spend = linebuf;
	hspend = holdsp;	/* Avoid "bus error" under "H" cmd. */
	fcode[0] = stdout;
	nfiles = 1;

	if(argc == 1)
		exit(0);

	{
		char	*ep;

		ep = getenv("_XPG");
		if (ep != NULL)
			xpg4command = (atoi(ep) > 0);
	}

	opterr = 0;
	while ((c = getopt(argc,argv,"nf:e:g")) != EOF)
		switch (c) {

		case 'n':
			nflag++;
			continue;

		case 'f':
			if((fin = fopen(optarg, "r")) == NULL) {
				fprintf(stderr, CATGETS(ncCatd,_MSG_SED_CANNOT_OPEN_PATTERNFILE), optarg); 
				exit(2);
			}

			fcomp();
			fclose(fin);
			continue;

		case 'e':
			eflag++;
			eargc = 1;
			eargv = (&optarg) - 1;
			fcomp();
			eflag = 0;
			continue;

		case 'g':
			gflag++;
			continue;

		case '?':
		default:
			switch (optopt) {
			case 'f':
				fprintf( stderr, CATGETS(ncCatd,_MSG_SED_F_REQUIRES_A_SCRIPT) ); 
				break;

			case 'g':
				fprintf( stderr, CATGETS(ncCatd,_MSG_SED_E_REQUIRES_A_SCRIPT) ); 
				break;
				
			default:
				fprintf(stderr, CATGETS(ncCatd,_MSG_SED_UNKNOWN_FLAG_FORMAT), optopt); 
				break;
			}
			exit(2);
		}


	if (compfl == 0) {
		/*
		 * if no -e or -f option, parse first non-option argument
		 * as if it were preceded by -e.
		 */
		eargc = argc - optind;
		eargv = argv + optind - 1;
		eflag++;
		fcomp();
		optind++;
		eflag = 0;
	}

	if(depth) {
		fprintf(stderr, CATGETS(ncCatd,_MSG_SED_TOO_MANY_S));
		exit(2);
	}

	labtab->address = rep;

	dechain();

	if (optind >= argc) {
		eargc = 0;
		if (execute((char *)NULL))
			error = 1;
	} else {
		for (; optind < argc; optind++) {
			eargc = (argc - optind) - 1;
			if (execute(argv[optind]))
				error = 1;
		}
	}
	fclose(stdout);
	exit(error);
	/* NOTREACHED */
}

fcomp()
{

	register char   *rp, *tp;
	struct savestr_s op;
	char *sp;
	union reptr     *pt, *pt1;
	int     i, ii;
	struct label    *lpt;
	int	address_result;
	

	compfl = 1;
	initsavestr(&op);
	savestr(&op,lastre.s);

	if(rline(linebuf) < 0)  return;
	if(*linebuf == '#') {
		if(linebuf[1] == 'n')
			nflag = 1;
	}
	else {
		cp = linebuf;
		goto comploop;
	}


	for(;;) {
		if(rline(linebuf) < 0)  break;

		cp = linebuf;

comploop:
		while(*cp == ' ' || *cp == '\t')	cp++;
		if(*cp == '\0' || *cp == '#')	 continue;
		if(*cp == ';') {
			cp++;
			goto comploop;
		}

		sp = cp;
		if (address(&rep->r1.ad1,&rep->r1.ad1lno,NULL)) {
			fprintf(stderr, CGMES, linebuf);
			exit(2);
		}
		if (rep->r1.ad1lno == LNO_EMPTY) {
			if (op.length == 0) {
				fprintf(stderr, CATGETS(ncCatd,_MSG_SED_FIRST_RE_MAY_NOT_BE)); 
				exit(2);
			}
			(void) address(&rep->r1.ad1,&rep->r1.ad1lno,op.s);
		} else if (rep->r1.ad1lno == LNO_RE)
			savestr(&op,address_string.s);

		if(*cp == ',' || *cp == ';') {
			cp++;
			sp = cp;
			if (address(&rep->r1.ad2,&rep->r1.ad2lno,NULL)) {
				fprintf(stderr, CGMES, linebuf);
				exit(2);
			}
			if (rep->r1.ad2lno == LNO_EMPTY) {
				if (op.length == 0) {
					fprintf(stderr, CATGETS(ncCatd,_MSG_SED_FIRST_RE_MAY_NOT_BE)); 
					exit(2);
				}
				(void) address(&rep->r1.ad2,&rep->r1.ad2lno,op.s);
			} else if (rep->r1.ad2lno == LNO_RE)
				savestr(&op,address_string.s);
		} else
			rep->r1.ad2lno = LNO_NONE;

		while(*cp == ' ' || *cp == '\t')	cp++;

swit:
		switch(*cp++) {

			default:
				fprintf(stderr, CATGETS(ncCatd,_MSG_SED_UNRECOGNIZED_COMMAND), linebuf); 
				exit(2);

			case '!':
				rep->r1.negfl = 1;
				goto swit;

			case '{':
				rep->r1.command = BCOM;
				rep->r1.negfl = !(rep->r1.negfl);
				cmpend[depth++] = &rep->r2.lb1;
				rep->r2.lb1 = NULL;
				if(++rep >= ptrend) {
					fprintf(stderr, CATGETS(ncCatd,_MSG_SED_TOO_MANY_COMMANDS_FORMAT), linebuf); 
					exit(2);
				}
				if(*cp == '\0') continue;

				goto comploop;

			case '}':
				if(rep->r1.ad1lno != LNO_NONE) {
					fprintf(stderr, AD0MES, linebuf);
					exit(2);
				}

				if(--depth < 0) {	/* { */
					fprintf(stderr, CATGETS(ncCatd,_MSG_SED_TOO_MANY_S)); 
					exit(2);
				}
				*cmpend[depth] = rep;

				rep->r3.p = recur; /* XXX */
				continue;

			case '=':
				rep->r1.command = EQCOM;
				if(rep->r1.ad2lno != LNO_NONE) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				break;

			case ':':
				if(rep->r1.ad1lno != LNO_NONE) {
					fprintf(stderr, AD0MES, linebuf);
					exit(2);
				}

				while(*cp++ == ' ');
				cp--;


				tp = lab->asc;
				while((*tp++ = *cp++))
					if(tp > &(lab->asc[MAXLABLEN])) {
						fprintf(stderr, LTL, linebuf);
						exit(2);
					}

				if(lpt = search(lab)) {
					if(lpt->address) {
						fprintf(stderr, CATGETS(ncCatd,_MSG_SED_DUPLICATE_LABELS_FORMAT), linebuf); 
						exit(2);
					}
				} else {
					lab->chain = 0;
					lpt = lab;
					if(++lab >= labend) {
						fprintf(stderr, CATGETS(ncCatd,_MSG_SED_TOO_MANY_LABELS_FORMAT), linebuf); 
						exit(2);
					}
				}
				lpt->address = rep;
				rep->r3.p = recur; /* XXX */

				continue;

			case 'a':
				rep->r1.command = ACOM;
				if(rep->r1.ad2lno != LNO_NONE) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				if(*cp == '\\') cp++;
				if(*cp++ != '\n') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->r3.p = recur;
				recur = text(rep->r3.p, &respace[RESIZE]);
				break;
			case 'c':
				rep->r1.command = CCOM;
				if(*cp == '\\') cp++;
				if(*cp++ != ('\n')) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->r3.p = recur;
				recur = text(rep->r3.p, &respace[RESIZE]);
				break;
			case 'i':
				rep->r1.command = ICOM;
				if(rep->r1.ad2lno != LNO_NONE) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				if(*cp == '\\') cp++;
				if(*cp++ != ('\n')) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->r3.p = recur;
				recur = text(rep->r3.p, &respace[RESIZE]);
				break;

			case 'g':
				rep->r1.command = GCOM;
				break;

			case 'G':
				rep->r1.command = CGCOM;
				break;

			case 'h':
				rep->r1.command = HCOM;
				break;

			case 'H':
				rep->r1.command = CHCOM;
				break;

			case 't':
				rep->r1.command = TCOM;
				goto jtcommon;

			case 'b':
				rep->r1.command = BCOM;
jtcommon:
				rep->r2.lb1 = NULL;
				while(*cp++ == ' ');
				cp--;

				if(*cp == '\0') {
					if(pt = labtab->chain) {
						while(pt1 = pt->r2.lb1)
							pt = pt1;
						pt->r2.lb1 = rep;
					} else
						labtab->chain = rep;
					break;
				}
				tp = lab->asc;
				while((*tp++ = *cp++))
					if(tp > &(lab->asc[MAXLABLEN])) {
						fprintf(stderr, LTL, linebuf);
						exit(2);
					}
				cp--;

				if(lpt = search(lab)) {
					if(lpt->address) {
						rep->r2.lb1 = lpt->address;
					} else {
						pt = lpt->chain;
						while(pt1 = pt->r2.lb1)
							pt = pt1;
						pt->r2.lb1 = rep;
					}
				} else {
					lab->chain = rep;
					lab->address = 0;
					if(++lab >= labend) {
						fprintf(stderr, CATGETS(ncCatd,_MSG_SED_TOO_MANY_LABELS_FORMAT), linebuf); 
						exit(2);
					}
				}
				break;

			case 'n':
				rep->r1.command = NCOM;
				break;

			case 'N':
				rep->r1.command = CNCOM;
				break;

			case 'p':
				rep->r1.command = PCOM;
				break;

			case 'P':
				rep->r1.command = CPCOM;
				break;

			case 'r':
				rep->r1.command = RCOM;
				if(rep->r1.ad2lno != LNO_NONE) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				if(*cp++ != ' ') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->r3.p = recur;
				recur = text(rep->r3.p, &respace[RESIZE]);
				break;

			case 'd':
				rep->r1.command = DCOM;
				break;

			case 'D':
				rep->r1.command = CDCOM;
				rep->r2.lb1 = ptrspace;
				break;

			case 'q':
				rep->r1.command = QCOM;
				if(rep->r1.ad2lno != LNO_NONE) {
					fprintf(stderr, AD1MES, linebuf);
					exit(2);
				}
				break;

			case 'l':
				rep->r1.command = LCOM;
				break;

			case 's':
				rep->r1.command = SCOM;
				if (scansavestr(&address_string,&cp,0)) {
					fprintf(stderr,CGMES,linebuf);
					exit(2);
				}
				if (address_string.length == 0) {
					if(op.length == 0) {
						fprintf(stderr,
							CATGETS(ncCatd,_MSG_SED_FIRST_RE_MAY_NOT_BE)); 
						exit(2);
					}
					savestr(&address_string,
						op.s);
				} else {
					savestr(&op,
						address_string.s);
				}
				rep->r1.re1lno = LNO_RE;
				if (regcomp(&rep->r1.re1,
					    address_string.s, 0)) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				rep->r1.rhs = recur;
				if(compsub(&rep->r1.re1,
					   rep->r1.rhs,&recur)) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}

				if(*cp == 'g') {
					cp++;
					rep->r1.gfl = 999;
				} else if(gflag)
					rep->r1.gfl = 999;

				if(isdigit(*cp))
					{i = *cp - '0';
					cp++;
					while(1)
						{ii = *cp;
						if(!isdigit(ii)) break;
						i = i*10 + ii - '0';
						if(i > 512)
							{fprintf(stderr, TOOBIG, linebuf);
							exit(2);
							}
						cp++;
						}
					rep->r1.gfl = i;
					}

				if(*cp == 'p') {
					cp++;
					rep->r1.pfl = 1;
				}

				if(*cp == 'P') {
					cp++;
					rep->r1.pfl = 2;
				}

				if(*cp == 'w') {
					cp++;
					if(*cp++ !=  ' ') {
						fprintf(stderr, CGMES, linebuf);
						exit(2);
					}
					if(nfiles >= 10) {
						fprintf(stderr, CATGETS(ncCatd,_MSG_SED_TOO_MANY_FILES_IN_W)); 
						exit(2);
					}

					text(fname[nfiles], 
						&fname[LOCMAXNFILES][0]);
					for(i = nfiles - 1; i >= 0; i--)
						if(cmp(fname[nfiles],fname[i]) == 0) {
							rep->r1.fcode = fcode[i];
							goto done;
						}
					if((rep->r1.fcode = fopen(fname[nfiles], "w")) == NULL) {
						fprintf(stderr, CATGETS(ncCatd,_MSG_SED_CANNOT_OPEN_FORMAT), fname[nfiles]); 
						exit(2);
					}
					fcode[nfiles++] = rep->r1.fcode;
				}
				break;

			case 'w':
				rep->r1.command = WCOM;
				if(*cp++ != ' ') {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				if(nfiles >= 10){
					fprintf(stderr, CATGETS(ncCatd,_MSG_SED_TOO_MANY_FILES_IN_W)); 
					exit(2);
				}

				text(fname[nfiles], &fname[LOCMAXNFILES][0]);
				for(i = nfiles - 1; i >= 0; i--)
					if(cmp(fname[nfiles], fname[i]) == 0) {
						rep->r1.fcode = fcode[i];
						goto done;
					}

				if((rep->r1.fcode = fopen(fname[nfiles], "w")) == NULL) {
					fprintf(stderr, CATGETS(ncCatd,_MSG_SED_CANNOT_CREATE_FORMAT), fname[nfiles]); 
					exit(2);
				}
				fcode[nfiles++] = rep->r1.fcode;
				break;

			case 'x':
				rep->r1.command = XCOM;
				break;

			case 'y':
				rep->r1.command = YCOM;
                            if ( I18N_SBCS_CODE ) 
				sseof = *cp++;
                            else 
				cp+=(nLen = mbtowc(&wSseof,cp,MB_CUR_MAX));
				rep->r3.p = recur;
				if (ycomp(rep->r3.p,&recur)) {
					fprintf(stderr, CGMES, linebuf);
					exit(2);
				}
				break;

		}
done:
		if(++rep >= ptrend) {
			fprintf(stderr, CATGETS(ncCatd,_MSG_SED_TOO_MANY_COMMANDS_LAST), linebuf); 
			exit(2);
		}

		rep->r1.ad1lno = LNO_NONE;
		rep->r1.ad2lno = LNO_NONE;
		rep->r1.re1lno = LNO_NONE;

		if(*cp++ != '\0') {
			if(cp[-1] == ';')
				goto comploop;
			fprintf(stderr, CGMES, linebuf);
			exit(2);
		}

	}
	rep->r1.command = 0;
	lastre = op;
}
int	compsub(regex_t *expbuf,char *rhsbuf,char **rp)
{
	register char   *p, *q;
	register int nBytes=0;
	wchar_t wC;

	p = rhsbuf;
	q = cp;
     if ( I18N_SBCS_CODE ) {
	for(;;) {
		if(p > reend) {
			fprintf(stderr, TMMES, linebuf);
			exit(2);
		}
		if((*p = *q++) == '\\') {
			p++;
			if(p > reend) {
				fprintf(stderr, TMMES, linebuf);
				exit(2);
			}
			*p = *q++;
			if(*p > expbuf->re_nsub + '0' && *p <= '9')
				return(1);
			p++;
			continue;
		}
		if(*p == sseof) {
			*p++ = '\0';
			cp = q;
			*rp = p;
			return(0);
		}
		if(*p++ == '\0')
			return(1);
	}
     } else {
	for(;;) {
		if(p > reend) {
			fprintf(stderr, TMMES, linebuf);
			exit(2);
		}
		if((*p = *q++) == '\\') {
			p++;
			if(p > reend) {
				fprintf(stderr, TMMES, linebuf);
				exit(2);
			}
			*p = *q;
			if(*p > expbuf->re_nsub + '0' && *p <= '9')
				return(1);
		        nBytes = mbtowc(0,q,MB_CUR_MAX);
			while ( nBytes-- ) *p++ = *q++;
			continue;
		}
		nBytes = mbtowc(&wC,(q-1),MB_CUR_MAX);
		if ( wC == wSseof ) {
			*p++ = '\0';
			cp = q+nBytes-1;
			*rp = p;
			return(0);
		} else 
			while ( --nBytes > 0 ) p++,*p=*q++;
		if(*p++ == '\0')
			return(1);
	}
       }
}

rline(lbuf)
char    *lbuf;
{
	register char   *p, *q;
	register	t;
	static char     *saveq;

	p = lbuf - 1;

	if(eflag) {
		if(eflag > 0) {
			eflag = -1;
			if(eargc-- <= 0)
				exit(2);
			q = *++eargv;
			while(*++p = *q++) {
				if((*p == '\\' && p!=lbuf && mblen((p-1),MB_CUR_MAX) <= 1) || 
				   (*p == '\\' && p==lbuf)) {
					if((*++p = *q++) == '\0') {
						saveq = 0;
						return(-1);
					} else
						continue;
				}
				if(*p == '\n') {
					*p = '\0';
					saveq = q;
					return(1);
				}
			}
			saveq = 0;
			return(1);
		}
		if((q = saveq) == 0)    return(-1);

		while(*++p = *q++) {
			if((*p == '\\' && p!=lbuf &&  mblen((p-1),MB_CUR_MAX) <= 1) ||
			  (*p == '\\' && p==lbuf)) {
				if((*++p = *q++) == '0') {
					saveq = 0;
					return(-1);
				} else
					continue;
			}
			if(*p == '\n') {
				*p = '\0';
				saveq = q;
				return(1);
			}
		}
		saveq = 0;
		return(1);
	}

	while((t = getc(fin)) != EOF) {
		*++p = t;
		if((*p == '\\' && p!=lbuf && mblen((p-1),MB_CUR_MAX) <= 1) ||
		  (*p == '\\' && p==lbuf)) {
			t = getc(fin);
			*++p = t;
		}
		else if(*p == '\n') {
			*p = '\0';
			return(1);
		}
	}
	return(-1);
}


int	address(regex_t *rep,int *replno,char *s)
{
	char   *rcp;
	char *base_rcp;
	char *scp;
	long    lno;
	int	result = 0;

	if (s != NULL)
		rcp = s;
	else
		rcp = cp;
	base_rcp = rcp;
	if(*rcp == '$') {
		*replno = LNO_LAST;
		rcp++;
		goto update_and_return;
	}
	if (*rcp == '/' || *rcp == '\\' ) {
		if (scansavestr(&address_string,&rcp,1))
			return(1);
		if (address_string.length == 0) {
			*replno = LNO_EMPTY;
			goto update_and_return;
		}
		*replno = LNO_RE;
		result = regcomp(rep,
				 address_string.s,
				 REG_NOSUB);
		goto update_and_return;
	}

	lno = 0;

	while(isdigit(*rcp))
		lno = lno*10 + *rcp++ - '0';

	if(rcp > base_rcp) {
		*replno = nlno;
		tlno[nlno++] = lno;
		if(nlno >= NLINES) {
			fprintf(stderr, CATGETS(ncCatd,_MSG_SED_TOO_MANY_LINE_NUMBERS)); 
			exit(2);
		}
	} else
		*replno = LNO_NONE;

update_and_return:
	if (s == NULL) 
		cp = rcp;
	else if (*rcp != 0)
		return(1);
	if (result != 0)
		return(1);
	return(0);
}
cmp(a, b)
char    *a,*b;
{
	register char   *ra, *rb;

	ra = a - 1;
	rb = b - 1;

	while(*++ra == *++rb)
		if(*ra == '\0') return(0);
	return(1);
}

char    *text(textbuf, maxptr)
char    *textbuf;
char	*maxptr;
{
	register char   *p, *q;
	register int nByte=0;

	p = textbuf;
	q = cp;
	for(;;) {

		if(((*p = *q++) == '\\' && p!=textbuf && mblen((p-1),MB_CUR_MAX) <= 1) ||
		   (*p == '\\' && p == textbuf)) {
			nByte = mblen(q,MB_CUR_MAX);
			if (nByte == 0)
				*p = *q++;
			else
				while( nByte-- ) {
					if((p+1) >= maxptr)
						break;
					*p++ = *q++;
				}
			continue ; 
		}
		if(*p == '\0') {
			cp = --q;
			return(++p);
		}
		if((p+1) >= maxptr)
			break;
		p++;
	}
}


struct label    *search(ptr)
struct label    *ptr;
{
	struct label    *rp;

	rp = labtab;
	while(rp < ptr) {
		if(cmp(rp->asc, ptr->asc) == 0)
			return(rp);
		rp++;
	}

	return(0);
}


dechain()
{
	struct label    *lptr;
	union reptr     *rptr, *trptr;

	for(lptr = labtab; lptr < lab; lptr++) {

		if(lptr->address == 0) {
			fprintf(stderr, CATGETS(ncCatd,_MSG_SED_UNDEFINED_LABEL_FORMAT), lptr->asc); 
			exit(2);
		}

		if(lptr->chain) {
			rptr = lptr->chain;
			while(trptr = rptr->r2.lb1) {
				rptr->r2.lb1 = lptr->address;
				rptr = trptr;
			}
			rptr->r2.lb1 = lptr->address;
		}
	}
}

int ycomp(char *expbuf,char **nep)
{
	register char   c; 
	register char *ep, *tsp;
	register int i;
	char    *sp;
	register wchar_t *wEp, *wEnp, *wTsp, *wTnsp, *wCp, wC;
	register nChar=0, nL=0, nB=0;

	ep = expbuf;
     if( I18N_SBCS_CODE ) {
	if(ep + 0377 > reend) {
		fprintf(stderr, TMMES, linebuf);
		exit(2);
	}
	sp = cp;
	for(tsp = cp; *tsp != sseof; tsp++) {
		if(*tsp == '\\')
			tsp++;
		if(*tsp == '\n' || *tsp == '\0')
			return(1);
	}
	tsp++;

	while((c = *sp++) != sseof) {
		c &= 0377;
		if(c == '\\' && *sp == 'n') {
			sp++;
			c = '\n';
		}
		if((ep[c] = *tsp++) == '\\' && *tsp == 'n') {
			ep[c] = '\n';
			tsp++;
		}
		if(ep[c] == sseof || ep[c] == '\0')
			return(1);
	}
	if(*tsp != sseof)
		return(1);
	cp = ++tsp;

	for(i = 0; i < 0400; i++)
		if(ep[i] == 0)
			ep[i] = i;

     } else { 
		nChar = mbstowcs(0,cp,0);
		if(errno == EILSEQ) {
			perror("y command fails on ");
			exit(1);
		}
		wCp = (wchar_t *)malloc((nChar+1)*sizeof(wchar_t));
		if ( wCp == NULL ) {
			fprintf(stderr,CATGETS(ncCatd,_MSG_SED_CANNOT_ALLOCATE_FORMAT),nChar + 1);
			exit(1);
		}
		mbstowcs( wCp, cp, nChar+1);
		wTnsp = wTsp = wCp;
		for (;
		     (wC = *wTnsp) != wSseof;
		     wTsp++, wTnsp++) {
			if (wC == L'\n' || wC == 0 )
				return (1);
			if (wC == L'\\') {
				wTnsp++;
				wC = *wTnsp;
				if (wC == wSseof) {
					nB--;
				} else if (wC == L'n') {
					wC = L'\n';
					nL++;
				} else {
					*wTsp++ = L'\\';
					nB++;
				}
			}
			*wTsp = wC;
		}
	        *wTsp =  0;
		wTsp = wEnp = wEp = ++wTnsp;
                for (;
                     (wC = *wEnp) != wSseof;
                     wEp++, wEnp++) {
			if (wC == L'\n' || wC == 0 )
				return (1);
                        if (wC == L'\\') {
                                wEnp++;
                                wC = *wEnp;
                                if (wC == wSseof) {
					nB--;
                                } else if (wC == L'n') {
                                        wC = L'\n';
					nL++;
                                } else {
                                        *wEp++ = L'\\';
					nB++;
                                }
                        }
                        *wEp = wC;
                }
		*wEp='\0';
		cp += wcstombs(0,wCp,0) + wcstombs(0,wTsp,0) + nLen + nLen + nL - nB;
		wCptr=wTsp;
		wCporg=wCp;
	}
	*nep = (ep + 0400);
	return(0);
}


void 
initsavestr(struct savestr_s *sp)
{
	sp->maxlength = 0;
	sp->length = 0;
	sp->s = NULL;
     if ( I18N_SBCS_CODE )
	bracket = NULL;	/* Initialize bracket expression open position */
     else
	wBracket = NULL;	/* Initialize bracket expression open position */
}


void 
freesavestr(struct savestr_s *sp)
{
	if (sp->s != NULL)
		free((void *) sp->s);
	initsavestr(sp);
}


int
savestrn(struct savestr_s *sp, char *s,int n)
{
	int	len;
	char	*ns;

	if (s == NULL)
		s = "";
	len = strlen(s);
	if (n >= 0 &&
	    len > n)
		len = n;
	if (len > sp->maxlength ||
	    sp->maxlength == 0) {
		sp->maxlength = len;
		if (sp->maxlength < 10)
			sp->maxlength = 10;
		ns = (char *) malloc(sp->maxlength + 1);
		if (ns == NULL) {
			fprintf(stderr,CATGETS(ncCatd,_MSG_SED_CANNOT_ALLOCATE_FORMAT),len + 1); 
			exit(1);
		}
		if (sp->s != NULL)
			free((void *) sp->s);
		sp->s = ns;
	}
	sp->length = len;
	if (len > 0)
		strncpy(sp->s,s,len);
	sp->s[len] = 0;
	return(0);
}


int
scansavestr(struct savestr_s *sp, char **cpp,int do_escape)
{
	char	*scp;
	int	did_escape = 0;
	char	ch;
	int	i;
	int	saw_escape = 0;
	char	*np;
	int	nBytes=0, nChar=0;
	wchar_t	*wScp, *wScpold;
	wchar_t	wCh;
	wchar_t	*wNp, *wCpp, *wCppold;

    if ( I18N_SBCS_CODE ) {
	sseof = *(*cpp)++;
	if (do_escape &&
	    sseof == '\\') {
		sseof = *(*cpp)++;
		if (sseof == 0)
			return(1);
		did_escape = 1;
	}
	scp = (*cpp);
	while ((ch = *(*cpp)) != 0) {
		if (!bracket && ch == '[' && sseof != ch)
			bracket = (char *)*cpp;	/* '[' position */
		if (ch == '\\') {
			(*cpp)++;
			ch = *(*cpp);
			if (ch == 0)
				break;
			if (ch == sseof ||
			    ch == 'n')
				saw_escape = 1;
		} else if (!bracket && (ch == sseof)) {
			savestrn(&address_string,scp,(*cpp) - scp);
			(*cpp)++;
			if (saw_escape) {
				for (scp = address_string.s, np = scp;
				     (ch = (*np)) != 0;
				     scp++, np++) {
					if (ch == '\\') {
						np++;
						ch = *np;
						if (ch == sseof) {
							NULL;
						} else if (ch == 'n') {
							ch = '\n';
						} else {
							*scp++ = '\\';
						}
					}
					*scp = ch;
				}
				*scp = 0;
				address_string.length = scp - address_string.s;
			}
			return(0);
		}
		if (bracket && ((*cpp - 1) != bracket) && 
			 (ch == ']'))
			bracket = 0;
		(*cpp)++;
	}
    } else {
	nChar=mbstowcs(0,*cpp,0);
	if(errno == EILSEQ) {
		perror("pattern contains ");
		exit(1);
	}
	wCpp=(wchar_t *)malloc((nChar+1)*sizeof(wchar_t));
	if (wCpp == NULL) {
		fprintf(stderr,CATGETS(ncCatd,_MSG_SED_CANNOT_ALLOCATE_FORMAT),nChar + 1);
		exit(1);
	}
	mbstowcs(wCpp,*cpp,nChar+1);
	wCppold = wCpp;
	wSseof = *wCpp++;
	if (do_escape &&
	    wSseof == L'\\') {
		wSseof = *wCpp++;
		(*cpp)+=mblen((*cpp),MB_CUR_MAX);
		if (wSseof == 0) {
			while(((*cpp)++)!=NULL);
			free(wCppold);
			return(1);
		}
		did_escape = 1;
	}
	(*cpp)+=mblen((*cpp),MB_CUR_MAX);
	wScp = wCpp;
	scp = (*cpp);
	while ((wCh = *wCpp) != 0) {
		if (!wBracket && wCh == L'[' && wSseof != wCh)
			wBracket = wCpp;	/* '[' position */
		if (wCh == L'\\') {
			wCpp++;
			wCh = *wCpp;
			if (wCh == 0)
				break;
			if (wCh == wSseof ||
			    wCh == L'n') {
				saw_escape = 1;
			}
		} else if (!wBracket && (wCh == wSseof)) {
			nChar = wCpp - wScp ;
			while(nChar--) (*cpp)+= mblen((*cpp),MB_CUR_MAX);
			savestrn(&address_string,scp,(*cpp) - scp);
			(*cpp)+= mblen((*cpp),MB_CUR_MAX);
			if (saw_escape) {
				nChar=mbstowcs(0,address_string.s,0);
				if(errno == EILSEQ) {
					perror("pattern contains ");
					exit(1);
				}
				mbstowcs(wScp,address_string.s,nChar+1);
				scp = address_string.s;
				wNp = wScpold = wScp;
				for (;
				     (wCh = (*wNp)) != 0;
				     wScp++, wNp++) {
					if (wCh == L'\\') {
						wNp++;
						wCh = *wNp;
						if (wCh == wSseof) {
							NULL;
						} else if (wCh == L'n') {
							wCh = L'\n';
						} else {
							*wScp++ = L'\\';
						}
					}
					*wScp = wCh;
				}
				*wScp = 0;
				address_string.length = wcstombs(scp,wScpold,wcstombs(0,wScpold,0)+1);
			}
			free(wCppold);
			return(0);
		}
		if (wBracket && ((wCpp - 1) != wBracket) && 
			 (wCh == L']'))
			wBracket = 0;
		wCpp++;
	}
	*wCpp=0;
	(*cpp)+=wcstombs(0,wCppold,0);
	free(wCppold);
      }
	return(1);
}
