/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:sh/test.c	1.4.4.1"

/*
 * test expression
 * [ expression ]
 * Rewritten by David Korn
 */

#include	"defs.h"
#include	"test.h"
#ifdef OLDTEST
#   include	"sym.h"
#endif /* OLDTEST */

#define	tio(a,f)	(sh_access(a,f)==0)
static time_t ftime_compare();
static int test_stat();
static struct stat statb;
int	test_binop();
int	unop_test();


#ifdef OLDTEST
/* single char string compare */
#define c_eq(a,c)	(*a==c && *(a+1)==0L)
/* two character string compare */
#define c2_eq(a,c1,c2)	(*a==c1 && *(a+1)==c2 && *(a+2)==0L)

int b_test();

static wchar_t *nxtarg();
static int exp();
static int e3();

static int ap, ac;
static wchar_t **av;

int b_test(argn, com)
wchar_t *com[];
register int argn;
{
	register wchar_t *p = com[0];
	av = com;
	ap = 1;
	if(c_eq(p,L'['))
	{
		p = com[--argn];
		if(!c_eq(p, L']'))
			sh_fail(wc_e_test, ksh_gettxt(_SGI_DMMX_e_bracket,e_bracket),ERROR);
	}
	if(argn <= 1)
		return(1);
	ac = argn;
	return(!exp(0));
}

/*
 * evaluate a test expression.
 * flag is 0 on outer level
 * flag is 1 when in parenthesis
 * flag is 2 when evaluating -a 
 */

static exp(flag)
{
	register int r;
	register wchar_t *p;
	r = e3();
	while(ap < ac)
	{
		p = nxtarg(0);
		/* check for -o and -a */
		if(flag && c_eq(p,L')'))
		{
			ap--;
			break;
		}
		if(*p==L'-' && *(p+2)==0L)
		{
			if(*++p == L'o')
			{
				if(flag==2)
				{
					ap--;
					break;
				}
				r |= exp(3);
				continue;
			}
			else if(*p == L'a')
			{
				r &= exp(2);
				continue;
			}
		}
		if(flag==0)
			break;
		sh_fail(wc_e_test, ksh_gettxt(_SGI_DMMX_e_synbad,e_synbad),ERROR);
	}
	return(r);
}

static wchar_t *nxtarg(mt)
{
	if(ap >= ac)
	{
		if(mt)
		{
			ap++;
			return(0);
		}
		sh_fail(wc_e_test, ksh_gettxt(_SGI_DMMX_e_argexp,e_argexp),ERROR);
	}
	return(av[ap++]);
}


static e3()
{
	register wchar_t *a;
	register wchar_t *p2;
	register int p1;
	wchar_t *op;
	a=nxtarg(0);
	if(c_eq(a, L'!'))
		return(!e3());
	if(c_eq(a, L'('))
	{
		p1 = exp(1);
		p2 = nxtarg(0);
		if(!c_eq(p2, L')'))
			sh_fail(wc_e_test, ksh_gettxt(_SGI_DMMX_e_paren,e_paren),ERROR);
		return(p1);
	}
	p2 = nxtarg(1);
	if(p2!=0L && (c_eq(p2,L'=') || c2_eq(p2,L'!',L'=')))
		goto skip;
	if(c2_eq(a,L'-',L't'))
	{
		if(p2 && iswdigit(*p2))
			 return(*(p2+1)?0:tty_check(*p2-L'0'));
		else
		{
		/* test -t with no arguments */
			ap--;
			return(tty_check(1));
		}
	}
	if((*a==L'-' && *(a+2)==0L))
	{
		if(!p2)
		{
			/* for backward compatibility with new flags */
			if(!wcschr(wc_test_unops+9,a[1]))
				return(1);
			sh_fail(wc_e_test, ksh_gettxt(_SGI_DMMX_e_argexp,e_argexp),ERROR);
		}
		if(wcschr(wc_test_unops,a[1]))
			return(unop_test(a[1],p2));
	}
	if(!p2)
	{
		ap--;
		return(*a!=0L);
	}
skip:
	p1 = sh_lookup(p2,test_optable);
	op = p2;
	if((p1&TEST_BINOP)==0)
		p2 = nxtarg(0);
	if(p1==0)
		sh_fail(op,ksh_gettxt(_SGI_DMMX_e_testop,e_testop),ERROR);
	return(test_binop(p1,a,p2));
}
#endif /* OLDTEST */

unop_test(op,arg)
register int op;
register wchar_t *arg;
{
	switch(op)
	{
	case L'r':
		return(tio(arg, R_OK));
	case L'w':
		return(tio(arg, W_OK));
	case L'x':
		return(tio(arg, X_OK));
	case L'e':
		return(!ksh_stat(arg,&statb));
	case L'd':
		return(test_type(arg,S_IFMT,S_IFDIR));
	case L'c':
		return(test_type(arg,S_IFMT,S_IFCHR));
	case L'b':
		return(test_type(arg,S_IFMT,S_IFBLK));
	case L'f':
		return(test_type(arg,S_IFMT,S_IFREG));
	case L'u':
		return(test_type(arg,S_ISUID,S_ISUID));
	case L'g':
		return(test_type(arg,S_ISGID,S_ISGID));
	case L'k':
#ifdef S_ISVTX
		return(test_type(arg,S_ISVTX,S_ISVTX));
#else
		return(0);
#endif /* S_ISVTX */
	case L'V':
#ifdef FS_3D
	{
		struct stat statb;
		if(*arg==0L || ksh_lstat(arg,&statb)<0)
			return(0);
		return((statb.st_mode&(S_IFMT|S_ISVTX|S_ISUID))==(S_IFDIR|S_ISVTX|S_ISUID));
	}
#else
		return(0);
#endif /* FS_3D */
	case L'L':
	case L'l':
	/* -h is not documented, and hopefully will disappear */
	case L'h':
#ifdef LSTAT
	{
		struct stat statb;
		if(*arg==0L || ksh_lstat(arg,&statb)<0)
			return(0);
		return((statb.st_mode&S_IFMT)==S_IFLNK);
	}
#else
		return(0);
#endif	/* S_IFLNK */

	case L'C':
#ifdef S_IFCTG
		return(test_type(arg,S_IFMT,S_IFCTG));
#else
		return(0);
#endif	/* S_IFCTG */

	case L'S':
#ifdef S_IFSOCK
		return(test_type(arg,S_IFMT,S_IFSOCK));
#else
		return(0);
#endif	/* S_IFSOCK */

	case L'p':
#ifdef S_IFIFO
		return(test_type(arg,S_IFMT,S_IFIFO));
#else
		return(0);
#endif	/* S_IFIFO */
	case L'n':
		return(*arg != 0);
	case L'z':
		return(*arg == 0);
	case L's':
	case L'O':
	case L'G':
	{
		struct stat statb;
		if(*arg==0L || test_stat(arg,&statb)<0)
			return(0);
		if(op==L's')
			return(statb.st_size>0);
		else if(op==L'O')
			return(statb.st_uid==sh.userid);
		return(statb.st_gid==sh.groupid);
	}
#ifdef NEWTEST
	case L'a':
		return(tio(arg, F_OK));
	case L'o':
		op = sh_lookup(arg,tab_options);
		return(op && is_option((1L<<op))!=0);

	case L't':
		if(iswdigit(*arg) && arg[1]==0L)
			 return(tty_check(*arg-L'0'));
		return(0);
#endif /* NEWTEST */
	default:
#ifdef OLDTEST
	{
		static wchar_t a[3] = L"-?";
		a[1]= op;
		sh_fail(a,ksh_gettxt(_SGI_DMMX_e_testop,e_testop),ERROR);
		/* NOTREACHED */
	}
#else
		return(0);
#endif /* OLDTEST */
	}
	/* NOTREACHED */
}

test_binop(op,left,right)
wchar_t *left, *right;
register int op;
{
	register int int1,int2;
	if(op&TEST_ARITH)
	{
		int1 = sh_arith(left);
		int2 = sh_arith(right);
	}
	switch(op)
	{
		/* op must be one of the following values */
#ifdef OLDTEST
		case TEST_AND:
		case TEST_OR:
			ap--;
			return(*left!=0L);
#endif /* OLDTEST */
#ifdef NEWTEST
		case TEST_PEQ:
			return(strmatch(left, right));
		case TEST_PNE:
			return(!strmatch(left, right));
		case TEST_SGT:
			return(wcscmp(left, right)>0);
		case TEST_SLT:
			return(wcscmp(left, right)<0);
#endif /* NEWTEST */
		case TEST_SEQ:
			return(wcscmp(left, right)==0);
		case TEST_SNE:
			return(wcscmp(left, right)!=0);
		case TEST_EF:
			return(test_inode(left,right));
		case TEST_NT:
			return(ftime_compare(left,right)>0);
		case TEST_OT:
			return(ftime_compare(left,right)<0);
		case TEST_EQ:
			return(int1==int2);
		case TEST_NE:
			return(int1!=int2);
		case TEST_GT:
			return(int1>int2);
		case TEST_LT:
			return(int1<int2);
		case TEST_GE:
			return(int1>=int2);
		case TEST_LE:
			return(int1<=int2);
	}
	/* NOTREACHED */
}

/*
 * returns the modification time of f1 - modification time of f2
 */

static time_t ftime_compare(file1,file2)
wchar_t *file1,*file2;
{
	struct stat statb1,statb2;
	if(test_stat(file1,&statb1)<0)
		statb1.st_mtime = 0;
	if(test_stat(file2,&statb2)<0)
		statb2.st_mtime = 0;
	return(statb1.st_mtime-statb2.st_mtime);
}

/*
 * return true if inode of two files are the same
 */

test_inode(file1,file2)
wchar_t *file1,*file2;
{
	struct stat stat1,stat2;
	if(test_stat(file1,&stat1)>=0  && test_stat(file2,&stat2)>=0)
		if(stat1.st_dev == stat2.st_dev && stat1.st_ino == stat2.st_ino)
			return(1);
	return(0);
}


/*
 * This version of access checks against effective uid/gid
 * The static buffer statb is shared with test_type.
 */

sh_access(name, mode)
register wchar_t	*name;
register int mode;
{
	if(*name==0L)
		return(-1);
	if(strmatch(name,(wchar_t*)wc_e_devfdNN))
		return(io_access(wcstol(name+8,(wchar_t **)0,10),mode));
	/* can't use access function for execute permission with root */
	if(mode==X_OK && sh.euserid==0)
		goto skip;
	if(sh.userid==sh.euserid && sh.groupid==sh.egroupid)
		return(ksh_access(name,mode));
#ifdef SETREUID
	/* swap the real uid to effective, check access then restore */
	/* first swap real and effective gid, if different */
	if(sh.groupid==sh.euserid || setregid(sh.egroupid,sh.groupid)==0) 
	{
		/* next swap real and effective uid, if needed */
		if(sh.userid==sh.euserid || setreuid(sh.euserid,sh.userid)==0)
		{
			mode = ksh_access(name,mode);
			/* restore ids */
			if(sh.userid!=sh.euserid)
				setreuid(sh.userid,sh.euserid);
			if(sh.groupid!=sh.egroupid)
				setregid(sh.groupid,sh.egroupid);
			return(mode);
		}
		else if(sh.groupid!=sh.egroupid)
			setregid(sh.groupid,sh.egroupid);
	}
#endif /* SETREUID */
skip:
	if(test_stat(name, &statb) == 0)
	{
		if(mode == F_OK)
			return(mode);
		else if(sh.euserid == 0)
		{
			if(!S_ISREG(statb.st_mode) || mode!=X_OK)
				return(0);
		    	/* root needs execute permission for someone */
			mode = (S_IEXEC|(S_IEXEC>>3)|(S_IEXEC>>6));
		}
		else if(sh.euserid == statb.st_uid)
			mode <<= 6;
		else if(sh.egroupid == statb.st_gid)
			mode <<= 3;
#ifdef MULTIGROUPS
		/* on some systems you can be in several groups */
		else
		{
#   if MUTLIGROUPS>0	/* pre-posix systems */
			register int n = MULTIGROUPS;
			int groups[MULTIGROUPS];
#   else
			gid_t *groups; 
			register int n = getgroups(0,(gid_t*)0);
			groups = (gid_t*)stakalloc(n*sizeof(gid_t));
#   endif /* MUTLIGROUPS>0 */
			n = getgroups(n,groups);
			while(--n >= 0)
			{
				if(groups[n] == statb.st_gid)
				{
					mode <<= 3;
					break;
				}
			}
		}
#   endif /* MULTIGROUPS */
		if(statb.st_mode & mode)
			return(0);
	}
	return(-1);
}


/*
 * Return true if the mode bits of file <f> corresponding to <mask> have
 * the value equal to <field>.  If <f> is null, then the previous stat
 * buffer is used.
 */

test_type(f,mask,field)
register wchar_t *f;
int field;
{
	if(f && (*f==0L || test_stat(f,&statb)<0))
		return(0);
	return((statb.st_mode&mask)==field);
}

/*
 * do an fstat() for /dev/fd/n, otherwise stat()
 */

static int test_stat(f,buff)
wchar_t *f;
struct stat *buff;
{
	if(strmatch(f,(wchar_t*)wc_e_devfdNN))
		return(fstat(wcstol(f+8,(wchar_t **)0,10),buff));
	else
		return(ksh_stat(f,buff));
}
