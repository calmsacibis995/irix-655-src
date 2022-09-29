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

#include "less.h"
#include <stdlib.h>
#include <getopt.h>

int top_scroll;			/* Repaint screen from top */
int massage = 1;		/* Interpret backspaces, tabs and <cr> */
int fold = 1;			/* Fold lines wider than the screen */
int make_printable;		/* Make unprintable chars printable */
int print_help;			/* Display help message instead of bell */
int caseless;			/* Do "caseless" searches */
int cbufs = 10;			/* Current number of buffers */
int linenums = 1;		/* Use line numbers */
int wait_at_eof = 1;		/* Require explicit quit at eof */
int squeeze;			/* Squeeze multiple blank lines into one */
int tabstop = 8;		/* Tab settings */
int tagoption;
int p_option = 0;
int wInitcmd_elms = 0;

char *initcmd;
#include <wchar.h>
#include <i18n_capable.h>
wchar_t *wInitcmd;

int
option(int argc, char **argv)
{
	int ch;
	char *p;
	int line_start_arg = -1;
	int line_search_arg = -1;

	/* backward compatible processing for "+/search" */
	char **a;

	/* The following nastiness deals with three special cases:
	 *
	 *	+/pattern
	 *	-######
	 *	+######
	 *
	 * First we locate the arguments by scanning argv.
	 * Then we act on the options, before the main loop so
	 * that we can distinguish and prioritise the options.
	 * Note the the regular XPG4 options will override regardless.
	 */
	for (a = argv; *a; ++a) {
		if ((*a)[0] == '+') {
			if ((*a)[1] == '/')
				line_search_arg = a - argv;
			else if (isdigit((*a)[1]))
				line_start_arg = a - argv;
			(*a)[0] = '-';

		} 
		else 
		if ((*a)[0] == '-' && isdigit((*a)[1])) {
			sc_height = atoi(*a + 1);
			if ( sc_height > LIMIT_SC_HEIGHT )
				sc_height = LIMIT_SC_HEIGHT;
		}
	}


   if (I18N_SBCS_CODE || I18N_EUC_CODE) {
	if (line_start_arg != -1 && line_start_arg > line_search_arg) {
		if (!(initcmd = malloc(strlen(argv[line_start_arg]+1) + 2))) {
			error(GETTXT(_MSG_MORE_CANT_ALLOC_MEM));
			exit(1);
		}
		strcpy(initcmd, argv[line_start_arg] + 1);
		strcat(initcmd, "G");

	} else if (line_search_arg != -1) {
		initcmd = argv[line_search_arg] + 1;
	}
  } else {
	wchar_t wG[]={'G','\0'};
	if (line_start_arg != -1 && line_start_arg > line_search_arg) {
		if (!(wInitcmd = (wchar_t *)malloc((strlen(argv[line_start_arg]+1) + 2 )*sizeof(wchar_t)))) {
			error(GETTXT(_MSG_MORE_CANT_ALLOC_MEM));
			exit(1);
		}
		mbstowcs(wInitcmd, argv[line_start_arg]+1, strlen(argv[line_start_arg]+1)+1);
		wInitcmd_elms = strlen(argv[line_start_arg]+1) + 2;
		wcscat(wInitcmd, wG);

	} else if (line_search_arg != -1) {
		if (!(wInitcmd = (wchar_t *)malloc((strlen(argv[line_search_arg]+1) + 1 )*sizeof(wchar_t)))) {
			error(GETTXT(_MSG_MORE_CANT_ALLOC_MEM));
			exit(1);
		}
		mbstowcs(wInitcmd, argv[line_search_arg]+1, strlen(argv[line_search_arg]+1)+1);
		wInitcmd_elms = strlen(argv[line_search_arg]+1) + 1;
	}
  }

	p_option = 0;		/* -p option with init command */

	getoptreset();		/* called twice, re-init getopt. */
	while ((ch = getopt(argc, argv,
			    "ceisudfrNn:p:t:x:/:0123456789wl")) != EOF)
		switch(ch) {
		case 'w':	/* now the default; accept the option for the sake of
			old scripts, including things like MANPAGER;  See #434809 */
		case 'l':	/* can't do it with xpg4 stuff, but same as above */
			break;
		case 'c':
			top_scroll = 1;
			break;
		case 'e':
			wait_at_eof = 0;
			break;
		case 'i':
			caseless = 1;
			break;
		case 's':
			squeeze = 1;
			break;
		case 'u':
			massage = 0;
			break;
		case 'd':
			print_help = 1;
			break;
		case 'f':
			fold = 0;
			break;
		case 'r':
			make_printable = 1;
			break;
		case 'N':	/* was bsd4.4 n option */
			linenums = 0;
			break;
		case 'n':
			sc_height = atoi(optarg);
                        if ( sc_height > LIMIT_SC_HEIGHT )
                                sc_height = LIMIT_SC_HEIGHT;
			break;
		case 'p':
			p_option = 1;
			if (I18N_SBCS_CODE || I18N_EUC_CODE)
				initcmd = optarg;
			else {
				if (!(wInitcmd = (wchar_t *) 
					malloc((strlen(optarg) + 1 )
					* sizeof(wchar_t))))
				{
					error(GETTXT(_MSG_MORE_CANT_ALLOC_MEM));
					exit(1);
				}
				mbstowcs(wInitcmd, optarg, strlen(optarg)+1);
			}
			break;
		case 't':
			tagoption = 1;
			if (I18N_SBCS_CODE || I18N_EUC_CODE)
				findtag(optarg);
			else {
				wchar_t *wTag;
				if (!(wTag= (wchar_t *)
					malloc((strlen(optarg) + 1 )
					* sizeof(wchar_t))))
				{
					error(GETTXT(_MSG_MORE_CANT_ALLOC_MEM));
					exit(1);
				}
				mbstowcs(wTag, optarg, strlen(optarg)+1);
				wfindtag(wTag);
				free(wTag);
			}
			break;
		case 'x':
			tabstop = atoi(optarg);
			if (tabstop <= 0)
				tabstop = 8;
			break;
		case '/':	/* ignore - dealt with above */
			break;
		case '0':	/* ignore - dealt with above */
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			break;
		case '?':
		default:
			_sgi_nl_usage(SGINL_USAGE, cmd_label,
				GETTXT(_MSG_MORE_USAGE1), cmd_name);
			_sgi_nl_usage(SGINL_USAGESPC, cmd_label,
				GETTXT(_MSG_MORE_USAGE2));
			exit(1);
		}
	return(optind);
}
