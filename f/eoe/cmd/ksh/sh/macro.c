/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:sh/macro.c	1.5.4.1"

/*
 * UNIX shell
 *
 * S. R. Bourne
 * AT&T Bell Laboratories
 * Rewritten by David Korn
 *
 */

#include	"defs.h"
#include	"sym.h"
#include	"builtins.h"
#ifdef MULTIBYTE
#   include	"national.h"
#endif /* MULTIBYTE */


/* These routines are defined by this module */
wchar_t	*mac_expand(wchar_t *);
wchar_t	*mac_try(wchar_t *);
wchar_t	*mac_trim(wchar_t *, int);
int	mac_here(struct ionod *);
void	mac_checkvoid();

/* These external routines are referenced by this module */
extern wchar_t		*ltos();
extern void		match_paren();
extern void             match_paren_comsubst();
extern wchar_t		*submatch();

#ifdef MULTIBYTE
    static int	charlen();
#endif /* MULTIBYTE */
static void	copyto();
static int	substring();
static void	skipto();
static int	getch();
static int	comsubst();
static void	mac_error();
static void	mac_copy();
#ifdef POSIX
    static void	tilde_expand();
#endif /* POSIX */

static char	quote;	/* used locally */
static char	quoted;	/* used locally */
static char	mflag;	/* 0 for $x, 1 for here docs */
static const wchar_t *ifs;
static int	w_fd = -1;
static int mactry;
static wchar_t *mac_current;
static jmp_buf mac_buf;
static wchar_t	idb[2];
#ifdef FLOAT
    extern char		*etos(),*ftos();
    static double numb;
#else
    static long numb;
#endif /* FLOAT */

static void copyto(endch,newquote)
register wchar_t	endch;
{
	register wint_t	c;
	register int count = 1;
	int saveq = quote;
#ifdef POSIX
	register wint_t tilde = -WC_SZ;
#endif /* POSIX */

	quote = newquote;
#ifdef POSIX
	/* check for tilde expansion */
	c = io_readc();
	if(c==L'~' && !mflag && !quote)
		tilde = staktell();
	io_unreadc(c);
#endif /* POSIX */
	while(c=getch(endch))
	{
		if((c==endch) && (saveq || !quote) && --count<=0)
			break;
		if(quote || c==ESCAPE)
		{
			if(c==ESCAPE)
			{
				c = io_readc();
				if(quote && !escchar(c) && c!= L'"')
				{
					stakputc(ESCAPE);
					stakputc(ESCAPE);
				}
			}
			if(!mflag || !escchar(c))
				stakputc(ESCAPE);
		}
		stakputc(c);
		if(c==L'[' && endch==L']')
			count++;
#ifdef POSIX
		else if(c==L'/' && tilde>=0)
		{
			tilde_expand(tilde,c);
			tilde = -WC_SZ;
		}
#endif /* POSIX */
	}
#ifdef POSIX
	if(tilde>=0)
		tilde_expand(tilde,0);
#endif /* POSIX */
	quote = saveq;
	if(c!=endch)
		mac_error();
}

#ifdef POSIX
/*
 * <offset> is byte offset for beginning of tilde string
 * if <c> is non-zero, append <c> to expansion
 */

static void tilde_expand(offset,c)
register offset;
{
	extern wchar_t *sh_tilde();
	register wchar_t *cp;
	int curoff = staktell();
	stakputc(0);
	if(cp = sh_tilde((wchar_t *)stakptr(offset)))
	{
		stakseek(offset);
		mac_copy(cp,-1);
		if(c)
			stakputc(c);
	}
	else
		stakseek(curoff);
}
#endif /* POSIX */

/* skip chars up to } */

static void skipto(endch)
register wchar_t endch;
{
	register wchar_t	c;
	while((c=io_readc()) && c!=endch)
	{
		switch(c)
		{
			case ESCAPE:
				io_readc();
				break;

			case SQUOTE:	case DQUOTE:
				skipto(c);
				break;

			case DOLLAR:
				if((c=io_readc()) == LBRACE)
					skipto(RBRACE);
				else if(!dolchar(c))
					io_unreadc(c);
		}
	}
	if(c!=endch)
		mac_error();
}

static int getch(endch)
wint_t	endch;
{
	register wint_t	c;
	register wint_t	bra; /* {...} bra =1, {#...} bra=2 */
	int atflag;  /* set if $@ or ${array[@]} within double quotes */
retry:
	c = io_readc();
	if(c==DOLLAR)
	{
		register wchar_t *v;
		register wchar_t *argp;
		register struct namnod	*n=(struct namnod*)NULL;
		int 	dolg=0;
		int dolmax = st.dolc+1;
		int 	nulflg;
		wchar_t *id=idb;
		int offset;
		int	vsize = -1;
		bra = 0;
		*id = 0L;
	retry1:
		c = io_readc();
		switch(c)
		{
			case DOLLAR:
				v=sh_itos(sh.pid);
				break;

			case L'!':
				if(sh.bckpid)
				{
					v=sh_itos(sh.bckpid);
				}
				else
					v = L"";
				break;

			case LBRACE:
				if(bra++ ==0)
					goto retry1;

			case LPAREN:
				if(bra==0 && mactry==0)
				{
					if(!xpg_compliant())
						goto nosub;

					if(comsubst(1))
						goto retry;
#ifdef FLOAT
					if((long)numb==numb)
						v = ltos((long)numb,10);
					else
					{
						double abnumb = numb;
						wchar_t *cp;
						if(abnumb < 0)
							abnumb = -abnumb;
						if(abnumb <1e10 && abnumb>1e-10)
						{
							v = ftos(numb,12);
							cp = v + wcslen(v);
							/* eliminate trailing zeros */
							while(cp>v && *--cp=='0')
								*cp = 0L;
						}
						else
							v = etos(numb,12);
					}
#else
					v = ltos(numb,10);
#endif /* FLOAT */
				}
				else
					goto nosub;
				break;

			case RBRACE:
				if(bra!=2)
					goto nosub;
				bra = 0;
			case L'#':
				if(bra ==1)
				{
					bra++;
					goto retry1;
				}
				v=sh_itos(st.dolc);
				break;

			case L'?':
				v=sh_itos(sh.savexit&EXITMASK);
				break;

			case L'-':
				v=arg_dolminus();
				break;
			
			default:
				if(is_walpha(c))
				{
					offset = staktell();
					while(is_walnum(c))
					{
						stakputc(c);
						c = io_readc();
					}
					while(c==L'[' && bra)
					{
						stakputc(L'[');
						copyto(L']',0);
						*id = *((wchar_t *)stakptr(staktell()-WC_SZ));
						stakputc(L']');
						c = io_readc();
					}
					io_unreadc(c);
					stakputc(0L);
					n=env_namset((wchar_t *)stakptr(offset),sh.var_tree,P_FLAG);
					stakseek(offset);
					v = nam_strval(n);
					c = (bra==2 && ((c= *id), astchar(c)));
					if(nam_istype(n,N_ARRAY))
					{
						if(c || (array_next(n) && v))
							dolg = -1;
						else
							dolg = 0;
					}
					else
					{
						if(c)
							dolmax = 0;
						id = n->namid;
					}
					goto cont1;
				}
				*id = c;
				if(astchar(c))
				{
					dolg=1;
					c=1;
				}
				else if(iswdigit(c))
				{
					c -= L'0';
					if(bra)
					{
						int d;
						while((d=io_readc(),iswdigit(d)))
							c = 10*c + (d-L'0');
						io_unreadc(d);
					}
				}
				else
					goto nosub;
				if(c==0L)
				{
					if((st.states&PROFILE) && !(st.states&FUNCTION))
						v = sh.shname;
					else
						v = st.cmdadr;
				}
				else if(c <= st.dolc)
					v = st.dolv[c];
				else
					dolg = 0, v = 0L;
			}
	cont1:
		c = io_readc();
		if(bra==2)
		{
			if(c!=RBRACE)
				mac_error();
			if(dolg==0 && dolmax)
#ifdef MULTIBYTE
				c = (v?charlen(v):0);
#else
				c = (v?wcslen(v):0);
#endif /* MULTIBYTE */
			else if(dolg>0)
				c = st.dolc;
			else if(dolg<0)
				c = array_elem(n);
			else
				c = (v!=0L);
			v = sh_itos(c);
			dolg = 0;
			c = RBRACE;
		}
		/* check for quotes @ */
		if(idb[0]==L'@' && quote && !atflag)
		{
			quoted--;
			atflag = 1;
		}
		if(c==L':' && bra)	/* null and unset fix */
		{
			nulflg=1;
			c=io_readc();
		}
		else
			nulflg=0;
		if(!defchar(c) && bra)
			mac_error();
		argp = 0L;
		if(bra)
		{
			if(c!=RBRACE)
			{
				offset = staktell();
				if(((v==0L || (nulflg && *v==0L)) ^ (setchar(c)!=0))
					|| is_option(NOEXEC))
				{
					int newquote = quote;
					if(c==L'#' || c == L'%')
						newquote = 0;
					copyto(RBRACE,newquote);
					/* add null byte */
					stakputc(0L);
					stakseek(staktell()-WC_SZ);
				}
				else
					skipto(RBRACE);
				argp=(wchar_t *)stakptr(offset);
			}
		}
		else
		{
			io_unreadc(c);
			c=0L;
		}
		/* check for substring operations */
		if(c == L'#' || c == L'%')
		{
			if(dolg != 0)
				mac_error();
			if(v && *v)
			{
#ifdef POSIX
				wchar_t *pat;
				int savec;
				do
				{
					bra = 0;
					if(*argp==c)
					{
						bra++;
						argp++;
					}
					pat = argp;
					while(1)
					{
						switch(*argp)
						{
						case L'#': case L'%':
						case 0L:
							goto endloop;

						case ESCAPE:
							argp++;
						default:
							argp++;
						}
					}
				endloop:
					savec = *argp;
					*argp++ = 0L;
					if(c==L'#')
					{
						wchar_t *savev = v;
						v = submatch(v,pat,bra);
						if(v==0L)
							v = savev;
					}
					else
						vsize = substring(v,pat,bra);
					c = savec;
				}
				while(c);
#else
				bra = 0;
				if(*argp==c)
				{
					bra++;
					argp++;
				}
				if(c==L'#')
				{
					wchar_t *savev = v;
					v = submatch(v,argp,bra);
					if(v==0L)
						v = savev;
				}
				else
					vsize = substring(v,argp,bra);
#endif /* POSIX */
			}
			if(v)
				stakseek(offset);
		}
	retry2:
		if(v && (!nulflg || *v ) && c!=L'+')
		{
			wint_t type = *id;
#define sep bra
			if(*ifs)
				sep = *ifs;
			else
				sep = SP;
			while(1)
			{
				/* quoted null strings have to be marked */
				if(*v==0L && quote)
				{
					stakputc(ESCAPE);
					stakputc(0L);
				}
				mac_copy(v,vsize);
				if(dolg==0)
					 break;
				if(dolg>0)
				{
					if(++dolg >= dolmax)
						break;
					v = st.dolv[dolg];
				}
				else
				{
					if(type == 0)
						break;
					v = nam_strval(n);
					type = array_next(n);
				}
				if(quote && *id==L'*')
				{
					if(*ifs==0L)
						continue;
					stakputc(ESCAPE);
				}
				stakputc(sep);
#undef sep
			}
		}
		else if(argp)
		{
			if(c==L'?' && !is_option(NOEXEC))
			{
				sh_trim(argp);
				if(!(st.states&INTFLG))
					st.states &= ~FUNCTION;
				sh_fail(id,*argp?argp:ksh_gettxt(_SGI_DMMX_e_nullset,e_nullset),ERROR);
			}
			else if(c==L'=')
			{
				if(n)
				{
					sh_trim(argp);
					nam_putval(n,argp);
					v = nam_strval(n);
					nulflg = 0;
					stakseek(offset);
					goto retry2;
				}
				else
					mac_error();
			}
		}
		else if(is_option(NOSET))
			sh_fail(id,ksh_gettxt(_SGI_DMMX_e_notset,e_notset),ERROR);
		goto retry;
	}
	else if(c==endch)
		return(c);
	else if(c==SQUOTE && mactry==0)
	{
		comsubst(0);
		goto retry;
	}
	else if(c==DQUOTE && !mflag)
	{
		if(quote ==0)
		{
			atflag = 0;
			quoted++;
		}
		quote ^= 1;
		goto retry;
	}
	return(c);
nosub:
	if(bra)
		mac_error();
	io_unreadc(c);
	return(DOLLAR);
}

	/* Strip "" and do $ substitution
	 * Leaves result on top of stack
	 */
wchar_t *mac_expand(as)
wchar_t *as;
{
	register int	savqu =quoted;
	register int	savq = quote;
	register wint_t	savpeekn = st.peekn;
	struct fileblk	cb;
	mac_current = as;
	st.peekn = 0L;
	io_push(&cb);
	io_sopen(as);
	stakseek(0);
	mflag = 0;
	quote=0;
	quoted=0;
	if(!(ifs = nam_fstrval(IFSNOD)))
		ifs = wc_e_sptbnl;
	copyto(0L,0);
	io_pop(1);
	st.peekn = savpeekn;
	if(quoted && staktell()==0)
	{
		stakputc(ESCAPE);
		stakputc(0L);
	}
	/* above is the fix for *'.c' bug	*/
	quote=savq;
	quoted=savqu;
	return((wchar_t *)wc_stakfreeze(WC_SZ));
}

/*
 * command substitution
 * type==0 for ``
 * type==1 for $()
*/

static int comsubst(type)
int type;
{
	struct fileblk	cb;
	register int	fd,dd;
	register wint_t	d;
	register union anynode *t;
	register wchar_t *argc;
	struct ionod *saviotemp = st.iotemp;
	struct slnod *saveslp = st.staklist;
	int savem = mflag;
	int savtop = staktell();
	char *savptr = stakfreeze(0);
	wchar_t inbuff[IOBSIZE+1];
	int saveflags = (st.states&FIXFLG);
	int stacksize = 0;
	register int waitflag = 0;

	if(is_option(WORDEXP) && is_option(NOCMDST))
		sh_exit(CMDBAD);
	if(type)
	{
		type = ((d=io_readc())==LPAREN);
		if(type || d==RPAREN)
			stakputc(d);
		else
			io_unreadc(d);
		if(d != RPAREN) {
			if(type) {
				match_paren(LPAREN,RPAREN);
			}
			else	match_paren_comsubst(LPAREN,RPAREN,0);
		}
		if(type && (d=io_readc())==RPAREN)
		{
			stakseek(staktell()-WC_SZ);
			argc = (wchar_t *)wc_stakfreeze(WC_SZ);
			numb = sh_arith(mac_trim(argc+1,0));
			stakset(savptr,savtop);
			mflag = savem;
			return(0);
		}
		else if(type)
		{
			/* nested command substitution, keep reading */
			stakputc(d);
			match_paren(LPAREN,RPAREN);
		}
		stakseek(staktell()-WC_SZ);
	}
	else
	{
		while((d=io_readc())!=SQUOTE && d)
		{
			if(d==ESCAPE)
			{
				d = io_readc();
				/*
				 * This is wrong but it preserves compatibility with
				 * the SVR2 shell
				 */
				if(!(escchar(d) || (d==L'"' && quote)))
					stakputc(ESCAPE);
			}
			stakputc(d);
		}
	}
	argc=(wchar_t *)wc_stakfreeze(WC_SZ);
	st.states &= ~FIXFLG;	/* do not save command subs in history file */
	if(w_fd>=0)
	{
		p_setout(w_fd);
		p_flush();	/* flush before executing command */
	}
	io_push(&cb);
	io_sopen(argc);
	sh.nested_sub = 0;
	st.exec_flag++;
	t = sh_parse(EOFSYM,MTFLG|NLFLG);
	st.exec_flag--;
	if(!t || is_option(NOEXEC))
		goto readit;
	if(!sh.nested_sub && !t->tre.treio && t->tre.tretyp==0 && is_rbuiltin(t))
	{
		/* nested command subs not handled specially */
		/* handle command substitution of most builtins separately */
		/* exec, login, cd, ., eval and shift not handled this way */
		/* put output into tmpfile */
		int save1_out = st.standout;
		if((st.states&IS_TMP)==0)
		{
			wchar_t tmp_fname[TMPSIZ];
			/* create and keep open a /tmp file for command subs */
			fd = io_mktmp(tmp_fname);
			fd = io_renumber(fd,TMPIO);
			st.states |= IS_TMP;
			/* root cannot unlink because fsck could give bad ref count */
			if(sh.userid || !is_option(INTFLG))
				ksh_unlink(tmp_fname);
			else
				st.states |= RM_TMP;
		}
		else
			fd = TMPIO;
		st.standout = fd;
		/* this will only flush the buffer if output is fd already */
		p_setout(fd);
		p_char(0L);
		st.subflag++;
		sh_funct(t,(wchar_t**)0,(int)(st.states&ERRFLG),(struct argnod*)0);
		st.subflag = 0;
		p_setout(fd);
		p_char(0L);
		if(*_SObuf != 0L || is_option(EXECPR))
		{
			/* file is larger than buffer, read from it */
			p_flush();
			io_seek(fd,(off_t)1,SEEK_SET);
			io_init(input=fd,st.standin,inbuff);
			waitflag = -1;
		}
		else
		{
			/* The file is all in the buffer */
			wcscpy(inbuff,_SObuf+1);
			io_sopen(inbuff);
			io_ftable[fd]->ptr = io_ftable[fd]->base;
		}
		st.standout = save1_out;
		goto readit;
	}
	else if(t->tre.tretyp==0 && t->com.comarg==0)
	{
		if(t->tre.treio && (((t->tre.treio)->iofile)&IOUFD)==0)
		{
			struct stat statb;
			fd = io_redirect(t->tre.treio,3);
			if(fstat(fd,&statb)>=0){
				stacksize = 4*statb.st_size;
			}
		}
		else
			fd = io_fopen(wc_e_devnull);
	}
	else
	{
		int 	pv[2];
		int forkflag = FPOU|FCOMSUB;
		waitflag++;
		if(st.iotemp!=saviotemp)
			forkflag |= FTMP;
		t = sh_mkfork(forkflag,t);
		  /* this is done like this so that the pipe
		   * is open only when needed
		   */
		io_popen(pv);
		sh.inpipe = 0;
		sh.outpipe = pv;
		sh_exec(t, (int)(st.states&ERRFLG));
		fd = pv[INPIPE];
		io_fclose(pv[OTPIPE]);
	}
	io_init(input=fd,st.standin,inbuff);

readit:
	mflag = savem;
	stakset(savptr,savtop);
	if(stacksize)
	{
		/* cause preallocation of stack frame */
		stakseek(savtop+stacksize);
		stakseek(savtop);
	}
	mac_copy((wchar_t*)0,-1);
	if(waitflag>0)
		job_wait(sh.subpid);
	dd = staktell();
	while(dd>0)
	{
		dd -= WC_SZ;
		if(*((wchar_t *)stakptr(dd)) != NL)
		{
			dd += WC_SZ;
			break;
		}
		else if(quote)
			dd -= WC_SZ;
	}
	stakseek(dd);
	io_pop(waitflag>=0?0:1);
	st.states |= saveflags;
	sh_freeup();
	st.iotemp = saviotemp;
	st.staklist = saveslp;
	if(w_fd >=0)
		p_setout(w_fd);
	if(type == 0 && !xpg_compliant() && sh.exitval && is_option(ERRFLG))
		sh_exit(sh.exitval);
	return(1);
}


/*
 * Copy and expand a here-document
 */

int mac_here(iop)
register struct ionod	*iop;
{
	register wchar_t	c;
	register int 	in;
	register int	ot;
	struct fileblk 	fb;
	wchar_t inbuff[IOBSIZE+1];
	quote = 0;
	ifs = wc_e_nullstr;
	mflag = 1;
	ot = io_mktmp(inbuff);
	ksh_unlink(inbuff);
	w_fd = ot;
	io_push(&fb);
	if(iop->iofile&IOSTRG)
	{
		io_sopen(iop->ioname);
		in = F_STRING;
	}
	else
	{
		in = io_fopen(iop->ioname);
		io_init(in,&fb,inbuff);
		}
	p_setout(ot);
	stakseek(0);
	if(is_option(EXECPR))
		sh.heretrace=1;
	while(1)
	{
		c=getch(0L);
		if(c==ESCAPE)
		{
			c = io_readc();
			if(!escchar(c) && !qotchar(c))
				stakputc(ESCAPE);
		}
		if(staktell())
		{
			*((wchar_t *)stakptr(staktell())) = 0L;
			p_str((wchar_t *)stakptr(0),c);
			stakseek(0);
		}
		else if(c)
			p_char(c);
		if(c==0L)
			break;
	}
	p_flush();
	sh.heretrace=0;
	mflag = 0;
	io_pop(0);
	w_fd = -1;
	io_ftable[ot] = 0;
	lseek(ot,(off_t)0,SEEK_SET);
	return(ot);
}


/*
 * copy value of string or file onto the stack inserting backslashes
 * as needed to prevent word splitting and file expansion
 */

static void mac_copy(str,size)
register wchar_t *str;
register int size;
{
	register wint_t c;
	while(size!=0 && (c = (str?*str++:io_readc())))
	{
		/*@ assert (!mflag&&quote)==0; @*/
		if(quote || (!mflag&&addescape(c)&&(c==ESCAPE||!wcschr(ifs,c))))
	 		stakputc(ESCAPE); 
 		stakputc(c); 
		if(size>0)
			size--;
	}
}
 
/*
 * Deletes the right substring of STRING using the expression PAT
 * the longest substring is deleted when FLAG is set.
 */

static int substring(string,pat,flag)
register wchar_t *string;
wchar_t *pat;
int flag;
{
	register wchar_t *sp = string;
	register int size;
	sp += wcslen(sp);
	size = sp-string;
	while(sp>=string)
	{
		if(strmatch(sp,pat))
		{
			size = sp-string;
			if(flag==0)
				break;
		}
		sp--;
#ifdef MULTIBYTE
		if(*sp&HIGHBIT)
		{
			if(*(sp-in_csize(3))==ESS3)
				sp -= in_csize(3);
			else if(*(sp-in_csize(2))==ESS2)
				sp -= in_csize(2);
			else
				sp -= (in_csize(1)-1);
		}
#endif /* MULTIBYTE */
	}
	return(size);
}


/*
 * do parameter and command substitution and strip of quotes
 * attempt file name expansion if <type> not zero
 */

wchar_t *mac_trim(s,type)
wchar_t *s;
{
	register wchar_t *t;
	struct argnod *schain = st.gchain;
	t=mac_expand(s);
	if(type && f_complete(t,e_nullstr)==1)
		t = st.gchain->argval;
	st.gchain = schain;
	sh_trim(t);
	return(t);
}

/*
 * perform only parameter substitution and catch failures
 */

wchar_t *mac_try(s)
register wchar_t *s;
{
	if(s)
	{
		register wint_t savec = st.peekn;
		mactry++;
		if(SETJMP(mac_buf)==0)
			s = mac_trim(s,0);
		else
		{
			io_pop(1);
			st.peekn = savec;
		}
		mactry = 0;
	}
	if(s==0L)
		return(WCNULLSTR);
	return(s);
}

static void mac_error()
{
	sh_fail(mac_current,ksh_gettxt(_SGI_DMMX_e_subst,e_subst),ERROR);
}

/*
 * check to see if the error occured while expanding prompt
 */

void mac_check()
{
	if(mactry)
		LONGJMP(mac_buf,1);
}



#ifdef MULTIBYTE
static int	charlen(str)
register char *str;
{
	register int n = 0;
	register int c;
	while(*str)
	{
		c = echarset(*str);		/* find character set */
		str += (in_csize(c)+(c>=2));	/* move to next char */
		n += out_csize(c);		/* add character size */
	}
	return(n);
}
#endif /* MULTIBYTE */
