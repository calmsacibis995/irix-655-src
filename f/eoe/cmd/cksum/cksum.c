/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James W. Williams of NASA Goddard Space Flight Center.
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

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)cksum.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <msgs/uxdfm.h>
#include <pfmt.h>
#include <locale.h>
#include "extern.h"

void usage __P((void));

int
main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	u_long len, val;
	register int ch, fd, rval;
	char *fn;
	int (*cfncn) __P((int, unsigned long *, unsigned long *));
	void (*pfncn) __P((char *, unsigned long, unsigned long));

	(void)setlocale( LC_ALL, "" );
	(void)setcat("uxdfm");
	(void)setlabel("UX:cksum");

	cfncn = crc;
	pfncn = pcrc;
	while ((ch = getopt(argc, argv, "o:")) != EOF)
		switch(ch) {
		case 'o':
			if (*optarg == '1') {
				cfncn = csum1;
				pfncn = psum1;
			} else if (*optarg == '2') {
				cfncn = csum2;
				pfncn = psum2;
			} else {
				(void)pfmt(stderr, MM_ACTION,
				    		  PFMTTXT(_MSG_CKSUM_ILL_OPT));
				usage();
			}
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	fd = STDIN_FILENO;
	fn = NULL;
	rval = 0;
	do {
		if (*argv) {
			fn = *argv++;
			if ((fd = open(fn, O_RDONLY, 0)) < 0) {
				(void)pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_CKSUM_FILE),
				    fn, strerror(errno));
				rval = 1;
				continue;
			}
		}
		if (cfncn(fd, &val, &len)) {
			(void)pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_CKSUM_FILE),
			    fn ? fn : "stdin", strerror(errno));
			rval = 1;
		} else
			pfncn(fn, val, len);
		(void)close(fd);
	} while (*argv);
	exit(rval);
}

void
usage()
{
	(void)pfmt(stderr, MM_ACTION, PFMTTXT(_MSG_CKSUM_USAGE));
	exit(1);
}
