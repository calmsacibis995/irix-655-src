/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:sh/expand.c	1.4.4.2"

/*
 *	File name expansion
 *
 *	David Korn
 *	AT&T Bell Laboratories
 *
 */

#include	"sh_config.h"
#ifdef KSHELL
#   include	"defs.h"
#else
#   include	<sys/stat.h>
#   include	<setjmp.h>
#   ifdef _unistd_
#	include	<unistd.h>
#   endif /* _unistd_ */
#endif /* KSHELL */
/* now for the directory reading routines */
#ifdef FS_3D
#   undef _ndir_
#   define _dirent_ 1
#endif /* FS_3D */
#ifdef _ndir_
#   undef	direct
#   define direct dirent
#   include	<ndir.h>
#else
#   undef	dirent
#   ifndef FS_3D
#	define dirent direct
#   endif /* FS_3D */
#   ifdef _dirent_
#	include	<dirent.h>
#   else
#	include	<sys/dir.h>
#	ifndef rewinddir	/* old system V */
#           define OLDSYS5 1
#	    define NDENTS	32
	    typedef struct 
	    {
		int		fd;
		struct direct	*next;
		struct direct	*last;
		struct direct	entries[NDENTS];
		char		extra;
		ino_t		save;
	    } DIR;
	    DIR *opendir();
	    struct direct *readdir();
#  	    define closedir(dir)	close(dir->fd)
#	endif /* rewinddir */
#   endif /* _dirent_ */
#endif


#ifdef KSHELL
#   define check_signal()	(sh.trapnote&SIGSET)
#    define argbegin	argnxt.cp
    extern char	*strrchr();
    int		path_expand();
    void	rm_files();
    int		f_complete();
    static	wchar_t	*sufstr;
    static	int	suflen;
#else
#   define check_signal()	(0)
#   define round(x,y)		(((int)(x)+(y)-1)&~((y)-1))
#   define sh_access		access
#   define suflen		0
    struct argnod
    {
	struct argnod	*argbegin;
	struct argnod	*argchn;
	char		argval[1];
    };
    static char		*sh_copy();
    static int		test_type();
#endif /* KSHELL */


/*
 * This routine builds a list of files that match a given pathname
 * Uses external routine strmatch() to match each component
 * A leading . must match explicitly
 *
 */

struct glob
{
	int		argn;
	wchar_t		**argv;
	int		flags;
	struct argnod	*rescan;
	struct argnod	*match;	
	DIR		*dirf;
#ifndef KSHELL
	char		*memlast;
	char		*last;
	struct argnod	*resume;
	jmp_buf		jmpbuf;
	char		begin[1];
#endif
};


#define GLOB_RESCAN 1
#define	argstart(ap)	((ap)->argbegin)
#define globptr()	((struct glob*)membase)

static struct glob	 *membase;

static void		addmatch();
static void		glob_dir();
#ifndef KSHELL
    extern int		strmatch();
#endif /* KSHELL */


int path_expand(pattern)
wchar_t *pattern;
{
	register struct argnod *ap;
	register struct glob *gp;
#ifdef KSHELL
	struct glob globdata;
	membase = &globdata;
#endif /* KSHELL */
	gp = globptr();
	ap = (struct argnod*)stakalloc((wcslen(pattern)*WC_SZ)+sizeof(struct argnod)+(suflen*WC_SZ));
	gp->rescan =  ap;
	gp->argn = 0;
#ifdef KSHELL
	gp->match = st.gchain;
#else
	gp->match = 0;
#endif /* KSHELL */
	ap->argbegin = ap->argval;
	ap->argchn = 0;
#ifdef KSHELL
	pattern = sh_copy(pattern,ap->argval);
	if(suflen)
		sh_copy(sufstr,pattern);
#else
	sh_copy(pattern,ap->argval);
#endif /* KSHELL */
	suflen = 0;
	do
	{
		gp->rescan = ap->argchn;
		glob_dir(ap);
	}
	while(ap = gp->rescan);
#ifdef KSHELL
	st.gchain = gp->match;
#endif /* KSHELL */
	return(gp->argn);
}

static void glob_dir(ap)
struct argnod *ap;
{
	register wchar_t	*rescan;
	register wchar_t	*prefix;
	register wchar_t	*pat;
	DIR 		*dirf;
	char		quote = 0;
	char		savequote = 0;
	char		meta = 0;
	char		bracket = 0;
	char		first;
	wchar_t		*dirname;
	char		mb_dirname[PATH_MAX];
	wchar_t		wc_dirname[PATH_MAX];
	wchar_t		*last;
	struct dirent	*dirp;
#ifdef LSTAT
	struct stat	statb;
#endif /* LSTAT */
	if(check_signal())
		return;
	pat = rescan = argstart(ap);
	prefix = dirname = ap->argval;
	first = (rescan == prefix);
	/* check for special chars */
	while(1) switch(*rescan++)
	{
		case 0L:
			last = 0L;
			if(*rescan==0L)
				last = rescan-1;
			rescan = 0L;
			if(meta)
				goto process;
			if(first)
				return;
			if(quote)
				sh_trim(argstart(ap));
			/* treat trailing / as trailing /. */
			if(last && last[-1]==L'/')
				*last = L'.';
#ifdef LSTAT
			if(ksh_lstat(prefix,&statb)>=0)
#else
			if(sh_access(prefix,F_OK)==0)
#endif /* LSTAT */
				addmatch((wchar_t*)0,prefix,(wint_t*)0,last);
			return;

		case L'/':
			if(meta)
				goto process;
			pat = rescan;
			bracket = 0;
			savequote = quote;
			break;

		case L'[':
			bracket = 1;
			break;

		case L']':
			meta |= bracket;
			break;

		case L'*':
		case L'?':
		case L'(':
			meta=1;
			break;

		case L'\\':
			quote = 1;
			rescan++;
	}
process:
	if(pat == prefix)
	{
		dirname = L".";
		prefix = 0L;
	}
	else
	{
		if(pat==prefix+1)
			dirname = L"/";
		*(pat-1) = 0L;
		if(savequote)
			sh_trim(argstart(ap));
	}
	if(MB_CUR_MAX == 1)
	{
		int i,len;
		len = wcslen(dirname);
		for(i=0;i<len;++i)
			mb_dirname[i] = dirname[i];
		mb_dirname[i] = 0;
	}
	else	wcstombs(mb_dirname,dirname,PATH_MAX);
	if(dirf=opendir(mb_dirname))
	{
		/* check for rescan */
		if(rescan)
			*(rescan-1) = 0L;
		while(dirp = readdir(dirf))
		{
			int cnt;
			if(dirp->d_ino==0 || (*dirp->d_name==L'.' && *pat!=L'.'))
				continue;
			if(MB_CUR_MAX == 1)
			{
				int i;
				cnt = strlen(dirp->d_name);
				for(i=0;i<cnt;++i)
					wc_dirname[i] = (unsigned char)dirp->d_name[i];
			}
			else	cnt = mbstowcs(wc_dirname,dirp->d_name,PATH_MAX);
			wc_dirname[cnt] = 0L;
			if(strmatch(wc_dirname, pat))
				addmatch(prefix,wc_dirname,rescan,(wchar_t*)0);
		}
		closedir(dirf);
	}
	return;
}

static  void addmatch(dir,pat,rescan,endslash)
wchar_t *dir, *pat, *endslash;
register wchar_t *rescan;
{
	register struct argnod *ap = (struct argnod*)stakseek(ARGVAL);
	register struct glob *gp = globptr();
	if(dir)
	{
		stakputs(dir);
		stakputc(L'/');
	}
	if(endslash)
		*endslash = 0L;
	stakputs(pat);
	if(rescan)
	{
		int offset;
		ap = (struct argnod*)stakptr(0);
		if(test_type(ap->argval,S_IFMT,S_IFDIR)==0)
			return;
		stakputc(L'/');
		offset = staktell();
		/* if null, reserve room for . */
		if(*rescan)
			stakputs(rescan);
		else
			stakputc(0L);
		stakputc(0L);
		rescan = (wchar_t *)stakptr(offset);
		ap = (struct argnod*)stakfreeze(0);
		ap->argbegin = rescan;
		ap->argchn = gp->rescan;
		gp->rescan = ap;
	}
	else
	{
#ifdef KSHELL
		if(!endslash && is_option(MARKDIR) && test_type(ap->argval,S_IFMT,S_IFDIR))
			stakputc(L'/');
#endif /* KSHELL */
		ap = (struct argnod*)wc_stakfreeze(WC_SZ);
		ap->argchn = gp->match;
		gp->match = ap;
		gp->argn++;
	}
#ifdef KSHELL
	ap->argflag = A_RAW;
#endif /* KSHELL */
}


#ifdef KSHELL

/*
 * remove tmp files
 * template of the form /tmp/sh$$.???
 */

void	rm_files(template)
register wchar_t *template;
{
	register wchar_t *cp;
	struct argnod  *schain;
	cp = wcsrchr(template,L'.');
	*(cp+1) = 0L;
	f_complete(template,L"*");
	schain = st.gchain;
	while(schain)
	{
		ksh_unlink(schain->argval);
		schain = schain->argchn;
	}
}

/*
 * file name completion
 * generate the list of files found by adding an suffix to end of name
 * The number of matches is returned
 */

f_complete(name,suffix)
wchar_t *name;
register wchar_t *suffix;
{
	st.gchain =  0;
	sufstr = suffix;
	suflen = wcslen(suffix);
	return(path_expand(name));
}

#else

static char *sh_copy(sp,dp)
register char *sp;
register char *dp;
{
	register char *memlast = globptr()->memlast;
	while(dp < memlast)
	{
		if((*dp = *sp++)==0L)
			return(dp);
		dp++;
	}
	LONGJMP(globptr()->jmpbuf);
}

/*
 * Return true if the mode bits of file <f> corresponding to <mask> have
 * the value equal to <field>.  If <f> is null, then the previous stat
 * buffer is used.
 */

static test_type(f,mask,field)
wchar_t *f;
int field;
{
	static struct stat statb;
	if(f && ksh_stat(f,&statb)<0)
		return(0);
	return((statb.st_mode&mask)==field);
}

/*
 * remove backslashes
 */

static void sh_trim(sp)
register wchar_t *sp;
{
	register wchar_t *dp = sp;
	register wint_t c;
	while(1)
	{
		if((c= *sp++) == L'\\')
			c = *sp++;
		*dp++ = c;
		if(c==0L)
			break;
	}
}
#endif /* KSHELL */

#ifdef OLDSYS5

static DIR dirbuff;

DIR *opendir(name)
char *name;
{
	register int fd;
	struct stat statb;
	if((fd = open(name,0)) < 0)
		return(0);
	if(fstat(fd,&statb) < 0 || !S_ISDIR(statb.st_mode))
	{
		close(fd);
		return(0);
	}
	dirbuff.fd = fd;
	dirbuff.next = dirbuff.last = dirbuff.entries + NDENTS;
	return(&dirbuff);
}

struct direct *readdir(dir)
register DIR *dir;
{
	register int n;
	struct direct *dp;
	if(dir->next >= dir->last)
	{
		n = read(dir->fd,(char*)dir->entries,NDENTS*sizeof(struct direct));
		n /= sizeof(struct direct);
		if(n <=0)
			return(0);
		dir->next = dir->entries;
		dir->last = dir->entries + n;
	}
	else
		dir->next->d_ino =  dir->save;
	dp = (struct direct*)dir->next++;
	dir->save = dir->next->d_ino;
	dir->next->d_ino = 0;
	return(dp);
}
#endif /* OLDSYS5 */

#ifdef BRACEPAT
int expbrace(todo)
struct argnod *todo;
/*@
	assume todo!=0;
	return count satisfying count>=1;
@*/
{
	register wchar_t *cp;
	register int brace;
	register struct argnod *ap;
	struct argnod *top = 0;
	struct argnod *apin;
	wchar_t *pat, *rescan, *bracep;
	wchar_t *sp;
	char comma;
	int count = 0;
	todo->argchn = 0;
again:
	apin = ap = todo;
	todo = ap->argchn;
	cp = ap->argval;
	comma = brace = 0;
	/* first search for {...,...} */
	while(1) switch(*cp++)
	{
		case L'{':
			if(brace++==0)
				pat = cp;
			break;
		case L'}':
			if(--brace>0)
				break;
			if(brace==0 && comma)
				goto endloop1;
			comma = brace = 0;
			break;
		case L',':
			if(brace==1)
				comma = 1;
			break;
		case L'\\':
			cp++;
			break;
		case 0L:
			/* insert on stack */
			ap->argchn = top;
			top = ap;
			if(todo)
				goto again;
			for(; ap; ap=apin)
			{
				apin = ap->argchn;
				if((brace = path_expand(ap->argval)))
					count += brace;
				else
				{
					ap->argchn = st.gchain;
					st.gchain = ap;
					count++;
				}
				st.gchain->argflag |= A_MAKE;
			}
			return(count);
	}
endloop1:
	rescan = cp;
	bracep = cp = pat-1;
	*cp = 0L;
	while(1)
	{
		brace = 0;
		/* generate each pattern and put on the todo list */
		while(1) switch(*++cp)
		{
			case L'\\':
				cp++;
				break;
			case L'{':
				brace++;
				break;
			case L',':
				if(brace==0)
					goto endloop2;
				break;
			case L'}':
				if(--brace<0)
					goto endloop2;
		}
	endloop2:
		/* check for match of '{' */
		brace = *cp;
		*cp = 0L;
		if(brace == L'}')
		{
			apin->argchn = todo;
			todo = apin;
			sp = sh_copy(pat,bracep);
			sp = sh_copy(rescan,sp);
			break;
		}
		ap = (struct argnod*)stakseek(ARGVAL);
		ap->argflag = 0;
		ap->argchn = todo;
		stakputs(apin->argval);
		stakputs(pat);
		stakputs(rescan);
		todo = ap = (struct argnod*)wc_stakfreeze(WC_SZ);
		pat = cp+1;
	}
	goto again;
}
#endif /* BRACEPAT */
