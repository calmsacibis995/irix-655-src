/*
 *	$Id: parse.c,v 1.7 1998/09/18 19:47:22 sherwood Exp $
 */
/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)awk:parse.c	2.9"

/***********************I18N File Header******************************
File                  : parse.c

Compatibility Options : No support/Improper support/ EUC single byte/
                        EUC multibyte/Sjis-Big5/Full multibyte/Unicode

Old Compatibility     : EUC single byte/EUC Multibyte/ Big5-Sjis

Present Compatibility : EUC single byte/EUC Multibyte/ Big5-Sjis

Type of Application (Normal/Important/Critical) : Critical 

Optimization Level (EUC & Single byte/Single byte/Not optimized)
                      : Not optimized

Change History        : 14 July 1997         HCL
			cataloging modifications are done. 
************************End of I18N File Header**********************/

#define DEBUG
#include <stdio.h>
#include <pfmt.h>
#include "awk.h"
#include "y.tab.h"
#include <msgs/uxawk.h>

extern const char outofspace[];

Node *nodealloc(n)
{
	register Node *x;
	x = (Node *) malloc(sizeof(Node) + (n-1)*sizeof(Node *));
	if (x == NULL)
		error(MM_ERROR, outofspace, "nodealloc");
	x->nnext = NULL;
	x->lineno = lineno;
	return(x);
}

Node *exptostat(a) Node *a;
{
	a->ntype = NSTAT;
	return(a);
}

Node *node1(a,b) Node *b;
{
	register Node *x;
	x = nodealloc(1);
	x->nobj = a;
	x->narg[0]=b;
	return(x);
}

Node *node2(a,b,c) Node *b, *c;
{
	register Node *x;
	x = nodealloc(2);
	x->nobj = a;
	x->narg[0] = b;
	x->narg[1] = c;
	return(x);
}

Node *node3(a,b,c,d) Node *b, *c, *d;
{
	register Node *x;
	x = nodealloc(3);
	x->nobj = a;
	x->narg[0] = b;
	x->narg[1] = c;
	x->narg[2] = d;
	return(x);
}

Node *node4(a,b,c,d,e) Node *b, *c, *d, *e;
{
	register Node *x;
	x = nodealloc(4);
	x->nobj = a;
	x->narg[0] = b;
	x->narg[1] = c;
	x->narg[2] = d;
	x->narg[3] = e;
	return(x);
}

Node *stat3(a,b,c,d) Node *b, *c, *d;
{
	register Node *x;
	x = node3(a,b,c,d);
	x->ntype = NSTAT;
	return(x);
}

Node *op2(a,b,c) Node *b, *c;
{
	register Node *x;
	x = node2(a,b,c);
	x->ntype = NEXPR;
	return(x);
}

Node *op1(a,b) Node *b;
{
	register Node *x;
	x = node1(a,b);
	x->ntype = NEXPR;
	return(x);
}

Node *stat1(a,b) Node *b;
{
	register Node *x;
	x = node1(a,b);
	x->ntype = NSTAT;
	return(x);
}

Node *op3(a,b,c,d) Node *b, *c, *d;
{
	register Node *x;
	x = node3(a,b,c,d);
	x->ntype = NEXPR;
	return(x);
}

Node *op4(a,b,c,d,e) Node *b, *c, *d, *e;
{
	register Node *x;
	x = node4(a,b,c,d,e);
	x->ntype = NEXPR;
	return(x);
}

Node *stat2(a,b,c) Node *b, *c;
{
	register Node *x;
	x = node2(a,b,c);
	x->ntype = NSTAT;
	return(x);
}

Node *stat4(a,b,c,d,e) Node *b, *c, *d, *e;
{
	register Node *x;
	x = node4(a,b,c,d,e);
	x->ntype = NSTAT;
	return(x);
}

Node *valtonode(a, b) Cell *a;
{
	register Node *x;

	a->ctype = OCELL;
	a->csub = b;
	x = node1(0, (Node *) a);
	x->ntype = NVALUE;
	return(x);
}

Node *rectonode()
{
	/* return valtonode(lookup("$0", symtab), CFLD); */
	return valtonode(recloc, CFLD);
}

Node *makearr(p) Node *p;
{
	Cell *cp;

	if (isvalue(p)) {
		cp = (Cell *) (p->narg[0]);
		if (isfunc(cp))
			/* I18NCAT_PGM_MSG */
			vyyerror(CATNUM_DEFMSG(_MSG_NAWK_NOT_FUNC_AN_ARRAY),
				cp->nval);
		else if (!isarr(cp)) {
			xfree(cp->sval);
			cp->sval = (uchar *) makesymtab(NSYMTAB);
			cp->tval = ARR;
		}
	}
	return p;
}

Node *pa2stat(a,b,c) Node *a, *b, *c;
{
	register Node *x;
	x = node4(PASTAT2, a, b, c, (Node *) paircnt);
	paircnt++;
	x->ntype = NSTAT;
	return(x);
}

Node *linkum(a,b) Node *a, *b;
{
	register Node *c;

	if (errorflag)	/* don't link things that are wrong */
		return a;
	if (a == NULL) return(b);
	else if (b == NULL) return(a);
	for (c = a; c->nnext != NULL; c = c->nnext)
		;
	c->nnext = b;
	return(a);
}

void
defn(v, vl, st)	/* turn on FCN bit in definition */
	Cell *v;
	Node *st, *vl;	/* body of function, arglist */
{
	Node *p;
	int n;

	if (isarr(v)) {
		/* I18NCAT_PGM_MSG */
		vyyerror(CATNUM_DEFMSG(_MSG_NAWK_ARRAY_AND_FUNC_NAME),
			v->nval);
		return;
	}
	v->tval = FCN;
	v->sval = (uchar *) st;
	n = 0;	/* count arguments */
	for (p = vl; p; p = p->nnext)
		n++;
	v->fval = n;
	dprintf( ("defining func %s (%d args)\n", v->nval, n) );
}

isarg(s)	/* is s in argument list for current function? */
	uchar *s;
{
	extern Node *arglist;
	Node *p = arglist;
	int n;

	for (n = 0; p != 0; p = p->nnext, n++)
		if (strcmp(((Cell *)(p->narg[0]))->nval, s) == 0)
			return n;
	return -1;
}
