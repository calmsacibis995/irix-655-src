/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:include/defs.h	1.2.4.1"

/*
 *	UNIX shell
 *	S. R. Bourne
 *	Rewritten by David Korn
 *
 */

#include	"sh_config.h"
#include	<setjmp.h>
#include	<signal.h>
#include	<string.h>
#include 	<locale.h>
#include	<msgs/uxsgish.h>
#ifndef NSIG
#   define NSIG	32
#endif /* NSIG */
#ifdef _unistd_h
#   include	<unistd.h>
#endif /* _unistd_h */
#ifdef _sys_times_
#   include	<sys/times.h>
#else
  	struct tms
	{
		time_t	tms_utime;
		time_t	tms_stime;
		time_t	tms_cutime;
		time_t	tms_cstime;
	};
#endif /* _sys_times */

struct sysnod		/* readonly tables */
{
#ifdef apollo
	/* pointers can not be in readonly sections */
	const char   sysnam[28];
#else
	const wchar_t	*sysnam;
#endif	/* apollo */
	unsigned sysval;
};

struct sysmsgnod		/* readonly tables */
{
	char const 	*sysmsgid;
#ifdef apollo
	/* pointers can not be in readonly sections */
	const char   sysnam[28];
#else
	const char	*sysnam;
#endif	/* apollo */
	unsigned sysval;
};

struct sys2msgnod		/* readonly tables */
{
	const char 	*sysmsgid;
	const wchar_t	*sysnam;
	const char 	*sys2msgid;
	const wchar_t	*sys2nam;
	unsigned sysval;
};

/* typedefs used in the shell */
typedef const char		MSG[];
typedef const wchar_t		WCMSG[];
typedef const struct sysnod	SYSTAB[];
typedef const struct sysmsgnod	SYSMSGTAB[];
typedef const struct sys2msgnod	SYS2MSGTAB[];

#include	"name.h"
#include	"shnodes.h"
#include	"stak.h"
#include	"shtype.h"


/* error exits from various parts of shell */
#define ERROR		1
#define SYNBAD		2
#define CMDBAD		124	/* For wordexp: command substitution not allowed */
#define CTXBAD		125	/* For wordexp: special chars in bad context */
#define EXEBAD		126	/* File found but not executable */
#define FNDBAD		127	/* File not found */
#define PIDBAD		127	/* Process id not found for b_wait() */

#define BYTESPERWORD	((unsigned)sizeof(wchar_t *))
#define WC_SZ	(sizeof(wchar_t))
#define	NIL	((wchar_t*)0)
#define ENDARGS	NIL	/* arg list terminator */
#ifndef NULL
#   define NULL 0
#endif
#define WCNULL  0L
#define NULLSTR		((char*)e_nullstr)
#define WCNULLSTR	((wchar_t*)wc_e_nullstr)

#define OPATH	2	/* path offset for path_join */
			/* leaves room for _= */
#define WC_OPATH  (OPATH*(sizeof(wchar_t))) 

#define round(a,b)      ((sizeof(wchar_t*)==sizeof(int))?\
				(((int)(((a)+b)-1))&~((b)-1)):\
				(((long)(((a)+b)-1))&~((b)-1)))
#define eq(a,b)		(wcscmp(a,b)==0)
#define max(a,b)	((a)>(b)?(a):(b))
#define assert(x)	;
#define exitset()	(sh.savexit=sh.exitval)

/* flags */

typedef long optflag;
#ifdef INT16
#   ifndef pdp11
#   define _OPTIM_	1
#   endif /* pdp11 */
#endif /* INT16 */

#ifdef _OPTIM_
#   define _HIGH_	1
#   define _LOW_	1-_HIGH_
#   define is_option(x)	((x)&0xffffL?\
			st.flags.i[_LOW_]&(unsigned int)(x):\
			st.flags.i[_HIGH_]&(unsigned int)((x)>>16))
#   define on_option(x)	((x)&0xffffL?\
				(st.flags.i[_LOW_] |= (unsigned int)(x)):\
				(st.flags.i[_HIGH_] |= (unsigned int)((x)>>16)))
#   define off_option(x)	((x)&0xffffL?\
				(st.flags.i[_LOW_] &= ~(unsigned int)(x)):\
				(st.flags.i[_HIGH_] &= ~(unsigned int)((x)>>16)))
#else
#   define is_option(x)	(st.flags.l & (x))
#   define on_option(x)	(st.flags.l |= (x))
#   define off_option(x)	(st.flags.l &= ~(x))
#endif /* _OPTIM_ */

#define Fixflg	1
#define Errflg	2
#define Readpr	3
#define Monitor	4
#define	Intflg	5
#define Rshflg	6
#define Execpr	7
#define Keyflg	8
#define Noset	9
#define Noglob	10
#define Allexp	11
#define Wordexp	12
#define Noeof	13
#define Noclob	14
#define Markdir	15
#define Bgnice	16
#define Editvi	17
#define Viraw	18
#define Oneflg	19
#define Hashall	20
#define Stdflg	21
#define Noexec	22
#define Notify	23
#define Gmacs	24
#define Emacs	25
#define	Privmod 26
#ifdef apollo
#   define	Aphysical	27
#endif /* apollo */
#define Nolog	28
#define Cflag	29
#define Nocmdst	30

#define FIXFLG	(1<<Fixflg) /* used also as a state */
#define	ERRFLG	(1<<Errflg) /* used also as a state */
#define	READPR	(1<<Readpr) /* used also as a state */
#define MONITOR	(1<<Monitor)/* used also as a state */
#define	INTFLG	(1<<Intflg) /* used also as a state */
#define	RSHFLG	(1L<<Rshflg)
#define	EXECPR	(1L<<Execpr)
#define	KEYFLG	(1L<<Keyflg)
#define NOSET	(1L<<Noset)
#define NOGLOB	(1L<<Noglob)
#define ALLEXP	(1L<<Allexp)
#define WORDEXP	(1L<<Wordexp)
#define NOEOF	(1L<<Noeof)
#define NOCLOB	(1L<<Noclob)
#define EMACS	(1L<<Emacs)
#define BGNICE	(1L<<Bgnice)
#define EDITVI	(1L<<Editvi)
#define VIRAW	(1L<<Viraw)
#define	ONEFLG	(1L<<Oneflg)
#define HASHALL	(1L<<Hashall)
#define	STDFLG	(1L<<Stdflg)
#define	NOEXEC	(1L<<Noexec)
#define	NOTIFY	(1L<<Notify)
#define GMACS	(1L<<Gmacs)
#define MARKDIR	(1L<<Markdir)
#define PRIVM	(1L<<Privmod)
#ifdef apollo
#   define PHYSICAL	(1L<<Aphysical)
#endif /* apollo */
#define NOLOG	(1L<<Nolog)
#define CFLAG	(1L<<Cflag)
#define NOCMDST	(1L<<Nocmdst)	/* Disable command substitution with WORDEXP */


/* states */
/* low numbered states are same as flags */
#define GRACE		0x1
#define	PROMPT		INTFLG
#define	FORKED		0x80
#define	PROFILE		0x100	/* set when processing profiles */
#define IS_TMP		0x200	/* set when TMPFD is available */
#define WAITING		0x400	/* set when waiting for command input */
#define RM_TMP		0x800	/* temp files to remove on exit */
#define FUNCTION 	0x1000	/* set when entering a function */
#define RWAIT		0x2000	/* set when waiting for a read */
#define BUILTIN		0x4000	/* set when processing built-in command */
#define LASTPIPE	0x8000	/* set for last element of a pipeline */
#ifdef VFORK
#   define VFORKED	0x10000	/* only used with VFORK mode */
#else
#   define VFORKED	0	
#endif /* VFORK */
#define CMD_SPC		0x20000 /* For builtin 'command' special features */
#define NOFUNC		0x40000 /* Builtin 'command' disables function lookup */
#define COMSUB		0x80000 /* Command substitution parsing */


#define FORKLIM 32		/* fork constant */
#define MEMSIZE   32*sizeof(int)/* default associative memory size for shell.
					Must be a power of 2 */
#define NL	L'\n'
#define SP	L' '
#define HIGHBIT	0200
#define TO_PRINT 0100		/* bit to set for printing control char */
#define MINTRAP	0
#define MAXTRAP NSIG+3		/* maximum number of traps */

/* print interface routines */
#ifdef PROTO
    extern void p_flush(void);
    extern void p_list(int,wchar_t*[]);
    extern void p_nchr(wchar_t,int);
    extern void p_char(wchar_t);
    extern void p_num(int,wchar_t);
    extern void p_prp(const wchar_t*);
    extern void p_setout(int);
    extern void p_str(const wchar_t*,wchar_t);
    extern void p_sub(int,wchar_t);
#ifdef sgi
#define p_time(x,y) lp_time(x,y)
    extern void lp_time(clock_t,int);
#else
    extern void p_time(clock_t,int);
#endif
#else
    extern void p_flush();
    extern void p_list();
    extern void p_nchr();
    extern void p_char();
    extern void p_num();
    extern void p_prp();
    extern void p_setout();
    extern void p_str();
    extern void p_sub();
    extern void	p_time();
#endif /*PROTO */


/* argument processing routines */
#ifdef PROTO
    extern wchar_t 		**arg_build(int*,struct comnod*);
    extern void 		arg_clear(void);
    extern wchar_t 		*arg_dolminus(void);
    extern struct dolnod	*arg_free(struct dolnod*,int);
    extern struct dolnod	*arg_new(wchar_t*[],struct dolnod**);
    extern int			arg_opts(int,wchar_t**,unsigned int);
    extern void 		arg_reset(struct dolnod*,struct dolnod*);
    extern void 		arg_set(wchar_t*[]);
    extern struct dolnod	*arg_use(void);
#else
    extern wchar_t 		**arg_build();
    extern void 		arg_clear();
    extern wchar_t 		*arg_dolminus();
    extern struct dolnod	*arg_free();
    extern struct dolnod	*arg_new();
    extern int			arg_opts();
    extern void 		arg_reset();
    extern void 		arg_set();
    extern struct dolnod	*arg_use();
#endif /*PROTO */

extern wchar_t		*opt_arg;
extern wchar_t		opt_opt;
extern int		opt_index;
extern int		opt_sp;
extern int		opt_plus;

/* routines for name/value pair environment */
#ifdef PROTO
    extern void 		env_arrayset(struct namnod*,int,wchar_t*[]);
    extern wchar_t 		**env_gen(void);
    extern int			env_init(void);
    extern struct namnod	*env_namset(wchar_t*,struct Amemory*,int);
    extern void 		env_nolocal(struct namnod*);
    extern void 		env_prattr(struct namnod*);
    extern int			env_prnamval(struct namnod*,wint_t);
    extern int 			env_readline(wchar_t**,int,int);
    extern void 		env_setlist(struct argnod*,int);
    extern void 		env_scan(int,int,struct Amemory*,int,int);
#else
    extern void 		env_arrayset();
    extern wchar_t 		**env_gen();
    extern int			env_init();
    extern struct namnod	*env_namset();
    extern void 		env_nolocal();
    extern void 		env_prattr();
    extern int			env_prnamval();
    extern int	 		env_readline();
    extern void 		env_setlist();
    extern void 		env_scan();
#endif /*PROTO */

/* pathname handling routines */
#ifdef PROTO
    extern void 	path_alias(struct namnod*,wchar_t*);
    extern wchar_t 	*path_absolute(const wchar_t*);
    extern wchar_t 	*path_basename(const wchar_t*);
    extern wchar_t		*pathcanon(wchar_t*);
    extern void 	path_exec(wchar_t*[],struct argnod*);
    extern int		path_open(const wchar_t*,wchar_t*);
    extern wchar_t 	*path_get(const wchar_t*);
    extern wchar_t 	*path_join(wchar_t*,const wchar_t*);
    extern wchar_t 	*path_pwd(int);
    extern int		path_search(const wchar_t*,int);
#   ifdef LSTAT
	extern int	path_physical(wchar_t*);
#   endif /* LSTAT */
#   ifndef INT16
	extern wchar_t	*path_relative(const wchar_t*);
#   endif /* INT16 */
#else
    extern void 	path_alias();
    extern wchar_t 	*path_absolute();
    extern wchar_t 	*path_basename();
    extern wchar_t		*pathcanon();
    extern void 	path_exec();
    extern int		path_open();
    extern wchar_t 	*path_get();
    extern wchar_t 	*path_join();
    extern wchar_t 	*path_pwd();
    extern int		path_search();
#   ifdef LSTAT
	extern int	path_physical();
#   endif /* LSTAT */
#   ifndef INT16
	extern wchar_t	*path_relative();
#   endif /* INT16 */
#endif /*PROTO */

/* error messages */
extern MSG	e_access;
extern MSG	e_alias;
extern MSG	e_aliasspc;
extern MSG      e_ambiguous;
extern MSG	e_argexp;
extern MSG	e_arglist;
extern MSG	e_atline;
extern MSG      e_badscale;
extern MSG      e_argv_conv;
extern MSG      e_env_conv;
extern MSG      e_hist_conv;
extern MSG      e_input_conv;
extern MSG	e_bltfn;
extern WCMSG	wc_e_builtexec;
extern WCMSG	wc_e_colon;
extern MSG	e_combo;
extern WCMSG	wc_e_defpath;
extern MSG	e_done;
extern WCMSG	wc_e_dot;
#ifdef ECHO_N
   extern MSG	e_echobin;
   extern MSG	e_echoflag;
#endif	/* ECHO_N */
extern WCMSG	wc_e_envmarker;
extern MSG	e_exec;
extern WCMSG	wc_e_fnhdr;
extern MSG	e_fork;
extern MSG	e_found;
extern WCMSG	wc_e_function;
extern MSG	e_function;
extern MSG	e_getmap;
extern MSG	e_getvers;
extern MSG	e_heading;
extern MSG	e_invalidnm;
#ifdef ELIBACC
    extern MSG	e_libacc;
    extern MSG	e_libbad;
    extern MSG	e_libscn;
    extern MSG	e_libmax;
#endif	/* ELIBACC */
extern MSG	e_logout;
extern MSG	e_mailmsg;
extern WCMSG	wc_e_minus;
extern MSG	e_nargs;
extern MSG	e_nlorsemi;
extern MSG      e_nosuchres;
extern MSG	e_no_access;
extern MSG	e_nullstr;
extern WCMSG	wc_e_nullstr;
extern MSG	e_off;
extern MSG	e_on;
extern MSG	e_option;
extern MSG	e_pexists;
#ifdef FLOAT
   extern MSG	e_precision;
#endif /* FLOAT */
#ifdef SHELLMAGIC
   extern MSG	e_prohibited;
#endif /* SHELLMAGIC */
extern MSG	e_pwd;
extern MSG	e_query;
extern MSG	e_real;
extern MSG	e_restricted;
extern MSG	e_setmap;
extern WCMSG	wc_e_setpwd;
extern MSG	e_setvpath;
extern WCMSG	wc_e_sptbnl;
extern WCMSG	wc_e_stdprompt;
extern MSG	e_subst;
extern WCMSG	wc_e_supprompt;
extern MSG	e_swap;
extern MSG	e_sys;
extern MSG	e_trap;
extern MSG	e_toobig;
extern WCMSG	wc_e_traceprompt;
extern MSG	e_txtbsy;
extern MSG      e_ulimit;
extern MSG	e_undefsym;
extern MSG	e_user;
extern WCMSG	wc_e_version;
extern MSG	e_write;
extern MSG	e_command;
extern MSG	is_;
extern MSG	is_alias;
extern MSG	is_builtin;
extern MSG	is_spc_builtin;
extern MSG	is_function;
extern MSG	is_reserved;
extern MSG	is_talias;
extern MSG	is_xalias;
extern MSG	is_xfunction;
extern MSG	is_ufunction;
extern MSG	is_pathfound;
extern MSG	e_recursive;
extern WCMSG	wc_e_recursive;
#ifdef sgi
extern MSG	e_badmagic;
#endif
extern MSG	sc_hours;
extern MSG	sc_minutes;
extern MSG	sc_seconds;
extern MSG	sc_megabytes;
extern MSG	sc_kbytes;
extern MSG	e_cant;
extern MSG	e_set;
extern MSG	e_remove;
extern MSG	e_hard;
extern MSG	e_limit;
extern MSG	e_malloc_freejobs;
extern MSG	e_jobs;

/* frequently referenced routines */
#ifdef PROTO
    extern unsigned	alarm(unsigned);
    extern void 	free(void*);
    extern void 	*malloc(unsigned);
    extern wchar_t 	*mac_expand(wchar_t*);
    extern wchar_t 	*mac_trim(wchar_t*,int);
    extern int 		mac_here(struct ionod*);
    extern wchar_t 	*mac_try(wchar_t*);
    extern int		sh_access(wchar_t*,int);
    extern void 	sh_cfail(wchar_t*);
    extern wchar_t 	*sh_copy(const wchar_t*,wchar_t*);
    extern void 	sh_fail(const wchar_t*,wchar_t*,int);
    extern void 	sh_fail_why(const wchar_t*,wchar_t*,int);
    extern void 	sh_funct(union anynode*,wchar_t*[],int,struct argnod*);
    extern wchar_t 	*sh_heap(const wchar_t*);
    extern wchar_t 	*sh_itos(int);
    extern int		sh_lookup(const wchar_t*,SYSTAB);
    extern wchar_t 	*sh_substitute(const wchar_t*,const wchar_t*,wchar_t*);
    extern void 	sh_trim(wchar_t*);
    extern int 		sh_whence(wchar_t**,int);
    extern int		strmatch(const wchar_t*,const wchar_t*);
    extern time_t	time(time_t*);
    extern clock_t	times(struct tms*);
    extern int		ksh_chdir(wchar_t *);
    extern int		ksh_unlink(wchar_t *);
#   ifdef FLOAT
	extern double	sh_arith(wchar_t*);
#else
	extern long long sh_arith(wchar_t*);
#   endif /* FLOAT */
    extern int		xpg_compliant(void);
    extern wchar_t	*ksh_gettxt(const char *, const char*);
#else
#error always use proto now
#endif /* PROTO */

/*
 * Saves the state of the shell
 */

struct sh_scoped
{
	struct ionod	*iotemp;	/* link list of here doc files */
	struct slnod	*staklist;	/* link list of function stacks */
	unsigned	states;
	union
	{
		long	l;
#ifdef _OPTIM_
		int	i[2];
#endif /* _OPTIM_ */
	}		flags;
	jmp_buf		jmpbuf;
	int		breakcnt;
	int		execbrk;
	int		loopcnt;
	int		fn_depth;
	int		dot_depth;
	wint_t		peekn;
	wchar_t		*cmdadr;
	VOID		(*intfn)();	/* Interrupt handler */	
	int		cmdline;
	int		firstline;
	int		exec_flag;
	int		subflag;
	int		dolc;
	wchar_t		**dolv;
	struct ionod	*iopend;
	struct argnod	*gchain;
	int		ioset;
	int		linked;
	struct fileblk	*standin;
	int		curin;
	int		standout;
	wchar_t		*trapcom[MAXTRAP];
	unsigned char	trapflg[MAXTRAP];
};

extern struct sh_scoped st;

#define opt_flags		st.flags.l

struct sh_static
{
	struct Amemory	*alias_tree;	/* for alias names */
	struct Amemory	*track_tree;	/* for tracked aliases*/
	struct Amemory	*fun_tree;	/* for function names */
	struct Amemory	*var_tree;	/* for shell variables*/
	struct namnod	*bltin_nodes;	/* pointer to built-in variables */
	struct namnod	*bltin_cmds;	/* pointer to built-in commands */
	wchar_t		*shname;	/* shell name */
	wchar_t		*shinvoke;	/* invoking shell name */
	wchar_t		*cmdname;	/* name of current command */
	wchar_t		*lastpath;	/* last alsolute path found */
	wchar_t		*comdiv;	/* points to sh -c argument */
	uid_t 		userid;		/* real user id */
	uid_t 		euserid;	/* effective user id */
	gid_t 		groupid;	/* real group id */
	gid_t 		egroupid;	/* effective group id */
	jmp_buf		subshell;	/* jump here for subshell */
	jmp_buf		errshell;	/* return here on failures */
	jmp_buf		*freturn;	/* return for functions return or
						fatal errors */
	wchar_t		*sigmsg[NSIG+1];/* pointers to signal messages */
	int		exitval;
	wchar_t		*lastarg;
	wchar_t		*pwd;		/* present working directory */
	int		oldexit;
	int		curout;		/* current output descriptor */
	pid_t		pid;		/* process id of shell */
	pid_t		bckpid;		/* background process id */
	pid_t		subpid;		/* command substitution process id */
	long		ppid;		/* parent process id of shell */
	int		savexit;
	int		topfd;
	int		trapnote;
	char		login_sh;
	wchar_t		nested_sub;	/* for nested command substitution */
	wchar_t		heretrace;	/* set when tracing here doc */	
	wchar_t		intrap;		/* set when processing trap */
	wchar_t		*readscript;	/* set before reading a script */
	int		reserv;		/* set when reserved word possible */
	int		wdset;		/* states for lexical analyzer
						see sym.h */
	struct argnod	*wdarg;		/* points to current token */
	int		wdnum;		/* number associated with token */
	wint_t		wdval;		/* type of current token */
	wint_t		owdval;		/* saved token type for EOF */
	int		olineno;	/* linenumber of saved token */
	int		lastsig;	/* last signal received */
	union io_eval	un;		/* used for sh_eval */
	int		*inpipe;	/* input pipe pointer */
	int		*outpipe;	/* output pipe pointer */
	int		cpipe[2];
	pid_t		cpid;
	struct fileblk	*savio;		/* top of io stack */
};

extern struct sh_static sh;
extern time_t	sh_mailchk;
extern int	errno;
extern int	sh_numjobs;

#ifdef pdp11
#   define ulong	long
#   ifndef INT16
#   define INT16
#   endif /* INT16 */
#else
#   define ulong	unsigned long
#endif	/* pdp11 */

#ifdef INT16
#   define path_relative(p)	(p)
#endif	/* INT16 */

struct sh_optget
{
	int     opt_index;
	wchar_t  opt_opt;
	int     opt_sp;
	int     opt_plus;
};
