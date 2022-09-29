#ident "$Revision: 1.12 $"

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bfs:bfs.c	1.22"

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <locale.h>
#include <regex.h>
#include <nl_types.h>
#include <msgs/uxeoe.h>

int setjmp();
jmp_buf env;


#ifdef pdp11

#define BRKTYP	char
#define BRKSIZ	4096
#define BRKTWO	2
#define BFSAND	&0377
#define BFSLIM	254
#define BFSTRU	255
#define BFSBUF	256

#else

#define BRKTYP	short
#define BRKSIZ	8192
#define BRKTWO	4
#define BFSAND
#define BFSLIM	511
#define BFSTRU	511
#define BFSBUF	512
#endif




struct Comd {
	int Cnumadr;
	int Cadr[2];
	char Csep;
	char Cop;
};

int Dot, Dollar;
int markarray[26], *mark;
int fstack[15]={1, -1};
int infildes=0;
int outfildes=1;
char internal[512], *intptr;
char comdlist[100];
char *endds;		/* See brk(2) in manual. */
char charbuf='\n';
wchar_t wCharBuf=L'\n';
int peeked;
char currex[100];
int trunc=BFSTRU;
int crunch = -1;
int txtfd;
off64_t segblk[512],prevblk;
int segoff[512], prevoff;
int oldfd=0;
int flag4=0;
int flag3=0;
int flag2=0;
int flag1=0;
int flag=0;
int lprev=1;
int status[1];

BRKTYP *lincnt;
regex_t rexPerBuf;
char tty, *bigfile;
char fle[80];
char prompt=1;
char verbose=1;		/* 1=print # of bytes read in; 0=silent. */
char varray[10][100];	/* Holds xv cmd parameters. */
double outcnt;
char strtmp[32];
wchar_t wStrtmp[512];
nl_catd catd;

/*
** The following structure and declaration are in
** stdio.h but that file can't be included in bfs.c
** because of name conflicts (e.g. getc) with functions
** used throughout the program.
*/
typedef struct {
	int	_cnt;
	unsigned char	*_ptr;
	unsigned char	*_base;
	char	_flag;
	char	_file;
} FILE;
extern FILE _iob[1];
#define stdout	&_iob[1]

extern int	printf(const char *, ...);

#define	REGERR_MAXLEN	256	/* Max error message length returned by 
				   regerror() */


/*****************************************************************************
 * Purpose : wide char version of getc()
 * Parameters : None
 * Return Values : a wide character
 *****************************************************************************
 */
static wchar_t
wGetC()
{
	if(!peeked) {
		while((!(infildes == oldfd && flag)) && (!flag1) && (!wReadC(infildes,&wCharBuf))) {
			if(infildes == 100 && (!flag)) flag1 = 1;
			if((infildes=pop(fstack)) == -1) quit();
			if((!flag1) && infildes == 0 && flag3 && prompt) printf("*");
		}
		if(infildes == oldfd && flag) flag2 = 0;
		flag1 = 0;
	}
	else peeked = 0;
	charbuf = (char)wCharBuf;
	return(wCharBuf);
}


static char
getc()
{
	if(!peeked) {
		while((!(infildes == oldfd && flag)) && (!flag1) && (!readc(infildes,&charbuf))) {
			if(infildes == 100 && (!flag)) flag1 = 1;
			if((infildes=pop(fstack)) == -1) quit();
			if((!flag1) && infildes == 0 && flag3 && prompt) printf("*");
		}
		if(infildes == oldfd && flag) flag2 = 0;
		flag1 = 0;
	}
	else peeked = 0;

	wCharBuf = (wchar_t)charbuf;
	return(charbuf);
}


main(argc,argv)
int argc;
char *argv[];
{
	void reset();
	struct Comd comdstruct, *p;

	char	reErrBuf[REGERR_MAXLEN];	/* Holds regerror message */
	int	iErrCode;

	(void) setlocale(LC_ALL, "");
	catd = catopen ("uxeoe", 0);
	if(argc < 2 || argc > 3) {
		err(1, CATGETS(catd, _MSG_BFS_ARG_COUNT));
		quit();
	}
	mark = markarray-'a';
	if(argc == 3) verbose = 0;
	setbuf(stdout, 0);
	if(bigopen(bigfile=argv[argc-1])) quit();
	tty = isatty(0);

	p = &comdstruct;
		/* Look for 0 or more non-'%' char followed by a '%' */
	iErrCode = regcomp(&rexPerBuf, "[^%]*%", 0);
	if(iErrCode) {
                regerror (iErrCode, &rexPerBuf, reErrBuf, REGERR_MAXLEN);
                err(1, reErrBuf);
	}
	setjmp(env);
#if defined(__STDC__)
	signal(SIGINT, (void (*)(int))reset);
#else
	signal(SIGINT, reset);
#endif
	err(0,"");
	printf("\n");
	flag = 0;
	prompt = 0;
	while(1) begin(p);
}

void
reset()		/* for longjmp on signal */
{
	longjmp(env,1);
}
 

begin(p)
struct Comd *p;
{
	char line[256];

strtagn:	if(flag == 0) eat();
	if(infildes != 100) {
		if(infildes == 0 && prompt) printf("*");
		flag3 = 1;
		getstr(1,line,0,0,0);
		flag3 = 0;
		if(percent(line) < 0) goto strtagn;
		newfile(1,"");
	}
	if(!(getcomd(p,1) < 0)) {

	switch(p->Cop) {

		case 'e':	if(!flag) ecomd(p);
				else err(0,"");
				break;

		case 'f':	fcomd(p);
				break;

		case 'g':	if(flag == 0) gcomd(p,1);
				else err(0,"");
				break;

		case 'k':	kcomd(p);
				break;

		case 'p':	pcomd(p);
				break;

		case 'q':	qcomd(p);
				break;

		case 'v':	if(flag == 0) gcomd(p,0);
				else err(0,"");
				break;

		case 'x':	if(!flag) xcomds(p);
				else err(0,"");
				break;

		case 'w':	wcomd(p);
				break;

		case '\n':	nlcomd(p);
				break;

		case '=':	eqcomd(p);
				break;

		case ':':	colcomd(p);
				break;

		case '!':	excomd(p);
				break;

		case 'P':	prompt = !prompt;
				break;


		default:	if(flag) err(0,"");
				else err(1,CATGETS(catd, _MSG_BFS_BAD_CMD));
				break;
	}
	}
}


bigopen(file)
char file[];
{
	register int l, off, cnt;
	int  newline, n, s;
	char block[512];
	off64_t blk;

	if((txtfd=open(file,0)) < 0) 
		return(err(1,CATGETS(catd, _MSG_BFS_CANT_OPEN)));

	blk = -1;
	newline = 1;
	l = cnt = s = 0;
	off = 512;
	if((lincnt=(BRKTYP*)sbrk(BRKSIZ)) == (BRKTYP*)-1)
		return(err(1,CATGETS (catd, _MSG_BFS_MANY_LINES)));
	endds = (char *)lincnt;	/* Save initial data space address. */

	while((n=read(txtfd,block,512)) > 0) {
		blk++;
		for(off=0; off<n; off++) {
			if(newline) {
				newline = 0;
				if(l>0 && !(l&07777))
					if((BRKTYP*)sbrk(BRKSIZ) == (BRKTYP*)-1)
					  return(err(1,CATGETS (catd, _MSG_BFS_MANY_LINES)));
				lincnt[l] = cnt;
				cnt = 0;
				if(!(l++ & 077)) {
					segblk[s] = blk;
					segoff[s++] = off;
				}
				if(l < 0 || l > 32767) 
					return(err(1,CATGETS (catd, _MSG_BFS_MANY_LINES)));
			}
			if(block[off] == '\n') newline = 1;
			cnt++;
#ifdef pdp11
			if(cnt > 255)
				{printf (CATGETS (catd, _MSG_BFS_TOO_LONG), l);
				return(-1);
				}
#endif
		}
	}
	if(!(l&07777)) if((BRKTYP*)sbrk(BRKTWO) == (BRKTYP*)-1)
		return(err(1,CATGETS(catd, _MSG_BFS_MANY_LINES)));
	lincnt[Dot = Dollar = l] = cnt;
	sizeprt(blk,off);
	return(0);
}


/*
 * Date: 22nd Sep 1997 (i18n by HCL)
 *
 * sizeprt() - prints the size of file in bytes.
 * parameter 'blk' type is changed from int to off64_t as it was not giving 
 * the correct file size in bytes. [was printing the total no. of blocks only]
 */
sizeprt(blk, off)
off64_t blk;
int off;
{
	if(verbose) printf("%.0f",512.*blk+off);
}


off64_t saveblk = -1;


bigread(l,rec)
int l;
char rec[];
{
	register int i;
	register char *r, *b;
	char	*pErrMsg;
	int off;
	static char savetxt[512];

	if((i=l-lprev) == 1) prevoff += lincnt[lprev]BFSAND;
	else if(i >= 0 && i <= 32)
		for(i=lprev; i<l; i++) prevoff += lincnt[i]BFSAND;
	else if(i < 0 && i >= -32)
		for(i=lprev-1; i>=l; i--) prevoff -= lincnt[i]BFSAND;
	else {
		prevblk = segblk[i=(l-1)>>6];
		prevoff = segoff[i];
		for(i=(i<<6)+1; i<l; i++) prevoff += lincnt[i]BFSAND;
	}

	prevblk += prevoff>>9;
	prevoff &= 0777;
	lprev = l;

	if(prevblk != saveblk) {
		lseek64(txtfd,((off64_t)(saveblk=prevblk))<<9,0);
		read(txtfd,savetxt,512);
	}

	r = rec;
	off = prevoff;
	while(1) {
		for(b=savetxt+off; b<savetxt+512; b++) {
			if((*r++ = *b) == '\n') {
				*(r-1) = '\0';
				return;
			}
			if(((unsigned)r - (unsigned)rec) > BFSLIM) {
#ifdef pdp11
				pErrMsg = CATGETS (catd, _MSG_BFS_TOO_LONG_N);
				write(2, pErrMsg, strlen(pErrMsg));
				bfsExit(1);
#else
				pErrMsg = CATGETS (catd, _MSG_BFS_TOO_LONG_O);
				write(2, pErrMsg, strlen(pErrMsg));
				return;
#endif
			}
		}
		read(txtfd,savetxt,512);
		off = 0;
		saveblk++;
	}
}


ecomd(p)
struct Comd *p;
{
	register int i = 0;

	while(peekc() == ' ') getc();
	while((fle[i++] = getc()) != '\n');
	fle[--i] = '\0';
	close(txtfd);	/* Without this, ~20 "e" cmds gave "can't open" msg. */
	brk(endds);	/* Reset data space addr. - mostly for 16-bit cpu's. */
	lprev=1; prevblk=0; prevoff=0; saveblk = -1;	/* Reset parameters. */
	if(bigopen(bigfile =fle)) quit();
	printf("\n");
}

fcomd(p)
struct Comd *p;
{
	if(more() || defaults(p,1,0,0,0,0,0)) return(-1);
	printf("%s\n",bigfile);
	return(0);
}


gcomd(p,k)
int k;
struct Comd *p;
{
	register char d;
	register int i, end;
	char line[BFSBUF];

	regex_t	rexData;			/* Holds compiled RE */
	char 	reErrBuf[REGERR_MAXLEN];	/* Holds regerror message */
	int	iErrCode;

	if(defaults(p,1,2,1,Dollar,0,0)) return(-1);

	if((d=getc()) == '\n') return(err(1,CATGETS(catd, _MSG_BFS_SYNTAX)));
	if(wPeekC() == (wchar_t)d) wGetC();
	else 
		if(getstr(1,currex,d,0,1)) return(-1);

	if ( iErrCode = regcomp (&rexData, currex, REG_EXTENDED | REG_NOSUB)) {
		regerror (iErrCode, &rexData, reErrBuf, REGERR_MAXLEN);
                err(1, reErrBuf);
		return -1;
	}

	if(getstr(1,comdlist,0,0,0)) return(-1);
	i = p->Cadr[0];
	end = p->Cadr[1];
	while (i<=end) {
		bigread(i,line);
		iErrCode = regexec(&rexData, line, 0, NULL, 0);
		if( iErrCode ) {
			if(!k) {
				Dot = i;
				if (xcomdlist(p)) {
					regfree (&rexData);
					return(err(1,CATGETS(catd, _MSG_BFS_BAD_CMD_LIST)));
				}
			}
			i++;
		}
		else {
			if(k) {
				Dot = i;
				if (xcomdlist(p)) {
					regfree (&rexData);
					return(err(1,CATGETS(catd, _MSG_BFS_BAD_CMD_LIST)));
				}
			}
			i++;
		}
	}
	regfree (&rexData);
	return(0);
}


kcomd(p)
struct Comd *p;
{
	register char c;

	c=peekc() ;
	if( !(isascii(c) && islower(c)) ) 
		return(err(1,CATGETS(catd, _MSG_BFS_BAD_MARK)));
	getc();
	if(more() || defaults(p,1,1,Dot,0,1,0)) return(-1);

	mark[c] = Dot = p->Cadr[0];
	return(0);
}


xncomd(p)
struct Comd *p;
{
	register char c;

	if(more() || defaults(p,1,0,0,0,0,0)) return(-1);

	for(c='a'; c<='z'; c++)
		if(mark[c]) printf("%c\n",c);

	return(0);
}


pcomd(p)
struct Comd *p;
{
	register int i;
	char line[BFSBUF];

	if(more() || defaults(p,1,2,Dot,Dot,1,0)) return(-1);

	for(i=p->Cadr[0]; i<=p->Cadr[1] && i>0; i++) {
		bigread(i,line);
		out(line);
	}
	return(0);
}


qcomd(p)
struct Comd *p;
{
	if(more()) return(-1);
	quit();
}


xcomds(p)
struct Comd *p;
{
	switch(getc()) {
		case 'b':	return(xbcomd(p));
		case 'c':	return(xccomd(p));
		case 'f':	return(xfcomd(p));
		case 'n':	return(xncomd(p));
		case 'o':	return(xocomd(p));
		case 't':	return(xtcomd(p));
		case 'v':	return(xvcomd(p));
		default:	return(err(1,CATGETS(catd, _MSG_BFS_BAD_CMD)));
	}
}


xbcomd(p)
struct Comd *p;
{
	register int fail, n;
	register char d;
	char str[50];

	fail = 0;
	if(defaults(p,0,2,Dot,Dot,0,1)) fail = 1;
	else {
		if((d=getc()) == '\n') 
			return(err(1,CATGETS(catd, _MSG_BFS_SYNTAX)));
		if(d == 'z') {
			if(status[0] != 0) return(0);
			getc();
			if(getstr(1,str,0,0,0)) return(-1);
			return(jump(1,str));
		}
		if(d == 'n') {
			if(status[0] == 0) return(0);
			getc();
			if(getstr(1,str,0,0,0)) return(-1);
			return(jump(1,str));
		}
		if(getstr(1,str,d,' ',0)) return(-1);
		if((n=hunt(0,str,p->Cadr[0]-1,1,0,1)) < 0) fail = 1;
		if(getstr(1,str,0,0,0)) return(-1);
		if(more()) 
			return(err(1,CATGETS(catd, _MSG_BFS_SYNTAX)));
	}

	if(!fail) {
		Dot = n;
		return(jump(1,str));
	}
	return(0);
}


xccomd(p)
struct Comd *p;
{
	char arg[100];

	if(getstr(1,arg,0,' ',0) || defaults(p,1,0,0,0,0,0)) return(-1);

	if(equal(arg,"")) crunch = -crunch;
	else if(equal(arg,"0")) crunch = -1;
	else if(equal(arg,"1")) crunch = 1;
	else 
		return(err(1,CATGETS(catd, _MSG_BFS_SYNTAX)));

	return(0);
}


xfcomd(p)
struct Comd *p;
{
	char fl[100];
	register char *f;

	if(defaults(p,1,0,0,0,0,0)) return(-1);

	while(peekc() == ' ') getc();
	for(f=fl; (*f=getc()) != '\n'; f++);
	if(f==fl) 
		return(err(1,CATGETS(catd, _MSG_BFS_NO_FILE)));
	*f = '\0';

	return(newfile(1,fl));
}


xocomd(p)
struct Comd *p;
{
	register int fd;
	char arg[100];

	if(getstr(1,arg,0,' ',0) || defaults(p,1,0,0,0,0,0)) return(-1);

	if(!arg[0]) {
		if(outfildes == 1) return(err(1,CATGETS(catd, _MSG_BFS_NO_DIVERSION)));
		close(outfildes);
		outfildes = 1;
	}
	else {
		if(outfildes != 1) 
			return(err(1,CATGETS(catd, _MSG_BFS_AL_DIVERTED)));
		if((fd=creat(arg,(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH))) < 0)
			return(err(1,CATGETS(catd, _MSG_BFS_CANT_CREATE)));
		outfildes = fd;
	}
	return(0);
}


xtcomd(p)
struct Comd *p;
{
	register int t;

	while(peekc() == ' ') getc();
	if((t=rdnumb(1)) < 0 || more() || defaults(p,1,0,0,0,0,0))
		return(-1);

	trunc = t;
	return(0);
}


xvcomd(p)
struct Comd *p;
{
	register char c;
	register int i;
	int temp0, temp1, temp2;
	int fildes[2];

	c=peekc() ;
	if( !(isascii(c) && isdigit(c)) ) 
		return(err(1, CATGETS(catd, _MSG_BFS_DIGIT_REQD)));
	getc();
	c -= '0';
	while(peekc() == ' ') getc();
	if(peekc()=='\\') getc();
	else if(peekc() == '!') {
		if(pipe(fildes) < 0) {
			printf(CATGETS(catd, _MSG_BFS_TRY_AGAIN));
			return;
		}
		temp0 = dup(0);
		temp1 = dup(1);
		temp2 = infildes;
		close(0);
		dup(fildes[0]);
		close(1);
		dup(fildes[1]);
		close(fildes[0]);
		close(fildes[1]);
		getc();
		flag4 = 1;
		excomd(p);
		close(1);
		infildes = 0;
	}
	for(i=0;(varray[c][i] = getc()) != '\n';i++);
	varray[c][i] = '\0';
	if(flag4) {
		infildes = temp2;
		close(0);
		dup(temp0);
		close(temp0);
		dup(temp1);
		close(temp1);
		flag4 = 0;
		charbuf = ' ';

		wCharBuf = L' ';
	}
	return(0);
}


wcomd(p)
struct Comd *p;
{
	register int i,fd, savefd;
	int savecrunch, savetrunc;
	char arg[100], line[BFSBUF];

	if(getstr(1,arg,0,' ',0) || defaults(p,1,2,1,Dollar,1,0)) return(-1);

	if(!arg[0]) 
		return(err(1,CATGETS(catd, _MSG_BFS_NO_FILE_NAME)));
	if(equal(arg,bigfile)) 
		return(err(1,CATGETS(catd, _MSG_BFS_NO_CHANGE)));
	if((fd=creat(arg,(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH))) <0)
		return(err(1,CATGETS(catd, _MSG_BFS_CANT_CREATE)));

	savefd = outfildes;
	savetrunc = trunc;
	savecrunch = crunch;
	outfildes = fd;
	trunc = BFSTRU;
	crunch = -1;

	outcnt = 0;
	for(i=p->Cadr[0]; i<=p->Cadr[1] && i>0; i++) {
		bigread(i,line);
		out(line);
	}
	if(verbose) printf("%.0f\n",outcnt);
	close(fd);

	outfildes = savefd;
	trunc = savetrunc;
	crunch = savecrunch;
	return(0);
}


nlcomd(p)
struct Comd *p;
{
	if(defaults(p,1,2,Dot+1,Dot+1,1,0)) {
		getc();
		return(-1);
	}
	return(pcomd(p));
}


eqcomd(p)
struct Comd *p;
{
	if(more() || defaults(p,1,1,Dollar,0,0,0)) return(-1);
	printf("%d\n",p->Cadr[0]);
}


colcomd(p)
struct Comd *p;
{
	return(defaults(p,1,0,0,0,0,0));
}


xcomdlist(p)
struct Comd *p;
{
	flag = 1;
	flag2 = 1;
	newfile(1,"");
	while(flag2) begin(p);
	if(flag == 0) return(1);
	flag = 0;
	return(0);
}


excomd(p)
struct Comd *p;
{
	register pid_t i;
	register int j;
	void reset();

	if(infildes != 100) {
		charbuf = '\n';

		wCharBuf = L'\n';
	}
	while((i=fork()) < (pid_t)0) sleep(10);
	if(i == (pid_t)0) {
		signal(SIGINT, SIG_DFL); /*Guarantees child can be intr. */
		if(infildes == 100 || flag4) {
			execl("/usr/bin/sh","sh","-c",intptr,0);
			bfsExit(0);
		}
		if(infildes != 0) {
			close(0);
			dup(infildes);
		}
		for(j=3; j<15; j++) close(j);
		execl("/usr/bin/sh","sh","-t",0);
		bfsExit(0);
	}
	signal(SIGINT, SIG_IGN);
	while(wait(status) != i);
	status[0] = status[0] >> 8;

#if defined(__STDC__)
	signal(SIGINT, (void (*)(int))reset);
#else
	signal(SIGINT, reset);	/* Restore signal to previous status */
#endif

	if((infildes == 0 || (infildes  == 100 && fstack[fstack[0]] == 0))
				&& verbose && (!flag4)) printf("!\n");
	return(0);
}


defaults(p,prt,max,def1,def2,setdot,errsok)
struct Comd *p;
int prt, max, def1, def2, setdot, errsok;
{
	if(!def1) def1 = Dot;
	if(!def2) def2 = def1;
	if(p->Cnumadr >= max) 
		return(errsok?-1:err(prt, CATGETS(catd, _MSG_BFS_ADR_COUNT)));
	if(p->Cnumadr < 0) {
		p->Cadr[++p->Cnumadr] = def1;
		p->Cadr[++p->Cnumadr] = def2;
	}
	else if(p->Cnumadr < 1)
		p->Cadr[++p->Cnumadr] = p->Cadr[0];
	if(p->Cadr[0] < 1 || p->Cadr[0] > Dollar ||
	   p->Cadr[1] < 1 || p->Cadr[1] > Dollar)
		return(errsok?-1:err(prt, CATGETS(catd, _MSG_BFS_RANGE_ERR)));
	if(p->Cadr[0] > p->Cadr[1]) 
		return(errsok?-1:err(prt, CATGETS(catd, _MSG_BFS_ADR1_GT_ADR2)));
	if(setdot) Dot = p->Cadr[1];
	return(0);
}


getcomd(p,prt)
struct Comd *p;
int prt;
{
	register int r;
	register char c;

	p->Cnumadr = -1;
	p->Csep = ' ';
	switch(c = peekc()) {
		case ',':
		case ';':	p->Cop = getc();
				return(0);
	}

	if((r=getadrs(p,prt)) < 0) return(r);

	c=peekc();
	if(c == '\n') p->Cop = '\n';
	else p->Cop = getc();

	return(0);
}


getadrs(p,prt)
struct Comd *p;
int prt;
{
	register int r;
	register char c;

	if((r=getadr(p,prt)) < 0) return(r);

	switch(c=peekc()) {
		case ';':	Dot = p->Cadr[0];
		case ',':	getc();
				p->Csep = c;
				return(getadr(p,prt));
	}

	return(0);
}


getadr(p,prt)
struct Comd *p;
int prt;
{
	register int r;
	register char c;

	r = 0;
	while(peekc() == ' ') getc();		/* Ignore leading spaces */
	switch(c = peekc()) {
		case '\n':
		case ',':
		case ';':	return(0);

		case '\'':	getc();
				r = getmark(p,prt);
				break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':	r = getnumb(p,prt);
				break;

		case '.':	getc();
		case '+':
		case '-':	p->Cadr[++p->Cnumadr] = Dot;
				break;

		case '$':	getc();
				p->Cadr[++p->Cnumadr] = Dollar;
				break;

		case '^':	getc();
				p->Cadr[++p->Cnumadr] = Dot - 1;
				break;

		case '/':
		case '?':
		case '>':
		case '<':	getc();
				r = getrex(p,prt,c);
				break;

		default:	return(0);
	}

	if(r == 0) r = getrel(p,prt);
	return(r);
}


getnumb(p,prt)
struct Comd *p;
int prt;
{
	register int i;

	if((i=rdnumb(prt)) < 0) return(-1);
	p->Cadr[++p->Cnumadr] = i;
	return(0);
}


rdnumb(prt)
int prt;
{
	char num[20], *n;
	int i;

	n = num;

	while( isascii(*n=peekc()) && isdigit(*n) ) {
		n++;
		getc();
	}

	*n = '\0';
	if((i=patoi(num)) >= 0) return(i);
	return(err(prt,CATGETS(catd, _MSG_BFS_BAD_NUM)));
}


getrel(p,prt)
struct Comd *p;
int prt;
{
	register int op, n;
	register char c;
	int j;

	n = 0;
	op = 1;
	while((c=peekc())=='+' || c=='-') {
		if(c=='+') n++;
		else n--;
		getc();
	}
	j = n;
	if(n < 0) op = -1;
	if(c=='\n') p->Cadr[p->Cnumadr] += n;
	else {
		if((n=rdnumb(0)) > 0 && p->Cnumadr >= 0) {
			p->Cadr[p->Cnumadr] += op*n;
			getrel(p,prt);
		}
		else {
			if(c=='-') p->Cadr[p->Cnumadr] += j;
			else p->Cadr[p->Cnumadr] += j;
		}
	}
	return(0);
}


getmark(p,prt)
struct Comd *p;
int prt;
{
	register char c;

	c=peekc() ;
	if( !(isascii(c) && islower(c)) ) 
		return(err(prt,CATGETS(catd, _MSG_BFS_BAD_MARK)));
	getc();

	if(!mark[c]) return(err(prt,CATGETS(catd, _MSG_BFS_UNDEF_MARK)));
	p->Cadr[++p->Cnumadr] = mark[c];
	return(0);
}


getrex(p,prt,c)
struct Comd *p;
int prt;
char c;
{
	register int down, wrap, start;

	if(wPeekC() == (wchar_t)c) wGetC();
	else if(getstr(prt,currex,c,0,1)) return(-1);

	switch(c) {
		case '/':	down = 1; wrap = 1; break;
		case '?':	down = 0; wrap = 1; break;
		case '>':	down = 1; wrap = 0; break;
		case '<':	down = 0; wrap = 0; break;
	}

	if(p->Csep == ';') start = p->Cadr[0];
	else start = Dot;

	if((p->Cadr[++p->Cnumadr]=hunt(prt,currex,start,down,wrap,0)) < 0)
		return(-1);
	return(0);
}


hunt(prt,rex,start,down,wrap,errsok)
int prt, errsok;
char rex[];
int start, down, wrap;
{
	register int i, end1, incr;
	int start1, start2;
	char line[BFSBUF];

	regex_t rexData;                        /* Holds compiled RE */
	char    reErrBuf[REGERR_MAXLEN];        /* Holds regerror message */
	int     iErrCode;

	if(down) {
		start1 = start + 1;
		end1 = Dollar;
		start2 = 1;
		incr = 1;
	}
	else {
		start1 = start  - 1;
		end1 = 1;
		start2 = Dollar;
		incr = -1;
	}

	if ( iErrCode = regcomp (&rexData, rex, REG_EXTENDED | REG_NOSUB)) {
                regerror (iErrCode, &rexData, reErrBuf, REGERR_MAXLEN);
                err(1, reErrBuf);
		/*
		 * Date: 22nd Jan 1998 (i18n by HCL)
		 * 
		 * original bfs was giving core dump when an erroneous
		 * regular expression is entered for '/', '?', '>' & '<'
		 * bfs commands. This was because of the reason that control
		 * was not returned back on encountering the error. To remove
		 * this problem, a return call with error status ('-1') has 
		 * been added.
		 */
		return -1;
	}

	for(i=start1; i != end1+incr; i += incr) {
		bigread(i,line);
		iErrCode = regexec(&rexData, line, 0, NULL, 0);
		if( ! iErrCode ) {
			regfree (&rexData);
			return(i);
		}
	}

	if(!wrap) return(errsok?-1:err(prt,CATGETS(catd, _MSG_BFS_NOT_FOUND)));

	for(i=start2; i != start1; i += incr) {
		bigread(i,line);
		iErrCode = regexec(&rexData, line, 0, NULL, 0);
		if( ! iErrCode ) {
			regfree (&rexData);
			return(i);
		}
	}

	regfree (&rexData);
	return(errsok?-1:err(prt,CATGETS(catd, _MSG_BFS_NOT_FOUND)));
}


jump(prt,label)
int prt;
char label[];
{
	register char *l;
	char line[256];

	regex_t rexData;			/* Holds compiled RE */
	char	reErrBuf[REGERR_MAXLEN];	/* Holds regerror message */
	int	iErrCode;

	if(infildes == 0 && tty) 
		return(err(prt,CATGETS(catd, _MSG_BFS_JMP_ON_TTY)));
	if(infildes == 100) intptr = internal;
	else lseek(infildes,0L,0);

	sprintf(strtmp, "^: *%s$", label);
	if ( iErrCode = regcomp(&rexData, strtmp, REG_EXTENDED | REG_NOSUB)) {
		regerror(iErrCode, &rexData, reErrBuf, REGERR_MAXLEN);
                err(1, reErrBuf);
		return -1;
	}

	for(l=line; readc(infildes,l); l++) {
		if(*l == '\n') {
			*l = '\0';
			iErrCode = regexec(&rexData, line, 0, NULL, 0);
			if( ! iErrCode ) {
				charbuf = '\n';

				wCharBuf = L'\n';
				regfree (&rexData);
				return(peeked = 0);
			}
			l = line - 1;
		}
	}

	regfree (&rexData);
	return(err(prt,CATGETS(catd, _MSG_BFS_LABEL_NOT_FOUND)));
}


getstr(prt,buf,brk,ignr,nonl)
int prt, nonl;
char buf[], brk, ignr;
{
	register char *b, c, prevc;

	register wchar_t *wB, wC, wPrevC;

	/* This modification has been done to avoid the processing on 
	   "\" which is in the second byte of a multibyte char */
	wPrevC = 0;
	for(wB=wStrtmp; wC=wPeekC(); wPrevC=wC) {
		if(wC == L'\n') {
			if(wPrevC == L'\\' && (!flag3)) *(wB-1) = wGetC();
			else if(wPrevC == L'\\' && flag3) {
				*wB++ = wGetC();
			}
			else if(nonl) break;
			else {
				*wB = L'\0';
				wcstombs (buf, wStrtmp, sizeof (wStrtmp));
				return(0);
			}
		}
		else {
			wGetC();
			if(wC == (wchar_t)brk) {
				if(wPrevC == L'\\') *(wB-1) = wC;
				else {
					*wB = L'\0';
					wcstombs (buf, wStrtmp, sizeof (wStrtmp));
					return(0);
				}
			}
			else if(wB != wStrtmp || wC != (wchar_t)ignr) *wB++ = wC;
		}
	}
	wcstombs (buf, wStrtmp, sizeof (wStrtmp));

	return(err(prt,CATGETS(catd, _MSG_BFS_SYNTAX)));
}

err(prt,msg)
int prt;
char msg[];
{
	if(prt) (prompt? printf("%s\n",msg): printf("?\n"));
	if(infildes != 0) {
		infildes = pop(fstack);
		charbuf = '\n';

		wCharBuf = L'\n';
		peeked = 0;
		flag3 = 0;
		flag2 = 0;
		flag = 0;
	}
	return(-1);
}


/*****************************************************************************
 * Purpose : wide char version of readc()
 * Parameters : iFilDes - file handler from which the wide char is to be read
 *              wCh     - buffer to store wide char read
 * Return Values : 1 - for successful read, and
 *		   0 - for failure.
 *****************************************************************************
 */
wReadC(iFilDes, wCh)
int iFilDes;
wchar_t *wCh;
{
	int	iLen;

	if(iFilDes == 100) {
		iLen = mbtowc (wCh, intptr, MB_CUR_MAX);
		if(iLen > 0) {
			intptr += iLen;
		}
		else {
			wCharBuf = L'\n';
			charbuf = '\n';
			return(0);
		}
	}
	else if(iReadfile_wc(iFilDes,wCh) <= 0) {
		close(iFilDes);
		wCharBuf = L'\n';
		charbuf = '\n';
		return(0);
	}
	charbuf = (char)wCharBuf;
	return(1);
}


readc(f,c)
int f;
char *c;
{
	if(f == 100) {
		if(!(*c = *intptr++)) {
			intptr--;
			charbuf = '\n';

			wCharBuf = L'\n';
			return(0);
		}
	}
	else if(read(f,c,1) != 1) {
		close(f);
		charbuf = '\n';

		wCharBuf = L'\n';
		return(0);
	}
	return(1);
}


percent(line)
char line[256];
{
	register char *lp, *var;
	char *front, *per, c[2], *olp, p[2], fr[256], *copy();
	int i,j;
	regmatch_t	rmMatch;
	int nFrLen, iLen=0;

	per = p;
	var = c;
	front = fr;
	j = 0;
	while(!j) {
		j = 1;
		olp = line;
		intptr = internal;
		rmMatch.rm_eo = 0;
		while( ! regexec(&rexPerBuf, olp, 1, &rmMatch, 0)) {
			while(rmMatch.rm_so < rmMatch.rm_eo) 
				*front++ = olp[rmMatch.rm_so++];
			*(--front) = '\0';
			front = fr;
			*per++ = '%';
			*per = '\0';
			per = p;
			*var = olp[rmMatch.rm_eo];
			/*
			 * This code checks if 'percent' is preceded
			 * by a '\' to escape its special meaning,
			 * then it doesn't replace variable by its
			 * value 
			 */
			nFrLen = strlen (front);
			i = 0;
			while ( i < nFrLen )
			{
				iLen = mblen (front, MB_CUR_MAX);
				front += iLen;
				i+= iLen;
			}
			front = fr;
				
			if((i = strlen(front)) >= 1 && fr[i-iLen] == '\\') {
				cat(front,"");
				--intptr;
				cat(per,"");
			}
			else {
				if (!(isdigit(*var)))
					return(err(1,CATGETS(catd,_MSG_BFS_USAGE_DIG)));
				cat(front,"");
				cat(varray[*var-'0'],"");
				j =0;
				rmMatch.rm_eo++;	/* Compensate for removing --lp above */
			}
			olp = olp + rmMatch.rm_eo;
		}
		cat(olp,"");
		*intptr = '\0';
		if(!j) {
			intptr = internal;
			lp = line;
			copy(intptr,lp);
		}
	}
	return(0);
}

cat(arg1,arg2)
char arg1[],arg2[];
{
	register char *arg;
	arg = arg1;
	while(*arg) *intptr++ = *arg++;
	if(*arg2) {
		arg = arg2;
		while(*arg) *intptr++ = *arg++;
	}
}


newfile(prt,f)
int prt;
char f[];
{
	register int fd;

	if(!*f) {
		if(flag != 0) {
			oldfd = infildes;
			intptr = comdlist;
		}
		else intptr = internal;
		fd = 100;
	}
	else if((fd=open(f,0)) < 0) {
		sprintf(strtmp, CATGETS (catd, _MSG_BFS_CANT_OPEN_FILE), f);
		return err(prt, strtmp);
	}

	push(fstack,infildes);
	if(flag4) oldfd = fd;
	infildes = fd;
	return(peeked=0);
}


push(s,d)
int s[], d;
{
	s[++s[0]] = d;
}


pop(s)
int s[];
{
	return(s[s[0]--]);
}


/*****************************************************************************
 * Purpose : wide char version of peekc()
 * Parameters : None
 * Return Values : a wide character
 *****************************************************************************
 */
wPeekC()
{
	register wchar_t wC;

	wC = wGetC();
	peeked = 1;

	return(wC);
}


peekc()
{
	register char c;

	c = getc();
	peeked = 1;

	return(c);
}


eat()
{
	if(charbuf != '\n') while(getc() != '\n');
	peeked = 0;
}


more()
{
	if(getc() != '\n') 
		return(err(1,CATGETS(catd, _MSG_BFS_SYNTAX)));
	return(0);
}


quit()
{
	bfsExit(0);
}


/*****************************************************************************
 * Purpose : closes the catalog file and exits with the status pased
 * Parameters : iStatus - Status to be passed while exiting the program
 * Return Values : None
 *****************************************************************************
 */
bfsExit(iStatus)
int iStatus;
{
	catclose (catd);
	regfree(&rexPerBuf);
	exit(iStatus);
}


out(ln)
char *ln;
{
	register char *rp, *wp, prev;
	register int w, width;
	char *oldrp;
	wchar_t cl;
	char *untab();
	int lim, p;

	if(crunch > 0) {

		ln = untab(ln);
		rp = wp = ln - 1;
		prev = ' ';

		while(*++rp) {
			if(prev != ' ' || *rp != ' ') *++wp = *rp;
			prev = *rp;
		}
		*++wp = '\n';
		lim = wp - ln;
		*++wp = '\0';

		if(*ln == '\n') return;
	}
	else ln[lim=strlen(ln)] = '\n';
	
	if(MB_CUR_MAX <= 1) {
		if(lim > trunc) ln[lim=trunc] = '\n';
	}
	else {
		if ((trunc < (BFSBUF -1)) || (lim > trunc)) {
			w = 0;
			oldrp = rp = ln;
			while (1) {
				if((p = mbtowc(&cl, rp, MB_LEN_MAX)) == 0) {
					break;
				}
				/*
				 * Date: 22nd Sep 1997 (i18n by HCL)
				 * 
				 * bfs gives erroneous output while printing
				 * single byte character in multi-byte locale
				 * when xt > length of the line. To remove 
				 * this error, a newline check has been added 
				 * within the loop.
				 */
				if (*rp == '\n')
					break;

				if (p == -1) {
					width = p = 1;
				} else {
					if ( iswprint(cl) )
						width = wcwidth(cl);
					else width = 1;

					if (width == 0)
						width = 1;
				}
				if ((w += width) > trunc)
					break;
				rp += p;
			}
			*rp = '\n';
			lim = rp - oldrp;
		}
	}

	outcnt += write(outfildes,ln,lim+1);
}




char *
untab(l)
char l[];
{
	static char line[BFSBUF];
	register char *q, *s;

	s = l;
	q = line;
	do {
		if(*s == '\t')
			do *q++ = ' '; while((q-line)%8);
		else *q++ = *s;
	} while(*s++);
	return(line);
}
/*
	Function to convert ascii string to integer.  Converts
	positive numbers only.  Returns -1 if non-numeric
	character encountered.
*/

patoi(b)
char *b;
{
	register int i;
	register char *a;

	a = b;
	i = 0;

	while( isdigit (*a) ) i = 10 * i + *a++ - '0';

	if(*a) return(-1);
	return(i);
}

/*
	Copy first string to second; no overflow checking.
	Returns pointer to null in new string.
*/

char *
copy(a,b)
register char *a, *b;
{
	while(*b++ = *a++);
	return(--b);
}
/*
	Compares 2 strings.  Returns 1 if equal, 0 if not.
*/

equal(a,b)
char *a, *b;
{
	register char *x, *y;

	x = a;
	y = b;
	while (*x == *y++) if (*x++ == 0) return(1);
	return(0);
}



/*****************************************************************************
 * Purpose : reads a single wide char from file f and accordingly
 *	     sets the file pointer.
 * Parameters : iFilDes - file handler from which the wide char is to be read
 *              c - buffer to store wide char read
 * Return Values : length (in bytes) of the wide char read
 *****************************************************************************
 */
iReadfile_wc (iFilDes,pWC)
int iFilDes;
wchar_t *pWC;
{
	int	i=0,iRet, nLen;
	char	mbStr[32], *p=mbStr;

	while ( i++ < MB_CUR_MAX)
	{
		iRet = read (iFilDes, p++, 1);
		if (iRet > 0) {
			nLen = mbtowc (pWC, mbStr, i);
			if (nLen > 0) {
				return nLen;
			}
		}
		else {
			return iRet;
		}
	}
	if (i > MB_CUR_MAX) {
		lseek (iFilDes, (1-i), SEEK_CUR);
	}
	return -1;
}


