/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:sh/string.c	1.2.4.1"

/*
 * string processing routines for Korn shell
 *
 */

#include	"defs.h"
#include	"sym.h"
#ifdef MULTIBYTE
#   include	"national.h"
#endif /* MULTIBYTE */

extern wchar_t	*utos();

/*
 * converts integer n into an unsigned decimal string
 */

wchar_t *sh_itos(n)
int n;
/*@
	return x satisfying atol(x)==n;
@*/ 
{
	return(utos((ulong)n,10));
}


/*
 * look for the substring <old> in <string> and replace with <new>
 * The new string is put on top of the stack
 */

wchar_t *sh_substitute(string,old,new)
const wchar_t *string;
const wchar_t *old;
wchar_t *new;
/*@
	assume string!=NULL && old!=NULL && new!=NULL;
	return x satisfying x==NULL ||
		strlen(x)==(strlen(in string)+strlen(in new)-strlen(in old));
@*/
{
	register const wchar_t *sp = string;
	register const wchar_t *cp;
	const wchar_t *savesp = NIL;
	stakseek(0);
	if(!sp || *sp==0L)
		return(NIL);
	if(*(cp=old) == 0L)
		goto found;
	do
	{
	/* skip to first character which matches start of old */
		while(*sp && (savesp==sp || *sp != *cp))
		{
#ifdef MULTIBYTE
			/* skip a whole character at a time */
			int c = *sp;
			c = echarset(c);
			c = in_csize(c) + (c>=2);
			while(c-- > 0)
#endif /* MULTIBYTE */
			stakputc(*sp++);
		}
		if(*sp == 0L)
			return(NIL);
		savesp = sp;
	        for(;*cp;cp++)
		{
			if(*cp != *sp++)
				break;
		}
		if(*cp==0L)
		/* match found */
			goto found;
		sp = savesp;
		cp = old;
	}
	while(*sp);
	return(NIL);

found:
	/* copy new */
	stakputs(new);
	/* copy rest of string */
	stakputs(sp);
	return((wchar_t *)wc_stakfreeze(WC_SZ));
}

/*
 * put string v onto the heap and return the heap pointer
 */

wchar_t *sh_heap(v)
register const wchar_t *v;
/*@
	return x satisfying (in v? strcmp(v,x)==0: x==0);
@*/
{
	register wchar_t *p;
	if(v)
	{
		sh_copy(v,p=(wchar_t *)malloc((wcslen(v)+1)*sizeof(wchar_t)));
		return(p);
	}
	else
		return(0L);
}


/*
 * TRIM(sp)
 * Remove escape characters from characters in <sp> and eliminate quoted nulls.
 */

void	sh_trim(sp)
register wchar_t *	sp;
/*@
	assume sp!=NULL;
	promise  strlen(in sp) <= in strlen(sp);
@*/
{
	register wchar_t *dp;
	register int c;
	if(sp)
	{
		dp = sp;
		while(c= *sp++)
		{
			if(c == ESCAPE)
				c = *sp++;
			if(c)
				*dp++ = c;
		}
		*dp = 0L;
	}
}

/*
 * copy string a to string b and return a pointer to the end of the string
 */

wchar_t *sh_copy(a,b)
register const wchar_t *a;
register wchar_t *b;
/*@
	assume a!=NULL && b!= NULL;
	promise strcmp(in a,in b)==0;
	return x satisfying (x-(in b))==strlen(in a);
 @*/
{
	while(*b++ = *a++);
	return(--b);
}

/*
 * G. S. Fowler
 * AT&T Bell Laboratories
 *
 * apply file permission expression expr to perm
 *
 * each expression term must match
 *
 *	[ugo]*[-&+|=]?[rwxst0-7]*
 *
 * terms may be combined using ,
 *
 * if non-null, e points to the first unrecognized char in expr
 */


#ifndef S_IRWXU
#   ifndef S_IREAD
#	define S_IREAD		00400
#	define S_IWRITE		00200
#	define S_IEXEC		00100
#   endif
#   ifndef S_ISUID
#	define S_ISUID		04000
#   endif
#   ifndef S_ISGID
#	define S_ISGID		02000
#   endif
#   ifndef S_ISVTX
#	define S_ISVTX		01000
#   endif
#   ifndef S_IRUSR
#	define S_IRUSR		S_IREAD
#	define S_IWUSR		S_IWRITE
#	define S_IXUSR		S_IEXEC
#	define S_IRGRP		(S_IREAD>>3)
#	define S_IWGRP		(S_IWRITE>>3)
#	define S_IXGRP		(S_IEXEC>>3)
#	define S_IROTH		(S_IREAD>>6)
#	define S_IWOTH		(S_IWRITE>>6)
#	define S_IXOTH		(S_IEXEC>>6)
#   endif

#   define S_IRWXU		(S_IRUSR|S_IWUSR|S_IXUSR)
#   define S_IRWXG		(S_IRGRP|S_IWGRP|S_IXGRP)
#   define S_IRWXO		(S_IROTH|S_IWOTH|S_IXOTH)
#endif


int
strperm(expr, e, perm)
wchar_t*		expr;
wchar_t**		e;
register int	perm;
/*@
	assume expr!=0;
	assume e==0 || *e!=0;
@*/
{
	register wchar_t	c;
	register wint_t	typ;
	register wint_t	who;
	wint_t		num;
	wchar_t		op;

	for (;;)
	{
		op = num = who = typ = 0L;
		for (;;)
		{
			switch (c = *expr++)
			{
			case L'u':
				who |= S_ISVTX|S_ISUID|S_IRWXU;
				continue;
			case L'g':
				who |= S_ISVTX|S_ISGID|S_IRWXG;
				continue;
			case L'o':
				who |= S_ISVTX|S_IRWXO;
				continue;
			case L'a':
				who = S_ISVTX|S_ISUID|S_ISGID|S_IRWXU|S_IRWXG|S_IRWXO;
				continue;
			default:
				if (c >= L'0' && c <= L'7') c = L'=';
				expr--;
				/*FALLTHROUGH*/
			case L'+':
			case L'=':
				if(c==L'=' && !who)
					who = S_ISVTX|S_ISUID|S_ISGID|S_IRWXU|S_IRWXG|S_IRWXO;
			case L'|':
			case L'-':
			case L'&':
				op = c;
				for (;;)
				{
					switch (c = *expr++)
					{
					case L'r':
						typ |= S_IRUSR|S_IRGRP|S_IROTH;
						continue;
					case L'w':
						typ |= S_IWUSR|S_IWGRP|S_IWOTH;
						continue;
					case L'x':
						typ |= S_IXUSR|S_IXGRP|S_IXOTH;
						continue;
					case L's':
						typ |= S_ISUID|S_ISGID;
						continue;
					case L't':
						typ |= S_ISVTX;
						continue;
					case L',':
					case 0L:
						if (who) 
							typ &= who;
						switch (op)
						{
						default:
							perm &= ~who;
							/*FALLTHROUGH*/
						case L'+':
						case L'|':
							perm |= typ;
							break;
						case L'-':
							perm &= ~typ;
							break;
						case L'&':
							perm &= typ;
							break;
						}
						if (c) break;
						/*FALLTHROUGH*/
					default:
						if (e) *e = expr - 1;
						return(perm);
					}
					break;
				}
				break;
			}
			break;
		}
	}
}

/*
 * some really old systems don't have memcpy()
 */

#ifdef NOMEMCPY
#   ifdef NOBCOPY
	char *memcpy(b,a,n)
	char *b;
	register char *a;
	{
		register int n;
		register char *d = b;
		while(n--)
			*d++ = *a++;
		return(b);
	}
#   else
	char *memcpy(b,a,n)
	char *b,*a;
	{
		bcopy(a,b,n);
		return(b);
	}
#   endif /* NOBCOPY */
#endif /* NOMEMCPY */

#ifdef NOMEMSET
	char *memset(region,c,n)
	register char *sp;
	register int c,n;
	{
		register char *sp = region;
		while(n--)
			*sp++ = c;
		return(region);
	}
#endif /* NOMEMSET */
