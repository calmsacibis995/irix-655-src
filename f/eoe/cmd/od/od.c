/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* 	Portions Copyright(c) 1988, Sun Microsystems Inc.	*/
/*	All Rights Reserved					*/

#ident	"$Revision: 1.17 $"
/*
 * od -- octal (also hex, decimal, and character) dump
 */

/*	mbCput() and mbAput() modified to print mb char first	*/
/*	and then for the corresponding bytes which are part of	*/
/*	mb char "**" are printed for -c or -tc or -ta options.	*/

/*	Function mbFseek() modified to remove a bug when null	*/
/*	characters were encountered in MB locales.		*/

/*	Error messages redirected to stderr			*/

/*	Bug fix for the case when iswprint fails for mb		*/
/*	characters. Affected functions: mbCput(), mbAput().	*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <assert.h>
#include <i18n_capable.h>
#include <widec.h>
#include <msgs/uxdfm.h>
#include <msgs/uxsgicore.h>



#define BSIZE		512	/* standard buffer size */
#define KILO		1024
#define MEG		1048576
#define YES		1
#define NO		0
#define DBUFSIZE	16
#define LIST_INCR	32

#define BAD_NONE	0
#define BAD_BYTE	1


struct dfmt {
	char	df_option;	/* command line option */
	int	df_field;	/* # of chars in a field */
	int	df_size;	/* # of bytes displayed in a field */
	int	df_radix;	/* conversion radix */
} **conv_vec;
unsigned int	conv_vec_len;
unsigned int	conv_vec_max;
unsigned int	conv_vec_cur;

char		**file_vec;
unsigned int	file_vec_len;
unsigned int	file_vec_max;
unsigned int	file_vec_cur;
int		file_vec_cur_opened = NO;
int		file_vec_nul = YES;

/* The names of the structures are chosen for the options action
 *	e.g. u_s_oct - unsigned octal
 *	     u_l_hex - unsigned long hex
*/


struct dfmt ncr		= {'a',  3, sizeof(char),		 8};
struct dfmt chr		= {'c',  3, sizeof(char),		 8};
struct dfmt dec_c	= {'d',  4, sizeof(char),		10};
struct dfmt dec_s	= {'d',  6, sizeof(short),		10};
struct dfmt dec_i	= {'d', 11, sizeof(int),		10};
struct dfmt dec_l	= {'d', 11, sizeof(long),		10};
struct dfmt flt_f	= {'f', 14, sizeof(float),		10};
struct dfmt flt_d	= {'f', 21, sizeof(double),		10};
struct dfmt flt_l	= {'f', 21, sizeof(long double),	10};
struct dfmt oct_c	= {'o',  3, sizeof(char),		 8};
struct dfmt oct_s	= {'o',  6, sizeof(short),		 8};
struct dfmt oct_i	= {'o', 11, sizeof(int),		 8};
struct dfmt oct_l	= {'o', 11, sizeof(long),		 8};
struct dfmt udc_c	= {'u',  3, sizeof(char),		10};
struct dfmt udc_s	= {'u',  5, sizeof(short),		10};
struct dfmt udc_i	= {'u', 10, sizeof(int),		10};
struct dfmt udc_l	= {'u', 10, sizeof(long),		10};
struct dfmt hex_c	= {'x',  2, sizeof(char),		16};
struct dfmt hex_s	= {'x',  4, sizeof(short),		16};
struct dfmt hex_i	= {'x',  8, sizeof(int),		16};
struct dfmt hex_l	= {'x',  8, sizeof(long),		16};

#define PRINT_FMT_FLT		"%14.7e"
#define PRINT_FMT_DBL		"%21.14e"
#define PRINT_FMT_LDB		"%21.14e"
#define         TRUE                       1
#define         FALSE                      0




char	word[DBUFSIZE+MB_LEN_MAX];
char	lastword[DBUFSIZE+MB_LEN_MAX];
int	base =	010;
int	max;
off64_t	addr;
off64_t	count;
int	count_limit = 0;
int	display=7;
char    CharSubs = '*' ;
int     BytesOfSPlitChar = 0 ;
int     PrintAsChar = FALSE ;
int	NoOfBytesAlreadyThere = 0;
int	NoOfBytesAdded = 0;
int	NoOfBytesAddedTmp = 0;
int	AlreadyReadBytes = 0;
int	FirstTime = 1;
int	FreshLineAfterSplitChar = 0;
char    *PgmName;
char   	TmpBuf[2*MB_LEN_MAX];


/** 	Explanation of various variables which handle split char
    	situation.
	Consider the situation where 0x8e is the 16th byte.
	The next bytes are 'a', 'b' which together with 0x8e
	form some mb chars(assume it).

	When mbCput() or mbAput() encounters 0x8e followed by null
	(null is appended at the end of the buffer) mbtowc will return -1.
	Here FirstTime is one.
	
FirstTime: is set if fresh read is done.ie. when file_vec_read() is called.
	This is to prevent -c -c option to confuse the second -c option.

	Now MB_CUR_MAX more bytes plus 0x8e are copied into a TmpBuf.

TmpBuf: used to hold some combined bytes so that WC could formed out of
	those bytes.
	
	A WC is formed and file pointer is set to begining of the new line.
	ie. it'll point to 'a'.

	At this moment NoOfBytesAdded into TmpBuf will contain 3.(0x8e, a, b)
	
NoOfBytesAdded: contains no. of bytes in TmpBuf which form WC.
	
	NoOfBytesAlreadyThere will be set to 1. (0x8e).

NoOfBytesAlreadyThere: contains no. of starting bytes of the split char
	which are present in the current line.

AlreadyReadBytes: same as NoOfBytesAlreadyThere.
	It is set only one when, before reading next line(ie. line
	begining with 'a', 'b'.

	Rest of the flags/variables take care of -c -c or -ta -ta
	like option conditions so that variables suitably set only 
	when next read is done.
**/

char *opt_A_args[] = {
	"d",
	"o",
	"x",
	"n",
	NULL
};
int opt_A_conv[] = { 10, 8, 16, -1 };


#define NAMED_CHARS_FIRST		0
#define NAMED_CHARS_LAST		040
#define NAMED_CHARS_OTHER_DEL		0177
#define NAMED_CHARS_OTHER_DEL_NAME	"del"

char	*named_chars[NAMED_CHARS_LAST-NAMED_CHARS_FIRST+1] = {
	"nul", "soh", "stx", "etx",
	"eot", "enq", "ack", "bel",
	" bs", " ht", " nl", " vt",
	" ff", " cr", " so", " si",
	"dle", "dc1", "dc2", "dc3",
	"dc4", "nak", "syn", "etb",
	"can", " em", "sub", "esc",
	" fs", " gs", " rs", " us",
	" sp"
};


int     mbCput(char c[]);
int     mbAput(char c[]);
int     mbFseek(FILE *fp, long NoOfBytes) ;
void	line	(off64_t a, char w[], int n);
int	putx	(char n[], struct dfmt *c);
void	cput	(int c);
void	aput	(int c);
void	putn	(unsigned long long n, unsigned int b, unsigned int c);
void	pre	(int f, int n);
void	offset	(char *s);

u_long	string_to_ul	(const char *str);
int	file_vec_read	(char *buf, off64_t max);
int	file_vec_skip	(off64_t skip);
void	conv_add	(struct dfmt *conv);
void	conv_parse	(char *arg);

char inputerror[] = CATNUM_DEFMSG(_MSG_INPUT_ERROR);



void
usage()
{
	pfmt(stderr, MM_ACTION,
		PFMTTXT(_MSG_OD_USAGE));
	exit(1);
}


int
main(int argc, char **argv)
{
	register n, f, same;
	int showall = NO;
	int opt;
	char *sub_opt;
	char *sub_val;
	struct stat stat_buf;
	int num_operands;
	int new_options = NO;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:od");

#ifdef STANDALONE
	if (argv[0][0] == '\0')
		argc = getargv("od", &argv, 0);
#endif


	PgmName = argv[0];
	conv_vec_max = LIST_INCR;
	conv_vec_len = 0;
	conv_vec_cur = 0;
	if (!(conv_vec = (struct dfmt **)
		malloc(conv_vec_max * sizeof(struct dfmt *)))) {
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_OUT_OF_MEMORY));
		exit(2);
	}

	file_vec_max = LIST_INCR;
	file_vec_len = 0;
	file_vec_cur = 0;
	if (!(file_vec = (char **) malloc(file_vec_max * sizeof(char *)))) {
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_OUT_OF_MEMORY));
		exit(2);
	}

	while ((opt = getopt(argc, argv, "A:j:n:N:t:odsxhcbDfFOSXv")) != -1) {

		switch(opt) {

		case 'A':
			sub_opt = optarg;
			base = getsubopt(&sub_opt, opt_A_args, &sub_val);
			if (base < 0 ||
			    getsubopt(&sub_opt, opt_A_args, &sub_val) >= 0)
				usage();
			base = opt_A_conv[base];
			new_options = YES;
			break;
		case 'j':
			addr = string_to_ul(optarg);
			new_options = YES;
			break;
		case 'n':
		case 'N':
			count = string_to_ul(optarg);
			count_limit = 1;
			new_options = YES;
			break;
		case 't':
			conv_parse(optarg);
			new_options = YES;
			break;
		case 'o':
			conv_add(&oct_s);
			break;
		case 'd':
			conv_add(&udc_s);
			break;
		case 's':
			conv_add(&dec_s);
			break;
		case 'x':
		case 'h':
			conv_add(&hex_s);
			break;
		case 'c':
			if(!I18N_SBCS_CODE)
                                PrintAsChar = TRUE ;
			conv_add(&chr);
			break;
		case 'b':
			conv_add(&oct_c);
			break;
		case 'D':
			conv_add(&udc_l);
			break;
		case 'f':
			conv_add(&flt_f);
			break;
		case 'F':
			conv_add(&flt_d);
			break;
		case 'O':
			conv_add(&oct_l);
			break;
		case 'S':
			conv_add(&dec_l);
			break;
		case 'X':
			conv_add(&hex_l);
			break;
		case 'v':
			showall = YES;
			break;
		default :
			usage ();
		}
	}

	if (conv_vec_len == 0) {
		conv_add(&oct_s);
	}


	/* Calucate max number chars in line */
	max = 0;
	for (conv_vec_cur = 0; conv_vec_cur < conv_vec_len; conv_vec_cur++) {
		f = (conv_vec[conv_vec_cur]->df_field + 1) *
			(DBUFSIZE / conv_vec[conv_vec_cur]->df_size);
		if (f > max)
			max = f;
	}

	argc -= optind -1;
	argv += optind;


	/* parse multiple files to dump and remember them */

	num_operands = argc - 1;

	for (file_vec_len = 0; argc > 1; argc--, argv++) {

		if (!new_options) {
			if (**argv == '+') {
				offset(*argv);
				continue;
			}
			/* backward compatibility hack more or less
			   accepted by xpg4 */
			if (num_operands == 2 && file_vec_len == 1) {
				if ((isdigit(**argv)) ||
				    (**argv == 'x') || (**argv == '.') ||
				    (**argv == 'b') || (**argv == 'B') ||
				    (**argv == 'l' && **(argv+1) == 'l' &&
							**(argv+2) == '\0') ||
				    (**argv == 'L' && **(argv+1) == 'L' &&
							**(argv+2) == '\0')) {

					offset(*argv);
					continue;
				}
			}
		}

		if (stat(*argv, &stat_buf) == -1) {
			pfmt(stderr, MM_ERROR,
				     PFMTTXT(_MSG_CANNOT_OPEN),
				     *argv, strerror(errno));
			exit(2);
		}
		else {
			if (file_vec_len == file_vec_max) {
				file_vec_max += LIST_INCR;
				file_vec = realloc(file_vec,
						file_vec_max*sizeof(char *));
				if (!file_vec) {
					pfmt(stderr, MM_ERROR,
						PFMTTXT(_MSG_OUT_OF_MEMORY));
					exit(2);
				}
			}

			file_vec[file_vec_len++] = *argv;
			file_vec_nul = NO;
		}
	}

	/* skip -j xxx bytes if needed	*/
	if (addr > 0) {
		if (file_vec_skip(addr) != addr) {
			pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_UNABLE_TO_SKIP), addr);
			exit(2);
		}
	}

	for (same = -1; ; addr += n, count -= n) {
	if (!I18N_SBCS_CODE) {
		if(NoOfBytesAdded) {
			FreshLineAfterSplitChar = 1;
			FirstTime = 1;
			AlreadyReadBytes = NoOfBytesAdded - NoOfBytesAlreadyThere;
			NoOfBytesAdded=0;
		}
	}
		if (count_limit && count <= 0) break;

		/* read from list of files */
		if (I18N_SBCS_CODE) 
		n = file_vec_read(word, (sizeof(word)-MB_LEN_MAX<count || !count_limit) ?
							sizeof(word)-MB_LEN_MAX : count);
		else
		n = file_vec_read(word, (sizeof(word)-MB_LEN_MAX<count || !count_limit) ?
							sizeof(word)-MB_LEN_MAX+BytesOfSPlitChar : count+BytesOfSPlitChar );
		if (n <= 0) break;

		if (same>=0 && !showall) {
			for (f=0; f<DBUFSIZE; f++)
				if (lastword[f] != word[f])
					goto notsame;
			if (same==0) {
				printf("*\n");
				same = 1;
			}
			continue;
		}
	notsame:
		line(addr, word, (n+sizeof(word[0])-1)/sizeof(word[0]));
		if (!I18N_SBCS_CODE) {
			if ((!NoOfBytesAdded) && (FreshLineAfterSplitChar == 1)) {
				FreshLineAfterSplitChar = 0;
				AlreadyReadBytes = 0;
			}
		}
		same = 0;
		for (f=0; f<DBUFSIZE; f++)
			lastword[f] = word[f];
		for (f=0; f<DBUFSIZE; f++)
			word[f] = 0;
	}


        if (ferror(stdin)) {
		pfmt(stderr, MM_ERROR, inputerror, strerror(errno),
		             (file_vec_nul && file_vec_cur_opened) ?
		             "stdin" : file_vec[file_vec_cur-1]);
                exit (2);
        }

	if (base > 0)
		putn(addr, base, display);
	putchar('\n');

	/* buffering from stdio screwed this up, set it back now */
	if (file_vec_nul)
		lseek64(0, ftell(stdin), SEEK_SET);

	exit(0);
}

void
line(off64_t a, char w[], int n)
{
	register i, f;
        int     cntr ;
        char    mb_pChar[DBUFSIZE+1] ;
        static int      FirstCall = 0 ;

        if(!I18N_SBCS_CODE)
        {
                for(cntr = 0; cntr < n; cntr++)
                        mb_pChar[cntr] =  w[cntr] ;
                mb_pChar[n] = '\0' ;
        }

	f = 1;

	for (conv_vec_cur = 0; conv_vec_cur < conv_vec_len; conv_vec_cur++) {
		NoOfBytesAddedTmp = AlreadyReadBytes;
		
		if (base > 0) {
			if(f) {
				putn(a, base, display);
				f = 0;
			} else
				putchar('\t');
		}
                if(!I18N_SBCS_CODE)
                {
                        if(conv_vec[conv_vec_cur]->df_option == 'c' || conv_vec[conv_vec_cur]->df_option == 'a') {
                                if(FirstCall == 0) {
                                        for(cntr = 0; cntr < BytesOfSPlitChar; cntr++) {
                                                putchar(' ') ;
                                                putchar(' ') ;
                                                putchar(CharSubs) ;
                                                putchar(CharSubs) ;
					}
                                }
                        }
                }

		i = 0;
                if(!I18N_SBCS_CODE)
			if(conv_vec[conv_vec_cur]->df_option == 'c' || conv_vec[conv_vec_cur]->df_option == 'a')
				if(FirstCall == 0)
					i += BytesOfSPlitChar ;
		while (i<n) {
		   if(I18N_SBCS_CODE)
			i += putx(w+i, conv_vec[conv_vec_cur]);
		   else
			i += putx(mb_pChar+i, conv_vec[conv_vec_cur]);
		}
		putchar('\n');
	}
	if(!I18N_SBCS_CODE)
	   if (FirstCall == 0) {
		FirstCall++;
		BytesOfSPlitChar = 0;
	}
}

int
putx(char n[], struct dfmt *c)
{
	long long	val;

	switch(c->df_option) {

	case 'a':
		pre(c->df_field, c->df_size);
                if(I18N_SBCS_CODE) {
                        aput(*n);
                        return c->df_size;
                }
                else
                        return mbAput(n) ;

	case 'c':
		pre(c->df_field, c->df_size);
                if(I18N_SBCS_CODE) {
                        cput(*n);
                        return c->df_size;
                }
                else
                        return mbCput(n) ;
	case 'f':
		pre(c->df_field, c->df_size);
		if (c->df_size == sizeof(float)) {
			float *fn = (float *)n;
			printf(PRINT_FMT_FLT,*fn);
		}
		else if (c->df_size == sizeof(double)) {
			double dn;
			memcpy(&dn, n, sizeof(dn));
			printf(PRINT_FMT_DBL,dn);
		}
		else if (c->df_size == sizeof(long double)) {
			long double ldn;
			memcpy(&ldn, n, sizeof(ldn));
			printf(PRINT_FMT_LDB, ldn);
		}
		else
			assert(0);

		return c->df_size;

	case 'o':
	case 'u':
	case 'x':
		pre(c->df_field, c->df_size);
		if (c->df_size == sizeof(char)) {
			unsigned char	*cn = (unsigned char *)n;
			putn((long long)*cn, c->df_radix, c->df_field);
		}
		else if (c->df_size == sizeof(short)) {
			unsigned short	*sn = (unsigned short *)n;
			putn((long long)*sn, c->df_radix, c->df_field);
		}
		else if (c->df_size == sizeof(int)) {
			unsigned int	*in = (unsigned int *)n;
			putn((long long)*in, c->df_radix, c->df_field);
		}
		else if (c->df_size == sizeof(long)) {
			unsigned long	*ln = (unsigned long *)n;
			putn((long long)*ln, c->df_radix, c->df_field);
		}
		else
			assert(0);

		return c->df_size;

	case 'd':
		putchar(' ');

		if (c->df_size == sizeof(char)) {
			unsigned char	*uc = (unsigned char *)n;
			if (*uc > CHAR_MAX) {
				pre(c->df_field, c->df_size);
				putchar('-');
				*uc = (~(*uc) + 1) & UCHAR_MAX;
			}
			else
				pre(c->df_field-1, c->df_size);

			putn((long long)*uc, c->df_radix, c->df_field-1);
		}
		else if (c->df_size == sizeof(short)) {
			unsigned short	*us = (unsigned short *)n;
			if (*us > SHRT_MAX) {
				pre(c->df_field, c->df_size);
				putchar('-');
				*us = (~(*us) + 1) & USHRT_MAX;
			}
			else
				pre(c->df_field-1, c->df_size);

			putn((long long)*us, c->df_radix, c->df_field-1);
		}
		else if (c->df_size == sizeof(int)) {
			unsigned int	*ui = (unsigned int *)n;
			if (*ui > INT_MAX) {
				pre(c->df_field, c->df_size);
				putchar('-');
				*ui = (~(*ui) + 1) & UINT_MAX;
			}
			else
				pre(c->df_field-1, c->df_size);

			putn((long long)*ui, c->df_radix, c->df_field-1);
		}
		else if (c->df_size == sizeof(long)) {
			unsigned long	*ul = (unsigned long *)n;
			if (*ul > LONG_MAX) {
				pre(c->df_field, c->df_size);
				putchar('-');
				*ul = (~(*ul) + 1) & ULONG_MAX;
			}
			else
				pre(c->df_field-1, c->df_size);

			putn((long long)*ul, c->df_radix, c->df_field-1);
		}
		else
			assert(0);

		return c->df_size;
	}

	assert( 0 );	/* never reached */
	return 0;
}

/* print a character that is multi byte				*/
/*    Parameters  c : The pointer to the char array where	*/
/*                    the multibyte characters read are stored	*/
/*    Return :    No of bytes comprising the last multibyte	*/
/*                character printed.				*/

int
mbCput(char c[])
{
 if (!I18N_SBCS_CODE) {
        int             Bytes ;
        wchar_t         wWideChar ;
        char            *mb_pCharPtr ;
        int             i;
	int		bad_byte = BAD_NONE;

	if(NoOfBytesAddedTmp) {
		putchar(' ');
		putchar('*');
		putchar('*');
		NoOfBytesAddedTmp--;
		return 1;
	}

        mb_pCharPtr = c ;

	NoOfBytesAlreadyThere = strlen(mb_pCharPtr);

        Bytes = mbtowc(&wWideChar, mb_pCharPtr, MB_CUR_MAX) ;

	if ( Bytes == 0 ) {
		bad_byte = BAD_BYTE;
		Bytes = 1;
	}
	else
        if ( Bytes == -1 ) {

	    if ( NoOfBytesAlreadyThere < MB_CUR_MAX ) {

		int 	Read = 0;
		int 	width;

		if (FirstTime) {
			for (i=0;i<NoOfBytesAlreadyThere;i++)
				TmpBuf[i] = mb_pCharPtr[i];

			if (file_vec_cur_opened) {
				Read = fread(TmpBuf+NoOfBytesAlreadyThere , 1, MB_CUR_MAX, stdin);
				if (Read <0 ) {
					pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_READ_ERROR));
					exit(2);
				}
			}
			TmpBuf[NoOfBytesAlreadyThere+Read]='\0';
		}

        	Bytes = mbtowc(&wWideChar, TmpBuf, MB_CUR_MAX) ;

		if (Bytes == -1 || Bytes == 0) {
			bad_byte = BAD_BYTE;
			Bytes = 1;	
		}

		if (FirstTime) {
			FirstTime = 0;
			if (file_vec_cur_opened)
				fseek(stdin, -Read , SEEK_CUR);
			NoOfBytesAdded = Bytes;
		}

		if ( bad_byte == BAD_NONE && iswprint(wWideChar) ) {
			width = wcwidth( wWideChar );
			for ( i=0; i<(3-width); i++ )
				putchar(' ');
			putwchar( wWideChar );
			return Bytes;
		}
		else {  
			Bytes = 1;		/* Including Non Printable characters. */
			NoOfBytesAdded = 0; 
		}
	    }
	    else {				/* NoOfBytesAlreadyThere >= MB_CUR_MAX */
		bad_byte = BAD_BYTE;
		Bytes = 1;
	    }
	}

        if ( bad_byte == BAD_NONE && iswprint(wWideChar) ) {
                for (i = 0; i < Bytes; i++) {
			if (i == 0) {
				int j;
				int width = wcwidth( wWideChar );
				for ( j=0; j<(3-width); j++ )
					putchar(' ');
                		putwchar( wWideChar ) ;
			}
			else {
				putchar(' ');
				putchar(' ');
                        	putchar(CharSubs) ;
                        	putchar(CharSubs) ;
			}
                }
               	return Bytes ;
        }

        switch(*c) {
                case '\0':
                        printf(" \\0");
                        break;
                case '\a':
                        printf(" \\a");
                        break;
                case '\b':
                        printf(" \\b");
                        break;
                case '\f':
                        printf(" \\f");
                        break;
                case '\n':
                        printf(" \\n");
                        break;
                case '\r':
                        printf(" \\r");
                        break;
                case '\t':
                        printf(" \\t");
                        break;
                case '\v':
                        printf(" \\v");
                        break;
                default:
                	for (i = 0; i < Bytes; i++) {
                            	putn((long long)mb_pCharPtr[i], 8, 3);
				if (i<Bytes-1)
					putchar(' ');
			}
                }

        if (*c == '\0')
                return 1 ;
        else
                return Bytes ;
 }
 return -1;
}

void
cput(int c)
{
	c &= 0377; /* this number is 0xff = 255 */
		if(isprint(c)) {
			printf("  ");
			putchar(c);
			return;
			}
		switch(c) {
		case '\0':
			printf(" \\0");
			break;
		case '\a':
			printf(" \\a");
			break;
		case '\b':
			printf(" \\b");
			break;
		case '\f':
			printf(" \\f");
			break;
		case '\n':
			printf(" \\n");
			break;
		case '\r':
			printf(" \\r");
			break;
		case '\t':
			printf(" \\t");
			break;
		case '\v':
			printf(" \\v");
			break;
		default:
			putn((long long)c, 8, 3);
		}
}

/* print a character (for -a option) that is multibyte.		*/ 
/*   Parameters:  c : The pointer to the char array where the	*/
/*                    multibyte characters read are stored	*/
/*   Return:      No of bytes comprising the last multibyte	*/
/*                character printed.				*/
int
mbAput(char c[])
{
 if (!I18N_SBCS_CODE) {
        int             Bytes ;
        wchar_t         wWideChar ;
        char            *mb_pCharPtr ;
        int             i;
	int		bad_byte = 0;

	if(NoOfBytesAddedTmp) {
		putchar(' ');
		putchar('*');
		putchar('*');
		NoOfBytesAddedTmp--;
		return 1;
	}

        mb_pCharPtr = c ;

	NoOfBytesAlreadyThere = strlen(mb_pCharPtr);

        Bytes = mbtowc(&wWideChar, mb_pCharPtr, MB_CUR_MAX) ;

	if ( Bytes == 0 ) {
		bad_byte = BAD_BYTE;
		Bytes = 1;
        }
	else
        if ( Bytes == -1 ) {

	    if ( NoOfBytesAlreadyThere < MB_CUR_MAX ) {

		int 	Read = 0;
		int 	width;

		if (FirstTime) {
			for(i=0;i<NoOfBytesAlreadyThere;i++)
				TmpBuf[i] = mb_pCharPtr[i];

			if (file_vec_cur_opened) {
				Read = fread(TmpBuf+NoOfBytesAlreadyThere , 1, MB_CUR_MAX, stdin);
				if (Read <0 ) {
					pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_READ_ERROR));
					exit(2);
				}
			}
			TmpBuf[NoOfBytesAlreadyThere+Read]='\0';
		}

        	Bytes = mbtowc(&wWideChar, TmpBuf, MB_CUR_MAX) ;

		if (Bytes == -1 || Bytes ==0) {
			bad_byte = BAD_BYTE;
			Bytes = 1;
		}

		if (FirstTime) {
			FirstTime = 0;
			if (file_vec_cur_opened) 
				fseek(stdin, -Read , SEEK_CUR);
			NoOfBytesAdded = Bytes;
		}

                if ( bad_byte == BAD_NONE && iswprint(wWideChar) ) {
			width = wcwidth(wWideChar);
			for(i=0;i<(3-width);i++)
				putchar(' ');
			putwchar(wWideChar);
			return Bytes;
		}
		else {
			Bytes = 1;		/* Including Non Printable characters. */
			NoOfBytesAdded = 0;
                }

	    }
	    else {				/* NoOfBytesAlreadyThere >= MB_CUR_MAX */
		bad_byte = BAD_BYTE;
		Bytes = 1;
	    }
        }

        if (*c <= NAMED_CHARS_LAST)
		printf("%s", named_chars[c[0]]);
	else
        if (*c == NAMED_CHARS_OTHER_DEL)
                printf(NAMED_CHARS_OTHER_DEL_NAME) ;
        else 
	if ( bad_byte == BAD_NONE && iswprint(wWideChar) ) {
                for(i = 0; i < Bytes; i++) {
			if(i == 0) {
				int i;
				int width = wcwidth(wWideChar);
				for(i=0;i<(3-width);i++)
					putchar(' ');
                		putwchar(wWideChar) ;
			}
			else {
				putchar(' ');
				putchar(' ');
                        	putchar(CharSubs) ;
                        	putchar(CharSubs) ;
			}
                }
                return Bytes ;
        }
	else {
                for(i = 0; i < Bytes; i++) {
                        putn((long long)mb_pCharPtr[i], 8, 3);
			if (i<Bytes-1)
				putchar(' ');
		}
        }

        if(*c == '\0')
                return 1 ;
        else
                return Bytes ;
 }
 return -1;
}


void
aput(int c)
{
	c &= 0377;

	if (c >= NAMED_CHARS_FIRST && c <= NAMED_CHARS_LAST) {
		printf("%s", named_chars[c]);
	}
	else if (c == NAMED_CHARS_OTHER_DEL) {
		printf(NAMED_CHARS_OTHER_DEL_NAME);
	}
	else if (isprint(c)){
		printf("  ");
		putchar(c);
		return;
	}
	else {
		putn((long long)c, 8, 3);
	}
}


void
putn(unsigned long long n, unsigned int b, unsigned int c)
{
	register unsigned long long  d;

	if(!c)
		return;
	putn(n/b, b, c-1);
	d = n%b;
	if (d > 9)
		putchar(d-10+'a');
	else
		putchar(d+'0');
}

void
pre(int f, int n)
{
	int i,m;

	m = (max/(DBUFSIZE/n)) - f;
	for(i=0; i<m; i++)
		putchar(' ');
}

void
offset(char *s)
{
	register char *p;
	off64_t a;
	register int d;

	if (*s=='+')
		s++;
	if (*s=='x') {
		s++;
		base = 16;
	} else if (*s=='0' && s[1]=='x') {
		s += 2;
		base = 16;
	} else if (*s == '0')
		base = 8;
	p = s;
	while(*p) {
		if (*p++=='.')
			base = 10;
	}
	for (a=0; *s; s++) {
		d = *s;
		if(d>='0' && d<='9')
			a = a*base + d - '0';
		else if (d>='a' && d<='f' && base==16)
			a = a*base + d + 10 - 'a';
		else
			break;
	}
	if (*s == '.')
		s++;
	if(*s=='b' || *s=='B'){
		a *= BSIZE;
		s++;
	}
	if(*s=='l'|| *s=='L'){
		s++;
		if(*s=='l' || *s=='L')
			display=15;
	}

	addr = a;
}



u_long
string_to_ul(const char *str)
{
	char		*token;
	unsigned long	num;

	errno = 0;
	num = strtoul(str, &token, 0);
	if (errno)
		usage();

	if (token) {
		switch(*token) {
		case 'b':
			num *= BSIZE;
			break;
		case 'k':
			num *= KILO;
			break;
		case 'm':
			num *= MEG;
			break;
		default:
			if (!(isspace(*token) || *token == '\0'))
				usage();
		}
	}

	return num; 
}

int
file_vec_read(char *buf, off64_t max)
{
        static int      FirstCall = 0 ;

	off64_t num = 0;
	off64_t left = max;
	off64_t n;

        if(!I18N_SBCS_CODE) {
                if(FirstCall == 0 ) {
                        left -= BytesOfSPlitChar ;
                        max -= BytesOfSPlitChar ;
                        FirstCall++ ;
                }
        }
	while (num != max) {
		if (!file_vec_cur_opened) {

			if ((!file_vec_nul && file_vec_cur == file_vec_len) ||
					(file_vec_nul && file_vec_cur == 1))
				break;


			if (!file_vec_nul &&
			    freopen(file_vec[file_vec_cur],"r",stdin) == NULL) {
				pfmt(stderr, MM_ERROR,
					     PFMTTXT(_MSG_CANNOT_OPEN),
					     file_vec[file_vec_cur],
					     strerror(errno));
				exit(2);
			}
			file_vec_cur_opened = YES;
			file_vec_cur++;
		}

		if ((n = fread(buf+num, 1, left, stdin)) <= 0) {
			if (ferror(stdin))
				break;

			if (!file_vec_nul) fclose(stdin);
			file_vec_cur_opened = NO;
		}
		else {
			num += n;
			left -= n;
		}
	}
	return num;
}

int
file_vec_skip(off64_t skip)
{
	off64_t 	num = 0;
	off64_t 	left = skip;
	struct stat	stat_buf;

	while (num != skip) {
		if (!file_vec_cur_opened) {

			if ((!file_vec_nul && file_vec_cur == file_vec_len) ||
					(file_vec_nul && file_vec_cur == 1))
				break;

			if (!file_vec_nul &&
			    freopen(file_vec[file_vec_cur],"r",stdin) == NULL) {
				pfmt(stderr, MM_ERROR,
					     PFMTTXT(_MSG_CANNOT_OPEN),
					     file_vec[file_vec_cur],
					     strerror(errno));
				exit(2);
			}
			/* else, simply use stdin */

			if (fstat(0, &stat_buf) < 0) {
				pfmt(stderr, MM_ERROR,
					     PFMTTXT(_MSG_CANNOT_STAT),
					     file_vec[file_vec_cur],
					     strerror(errno));
				exit(2);
			}
			file_vec_cur_opened = YES;
			file_vec_cur++;

			if (stat_buf.st_mode == S_IFREG) {
				if (left > stat_buf.st_size){
					num += stat_buf.st_size;
					left -= stat_buf.st_size;
					if (!file_vec_nul) fclose(stdin);
					file_vec_cur_opened = NO;
					continue;
				}
				else {
					fseek64(stdin, left, 0);
					num += left;
					left = 0;
					continue;
				}
			}
		}

                if(I18N_SBCS_CODE) {
                        if (getchar() != EOF) {
                                num++;
                                left--;
                        }
                        else{
                                if (!file_vec_nul) fclose(stdin);
                                file_vec_cur_opened = NO;
                        }
                }
                else {
                        if(PrintAsChar == TRUE) {
                                BytesOfSPlitChar = mbFseek(stdin, skip) ;
                                if (fstat(0, &stat_buf) < 0) {
                                        pfmt(stderr, MM_ERROR,
                                                     PFMTTXT(_MSG_CANNOT_STAT),
                                                     file_vec[file_vec_cur-1],
                                                     strerror(errno));
                                        exit(2);
                                }
                                if(skip > stat_buf.st_size) {
                                        num = stat_buf.st_size ;
                                        if (!file_vec_nul) fclose(stdin);
                                        file_vec_cur_opened = NO;
                                }
                                else
                                        num = skip ;
                        }
                        else {
                                if (getchar() != EOF) {
                                        num++;
                                        left--;
                                }
                                else{
                                        if (!file_vec_nul) fclose(stdin);
                                        file_vec_cur_opened = NO;
                                }
                        }
                }

	}

	return num;
}

void
conv_add(struct dfmt *conv)
{
	if (conv_vec_len == conv_vec_max) {
		
		conv_vec_max += LIST_INCR;
		conv_vec = (struct dfmt **) realloc(conv_vec,
					conv_vec_max*sizeof(struct dfmt *));
		if (!conv_vec) {
			pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_OUT_OF_MEMORY));
			exit(2);
		}
	}

	conv_vec[conv_vec_len++] = conv;
}

void
conv_parse(char *arg)
{
	char		*c;

	for (c = arg; *c != '\0'; c++) {

		switch(*c) {

			/* num   : d f o u x
			   F D L : f
			   CSIL  : d o u x
			*/

		case 'a':
                        if(!I18N_SBCS_CODE)
                                PrintAsChar = TRUE ;
			conv_add(&ncr);
			break;

		case 'c':
                        if(!I18N_SBCS_CODE)
                                PrintAsChar = TRUE ;
			conv_add(&chr);
			break;

		case 'f':
			c++;
			if (*c == 'L' || *c == sizeof(long double) + '0')
				conv_add(&flt_l);
			else if (*c == 'F' || *c == sizeof(float) + '0')
				conv_add(&flt_f);
			else {
				if (*c != 'D' && *c != sizeof(double)+'0') c--;
				conv_add(&flt_d);
			}
			break;

		case 'd':
			c++;
			if (*c == 'C' || *c == sizeof(char) + '0')
				conv_add(&dec_c);
			else if (*c == 'S' || *c == sizeof(short) +'0')
				conv_add(&dec_s);
			else if (*c == 'L' || *c == sizeof(long) + '0')
				conv_add(&dec_l);
			else {
				if (*c != 'I' && *c != sizeof(int) + '0') c--;
				conv_add(&dec_i);
			}
			break;
		case 'o':
			c++;
			if (*c == 'C' || *c == sizeof(char) + '0')
				conv_add(&oct_c);
			else if (*c == 'S' || *c == sizeof(short) +'0')
				conv_add(&oct_s);
			else if (*c == 'L' || *c == sizeof(long) + '0')
				conv_add(&oct_l);
			else {
				if (*c != 'I' && *c != sizeof(int) + '0') c--;
				conv_add(&oct_i);
			}
			break;
		case 'u':
			c++;
			if (*c == 'C' || *c == sizeof(char) + '0')
				conv_add(&udc_c);
			else if (*c == 'S' || *c == sizeof(short) +'0')
				conv_add(&udc_s);
			else if (*c == 'L' || *c == sizeof(long) + '0')
				conv_add(&udc_l);
			else {
				if (*c != 'I' && *c != sizeof(int) + '0') c--;
				conv_add(&udc_i);
			}
			break;
		case 'x':
			c++;
			if (*c == 'C' || *c == sizeof(char) + '0')
				conv_add(&hex_c);
			else if (*c == 'S' || *c == sizeof(short) +'0')
				conv_add(&hex_s);
			else if (*c == 'L' || *c == sizeof(long) + '0')
				conv_add(&hex_l);
			else {
				if (*c != 'I' && *c != sizeof(int) + '0') c--;
				conv_add(&hex_i);
			}
			break;

		default:
			usage();
		}
	}
}

/* seek for the  first broken character				*/
/*   Parameters: fp: Pointer to the file from where characters	*/
/*                   are read					*/
/*   Return:     Remaining no of bytes of the first broken	*/
/*               character					*/
int mbFseek(FILE *fp, long NoOfBytes)
{
if (!I18N_SBCS_CODE) {
        int             bytes=0, BytesLeft ;
        char            *PStr, *Str, MbChar[MB_LEN_MAX*2] ;
        int             temp, cntr, PrevBuff = 0 ;
        wchar_t         WideChar ;
        long             ByteCount=0 ;
        long            Loc ;

        fseek(fp, 0, SEEK_END) ;
        Loc = ftell(fp) ;
        if(NoOfBytes > Loc)
                return (int)Loc ;
        rewind(fp) ;

        if(NoOfBytes == 0)
                return 0 ;
 	
	
	if ((Str=(char *)malloc(NoOfBytes+MB_CUR_MAX+1))==NULL) {
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_OUT_OF_MEMORY));
		exit(2);
	}
		
	while(ByteCount<NoOfBytes) {
		bytes=fread(Str+ByteCount, 1, NoOfBytes, fp);
		ByteCount+=bytes;
	}
	Str[NoOfBytes] = '\0' ;
	BytesLeft = NoOfBytes ;
	PStr = Str ;
	do {
		bytes = mbtowc(&WideChar, PStr, MB_CUR_MAX) ;
		if(bytes == 0) { /* Skipping nulls */
			bytes=1;
			BytesLeft -= bytes ;
			PStr += bytes ;
			if(BytesLeft == 0) {
				free(Str);
				return 0 ;
			}
			continue;
		}
		if(bytes != -1) {
			BytesLeft -= bytes ;
			PStr += bytes ;
			if(BytesLeft == 0) {
				free(Str);
				return 0 ;
			}
		}
		else {
			int i;
			if ((PStr-Str)<=(NoOfBytes-MB_CUR_MAX)) {
				free(Str);
                           	pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_OD_FSEEKMB_ERROR), MB_LEN_MAX);
				exit(2);
			}
			BytesLeft = strlen(PStr) ;
			for(cntr = 0; cntr < BytesLeft; cntr++)
				MbChar[cntr] = PStr[cntr] ;

			bytes=fread(Str ,1, MB_CUR_MAX-1, fp);

			for(cntr = BytesLeft; cntr <= BytesLeft+bytes+1; cntr++)
				MbChar[cntr] = Str[cntr-BytesLeft] ;
			MbChar[cntr]='\0';
			temp = mbtowc(&WideChar, MbChar, MB_CUR_MAX) ;
			if(temp == -1) {
				free(Str);
				pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_OD_FSEEKMB_ERROR), MB_LEN_MAX);
				exit(2);

			}
			else
				fseek(fp, -bytes, SEEK_CUR) ;
			free(Str);
			return (temp-BytesLeft) ;
		}
	}
	while(1) ;	
}
return -1;
}
