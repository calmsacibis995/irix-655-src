/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:sh/msg.c	1.5.4.2"

/*
 *	UNIX shell
 *	S. R. Bourne
 *	Rewritten by David Korn
 *
 *	AT&T Bell Laboratories
 *
 */

#include	<errno.h>
#include	"defs.h"
#include	"sym.h"
#include	"builtins.h"
#include	"test.h"
#include	"timeout.h"
#include	"history.h"

#ifdef MULTIBYTE
#include	"national.h"
    MSG e_version = "\n@(#)Version M-11/16/88f\0\n";
#else
    WCMSG wc_e_version = L"\n@(#)Version 11/16/88f\0\n";
#endif /* MULTIBYTE */

extern struct Bfunction sh_randnum;
extern struct Bfunction sh_seconds;
extern struct Bfunction line_numbers;
extern struct Bfunction opt_indexs;

/* error messages */
MSG	e_timewarn	= "\r\n\007shell will time out in 60 seconds";
MSG	e_timeout	= "timed out waiting for input";
MSG	e_mailmsg	= "you have mail in $_";
MSG	e_query		= "no query process";
MSG	e_history	= "no history file";
MSG	e_option	= "bad option(s)";
MSG	e_space		= "no space";
MSG	e_argexp	= "argument expected";
MSG	e_bracket	= "] missing";
MSG	e_number	= "bad number";
WCMSG	wc_e_number	= L"bad number";
MSG	e_nullset	= "parameter null or not set";
MSG	e_notset	= "parameter not set";
MSG	e_subst		= "bad substitution";
MSG	e_create	= "cannot create";
MSG	e_restricted	= "restricted";
MSG	e_fork		= "cannot fork: too many processes";
MSG	e_pexists	= "process already exists";
MSG	e_fexists	= "file already exists";
MSG	e_swap		= "cannot fork: no swap space";
MSG	e_pipe		= "cannot make pipe";
MSG	e_open		= "cannot open";
MSG	e_logout	= "Use 'exit' to terminate this shell";
MSG	e_arglist	= "arg list too long";
MSG	e_txtbsy	= "text busy";
MSG	e_toobig	= "too big";
MSG	e_exec		= "cannot execute";
MSG	e_pwd		= "cannot access parent directories";
MSG	e_found		= " not found";
MSG	e_flimit	= "too many open files";
MSG	e_ulimit	= "exceeds allowable limit";
MSG	e_subscript	= "subscript out of range";
MSG	e_nargs		= "bad argument count";
MSG	e_combo		= "bad option combination";
MSG     e_nosuchres     = "no such resource";
MSG     e_badscale      = "Improper or unknown scale factor";
MSG     e_argv_conv     = "Invalid multibyte sequence in argv";
MSG     e_env_conv      = "Invalid multibyte sequence in env variable: ";
MSG     e_input_conv    = "Invalid multibyte sequence in input";
MSG     e_hist_conv     = "Invalid multibyte sequence in history file";

#ifdef ELIBACC
    /* shared library error messages */
    MSG	e_libacc 	= "can't access a needed shared library";
    MSG	e_libbad	= "accessing a corrupted shared library";
    MSG	e_libscn	= ".lib section in a.out corrupted";
    MSG	e_libmax	= "attempting to link in too many libs";
#endif	/* ELIBACC */
#ifdef EMULTIHOP
    MSG   e_multihop	= "multihop attempted";
#endif /* EMULTIHOP */
#ifdef ENAMETOOLONG
    MSG   e_longname	= "name too long";
#endif /* ENAMETOOLONG */
#ifdef ENOLINK
    MSG	e_link		= "remote link inactive";
#endif /* ENOLINK */
MSG	e_access	= "permission denied";
MSG	e_direct	= "bad directory";
MSG	e_notdir	= "not a directory";
MSG	e_file		= "bad file unit number";
MSG	e_trap		= "bad trap";
MSG	e_readonly	= "is read only";
MSG	e_ident		= "is not an identifier";
MSG	e_aliname	= "invalid alias name";
MSG	e_testop	= "unknown test operator";
MSG	e_alias		= " alias not found";
MSG	e_aliasspc	= "alias ";
MSG	e_function	= "unknown function";
WCMSG	wc_e_function	= L"unknown function";
MSG	e_format	= "bad format";
MSG	e_on		= "on";
MSG	e_off		= "off";
MSG	e_undefsym	= "undefined symbols";
MSG	e_getmap	= "cannot get mapping";
MSG	e_getvers	= "cannot get versions";
MSG	e_setmap	= "cannot set mapping";
MSG	e_setvpath	= "cannot set vpath";
MSG	e_invalidnm	= "invalid name";
MSG	e_no_access	= "not accessible";
MSG	e_nlorsemi	= "newline or ;";
WCMSG	wc_e_builtexec	= L"/sbin/builtin_exec";
MSG	e_write		= "write";
MSG	is_reserved	= " is a reserved keyword";
MSG	is_builtin	= " is a shell builtin";
MSG	is_spc_builtin	= " is a special shell builtin";
MSG	is_alias	= " is an alias for ";
MSG	is_function	= " is a function";
MSG	is_xalias	= " is an exported alias for ";
MSG	is_talias	= " is a tracked alias for ";
MSG	is_xfunction	= " is an exported function";
MSG	is_ufunction	= " is an undefined function";
MSG	is_pathfound	= " is found in PATH";
MSG	is_		= " is ";
WCMSG	wc_e_fnhdr	= L"\n{\n";
WCMSG	wc_e_runvi	= L"fc -e \"${VISUAL:-${EDITOR:-vi}}\" ";
#ifdef JOBS
#   ifdef SIGTSTP
	MSG	e_newtty	= "Switching to new tty driver...";
	MSG	e_oldtty	= "Reverting to old tty driver...";
	MSG	e_no_start	= "Cannot start job control";
	MSG	e_no_jctl	= "No job control";
	MSG	e_terminate	= "You have stopped jobs";
#   endif /*SIGTSTP */
   MSG	e_done		= " Done";
   WCMSG	wc_e_nlspace	= L"\n      ";
   MSG	e_Running	= " Running";
   MSG	e_ambiguous	= "Ambiguous";
   MSG	e_running	= "You have running jobs";
   MSG	e_no_job	= "no such job";
   MSG	e_no_proc	= "no such process";
   MSG	e_killcolon	= "kill: ";
   MSG	e_jobusage	= "Arguments must be %job or process ids";
   MSG	e_kill		= "kill";
#endif /* JOBS */
MSG	e_coredump	= "(coredump)";
#ifdef DEVFD
    WCMSG	wc_e_devfd	= L"/dev/fd/";
#endif /* DEVFD */
#ifdef VPIX
    MSG	e_vpix		= "/vpix";
    MSG	e_vpixdir	= "/usr/bin";
#endif /* VPIX */
#ifdef apollo
    MSG e_rootnode 	= "Bad root node specification";
    MSG e_nover    	= "Version not defined";
    MSG e_badver   	= "Unrecognized version";
#endif /* apollo */
#ifdef LDYNAMIC
    MSG e_badinlib	= "Cannot inlib";
#endif /* LDYNAMIC */
#ifdef sgi
    MSG e_badmagic	= "Program not supported by architecture";
#endif /* sgi */

/* string constants */
WCMSG	wc_test_unops	= L"LSVOGClaeohrwxdcbfugkpsnzt";
MSG	e_heading	= "Current option settings";
MSG	e_nullstr	= "";
WCMSG	wc_e_nullstr	= L"";
WCMSG	wc_e_sptbnl	= L" \t\n";
WCMSG	wc_e_defpath	= L"/bin:/usr/bin:";
WCMSG	wc_e_defedit	= L"/bin/ed";
WCMSG	wc_e_colon	= L": ";
WCMSG	wc_e_minus	= L"-";
MSG	e_endoffile	= "end of file";
MSG	e_unexpected 	= " unexpected";
MSG	e_unmatched 	= " unmatched";
MSG	e_unknown 	= "<command unknown>";
MSG	e_atline	= " at line ";
WCMSG	wc_e_devnull	= L"/dev/null";
WCMSG	wc_e_traceprompt= L"+ ";
WCMSG	wc_e_supprompt	= L"# ";
WCMSG	wc_e_stdprompt	= L"$ ";
WCMSG	wc_e_profile	= L"${HOME:-.}/.profile";
WCMSG	wc_e_sysprofile	= L"/etc/profile";
WCMSG	wc_e_suidprofile	= L"/etc/suid_profile";
WCMSG	wc_e_crondir	= L"/var/spool/cron/atjobs";
#ifndef INT16
   MSG	e_prohibited	= "login setuid/setgid shells prohibited";
#endif /* INT16 */
#ifdef SUID_EXEC
   MSG	e_suidexec	= "/etc/suid_exec";
#endif /* SUID_EXEC */
WCMSG	wc_e_devfdNN	= L"/dev/fd/+([0-9])";
WCMSG	hist_fname	= L"/.sh_history";
MSG	e_unlimited	= "unlimited";
#ifdef ECHO_N
   MSG	e_echobin	= "/bin/echo";
   MSG	e_echoflag	= "-R";
#endif	/* ECHO_N */
WCMSG	wc_e_test		= L"test";
WCMSG	wc_e_dot	= L".";
MSG	e_bltfn		= "function ";
MSG	e_intbase	= "base";
WCMSG	wc_e_envmarker	= L"A__z";
#ifdef FLOAT
    MSG	e_precision	= "precision";
#endif /* FLOAT */
#ifdef PDUBIN
        WCMSG	wc_e_setpwd	= L"PWD=`/usr/pdu/bin/pwd 2>/dev/null`";
#else
        WCMSG	wc_e_setpwd	= L"PWD=`/bin/pwd 2>/dev/null`";
#endif /* PDUBIN */
MSG	e_real		= "\nreal";
MSG	e_user		= "user";
MSG	e_sys		= "sys";
MSG	e_command	= "command";
MSG	e_cant		= ": Can't ";
MSG	e_remove	= "remove ";
MSG	e_set		= "set ";
MSG	e_hard		= "hard ";
MSG	e_limit		= "limit";
MSG	e_malloc_freejobs = "malloc failed for freejobs list";
MSG	e_jobs 		= "too many jobs";
MSG	sc_hours	= "hours";
MSG	sc_minutes	= "minutes";
MSG	sc_megabytes	= "megabytes";
MSG	sc_seconds	= "seconds";
MSG	sc_kbytes	= "kbytes";

#ifdef apollo
#   undef NULL
#   define NULL ""
#   define e_nullstr	""
#endif	/* apollo */

/* built in names */
const struct name_value node_names[] =
{
	L"PATH",	WCNULL,	0,
	L"PS1",		WCNULL,	0,
	L"PS2",		L"> ",	N_FREE,
#ifdef apollo
	L"IFS",		L" \t\n",	N_FREE,
#else
	L"IFS",		wc_e_sptbnl,	N_FREE,
#endif	/* apollo */
	L"PWD",		WCNULL,	0,
	L"HOME",	WCNULL,	0,
	L"MAIL",	WCNULL,	0,
	L"REPLY",	WCNULL,	0,
	L"SHELL",	L"/bin/sh",	N_FREE,
	L"EDITOR",	WCNULL,	0,
#ifdef apollo
	L"MAILCHECK",	WCNULL,	N_FREE|N_INTGER,
	L"RANDOM",	WCNULL,	N_FREE|N_INTGER,
#else
	L"MAILCHECK",	(wchar_t*)(&sh_mailchk),	N_FREE|N_INTGER,
	L"RANDOM",	(wchar_t*)(&sh_randnum),	N_FREE|N_INTGER|N_BLTNOD,
#endif	/* apollo */
	L"ENV",		WCNULL,	0,
	L"HISTFILE",	WCNULL,	0,
	L"HISTSIZE",	WCNULL,	0,
	L"FCEDIT",	L"/bin/ed",	N_FREE,
	L"CDPATH",	WCNULL,	0,
	L"MAILPATH",	WCNULL,	0,
	L"PS3",		L"#? ",	N_FREE,
	L"OLDPWD",	WCNULL,	0,
	L"VISUAL",	WCNULL,	0,
	L"COLUMNS",	WCNULL,	0,
	L"LINES",	WCNULL,	0,
#ifdef apollo
	L"PPID",	WCNULL,	N_FREE|N_INTGER,
	L"_",		WCNULL,	N_FREE|N_INDIRECT|N_EXPORT,
	L"TMOUT",	WCNULL,	N_FREE|N_INTGER,
	L"SECONDS",	WCNULL,	N_FREE|N_INTGER|N_BLTNOD,
	L"ERRNO",	WCNULL,	N_FREE|N_INTGER,
	L"LINENO",	WCNULL,	N_FREE|N_INTGER|N_BLTNOD,
	L"OPTIND",	WCNULL,	N_FREE|N_INTGER,
	L"OPTARG",	WCNULL,	N_FREE|N_INDIRECT,
#else
	L"PPID",	(wchar_t*)(&sh.ppid),	N_FREE|N_INTGER,
	L"_",		(wchar_t*)(&sh.lastarg),	N_FREE|N_INDIRECT|N_EXPORT,
	L"TMOUT",	(wchar_t*)(&sh_timeout),	N_FREE|N_INTGER,
	L"SECONDS",	(wchar_t*)(&sh_seconds),	N_FREE|N_INTGER|N_BLTNOD,
	L"ERRNO",	WCNULL,	N_FREE|N_INTGER,
	L"LINENO",	(wchar_t*)(&line_numbers),	N_FREE|N_INTGER|N_BLTNOD,
	L"OPTIND",	(wchar_t*)(&opt_indexs),	N_FREE|N_INTGER|N_BLTNOD|N_IMPORT,
	L"OPTARG",	WCNULL,			N_FREE,
#endif	/* apollo */
	L"PS4",		WCNULL,	0,
	L"FPATH",	WCNULL,	0,
	L"LANG",	WCNULL,	0,
	L"LC_CTYPE",	WCNULL,	0,
#ifdef VPIX
	L"DOSPATH",	WCNULL,	0,
	L"VPIXDIR",	WCNULL,	0,
#endif	/* VPIX */
#ifdef ACCT
	L"SHACCT",	WCNULL,	0,
#endif	/* ACCT */
#ifdef MULTIBYTE
	L"CSWIDTH",	WCNULL,	0,
#endif /* MULTIBYTE */
#ifdef apollo
	L"SYSTYPE",	WCNULL,	0,
#endif /* apollo */
	L"_XPG",	L"0",	N_FREE,
	L"NOMSGSEVERITY",L"1",	N_FREE,
	L"NOMSGLABEL",	L"1",	N_FREE,
	wc_e_nullstr,	WCNULL,	0
};

#ifdef VPIX
   const char *suffix_list[] = { ".com", ".exe", ".bat", e_nullstr };
#endif	/* VPIX */

/* built in aliases - automatically exported */
const struct name_value alias_names[] =
{
#ifdef FS_3D
	L"2d",		L"set -f;_2d ",	N_FREE|N_EXPORT,
#endif /* FS_3D */
	L"autoload",	L"typeset -fu",	N_FREE|N_EXPORT,
	L"chdir",	L"cd",		N_FREE|N_EXPORT,
	L"functions",	L"typeset -f",	N_FREE|N_EXPORT,
	L"history",	L"fc -l",	N_FREE|N_EXPORT,
	L"integer",	L"typeset -i",	N_FREE|N_EXPORT,
#ifdef POSIX
	L"local",	L"typeset",	N_FREE|N_EXPORT,
#endif /* POSIX */
	L"nohup",	L"nohup ",	N_FREE|N_EXPORT,
	L"r",		L"fc -e -",	N_FREE|N_EXPORT,
#ifdef SIGTSTP
	L"stop",	L"kill -STOP",	N_FREE|N_EXPORT,
	L"suspend",	L"kill -STOP $$",	N_FREE|N_EXPORT,
#endif /*SIGTSTP */
	wc_e_nullstr,	WCNULL,	0
};

const struct name_value tracked_names[] =
{
	L"cat",		L"/bin/cat",	N_FREE|N_EXPORT|T_FLAG,
	L"chmod",	L"/bin/chmod",	N_FREE|N_EXPORT|T_FLAG,
	L"cc",		L"/bin/cc",	N_FREE|N_EXPORT|T_FLAG,
	L"cp",		L"/bin/cp",	N_FREE|N_EXPORT|T_FLAG,
	L"date",	L"/bin/date",	N_FREE|N_EXPORT|T_FLAG,
	L"ed",		L"/bin/ed",	N_FREE|N_EXPORT|T_FLAG,
#ifdef _bin_grep_
	L"grep",	L"/bin/grep",	N_FREE|N_EXPORT|T_FLAG,
#else
#  ifdef _usr_ucb_
	L"grep",	L"/usr/ucb/grep",N_FREE|N_EXPORT|T_FLAG,
#  endif /* _usr_ucb_ */
#endif	/* _bin_grep */
#ifdef _usr_bin_lp
	L"lp",		L"/usr/bin/lp",	N_FREE|N_EXPORT|T_FLAG,
#endif /* _usr_bin_lpr */
#ifdef _usr_bin_lpr
	L"lpr",		L"/usr/bin/lpr",	N_FREE|N_EXPORT|T_FLAG,
#endif /* _usr_bin_lpr */
	L"ls",		L"/bin/ls",	N_FREE|N_EXPORT|T_FLAG,
	L"make",	L"/bin/make",	N_FREE|N_EXPORT|T_FLAG,
	L"mail",	L"/bin/mail",	N_FREE|N_EXPORT|T_FLAG,
	L"mv",		L"/bin/mv",	N_FREE|N_EXPORT|T_FLAG,
	L"pr",		L"/bin/pr",	N_FREE|N_EXPORT|T_FLAG,
	L"rm",		L"/bin/rm",	N_FREE|N_EXPORT|T_FLAG,
	L"sed",		L"/bin/sed",	N_FREE|N_EXPORT|T_FLAG,
	L"sh",		L"/bin/sh",	N_FREE|N_EXPORT|T_FLAG,
#ifdef _usr_bin_vi_
	L"vi",		L"/usr/bin/vi",	N_FREE|N_EXPORT|T_FLAG,
#else
#  ifdef _usr_ucb_
	L"vi",		L"/usr/ucb/vi",	N_FREE|N_EXPORT|T_FLAG,
#  endif /* _usr_ucb_ */
#endif	/* _usr_bin_vi_ */
	L"who",		L"/bin/who",	N_FREE|N_EXPORT|T_FLAG,
	wc_e_nullstr,	WCNULL,	0
};

/* tables */
SYSTAB tab_reserved =
{
#ifdef POSIX
		{ L"!",		NOTSYM},
#endif /* POSIX */
#ifdef NEWTEST
		{ L"[[",	BTSTSYM},
#endif /* NEWTEST */
		{ L"case",	CASYM},
		{ L"do",	DOSYM},
		{ L"done",	ODSYM},
		{ L"elif",	EFSYM},
		{ L"else",	ELSYM},
		{ L"esac",	ESSYM},
		{ L"fi",	FISYM},
		{ L"for",	FORSYM},
		{ L"function",	PROCSYM},
		{ L"if",	IFSYM},
		{ L"in",	INSYM},
		{ L"select",	SELSYM},
		{ L"then",	THSYM},
		{ L"time",	TIMSYM},
		{ L"until",	UNSYM},
		{ L"while",	WHSYM},
		{ L"{",		BRSYM},
		{ L"}",		KTSYM},
		{ wc_e_nullstr,	0},
};

/*
 * The signal numbers go in the low bits and the attributes go in the high bits
 */

SYSTAB	sig_names =
{
#ifdef SIGABRT
		{ L"ABRT",	(SIGABRT+1)|(SIGDONE<<SIGBITS)},
#endif /*SIGABRT */
		{ L"ALRM",	(SIGALRM+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
#ifdef SIGAPOLLO
		{ L"APOLLO",	(SIGAPOLLO+1)},
#endif /* SIGAPOLLO */
#ifdef SIGBUS
		{ L"BUS",		(SIGBUS+1)|(SIGDONE<<SIGBITS)},
#endif /* SIGBUS */
#ifdef SIGCHLD
		{ L"CHLD",	(SIGCHLD+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
#   ifdef SIGCLD
#	if SIGCLD!=SIGCHLD
		{ L"CLD",		(SIGCLD+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
#	endif
#   endif	/* SIGCLD */
#else
#   ifdef SIGCLD
		{ L"CLD",		(SIGCLD+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
#   endif	/* SIGCLD */
#endif	/* SIGCHLD */
#ifdef SIGCONT
		{ L"CONT",	(SIGCONT+1)},
#endif	/* SIGCONT */
		{ L"DEBUG",	(DEBUGTRAP+1)},
#ifdef SIGEMT
		{ L"EMT",		(SIGEMT+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGEMT */
		{ L"ERR",		(ERRTRAP+1)},
		{ L"EXIT",	1},
		{ L"FPE",		(SIGFPE+1)|(SIGDONE<<SIGBITS)},
		{ L"HUP",		(SIGHUP+1)|(SIGDONE<<SIGBITS)},
		{ L"ILL",		(SIGILL+1)|(SIGDONE<<SIGBITS)},
		{ L"INT",		(SIGINT+1)|(SIGCAUGHT<<SIGBITS)},
#ifdef SIGIO
		{ L"IO",		(SIGIO+1)},
#endif	/* SIGIO */
#ifdef SIGIOT
		{ L"IOT",		(SIGIOT+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGIOT */
		{ L"KILL",	(SIGKILL+1)},
#ifdef SIGLAB
		{ L"LAB",		(SIGLAB+1)},
#endif	/* SIGLAB */
#ifdef SIGLOST
		{ L"LOST",	(SIGLOST+1)},
#endif	/* SIGLOST */
#ifdef SIGPHONE
		{ L"PHONE",	(SIGPHONE+1)},
#endif	/* SIGPHONE */
#ifdef SIGPIPE
		{ L"PIPE",	(SIGPIPE+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGPIPE */
#ifdef SIGPOLL
		{ L"POLL",	(SIGPOLL+1)},
#endif	/* SIGPOLL */
#ifdef SIGPROF
		{ L"PROF",	(SIGPROF+1)},
#endif	/* SIGPROF */
#ifdef SIGPWR
#   if SIGPWR>0
		{ L"PWR",		(SIGPWR+1)},
#   endif
#endif	/* SIGPWR */
		{ L"QUIT",	(SIGQUIT+1)|((SIGCAUGHT|SIGIGNORE)<<SIGBITS)},
		{ L"SEGV",	(SIGSEGV+1)},
#ifdef SIGSTOP
		{ L"STOP",	(SIGSTOP+1)},
#endif	/* SIGSTOP */
#ifdef SIGSYS
		{ L"SYS",		(SIGSYS+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGSYS */
		{ L"TERM",	(SIGTERM+1)|(SIGDONE<<SIGBITS)},
#ifdef SIGTINT
		{ L"TINT",	(SIGTINT+1)},
#endif	/* SIGTINT */
#ifdef SIGTRAP
		{ L"TRAP",	(SIGTRAP+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGTRAP */
#ifdef SIGTSTP
		{ L"TSTP",	(SIGTSTP+1)},
#endif	/* SIGTSTP */
#ifdef SIGTTIN
		{ L"TTIN",	(SIGTTIN+1)},
#endif	/* SIGTTIN */
#ifdef SIGTTOU
		{ L"TTOU",	(SIGTTOU+1)},
#endif	/* SIGTTOU */
#ifdef SIGURG
		{ L"URG",		(SIGURG+1)},
#endif	/* SIGURG */
#ifdef SIGUSR1
		{ L"USR1",	(SIGUSR1+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGUSR1 */
#ifdef SIGUSR2
		{ L"USR2",	(SIGUSR2+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGUSR2 */
#ifdef SIGVTALRM
		{ L"VTALRM",	(SIGVTALRM+1)},
#endif	/* SIGVTALRM */
#ifdef SIGWINCH
		{ L"WINCH",	(SIGWINCH+1)},
#endif	/* SIGWINCH */
#ifdef SIGWINDOW
		{ L"WINDOW",	(SIGWINDOW+1)},
#endif	/* SIGWINDOW */
#ifdef SIGWIND
		{ L"WIND",	(SIGWIND+1)},
#endif	/* SIGWIND */
#ifdef SIGXCPU
		{ L"XCPU",	(SIGXCPU+1)},
#endif	/* SIGXCPU */
#ifdef SIGXFSZ
		{ L"XFSZ",	(SIGXFSZ+1)|((SIGCAUGHT|SIGIGNORE)<<SIGBITS)},
#endif	/* SIGXFSZ */
		{ wc_e_nullstr,	0}
};

SYSMSGTAB	sig_messages =
{
#ifdef SIGABRT
	{_SGI_DMMX_s_abort,	"Abort",			(SIGABRT+1)},
#endif /*SIGABRT */
	{_SGI_DMMX_s_alarmcall,	"Alarm call",			(SIGALRM+1)},
	{_SGI_DMMX_s_buserror,	"Bus error",			(SIGBUS+1)},
#ifdef SIGCHLD
	{_SGI_DMMX_s_childstop,	"Child stopped or terminated",	(SIGCHLD+1)},
#   ifdef SIGCLD
#	if SIGCLD!=SIGCHLD
	{_SGI_DMMX_s_childdeath,"Death of Child", 		(SIGCLD+1)},
#	endif
#   endif	/* SIGCLD */
#else
#   ifdef SIGCLD
	{_SGI_DMMX_s_childdeath,"Death of Child", 		(SIGCLD+1)},
#   endif	/* SIGCLD */
#endif	/* SIGCHLD */
#ifdef SIGCONT
	{_SGI_DMMX_s_stopcont,	"Stopped process continued",	(SIGCONT+1)},
#endif	/* SIGCONT */
#ifdef SIGEMT
	{_SGI_DMMX_s_emttrap,	"EMT trap",			(SIGEMT+1)},
#endif	/* SIGEMT */
	{_SGI_DMMX_s_floatex,	"Floating exception",		(SIGFPE+1)},
	{_SGI_DMMX_s_hangup,	"Hangup",			(SIGHUP+1)},
	{_SGI_DMMX_s_illegal,	"Illegal instruction",		(SIGILL+1)},
#ifdef JOBS
	{_SGI_DMMX_s_intr,	"Interrupt",			(SIGINT+1)},
#else
	{"",		"",			(SIGINT+1)},
#endif	/* JOBS */
#ifdef SIGIO
	{_SGI_DMMX_s_iosignal,	"IO signal",			(SIGIO+1)},
#endif	/* SIGIO */
	{_SGI_DMMX_s_abort,	"Abort",			(SIGIOT+1)},
	{_SGI_DMMX_s_killed,	"Killed",			(SIGKILL+1)},
	{_SGI_DMMX_s_quit,	"Quit",				(SIGQUIT+1)},
#ifdef JOBS
	{_SGI_DMMX_s_brokenpipe,"Broken Pipe",			(SIGPIPE+1)},
#else
	{"",		"",			(SIGPIPE+1)},
#endif	/* JOBS */
#ifdef SIGPROF
	{_SGI_DMMX_s_profalarm,	"Profiling time alarm",		(SIGPROF+1)},
#endif	/* SIGPROF */
#ifdef SIGPWR
#   if SIGPWR>0
	{_SGI_DMMX_s_powerfail,	"Power fail",			(SIGPWR+1)},
#   endif
#endif	/* SIGPWR */
	{_SGI_DMMX_s_memoryfault,"Memory fault",		(SIGSEGV+1)},
#ifdef SIGSTOP
	{_SGI_DMMX_s_stopstop,	"Stopped (SIGSTOP)",		(SIGSTOP+1)},
#endif	/* SIGSTOP */
	{_SGI_DMMX_s_badsyscall,"Bad system call", 		(SIGSYS+1)},
	{_SGI_DMMX_s_term,	"Terminated",			(SIGTERM+1)},
#ifdef SIGTINT
#   ifdef JOBS
	{_SGI_DMMX_s_intr,	"Interrupt",			(SIGTINT+1)},
#   else
	{"",		"",			(SIGTINT+1)},
#   endif /* JOBS */
#endif	/* SIGTINT */
	{_SGI_DMMX_s_tracetrap,	"Trace/BPT trap",		(SIGTRAP+1)},
#ifdef SIGTSTP
	{_SGI_DMMX_s_stoptstp,	"Stopped (SIGTSTP)",		(SIGTSTP+1)},
#endif	/* SIGTSTP */
#ifdef SIGTTIN
	{_SGI_DMMX_s_stopttin,	"Stopped (SIGTTIN)",		(SIGTTIN+1)},
#endif	/* SIGTTIN */
#ifdef SIGTTOU
	{_SGI_DMMX_s_stopttou,	"Stopped (SIGTTOU)",		(SIGTTOU+1)},
#endif	/* SIGTTOU */
#ifdef SIGURG
	{_SGI_DMMX_s_sockintr,	"Socket interrupt",		(SIGURG+1)},
#endif	/* SIGURG */
#ifdef SIGUSR1
	{_SGI_DMMX_s_usrsig1,	"User signal 1",		(SIGUSR1+1)},
#endif	/* SIGUSR1 */
#ifdef SIGUSR2
	{_SGI_DMMX_s_usrsig2,	"User signal 2",		(SIGUSR2+1)},
#endif	/* SIGUSR2 */
#ifdef SIGVTALRM
	{_SGI_DMMX_s_virtalarm,	"Virtual time alarm",		(SIGVTALRM+1)},
#endif	/* SIGVTALRM */
#ifdef SIGWINCH
	{_SGI_DMMX_s_winsizechg,"Window size change", 		(SIGWINCH+1)},
#endif	/* SIGWINCH */
#ifdef SIGXCPU
	{_SGI_DMMX_s_timelimit,	"Exceeded CPU time limit",	(SIGXCPU+1)},
#endif	/* SIGXCPU */
#ifdef SIGXFSZ
	{_SGI_DMMX_s_sizelimit,	"Exceeded file size limit",	(SIGXFSZ+1)},
#endif	/* SIGXFSZ */
#ifdef SIGLOST
	{_SGI_DMMX_s_resource,	"Resources lost", 		(SIGLOST+1)},
#endif	/* SIGLOST */
#ifdef SIGLAB
	{_SGI_DMMX_s_securitychg,"Security label changed",	(SIGLAB+1)},
#endif	/* SIGLAB */
	{"",		"",			0}
};

SYSTAB tab_options=
{
	{ L"allexport",		Allexp},
	{ L"bgnice",		Bgnice},
	{ L"emacs",		Emacs},
	{ L"errexit",		Errflg},
	{ L"gmacs",		Gmacs},
	{ L"ignoreeof",		Noeof},
	{ L"interactive",	Intflg},
	{ L"keyword",		Keyflg},
	{ L"markdirs",		Markdir},
	{ L"monitor",		Monitor},
	{ L"noexec",		Noexec},
	{ L"noclobber",		Noclob},
	{ L"noglob",		Noglob},
	{ L"nolog",		Nolog},
	{ L"nounset",		Noset},
	{ L"notify",		Notify},
#ifdef apollo
	{ L"physical",		Aphysical},
#endif /* apollo */
	{ L"privileged",	Privmod},
	{ L"restricted",	Rshflg},
	{ L"trackall",		Hashall},
	{ L"verbose",		Readpr},
	{ L"vi",		Editvi},
	{ L"viraw",		Viraw},
	{ L"xtrace",		Execpr},
	{ wc_e_nullstr,		0}
};

#ifdef _sys_resource_
#   ifndef included_sys_time_
#	include <sys/time.h>
#   endif
#   include	<sys/resource.h>/* needed for ulimit */
#   define	LIM_FSIZE	RLIMIT_FSIZE
#   define	LIM_DATA	RLIMIT_DATA
#   define	LIM_STACK	RLIMIT_STACK
#   define	LIM_CORE	RLIMIT_CORE
#   define	LIM_CPU		RLIMIT_CPU
#   ifdef RLIMIT_RSS
#	define	LIM_MAXRSS	RLIMIT_RSS
#   endif /* RLIMIT_RSS */
#else
#   ifdef VLIMIT
#	include	<sys/vlimit.h>
#   endif /* VLIMIT */
#endif	/* _sys_resource_ */

#ifdef LIM_CPU
#   define size_resource(a,b) ((a)|((b)<<11))	
SYSMSGTAB limit_names =
{
	{_SGI_DMMX_r_time,	"time(seconds)       ",	size_resource(1,LIM_CPU)},
	{_SGI_DMMX_r_file,	"file(blocks)        ",	size_resource(512,LIM_FSIZE)},
	{_SGI_DMMX_r_data,	"data(kbytes)        ",	size_resource(1024,LIM_DATA)},
	{_SGI_DMMX_r_stack,	"stack(kbytes)       ",	size_resource(1024,LIM_STACK)},
#   ifdef LIM_MAXRSS
	{_SGI_DMMX_r_memory,	"memory(kbytes)      ",	size_resource(1024,LIM_MAXRSS)},
#   else
	{_SGI_DMMX_r_memory,	"memory(kbytes)      ",	size_resource(1024,0)},
#   endif /* LIM_MAXRSS */
	{_SGI_DMMX_r_coredump,	"coredump(blocks)    ",	size_resource(512,LIM_CORE)},
#   ifdef RLIMIT_NOFILE
	{_SGI_DMMX_r_nofiles,	"nofiles(descriptors)",	size_resource(1,RLIMIT_NOFILE)},
#   else
	{_SGI_DMMX_r_nofiles,	"nofiles(descriptors)",	size_resource(1,0)},
#   endif /* RLIMIT_NOFILE */
#   ifdef RLIMIT_VMEM
	{_SGI_DMMX_r_vmemory,	"vmemory(kbytes)     ",	size_resource(1024,RLIMIT_VMEM)},
#   else
	{_SGI_DMMX_r_vmemory,	"vmemory(kbytes)     ",	size_resource(1024,0)},
#   endif /* RLIMIT_VMEM */
#   ifdef RLIMIT_PTHREAD
	{_SGI_DMMX_r_concurrency,	"concurrency(threads)",	size_resource(1,RLIMIT_PTHREAD)}
#   else
	{_SGI_DMMX_r_concurrency,	"concurrency(threads)",	size_resource(1,0)}
#   endif /* RLIMIT_PTHREAD */
};

SYS2MSGTAB blimit_names =
{
	{_SGI_DMMX_l_cputime,		L"cputime      ",
	 _SGI_DMMX_sc_seconds,		L"seconds",	size_resource(1,LIM_CPU)},
	{_SGI_DMMX_l_filesize,		L"filesize     ",
	 _SGI_DMMX_sc_kbytes,		L"kbytes",	size_resource(1024,LIM_FSIZE)},
	{_SGI_DMMX_l_datasize,		L"datasize     ",
	 _SGI_DMMX_sc_kbytes,		L"kbytes",	size_resource(1024,LIM_DATA)},
	{_SGI_DMMX_l_stacksize,		L"stacksize    ",
	 _SGI_DMMX_sc_kbytes,		L"kbytes",	size_resource(1024,LIM_STACK)},
	{_SGI_DMMX_l_coredumpsize,	L"coredumpsize ",
	 _SGI_DMMX_sc_kbytes,		L"kbytes",	size_resource(1024,LIM_CORE)},
#   ifdef LIM_MAXRSS
	{_SGI_DMMX_l_memoryuse,		L"memoryuse    ",
	 _SGI_DMMX_sc_kbytes,		L"kbytes",	size_resource(1024,LIM_MAXRSS)},
#   else
	{_SGI_DMMX_l_memoryuse,		L"memoryuse    ",
	 _SGI_DMMX_sc_kbytes,		L"kbytes",	size_resource(1024,0)},
#   endif /* LIM_MAXRSS */
#   ifdef RLIMIT_NOFILE
	{_SGI_DMMX_l_descriptors,	L"descriptors  ",
			"",		L"",		size_resource(1,RLIMIT_NOFILE)},
#   else
	{_SGI_DMMX_l_descriptors,	L"descriptors  ",
	 		"",		L"",		size_resource(1,0)},
#   endif /* RLIMIT_NOFILE */
#   ifdef RLIMIT_VMEM
	{_SGI_DMMX_l_vmemory,		L"vmemory      ",
	 _SGI_DMMX_sc_kbytes,		L"kbytes",	size_resource(1024,RLIMIT_VMEM)},
#   else
	{_SGI_DMMX_l_vmemory,		L"vmemory      ",
	 _SGI_DMMX_sc_kbytes,		L"kbytes",	size_resource(1024,0)},
#   endif /* RLIMIT_VMEM */
#   ifdef RLIMIT_PTHREAD
	{_SGI_DMMX_l_threads,		L"threads      ",
			"",		L"",		size_resource(1,RLIMIT_PTHREAD)},
#   else
	{_SGI_DMMX_l_threads,		L"threads      ",
	 		"",		L"",		size_resource(1,0)},
#   endif /* RLIMIT_PTHREAD */
	{NULLSTR,	WCNULLSTR,	NULLSTR,	WCNULLSTR,	0}
};
#endif	/* LIM_CPU */

#ifdef cray
    const struct name_fvalue built_ins[] =
#   define VALPTR(x)	x
#else
#   define VALPTR(x)	((wchar_t*)x)
    const struct name_value built_ins[] =
#endif /* cray */
{
		{ L"login",	VALPTR(b_login),	N_BLTIN|BLT_ENV},
		{ L"exec",	VALPTR(b_exec),		N_BLTIN|BLT_ENV|BLT_SPC},
		{ L"set",		VALPTR(b_set),		N_BLTIN|BLT_SPC},
		{ L":",		VALPTR(b_null),		N_BLTIN|BLT_SPC},
		{ L"true",	VALPTR(b_null),		N_BLTIN},
		{ L"builtin_exec",VALPTR(b_null),		N_BLTIN|BLT_EXEC},
#ifdef _bin_newgrp_
		{ L"newgrp",	VALPTR(b_login),	N_BLTIN|BLT_ENV},
#endif	/* _bin_newgrp_ */
		{ L"false",	VALPTR(b_null),		N_BLTIN},
#ifdef apollo
		{ L"rootnode",	VALPTR(b_rootnode),	N_BLTIN},
		{ L"ver",		VALPTR(b_ver),		N_BLTIN},
#endif	/* apollo */
#ifdef LDYNAMIC
		{ L"inlib",	VALPTR(b_inlib),	N_BLTIN},
#   ifndef apollo
		{ L"builtin",	VALPTR(b_builtin),	N_BLTIN},
#   endif	/* !apollo */
#endif	/* LDYNAMIC */
		{ L".",		VALPTR(b_dot),		N_BLTIN|BLT_SPC|BLT_FSUB},
		{ L"readonly",	VALPTR(b_readonly),	N_BLTIN|BLT_SPC|BLT_DCL},
		{ L"typeset",	VALPTR(b_typeset),	N_BLTIN|BLT_DCL},
		{ L"return",	VALPTR(b_ret_exit),	N_BLTIN|BLT_SPC},
		{ L"export",	VALPTR(b_export),	N_BLTIN|BLT_SPC|BLT_DCL},
		{ L"eval",	VALPTR(b_eval),		N_BLTIN|BLT_SPC|BLT_FSUB},
		{ L"fc",		VALPTR(b_fc),		N_BLTIN|BLT_FSUB|BLT_EXEC},
		{ L"shift",	VALPTR(b_shift),	N_BLTIN|BLT_SPC},
		{ L"cd",		VALPTR(b_chdir),	N_BLTIN|BLT_EXEC},
#ifdef OLDTEST
		{ L"[",		VALPTR(b_test),		N_BLTIN},
#endif /* OLDTEST */
		{  L"alias",	VALPTR(b_alias),	N_BLTIN|BLT_SPC|BLT_DCL|BLT_EXEC},
		{ L"break",	VALPTR(b_break),	N_BLTIN|BLT_SPC},
		{ L"continue",	VALPTR(b_continue),	N_BLTIN|BLT_SPC},
#ifdef ECHOPRINT
		{ L"echo",	VALPTR(b_print),	N_BLTIN},
#else
		{ L"echo",	VALPTR(b_echo),		N_BLTIN},
#endif /* ECHOPRINT */
		{ L"exit",	VALPTR(b_ret_exit),	N_BLTIN|BLT_SPC},
#ifdef JOBS
# ifdef SIGTSTP
		{ L"bg",		VALPTR(b_bgfg),		N_BLTIN|BLT_EXEC},
		{ L"fg",		VALPTR(b_bgfg),		N_BLTIN|BLT_EXEC},
# endif	/* SIGTSTP */
		{ L"jobs",	VALPTR(b_jobs),		N_BLTIN|BLT_EXEC},
		{ L"kill",	VALPTR(b_kill),		N_BLTIN|BLT_EXEC},
#endif	/* JOBS */
		{ L"let",		VALPTR(b_let),		N_BLTIN},
		{ L"print",	VALPTR(b_print),	N_BLTIN},
		{ L"pwd",		VALPTR(b_pwd),		N_BLTIN},
		{ L"read",	VALPTR(b_read),		N_BLTIN|BLT_EXEC},
#ifdef SYSCOMPILE
		{ L"shcomp",	VALPTR(b_shcomp),	N_BLTIN},
#endif /* SYSCOMPILE */
#ifdef SYSSLEEP
		{ L"sleep",	VALPTR(b_sleep),	N_BLTIN},
#endif /* SYSSLEEP */
#ifdef OLDTEST
		{ L"test",	VALPTR(b_test),		N_BLTIN},
#endif /* OLDTEST */
		{ L"times",	VALPTR(b_times),	N_BLTIN|BLT_SPC},
		{ L"trap",	VALPTR(b_trap),		N_BLTIN|BLT_SPC},
		{ L"type",	VALPTR(b_type),		N_BLTIN|BLT_EXEC},
		{ L"ulimit",	VALPTR(b_ulimit),	N_BLTIN|BLT_EXEC},
		{ L"limit",       VALPTR(b_limit),        N_BLTIN|BLT_EXEC},
		{ L"unlimit",     VALPTR(b_unlimit),      N_BLTIN|BLT_EXEC},
		{ L"umask",	VALPTR(b_umask),	N_BLTIN|BLT_EXEC},
		{ L"unalias",	VALPTR(b_unalias),	N_BLTIN|BLT_EXEC},
		{ L"unset",	VALPTR(b_unset),	N_BLTIN|BLT_SPC},
		{ L"wait",	VALPTR(b_wait),		N_BLTIN|BLT_EXEC},
		{ L"whence",	VALPTR(b_whence),	N_BLTIN},
		{ L"getopts",	VALPTR(b_getopts),	N_BLTIN|BLT_EXEC},
		{ L"hash",	VALPTR(b_hash),		N_BLTIN|BLT_EXEC},
#ifdef UNIVERSE
		{ L"universe",	VALPTR(b_universe),	N_BLTIN},
#endif /* UNIVERSE */
#ifdef FS_3D
		{ L"vpath",	VALPTR(b_vpath_map),	N_BLTIN},
		{ L"vmap",	VALPTR(b_vpath_map),	N_BLTIN},
#endif /* FS_3D */
		{ L"command",	VALPTR(b_command),	N_BLTIN|BLT_EXEC},
		{ wc_e_nullstr,		0, 0 }
};

SYSTAB	test_optable =
{
		{ L"!=",	TEST_SNE},
		{ L"-a",	TEST_AND},
		{ L"-ef",	TEST_EF},
		{ L"-eq",	TEST_EQ},
		{ L"-ge",	TEST_GE},
		{ L"-gt",	TEST_GT},
		{ L"-le",	TEST_LE},
		{ L"-lt",	TEST_LT},
		{ L"-ne",	TEST_NE},
		{ L"-nt",	TEST_NT},
		{ L"-o",	TEST_OR},
		{ L"-ot",	TEST_OT},
		{ L"=",		TEST_SEQ},
		{ L"==",	TEST_SEQ},
#ifdef NEWTEST
		{ L"<",		TEST_SLT},
		{ L">",		TEST_SGT},
		{ L"]]",	TEST_END},
#endif /* NEWTEST */
		{ wc_e_nullstr,	0}
};

SYSTAB	tab_attributes =
{
		{ L"export",	N_EXPORT},
		{ L"readonly",	N_RDONLY},
		{ L"tagged",	T_FLAG},
#ifdef FLOAT
		{ L"exponential",	(N_DOUBLE|N_INTGER|N_EXPNOTE)},
		{ L"float",	(N_DOUBLE|N_INTGER)},
#endif /* FLOAT */
		{ L"long",	(L_FLAG|N_INTGER)},
		{ L"unsigned",	(N_UNSIGN|N_INTGER)},
		{ L"function",	(N_BLTNOD|N_INTGER)},
		{ L"integer",	N_INTGER},
		{ L"filename",	N_HOST},
		{ L"lowercase",	N_UTOL},
		{ L"zerofill",	N_ZFILL},
		{ L"leftjust",	N_LJUST},
		{ L"rightjust",	N_RJUST},
		{ L"uppercase",	N_LTOU},
		{ wc_e_nullstr,	0}
};


#ifndef IODELAY
#   undef _SELECT5_
#endif /* IODELAY */
#ifdef _sgtty_
#   ifdef _SELECT5_
	const int tty_speeds[] = {0, 50, 75, 110, 134, 150, 200, 300,
			600,1200,1800,2400,9600,19200,0};
#   endif /* _SELECT5_ */
#endif /* _sgtty_ */
