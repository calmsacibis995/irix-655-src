/*
 * Copyright (c) 1988 Mark Nudleman
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Operating system dependent routines.
 *
 * Most of the stuff in here is based on Unix, but an attempt
 * has been made to make things work on other operating systems.
 * This will sometimes result in a loss of functionality, unless
 * someone rewrites code specifically for the new operating system.
 *
 * The makefile provides defines to decide whether various
 * Unix features are present.
 */

#include "less.h"		/* _MB_BAD_BYTE */
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <signal.h>
#include <setjmp.h>
#include "pathnames.h"
#include <libgen.h>
#include <limits.h>
#include <alloca.h>

#include <wchar.h>
int reading;
int nCurFileSize;		/* Stores size of the current file in bytes */
int nFlag;			/* Flag used to get file size quickly */ 

static jmp_buf read_label;

/*
 * Pass the specified command to a shell to be executed.
 * Like plain "system()", but handles resetting terminal modes, etc.
 */
void
lsystem(char *cmd)
{
	int inp;
	char *cmdbuf;
	char *shell;

	/*
	 * Print the command which is to be executed,
	 * unless the command starts with a "-".
	 */
	if (cmd[0] == '-')
		cmd++;
	else
	{
		lower_left();
		clear_eol();
		putstr("!");
		putstr(cmd);
		putstr("\n");
	}

	/*
	 * De-initialize the terminal and take out of raw mode.
	 */
	deinit();
	flush();
	raw_mode(0);

	/*
	 * Restore signals to their defaults.
	 */
	init_signals(0);

	/*
	 * Force standard input to be the terminal, "/dev/tty",
	 * even if less's standard input is coming from a pipe.
	 */
	inp = dup(0);
	(void)close(0);
	if (open(_PATH_TTY, O_RDONLY, 0) < 0)
		(void)dup(inp);

	/*
	 * Pass the command to the system to be executed.
	 * If we have a SHELL environment variable, use
	 * <$SHELL -c "command"> instead of just <command>.
	 * If the command is empty, just invoke a shell.
	 */
	if ((shell = getenv("SHELL")) != NULL && *shell != '\0')
	{
		if (*cmd == '\0')
			cmd = shell;
		else
		{
			/* If you input a command by ! command, the length 
			   of the command (cmd) is CMDBUFLEN at most.
			   But when you set a long path to EDITOR for example,
			   length of cmd becomes long. 
			*/
                        cmdbuf = (char *)alloca( strlen(shell) + strlen(cmd) + 32 );

			(void)sprintf(cmdbuf, "%s -c \"%s\"", shell, cmd);
			cmd = cmdbuf;
		}
	}
	if (*cmd == '\0')
		cmd = "sh";

	(void)system(cmd);

	/*
	 * Restore standard input, reset signals, raw mode, etc.
	 */
	(void)close(0);
	(void)dup(inp);
	(void)close(inp);

	init_signals(1);
	raw_mode(1);
	init();
	screen_trashed = 1;
#if defined(SIGWINCH) || defined(SIGWIND)
	/*
	 * Since we were ignoring window change signals while we executed
	 * the system command, we must assume the window changed.
	 */
	winchsig();
#endif
}

/*
 * Like read() system call, but is deliberately interruptable.
 * A call to intread() from a signal handler will interrupt
 * any pending iread().
 */
int
iread(int fd, char *buf, int len)
{
	register int n;

	if (setjmp(read_label))
		/*
		 * We jumped here from intread.
		 */
		return (READ_INTR);

	flush();
	reading = 1;
	n = read(fd, buf, len);
	reading = 0;
	if (n < 0)
		return (-1);
	return (n);
}




off64_t
mblseek64(int fd, off64_t off,  int nFmWhere)
{
	off64_t bytes_read, 
		bytes = 0, 
		chars = 0;
	off64_t	ln;
 static off64_t nLength; 
	
	register wchar_t *wTmpbuf   = NULL;
	register char    *tmpbufptr = NULL;
	register char    *tmpbuf    = NULL;

	/* used to seek at the end of the file */
	if (!ispipe && (int) off == 0 && nFmWhere == SEEK_END) {
		struct stat statBuf;

		if (nFlag == 1) {
			return nLength;
		}
		nFlag = 1;

		fstat( fd, &statBuf );
	
		nLength	     = mblseek64( fd, statBuf.st_size, SEEK_SET );
		nCurFileSize = statBuf.st_size;
		return         nLength;
	}
	if(ispipe)
		goto StdIn;

	tmpbuf = (char *)malloc( off * MB_CUR_MAX + 1 ); 
	if ( tmpbuf == NULL ) {
		error(GETTXT(_MSG_MORE_CANT_ALLOC_MEM));
		exit(1);
	}
	memset( tmpbuf, 0, off * MB_CUR_MAX + 1 );

	if ( lseek64(fd, 0, SEEK_SET) < 0 ) {
		free( tmpbuf );
		return -1;
	}

	/* The argument: off means the number of characters (offset) and this function 
	   returns the number of characters. In case of searching end of the file, 'off' 
	   has the size of the file (bytes), but it still works. The bytes_read will be 
	   smaller value than 'off * MB_CUR_MAX'.
	*/
	bytes_read = iread( fd, tmpbuf, off * MB_CUR_MAX ); 

	if ( bytes_read <= 0 ) {
		free( tmpbuf );
		return ( bytes_read == 0 ) ? 0 : -1;
	}

	tmpbufptr = tmpbuf;

	while( chars < off && bytes < bytes_read ) {

        	if ( (ln = mblen( tmpbufptr, MB_CUR_MAX )) < 0 ) {

#ifdef _MB_BAD_BYTE
			chars ++;
#endif
                        bytes     ++;
                        tmpbufptr ++;
		}
		else {
                        chars     ++;
                        bytes     += (ln == 0)? 1 : ln; 
			tmpbufptr += (ln == 0)? 1 : ln;
		}
	}

	free( tmpbuf ); tmpbuf = NULL;

	if ( lseek64( fd, bytes, SEEK_SET ) < 0 )
		return -1;

	return chars;


StdIn:
	wTmpbuf = (wchar_t *)malloc( off * MB_CUR_MAX + 1 ); 

	if ( wTmpbuf == NULL ) {
		error(GETTXT(_MSG_MORE_CANT_ALLOC_MEM));
		exit(1);
	}
	memset( wTmpbuf, 0, off * MB_CUR_MAX + 1 );
	
	if ( lseek64(fd, 0, SEEK_SET) < 0 ) {
		free( wTmpbuf );
		return -1;
	}

	bytes_read = wiread( fd, wTmpbuf, off ); 

	free( wTmpbuf ); wTmpbuf = NULL;

	if ( bytes_read == READ_INTR )
		return ((wchar_t)EOI);
	if ( bytes_read < 0 )
		return -1;

	return 0;
}



int
wiread(int fd, wchar_t *wbuf, off64_t len)
{
	off64_t  bytes = 0,     bytes_read = 0;
	off64_t  ln    = 0;
        int      chars = 0,     mb_len     = 0;
	int	 i     = 0,     next       = 0;
	char 	*tmpbuf,       *sttmpbuf;
	char 	*base;
	int	 bad;

	
	if (setjmp(read_label))
		/*
		 * We jumped here from intread.
		 */
		return READ_INTR;

	flush();

	if (fd < 3) 		/* stdin */
		goto StdIn;


	sttmpbuf = tmpbuf = (char *)malloc( len * MB_CUR_MAX + 1 );

	if (tmpbuf == NULL) {
		error(GETTXT(_MSG_MORE_CANT_ALLOC_MEM));
		exit(1);
	}
        memset( tmpbuf, 0, len * MB_CUR_MAX + 1 );

	reading = 1;

	bytes_read = read( fd, (char *)tmpbuf, len * MB_CUR_MAX ); 

        if ( bytes_read <= 0 ) {
		reading = 0;
	  	free( sttmpbuf );
 	  	return (bytes_read < 0) ? -1 : 0;
	}


        while(  chars < len  &&  bytes < bytes_read ) {

		if ( (ln = mbtowc( wbuf + chars, tmpbuf, MB_CUR_MAX )) < 0 ) {


#ifdef _MB_BAD_BYTE
                        *(wbuf + chars) = (wchar_t)(*tmpbuf);
                        chars  ++;
#endif

			bytes  ++;
			tmpbuf ++;
		}
		else {
			if ( ln == 0 )
				*(wbuf + chars) = (wchar_t)0;
			chars  ++;
			bytes  += (ln == 0)? 1 : ln;
			tmpbuf += (ln == 0)? 1 : ln;
		}
	}

	ln = lseek64( fd, bytes - bytes_read, SEEK_CUR );

        reading = 0;
        free( sttmpbuf );

	if ( ln < 0 )
		return -1;

	return( chars );


StdIn:
	tmpbuf = (char *)malloc( len * MB_CUR_MAX + 1 ); 
	if ( tmpbuf == NULL ) {
		error( GETTXT(_MSG_MORE_CANT_ALLOC_MEM) );
		exit(1);
	}
	memset( tmpbuf, 0, len * MB_CUR_MAX + 1 );

	reading = 1;
	base    = tmpbuf;
	next    = 0;
	mb_len  = 1;

        while ( chars < len  &&  base - tmpbuf < len * MB_CUR_MAX ) {

		bytes_read = read( fd, tmpbuf + next, 1 );

		if ( bytes_read <= 0 ) {
			reading = 0;
			free( tmpbuf );
			return (bytes_read < 0) ? -1 : chars;
		}

                for ( i = 0; i < mb_len; i++ ) {

                        if ( (ln = mbtowc( wbuf + chars, base + i, mb_len - i )) < 0 ) {

                                if ( i >= MB_CUR_MAX - 1 ) {
#ifdef _MB_BAD_BYTE
					for ( bad=0; bad < MB_CUR_MAX && chars < len; bad++ ) {
						*(wbuf + chars) = (wchar_t)(*(base + bad));
						chars ++;
					}
					if ( bad != MB_CUR_MAX ) {
                                        /* 
                                           some bytes that should be processed
                                           are still in the buffer ...         
                                        */
					}
#endif

                                        base  += MB_CUR_MAX;
                                        mb_len = 1;
                                        break;                                          
                                }
                                if ( i >= mb_len - 1  &&  mb_len < MB_CUR_MAX ) {
                                        mb_len ++;
                                        break;
                                }
                        }
                        else {
#ifdef _MB_BAD_BYTE
				for ( bad=0; bad < i && chars < len; bad++ ) {
					*(wbuf + chars) = (wchar_t)(*(base + bad));
					chars ++;
				}
				if ( bad != i || chars > len - 1 ) {
                                        /* 
				 	   some bytes that should be processed 
				  	   are still in the buffer ...         
					*/
				} 
#endif
	
                                if ( ln == 0 )
                                        *(wbuf + chars) = (wchar_t)0;
                                chars  ++;
                                base   += (ln == 0)? 1 : ln + i;
                                mb_len = 1;
                                break;
                        }

			continue;
                }

		next ++;
        }

	reading = 0;

        free( tmpbuf );
        return chars;
}



void
intread()
{
	(void)sigsetmask(0L);
	longjmp(read_label, 1);
}

/*
 * Expand a filename, substituting any environment variables, etc.
 * The implementation of this is necessarily very operating system
 * dependent.  This implementation is unabashedly only for Unix systems.
 */
FILE *popen();

char *
uxglob(char *filename)
{
	FILE *f;
	char *p;
	int ch;
	char *cmd;
	static char buffer[ PATH_MAX ];

	if (filename[0] == '#')
		return (filename);

	/*
	 * We get the shell to expand the filename for us by passing
	 * an "echo" command to the shell and reading its output.
	 */
	p = getenv("SHELL");
	if (p == NULL || *p == '\0')
	{
		/*
		 * Read the output of <echo filename>.
		 */
		cmd = malloc((u_int)(strlen(filename)+8));
		if (cmd == NULL)
			return (filename);
		(void)sprintf(cmd, "echo \"%s\"", filename);
	} else
	{
		/*
		 * Read the output of <$SHELL -c "echo filename">.
		 */
		cmd = malloc((u_int)(strlen(p)+strlen(filename)+12));
		if (cmd == NULL)
			return (filename);
		(void)sprintf(cmd, "%s -c \"echo %s\"", p, filename);
	}

	f = popen(cmd, "r");
	free(cmd);
	if (f == NULL)
		return (filename);

	for (p = buffer;  p < &buffer[sizeof(buffer)-1];  p++)
	{
		if ((ch = getc(f)) == '\n' || ch == EOF)
			break;
		*p = ch;
	}
	*p = '\0';
	(void)pclose(f);
	return(buffer);
}

#if 0
/*
 * Copy a string, truncating to the specified length if necessary.
 * Unlike strncpy(), the resulting string is guaranteed to be null-terminated.
 *
 */
void
strtcpy(char *to, char *from, int len)
{
        int len_from;

        if ( len <= 0 ) return;
        len_from = strlen( from );

        (void)strncpy(to, from, len);

        if ( len_from < len )
                to[ len_from ] = '\0';
        else    
                to[ len - 1  ] = '\0';
}
#endif

char *
bad_file(char *filename, char *message, u_int len)
{
	struct stat statbuf;

	if (stat(filename, &statbuf) < 0) {

		if ( (strlen(filename) + strlen(strerror(errno)) + 32) < len )
                        (void)sprintf( message, GETTXT(_MSG_MORE_FILE_ACCESS_ERR), filename, strerror(errno) );
		else
                	/* Assuming enough length for message area practically, 
                   	   no long filename(basename) and no long error message. 
                	*/
			(void)sprintf( message, GETTXT(_MSG_MORE_FILE_ACCESS_ERR), basename(filename), strerror(errno)  );

		return(message);
	}

	if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
		char *is_dir = GETTXT(_MSG_MORE_A_DIR);

		if ( (strlen(filename) + strlen(is_dir) + 32) < len )
			(void)sprintf( message, "%s%s", filename, is_dir );
		else
			(void)sprintf( message, "%s%s", basename(filename), is_dir );

		return(message);
	}

        if ((statbuf.st_mode & S_IFMT) == S_IFIFO)
                strcpy(message, "S_IFIFO");

	return((char *)NULL);
}
