#ident "$Revision: 1.3 $"
/*-
 * Copyright (c) 1980, 1993
 *      The Regents of the University of California.  All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1980, 1993\n\
        The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)expand.c  8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <stdio.h>
#include <locale.h>
#include <msgs/uxeoe.h>
#include <nl_types.h>
#include <stdlib.h>
#include <wchar.h>
#include <i18n_capable.h>
/*
 * expand - expand tabs to equivalent spaces
 */

int nstops;
int tabstops[100];
nl_catd catd;

main(argc, argv)
	int argc;
	char *argv[];
{
	register int c, column;
	register int n;
	register int i;

	(void) setlocale(LC_ALL, "");
	catd = catopen("uxeoe", 0);
	argc--, argv++;
	nstops = 0;

	if (argc > 0 && argv[0][0] == '-') {
	  if (argv[0][1] == 't' && argv[0][2] == '\0')
	    argc--, argv++;
	  else if (argv[0][1] == 't' && argv[0][2] != '\0') {
		 if (!getstops(&argv[0][2]))
			argc--, argv++;
	       }
	  while (argc > 0 && (isdigit(argv[0][0]))) {
	     if (!getstops(argv[0]))
	     	argc--, argv++;
	     else /* process file arguments */
		break;
	  }
	  if (argc > 0 && (argv[0][0] == '-' && argv[0][1] =='-')) {
	  	/* End of options. Check for "--" */
	  	argc--, argv++;
	  }
	}
	do {
		if (argc > 0) {
		  if (freopen(argv[0], "r", stdin) == NULL) {
		    perror(argv[0]);
		    exit(1);
		  }
		  argc--, argv++;
		}
		column = 0;
		if (I18N_SBCS_CODE) {
			for (;;) {
				c = getc(stdin);
				if (c == -1)
					break;
				switch (c) {
	
				case '\t':
					if (nstops == 0) {
						do {
							putchar(' ');
							column++;
						} while (column & 07);
						continue;
					}
					if (nstops == 1) {
						do {
							putchar(' ');
							column++;
						} while (((column - 1) % tabstops[0]) != (tabstops[0] - 1));
						continue;
					}
					for (n = 0; n < nstops; n++)
						if (tabstops[n] > column)
							break;
					if (n == nstops) {
						putchar(' ');
						column++;
						continue;
					}
					while (column < tabstops[n]) {
						putchar(' ');
						column++;
					}
					continue;
	
				case '\b':
					if (column)
						column--;
					putchar('\b');
					continue;
	
				default:
					putchar(c);
					column++;
					continue;
	
				case '\n':
					putchar(c);
					column=0;
					continue;
				}
			}
		} 
		else { /* i18n: wide char code follows */
                        for (;;) {
                                c = getwc(stdin);
                                if (c == -1)
                                        break;
                                switch (c) {

                                case L'\t':
                                        if (nstops == 0) {
                                                do {
                                                        putchar(' ');
                                                        column++;
                                                } while (column & 07);
                                                continue;
                                        }
                                        if (nstops == 1) {
                                                do {
                                                        putchar(' ');
                                                        column++;
                                                } while (((column - 1) % tabstops[0]) != (tabstops[0]
 - 1));
                                                continue;
                                        }
                                        for (n = 0; n < nstops; n++)
                                                if (tabstops[n] > column)
                                                        break;
                                        if (n == nstops) {
                                                putchar(' ');
                                                column++;
                                                continue;
                                        }
                                        while (column < tabstops[n]) {
                                                putchar(' ');
                                                column++;
                                        }
                                        continue;
                                case L'\b':
                                        if (column)
                                                column--;
                                        putchar('\b');
                                        continue;

                                default:
                                        putwchar(c);
					i = wcwidth(c);
					if (i <= 0)
						column++;
					else
                                        	column += i; 
                                        continue;

                                case L'\n':
                                        putchar('\n');
                                        column=0;
                                        continue;
                                }
                        }

		}
        } while (argc > 0);
	exit(0);
}


int getstops(cp)
	register char *cp;
{
	register int i;
	for (;;) {
		i = 0;
		while (isdigit(*cp))
			i = i * 10 + *cp++ - '0';
		if (i <= 0 || i > 2147483647) {
bad:
			fprintf(stderr,CATGETS(catd, _MSG_EXPAND_BADTABSTOP)); 
			exit(1);
		}
		if ((nstops > 0 && i <= tabstops[nstops-1])  || (nstops == 100))
			goto bad;
		tabstops[nstops++] = i;
		if (*cp == 0)
			break;
		if ((*cp != ',')&&(*cp != ' ')) {/* 2 possibilities: a) Error or 
				   * (b) Filename starting with a digit. In
				   * either case, return and let the error 
				   * processing be done later.
				   */
                        *cp++;
			return(1);
                } else *cp++;
	}
return(0);
}