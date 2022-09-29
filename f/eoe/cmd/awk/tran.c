/*
 *	$Id: tran.c,v 1.14 1998/09/18 19:47:22 sherwood Exp $
 */
/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)awk:tran.c	2.15"

/***********************I18N File Header******************************
File                  : tran.c

Compatibility Options : No support/Improper support/ EUC single byte/
                        EUC multibyte/Sjis-Big5/Full multibyte/Unicode

Old Compatibility     : No support

Present Compatibility :  EUC single byte/EUC Multibyte/ Big5-Sjis

Type of Application (Normal/Important/Critical) : Critical 

Optimization Level (EUC & Single byte/Single byte/Not optimized)
                      : EUC & Single byte

Change History        :  16 Sept 1997       HCL
       			 qstring() is modified to safe guard char 
			 processing for multibyte.
Change History        :  13 Jan 1998       HCL
        * VSC BUG FIX : for failed assertion#(202,232,377) on
            Baseline version, in VSC .
************************End of I18N File Header**********************/


#define	DEBUG
#include <stdio.h>
#include <ctype.h>
#include <pfmt.h>
#include <string.h>
#include "awk.h"
#include "y.tab.h"
#include <i18n_capable.h>
#include <wchar.h>
#include <msgs/uxawk.h>

#ifdef isnumber
#undef isnumber
#endif

#define	FULLTAB	2	/* rehash when table gets this x full */
#define	GROWTAB 4	/* grow table by this factor */

Array	*symtab;	/* main symbol table */

uchar	**FS;		/* initial field sep */
uchar	**RS;		/* initial record sep */
uchar	**OFS;		/* output field sep */
uchar	**ORS;		/* output record sep */
uchar	**OFMT;		/* output format for numbers*/
Awkfloat *NF;		/* number of fields in current record */
Awkfloat *NR;		/* number of current record */
Awkfloat *FNR;		/* number of current record in current file */
uchar	**FILENAME;	/* current filename argument */
Awkfloat *ARGC;		/* number of arguments from command line */
uchar	**SUBSEP;	/* subscript separator for a[i,j,k]; default \034 */
Awkfloat *RSTART;	/* start of re matched with ~; origin 1 (!) */
Awkfloat *RLENGTH;	/* length of same */
uchar   **CONVFMT;      /* conversion format for numbers to string*/

Cell	*recloc;	/* location of record */
Cell	*nrloc;		/* NR */
Cell	*nfloc;		/* NF */
Cell	*fnrloc;	/* FNR */
Array	*ARGVtab;	/* symbol table containing ARGV[...] */
Array	*ENVtab;	/* symbol table containing ENVIRON[...] */
Cell	*rstartloc;	/* RSTART */
Cell	*rlengthloc;	/* RLENGTH */
Cell	*symtabloc;	/* SYMTAB */

Cell	*nullloc;
Node	*nullnode;	/* zero&null, converted into a node for comparisons */

extern Node *valtonode();
extern Cell fldtab[];

static	const	char
	/* I18NCAT_PGM_MSG */
	assigntovid[] = CAT_MSG_NUM(_MSG_NAWK_ASSIGN_TO),
	assigntov[] = DEF_MSG(_MSG_NAWK_ASSIGN_TO);

const char
	/* I18NCAT_PGM_MSG */
	readvofid[] =  CAT_MSG_NUM(_MSG_NAWK_READ_VALUE_OF),
	readvof[] =  DEF_MSG(_MSG_NAWK_READ_VALUE_OF),
	outofspace[] =  CATNUM_DEFMSG(_MSG_NAWK_OUT_OF_SPACE_IN_STR),
	nlstring[] =  CATNUM_DEFMSG(_MSG_NAWK_NL_IN_STRING);

void rehash(), funnyvar();

void
syminit()
{
	symtab = makesymtab(NSYMTAB);
	setsymtab("0", "0", 0.0, NUM|STR|CON|DONTFREE, symtab);
	/* this is used for if(x)... tests: */
	nullloc = setsymtab("$zero&null", "", 0.0, NUM|STR|CON|DONTFREE, symtab);
	nullnode = valtonode(nullloc, CCON);
	/* recloc = setsymtab("$0", record, 0.0, REC|STR|DONTFREE, symtab); */
	recloc = &fldtab[0];
	FS = &setsymtab("FS", " ", 0.0, STR|DONTFREE, symtab)->sval;
	RS = &setsymtab("RS", "\n", 0.0, STR|DONTFREE, symtab)->sval;
	OFS = &setsymtab("OFS", " ", 0.0, STR|DONTFREE, symtab)->sval;
	ORS = &setsymtab("ORS", "\n", 0.0, STR|DONTFREE, symtab)->sval;
	OFMT = &setsymtab("OFMT", "%.6g", 0.0, STR|DONTFREE, symtab)->sval;
	FILENAME = &setsymtab("FILENAME", "-", 0.0, STR|DONTFREE, symtab)->sval;
	nfloc = setsymtab("NF", "", 0.0, NUM, symtab);
	NF = &nfloc->fval;
	nrloc = setsymtab("NR", "", 0.0, NUM, symtab);
	NR = &nrloc->fval;
	fnrloc = setsymtab("FNR", "", 0.0, NUM, symtab);
	FNR = &fnrloc->fval;
	SUBSEP = &setsymtab("SUBSEP", "\034", 0.0, STR|DONTFREE, symtab)->sval;
	rstartloc = setsymtab("RSTART", "", 0.0, NUM, symtab);
	RSTART = &rstartloc->fval;
	rlengthloc = setsymtab("RLENGTH", "", 0.0, NUM, symtab);
	RLENGTH = &rlengthloc->fval;
	symtabloc = setsymtab("SYMTAB", "", 0.0, ARR, symtab);
	symtabloc->sval = (uchar *) symtab;
}

void
arginit(ac, av)
	int ac;
	uchar *av[];
{
	Cell *cp;
	Array *makesymtab();
	int i;
	uchar temp[5];

	for (i = 1; i < ac; i++)	/* first make FILENAME first real argument */
		if (!isclvar(av[i])) {
			setsval(lookup("FILENAME", symtab), av[i]);
			break;
		}
	ARGC = &setsymtab("ARGC", "", (Awkfloat) ac, NUM, symtab)->fval;
	cp = setsymtab("ARGV", "", 0.0, ARR, symtab);
	ARGVtab = makesymtab(NSYMTAB);	/* could be (int) ARGC as well */
	cp->sval = (uchar *) ARGVtab;
	for (i = 0; i < ac; i++) {
		sprintf((char *)temp, "%d", i);
		if (isnumber(*av))
			setsymtab(temp, *av, atof(*av), STR|NUM, ARGVtab);
		else
			setsymtab(temp, *av, 0.0, STR, ARGVtab);
		av++;
	}
}

void
envinit(envp)
	uchar *envp[];
{
	Cell *cp;
	Array *makesymtab();
	uchar *p;

	cp = setsymtab("ENVIRON", "", 0.0, ARR, symtab);
	ENVtab = makesymtab(NSYMTAB);
	cp->sval = (uchar *) ENVtab;
	for ( ; *envp; envp++) {
		if ((p = (uchar *) strchr((char *) *envp, '=')) == NULL)	/* index() on bsd */
			continue;
		*p++ = 0;	/* split into two strings at = */
		if (isnumber(p))
			setsymtab(*envp, p, atof(p), STR|NUM, ENVtab);
		else
			setsymtab(*envp, p, 0.0, STR, ENVtab);
		p[-1] = '=';	/* restore in case env is passed down to a shell */
	}
}

Array *makesymtab(n)
	int n;
{
	Array *ap;
	Cell **tp;

	ap = (Array *) malloc(sizeof(Array));
	tp = (Cell **) calloc(n, sizeof(Cell *));
	if (ap == NULL || tp == NULL)
		error(MM_ERROR, outofspace, "makesymtab");
	ap->nelem = 0;
	ap->size = n;
	ap->tab = tp;
	return(ap);
}

void
freesymtab(ap)	/* free symbol table */
	Cell *ap;
{
	Cell *cp, *temp;
	Array *tp;
	int i;

	if (!isarr(ap))
		return;
	tp = (Array *) ap->sval;
	if (tp == NULL)
		return;
	for (i = 0; i < tp->size; i++) {
		for (cp = tp->tab[i]; cp != NULL; cp = temp) {
			xfree(cp->nval);
			if (freeable(cp))
				xfree(cp->sval);
			temp = cp->cnext;	/* avoids freeing then using */
			free(cp);
		}
	}
	free(tp->tab);
	free(tp);
}

void
freeelem(ap, s)		/* free elem s from ap (i.e., ap["s"] */
	Cell *ap;
	uchar *s;
{
	Array *tp;
	Cell *p, *prev = NULL;
	int h;
	
	tp = (Array *) ap->sval;
	h = hash(s, tp->size);
	for (p = tp->tab[h]; p != NULL; prev = p, p = p->cnext)
		if (strcmp((char *) s, (char *) p->nval) == 0) {
			if (prev == NULL)	/* 1st one */
				tp->tab[h] = p->cnext;
			else			/* middle somewhere */
				prev->cnext = p->cnext;
			if (freeable(p))
				xfree(p->sval);
			free(p->nval);
			free(p);
			tp->nelem--;
			return;
		}
}

Cell *setsymtab(n, s, f, t, tp)
	uchar *n, *s;
	Awkfloat f;
	unsigned t;
	Array *tp;
{
	register int h;
	register Cell *p;
	Cell *lookup();

	if (n != NULL && (p = lookup(n, tp)) != NULL) {
		dprintf( ("setsymtab found %o: n=%s", p, p->nval) );
		dprintf( (" s=\"%s\" f=%g t=%o\n", p->sval, p->fval, p->tval) );
		return(p);
	}
	p = (Cell *) malloc(sizeof(Cell));
	if (p == NULL)
		/* I18NCAT_PGM_MSG */
		error(MM_ERROR, CATNUM_DEFMSG(_MSG_NAWK_SYMBOL_TABLE_OVERFLOW), n);
	p->nval = tostring(n);
	p->sval = s ? tostring(s) : tostring("");
	p->fval = f;
	p->tval = t;
#ifdef sgi
       p->csub = CVAR;
#endif
	tp->nelem++;
	if (tp->nelem > FULLTAB * tp->size)
		rehash(tp);
	h = hash(n, tp->size);
	p->cnext = tp->tab[h];
	tp->tab[h] = p;
	dprintf( ("setsymtab set %o: n=%s", p, p->nval) );
	dprintf( (" s=\"%s\" f=%g t=%o\n", p->sval, p->fval, p->tval) );
	return(p);
}

hash(s, n)	/* form hash value for string s */
	register uchar *s;
	int n;
{
	register unsigned hashval;

	for (hashval = 0; *s != '\0'; s++)
		hashval = (*s + 31 * hashval);
	return hashval % n;
}

void
rehash(tp)	/* rehash items in small table into big one */
	Array *tp;
{
	int i, nh, nsz;
	Cell *cp, *op, **np;

	nsz = GROWTAB * tp->size;
	np = (Cell **) calloc(nsz, sizeof(Cell *));
	if (np == NULL)
		error(MM_ERROR, outofspace, "rehash");
	for (i = 0; i < tp->size; i++) {
		for (cp = tp->tab[i]; cp; cp = op) {
			op = cp->cnext;
			nh = hash(cp->nval, nsz);
			cp->cnext = np[nh];
			np[nh] = cp;
		}
	}
	free(tp->tab);
	tp->tab = np;
	tp->size = nsz;
}

Cell *lookup(s, tp)	/* look for s in tp */
	register uchar *s;
	Array *tp;
{
	register Cell *p, *prev = NULL;
	int h;

	h = hash(s, tp->size);
	for (p = tp->tab[h]; p != NULL; prev = p, p = p->cnext)
		if (strcmp((char *) s, (char *) p->nval) == 0)
			return(p);	/* found it */
	return(NULL);			/* not found */
}

Awkfloat setfval(vp, f)
	register Cell *vp;
	Awkfloat f;
{
	if ((vp->tval & (NUM | STR)) == 0) 
		funnyvar(vp, gettxt(assigntovid, assigntov));
	if (vp->tval & FLD) {
		donerec = 0;	/* mark $0 invalid */
		if (vp-fldtab > *NF)
			newfld(vp-fldtab);
		dprintf( ("setting field %d to %g\n", vp-fldtab, f) );
	} else if (vp->tval & REC) {
		donefld = 0;	/* mark $1... invalid */
		donerec = 1;
	}
	vp->tval &= ~STR;	/* mark string invalid */
	vp->tval |= NUM;	/* mark number ok */
	dprintf( ("setfval %o: %s = %g, t=%o\n", vp, vp->nval, f, vp->tval) );
	return vp->fval = f;
}

void
funnyvar(vp, rw)
	Cell *vp;
	char *rw;
{
	if (vp->tval & ARR)
		/* I18NCAT_PGM_MSG */
		error(MM_ERROR, CATNUM_DEFMSG(_MSG_NAWK_CANNOT_ITS_AN_ARRAY_NAME),
			rw, vp->nval);
	if (vp->tval & FCN)
		/* I18NCAT_PGM_MSG */
		error(MM_ERROR, CATNUM_DEFMSG(_MSG_NAWK_CANNOT_ITS_A_FUNC),
			rw, vp->nval);
	/* I18NCAT_PGM_MSG */
	error(MM_ERROR,  CATNUM_DEFMSG(_MSG_NAWK_FUNNY_VAR),
		vp, vp->nval, vp->sval, vp->fval, vp->tval);
}

uchar *setsval(vp, s)
register Cell *vp;
uchar *s;
{
	if ((vp->tval & (NUM | STR)) == 0)
		funnyvar(vp, gettxt(assigntovid, assigntov));
	if (vp->tval & FLD) {
		donerec = 0;	/* mark $0 invalid */
		if (vp-fldtab > *NF)
			newfld(vp-fldtab);
		dprintf( ("setting field %d to %s\n", vp-fldtab, s) );
	} else if (vp->tval & REC) {
		donefld = 0;	/* mark $1... invalid */
		donerec = 1;
	}
	vp->tval &= ~NUM;
	vp->tval |= STR;
	if (freeable(vp))
		xfree(vp->sval);
	vp->tval &= ~DONTFREE;
	dprintf( ("setsval %o: %s = \"%s\", t=%o\n", vp, vp->nval, s, vp->tval) );
	return(vp->sval = tostring(s));
}

Awkfloat r_getfval(vp)
register Cell *vp;
{
	/* if (vp->tval & ARR)
		ERROR "Illegal reference to array %s", vp->nval FATAL;
		return 0.0; */
	if ((vp->tval & (NUM | STR)) == 0)
		funnyvar(vp, gettxt(readvofid, readvof));
	if ((vp->tval & FLD) && donefld == 0)
		fldbld();
	else if ((vp->tval & REC) && donerec == 0)
		recbld();
	if (!isnum(vp)) {	/* not a number */
		vp->fval = atof(vp->sval);	/* best guess */
		if (isnumber(vp->sval) && !(vp->tval&CON))
			vp->tval |= NUM;	/* make NUM only sparingly */
	}
	dprintf( ("getfval %o: %s = %g, t=%o\n", vp, vp->nval, vp->fval, vp->tval) );
	return(vp->fval);
}

uchar *r_getsval(vp)
register Cell *vp;
{
	uchar s[100];

	/* if (vp->tval & ARR)
		ERROR "Illegal reference to array %s",
			vp->nval FATAL;
		return ""; */
	if ((vp->tval & (NUM | STR)) == 0)
		funnyvar(vp, gettxt(readvofid, readvof));
	if ((vp->tval & FLD) && donefld == 0)
		fldbld();
	else if ((vp->tval & REC) && donerec == 0)
		recbld();
	if ((vp->tval & STR) == 0) {
		if (!(vp->tval&DONTFREE))
			xfree(vp->sval);
		if ((long)vp->fval == vp->fval)
			sprintf((char *)s, "%.20g", vp->fval);
		else
		        sprintf((char *)s, (char *)*OFMT, vp->fval);
		vp->sval = tostring(s);
		vp->tval &= ~DONTFREE;
		vp->tval |= STR;
	}
	dprintf( ("getsval %o: %s = \"%s\", t=%o\n", vp, vp->nval, vp->sval, vp->tval) );

	/* SGI BUG FIX: Some places assume this returns an actual string */
	return((vp->sval) ? (vp->sval) : tostring(""));
}

uchar *tostring(s)
register uchar *s;
{
	register uchar *p;

	p = malloc(strlen((char *) s)+1);
	if (p == NULL)
		/* I18NCAT_PGM_MSG */
		error(MM_ERROR,  CATNUM_DEFMSG(_MSG_NAWK_OUT_OF_SPACE_IN_TOSTRING), s);
	strcpy((char *) p, (char *) s);
	return(p);
}

uchar *qstring(s, delim)	/* collect string up to delim */
	uchar *s;
	int delim;
{
	uchar *q;
	int c, n;

   /* I18NCAT_CHAR_BOUNDARY */
   if ((I18N_SBCS_CODE) || (I18N_EUC_CODE))
	for (q = cbuf; (c = *s) != delim; s++) {
		if (q >= cbuf + CBUFLEN - 1)
			/* I18NCAT_PGM_MSG */
			vyyerror(CATNUM_DEFMSG(_MSG_NAWK_STRING_TOO_LONG), cbuf);
		else if (c == '\n')
			vyyerror(nlstring, cbuf);
		else if (c != '\\')
			*q++ = c;
		else	/* \something */	
			switch (c = *++s) {
			case 'n':	*q++ = '\n'; break;
			case 't':	*q++ = '\t'; break;
			case 'b':	*q++ = '\b'; break;
			case 'f':	*q++ = '\f'; break;
			case 'r':	*q++ = '\r'; break;
			default:
				if (!isdigit(c)) {
					*q++=*(s-1);
					*q++ = c;
					break;
				}
				n = c - '0';
				if (isdigit(s[1])) {
					n = 8 * n + *++s - '0';
					if (isdigit(s[1]))
						n = 8 * n + *++s - '0';
				}
				*q++ = n;
				break;
			}
   } else {
	wint_t wc,wdelim; 
	int nBytes=0;
	wdelim = (wint_t)delim;

	nBytes=mbtowc(&wc,s,MB_CUR_MAX);
	for (q = cbuf; wc != wdelim; ) {
		if (q >= cbuf + CBUFLEN - 1)
			/* I18NCAT_PGM_MSG */
			vyyerror(CATNUM_DEFMSG(_MSG_NAWK_STRING_TOO_LONG), cbuf);
		else if (wc == L'\n')
			vyyerror(nlstring, cbuf);
		else if (wc != L'\\') {
			while (nBytes--)
				*q++ = *s++;
			s--;
		} else	/* \something */	
		switch (c = *++s) {
		case 'n':	*q++ = '\n'; break;
		case 't':	*q++ = '\t'; break;
		case 'b':	*q++ = '\b'; break;
		case 'f':	*q++ = '\f'; break;
		case 'r':	*q++ = '\r'; break;
		default:
			if (!isdigit(c)) { /* copy only multi byte of \multi byte */
				*q++=*(s-1);
				nBytes = mblen(s,MB_CUR_MAX);
				while (nBytes--)
					*q++ = *s++;
				s--;
				break;
			}
			n = c - '0';
			if (isdigit(s[1])) {
				n = 8 * n + *++s - '0';
				if (isdigit(s[1]))
					n = 8 * n + *++s - '0';
			}
			*q++ = n;
			break;
		}
		s++;
		nBytes = mbtowc(&wc,s,MB_CUR_MAX);
	}
  }
	*q = '\0';
	return cbuf;
}