/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:sh/args.c	1.4.4.1"

/*
 * UNIX shell
 *
 * S. R. Bourne
 * Rewritten by David Korn
 * AT&T Bell Laboratories
 *
 */

#include	"defs.h"
#ifdef DEVFD
#   include	"jobs.h"
#endif	/* DEVFD */
#include	"terminal.h"
#undef ESCAPE
#include	"sym.h"
#include	"builtins.h"


#ifdef DEVFD
    void	close_pipes();
#endif	/* DEVFD */

extern void	gsort();
extern int	strcmp();

static int		arg_expand();
static struct dolnod*	copyargs();
static void		print_opts();
static int		split();

static wchar_t		*null;
static struct dolnod	*argfor; /* linked list of blocks to be cleaned up */
static struct dolnod	*dolh;
static wchar_t flagadr[22];
static const wchar_t flagchar[] =
{
	L'i',	L'n',	L'v',	L't',	L's',	L'x',	L'e',	L'r',	L'k',
	L'u',   L'f',	L'a',	L'm',	L'h',	L'p',	L'c',	L'C',	L'b',
	L'w',	L'P',0L
};
static const optflag flagval[]  =
{
	INTFLG,	NOEXEC,	READPR,	ONEFLG, STDFLG,	EXECPR,	ERRFLG,	RSHFLG,	KEYFLG,
	NOSET,	NOGLOB,	ALLEXP,	MONITOR, HASHALL, PRIVM, CFLAG, NOCLOB, NOTIFY,
	WORDEXP,NOCMDST,0
};

/* ======== option handling	======== */

/*
 *  This routine turns options on and off
 *  The options "sicr" are illegal from set command.
 *  The -o option is used to set option by name
 *  This routine returns the number of non-option arguments
 */

int arg_opts(argc,com,builtin_flag)
wchar_t **com;
int  argc;
unsigned int builtin_flag;
{
	register wchar_t *cp;
	register wchar_t c;
	register wchar_t *flagc;
	register wchar_t **argv = com;
	register optflag newflags=opt_flags;
	register optflag opt;
	int trace = is_option(EXECPR);
	wchar_t minus;
	struct namnod *np = (struct namnod*)0;
	char sort = 0;
	char minmin = 0;
	while((cp= *++argv) && ((c= *cp) == L'-' || c==L'+'))
	{
		minus = (c == L'-');
		argc--;
		if((c= *++cp)==0L)
		{
			newflags &= ~(EXECPR|READPR);
			trace = 0;
			argv++;
			break;
		}
		else if(c == L'-')
		{
			minmin = 1;
			argv++;
			break;
		}
		while(c= *cp++)
		{
			if(builtin_flag)
			{
				if(c==L's')
				{
					sort = 1;
					continue;
				}
				else if(c==L'A')
				{
					if(argv[1]==0L)
						sh_fail(*argv, ksh_gettxt(_SGI_DMMX_e_argexp,e_argexp),ERROR);
					np = env_namset(*++argv,sh.var_tree,P_FLAG|V_FLAG);
					argc--;
					if(minus)
						nam_free(np);
					continue;
				}
				else if(wcschr(L"icrwP",c))
					sh_fail(*argv, ksh_gettxt(_SGI_DMMX_e_option,e_option),ERROR);
			}
			if(c==L'c' && minus && argc>=2 && sh.comdiv==0L)
			{
				sh.comdiv= *++argv;
				argc--;
				newflags |= CFLAG;
				continue;
			}
#ifdef apollo
			/* 
			 * New option(-D) allowing the user to define
			 * envirnoment variables on the command line.
			 */
			if (c == L'D')	/* define env variable */
			{
				char *newenv;
				
				if (minus)
				{
					if (cp && *cp)
					{
						if (strchr(cp, L'='))
							newenv = cp;
						else
						{
							newenv = malloc(strlen(cp) + 2);
							strcpy(newenv, cp);
							strcat(newenv, "=");
						}
						env_namset(newenv,sh.var_tree,N_EXPORT|N_FREE);
					}
				} else
					sh_fail(*argv, ksh_gettxt(_SGI_DMMX_e_option,e_option),ERROR);
				argc--;
				break;
			}
#endif /* apollo */
			if(flagc=wcschr(flagchar,c))
				opt = flagval[flagc-flagchar];
			else if(c != L'o')
				sh_fail(*argv,ksh_gettxt(_SGI_DMMX_e_option,e_option),ERROR);
			else
			{
				argv++;
				if(*argv==NIL)
				{
					if(trace)
						sh_trace(com,1);
					trace = 0;
					print_opts(newflags);
					argv--;
					continue;
				}
				else
				{
					unsigned val;
					argc--;
					val=sh_lookup(*argv,tab_options);
					opt = 1L<<val;
					if(opt&(1|INTFLG|RSHFLG))
						sh_fail(*argv,ksh_gettxt(_SGI_DMMX_e_option,e_option),ERROR);
				}
			}
			if(minus)
			{
#if ESH || VSH
				if(opt&(EDITVI|EMACS|GMACS))
					newflags &= ~ (EDITVI|EMACS|GMACS);
#endif
				newflags |= opt;
			}
			else
			{
				if(opt==EXECPR)
					trace = 0;
				newflags &= ~opt;
			}
		}
	}
	/* cannot set -n for interactive shells since there is no way out */
	if(is_option(INTFLG))
		newflags &= ~NOEXEC;
#ifdef RAWONLY
	if(is_option(EDITVI))
		newflags |= VIRAW;
#endif	/* RAWONLY */
	if(!builtin_flag)
		goto skip;
	if(sort)
	{
		if(argc>1)
			gsort(argv,argc-1,strcmp);
		else
			gsort(st.dolv+1,st.dolc,strcmp);
	}
	if((newflags&PRIVM) && !is_option(PRIVM))
	{
		if((sh.userid!=sh.euserid && setuid(sh.euserid)<0) ||
			(sh.groupid!=sh.egroupid && setgid(sh.egroupid)<0) ||
			(sh.userid==sh.euserid && sh.groupid==sh.egroupid))
			newflags &= ~PRIVM;
	}
	else if(!(newflags&PRIVM) && is_option(PRIVM))
	{
		setuid(sh.userid);
		setgid(sh.groupid);
		if(sh.euserid==0)
		{
			sh.euserid = sh.userid;
			sh.egroupid = sh.groupid;
		}
	}
skip:
	if(trace)
		sh_trace(com,1);
	opt_flags = newflags;
	if(builtin_flag)
	{
		argv--;
		if(np)
			env_arrayset(np,argc,argv);
		else if(argc>1 || minmin)
			arg_set(argv);
	}
	return(argc);
}

/*
 * returns the value of $-
 */

wchar_t *arg_dolminus()
{
	register const wchar_t *flagc=flagchar;
	register wchar_t *flagp=flagadr;
	while(*flagc)
	{
		if(opt_flags&flagval[flagc-flagchar])
			*flagp++ = *flagc;
		flagc++;
	}
	*flagp = 0L;
	return(flagadr);
}

/*
 * set up positional parameters 
 */

void arg_set(argi)
wchar_t *argi[];
{
	register wchar_t **argp=argi;
	register int size = 0; /* count number of bytes needed for strings */
	register wchar_t *cp;
	register int 	argn;
	/* count args and number of bytes of arglist */
	while((cp=(wchar_t*)*argp++) != ENDARGS)
	{
		size += wcslen(cp);
	}
	/* free old ones unless on for loop chain */
	argn = argp - argi;
	arg_free(dolh,0);
	dolh=copyargs(argi, --argn, size);
	st.dolc=argn-1;
}

/*
 * free the argument list if the use count is 1
 * If count is greater than 1 decrement count and return same blk
 * Free the argument list if the use count is 1 and return next blk
 * Delete the blk from the argfor chain
 * If flag is set, then the block dolh is not freed
 */

struct dolnod *arg_free(blk,flag)
struct dolnod *	blk;
{
	register struct dolnod*	argr=blk;
	register struct dolnod*	argblk;
	if(argblk=argr)
	{
		if((--argblk->doluse)==0)
		{
			argr = argblk->dolnxt;
			if(flag && argblk==dolh)
				dolh->doluse = 1;
			else
			{
				/* delete from chain */
				if(argfor == argblk)
					argfor = argblk->dolnxt;
				else
				{
					for(argr=argfor;argr;argr=argr->dolnxt)
						if(argr->dolnxt==argblk)
							break;
					if(argr==0)
					{
						return(NULL);
					}
					argr->dolnxt = argblk->dolnxt;
					argr = argblk->dolnxt;
				}
				free((char*)argblk);
			}
		}
	}
	return(argr);
}

/*
 * grab space for arglist and link argblock for cleanup
 * The strings are copied after the argment vector
 */

static struct dolnod *copyargs(from, n, size)
wchar_t *from[];
{
	register struct dolnod *dp=new_of(struct dolnod,n*sizeof(wchar_t*)+((size+n)*WC_SZ));
	register wchar_t **pp;
	register wchar_t *sp;
	dp->doluse=1;	/* use count */
	/* link into chain */
	dp->dolnxt = argfor;
	argfor = dp;
	pp= dp->dolarg;
	st.dolv=pp;
	sp = (wchar_t*)((char *)dp + sizeof(struct dolnod) + n*sizeof(wchar_t*));
	while(n--)
	{
		*pp++ = sp;
		sp = sh_copy(*from++,sp) + 1;
	}
	*pp = ENDARGS;
	return(dp);
}

/*
 *  used to set new argument chain for functions
 */

struct dolnod *arg_new(argi,savargfor)
wchar_t *argi[];
struct dolnod **savargfor;
{
	register struct dolnod *olddolh = dolh;
	*savargfor = argfor;
	dolh = NULL;
	argfor = NULL;
	arg_set(argi);
	return(olddolh);
}

/*
 * reset arguments as they were before function
 */

void arg_reset(blk,afor)
struct dolnod *blk;
struct dolnod *afor;
{
	while(argfor=arg_free(argfor,0));
	dolh = blk;
	argfor = afor;
}

void arg_clear()
{
	/* force `for' $* lists to go away */
	while(argfor=arg_free(argfor,1));
	argfor = dolh;
#ifdef DEVFD
	close_pipes();
#endif	/* DEVFD */
}

/*
 * increase the use count so that an arg_set will not make it go away
 */

struct dolnod *arg_use()
{
	register struct dolnod *dh;
	if(dh=dolh)
		dh->doluse++;
	return(dh);
}

/*
 *  Print option settings on standard output
 */

static void print_opts(oflags)
#ifndef pdp11
register
#endif	/* pdp11 */
optflag oflags;
{
	register const struct sysnod *syscan = tab_options;
#ifndef pdp11
	register
#endif	/* pdp11 */
	optflag value;
	p_setout(st.standout);
	p_str(ksh_gettxt(_SGI_DMMX_e_heading,e_heading),NL);
	while(value=syscan->sysval)
	{
		value = 1<<value;
		p_str(syscan->sysnam,SP);
		p_nchr(SP,16-wcslen(syscan->sysnam));
		if(oflags&value)
			p_str(ksh_gettxt(_SGI_DMMX_e_on,e_on),NL);
		else
			p_str(ksh_gettxt(_SGI_DMMX_e_off,e_off),NL);
		syscan++;
	}
}

#ifdef DEVFD
static int to_close[15];
static int indx;

void close_pipes()
{
	register int *fd = to_close;
	while(*fd)
	{
		close(*fd);
		*fd++ = -1;
	}
	indx = 0;
}
#endif	/* DEVFD */

#ifdef VPIX
#   define EXTRA 2
#else
#   define EXTRA 1
#endif /* VPIX */

/*
 * build an argument list
 */

wchar_t **arg_build(nargs,comptr)
int 	*nargs;
struct comnod	*comptr;
{
	register struct argnod	*argp;
	{
		register struct comnod	*ac = comptr;
		register struct argnod	*schain;
		/* see if the arguments have already been expanded */
		if(ac->comarg==NULL)
		{
			*nargs = 0L;
			return(&null);
		}
		else if((ac->comtyp&COMSCAN)==0)
		{
			*nargs = ((struct dolnod*)ac->comarg)->doluse;
			return(((struct dolnod*)ac->comarg)->dolarg+EXTRA);
		}
		schain = st.gchain;
		st.gchain = NULL;
#ifdef DEVFD
		close_pipes();
#endif	/* DEVFD */
		*nargs = 0L;
		if(ac)
		{
			argp = ac->comarg;
			while(argp)
			{
				*nargs += arg_expand(argp);
				argp = argp->argnxt.ap;
			}
		}
		argp = st.gchain;
		st.gchain = schain;
	}
	{
		register wchar_t	**comargn;
		register int	argn;
		register wchar_t	**comargm;
		argn = *nargs;
		argn += EXTRA;	/* allow room to prepend args */
		comargn=(wchar_t**)stakalloc((unsigned)(argn+1)*sizeof(wchar_t*));
		comargm = comargn += argn;
		*comargn = ENDARGS;
		if(argp==0)
		{
			/* reserve an extra null pointer */
			*--comargn = 0L;
			return(comargn);
		}
		while(argp)
		{
			struct argnod *nextarg = argp->argchn;
			argp->argchn = 0;
			*--comargn = argp->argval;
			if((argp->argflag&A_RAW)==0)
				sh_trim(*comargn);
			if((argp=nextarg)==0 || (argp->argflag&A_MAKE))
			{
				if((argn=comargm-comargn)>1)
					gsort(comargn,argn,wcscmp);
				comargm = comargn;
			}
		}
		return(comargn);
	}
}

/* Argument expansion */

static int arg_expand(argp)
register struct argnod *argp;
{
	register int count = 0;
	argp->argflag &= ~A_MAKE;
#ifdef DEVFD
	if(*argp->argval==0L && (argp->argflag&A_EXP))
	{
		/* argument of the form (cmd) */
		register struct argnod *ap;
		int pv[2];
		int fd;
		ap = (struct argnod*)stakseek(ARGVAL);
		ap->argflag |= A_MAKE;
		ap->argflag &= ~A_RAW;
		ap->argchn= st.gchain;
		st.gchain = ap;
		count++;
		stakputs(wc_e_devfd);
		io_popen(pv);
		fd = argp->argflag&A_RAW;
		stakputs(sh_itos(pv[fd]));
		ap = (struct argnod*)wc_stakfreeze(WC_SZ);
		sh.inpipe = sh.outpipe = 0;
		if(fd)
		{
			sh.inpipe = pv;
			sh_exec((union anynode*)argp->argchn,(int)(st.states&ERRFLG));
		}
		else
		{
			sh.outpipe = pv;
			sh_exec((union anynode*)argp->argchn,(int)(st.states&ERRFLG));
		}
#ifdef JOBS
		job.pipeflag++;
#endif	/* JOBS */
		close(pv[1-fd]);
		to_close[indx++] = pv[fd];
	}
	else
#endif	/* DEVFD */
	if((argp->argflag&A_RAW)==0)
	{
		register wchar_t *ap = argp->argval;
		if(argp->argflag&A_MAC)
			ap = mac_expand(ap);
		count = split(ap,argp->argflag&(A_SPLIT|A_EXP));
	}
	else
	{
		argp->argchn= st.gchain;
		st.gchain = argp;
		argp->argflag |= A_MAKE;
		count++;
	}
	return(count);
}

static int split(s,macflg) /* blank interpretation routine */
wchar_t *s;
{
	register int 	lastsep,bol;
	register wchar_t c;
	register struct argnod *ap;
	int 	count=0;
	int expflag = (!is_option(NOGLOB) && (macflg&A_EXP));
	const wchar_t *seps;
	if(macflg &= A_SPLIT)
		seps = nam_fstrval(IFSNOD);
	else
		seps = wc_e_nullstr;
	if(seps==WCNULL)
		seps = wc_e_sptbnl;
	lastsep=0;
	bol=1;		/* Beginning of line */
	while(1)
	{
		if(sh.trapnote&SIGSET)
			sh_exit(SIGFAIL);
		ap = (struct argnod*)stakseek(ARGVAL);
		while(c= *s++)
		{
			if(c == ESCAPE)
			{
				c = *s++;
				if(c!=L'/')
					stakputc(ESCAPE);
			}
			else if(wcschr(seps,c))			/* sep */
			{
				if(macflg==0)
					continue;
				if(wcschr(wc_e_sptbnl,c))  	/* Current is IFS white */
				{
				   if(bol || 			/* IFS white at bol or */
				      *s == 0L ||		/* next is eol or */
				      wcschr(seps,*s) ||	/* next is sep or */
				      wcschr(wc_e_sptbnl,*s) ||	/* next is IFS white or */
				      lastsep)			/* last was sep */
					continue;
				}
				else {				/* Seperator and not IFS white */
					if(bol-- && 		/* beginning of line and */
					   !wcschr(seps,*s) ) 	/* next is not seperator */
					continue;
				}
				lastsep = c;
				bol=0;
				break;
			}
			stakputc(c);
			lastsep=0;
			bol=0;
		}
	/* This allows contiguous visible delimiters to count as delimiters */
		if(staktell()==ARGVAL)
		{
			if(c==0L)
				return(count);
		}
		else if(c==0L)
		{
			s--;
		}
		/* file name generation */
		ap = (struct argnod*)wc_stakfreeze(WC_SZ);
		ap->argflag &= ~(A_RAW|A_MAKE);
#ifdef BRACEPAT
		if(expflag)
			count += expbrace(ap);
#else
		if(expflag && (c=path_expand(ap->argval)))
			count += c;
#endif /* BRACEPAT */
		else
		{
			count++;
			ap->argchn= st.gchain;
			st.gchain = ap;
		}
		st.gchain->argflag |= A_MAKE;
	}
}

