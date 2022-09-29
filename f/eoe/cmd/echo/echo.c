/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)echo:echo.c	1.3.1.1"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <widec.h>
#include <locale.h>

main(argc, argv)
int argc;
char **argv;
{
  	register wchar_t *wCptr=NULL,*wPtr=NULL;
       	int i=0,nLen=0;
        register int j,wd;
	int fnewline = 1;
	char *xpg;                 /* "on": use xpg standard for -n optin */

	(void)setlocale(LC_ALL, "");
	(void)setlabel("UX:echo");

	if(--argc == 0) {
		putchar('\n'); 
		goto done;
	} else if (!strcmp(argv[1], "-n")) {
	    if( (xpg = getenv("_XPG")) != NULL && atoi(xpg) > 0) {
	        fputs("-n ", stdout);
	    }
	    else{
	        fnewline = 0;
	    }
	    ++argv;
	    --argc;
	}

	/* '\' can be in the 2nd byte of a multibyte char. Special
	   procesing used here	*/

	for(i = 1; i <= argc; i++) {
             	nLen=strlen(argv[i])+1;
             	wPtr=(wchar_t *)malloc(nLen*sizeof(wchar_t));
                if(wPtr==NULL) {
                	/* memory allocation fails */
			fprintf(stderr, "echo: %s\n", strerror(errno)); 
                        exit(0);
 		}

              	if(mbstowcs(wPtr,argv[i],nLen)==-1) {
                        /* illegal byte sequence is  encountered. */
			fprintf(stderr, "echo: %s\n", strerror(errno)); 
                        exit(0);
 		}
             	wPtr[nLen-1]='\0'; 
		for(wCptr =  wPtr; *wCptr; wCptr++) {
			if(*wCptr == L'\\')
			switch(*++wCptr) {
				case L'b':
					putchar('\b');
					continue;

				case L'c':
#ifdef sgi
					fnewline = 0;
					continue;
#else
					exit(0);
#endif
				case L'f':
					putchar('\f');
					continue;
				case L'n':
					putchar('\n');
					continue;
				case L'r':
					putchar('\r');
					continue;
				case L't':
					putchar('\t');
					continue;
				case L'v':
					putchar('\v');
					continue;
				case L'\\':
					putchar('\\');
					continue;
				case L'0':
					j = wd = 0;
					while ((*++wCptr >= L'0' && *wCptr <= L'7') && j++ < 3) { /* check for valid octal number */
						wd <<= 3;
						wd |= (*wCptr - L'0');
					}
					putchar(wd);
					--wCptr;
					continue;

				default:
					wCptr--;
				}
		printf("%C",*wCptr);
		}
		free(wPtr);
#ifdef sgi
		if (i == argc) {
			if (fnewline)
				putchar ('\n');
		} else {
			putchar(' ');
		}
#else
		putchar(((i == argc) && fnewline)? '\n': ' ');
#endif
	}
done:
	/* done so we can report errors over nfsv2, and writes to a file on
	 * a full filesystem. */
	if(fclose(stdout) == EOF)
		fprintf(stderr, "echo: %s\n", strerror(errno));
	exit(0);
}
