/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:sh/main.c	1.5.4.2"

/*
 * UNIX shell
 *
 * S. R. Bourne
 * Rewritten by David Korn
 * AT&T Bell Laboratories
 *
 */

#include	"defs.h"
#include	"jobs.h"
#include	"sym.h"
#include	"history.h"
#include	"timeout.h"
#include	"builtins.h"
#include	<errno.h>
#include	<unistd.h>
#ifdef pdp11
#   include	<execargs.h>
#endif	/* pdp11 */
#ifdef TIOCLBIC
#   undef TIOCLBIC
#   ifdef _sys_ioctl_
#	include	<sys/ioctl.h>
#   endif /* _sys_ioctl_ */
#endif /* TIOCLBIC */


/* These routines are referenced by this module */
extern int	gscan_some();

static void	exfile();
static void	pr_prompt();
static void	chkmail();
static int	notstacked();
static void	dfault();
static void	swallow();
static void	freejobs_init();
static char	*nospace();
extern wchar_t **argvtowc();

extern char	**environ;

static struct fileblk topfile;
static struct stat lastmail;
static time_t	mailtime;
static char	beenhere = 0;

long		sysconf();

main(c, v)
int 	c;
register char *v[];
{
	register wchar_t *sim;
	register int 	rsflag;	/* local restricted flag */
	wchar_t *command;
	wchar_t **wc_argv;
	int prof;
#ifdef MTRACE
	extern int Mt_certify;
	Mt_certify = 1;
#endif /* MTRACE */
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxsgish");
	st.standout = 1;
	sh.cpipe[0] = -1;
	sh.userid=getuid();
	sh.euserid=geteuid();
	sh.groupid=getgid();
	sh.egroupid=getegid();
	time(&mailtime);
	sig_init();
	stakinstall((Stak_t*)0,nospace);

	/* Convert argv array to wide chars */
	if((wc_argv = argvtowc(c,v)) == (wchar_t **)0)
		sh_fail(ksh_gettxt(_SGI_DMMX_e_argv_conv,e_argv_conv),NIL,ERROR);

	/* set up memory for name-value pairs */
	nam_init();

	/* set up memory for freejob list */
	freejobs_init();

	/* save the name of the invoking process */
	sh.shinvoke = sim = path_basename(*wc_argv);

	/* Normally a builtin alias for Bourne */
        if(!strmatch(sh.shinvoke,L"sh")){
                struct namnod  *np;
                np = nam_search(L"chdir",sh.alias_tree,N_NULL);
                if(np) nam_free(np);
        }

	/* set name-value pairs from userenv
	 *  'rsflag' is zero if SHELL variable is
	 * set in environment and contains an'r' in
	 * the basename part of the value.
	 */
	rsflag=env_init();

	/* a shell is also restricted if argv(0) has
	 *  an 'rsh' for its basename
	 */
	if(*sim==L'-')
	{
		sim++;
		sh.login_sh = 2;
	}
	/* check for restricted shell */
	if(c>0 && strmatch(sim,L"@(rk|kr|r)sh"))
		rsflag=0;

	/* look for options */
	/* st.dolc is $#	*/
	st.dolc=arg_opts(c,wc_argv,0);
	st.dolv=wc_argv+c-st.dolc--;
	st.dolv[0] = wc_argv[0];
	if(st.dolc < 1)
		on_option(STDFLG);
	if(!is_option(STDFLG))
	{
		st.dolc--;
		st.dolv++;
	}
	/* set[ug]id scripts require the -p flag */
	prof = !is_option(PRIVM);
	if(sh.userid!=sh.euserid || sh.groupid!=sh.egroupid)
	{
		prof = 0;
/*
**	USL change:
**		P_UID changed to PUID because of conflict with sys/procset.h
**
*/
#ifdef PUID
		/* require -p option to run setuid and/or setgid */
		if(!is_option(PRIVM) && sh.userid >= PUID)
		{
			/*
			 *  setuid or setgid only if NOT invoked as .../bin/sh
			 *  If not invoked as "sh", then make the effective
			 *  uid (euid) be equal to the userid (uid).
			 */
			if(!strmatch(sim,L"sh")) {
				setuid(sh.euserid=sh.userid);
				setgid(sh.egroupid=sh.groupid);
			}
		}
		else
#else
		{
			on_option(PRIVM);
			prof = 0;
		}
#endif /* PUID */
#ifdef SHELLMAGIC
		/* careful of #! setuid scripts with name beginning with - */
		if(sh.login_sh && eq(wc_argv[0],wc_argv[1]))
			sh_fail(ksh_gettxt(_SGI_DMMX_e_prohibited,e_prohibited),NIL,ERROR);
#endif /*SHELLMAGIC*/
	}
	else
		off_option(PRIVM);
	if(is_option(RSHFLG))
	{
		rsflag = 0;
		off_option(RSHFLG);
	}
	sh.shname = st.dolv[0]; /* shname for $0 in profiles */
	/*
	 * return here for shell file execution
	 * but not for parenthesis subshells
	 */
	st.cmdadr = st.dolv[0]; /* cmdadr is $0 */
	SETJMP(sh.subshell);
	command = st.cmdadr;
	/* set pidname '$$' */
	sh.pid = getpid();
	io_settemp(sh.pid);
	srand(sh.pid&0x7fff);
	sh.ppid = getppid();
	dfault(PS4NOD,wc_e_traceprompt);
#ifdef apollo
	/*
	 * The following code initializes apollo specific features.
	 * See description below for more details.
	 */
	{
		extern char *getenv();
		/*
		 * initialize the SYSTYPENOD internal data. Use
		 * the current "SYSTYPE" environment variable value
		 * as the initial value.
		 */		
		dfault(SYSTYPENOD,getenv("SYSTYPE"));
		
		/* 
		 * if the current systype is not bsd..., then
		 * set the default path to the sys5.3 default path.
		 * It is setup in this order because the default path
		 * for bsd4.3 is longer than the sys5.3, this order insures that
		 * there is always enough storage space allocated.
		 */
		if (strncmp(SYSTYPENOD->value.namval.cp, "bsd", 3) != 0)
		{
			/* change the default path of bsd4.3 to sys5.3 */
			wcscpy(wc_e_defpath,L"/bin:/usr/bin:/usr/apollo/bin:");
		}
		/*
		 * setup cd and pwd to use physical instead of
		 * logical path.
		 */
		on_option(PHYSICAL);
	}
#endif	/* apollo */
	path_pwd(1);
	if((beenhere++)==0)
	{
		st.states |= PROFILE;
		/* decide whether shell is interactive */
		if(is_option(ONEFLG|CFLAG)==0 && is_option(STDFLG) &&
			tty_check(0) && tty_check(ERRIO))
			on_option(INTFLG|BGNICE);
		if(sh.ppid==1)
			sh.login_sh++;
		if(sh.login_sh >= 2)
		{
			/* ? profile */
			sh.login_sh += 2;
			
			/* If we are an interactive shell, call job_init()
			 * which will set up job control for us.  No need if
			 * we aren't interactive (possibly buggy if we do).
			 */
			if (is_option(INTFLG))
				job_init(1);
			
			/*	system profile	*/
			if((input=path_open(wc_e_sysprofile,WCNULLSTR)) >= 0)
			{
				st.cmdadr = (wchar_t *)wc_e_sysprofile;
				exfile();	/* file exists */
				io_fclose(input);
			}
			if(prof &&  (input=path_open(mac_try((wchar_t *)wc_e_profile),WCNULLSTR)) >= 0)
			{
				st.cmdadr = path_basename(wc_e_profile);
				exfile();
				io_fclose(input);
			}
		}
		/* make sure PWD is set up correctly */
		path_pwd(1);
		if(prof) {
			struct namnod  *np;
			np = nam_search(L"_XPG",sh.var_tree,N_NULL);
			if(wcscmp(sim,L"ksh") == 0 || np && *(np->value.namval.cp) > L'0')
				sim = mac_try(nam_strval(ENVNOD));
			else	sim = WCNULLSTR;
		}
		else if(is_option(PRIVM))
			sim = (wchar_t *)wc_e_suidprofile;
		else
			sim = WCNULLSTR;
		if(*sim && (input = path_open(sim,WCNULLSTR)) >= 0)
		{
			st.cmdadr = sh_heap(sim);
			exfile();
			free(st.cmdadr);
			st.cmdadr = command;
			io_fclose(input);
		}
		st.cmdadr = command;
		st.states &= ~PROFILE;
		if(rsflag==0)
		{
			on_option(RSHFLG);
			nam_ontype(PATHNOD,N_RESTRICT);
			nam_ontype(ENVNOD,N_RESTRICT);
			nam_ontype(SHELLNOD,N_RESTRICT);
		}
		/* open input file if specified */
		st.cmdline = 0;
		st.standin = 0;
		if(sh.comdiv)
		{
		shell_c:
			st.standin = &topfile;
			io_sopen(sh.comdiv);
			input = F_STRING;
		}
		else
		{
			sim = st.cmdadr;
			st.cmdadr = wc_argv[0];
			if(is_option(STDFLG))
				input = 0;
			else
			{
				wchar_t *sp;
				/* open stream should have been passed into shell */
				if(strmatch(sim,wc_e_devfdNN))
				{
					struct stat statb;
					input = wcstol(sim+8,(wchar_t **)0,10);
					if(fstat(input,&statb)<0)
						sh_fail(st.cmdadr,ksh_gettxt(_SGI_DMMX_e_open,e_open),ERROR);
					sim = st.cmdadr;
					off_option(EXECPR|READPR);
				}
				else
				{
#ifdef VFORK
				if(strcmp(sim,path_basename(*environ))==0)
					sp = (*environ)+2;
				else
#endif /* VFORK */
				if(strmatch(sh.shinvoke,L"sh") && !wcschr(sim,L'/') && 
				   sh_access(sim,R_OK)==0)
					sp = sim;
				else	sp = path_absolute(sim);
				if(sp==0L || (input=ksh_open(sp,O_RDONLY))<0)
#ifndef VFORK
					/* for compatibility with bsh */
					if((input=ksh_open(sim,O_RDONLY))<0)
#endif /* VFORK */
					{
						if(errno != ENOENT)
							sh_fail_why(sim,ksh_gettxt(_SGI_DMMX_e_open,e_open),ERROR);
						/* try sh -c 'name "$@"' */
						on_option(CFLAG);
						sh.comdiv = malloc((sizeof(wchar_t))*(wcslen(sim)+7));
						sim = sh_copy(sim,sh.comdiv);
						if(st.dolc)
							sh_copy(L" \"$@\"",sim);
						goto shell_c;
					}
				}
				sh.readscript = wc_argv[0];
			}
			st.cmdadr = sim;
			sh.comdiv--;
#ifdef ACCT
			initacct();
			if(input != 0)
				preacct(st.cmdadr);
#endif	/* ACCT */
		}
		if(!is_option(INTFLG))
		{
			/* eliminate local aliases and functions */
			gscan_some(env_nolocal,sh.alias_tree,N_EXPORT,0);
			gscan_some(env_nolocal,sh.fun_tree,N_EXPORT,0);
		}
	}
#ifdef pdp11
	else
		*execargs=(char *) st.dolv;	/* for `ps' cmd */
#endif	/* pdp11 */
	st.states |= is_option(INTFLG);

	if(!(nam_fstrval(IFSNOD)))	/* Don't clobber if set in environ */
		nam_fputval(IFSNOD,(wchar_t *)wc_e_sptbnl);
	exfile();
	if(st.states&PROMPT)
	        newline();
	sh_done(0);
	/* NOTREACHED */
}

static void	exfile()
{
	time_t curtime;
	union anynode *t;
	register struct fileblk *fp = st.standin;
	register int fno = input;
	int maxtry = MAXTRY;
	int execflags;

	/* handle early errors; if for example a tmp file can't be created
	 * in hist_open, we try to longjmp back through freturn, but it's
	 * never been set up before, so we segv... */
	if(SETJMP(sh.errshell))
		return;
	sh.freturn = (jmp_buf*)sh.errshell;

	/* move input */
	if(fno!=F_STRING)
	{
		if(fno > 0)
		{
			fno = input = io_renumber(fno,INIO);
			io_ftable[0] = 0;
		}
		io_init(input=fno,(struct fileblk*)0,NIL);
		st.standin = fp = io_ftable[fno];
		if(is_option(ONEFLG) && !(st.states&PROFILE))
			fp->flag |= IONBF;
	}
	if(st.states&INTFLG)
	{
		dfault(PS1NOD, (sh.euserid?wc_e_stdprompt:wc_e_supprompt));
		fp->flag |= IOEDIT;
		sig_ignore(SIGTERM);
		if(hist_open())
		{
			on_option(FIXFLG);
		}
		if(sh.login_sh<=1)
			job_init(0);
	}
	else
	{
		if(!(st.states&PROFILE))
		{
			on_option(HASHALL);
			off_option(MONITOR);
		}
		st.states &= ~(PROMPT|MONITOR|FIXFLG);
		off_option(FIXFLG);
	}
	if(SETJMP(sh.errshell) && (st.states&PROFILE))
	{
		io_fclose(fno);
		st.states &= ~(INTFLG|MONITOR);
		return;
	}
	/* error return here */
	optgetreset();
	sh.freturn = (jmp_buf*)sh.errshell;
	st.loopcnt = 0;
	st.peekn = 0L;
	st.linked = 0;
	st.fn_depth = 0;
	sh.trapnote = 0;
	if(fiseof(fp))
		goto eof_or_error;
	/* command loop */
	while(1)
	{
		sh_freeup();
		stakset((char *)0,0);
		exitset();
		st.states &= ~(ERRFLG|READPR|GRACE|MONITOR);
		st.states |= is_option(READPR)|WAITING|ERRFLG;
		/* -eim  flags don't apply to profiles */
		if(st.states&PROFILE)
			st.states &= ~(INTFLG|ERRFLG|MONITOR);
		p_setout(ERRIO);
		if((st.states&PROMPT) && notstacked() && !fiseof(fp))
		{
			register wchar_t *mail;
#ifdef JOBS
			st.states |= is_option(MONITOR);
			job_walk(job_list,N_FLAG,(wchar_t**)0);
#endif	/* JOBS */
			if((mail=nam_strval(MAILPNOD)) || (mail=nam_strval(MAILNOD)))
			{
				time(&curtime);
				if ((curtime - mailtime) >= sh_mailchk)
				{
					chkmail(mail);
					mailtime = curtime;
				}
			}
			if(hist_ptr)
				hist_eof();
			pr_prompt(mac_try(nam_strval(PS1NOD)));
			/* sets timeout for command entry */
#ifdef TIMEOUT
			if(sh_timeout <= 0 || sh_timeout > TIMEOUT)
				sh_timeout = TIMEOUT;
#endif /* TIMEOUT */
			if(sh_timeout>0)
				alarm((unsigned)sh_timeout);
			fp->flin = 1;
		}
		st.peekn = io_readc();
		if(fiseof(fp) || fiserror(fp))
		{
		eof_or_error:
			if((st.states&PROMPT) && notstacked() && fiserror(fp)==0) 
			{
				clearerr(fp);
				st.peekn = 0L;
				if(--maxtry>0 && is_option(NOEOF) &&
					 !fiserror(&io_stdout) && tty_check(fno))
				{
					p_str(ksh_gettxt(_SGI_DMMX_e_logout,e_logout),NL);
					continue;
				}
				else if(job_close()<0)
					continue;
				hist_close();
			}
			return;
		}
		maxtry = MAXTRY;
		if(sh_timeout>0)
			alarm(0);
		if((st.states&PROMPT) && hist_ptr && st.peekn)
		{
			hist_eof();
			p_char(st.peekn);
			p_setout(ERRIO);
		}
		st.states |= is_option(FIXFLG);
		st.states &= ~ WAITING;
		st.cmdline = fp->flin;
		t = sh_parse(NL,MTFLG);
		sh.readscript = 0L;
		if((st.states&PROMPT) && hist_ptr)
		{
			if(t==0 && nextchar(st.standin)==L'#')
				swallow(st.standin);
			hist_flush();
		}
		if(t)
		{
			execflags = (ERRFLG|MONITOR|PROMPT);
			/* sh -c simple-command may not have to fork */
			if(!(st.states&PROFILE) && is_option(CFLAG) && nextchar(fp)==0L)
				execflags |= 1;
			st.execbrk = 0;
			p_setout(ERRIO);
			sh_exec(t,execflags);
			/* This is for sh -t */
			if(is_option(ONEFLG))
			{
				io_unreadc(NL);
				fp->flag |= IOEOF;
			}
		}
	}
}

/*
 * if there is pending input, the prompt is not printed.
 * prints PS2 if flag is zero otherwise PS3
 */

void	sh_prompt(flag)
register int flag;
{
	register wchar_t *cp;
	register struct fileblk *fp;
	if(flag || (st.states&PROMPT) && notstacked())
	{
		if(flag)
			fp = io_ftable[0];
		else
			fp = st.standin;
		/* if characters are in input buffer don't issue prompt */
		if(fp && finbuff(fp))
			return;
		if(cp = nam_fstrval(flag?PS3NOD:PS2NOD))
		{
			flag = output;
			p_setout(ERRIO);
			p_str(cp,0L);
			p_setout(flag);
		}
	}
}



/* prints out messages if files in list have been modified since last call */
static void chkmail(files)
wchar_t *files;
{
	register wchar_t *cp,*sp,*qp;
	register wchar_t save;
	struct stat	statb;
	if(*(cp=files) == 0L)
		return;
	sp = cp;
	do
	{
		/* skip to : or end of string saving first '?' */
		for(qp=0L;*sp && *sp != L':';sp++)
			if((*sp == L'?' || *sp==L'%') && qp == 0L)
				qp = sp;
		save = *sp;
		*sp = 0L;
		/* change '?' to end-of-string */
		if(qp)
			*qp = 0L;
		st.gchain = NULL;
		do
		{
			/* see if time has been modified since last checked
			 * and the access time <= the modification time
			 */
			if(ksh_stat(cp,&statb) >= 0 && statb.st_mtime >= mailtime
				&& statb.st_atime <= statb.st_mtime)
			{
				/* check for directory */
				if(st.gchain==NULL && S_ISDIR(statb.st_mode)) 
				{
					/* generate list of directory entries */
					f_complete(cp,L"/*");
				}
				else
				{
					/*
					 * If the file has shrunk,
					 * or if the size is zero
					 * then don't print anything
					 */
					if(statb.st_size &&
						(  statb.st_ino != lastmail.st_ino
						|| statb.st_dev != lastmail.st_dev
						|| statb.st_size > lastmail.st_size))
					{
						/* save and restore $_ */
						wchar_t *save = sh.lastarg;
						sh.lastarg = cp;
						p_str(mac_try(qp==0L?ksh_gettxt(_SGI_DMMX_e_mailmsg,e_mailmsg):qp+1),NL);
						sh.lastarg = save;
					}
					lastmail = statb;
					break;
				}
			}
			if(st.gchain)
			{
				cp = st.gchain->argval;
				st.gchain = st.gchain->argchn;
			}
			else
				cp = WCNULL;
		}
		while(cp);
		if(qp)
			*qp = L'?';
		*sp++ = save;
		cp = sp;
	}
	while(save);
	stakset((char *)0,0);
}

/*
 * print the primary prompt
 */

static void pr_prompt(str)
register wchar_t *str;
{
	register wint_t c;
	static short cmdno;
#ifdef TIOCLBIC
	int mode;
	mode = LFLUSHO;
	ioctl(ERRIO,TIOCLBIC,&mode);
#endif	/* TIOCLBIC */
	p_flush();
	p_setout(ERRIO);
	for(;c= *str;str++)
	{
		if(c==FC_CHAR)
		{
			/* look at next character */
			c = *++str;
			/* print out line number if not !! */
			if(c!= FC_CHAR)
				p_num(hist_ptr?hist_ptr->fixind:++cmdno,0L);
			if(c==0L)
				break;
		}
		p_char(c);
	}
#if VSH || ESH
	*io_ftable[ERRIO]->ptr = 0L;
	/* prompt flushed later */
#else
	p_flush();
#endif
}

static void dfault(np,value)
register struct namnod *np;
wchar_t *value;
{
	if(isnull(np))
		nam_fputval(np,value);
}

static notstacked()
{
	register struct fileblk *fp;
	/* clean up any completed aliases */
	while((fp=st.standin)->ftype==F_ISALIAS && fp->flast==0L)
		io_pop(1);
	return(fp->fstak==0);
}

/*
 * swallow up additional comment lines and put them into the history file
 */

static void swallow(fp)
struct fileblk *fp;
{
	register wchar_t *sp = fp->ptr;
	register wchar_t *ep;
#ifndef sgi
	/*
	 * Don't add comment lines to the history file
	 */
	p_setout(hist_ptr->fixfd);
#endif
	for(ep=sp; *ep; ep++)
	{
		if(*ep==NL)
		{
			*ep = 0L;
#ifndef sgi
			p_str(sp,NL);
#endif
			*ep++ = NL;
			if(*(sp=ep)!=L'#')
				break;
		}
	}
	fp->ptr = sp;
}

static char *nospace()
{
	sh_fail(ksh_gettxt(_SGI_DMMX_e_space,e_space),NIL,ERROR);
	return((char*)0);
}

/* Routine to check for XPG compliance:
 *	1) Shell not invoked as "sh" - this allows "ksh" and "rksh" by
 *         default to be compliant.
 *	2) Shell invoked as "sh" and _XPG variable defined and greater than '0'
 *
 *	Returns 1 for true, 0 for false
 */
int
xpg_compliant()
{
	struct namnod  *np;

	if(!strmatch(sh.shinvoke,L"sh"))
		return(1);

	np = nam_search(L"_XPG",sh.var_tree,N_NULL);

	if(!np)
		return(0);
	if(*(np->value.namval.cp) > L'0')
		return(1);
	return(0);
}

static void freejobs_init()
{
	size_t jbytes;

	if( (sh_numjobs = sysconf(_SC_CHILD_MAX)) == -1)
		sh_numjobs = MAXJ;

	/* Calculate number of job bytes - 8 jobs per byte */
	jbytes = 1+((sh_numjobs-1)/(8));

	if((job.freejobs = (unsigned char *)calloc(jbytes,sizeof(char))) == NULL)
		sh_fail(ksh_gettxt(_SGI_DMMX_e_malloc_freejobs,e_malloc_freejobs),NIL,ERROR);
}
