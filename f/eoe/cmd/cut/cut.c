/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cut:cut.c	1.11.1.1"
#
/* cut : cut and paste columns of a table (projection of a relation) */
/* Release 1.5; handles single backspaces as produced by nroff    */
# include <stdlib.h>
# include <stdio.h>	/* make: cc cut.c */
# include <ctype.h>
# include <locale.h>
# include <pfmt.h>
# include <errno.h>
# include <string.h>
# include <bstring.h>
# include <limits.h>
# include <wchar.h>

# define BUFSIZE 1024	/* max no of fields or resulting line length */
# define BACKSPACE '\b'

#define MLTDUMB ' '
#include <sys/euc.h>

#include <widec.h>
#include <i18n_capable.h>
#include <msgs/uxcore.h>

void
usage(int complain)
{
	if (complain)
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_INCORRECT_USAGE));
	pfmt(stderr, MM_ACTION,
		PFMTTXT(_MSG_CUT_USAGE));
	exit(2);
}

void diag(char *s);
int selcheck(void);
void bytesplit(FILE *fp);
void charsplit(FILE *fp);
void fieldsplit(FILE *fp, char *del);
void dump(int nl, int chwidth);
int getmb(char *buf, FILE *fp);

int state;
int maxnum;
int low;
char	*sel;
int split_chars;
int supflag;

#define STATE_START 0
#define STATE_HASNUM 1
#define	STATE_INRANGE 2
#define	STATE_HASRANGE 3

#define MID_VALUE 1
#define TERM_VALUE 2

int tty;

static char    cflist[] = PFMTTXT(_MSG_CUT_BAD_LIST);
static char long_line[] = PFMTTXT(_MSG_LINE_TOO_LONG);

char *
liststate(char *p)
{
	int i;
	char c = *p;
	int num = 0;
	enum {TERM, RANGE, DIGIT} type;

	if (isdigit(c)) {
		num = atoi(p);
		type = DIGIT;
		while (isdigit(*p))
			p++;
	} else if (c == '\t' || c == ' ' || c == ',' || c == '\0') {
		type = TERM;
		p++;
	} else if (c == '-') {
		type = RANGE;
		p++;
	} else {
		diag(cflist);
		/* NOTREACHED */
	}

	switch (state) {
	case STATE_START:
		if (type == TERM)
			diag(cflist);
		if (type == DIGIT) {
			low = num;
			state = STATE_HASNUM;
		} else {
			low = 1;
			state = STATE_INRANGE;
		}
		break;
	case STATE_HASNUM:
		if (type == DIGIT)
			diag(cflist);
		if (type == TERM) {
			sel[low] = TERM_VALUE;
			state = STATE_START;
			low = 0;
		} else {
			state = STATE_INRANGE;
		}
		break;
	case STATE_INRANGE:
		if (type == RANGE)
			diag(cflist);
		if (type == TERM) {
			for (i = low; i <= maxnum; i++)
				sel[i] = MID_VALUE;
		} else {
			for (i = low; i < num; i++)
				sel[i] = MID_VALUE;
			sel[num] = TERM_VALUE;
		}
		low = 0;
		state = STATE_HASRANGE;
		break;
	case STATE_HASRANGE:
		if (type == RANGE || type == DIGIT)
			diag(cflist);
		state = STATE_START;
		break;
	}
	return p;
}

int
selcheck()
{
	int found = 0;
	char *cur = &sel[1], *last = &sel[maxnum + 1];

	while (cur < last) {
		while (cur < last)
			if (*cur++) {
				found = 1;
				break;
			}
		while (cur < last) {
			if (*cur == MID_VALUE) {
				cur++;
			} else if (*cur == 0) {
				cur++;
			} else if (*(cur + 1) != 0) {
				*cur++ = MID_VALUE;
			} else
				cur++;
		}
	}
	return found;
}

main(int argc, char **argv)
{
	extern int	optind;
	extern char	*optarg;
	register int	c;
	register char	*p, *list;
	/* permits multibyte delimiter */
	char	*del;
	int	bflag, cflag, fflag, filenr;
	FILE	*inptr;
	int delw = 1;
	int exitval = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:cut");

	del = "\t";

	supflag = bflag = cflag = fflag = 0;
	split_chars = 1;
	if (isatty(1)) {
		setvbuf(stdout, malloc(BUFSIZE), _IOFBF, BUFSIZE);
		tty = 1;
	}

	while((c = getopt(argc, argv, "b:c:d:f:ns")) != EOF)
		switch(c) {
			case 'b':
				bflag = 1;
				list = optarg;
				break;
			case 'c':
				cflag = 1;
				list = optarg;
				break;
			case 'd':
			/* permits multibyte delimiter 	*/
				if ( I18N_SBCS_CODE )
					delw = 1;
				else
					if ( (delw = mblen(optarg, MB_CUR_MAX )) <= 0 )
						diag(PFMTTXT(_MSG_NO_DELIMITER));

				if ((int)strlen(optarg) > delw)
					diag(PFMTTXT(_MSG_NO_DELIMITER));
				else
					del = optarg;
				break;
			case 'f':
				fflag = 1;
				list = optarg;
				break;
			case 'n':
				split_chars = 0;
				break;
			case 's':
				supflag++;
				break;
			case '?':
				usage(0);
		}

	argv = &argv[optind];
	argc -= optind;

	if (bflag + cflag + fflag > 1)
		usage(1);
	if (bflag + cflag + fflag == 0)
		usage(1);
	if (!split_chars && bflag == 0)
		usage(1);
	
	for (p = list, maxnum = 0; *p != '\0'; ) {
		if (isdigit(*p)) {
			int tmp = atoi(p);
			if (tmp > maxnum) maxnum = tmp;
			while (isdigit(*p))
				p++;
		} else
			p++;
	}

	sel = malloc(maxnum + 2);
	bzero(sel, maxnum + 2);

	for (p = list; *p != '\0'; )
		p = liststate(p);
	liststate(p);

	if (!selcheck())
		diag(PFMTTXT(_MSG_NO_FIELDS));

	filenr = 0;
	do {	/* for all input files */
		if ( argc == 0 || strcmp(argv[filenr],"-") == 0 )
			inptr = stdin;
		else
			if ((inptr = fopen(argv[filenr], "r")) == NULL) {
				pfmt(stderr, MM_WARNING,
					PFMTTXT(_MSG_CANNOT_OPEN),
					argv[filenr], strerror(errno));
				exitval = 1;
				continue;
			}

		if (bflag)
			bytesplit(inptr);
		else if (cflag)
			charsplit(inptr);
		else
			fieldsplit(inptr, del);
		fclose(inptr);
	} while (++filenr < argc);

	return exitval;
}

void
bytesplit(FILE *fp)
{
	for (;;) {	/* for all lines of a file */
		int bytecnt = 0;
		int c;
		int len;

		for (;;) {
			char mcbuf[ MB_CUR_MAX + 1 ];

			c = 0;

                        if ( I18N_SBCS_CODE || split_chars ) {
			    c = getc(fp);
			    bytecnt++;
                        }
                        else {

			    len = getmb( mcbuf, fp );

			    if ( len == EOF ) {
				bytecnt ++;		/* count it */  
				c = EOF;
			    }
                            else 
			    if ( len == 1 && mcbuf[0] == '\n' ) {
				bytecnt ++;
			        c = '\n';
			    }
			    else
			    if ( len == 0 ) {		/* 0 or invalid byte */
#ifndef _IGNORE_BAD_BYTE
				bytecnt ++;
#else
				mcbuf[0] = '\0';
#endif /* if not defined */
			    }
			    else 
			    if ( len > 0 ) 
				bytecnt += len;
			    else			/* not happen */
				mcbuf[0] = '\0';
                        }

			if (c == EOF) {
				if (bytecnt != 1) {
					putchar('\n');
					if (tty)
						fflush(stdout);
				}
				return;
			} else if (c == '\n') {
				putchar(c);
				if (tty)
					fflush(stdout);
				break;
			}

			if (I18N_SBCS_CODE || split_chars) {
				if (bytecnt <= maxnum ? sel[bytecnt]
					: sel[maxnum] == MID_VALUE)
					putchar(c);
				continue;
			}
			else {
				if (bytecnt <= maxnum) {
			      		if (sel[bytecnt]) {
#ifndef _IGNORE_BAD_BYTE 
						if ( mcbuf[0] == '\0' )
							putchar(0);
						else
#endif /* if not defined */
							fputs(mcbuf, stdout);
					}
				} 
			        else 
			        if (sel[maxnum] == MID_VALUE) {
#ifndef _IGNORE_BAD_BYTE 
                                                if ( mcbuf[0] == '\0' )
                                                        putchar(0);
                                                else
#endif /* if not defined */
							fputs(mcbuf, stdout);
			        } 
			}
		}
	}
}

static int cnt;
static int cap;
static char *buf;

void
save(int c)
{
	if (c == EOF)
		return;
	if (cnt == cap) {
		if (cap == 0)
			cap = 1024;
		cap *= 2;
		buf = realloc(buf, cap);
		if (!buf)
			diag(long_line);
	}
	buf[cnt++] = (char)c;
}

void
dump(nl,chwidth)
{
	fwrite(buf, nl ? cnt : cnt - chwidth, 1, stdout);
	if (nl && cnt > 0 && buf[cnt - 1] != '\n')
		putchar('\n');
	if (tty)
		fflush(stdout);
	cnt = 0;
}

void
newline()
{
	cnt = 0;
}

void
fieldsplit(FILE *fp, char *del)
{
	for (;;) {	/* for all lines of a file */
		int fieldcnt = 1;
		int del_found = 0;
		int no_fields_printed = 1;

		newline();

		for (;;) {
			int i;
			char  mcbuf[ MB_CUR_MAX + 1 ];
			int c, len;

			c = 0;

                        if( I18N_SBCS_CODE ) {
			    c = getc(fp);
			    len = 1;
			    mcbuf[0] = (char)c;
			    mcbuf[1] = '\0';
			    if (fieldcnt == 1)
				    save(c);
                        }
                        else {

                            len = getmb( mcbuf, fp );

                            if ( len == EOF ) {
                                c = EOF;
				len = 1;			/* doesn't effect */
			    }
                            else
                            if ( len == 1 && mcbuf[0] == '\n' ) {
                                c = '\n';
                                if ( fieldcnt == 1 )
                                	save( c );
			    }
                            else
                            if ( len == 0 ) {			/* 0 or invalid byte */
#ifndef _IGNORE_BAD_BYTE
				if ( fieldcnt == 1 )
					save( mcbuf[ 0 ] );
				len = 1;
#else
                                mcbuf[0] = '\0';
#endif /* if not defined */
                            }
			    else
			    if ( len > 0 ) {
				if ( fieldcnt == 1 )
			    		for ( i=0; i < len; save( mcbuf[i++] ) ) ;
			  			mcbuf[len] = '\0';
                            }
			    else				/* not happen */ 
				mcbuf[0] = '\0';
                        }

			if (c == EOF) {
				if (fieldcnt == 1 && !supflag)
					dump(1,1);
				return;
			} else if (c == '\n') {
				if (fieldcnt > 1) {
					putchar(c);
					if (tty)
						fflush(stdout);
				} else if (!supflag) {
					dump(1,1);
				}
				break;
			}

			if (!strcmp(mcbuf, del))
				fieldcnt++;
			if (fieldcnt == 1)
				continue;
			if (!del_found) {
				if (sel[1]) {
					no_fields_printed = 0;
					dump(0,len);
				}
				del_found = 1;
			}
			if (fieldcnt <= maxnum
				? sel[fieldcnt] : sel[maxnum] == MID_VALUE)
			{
				if (no_fields_printed)
					no_fields_printed = 0;
				else {
#ifndef _IGNORE_BAD_BYTE
                                        if ( mcbuf[0] == '\0' )
                                                putchar(0);
                                        else
#endif /* if not defined */
						fputs(mcbuf, stdout);
				}
			}
		}
	}
}

void
charsplit(FILE *fp)
{
	for (;;) {	/* for all lines of a file */
		int charcnt = 0;
		int c, len;

		for (;;) {
			int i;
			char  mcbuf[ MB_CUR_MAX + 1 ];

			c = 0;

			if ( I18N_SBCS_CODE ) {
				len = 1;
				c = getc(fp);
				if ( c == EOF || ISASCII(c) || isprint(c) )
					charcnt++;
                        }
			else {
				len = getmb( mcbuf, fp );

				charcnt++;

                        	if ( len == EOF )
                        		c = EOF;		/* counted for charcnt */
                        	else
                        	if ( len == 1 && mcbuf[0] == '\n' )
                        		c = '\n';
                        	else 
				if ( len == 0 ) {		/* 0 or invalid byte */
#ifdef _KEEP_BAD_BYTE
					len = 1;
#else
                                	mcbuf[0] = '\0';
					charcnt--;
#endif /* if defined */
				}
				else
                        	if ( len > 0 ) 
		                    mcbuf[ len ] = '\0';
                                else				/* not happen */
				    mcbuf[0] = '\0';
                        }

			if (c == EOF) {
				if (charcnt != 1) {
					putchar('\n');
					if (tty)
						fflush(stdout);
				}
				return;
			} else if (c == '\n') {
				putchar(c);
				if (tty)
					fflush(stdout);
				break;
			}

			if (I18N_SBCS_CODE) {
				if (charcnt <= maxnum ? sel[charcnt]
					: sel[maxnum] == MID_VALUE)
									/* compatibility */
				    	if ( ISASCII(c) || isprint(c) )
						putchar(c);
			} 
			else {
			    	if (charcnt <= maxnum ? sel[charcnt]
			                : sel[maxnum] == MID_VALUE)
				{
#ifdef _KEEP_BAD_BYTE
                                	if ( mcbuf[0] == '\0' )
                                        	putchar(0);
                                        else
#endif /* if defined */
						fputs(mcbuf, stdout);
				}
                        }
		}
	}
}


void
diag(char *s)
{
	pfmt(stderr, MM_ERROR, s);
	exit(2);
}



/*  getmb() - gets a multibyte character from a byte stream. 
 *  Return: 
 *          -1: EOF.
 *           0: 0 value or an invalid byte. It is contained in buf[0].
 *		buf[1] contains a null-terminate.
 *    Positive: The number of bytes of the multi-byte character.
 *              the buf contains the bytes with a null-terminate.
 */
int
getmb( char *buf, FILE *fp )
{
    register int i, c;
    int len, eof;

    /* The length of buf should be more than MB_CUR_MAX. */

    if ( buf == NULL ) return EOF;

    for ( eof=0,buf[0]='\0',i=0; i<MB_CUR_MAX; i++ ) {

        if ( (c = getc( fp )) == EOF ) {
                if ( i==0 && buf[0]=='\0' )
                        return EOF;
                i--;
                eof = 1;
                goto eof;
        }

        buf[ i ] = c;

        if ( (len = mblen( buf, i+1 )) > 0 ) {

                buf[ len ] = '\0';
                return len;
        }

        if ( len == 0 )         /* it happens when the the first byte */ 
                return 0;       /* of buf is 0, that is null string.  */
eof:
        if ( i >= MB_CUR_MAX-1  || eof ) {
                while ( i > 0 )
                        ungetc( buf[ i-- ], fp );
                buf[ i+1 ] = '\0';
                return 0;
        }
    }

    return 0;
}


