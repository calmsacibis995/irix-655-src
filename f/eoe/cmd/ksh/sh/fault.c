/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:sh/fault.c	1.5.4.1"

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
#include	"timeout.h"


static int sleeping = 0;
/* ========	fault handling routines	   ======== */

#ifndef JOBS
#   undef SIGCHLD
#endif /* JOBS */

void	sh_fault(sig)
register int 	sig;
{
	register int 	flag;
#ifdef OLDTERMIO
	/* This .1 sec delay eliminates break key problems on 3b consoles */
#   ifdef _poll_
	if(sig==2)
		poll("",0,100);
#   endif /* _poll_ */
#endif /* OLDTERMIO */
#ifdef apollo
	/*
	 * Since this routine only handles SIGCHLD, make SIGCLD look like
	 * a SIGCHLD. Since both signals are defined on an apollo.
	 */
	if(sig==SIGCLD)
		sig = SIGCHLD;
#endif /* apollo */
#ifdef	SIGCHLD
	if(sig==SIGCHLD)
	{
		job.waitsafe++;
		if(st.trapcom[SIGCHLD])
		{
			sh.trapnote |= SIGSLOW;
#   ifndef SIG_NORESTART
			if(st.intfn)
			{
				sigrelease(sig);
				(*st.intfn)();
			}
#   endif	/* SIG_NORESTART */
		}
		return;
	}
#endif	/* SIGCHLD */
	signal(sig, sh_fault);
	if(sig==SIGALRM)
	{
		if((st.states&WAITING) && sh_timeout>0)
		{
			if(st.states&GRACE)
			{
				/* force exit */
					st.states &= ~GRACE;
					st.states |= FORKED;
					sh_fail(ksh_gettxt(_SGI_DMMX_e_timeout,e_timeout),NIL,ERROR);
			}
			else
			{
				st.states |= GRACE;
				alarm((unsigned)TGRACE);
				p_str(ksh_gettxt(_SGI_DMMX_e_timewarn,e_timewarn),NL);
				p_flush();
			}
		}
		if(sleeping)
			return;
	}
	else
	{
		if(st.trapcom[sig])
			flag = TRAPSET;
		else
		{
			sh.lastsig = sig;
			flag = SIGSET;
		}
		sh.trapnote |= flag;
		st.trapflg[sig] |= flag;
		if(sig <= SIGQUIT)
			sh.trapnote |= SIGSLOW;
	}
#ifndef SIG_NORESTART
	/* This is needed because interrupted reads automatically restart */
	if(st.intfn)
	{
		sigrelease(sig);
		(*st.intfn)();
	}
#endif	/* SIG_NORESTART */
}

void sig_init()
{
	register int i;
	register int n;
	register const struct sysnod	*syscan = sig_names;
	register const struct sysmsgnod	*syscanmsg;
	sig_begin();
	while(*syscan->sysnam)
	{
		n = syscan->sysval;
		i = n&((1<<SIGBITS)-1);
		n >>= SIGBITS;
		st.trapflg[--i] = (n&~SIGIGNORE);
		if(n&SIGFAULT)
			signal(i,(VOID(*)())sh_fault);
		else if(n&SIGIGNORE)
			sig_ignore(i);
		else if(n&SIGCAUGHT)
			sig_ontrap(i);
		else if(n&SIGDONE)
		{
			sh.trapnote |= SIGBEGIN;
			if(signal(i,(VOID(*)())sh_done)==SIG_IGN)
			{
				sig_ignore(i);
				st.trapflg[i] = SIGOFF;
			}
			else
				st.trapflg[i] = SIGMOD|SIGDONE;
			sh.trapnote &= ~SIGBEGIN;
		}
		syscan++;
	}
	for(syscanmsg=sig_messages; n=syscanmsg->sysval; syscanmsg++)
	{
		if(n > NSIG+1)
			continue;
		if(*syscanmsg->sysnam) {
			wchar_t *wc_sysnam = ksh_gettxt(syscanmsg->sysmsgid,syscanmsg->sysnam);
			size_t msgsz = wcslen( wc_sysnam);
			if((sh.sigmsg[n-1] = (wchar_t *)malloc((size_t)((msgsz+1)*sizeof(wchar_t)))) == NULL)
				sh.sigmsg[n-1] = wc_sysnam;
			else 
				wcscpy(sh.sigmsg[n-1],wc_sysnam);
		}
	}
}

/*
 * set signal n to ignore
 * returns 1 if signal was already ignored, 0 otherwise
 */
int	sig_ignore(n)
register int n;
{
	if(n < MAXTRAP-1 && !(st.trapflg[n]&SIGIGNORE))
	{
		if(signal(n,SIG_IGN) != SIG_IGN)
		{
			st.trapflg[n] |= SIGIGNORE;
			st.trapflg[n] &= ~SIGFAULT;
			return(0);
		}
		st.trapflg[n] = SIGOFF;
	}
	return(1);
}

/*
 * Turn on trap handler for signal <n>
 */

void	sig_ontrap(n)
register int n;
{
	register int flag;
	if(n==DEBUGTRAP)
		sh.trapnote |= TRAPSET;
	/* don't do anything if already set or off by parent */
	else if(!(st.trapflg[n]&(SIGFAULT|SIGOFF)))
	{
		flag = st.trapflg[n];
		if(signal(n,(VOID(*)())sh_fault)==SIG_IGN) 
		{
			/* has it been set to ignore by shell */
			if(flag&SIGIGNORE)
				flag |= SIGFAULT;
			else
			{
				/* It ignored already, keep it ignored */ 
				sig_ignore(n);
				flag = SIGOFF;
			}
		}
		else
			flag |= SIGFAULT;
		flag &= ~(SIGSET|TRAPSET|SIGIGNORE|SIGMOD);
		st.trapflg[n] = flag;
	}
}

/*
 * Restore to default signals
 * Do not free the trap strings if flag is non-zero
 */

void	sig_reset(flag)
{
	register int 	i;
	register wchar_t *t;
	i=MAXTRAP;
	while(i--)
	{
		t=st.trapcom[i];
		if(t==0L || *t)
		{
			if(flag)
				st.trapcom[i] = 0L; /* don't free the traps */
			sig_clear(i);
		}
		st.trapflg[i] &= ~(TRAPSET|SIGSET);
	}
	sh.trapnote=0;
}

/*
 * reset traps at start of function execution
 * keep track of which traps are caught by caller in case they are modified
 * flag==0 before function, flag==1 after function
 */

void	sig_funset(flag)
{
	register int 	i;
	register wchar_t *tp;
	i=MAXTRAP;
	while(i--)
	{
		tp = st.trapcom[i];
		if(flag==0)
		{
			if(tp && *tp==0L)
				st.trapflg[i] = SIGOFF;
			else
			{
				if(tp)
					st.trapflg[i] |= SIGCAUGHT;
				st.trapflg[i] &= ~(TRAPSET|SIGSET);
			}
			st.trapcom[i] = 0L;
		}
		else if(tp)
			sig_clear(i);
	}
	sh.trapnote = 0;
}

/*
 * free up trap if set and restore signal handler if modified
 */

void	sig_clear(n)
register int 	n;
{
	register int flag = st.trapflg[n];
	register wchar_t *t;
	if(t=st.trapcom[n])
	{
		free((char *)t);
		st.trapcom[n]=0;
		flag &= ~(TRAPSET|SIGSET);
	}
	if(flag&(SIGFAULT|SIGMOD|SIGIGNORE))
	{
		if(flag&SIGCAUGHT)
		{
			if(flag&(SIGMOD|SIGIGNORE))
				signal(n, sh_fault);
		}
		else if((flag&SIGDONE))
		{
			if(t || (flag&SIGIGNORE))
				signal(n, sh_done);
		}
		else
			 signal(n, SIG_DFL);
		flag &= ~(SIGMOD|SIGFAULT|SIGIGNORE);
		if(flag&SIGCAUGHT)
			flag |= SIGFAULT;
		else if(flag&SIGDONE)
			flag |= SIGMOD;
	}
	st.trapflg[n] = flag;
	if(n==SIGTERM && (st.states&INTFLG) && !(st.states&FORKED))
		sig_ignore(SIGTERM);
}


/*
 * check for traps
 */

void	sh_chktrap()
{
	register int 	i=MAXTRAP;
	register wchar_t *t;
#ifdef JOBS
	if(job.waitsafe)
		job_wait((pid_t)0);
#endif /* JOBS */
	/* process later if doing command substitution */
	if(st.subflag)
		return;
	sh.trapnote &= ~(TRAPSET|SIGSLOW);
	if((st.states&ERRFLG) && sh.exitval)
	{
		if(st.trapcom[ERRTRAP])
			st.trapflg[ERRTRAP] = TRAPSET;
		if(is_option(ERRFLG))
			sh_exit(sh.exitval);
	}
	while(--i)
	{
		if(st.trapflg[i]&TRAPSET)
		{
			st.trapflg[i] &= ~TRAPSET;
			if(t=st.trapcom[i])
			{
				int savxit=sh.exitval;
				sh.intrap++;
				sh_eval(t);
				sh.intrap--;
				p_flush();
				sh.exitval=savxit;
				exitset();
			}
		}
	}
	if(st.trapcom[DEBUGTRAP])
	{
		st.trapflg[DEBUGTRAP] |= TRAPSET;
		sh.trapnote |= TRAPSET;
	}
}

void
mysleep(ticks)
int ticks;
{
	sigset_t set, oset;
	struct sigaction act, oact;


	/*
	 * add SIGALRM to mask
	 */

	(void)sigemptyset(&set);
	(void)sigaddset(&set, SIGALRM);
	(void)sigprocmask(SIG_BLOCK, &set, &oset);

	/*
	 * catch SIGALRM
	 */

	(void)sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = sh_fault;
	(void)sigaction(SIGALRM, &act, &oact);

	/*
	 * start alarm and wait for signal
	 */

	(void)alarm(ticks);
	sleeping = 1;
	(void)sigsuspend(&oset);
	sleeping = 0;

	/*
	 * reset alarm, catcher and mask
	 */

	(void)alarm(0); 
	(void)sigaction(SIGALRM, &oact, NULL);
	(void)sigprocmask(SIG_SETMASK, &oset, 0);
}
