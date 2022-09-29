/*
 * Copyright 1995, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON
 * GRAPHICS, INC.
 */
/*
 * fold.c
 *
 */
#ident "$Revision: 1.4 $"
/* Modified to support EUC Multibyte/Big5-Sjis/Full multibyte */

#include        <stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include        <locale.h>
#include        <pfmt.h>
#include        <msgs/uxcore.abi.h>
#include        <msgs/uxsgicore.h>
#include	<errno.h>
#include	<ctype.h>
#include	<string.h>
#include        <libw.h>
#include        <wchar.h>
#include        <limits.h>

#define IDENTICAL(A,B)  (A.st_dev==B.st_dev && A.st_ino==B.st_ino)
#define MAX_ZERO_WR     5

int DEBUG = 0;
#define PRINTD if (DEBUG) printf
int width = 80;
int pick_width = 0;
int break_at_space = 0;
int use_bytes = 0;
int errnbr = 0;
int S_FLAG = 0;
int old_fold = 0;
int new_fold = 0;
int old_width = 80;
int peekchar = 0;
FILE *fi;

void flush_at_space (char *this_line, int last_space, int *buf_chars, int *line_chars);
void flush_line(char *line, int len);
void readerr(char *filename);
void putch(int c);
void foldungetc(int c);
int foldgetc(void);

main(int argc, char **argv)
{
    register int c;
    extern int optind;
    extern char *optarg;
    int i, count;
    int errflg = 0;
    int	stdinflg = 0;
    struct stat64 source, target;
    char *widthstring;
    int status = 0;

    (void)setlocale(LC_ALL, "");
    (void)setcat("uxcore.abi");
    (void)setlabel("UX:fold");

/*
  for (i = 0; i < argc; i++) 
  printf ("opt %d: %s\n", i, argv[i]);
*/
    while( (c=getopt(argc,argv,"0123456789dsbw:")) != EOF ) {
	switch(c) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':	
	case '8':
	case '9':
	    if (new_fold) { 
		pfmt(stderr, MM_ACTION, PFMTTXT(_MSG_FOLD_USAGE));
		exit(2);
	    }
	    if (!old_fold)
		old_width = 0;
	    old_fold = 1;
	    old_width *= 10;
	    old_width += c - '0';
	    continue; 
	case 'd':
	    DEBUG = 1;
	    continue;
	case 'w':
	    if (old_fold) {
		pfmt(stderr, MM_ACTION, PFMTTXT(_MSG_FOLD_USAGE));
		exit(2);
	    }
	    new_fold = 1;
	    pick_width = 1;
	    widthstring = optarg;
	    if (!isdigit(optarg[0])) 
	      errflg++;
	    width = atoi(widthstring);
	    continue;
	case 'b':
	    if (old_fold) {
		pfmt(stderr, MM_ACTION, PFMTTXT(_MSG_FOLD_USAGE));
		exit(2);	
	    }	    
	    use_bytes = 1;
	    new_fold = 1;
	    continue;
	case 's':
	    if (old_fold) {
		pfmt(stderr, MM_ACTION, PFMTTXT(_MSG_FOLD_USAGE));
		exit(2);	
	    }	    new_fold = 1;
	    break_at_space++;
	    continue;
	case '?':
	    errflg++;
	    break;   
	}
    }

    if (errflg) {
	pfmt(stderr, MM_ACTION, PFMTTXT(_MSG_FOLD_USAGE));
	exit(2);
    }
    
    if (old_fold) {
	width = old_width;
	if (!width) {
	    pfmt(stderr, MM_ACTION, PFMTTXT(_MSG_FOLD_USAGE));
	    exit(2);   
	}
	argc = argc - optind;
	argv = &argv[optind];
	do {
	    if (argc > 0) {
		if (freopen(argv[0], "r", stdin) == NULL) {
		    pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_CANNOT_OPEN),
			 argv[0], strerror(errno));
		    exit(1);
		}
		argc--, argv++;
	    }
	    for (;;) {
		c = getc(stdin);
		if (c == -1)
		    break;
		putch(c);
	    }
	} while (argc > 0);
	exit(0);
    }
    
    /*
     * Stat stdout to be sure it is defined.
     */
    if(fstat64(fileno(stdout), &target) < 0) {
	pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_CANNOT_ACCESS_STDOUT), strerror(errno));
	exit(2);
    } 
    	/*
	 * If no arguments given, then use stdin for input.
	 */

    if (optind == argc) {
	argc++;
	stdinflg++;
    }
    
    /*
     * Process each remaining argument,
     * unless there is an error with stdout.
     */
    
    for (argv = &argv[optind];
	 optind < argc && !ferror(stdout); optind++, argv++) {
	
	/*
	 * If the argument was '-' or there were no files
	 * specified, take the input from stdin.
	 */
	
		if (stdinflg
		 ||((*argv)[0]=='-' 
		 && (*argv)[1]=='\0'))
			fi = stdin;
		else {
			/*
			 * Attempt to open each specified file.
			 */

			if ((fi = fopen(*argv, "r")) == NULL) {
			    pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_CANNOT_OPEN),
				 *argv, strerror(errno));
			    exit(2);
			    PRINTD("Exit failed\n");
			}
		}
		
		/*
		 * Stat source to make sure it is defined.
		 */

		if(fstat64(fileno(fi), &source) < 0) {
		    pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_CANNOT_ACCESS),
			 *argv, strerror(errno));
		    status = 2;
		    continue;
		}

		/*
		 * If the source is not a character special file or a
		 * block special file, make sure it is not identical
		 * to the target.
		 */
	
		if (!S_ISCHR(target.st_mode)
		    && !S_ISBLK(target.st_mode)
		    && IDENTICAL(target, source)) {
		    pfmt(stderr, MM_ERROR,
			 PFMTTXT(_MSG_FILES_IDENTICAL),
			 stdinflg?"-": *argv);
		    if (fclose(fi) != 0 )
			pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_WRITE_ERROR), strerror(errno));
		    status = 2;
		    continue;
		}
		
		/*
		 * If in visible mode, use vcat; otherwise, use cat.
		 */
		if (break_at_space) {
		    status = fold_at_space(*argv);
		} else {
		    status = fold(*argv);
		}		
		/*
		 * If the input is not stdin, flush stdout.
		 */

		if (fi!=stdin) {
		    fflush(stdout);
		    
		    /* 
		     * Attempt to close the source file.
		     */
		    
		    if (fclose(fi) != 0) 
			pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_WRITE_ERROR), strerror(errno));
		}
    }
}

void flush_line(char *line, int len)
{
    int i;

    for(i = 0; i < len; i++)
	putchar(line[i]);
}

/* May need to use no_new_line here too*/
fold_at_space(filename)
	char *filename;
{
    register int c;
    int i;
    int line_chars = 0;
    int buf_chars = 0;
    char *this_line;
    int last_space = -1;
    int no_new_line = 0;

    this_line = (char *) calloc (width + MB_LEN_MAX, sizeof(char));
    while ((c = foldgetc()) != EOF)  {
	switch (c) {
	case '\t':
	    this_line[buf_chars++] = c;
	    line_chars = line_chars - (line_chars % 8) + 8;  
	    if ((!use_bytes && line_chars >= width + 1) ||
		    (use_bytes && buf_chars >= width + 1)) {
		flush_line(this_line, buf_chars-1);
		putchar('\n');
    		buf_chars = 0;
		this_line[buf_chars++] = '\t';
		last_space = 0;
		line_chars = 8;
	    } else {
		last_space = buf_chars - 1;
	    }
	    continue;
	case '\b':
	    if (line_chars == 0) {
		putchar('\b');
	    } else {
		this_line[buf_chars++] = c;
		line_chars++;
	        if ((!use_bytes && line_chars == width + 1) ||
		        (use_bytes && buf_chars == width + 1)) {
		    flush_at_space(this_line, last_space, &buf_chars, &line_chars);
		    last_space = -1;
		}
	    }
	    continue;
	case '\n':  
	    flush_line(this_line, buf_chars);
	    bzero(this_line, (width + 1) * sizeof(char));
	    putchar('\n');
	    buf_chars = 0;
	    line_chars = 0;
	    last_space = -1;
	    continue;
	case ' ':
	    PRINTD("Got space, last_space: %d\n", last_space);
	    this_line[buf_chars++] = c;
	    line_chars++;
	    if ((!use_bytes && line_chars == width + 1) ||
		    (use_bytes && buf_chars == width + 1)) {
		flush_at_space(this_line, last_space, &buf_chars, &line_chars);
		last_space = 0;
	    } else {
		last_space = buf_chars - 1;
	    }
	    continue;
	default:
	    {
		int length;
		wchar_t wchar;
		char multic[MB_LEN_MAX];
		char *mbp;

		foldungetc(c);
		length = mbftowc(multic, &wchar, foldgetc, &peekchar);
		if (length <= 0)
			length = 1;

	        if ((!use_bytes && (line_chars + scrwidth(wchar)) > width) ||
		    (use_bytes && (buf_chars + length) > width)) {
			if (last_space == -1) {
				flush_line(this_line, buf_chars);
				putchar('\n');
    				buf_chars = 0;
				line_chars = 0;
				last_space = -1;
			} else {
				flush_at_space(this_line, last_space, &buf_chars, &line_chars);
				last_space = -1;
			}
	    	}

		mbp = multic;
		while (length--)
			this_line[buf_chars++] = *mbp++;
		if (__iswblank(wchar))
			last_space = buf_chars - 1;
	    	line_chars += scrwidth(wchar);
	    }
	    continue;
	    
	}
    }
    return(0);
}

void flush_at_space (char *this_line, int last_space, int *buf_chars, int *line_chars)
{
    int length;
    wchar_t wchar;
    char multic[MB_LEN_MAX];
    char *mbp;
    int i;
    int bch = *buf_chars;

    if (last_space == -1) {
	for (i = 0; i < bch-1; i++)
	    putchar(this_line[i]);
	putchar('\n');
	this_line[0] = this_line[bch-1];
	*line_chars = 1;
	*buf_chars = 1;
    } else {
	PRINTD ("last_space: %d, line_chars: %d", last_space, *line_chars);
	for (i = 0; i < last_space + 1; i++)
	    putchar(this_line[i]);
	putchar('\n');
	*line_chars = 0;
	*buf_chars = 0;
	for (i = last_space + 1; i < bch; ) {
	    if (this_line[i] == '\t') {
		*line_chars += (8 - (*line_chars % 8));
		length = 1;
	    } else if (MB_CUR_MAX > 1) {
		length = mbtowc(&wchar, (char *)&(this_line[i]), MB_LEN_MAX);
		*line_chars += scrwidth(wchar);
	    } else {
	    	*line_chars += 1;
		length = 1;
	    }
	    while (length--)
		this_line[(*buf_chars)++] = this_line[i++];
	}
    }
}


fold(filename)
	char *filename;
{
    int line_chars = 0;
    int buf_chars = 0;
    register int c;
    int new_line_ok = 1;

	while ((c = foldgetc()) != EOF)  {
	    switch (c) {
	    case '\t':		
		putchar(c);
		line_chars = line_chars - (line_chars % 8) + 8;  
		buf_chars++;
	        if ((!use_bytes && line_chars >= width) ||
		        (use_bytes && buf_chars >= width)) {
		    putchar('\n');
		    new_line_ok = 0;
		    line_chars = 0;
		    buf_chars = 0;
		} else {
		    new_line_ok = 1;
		}
		continue;
	    case '\n':
		if (new_line_ok) {
		    putchar(c);
		    line_chars = 0;
		    buf_chars = 0;
		} else {
		    new_line_ok = 1;
		}
		continue;
	    case '\b':
		if (new_line_ok == 0) {
		    new_line_ok = 1;
		} else {
		    putchar(c);
		    if (line_chars != 0) { 
			line_chars++;
			buf_chars++;
	        	if ((!use_bytes && line_chars == width) ||
		        	(use_bytes && buf_chars == width)) {
			    putchar('\n');
			    line_chars = 0;
			    buf_chars = 0;
			    new_line_ok = 0;
			} 
		    } else {
			new_line_ok = 1;
		    }
		}
		continue;
	    default: 
		{
		    int length;
		    wchar_t wchar;
		    char multic[MB_LEN_MAX];
		    char *mbp;

		    foldungetc(c);
		    length = mbftowc(multic, &wchar, foldgetc, &peekchar);
		    if (length <= 0)
			    length = 1;
	            if ((!use_bytes && (line_chars + scrwidth(wchar)) > width) ||
		            (use_bytes && (buf_chars + length) > width)) {
		        putchar('\n');
		        line_chars = 0;
			buf_chars = 0;
		    }

		    line_chars += scrwidth(wchar);
		    buf_chars += length;
		    mbp = multic;
		    while (length--)
			putchar(*mbp++);
	            if ((!use_bytes && line_chars == width) ||
		            (use_bytes && buf_chars == width)) {
		        putchar('\n');
		        line_chars = 0;
			buf_chars = 0;
		        new_line_ok = 0;
		    } else {
		        new_line_ok = 1;
		    }
		}
		continue;
	    }
	}
    return(0);
}

void readerr(filename)
    char *filename;
{
    if(filename==(char *)NULL)
	pfmt(stderr, MM_ERROR, PFMTTXTCAT(_MSG_INPUT_ERROR),
	     strerror(errno), "stdin");
    else
	pfmt(stderr, MM_ERROR, PFMTTXTCAT(_MSG_INPUT_ERROR),
	     strerror(errno), filename);
}


int	col;
void putch(c)
	register c;
{
	register ncol;

	switch (c) {
		case '\n':
			ncol = 0;
			break;
		case '\t':
			ncol = (col + 8) &~ 7;
			break;
		case '\b':
			ncol = col ? col - 1 : 0;
			break;
		case '\r':
			ncol = 0;
			break;
		default:
			ncol = col + 1;
	}
	if (ncol > width)
		putchar('\n'), col = 0;
	putchar(c);
	switch (c) {
		case '\n':
			col = 0;
			break;
		case '\t':
			col += 8;
			col &= ~7;
			break;
		case '\b':
			if (col)
				col--;
			break;
		case '\r':
			col = 0;
			break;
		default:
			col++;
			break;
	}
}

int foldgetc()
{
	int	c;
	if (peekchar != 0) {
		c = peekchar;
		peekchar = 0;
	} else {
		c = getc(fi);
	}
	return(c);
}

void foldungetc(c)
int	c;
{
	if (peekchar != 0) {
		/* this shouldn't happen, but just in case ... */
		ungetc(peekchar, fi);
	}
	peekchar = c;
}

