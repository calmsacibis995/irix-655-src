/*
 *	$Id: lib.c,v 1.16 1998/09/18 19:47:22 sherwood Exp $
 */

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)awk:lib.c	2.14"

/***********************I18N File Header******************************
File                  : lib.c

Compatibility Options : No support/Improper support/ EUC single byte/
                        EUC multibyte/Sjis-Big5/Full multibyte/Unicode

Old Compatibility     : No Support

Present Compatibility : EUC single byte/ EUC  Multibyte / Big5-Sjis

Type of Application (Normal/Important/Critical) : Critical 

Optimization Level (EUC & Single byte/Single byte/Not optimized)
                      : Single byte

Change History        : 16 Sept 1997       HCL
	macro 'isnumber' defined in  wchar.h is
	overidden by  isnumber().
	readrec() is modified to prcoess 
	input file  in wide characters. 
	fldbld() now prcosesses record by using mblen(). 
	nematch() is  now called with three input arguments.
	The Third argument is a flag used for the '^' in regular
	expressions.
Change History        : 17 Dec 1997     HCL
        Error handling conditions are added to make check on 'mbtowc()'
        function calls. 
Change History	      : 13 Jan 1998     HCL
	* VSC BUG FIX : for failed assertion#(25,240,245,247,302) on 
	    Baseline version, in VSC . 
************************End of I18N File Header**********************/

#define DEBUG
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include "awk.h"
#include "y.tab.h"
#include <pfmt.h>
#include <i18n_capable.h>
#include <wchar.h>
#include <msgs/uxawk.h>

#ifdef isnumber
#undef isnumber
#endif

#define	getfval(p)	(((p)->tval & (ARR|FLD|REC|NUM)) == NUM ? (p)->fval : r_getfval(p))
#define	getsval(p)	(((p)->tval & (ARR|FLD|REC|STR)) == STR ? (p)->sval : r_getsval(p))

extern	Awkfloat r_getfval();
extern	uchar	*r_getsval();

FILE	*infile	= NULL;
uchar	*file	= (uchar*) "";
uchar	recdata[RECSIZE];
uchar	*record	= recdata;
uchar	fields[RECSIZE];

#define	MAXFLD	200
int	donefld;	/* 1 = implies rec broken into fields */
int	donerec;	/* 1 = record is valid (no flds have changed) */

#define	FINIT	{ OCELL, CFLD, NULL, (uchar*) "", 0.0, FLD|STR|DONTFREE }

Cell fldtab[MAXFLD] = {		/* room for fields */
	{ OCELL, CFLD, (uchar*) "$0", recdata, 0.0, REC|STR|DONTFREE},
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
 	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
};
int	maxfld	= 0;	/* last used field */
int	argno	= 1;	/* current input argument number */
extern	Awkfloat *ARGC;
extern	uchar	*getargv();

/* I18NCAT_PGM_MSG */
const char badopen[] = CATNUM_DEFMSG(_MSG_NAWK_CANNOT_OPEN);

void initgetrec(),setclvar(),fldbld(),cleanfld(),newfld(),recbld(),fpecatch();
void bracecheck(),bcheck2(),error(),eprint(),bclass(),PUTS();

void
initgetrec()
{
	int i;
	uchar *p;

	for (i = 1; i < *ARGC; i++) {
		if (!isclvar(p = getargv(i)))	/* find 1st real filename */
			return;
		setclvar(p);	/* a commandline assignment before filename */
		argno++;
	}
	infile = stdin;		/* no filenames, so use stdin */
	/* *FILENAME = file = (uchar*) "-"; */
}

getrec(buf)
	uchar *buf;
{
	int c;
	static int firsttime = 1;

	if (firsttime) {
		firsttime = 0;
		initgetrec();
	}

	/* SGI BUG FIX: FILENAME may not be initialized yet, so we can't
	 * print it.  Trying to do so causes seg faults.
	 */
	dprintf( ("RS=<%s>, FS=<%s>, ARGC=%d\n",
		*RS, *FS, *ARGC) );
	donefld = 0;
	donerec = 1;
	buf[0] = 0;
	while (argno < *ARGC || infile == stdin) {
		dprintf( ("argno=%d, file=|%s|\n", argno, file) )
		;
		if (infile == NULL) {	/* have to open a new file */
			file = getargv(argno);
			if (*file == '\0') {	/* it's been zapped */
				argno++;
				continue;
			}
			if (isclvar(file)) {	/* a var=value arg */
				setclvar(file);
				argno++;
				continue;
			}
			*FILENAME = file;
			dprintf( ("opening file %s\n", file) );
			if (*file == '-' && *(file+1) == '\0')
				infile = stdin;
			else if ((infile = fopen((char *)file, "r")) == NULL)
				error(MM_ERROR, badopen, file, strerror(errno));
			setfval(fnrloc, 0.0);
		}
		c = readrec(buf, RECSIZE, infile);
		if (c != 0 || buf[0] != '\0') {	/* normal record */
			if (buf == record) {
				if (!(recloc->tval & DONTFREE))
					xfree(recloc->sval);
				recloc->sval = record;
				recloc->tval = REC | STR | DONTFREE;
				if (isnumber(recloc->sval)) {
					recloc->fval = atof(recloc->sval);
					recloc->tval |= NUM;
				}
			}
			setfval(nrloc, nrloc->fval+1);
			setfval(fnrloc, fnrloc->fval+1);
			return 1;
		}
		/* EOF arrived on this file; set up next */
		if (infile != stdin)
			fclose(infile);
		infile = NULL;
		argno++;
	}
	return 0;	/* true end of file */
}

readrec(buf, bufsize, inf)	/* read one record into buf */
	uchar *buf;
	int bufsize;
	FILE *inf;
{
	register int sep, c;
	register uchar *rr;
	extern int errno;

	wint_t wsep,wc,wrs;
    /* I18NCAT_CHAR_BOUNDARY I18NCAT_WINT_WCHAR */
   if( I18N_SBCS_CODE ) {
	if ((sep = **RS) == 0) {
		sep = '\n';
		while ((c=getc(inf)) == '\n' && c != EOF)	/* skip leading \n's */
			;
		if (c != EOF)
			ungetc(c, inf);
	}
	for (rr = buf; ; ) {
		/* SGI BUG FIX: Do bounds checking on buffer */
		for (; (c=getc(inf)) != sep && c != EOF && rr < buf+bufsize; 
	   	     *rr++ = c) ;

		/* SGI BUG FIX: Exit loop if exceeded buffer */
		if (**RS == sep || c == EOF || rr >= buf+bufsize)
			break;
		if ((c = getc(inf)) == '\n' || c == EOF) /* 2 in a row */
			break;
		*rr++ = '\n';
		*rr++ = c;
	}

	/* SGI BUG FIX: check for >= rather than > */
	if (rr >= buf + bufsize)
		/* I18NCAT_PGM_MSG */
		error(MM_ERROR,PFMTTXT(_MSG_NAWK_INPUT_REC_TOO_LONG), buf);
	*rr = 0;
	dprintf( ("readrec saw <%s>, returns %d\n", buf, c == EOF
		&& rr == buf ? 0 : 1) );
	return c == EOF && rr == buf ? 0 : 1;

   } else {
	/* get the mb seperator */
	char s[MB_LEN_MAX+1];
	int buflen=0;
	strcpy(s,*RS);
	if (mbtowc(&wsep,s,MB_CUR_MAX) < 0 ) {
		printf(GETTXT(_MSG_NAWK_READREC));
		exit(1);
	}
	wrs = wsep; /* take a copy of wsep incase wsep changes to L'\n' */
	if (wsep == 0) {
		wsep = L'\n';
		while((wc=getwc(inf)) == L'\n' && wc != WEOF && errno!=EILSEQ)	/* skip leading \n's */
			;
		if (errno==EILSEQ) {
			perror("readrec");
			exit(1);
		}

		if (wc != WEOF)
			ungetwc(wc,inf);
	}

	for (rr=buf; ; ) {
		for( ;(wc=getwc(inf))!= wsep && wc != WEOF && errno!=EILSEQ && rr < buf+bufsize; rr +=mblen(s,MB_CUR_MAX)) {
			wctomb(s,wc); 
			strcpy(rr,s);
		}
		if ( wrs==wsep || wc == WEOF || errno==EILSEQ || rr >= buf+bufsize )
			break;
		if ((wc=getwc(inf)) == L'\n' || wc == WEOF ) /* 2 in a row */
			break;
		*rr++ = '\n';
		wctomb(s,wc); 
		strcpy(rr,s); 
		rr += mblen(s,MB_CUR_MAX);
	}
	
		if (errno==EILSEQ) {
			perror("readrec");
			exit(1);
		}

	if (rr >= buf + bufsize)
		/* I18NCAT_PGM_MSG */
		error(MM_ERROR,PFMTTXT(_MSG_NAWK_INPUT_REC_TOO_LONG), buf);
	*rr = 0;
	dprintf( ("readrec saw <%s>, returns %d\n", buf, c == EOF
	    && rr == buf ? 0 : 1) );
	return wc == WEOF && rr == buf ? 0 : 1;

   }
}

uchar *getargv(n)	/* get ARGV[n] */
	int n;
{
	Cell *x;
	uchar *s, temp[10];
	extern Array *ARGVtab;

	sprintf((char *)temp, "%d", n);
	x = setsymtab(temp, "", 0.0, STR, ARGVtab);
	s = getsval(x);
	dprintf( ("getargv(%d) returns |%s|\n", n, s) );
	return s;
}

void
setclvar(s)	/* set var=value from s */
uchar *s;
{
	uchar *p;
	Cell *q;

	for (p=s; *p != '='; p++)
		;
	*p++ = 0;
	p = qstring(p, '\0');
	q = setsymtab(s, p, 0.0, STR, symtab);
	setsval(q, p);
	if (isnumber(q->sval)) {
		q->fval = atof(q->sval);
		q->tval |= NUM;
	}
	dprintf( ("command line set %s to |%s|\n", s, p) );
}


void
fldbld()
{
	register uchar *r, *fr, sep;
	Cell *p;
	int i;

	if (donefld)
		return;
	if (!(recloc->tval & STR))
		getsval(recloc);
	r = recloc->sval;	/* was record! */
	fr = fields;
	i = 0;	/* number of fields accumulated here */
   /* I18NCAT_CHAR_BOUNDARY */
   if (I18N_SBCS_CODE) {
	if ((int) strlen((char*) *FS) > 1) {	/* it's a regular expression */
		i = refldbld(r, *FS);
	} else if ((sep = **FS) == ' ') {
		for (i = 0; ; ) {
			while (*r == ' ' || *r == '\t' || *r == '\n')
				r++;
			if (*r == 0)
				break;
			i++;
			if (i >= MAXFLD)
				break;
			if (!(fldtab[i].tval & DONTFREE))
				xfree(fldtab[i].sval);

			/* SGI BUG FIX: If fr is NULL, make the symbol "" */
			fldtab[i].sval = ((fr) ? (fr) : tostring(""));
			fldtab[i].tval = FLD | STR | DONTFREE;
			do
				*fr++ = *r++;
			while (*r != ' ' && *r != '\t' && *r != '\n' && *r != '\0');
			*fr++ = 0;
		}
		*fr = 0;
	} else if (*r != 0) {	/* if 0, it's a null field */
		for (;;) {
			i++;
			if (i >= MAXFLD)
				break;
			if (!(fldtab[i].tval & DONTFREE))
				xfree(fldtab[i].sval);

			/* SGI BUG FIX */
			fldtab[i].sval = ((fr) ? (fr) : tostring(""));
			fldtab[i].tval = FLD | STR | DONTFREE;
			while (*r != sep && *r != '\n' && *r != '\0')	/* \n always a separator */
				*fr++ = *r++;
			*fr++ = 0;
			if (*r++ == 0)
				break;
		}
		*fr = 0;
	}
   } else {	/* EUC and Multi byte block */

	if ((int) strlen((char*) *FS) > 1) { 	/* it's a regular expression */
		i = refldbld(r, *FS);
	} else if ((sep = **FS) == ' ') {
	for (i = 0; ; ) {
		while (*r == ' ' || *r == '\t' || *r == '\n')
			r++;
		if (*r == 0)
			break;
		i++;
		if (i >= MAXFLD)
			break;
		if (!(fldtab[i].tval & DONTFREE))
			xfree(fldtab[i].sval);

		/* If fr is NULL, make the symbol "" */
		fldtab[i].sval = ((fr) ? (fr) : tostring(""));
		fldtab[i].tval = FLD | STR | DONTFREE;
		do
			*fr++ = *r++;
		while (*r != ' ' && *r != '\t' && *r != '\n' && *r != '\0');
		*fr++ = 0;
	}
	*fr = 0;
	} else if (*r != 0) {	/* if 0, it's a null field */
		for (;;) {
			int nBytes=0;
			wchar_t wr=L'\0',wsep;
			i++;
			if (i >= MAXFLD)
				break;
			if (!(fldtab[i].tval & DONTFREE))
				xfree(fldtab[i].sval);

			fldtab[i].sval = ((fr) ? (fr) : tostring(""));
			fldtab[i].tval = FLD | STR | DONTFREE;
			mbtowc(&wsep,*FS,MB_CUR_MAX);
			nBytes = mbtowc(&wr,r,MB_CUR_MAX); 
			while (wr != wsep && *r != '\n' && *r != '\0') {	/* \n always a separator */
				for( ; (nBytes--)>0; )
					*fr++ = *r++;
				nBytes = mbtowc(&wr,r,MB_CUR_MAX);
			}
			*fr++ = 0;
			if (*r++ == 0)
				break;
		}
		*fr = 0;
	}
   }
	if (i >= MAXFLD)
		/* I18NCAT_PGM_MSG */
		error(MM_ERROR,PFMTTXT(_MSG_NAWK_REC_HAS_TOO_MANY_FLDS),
			record);
	/* clean out junk from previous record */
	cleanfld(i, maxfld);
	maxfld = i;
	donefld = 1;
	for (p = fldtab+1; p <= fldtab+maxfld; p++) {
		if(isnumber(p->sval)) {
			p->fval = atof(p->sval);
			p->tval |= NUM;
		}
	}
	setfval(nfloc, (Awkfloat) maxfld);
	if (dbg)
		for (p = fldtab; p <= fldtab+maxfld; p++)
			/* I18NCAT_PGM_MSG */
			pfmt(stdout, MM_INFO,PFMTTXT(_MSG_NAWK_FIELD_IS), p-fldtab, 
				p->sval);
}

void
cleanfld(n1, n2)	/* clean out fields n1..n2 inclusive */
{
	static uchar *nullstat = (uchar *) "";
	register Cell *p, *q;

	for (p = &fldtab[n2], q = &fldtab[n1]; p > q; p--) {
		if (!(p->tval & DONTFREE))
			xfree(p->sval);
		p->tval = FLD | STR | DONTFREE;
		p->sval = nullstat;
	}
}

void
newfld(n)	/* add field n (after end) */
{
	if (n >= MAXFLD)
		/* I18NCAT_PGM_MSG */
		error(MM_ERROR,PFMTTXT(_MSG_NAWK_CREATING_TOO_MANY_FLDS), record);
	cleanfld(maxfld, n);
	maxfld = n;
	setfval(nfloc, (Awkfloat) n);
}

refldbld(rec, fs)	/* build fields from reg expr in FS */
	uchar *rec, *fs;
{
	fa *makedfa();
	uchar *fr;
	int i, tempstat;
	fa *pfa;

	fr = fields;
	*fr = '\0';
	if (*rec == '\0')
		return 0;
	pfa = makedfa(fs, 1);
	dprintf( ("into refldbld, rec = <%s>, pat = <%s>\n", rec,
		fs) );
	tempstat = pfa->initstat;
	for (i = 1; i < MAXFLD; i++) {
		if (!(fldtab[i].tval & DONTFREE))
			xfree(fldtab[i].sval);
		fldtab[i].tval = FLD | STR | DONTFREE;
		fldtab[i].sval = fr;
		dprintf( ("refldbld: i=%d\n", i) );
		/* I18NCAT_OTHER */
		if (nematch(pfa, rec,(i == 1) ? 1 : 0)) {
			pfa->initstat = 2;
			dprintf( ("match %s (%d chars\n", 
				patbeg, patlen) );
			strncpy((char*) fr, (char*) rec, patbeg-rec);
			fr += patbeg - rec + 1;
			*(fr-1) = '\0';
			rec = patbeg + patlen;
		} else {
			dprintf( ("no match %s\n", rec) );
			strcpy((char*) fr, (char*) rec);
			pfa->initstat = tempstat;
			break;
		}
	}
	return i;		
}

void
recbld()
{
	int i;
	register uchar *r, *p;
	static uchar rec[RECSIZE];

	if (donerec == 1)
		return;
	r = rec;
	for (i = 1; i <= *NF; i++) {
		p = getsval(&fldtab[i]);
		while (*r = *p++)
			r++;
		if (i < *NF)
			for (p = *OFS; *r = *p++; )
				r++;
	}
	*r = '\0';
	dprintf( ("in recbld FS=%o, recloc=%o\n", **FS, 
		recloc) );
	recloc->tval = REC | STR | DONTFREE;
	recloc->sval = record = rec;
	dprintf( ("in recbld FS=%o, recloc=%o\n", **FS, 
		recloc) );
	if (r > record + RECSIZE)
		/* I18NCAT_PGM_MSG */
		error(MM_ERROR,PFMTTXT(_MSG_NAWK_BUILD_GIANT_REC),
			record);
	dprintf( ("recbld = |%s|\n", record) );
	donerec = 1;
}

Cell *fieldadr(n)
{
	if (n < 0 || n >= MAXFLD)
		/* I18NCAT_PGM_MSG */
		error(MM_ERROR,PFMTTXT(_MSG_NAWK_TRYING_TO_ACCESS_FLD), n);
	return(&fldtab[n]);
}

int	errorflag	= 0;
char	errbuf[200];

static int been_here = 0;
static const char
	/* I18NCAT_PGM_MSG */
	atline[] = CATNUM_DEFMSG(_MSG_NAWK_AT_SOURCE_LINE),
	infunc[] = CATNUM_DEFMSG(_MSG_NAWK_IN_FUNCTION);

void
vyyerror(msg, a1, a2, a3, a4, a5)
char *msg, *a1, *a2, *a3, *a4, *a5;
{
	extern uchar *cmdname, *curfname;

	if (been_here++ > 2)
		return;
	pfmt(stderr, MM_ERROR, msg, a1, a2, a3, a4, a5);
	pfmt(stderr, MM_NOSTD, atline, lineno);
	if (curfname != NULL)
		pfmt(stderr, MM_NOSTD, infunc, curfname);
	fprintf(stderr, "\n");
	errorflag = 2;
	eprint();
}
	
void
yyerror(s)
	uchar *s;
{
	extern uchar *cmdname, *curfname;
	static int been_here = 0;

	if (been_here++ > 2)
		return;
	pfmt(stderr, (MM_ERROR | MM_NOGET), "%s", s);
	pfmt(stderr, MM_NOSTD, atline, lineno);
	if (curfname != NULL)
		pfmt(stderr, MM_NOSTD, infunc, curfname);
	fprintf(stderr, "\n");
	errorflag = 2;
	eprint();
}

void
fpecatch()
{
	/* I18NCAT_PGM_MSG */
	error(MM_ERROR,PFMTTXT(_MSG_NAWK_FLOATING_PT_EXCEPTION));
}

extern int bracecnt, brackcnt, parencnt;

void
bracecheck()
{
	int c;
	static int beenhere = 0;

	if (beenhere++)
		return;
	while ((c = input()) != EOF && c != '\0')
		bclass(c);
	bcheck2(bracecnt, '{', '}');
	bcheck2(brackcnt, '[', ']');
	bcheck2(parencnt, '(', ')');
}

void
bcheck2(n, c1, c2)
{
	if (n == 1)
		/* I18NCAT_PGM_MSG */
		pfmt(stderr, MM_ERROR,PFMTTXT(_MSG_NAWK_MISSING_CHAR), c2);
	else if (n > 1)
		/* I18NCAT_PGM_MSG */
		pfmt(stderr, MM_ERROR,PFMTTXT(_MSG_NAWK_MISSING_NCHARS), n, c2);
	else if (n == -1)
		/* I18NCAT_PGM_MSG */
		pfmt(stderr, MM_ERROR,PFMTTXT(_MSG_NAWK_EXTRA_CHAR_NL), c2);
	else if (n < -1)
		/* I18NCAT_PGM_MSG */
		pfmt(stderr, MM_ERROR,PFMTTXT(_MSG_NAWK_EXTRA_NCHARS), -n, c2);
}

void
error(flag, msg, a1, a2, a3, a4, a5)
int flag;
char *msg, *a1, *a2, *a3, *a4, *a5;
{
	int errline;
	extern Node *curnode;
	extern uchar *cmdname;

	fflush(stdout);
	pfmt(stderr, flag, msg, a1, a2, a3, a4, a5);
	putc('\n', stderr);

	if (compile_time != 2 && NR && *NR > 0) {
		pfmt(stderr, MM_INFO,
		    /* I18NCAT_PGM_MSG */
		    PFMTTXT(_MSG_NAWK_INPUT_REC_NO), *FNR);
		if (strcmp((char*) *FILENAME, "-") != 0)
			pfmt(stderr, MM_NOSTD,
			    /* I18NCAT_PGM_MSG */
			    PFMTTXT(_MSG_NAWK_FILE_NAME), *FILENAME);
		fprintf(stderr, "\n");
	}
	errline = 0;
	if (compile_time != 2 && curnode)
		errline = curnode->lineno;
	else if (compile_time != 2 && lineno)
		errline = lineno;
	if (errline)
		/* I18NCAT_PGM_MSG */
		pfmt(stderr, MM_INFO,PFMTTXT(_MSG_NAWK_SOURCE_LINE_NO), errline);
	eprint();
	if (flag == MM_ERROR) {
		if (dbg)
			abort();
		exit(2);
	}
}

void
eprint()	/* try to print context around error */
{
	uchar *p, *q;
	int c;
	static int been_here = 0;
	extern uchar ebuf[300], *ep;

	if (compile_time == 2 || compile_time == 0 || been_here++ > 0)
		return;
	p = ep - 1;
	if (p > ebuf && *p == '\n')
		p--;
	for ( ; p > ebuf && *p != '\n' && *p != '\0'; p--)
		;
	while (*p == '\n')
		p++;
	/* I18NCAT_PGM_MSG */
	pfmt(stderr, MM_INFO,PFMTTXT(_MSG_NAWK_CONTEXT_IS_NL_TAB));
	for (q=ep-1; q>=p && *q!=' ' && *q!='\t' && *q!='\n'; q--)
		;
	for ( ; p < q; p++)
		if (*p)
			putc(*p, stderr);
	fprintf(stderr, " >>> ");
	for ( ; p < ep; p++)
		if (*p)
			putc(*p, stderr);
	fprintf(stderr, " <<< ");
	if (*ep)
		while ((c = input()) != '\n' && c != '\0' && c != EOF) {
			putc(c, stderr);
			bclass(c);
		}
	putc('\n', stderr);
	ep = ebuf;
}

void
bclass(c)
{
	switch (c) {
	case '{': bracecnt++; break;
	case '}': bracecnt--; break;
	case '[': brackcnt++; break;
	case ']': brackcnt--; break;
	case '(': parencnt++; break;
	case ')': parencnt--; break;
	}
}

double errcheck(x, s)
	double x;
	uchar *s;
{
	extern int errno;

	if (errno == EDOM) {
		errno = 0;
		/* I18NCAT_PGM_MSG */
		error(MM_WARNING,PFMTTXT(_MSG_NAWK_ARG_OUT_OF_DOMAIN), s);
		x = 1;
	} else if (errno == ERANGE) {
		errno = 0;
		/* I18NCAT_PGM_MSG */
		error(MM_WARNING,PFMTTXT(_MSG_NAWK_RESULT_OUT_OF_RANGE), s);
		x = 1;
	}
	return x;
}

void
PUTS(s) uchar *s; {
	dprintf( ("%s\n", s) );
}

isclvar(s)	/* is s of form var=something? */
	char *s;
{
	char *os = s;

	for ( ; *s; s++)
		if (!(isalnum(*s) || *s == '_'))
			break;
	return *s == '=' && s > os && *(s+1) != '=';
}

#define	MAXEXPON	38	/* maximum exponent for fp number */

isnumber(s)
register uchar *s;
{
	register int d1, d2;
	int point;
	uchar *es;

	d1 = d2 = point = 0;
	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;
	if (*s == '\0')
		return(0);	/* empty stuff isn't number */
	if (*s == '+' || *s == '-')
		s++;
	if (!isdigit(*s) && *s != '.')
		return(0);
	if (isdigit(*s)) {
		do {
			d1++;
			s++;
		} while (isdigit(*s));
	}
	if(d1 >= MAXEXPON)
		return(0);	/* too many digits to convert */
	if (*s == '.') {
		point++;
		s++;
	}
	if (isdigit(*s)) {
		d2++;
		do {
			s++;
		} while (isdigit(*s));
	}
	if (!(d1 || point && d2))
		return(0);
	if (*s == 'e' || *s == 'E') {
		s++;
		if (*s == '+' || *s == '-')
			s++;
		if (!isdigit(*s))
			return(0);
		es = s;
		do {
			s++;
		} while (isdigit(*s));
		if (s - es > 2)
			return(0);
		else if (s - es == 2 && (int)(10 * (*es-'0') + *(es+1)-'0') >= MAXEXPON)
			return(0);
	}
	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;
	if (*s == '\0')
		return(1);
	else
		return(0);
}
