/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:sh/print.c	1.4.4.1"

/*
 * UNIX shell
 *
 * S. R. Bourne
 * Rewritten by David Korn
 * AT&T Bell Laboratories
 *
 */

#include	"defs.h"
#include	"builtins.h"

/* This module references the following external */
extern void	nam_rjust();

/* printing and io conversion */

#ifndef TIC_SEC
#   ifdef HZ
#	define TIC_SEC	HZ	/* number of ticks per second */
#   else
#	define TIC_SEC	60	/* number of ticks per second */
#   endif /* HZ */
#endif /* TIC_SEC */


/*
 *  flush the output queue and reset the output stream
 */

void	p_setout(fd)
register int fd;
{
	register struct fileblk *fp;
	register int count;
	if(!(fp=io_ftable[fd]))
		fp = io_ftable[fd] = &io_stdout;
	else if(fp->flag&IOREAD)
	{
		if(count=fp->last-fp->ptr)
			lseek(fd,-((off_t)count),SEEK_CUR);
		fp->fseek = 0;
		fp->ptr = fp->base;
	}
	fp->last = fp->base + IOBSIZE;
	fp->flag &= ~(IOREAD|IOERR|IOEOF);
	fp->flag |= IOWRT;
	if(output==fd)
		return;
	if(fp = io_ftable[output])
		if(io_ftable[fd]==fp || (fp->flag&IOSLOW))
			p_flush();
	output = fd;
}

/*
 * flush the output if necessary and null terminate the buffer
 */

void p_flush()
{
	register struct fileblk *fp = io_ftable[output];
	register unsigned count;
	if(fp)
	{
		if(count=fp->ptr-fp->base)
		{
			if(ksh_write(output,fp->base,count) < 0)
				fp->flag |= IOERR;
			if(sh.heretrace)
				ksh_write(ERRIO,fp->base,count);
			fp->ptr = fp->base;
		}
		/* leave buffer as a null terminated string */
		*fp->ptr = 0L;
	}
}

/*
 * print a given character
 */

void	p_char(c)
register wchar_t c;
{
	register struct fileblk *fp = io_ftable[output];
	if(fp->ptr >= fp->last)
		p_flush();
	*fp->ptr++ = c;
}

/*
 * print a string optionally followed by a character
 * The buffer is always terminated with a zero byte.
 */

void	p_str(string,c)
register const wchar_t *string;
wint_t c;
{
	register struct fileblk *fp = io_ftable[output];
	register wint_t cc;
	while(1)
	{
		if((cc= *string)==0L)
			cc = c,c = 0L;
		else
			string++;
		if(fp->ptr >= fp->last)
			p_flush();
		*fp->ptr = cc;
		if(cc==0L)
			break;
		fp->ptr++;
	}
}

/*
 * print a given character a given number of times
 */

void	p_nchr(c,n)
register int n;
wchar_t c;
{
	register struct fileblk *fp = io_ftable[output];
	while(n-- > 0)
	{
		if(fp->ptr >= fp->last)
			p_flush();
		*fp->ptr++ = c;
	}
}
/*
 * print a message preceded by the command name
 */

void p_prp(s1)
const wchar_t *s1;
{
	register wchar_t *cp;
	register wint_t c;
	if(cp=st.cmdadr)
	{
		if(*cp==L'-')
			cp++;
		c = ((st.cmdline>1)?0:L':');
		p_str(cp,c);
		if(c==0L)
			p_sub(st.cmdline,L':');
		p_char(SP);
	}
	if(cp = (wchar_t *)s1)
	{
		for(;c= *cp;cp++)
		{
			if(!iswprint(c))
			{
				p_char(L'^');
				/* c ^= TO_PRINT; */
			}
			p_char(c);
		}
	}
}

/*
 * print a time and a separator 
 */

void	p_time(t,c)
#ifndef pdp11
    register
#endif /* pdp11 */
clock_t t;
int c;
{
	register int  min, sec, frac;
	register int hr;
	frac = t%TIC_SEC;
	frac = (frac*100)/TIC_SEC;
	t /= TIC_SEC;
	sec=t%60; t /= 60;
	min=t%60;
	if(hr=t/60)
	{
		p_num(hr,L'h');
	}
	p_num(min,L'm');
	p_num(sec,L'.');
	if(frac<10)
		p_char(L'0');
	p_num(frac,L's');
	p_char((wchar_t)c);
}

/*
 * print a number optionally followed by a character
 */

void	p_num(n,c)
int 	n;
wint_t c;
{
	p_str(sh_itos(n),c);
}


/* 
 * print a list of arguments in columns
 */
#define NROW	15	/* number of rows in output before going to multi-columns */
#define LBLSIZ	3	/* size of label field and interfield spacing */

void	p_list(argn,com)
wchar_t *com[];
{
	register int i,j;
	register wchar_t **arg;
	wchar_t a1[12];
	int nrow;
	int ncol = 1;
	int ndigits = 1;
	int fldsize;
#if ESH || VSH
	int wsize = ed_window();
#else
	int wsize = 80;
#endif
	wchar_t *cp = nam_fstrval(LINES);
	nrow = (cp?1+2*(wcstol(cp,(wchar_t **)0,10)/3):NROW);
	for(i=argn;i >= 10;i /= 10)
		ndigits++;
	if(argn < nrow)
	{
		nrow = argn;
		goto skip;
	}
	i = 0;
	for(arg=com; *arg;arg++)
	{
		i = max(i,wcslen(*arg));
	}
	i += (ndigits+LBLSIZ);
	if(i < wsize)
		ncol = wsize/i;
	if(argn > nrow*ncol)
	{
		nrow = 1 + (argn-1)/ncol;
	}
	else
	{
		ncol = 1 + (argn-1)/nrow;
		nrow = 1 + (argn-1)/ncol;
	}
skip:
	fldsize = (wsize/ncol)-(ndigits+LBLSIZ);
	for(i=0;i<nrow;i++)
	{
		j = i;
		while(1)
		{
			arg = com+j;
			wcscpy(a1,sh_itos(j+1));
			nam_rjust(a1,ndigits,L' ');
			p_str(a1,L')');
			p_char(SP);
			p_str(*arg,0L);
			j += nrow;
			if(j >= argn)
				break;
			p_nchr(SP,fldsize-wcslen(*arg));
		}
		newline();
	}
}

/*
 * Print a number enclosed in [] followed by a character
 */

void	p_sub(n,c)
register int n;
register wchar_t c;
{
	p_char(L'[');
	p_num(n,L']');
	if(c)
		p_char(c);
}

#ifdef POSIX
/*
 * print <str> qouting chars so that it can be read by the shell
 * terminate with the character <cc>
 */
void	p_qstr(str,cc)
wchar_t *str;
{
	register wchar_t *cp = str;
	register wchar_t c = *cp;
	register int state = (c==0L);
	do
	{
		if(is_walpha(c))
		{
			while((c = *++cp),is_walnum(c));
			if(c==L'=')
			{
				*cp = 0L;
				p_str(str,c);
				*cp++ = c;
				str = cp;
				c = *cp;
			}
		}
		if(c==L'~')
			state++;
		while((c = *cp++) && (c!= L'\'') && c!=L'^')
			state |= expchar(c)|iswspace(c);
		if(c || state)
		{
			/* needs single quotes */
			p_char(L'\'');
			if(c)
			{
				/* string contains single quote */
				c = *cp;
				*cp = 0L;
				state = L'\\';
			}
			else
				state = L'\'';
			p_str(str,state);
			if(c)
				*cp = c;
			str = (cp-1);
		}
	}
	while(c);
	p_str(str,cc);
}
#endif /* POSIX */
