/* Modified to support EUC Multibyte/Big5-Sjis/Full multibyte	*/
/*								*/
/*	1.Modified rel, substr(), length(), index() and		*/ 
/*	  ematch() functions to handle MB data.			*/		
/*	2.Changed the cataloguing mechanism.			*/
/*	3.Removed dead code (ecmp, getrnge functions and	*/
/*	  compile() related declarations).			*/
/*	4.Modified regerr() to dynamically allocate error	*/

/* Yacc productions for "expr" command: */
%{
typedef char *yystype;
#define	YYSTYPE yystype
%}

%token OR AND ADD SUBT MULT DIV REM EQ GT GEQ LT LEQ NEQ
%token A_STRING SUBSTR LENGTH INDEX NOARG MATCH

/* operators listed below in increasing precedence: */
%left OR
%left AND
%left EQ LT GT GEQ LEQ NEQ
%left ADD SUBT
%left MULT DIV REM
%left MCH
%left MATCH
%left SUBSTR
%left LENGTH INDEX
%%

/* a single `expression' is evaluated and printed: */

expression:	expr NOARG = {
			/* Nothing is written to indicate a null string - XPG4 */
			if (strcmp($1,"\0")) {
				int	rv;
				long n;

				rv = str_is_numeric($1, &n);
				if (rv) {
					printf("%s\n", $1);
					exit(0);
				} else {
					printf("%ld\n", n);
					exit(n?0:1);
				}
			}
			exit((!strcmp($1,"0")||!strcmp($1,"\0"))? 1: 0);
			}
	;


expr:	'(' expr ')' = { $$ = $2; }
	| expr OR expr   = { $$ = conj(OR, $1, $3); }
	| expr AND expr   = { $$ = conj(AND, $1, $3); }
	| expr EQ expr   = { $$ = rel(EQ, $1, $3); }
	| expr GT expr   = { $$ = rel(GT, $1, $3); }
	| expr GEQ expr   = { $$ = rel(GEQ, $1, $3); }
	| expr LT expr   = { $$ = rel(LT, $1, $3); }
	| expr LEQ expr   = { $$ = rel(LEQ, $1, $3); }
	| expr NEQ expr   = { $$ = rel(NEQ, $1, $3); }
	| expr ADD expr   = { $$ = arith(ADD, $1, $3); }
	| expr SUBT expr   = { $$ = arith(SUBT, $1, $3); }
	| expr MULT expr   = { $$ = arith(MULT, $1, $3); }
	| expr DIV expr   = { $$ = arith(DIV, $1, $3); }
	| expr REM expr   = { $$ = arith(REM, $1, $3); }
	| expr MCH expr	 = { $$ = match($1, $3); }
	| MATCH expr expr = { $$ = match($2, $3); }
	| SUBSTR expr expr expr = { $$ = substr($2, $3, $4); }
	| LENGTH expr       = { $$ = length($2); }
	| INDEX expr expr = { $$ = index($2, $3); }
	| A_STRING
	;
%%
/*	expression command */
#include <stdio.h>
#include  <locale.h>
#include  <pfmt.h>
#include <regex.h>
#include <sgi_nl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <msgs/uxcore.abi.h>
/*	Brute-force to fix LINE_MAX problem */
#ifndef _POSIX2
#define _POSIX2
#endif
#include <limits.h>

#define ESIZE	256
#define EQL(x,y) !strcmp(x,y)
long atol();
char	**Av;
int	Ac;
int	Argi;

int	str_res = 0;
int	backref = 0;
int	matchstr = 0;
char Mstring[LINE_MAX];
void yyerror();

main(argc, argv) char **argv; {
	extern char    cmd_label[];

	Ac = argc;
	Argi = 1;
	Av = argv;
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:expr");
	if( argv[1] && !strcmp(argv[1], "--") ) {
		--Ac;
		++Av;
	}

	yyparse();
}

/*
 * some error prints
 */
char    cmd_label[] = "UX:expr";

static void
regerr(preg,re_err)
regex_t  *preg;
int	re_err;
{

	char *msg;
	int nLen;

	nLen = regerror(re_err, preg, (char *)NULL,(size_t)0);
	msg = (char *) malloc(nLen * sizeof(char));
	if (msg == NULL) {
		fprintf(stderr, GETTXT(_MSG_EXPR_MALLOC_FAILURE));
		exit(3);
	}

	(void)regerror(re_err, preg, msg, nLen);

	_sgi_nl_error(SGINL_NOSYSERR, cmd_label,
	    GETTXT(_MSG_EXPR_REG_EXPR));
	_sgi_nl_error(SGINL_NOSYSERR, cmd_label, "%s", msg);
	exit(2);
}

char *operators[] = { "|", "&", "+", "-", "*", "/", "%", ":",
	"=", "==", "<", "<=", ">", ">=", "!=",
	"match", "substr", "length", "index", "\0" };
int op[] = { OR, AND, ADD,  SUBT, MULT, DIV, REM, MCH,
	EQ, EQ, LT, LEQ, GT, GEQ, NEQ,
	MATCH, SUBSTR, LENGTH, INDEX };
yylex() {
	register char *p;
	register i;

	if(Argi >= Ac) return NOARG;

	p = Av[Argi++];

	/*if(((*p == '(') || (*p == ')')) &&
		 (((p+1) == NULL) || ((p+1) && isspace(*(p+1)))))*/
/*	if((*p == '(') || (*p == ')'))*/
	if(!strcmp(p,"(") || !strcmp(p,")"))
		return (int)*p;
	for(i = 0; *operators[i]; ++i)
		if(EQL(operators[i], p)) {
			if (i == 16)
				str_res = 1;
			if (i == 7)
				matchstr = 1;
			else
				matchstr = 0;
			return op[i];
		}

	yylval = p;
	return A_STRING;
}

char *
rel(op, r1, r2)
register char *r1, *r2;
{
	register long i;

	if((ematch(r1, "^-*[0-9]*$") || ematch(r1, "^[0-9]*$"))
		&& (ematch(r2, "^-*[0-9]*$") || ematch(r2, "^[0-9]*$")))
		i = atol(r1) - atol(r2);
	else
		i = strcoll(r1, r2);
	switch(op) {
	case EQ: i = i==0; break;
	case GT: i = i>0; break;
	case GEQ: i = i>=0; break;
	case LT: i = i<0; break;
	case LEQ: i = i<=0; break;
	case NEQ: i = i!=0; break;
	}
	return i? "1": "0";
}

char *
arith(op, r1, r2)
char *r1, *r2;
{
	long i1, i2;
	register char *rv;

	if(!((ematch(r1, "^[0-9]*$") || ematch(r1, "^-[0-9]*$")) &&
	     (ematch(r2, "^[0-9]*$") || ematch(r2, "^-[0-9]*$"))))
		yyerror(GETTXT(_MSG_EXPR_NON_NUM_ARG));
	i1 = atol(r1);
	i2 = atol(r2);

	switch(op) {
	case ADD: i1 = i1 + i2; break;
	case SUBT: i1 = i1 - i2; break;
	case MULT: i1 = i1 * i2; break;
	case DIV: 
		if (i2 == 0)
			yyerror(GETTXT(_MSG_EXPR_DIV_ZERO));
		i1 = i1 / i2; break;
	case REM: 
		if (i2 == 0)
			yyerror(GETTXT(_MSG_EXPR_DIV_ZERO));
		i1 = i1 % i2; break;
	}
	rv = malloc(16);
	(void)sprintf(rv, "%ld", i1);
	return rv;
}
char *
conj(op, r1, r2)
char *r1, *r2;
{
	register char *rv;

	switch(op) {

	case OR:
		if(EQL(r1, "0")
		|| EQL(r1, ""))
		/* This code is not used for now as we have
		 * a waiver
		 */
#ifdef	XPG4
			if(EQL(r2, "0")
			|| EQL(r2, ""))
				rv = "0";
			else
#endif
				rv = r2;
		else
			rv = r1;
		break;
	case AND:
		if(EQL(r1, "0")
		|| EQL(r1, ""))
			rv = "0";
		else if(EQL(r2, "0")
		|| EQL(r2, ""))
			rv = "0";
		else
			rv = r1;
		break;
	}
	return rv;
}

char *substr(v, s, w) char *v, *s, *w; {
register si, wi;
register char *res;
register int nLen;

	si = atol(s);
	wi = atol(w);

	while(--si) if(*v) {
			nLen = mblen(v, MB_CUR_MAX);
			if(nLen == -1)	{
				yyerror(GETTXT(_MSG_EXPR_ILLEGAL_CHAR_SEQ));
			}
			v += nLen;
		    }

	res = v;

	while(wi--) if(*v) {
			nLen = mblen(v, MB_CUR_MAX);
			if(nLen == -1)	{
				yyerror(GETTXT(_MSG_EXPR_ILLEGAL_CHAR_SEQ));
			}
			v += nLen;
		    }

	*v = '\0';
	return res;
}

char *length(s) register char *s; {
	register i = 0;
	register char *rv;
	register int nLen;

	while(*s) {
		nLen = mblen(s, MB_CUR_MAX);
		if(nLen == -1)	{
			yyerror(GETTXT(_MSG_EXPR_ILLEGAL_CHAR_SEQ));
		}
		s += nLen;
		++i;
	}

	rv = malloc(8);
	(void)sprintf(rv, "%d", i); 
	return rv;
}

char *index(s, t) char *s, *t; {
	register i, j;
	register char *rv;
	register int nLenS, nLenT;
	register wchar_t *wS, *wT;

	nLenS = strlen(s) + 1;
	nLenT = strlen(t) + 1;
	wS = (wchar_t *) malloc(nLenS * sizeof(wchar_t));
	if(wS == NULL)	{
		fprintf(stderr, GETTXT(_MSG_EXPR_MALLOC_FAILURE));
		exit(3);
	}
	wT = (wchar_t *) malloc(nLenT * sizeof(wchar_t));
	if(wT == NULL)	{
		fprintf(stderr, GETTXT(_MSG_EXPR_MALLOC_FAILURE));
		exit(3);
	}

	if(mbstowcs(wS, s, nLenS) == -1 && errno == EILSEQ)	{
		yyerror(GETTXT(_MSG_EXPR_ILLEGAL_CHAR_SEQ));
	}
	if(mbstowcs(wT, t, nLenT) == -1 && errno == EILSEQ)	{
		yyerror(GETTXT(_MSG_EXPR_ILLEGAL_CHAR_SEQ));
	}

	for(i = 0; wS[i] ; ++i)
		for(j = 0; wT[j] ; ++j)
			if(wS[i]==wT[j]) {
				(void)sprintf(rv = malloc(8), "%d", ++i);
				return rv;
			}
	return "0";
}

char *
match(s, p)
char	*s;
char	*p;
{
	register char *rv;
	register	char	*q=(char *)p;
	register	int	m;
	static char *expbuf;

	expbuf = (char *)malloc(strlen(p)+2);
	if (q && *q != '^')		/* No need to add anchor if it already */
		strcpy(expbuf,"^");	/* there */
	else
		strcpy(expbuf,"");

	strcat(expbuf,(char *)p);
	m = ematch(s, expbuf);
	
	if (m) {
		if(backref) {
			rv = malloc(strlen(Mstring)+1);
			strcpy(rv, Mstring);
		} else
			(void)sprintf(rv = malloc(8), "%d", m);
	} else {
		if(backref)
			rv = "";
		else
			rv = "0";
	}
	free(expbuf);

	return rv;
}

int
ematch(s, p)
char *s;
register char *p;
{
	register int ep;
	regex_t  preg;   /* data structure for regex */
	regmatch_t  pmatch[2];
	int err_ret;
	size_t	nmatch = (size_t)1;		/* Default value */
	register int i, nLen, nChars = 0;

	ep = regcomp(&preg, p, 0);
	if(ep)
	    regerr(&preg,ep);
	if (preg.re_nsub)
		nmatch = (size_t)2;		/* Get \1 information back */
	err_ret=regexec(&preg, s, (size_t)nmatch, pmatch, 0);

	/* Abort if there is an error */
	if (err_ret  && err_ret != REG_NOMATCH)
			regerr(&preg,err_ret);
	
	backref = 0;	/* Reset for each Regular expression evaluation */
	if (err_ret == 0) {
		/* Extract back reference info */
		if (preg.re_nsub && !backref) {
			register	int slen;
			slen = (pmatch[1].rm_eo >= pmatch[1].rm_so)?pmatch[1].rm_eo - pmatch[1].rm_so : 1;

			backref++;
			if (slen) {
				strncpy(Mstring,s+pmatch[1].rm_so,slen);
				Mstring[slen] = '\0';
			}
			else
				Mstring[0] = 0;
		}

		for(i = 0; i < pmatch[0].rm_eo; nChars++)	{
			nLen = mblen(s + i, MB_CUR_MAX);
			if(nLen == -1)	{
				yyerror(GETTXT(_MSG_EXPR_ILLEGAL_CHAR_SEQ));
			}
			i += nLen;
		}
		return(nChars);
	}
	if (preg.re_nsub)
		backref++;

	return(0);
}

static char *sccsid = "@(#)expr.y	4.8 (Berkeley) 6/24/90";
void
yyerror(const char *s)
{
	fprintf(stderr, "%s\n", s);
	exit(2);
}

static int
str_is_numeric(s, n)
register	char	*s;
register	long	*n;
{
	register	char	*p = s;

	if(p && *p == '-')
		p++;

	if (!p || *p == '\0')
		return(1);
			
	for(;p && *p; p++)
		if (!isdigit((int)*p))
			return (1);

	if (str_res)
		return(1);	/* Substring operation */

	*n = atol(s);	
	return (matchstr && backref && (strlen(s) > 1)? 1 : 0);
}
