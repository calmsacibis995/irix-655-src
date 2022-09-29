/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:shlib/optget.c	1.3.4.1"

/*
 * G. S. Fowler
 * AT&T Bell Laboratories
 *
 * command line option parse assist
 * 
 * This is a copy of the getopt(2) library call with +opt and ++ end of args
 * for backward compatibility.
 *
 *	'--' or '++' terminates option list
 *
 *	'-' without option letter returns 0
 *
 */

#ifdef KSHELL
#include	"sh_config.h"
#include	"defs.h"
#endif

/* The variables and their saved components */
int		opt_index,sv_opt_index;	/* argv index			*/
int		opt_sp,sv_opt_sp;	/* char pos in argv[opt_index]	*/
int		opt_plus,sv_opt_plus;	/* Flag for an +opt char	*/
wchar_t		opt_opt,sv_opt_opt;	/* Current option char		*/
wchar_t		*opt_arg;		/* : string argument		*/

wint_t
optget(int argc, wchar_t **argv, wchar_t *opts)
{
	register wchar_t c;
	register wchar_t *cp;

	if(opt_sp == 1)
	{
		if(opt_index >= argc)
			return(-1);
		if(argv[opt_index][0] != L'-' && argv[opt_index][0] != L'+')
			return(-1);
		if(argv[opt_index][0] == L'-' && argv[opt_index][1] == L'\0') {
			opt_index++;
			return(0L);
		}
		if(wcscmp(argv[opt_index], L"--") == NULL ||
		   wcscmp(argv[opt_index], L"++") == NULL)
		{
			opt_index++;
			return(-1L);
		}
		if(argv[opt_index][0] == L'+')
			++opt_plus;
		else	opt_plus=0;
	}
	opt_opt = c = argv[opt_index][opt_sp];
	if(c == L':' || (cp=wcschr(opts, c)) == WCNULL) 
	{
		if(argv[opt_index][++opt_sp] == L'\0') 
		{
			opt_index++;
			opt_sp = 1;
		}
		return(L'?');
	}
	if(*++cp == L':') 
	{
		if(argv[opt_index][opt_sp+1] != L'\0')
			opt_arg = &argv[opt_index++][opt_sp+1];
		else if(++opt_index >= argc) 
		{
			opt_arg = 0L;
			if (*opts == L':') 
			{
				opt_sp = 1;
				return(L':');
			}
			opt_sp = 1;
			return(L'?');
		} 
		else opt_arg = argv[opt_index++];
		opt_sp = 1;
	} 
	else 
	{
		if(argv[opt_index][++opt_sp] == L'\0') 
		{
			opt_sp = 1;
			opt_index++;
		}
		opt_arg = 0L;
	}
	return(c);
}

/*
 * reset everyting.
 */
void
optgetreset(void)
{
	opt_index = 1;
	opt_arg = 0L;
	opt_opt = 0L;
	opt_sp = 1;
	opt_plus = 0;
}
/*
 * Save everything and reset
 */
void
sv_optget(struct sh_optget *sv)
{
	sv->opt_index = opt_index;
	sv->opt_opt   = opt_opt;
	sv->opt_sp    = opt_sp;
	sv->opt_plus  = opt_plus;
	optgetreset();
}
/*
 * Restore everything
 */
void
rs_optget(struct sh_optget *rs)
{
	opt_index = rs->opt_index;
	opt_opt   = rs->opt_opt;
	opt_sp    = rs->opt_sp;
	opt_plus  = rs->opt_plus;
}
