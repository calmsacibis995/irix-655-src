/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:sh/echo.c	1.3.4.1"

/*
 * This is the code for the echo and print command
 */

#ifdef KSHELL
#   include	"defs.h"
#endif	/* KSHELL */

#ifdef __STDC__
#   define ALERT	L'\a'
#else
#   define ALERT	07
#endif /* __STDC__ */

/*
 * echo the argument list
 * if raw is non-zero then \ is not a special character.
 * returns 0 for \c otherwise 1.
 */

int echo_list(raw,com)
int raw;
wchar_t *com[];
{
	register wint_t outc;
	register wchar_t *cp;
	while(cp= *com++)
	{
		if(!raw) for(; *cp; cp++)
		{
			outc = *cp;
			if(outc == L'\\')
			{
				switch(*++cp)
				{
					case L'a':
						outc = ALERT;
						break;
					case L'b':
						outc = L'\b';
						break;
					case L'c':
						return(0);
					case L'f':
						outc = L'\f';
						break;
					case L'n':
						outc = L'\n';
						break;
					case L'r':
						outc = L'\r';
						break;
					case L'v':
						outc = L'\v';
						break;
					case L't':
						outc = L'\t';
						break;
					case L'\\':
						outc = L'\\';
						break;
					case L'0':
					{
						register wchar_t *cpmax;
						outc = 0L;
						cpmax = cp + 4;
						while(++cp<cpmax && *cp>=L'0' && 
							*cp<=L'7')
						{
							outc <<= 3;
							outc |= (*cp-L'0');
						}
						cp--;
						break;
					}
					default:
					cp--;
				}
			}
			p_char(outc);
		}
#ifdef POSIX
		else if(raw>1)
			p_qstr(cp,0L);
#endif /* POSIX */
		else
			p_str(cp,0L);
		if(*com)
			p_char((is_option(WORDEXP)?0:L' '));
#ifdef KSHELL
		if(sh.trapnote&SIGSET)
			sh_exit(SIGFAIL);
#endif	/* KSHELL */
	}
	return(1);
}

