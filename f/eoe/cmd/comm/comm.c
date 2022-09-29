/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)comm:comm.c	1.3.1.1"
/*
**	process common lines of two files
*/

#include	<stdio.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<string.h>
#include	<widec.h>
#include 	<wchar.h>
#include 	<i18n_capable.h>
#include 	<msgs/uxdfm.h>

#define	LB	4096 /* previously 256 */

int	one;
int	two;
int	three;

char	*ldr[3];

FILE	*ib1;
FILE	*ib2;
FILE	*openfil();

main(argc,argv)
char **argv;
{
	int	l = 1;
	char sb_lb1[LB], sb_lb2[LB];
	char *lb1, *lb2;

	if (I18N_SBCS_CODE) {
		lb1 = sb_lb1;
		lb2 = sb_lb2;
	} else {
		lb1 = (char *) malloc(LB * sizeof(wchar_t));
		lb2 = (char *) malloc(LB * sizeof(wchar_t));
	}

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:comm");

	ldr[0] = "";
	ldr[1] = "\t";
	ldr[2] = "\t\t";
	while (argc > 1)  {
		if(*argv[1] != '-' || argv[1][1] == 0)
			break;
		while(*++argv[1]) {
			switch(*argv[1]) {
			case '1':
				one = 1;
				break;

			case '2':
				two = 1;
				break;

			case '3':
				three = 1;
				break;

			case '-':
				argv++;
				argc--;
				goto Break;

			default:
				if(I18N_SBCS_CODE) {
					pfmt(stderr, MM_ERROR,
						PFMTTXT(_MSG_ILLEGAL_OPT_STRING),
						*argv[1]);
				}
				else {
					wchar_t wTemp;
					if(mbtowc(&wTemp,argv[1],MB_CUR_MAX) == -1)
						wTemp = L'?';
                                        pfmt(stderr, MM_ERROR,        
                                                PFMTTXT(_MSG_ILLEGAL_OPT_MB),
                                                wTemp);
                                     }
					usage(0);
			}
		}
		argv++;
		argc--;
	}
 Break:

	if(argc < 3)
		usage(1);
	if (one) {
		ldr[1][0] = '\0';
		ldr[2][l--] = '\0';
	}
	if (two)
		ldr[2][l] = '\0';
	ib1 = openfil(argv[1]);
	ib2 = openfil(argv[2]);
	if(rd(ib1,lb1) < 0) {
		if(rd(ib2,lb2) < 0)
			exit(0);
		copy(ib2,lb2,2);
	}
	if(rd(ib2,lb2) < 0)
		copy(ib1, lb1, 1);
	while(1) {
		switch(compare(lb1,lb2)) {
			case 0:
				wr(lb1,3);
				if(rd(ib1,lb1) < 0) {
					if(rd(ib2,lb2) < 0)
						exit(0);
					copy(ib2,lb2,2);
				}
				if(rd(ib2,lb2) < 0)
					copy(ib1, lb1, 1);
				continue;

			case 1:
				wr(lb1,1);
				if(rd(ib1,lb1) < 0)
					copy(ib2, lb2, 2);
				continue;

			case 2:
				wr(lb2,2);
				if(rd(ib2,lb2) < 0)
					copy(ib1, lb1, 1);
				continue;
		}
	}
}

rd(file,buf)
FILE *file;
char *buf;
{

	register int i, j;
	register wint_t wj = 0;
	wchar_t *wBuf;
	i = j = 0;
	if (I18N_SBCS_CODE) {
		while((j = getc(file)) != EOF) {
			*buf = j;
			if(*buf == '\n' || i > LB-2) {
				*buf = '\0';
				return(0);
			}
			i++;
			buf++;
		}
	} else {
		wBuf = (wchar_t *) buf;
		while((wj = getwc(file)) != WEOF) {
			*wBuf = wj;
			if(*wBuf == L'\n' || i > LB-2) {
				*wBuf = L'\0';
				return(0);
			}
			i++;
			wBuf++;
		}
	}
	return(-1);
}

wr(str,n)
char *str;
{
	switch(n) {
		case 1:
			if(one)
				return 0;
			break;

		case 2:
			if(two)
				return 0;
			break;

		case 3:
			if(three)
				return 0;
	}
	if (I18N_SBCS_CODE) 
		printf("%s%s\n",ldr[n-1],str);
	else
		printf("%s%S\n",ldr[n-1], (wchar_t *) str);
	return 0;
}

copy(ibuf,lbuf,n)
FILE *ibuf;
char *lbuf;
{
	do {
		wr(lbuf,n);
	} while(rd(ibuf,lbuf) >= 0);

	exit(0);
}

compare(a,b)
char *a,*b;
{
	register char *ra,*rb;

	if (I18N_SBCS_CODE) {
		ra = --a;
		rb = --b;
		while(*++ra == *++rb)
			if(*ra == '\0')
				return(0);
		if(*ra < *rb)
			return(1);
	} else {
		register wchar_t *wra, *wrb;
                int Temp;

		wra = (wchar_t *) a;
		wrb = (wchar_t *) b;
		Temp = wcscoll(wra,wrb);
		if(Temp == 0)
			 return(0);
                else if(Temp < 0)
			 return(1);
	}
	return(2);
}
FILE *openfil(s)
char *s;
{
	FILE *b;
	if(s[0]=='-' && s[1]==0)
		b = stdin;
	else if((b=fopen(s,"r")) == NULL) {
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_CANNOT_OPEN),
			s, strerror (errno));
		exit(2);
	}
	return(b);
}

usage(complain)
int complain;
{
	if (complain)
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_INCORRECT_USAGE));
	pfmt(stderr, MM_ACTION, PFMTTXT(_MSG_COMM_USAGE));
	exit(2);
}
