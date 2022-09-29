/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:shlib/strmatch.c	1.3.4.1"

/*
 * D. G. Korn
 * G. S. Fowler
 * AT&T Bell Laboratories
 *
 * match shell file patterns -- derived from Bourne and Korn shell gmatch()
 *
 *	sh pattern	egrep RE	description
 *	----------	--------	-----------
 *	*		.*		0 or more chars
 *	?		.		any single char
 *	[.]		[.]		char class
 *	[!.]		[^.]		negated char class
 *	[[:.:]]		[[:.:]]		ctype class
 *	[[=.=]]		[[=.=]]		equivalence class
 *	[[.C.]] 	[[.C.]] 	(C)ollating element
 *	*(.)		(.)*		0 or more of
 *	+(.)		(.)+		1 or more of
 *	?(.)		(.)?		0 or 1 of
 *	(.)		(.)		1 of
 *	@(.)		(.)		1 of
 *	a|b		a|b		a or b
 *	a&b				a and b
 *	!(.)				none of
 *
 * \ used to escape metacharacters
 *
 *	*, ?, (, |, &, ), [, \ must be \'d outside of [...]
 *	only ] must be \'d inside [...]
 *
 * BUG: unbalanced ) terminates top level pattern
 *
 */

#include <ctype.h>
#include <regex.h>
#include "sh_config.h"
#include "shtype.h"

#ifndef isequiv
#define isequiv(a,s)	((a)==(s))
#endif


#ifdef MULTIBYTE

#include "national.h"

#define REGISTER

#define C_MASK		(3<<(7*ESS_MAXCHAR))	/* character classes	*/
#define getchar(x)	mb_getchar((unsigned char**)(&(x)))

static int		mb_getchar();

#else

#define REGISTER	register

#define get_char(x)	(*x++)

#endif

#define getsource(s,e)	(((s)>=(e))?0:get_char(s))

static wchar_t*		endmatch;
static int		minmatch;

static int		grpmatch();
static int		onematch();
static wchar_t*		gobble();

/*
 * strmatch compares the string s with the shell pattern p
 * returns 1 for match 0 otherwise
 */

int
strmatch(s, p)
register wchar_t*	s;
wchar_t*		p;
{
	minmatch = 0;
	return(grpmatch(s, p, s + wcslen(s), (wchar_t*)0));
}

/*
 * leading substring match
 * first char after end of substring returned
 * 0 returned if no match
 * m: (0-min, 1-max) match
 */

wchar_t*
submatch(s, p, m)
register wchar_t*	s;
wchar_t*		p;
int		m;
{
	endmatch = 0L;
	minmatch = !m;
	(void)grpmatch(s, p, s + wcslen(s), (wchar_t*)0);
	return(endmatch);
}

/*
 * match any pattern in a group
 * | and & subgroups are parsed here
 */

static int
grpmatch(s, p, e, g)
wchar_t*		s;
register wchar_t*	p;
wchar_t*		e;
wchar_t*		g;
{
	register wchar_t*	a;

	do
	{
		a = p;
		do
		{
			if (!onematch(s, a, e, g)) break;
		} while (a = gobble(a, L'&'));
		if (!a) return(1);
	} while (p = gobble(p, L'|'));
	return(0);
}

/*
 * match a single pattern
 * e is the end (0) of the substring in s
 * g marks the start of a repeated subgroup pattern
 */

static int
onematch(s, p, e, g)
wchar_t*		s;
REGISTER wchar_t*	p;
wchar_t*		e;
wchar_t*		g;
{
	register wint_t	pc;
	register wint_t	sc;
	register int	n;
	wchar_t*		olds;
	wchar_t*		oldp;

	do
	{
		olds = s;
		sc = getsource(s, e);
		switch (pc = get_char(p))
		{
		case L'(':
		case L'*':
		case L'?':
		case L'+':
		case L'@':
		case L'!':
			if (pc == L'(' || *p == L'(')
			{
				wchar_t*	subp;

				s = olds;
				oldp = p - 1;
				subp = p + (pc != L'(');
				if (!(p = gobble(subp, 0))) return(0);
				if (pc == L'*' || pc == L'?' || pc == L'+' && oldp == g)
				{
					if (onematch(s, p, e, (wchar_t*)0)) return(1);
					if (!sc || !getsource(s, e)) return(0);
				}
				if (pc == L'*' || pc == L'+') p = oldp;
				pc = (pc != L'!');
				do
				{
					if (grpmatch(olds, subp, s, (wchar_t*)0) == pc && onematch(s, p, e, oldp)) return(1);
				} while (s < e && get_char(s));
				return(0);
			}
			else if (pc == L'*')
			{
				/*
				 * several stars are the same as one
				 */

				while (*p == L'*')
					if (*(p + 1) == L'(') break;
					else p++;
				oldp = p;
				switch (pc = get_char(p))
				{
				case L'@':
				case L'!':
				case L'+':
					n = *p == L'(';
					break;
				case L'(':
				case L'[':
				case L'?':
				case L'*':
					n = 1;
					break;
				case 0L:
					endmatch = minmatch ? olds : e;
					/*FALLTHROUGH*/
				case L'|':
				case L'&':
				case L')':
					return(1);
				case L'\\':
					if (!(pc = get_char(p))) return(0);
					/*FALLTHROUGH*/
				default:
					n = 0;
					break;
				}
				p = oldp;
				for (;;)
				{
					if ((n || pc == sc) && onematch(olds, p, e, (wchar_t*)0)) return(1);
					if (!sc) return(0);
					olds = s;
					sc = getsource(s, e);
				}
			}
			else if (pc != L'?' && pc != sc) return(0);
			break;
		case 0L:
			endmatch = olds;
			if (minmatch) return(1);
			/*FALLTHROUGH*/
		case L'|':
		case L'&':
		case L')':
			return(!sc);
		case L'[':
			{
			regex_t	re;
			int 	bra_cnt = 1;
			char	first = 1;
			wchar_t spec;
			wchar_t	wc_bra[LINE_MAX];
			wchar_t	wc_src[2];
			wchar_t *b = wc_bra;
			char    mb_bra[LINE_MAX*MB_CUR_MAX];
			char	mb_src[MB_CUR_MAX+1];


			*b++ = pc;	/* Copy the '[' */
			wc_src[0] = sc;
			wc_src[1] = 0L;
			while(1)
			{
				switch(pc = get_char(p))
				{
					case L'!':		
						if(first)
							*b++ = L'^';	/* regexec negation */
						else	*b++ = pc;
						break;
					case L'[':	
						first = 0;
						*b++ = pc;
						if(*p == L':' || *p == L'.' || *p == L'=')
						{
							*b++ = spec = get_char(p); /* Get special */
							*b++ = get_char(p);	   /* Get next */
							while(*b++ = get_char(p))
							{
								if(*(b-1) == spec && *p == L']')
								{
									*b++ = get_char(p);
									break;
								}
							}
							if(*(b-1))
								continue;
							return(0);
						}
						break;
					case L']':	
						*b++ = pc;
						if(first && (*(b-2) == L'[' || *(b-2) == L'^')){
							first = 0;
							continue;
						}
						if(--bra_cnt == 0)
						{
							*b = 0L;
							goto compile;
						}
						break;
					case 0L:
						return(0);
					default:
						first = 0;
						*b++ = pc;
						break;
				}
			}
compile:
			wcstombs(mb_bra,wc_bra,LINE_MAX);
			wcstombs(mb_src,wc_src,2);
			if(regcomp(&re,mb_bra, REG_EXTENDED|REG_NOSUB) != 0 ||
			   regexec(&re,mb_src,(size_t)0,NULL,0) != 0)
			{
				regfree(&re);
				return(0);
			}
			regfree(&re);
			break;
			}
		case L'\\':
			if (!(pc = get_char(p))) return(0);
			/*FALLTHROUGH*/
		default:
			if (pc != sc) return(0);
			break;
		}
	} while (sc);
	return(0);
}

/*
 * gobble chars up to <sub> or ) keeping track of (...) and [...]
 * sub must be one of { L'|', L'&', 0 }
 * 0 returned if s runs out
 */

static wchar_t*
gobble(s, sub)
wchar_t*	s;
register int	sub;
{
	register int	p = 0;
	register wchar_t*	b = 0L;

	for (;;) switch (get_char(s))
	{
	case L'\\':
		if (get_char(s)) break;
		/*FALLTHROUGH*/
	case 0L:
		return(0L);
	case L'[':
		if (!b) b = s;
		break;
	case L']':
		if (b && b != (s - 1)) b = 0L;
		break;
	case L'(':
		if (!b) p++;
		break;
	case L')':
		if (!b && p-- <= 0) return(sub ? 0L : s);
		break;
	case L'&':
		if (!b && !p && sub == L'&') return(s);
		break;
	case L'|':
		if (!b && !p)
		{
			if (sub == L'|') return(s);
			else if (sub == L'&') return(0L);
		}
		break;
	}
}

#ifdef MULTIBYTE

/*
 * return the next char in (*address) which may be from one to three bytes
 * the character set designation is in the bits defined by C_MASK
 */

static int
mb_getchar(address)
unsigned char**	address;
{
	register unsigned char*	cp = *(unsigned char**)address;
	register int		c = *cp++;
	register int		size;
	int			d;

	if (size = echarset(c))
	{
		d = (size == 1 ? c : 0);
		c = size;
		size = in_csize(c);
		c <<= 7 * (ESS_MAXCHAR - size);
		if (d)
		{
			size--;
			c = (c << 7) | (d & ~HIGHBIT);
		}
		while (size-- > 0)
			c = (c << 7) | ((*cp++) & ~HIGHBIT);
	}
	*address = cp;
	return(c);
}

#endif
