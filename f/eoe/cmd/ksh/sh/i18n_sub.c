
#ident	"@(#)ksh:sh/sys.c	1.1"

#include	"defs.h"
#include	"history.h"


/*	Multibyte and wide-character conversions routines for system calls,
 *	arg lists and message handling. 
 */


int ksh_access(wchar_t *wc_path,int mode)
{
	char  *mb_path;
	int len = wcslen(wc_path)*MB_CUR_MAX+1;
	int ret;

	mb_path = (char *)malloc(len);

	if(MB_CUR_MAX == 1)
	{
		int i;
		for(i=0;i<len && wc_path[i] != 0L;++i)
			mb_path[i] = wc_path[i];
		mb_path[i] = 0;
	}
	else 	wcstombs(mb_path,wc_path,len);
	ret = access(mb_path,mode);
	free(mb_path);
	return(ret);
}
int ksh_chdir(wchar_t *wc_path)
{
	char  *mb_path;
	int len = wcslen(wc_path)*MB_CUR_MAX+1;
	int ret;

	mb_path = (char *)malloc(len);

	if(MB_CUR_MAX == 1)
	{
		int i;
		for(i=0;i<len && wc_path[i] != 0L;++i)
			mb_path[i] = wc_path[i];
		mb_path[i] = 0;
	}
	else 	wcstombs(mb_path,wc_path,len);
	ret = chdir(mb_path);
	free(mb_path);
	return(ret);
}
int ksh_link(wchar_t *wc_path1,wchar_t *wc_path2)
{
	char  *mb_path1,*mb_path2;
	int len1 = wcslen(wc_path1)*MB_CUR_MAX+1;
	int len2 = wcslen(wc_path2)*MB_CUR_MAX+1;
	int ret;

	mb_path1 = (char *)malloc(len1);
	mb_path2 = (char *)malloc(len2);
	if(MB_CUR_MAX == 1)
	{
		int i;
		for(i=0;i<len1 && wc_path1[i] != 0L;++i)
			mb_path1[i] = wc_path1[i];
		mb_path1[i] = 0;
		for(i=0;i<len2 && wc_path2[i] != 0L;++i)
			mb_path2[i] = wc_path2[i];
		mb_path2[i] = 0;
	}
	else
	{
		wcstombs(mb_path1,wc_path1,len1);
		wcstombs(mb_path2,wc_path2,len2);
	}
	ret = link(mb_path1,mb_path2);
	free(mb_path1);
	free(mb_path2);
	return(ret);
}
int ksh_unlink(wchar_t *wc_path)
{
	char  *mb_path;
	int len = wcslen(wc_path)*MB_CUR_MAX+1;
	int ret;

	mb_path = (char *)malloc(len);

	if(MB_CUR_MAX == 1)
	{
		int i;
		for(i=0;i<len && wc_path[i] != 0L;++i)
			mb_path[i] = wc_path[i];
		mb_path[i] = 0;
	}
	else 	wcstombs(mb_path,wc_path,len);
	ret = unlink(mb_path);
	free(mb_path);
	return(ret);
}
int ksh_lstat(wchar_t *wc_path, struct stat *buf)
{
	char  *mb_path;
	int len = wcslen(wc_path)*MB_CUR_MAX+1;
	int ret;

	mb_path = (char *)malloc(len);

	if(MB_CUR_MAX == 1)
	{
		int i;
		for(i=0;i<len && wc_path[i] != 0L;++i)
			mb_path[i] = wc_path[i];
		mb_path[i] = 0;
	}
	else 	wcstombs(mb_path,wc_path,len);
	ret = lstat(mb_path,buf);
	free(mb_path);
	return(ret);
}
int ksh_stat(wchar_t *wc_path, struct stat *buf)
{
	char  *mb_path;
	int len = wcslen(wc_path)*MB_CUR_MAX+1;
	int ret;

	mb_path = (char *)malloc(len);

	if(MB_CUR_MAX == 1)
	{
		int i;
		for(i=0;i<len && wc_path[i] != 0L;++i)
			mb_path[i] = wc_path[i];
		mb_path[i] = 0;
	}
	else 	wcstombs(mb_path,wc_path,len);
	ret = stat(mb_path,buf);
	free(mb_path);
	return(ret);
}
int ksh_open(wchar_t *wc_path, int oflag, mode_t mode)
{
	char  *mb_path;
	int len = wcslen(wc_path)*MB_CUR_MAX+1;
	int ret;

	mb_path = (char *)malloc(len);

	if(MB_CUR_MAX == 1)
	{
		int i;
		for(i=0;i<len && wc_path[i] != 0L;++i)
			mb_path[i] = wc_path[i];
		mb_path[i] = 0;
	}
	else 	wcstombs(mb_path,wc_path,len);
	ret = open(mb_path,oflag,mode);
	free(mb_path);
	return(ret);
}
int ksh_readlink(wchar_t *wc_path,wchar_t *wc_buf, size_t bufsize)
{
	char  *mb_path,*mb_buf;
	int len = wcslen(wc_path)*MB_CUR_MAX+1;
	int buflen,ret;

	mb_path = (char *)malloc(len);
	mb_buf  = (char *)malloc(bufsize)+1;
	if(MB_CUR_MAX == 1)
	{
		int i;
		for(i=0;i<len && wc_path[i] != 0L;++i)
			mb_path[i] = wc_path[i];
		mb_path[i] = 0;
	}
	else	wcstombs(mb_path,wc_path,len);
	ret = readlink(mb_path,mb_buf,bufsize);
	buflen = strlen(mb_buf);
	if(MB_CUR_MAX == 1)
	{
		int i;
		for(i=0;i<buflen;++i)
			wc_buf[i] = (unsigned char)mb_buf[i];
		mb_path[i] = 0;
	}
	else	mbstowcs(wc_buf,mb_buf,buflen+1);
	free(mb_buf);
	free(mb_path);
	return(ret);
}
int ksh_creat(wchar_t *wc_path, mode_t mode)
{
	char  *mb_path;
	int len = wcslen(wc_path)*MB_CUR_MAX+1;
	int ret;

	mb_path = (char *)malloc(len);

	if(MB_CUR_MAX == 1)
	{
		int i;
		for(i=0;i<len && wc_path[i] != 0L;++i)
			mb_path[i] = wc_path[i];
		mb_path[i] = 0;
	}
	else 	wcstombs(mb_path,wc_path,len);
	ret = creat(mb_path,mode);
	free(mb_path);
	return(ret);
}

/* argvtowc()
 *
 *	Convert all args to the shell from multibyte to wide-char.
 */
wchar_t **argvtowc(int argc,char **argv) {
        int a,wc_codes,cnt;
        wchar_t **wc_argv;

	wc_argv = (wchar_t **)malloc((argc+1)*sizeof(wchar_t *));

        for(a=0;a<argc;++a)
        {
		if(MB_CUR_MAX == 1)
		{
			wc_codes = strlen(argv[a]);
		}
                else if((wc_codes = mbstowcs((wchar_t *)0,argv[a],1)) == -1){
                        return (wchar_t **)0;
		}
                wc_argv[a] = (wchar_t *)malloc((wc_codes+1)*sizeof(wchar_t));
		if(MB_CUR_MAX == 1)
		{
			int i;
			for(i=0;i<wc_codes;++i)
				wc_argv[a][i] = (unsigned char)argv[a][i];
		}
		else	mbstowcs(wc_argv[a],argv[a],wc_codes);
		wc_argv[a][wc_codes] = 0L;
        }
        wc_argv[argc] = 0L;
        return wc_argv;
}
/* ksh_execve()
 *
 *	Convert all wide char args and environment to multibyte.
 */
void
ksh_execve(wchar_t *wc_path,wchar_t **argv,wchar_t **env)
{
	wchar_t **a = argv;
	wchar_t **e = env;
	char **mb_argv, **mb_env;
	char  *mb_path;
	int mb_len,i;
	int argn=0;
	int envn=0;

	while(*a++)argn++;
	while(*e++)envn++;

	mb_argv = (char **)malloc((argn+1)*sizeof(char *));
	mb_env  = (char **)malloc((envn+1)*sizeof(char *));
	mb_path = (char *) malloc(wcslen(wc_path)*MB_CUR_MAX+1);

	if(MB_CUR_MAX == 1)
	{
		int len = wcslen(wc_path);
		for(i=0;i<len;++i)
			mb_path[i] = wc_path[i];
	}
	else	i = wcstombs(mb_path,wc_path,PATH_MAX);
	mb_path[i] = 0;
	argn = 0;
	a = argv;
	while (*a){
		if(MB_CUR_MAX == 1)
			mb_len = wcslen(*a);
		else	mb_len = wcstombs((char *)0,*a,ARG_MAX);
		mb_argv[argn] = (char *)malloc(mb_len+1);
		if(MB_CUR_MAX == 1)
		{
			int i;
			wchar_t *ar = *a;
			for(i=0;i<mb_len;++i)
				mb_argv[argn][i] = ar[i];
		}
		else	mb_len = wcstombs(mb_argv[argn],*a,ARG_MAX);
		mb_argv[argn++][mb_len] = 0;
		++a;
	}
	mb_argv[argn] = (char *)0;

	envn = 0;
	e = env;
	while (*e){
		if(MB_CUR_MAX == 1)
			mb_len = wcslen(*e);
		else	mb_len = wcstombs((char *)0,*e,ARG_MAX);
		mb_env[envn] = (char *)malloc(mb_len+1);
		if(MB_CUR_MAX == 1)
		{
			int i;
			wchar_t *er = *e;
			for(i=0;i<mb_len;++i)
				mb_env[envn][i] = er[i];
		}
		else	mb_len = wcstombs(mb_env[envn],*e,ARG_MAX);
		mb_env[envn++][mb_len] = 0;
		++e;
	}
	mb_env[envn] = (char *)0;
	execve(mb_path,mb_argv,mb_env);
}
wchar_t *
ksh_gettxt(const char *s1,const char *s2)
{
	wchar_t wc_txt[LINE_MAX];
	char *mb = gettxt(s1,s2);
	int cnt = mbstowcs(wc_txt,mb,LINE_MAX);
	wc_txt[cnt] = 0L;
	return(wc_txt);
}
/* ksh_write()
 *
 *	Treat bad conversions (-1) and null (0) returns from 
 *	wctomb() as raw data. This handles the history mechanism
 *	H_UNDO(0201), H_CMDNO(0202), NULL bytes.
 */

size_t
ksh_write(int fildes, wchar_t *buf, size_t count)
{
	char *mb;
	wchar_t *wc = buf;
	int mb_cnt;
	size_t tot = 0;
	size_t ret;
	int wc_cnt = count;
	char *mb_base = (char *)0;
	mb = mb_base = (char *)malloc(count*MB_CUR_MAX+1);
	if(mb) {
		while(wc_cnt > 0){
			switch(mb_cnt = wctomb(mb,*wc)){
				case  0:
				case -1:
					*mb++ = *wc++;
					tot += 1; break;
				default:
					mb += mb_cnt;
					tot += mb_cnt;
					++wc;
					break;
			}
			--wc_cnt;
		}
		ret=write(fildes,mb_base,tot);
		free(mb_base);
		return(ret);
	}
	return(-1);
}
/* ksh_read()
 *
 *	If the count is greater than the current MB_CUR_MAX,
 *	then subtract MB_CUR_MAX from the total count to
 *	insure we have room to read MB_CUR_MAX characters
 *	in case the read ends in the middle of a multibyte
 *	sequence.
 *
 *	Errors in conversions are treated as raw data and passed through.
 *
 *	Reads from the history file (FCIO descriptor) are checked
 *	for multibyte sequences and padded with spaces before the
 *	newline to account for discrepancy between offsets in the
 *	history file and offsets in the internal wide-character
 *	representation. These are stripped later.
 */
size_t 
ksh_read(int fno, wchar_t *wc_buf, size_t count)
{
	unsigned char mb_read[IOBSIZE+1];
	int ret,tot_read,n;
	register int w=0;
	register int m=0;
	int hist_flag = 0;
	int more_flag;
	int i;
	int hist_pad = 0;

	if(fno == FCIO)
		hist_flag = 1;
	if(MB_CUR_MAX == 1){
		n = count;
		hist_flag = 0;
	}
	else {
		n = (count>MB_CUR_MAX)?(count-MB_CUR_MAX):count;
	}
	if((tot_read = read(fno,mb_read, (unsigned)n)) > 0) {
		mb_read[tot_read] = 0;
		while(m < tot_read) {
			more_flag = 0;
convert:
			switch(ret=mbtowc(wc_buf+w,(char *)mb_read+m,MB_CUR_MAX)) {
			case 0:
				*(wc_buf+w) = 0L;
				++w;
				++m;
				hist_pad = 0;
				break;
			case -1:
			{
				int more;
					/* Bad conversion or need more input */
				if(!more_flag && (m+MB_CUR_MAX) > tot_read)
				{
					more = (m+MB_CUR_MAX) - tot_read;
					++more_flag;
					if((more = read(fno,mb_read+tot_read,more)) > 0)
					{
						tot_read += more;
						goto convert;
					}
				}
				wc_buf[w++] = (wchar_t)((unsigned char)mb_read[m++]);
				break;
			}
			default:
				m += ret;
				if(fno == FCIO){
					/* KLUDGE
					 * When reading history file, pad spaces before the
					 * newline to account for discrepancy between the
					 * multibyte count and the wide character count offsets.
					 */
					if(ret > 1)
						hist_pad += (ret-1);
					if(*(wc_buf+w) == L'\n'){
						while(hist_pad){
							*(wc_buf+w) = L' ';
							++w;
							--hist_pad;
						}
						*(wc_buf+w) = L'\n';
					}
				}
				++w;
				break;
			}
		}
		n = w;
	}
	else n = tot_read;
	return(n);
}

