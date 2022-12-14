/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Multibyte support added for the following cases:	*/
/*	- LCREBLOCK : Option conv=lcase				*/
/*	- UCREBLOCK : Option conv=ucase				*/
/*	- LCUNBLOCK : Option conv=lcase,unblock			*/
/*	- UCUNBLOCK : Option conv=ucase,unblock			*/
/*	- LCBLOCK : Option conv=lcase,block			*/
/*	- UCBLOCK : Option conv=ucase,block			*/

#ident	"$Revision: 1.18 $"
/* Modified to support EUC Multibyte/Big5-Sjis/Full multibyte */

/*
**	convert and copy
*/

#include	<stdio.h>
#include	<signal.h>
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>
#include        <sys/fcntl.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<string.h>
#include	<errno.h>
#include        <sgi_nl.h>
#include        <msgs/uxsgicore.h>
#include        <msgs/uxcore.abi.h>
#include	<i18n_capable.h>


/* The BIG parameter is machine dependent.  It should be a long integer	*/
/* constant that can be used by the number parser to check the validity	*/
/* of numeric parameters.  On 16-bit machines, it should probably be	*/
/* the maximum unsigned integer, 0177777L.  On 32-bit machines where	*/
/* longs are the same size as ints, the maximum signed integer is more	*/
/* appropriate.  This value is 017777777777L.				*/

#ifdef __sgi
#define	BIGLL	0x7FFFFFFFFFFFFFFFLL
#define	DD_BSIZE	512	/* dd's default block size */
#endif /* __sgi */
#define	BIG	017777777777L

/* Option parameters */

#define COPY		0	/* file copy, preserve input block size */
#define	REBLOCK		1	/* file copy, change block size */
#define	LCREBLOCK	2	/* file copy, convert to lower case */
#define	UCREBLOCK	3	/* file copy, convert to upper case */
#define NBASCII		4	/* file copy, convert from EBCDIC to ASCII */
#define LCNBASCII	5	/* file copy, EBCDIC to lower case ASCII */
#define UCNBASCII	6	/* file copy, EBCDIC to upper case ASCII */
#define NBEBCDIC	7	/* file copy, convert from ASCII to EBCDIC */
#define LCNBEBCDIC	8	/* file copy, ASCII to lower case EBCDIC */
#define UCNBEBCDIC	9	/* file copy, ASCII to upper case EBCDIC */
#define NBIBM		10	/* file copy, convert from ASCII to IBM */
#define LCNBIBM		11	/* file copy, ASCII to lower case IBM */
#define UCNBIBM		12	/* file copy, ASCII to upper case IBM */
#define	UNBLOCK		13	/* convert blocked ASCII to ASCII */
#define	LCUNBLOCK	14	/* convert blocked ASCII to lower case ASCII */
#define	UCUNBLOCK	15	/* convert blocked ASCII to upper case ASCII */
#define	ASCII		16	/* convert blocked EBCDIC to ASCII */
#define	LCASCII		17	/* convert blocked EBCDIC to lower case ASCII */
#define	UCASCII		18	/* convert blocked EBCDIC to upper case ASCII */
#define	BLOCK		19	/* convert ASCII to blocked ASCII */
#define	LCBLOCK		20	/* convert ASCII to lower case blocked ASCII */
#define	UCBLOCK		21	/* convert ASCII to upper case blocked ASCII */
#define	EBCDIC		22	/* convert ASCII to blocked EBCDIC */
#define	LCEBCDIC	23	/* convert ASCII to lower case blocked EBCDIC */
#define	UCEBCDIC	24	/* convert ASCII to upper case blocked EBCDIC */
#define	IBM		25	/* convert ASCII to blocked IBM */
#define	LCIBM		26	/* convert ASCII to lower case blocked IBM */
#define	UCIBM		27	/* convert ASCII to upper case blocked IBM */
#define	LCASE		01	/* flag - convert to lower case */
#define	UCASE		02	/* flag - convert to upper case */
#define	SWAB		04	/* flag - swap bytes before conversion */
#define NERR		010	/* flag - proceed on input errors */
#define SYNC            020     /* flag - pad short input blocks with spaces */
#define NOTRUNC         040     /* flag - XXX */
#define IGNERR		0100	/* "ignore" (zero-fill output) errors */

#define BADLIMIT	5	/* give up if no progress after BADLIMIT tries */

int badlimit = BADLIMIT;	/* no limit if conv=ignerr */

/* Global references */

/* Local routine declarations */

int		match();
void	term();
unsigned int	number();
#ifdef __sgi
unsigned long long bignumber();
#endif /* __sgi */
unsigned char	*flsh();
void		stats();

/* Local data definitions */

static unsigned ibs;	/* input buffer size */
static unsigned obs;	/* output buffer size */
static unsigned bs;	/* buffer size, overrules ibs and obs */
static unsigned cbs;	/* conversion buffer size, used for block conversions */
static unsigned ibc;	/* number of bytes still in the input buffer */
static unsigned obc;	/* number of bytes in the output buffer */
static unsigned cbc;	/* number of bytes in the conversion buffer */

static int	ibf;	/* input file descriptor */
static int	obf;	/* output file descriptor */
static int	cflag;	/* conversion option flags */
static int	skipf;	/* if skipf == 1, skip rest of input line */
static long long	nifr;	/* count of full input records */
static int	nipr;	/* count of partial input records */
static long long	nofr;	/* count of full output records */
static int	nopr;	/* count of partial output records */
static int	ntrunc;	/* count of truncated input lines */
static int	nbad;	/* count of bad records since last good one */
static int	files;	/* number of input files to concatenate (tape only) */
static int	skip;	/* number of input records to skip */
#ifdef __sgi
static long long	iseekn;	/* number of input records to seek past */
static long long	oseekn;	/* number of output records to seek past */
#else
static int	iseekn;	/* number of input records to seek past */
static int	oseekn;	/* number of output records to seek past */
#endif /* __sgi */
static int	count;	/* number of input records to copy (0 = all) */

static char		*string;	/* command arg pointer */
static char		*ifile;		/* input file name pointer */
static char		*ofile;		/* output file name pointer */
static unsigned char	*ibuf;		/* input buffer pointer */
static unsigned char	*obuf;		/* output buffer pointer */

static int ncount = 0;
static char *old_string_buf;
static int old_string_len;
static int temp_str_len;


/* This is an EBCDIC to ASCII conversion table	*/
/* from a proposed BTL standard April 16, 1979	*/

static unsigned char etoa [] =
{
	0000,0001,0002,0003,0234,0011,0206,0177,
	0227,0215,0216,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0235,0205,0010,0207,
	0030,0031,0222,0217,0034,0035,0036,0037,
	0200,0201,0202,0203,0204,0012,0027,0033,
	0210,0211,0212,0213,0214,0005,0006,0007,
	0220,0221,0026,0223,0224,0225,0226,0004,
	0230,0231,0232,0233,0024,0025,0236,0032,
	0040,0240,0241,0242,0243,0244,0245,0246,
	0247,0250,0325,0056,0074,0050,0053,0174,
	0046,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0041,0044,0052,0051,0073,0176,
	0055,0057,0262,0263,0264,0265,0266,0267,
	0270,0271,0313,0054,0045,0137,0076,0077,
	0272,0273,0274,0275,0276,0277,0300,0301,
	0302,0140,0072,0043,0100,0047,0075,0042,
	0303,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0304,0305,0306,0307,0310,0311,
	0312,0152,0153,0154,0155,0156,0157,0160,
	0161,0162,0136,0314,0315,0316,0317,0320,
	0321,0345,0163,0164,0165,0166,0167,0170,
	0171,0172,0322,0323,0324,0133,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0135,0346,0347,
	0173,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0350,0351,0352,0353,0354,0355,
	0175,0112,0113,0114,0115,0116,0117,0120,
	0121,0122,0356,0357,0360,0361,0362,0363,
	0134,0237,0123,0124,0125,0126,0127,0130,
	0131,0132,0364,0365,0366,0367,0370,0371,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0372,0373,0374,0375,0376,0377,
};

/* This is an ASCII to EBCDIC conversion table	*/
/* from a proposed BTL standard April 16, 1979	*/

static unsigned char atoe [] =
{
	0000,0001,0002,0003,0067,0055,0056,0057,
	0026,0005,0045,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0074,0075,0062,0046,
	0030,0031,0077,0047,0034,0035,0036,0037,
	0100,0132,0177,0173,0133,0154,0120,0175,
	0115,0135,0134,0116,0153,0140,0113,0141,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0172,0136,0114,0176,0156,0157,
	0174,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0321,0322,0323,0324,0325,0326,
	0327,0330,0331,0342,0343,0344,0345,0346,
	0347,0350,0351,0255,0340,0275,0232,0155,
	0171,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0221,0222,0223,0224,0225,0226,
	0227,0230,0231,0242,0243,0244,0245,0246,
	0247,0250,0251,0300,0117,0320,0137,0007,
	0040,0041,0042,0043,0044,0025,0006,0027,
	0050,0051,0052,0053,0054,0011,0012,0033,
	0060,0061,0032,0063,0064,0065,0066,0010,
	0070,0071,0072,0073,0004,0024,0076,0341,
	0101,0102,0103,0104,0105,0106,0107,0110,
	0111,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0142,0143,0144,0145,0146,0147,
	0150,0151,0160,0161,0162,0163,0164,0165,
	0166,0167,0170,0200,0212,0213,0214,0215,
	0216,0217,0220,0152,0233,0234,0235,0236,
	0237,0240,0252,0253,0254,0112,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0241,0276,0277,
	0312,0313,0314,0315,0316,0317,0332,0333,
	0334,0335,0336,0337,0352,0353,0354,0355,
	0356,0357,0372,0373,0374,0375,0376,0377,
};

/* Table for ASCII to IBM (alternate EBCDIC) code conversion	*/

static unsigned char atoibm[] =
{
	0000,0001,0002,0003,0067,0055,0056,0057,
	0026,0005,0045,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0074,0075,0062,0046,
	0030,0031,0077,0047,0034,0035,0036,0037,
	0100,0132,0177,0173,0133,0154,0120,0175,
	0115,0135,0134,0116,0153,0140,0113,0141,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0172,0136,0114,0176,0156,0157,
	0174,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0321,0322,0323,0324,0325,0326,
	0327,0330,0331,0342,0343,0344,0345,0346,
	0347,0350,0351,0255,0340,0275,0137,0155,
	0171,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0221,0222,0223,0224,0225,0226,
	0227,0230,0231,0242,0243,0244,0245,0246,
	0247,0250,0251,0300,0117,0320,0241,0007,
	0040,0041,0042,0043,0044,0025,0006,0027,
	0050,0051,0052,0053,0054,0011,0012,0033,
	0060,0061,0032,0063,0064,0065,0066,0010,
	0070,0071,0072,0073,0004,0024,0076,0341,
	0101,0102,0103,0104,0105,0106,0107,0110,
	0111,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0142,0143,0144,0145,0146,0147,
	0150,0151,0160,0161,0162,0163,0164,0165,
	0166,0167,0170,0200,0212,0213,0214,0215,
	0216,0217,0220,0232,0233,0234,0235,0236,
	0237,0240,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0312,0313,0314,0315,0316,0317,0332,0333,
	0334,0335,0336,0337,0352,0353,0354,0355,
	0356,0357,0372,0373,0374,0375,0376,0377,
};

/* Table for conversion of ASCII to lower case ASCII	*/

static unsigned char utol[] =
{
	0000,0001,0002,0003,0004,0005,0006,0007,
	0010,0011,0012,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0024,0025,0026,0027,
	0030,0031,0032,0033,0034,0035,0036,0037,
	0040,0041,0042,0043,0044,0045,0046,0047,
	0050,0051,0052,0053,0054,0055,0056,0057,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0072,0073,0074,0075,0076,0077,
	0100,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0152,0153,0154,0155,0156,0157,
	0160,0161,0162,0163,0164,0165,0166,0167,
	0170,0171,0172,0133,0134,0135,0136,0137,
	0140,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0152,0153,0154,0155,0156,0157,
	0160,0161,0162,0163,0164,0165,0166,0167,
	0170,0171,0172,0173,0174,0175,0176,0177,
	0200,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0212,0213,0214,0215,0216,0217,
	0220,0221,0222,0223,0224,0225,0226,0227,
	0230,0231,0232,0233,0234,0235,0236,0237,
	0240,0241,0242,0243,0244,0245,0246,0247,
	0250,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0300,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0312,0313,0314,0315,0316,0317,
	0320,0321,0322,0323,0324,0325,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0345,0346,0347,
	0350,0351,0352,0353,0354,0355,0356,0357,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0372,0373,0374,0375,0376,0377,
};

/* Table for conversion of ASCII to upper case ASCII	*/

static unsigned char ltou[] =
{
	0000,0001,0002,0003,0004,0005,0006,0007,
	0010,0011,0012,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0024,0025,0026,0027,
	0030,0031,0032,0033,0034,0035,0036,0037,
	0040,0041,0042,0043,0044,0045,0046,0047,
	0050,0051,0052,0053,0054,0055,0056,0057,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0072,0073,0074,0075,0076,0077,
	0100,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0112,0113,0114,0115,0116,0117,
	0120,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0132,0133,0134,0135,0136,0137,
	0140,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0112,0113,0114,0115,0116,0117,
	0120,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0132,0173,0174,0175,0176,0177,
	0200,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0212,0213,0214,0215,0216,0217,
	0220,0221,0222,0223,0224,0225,0226,0227,
	0230,0231,0232,0233,0234,0235,0236,0237,
	0240,0241,0242,0243,0244,0245,0246,0247,
	0250,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0300,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0312,0313,0314,0315,0316,0317,
	0320,0321,0322,0323,0324,0325,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0345,0346,0347,
	0350,0351,0352,0353,0354,0355,0356,0357,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0372,0373,0374,0375,0376,0377,
};

main(argc, argv)
int argc;
char **argv;
{
	register unsigned char *ip, *op;/* input and output buffer pointers */
	register int c;			/* character counter */
	register int ic;		/* input character */
	register int conv;		/* conversion option code */
        static char null_string[1];     /* "", but you can take its address */
	
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
#define CMD_LABEL "UX:dd"
        (void)setlabel(CMD_LABEL);

	/* Set option defaults */

#ifdef __sgi
	ibs = DD_BSIZE;
	obs = DD_BSIZE;
#else
	ibs = BSIZE;
	obs = BSIZE;
#endif /* __sgi */
	files = 1;
	conv = COPY;
	old_string_buf = (char *)calloc( MB_CUR_MAX+1, 1);

	/* Parse command options */

	for (c = 1; c < argc; c++)
	{
		string = argv[c];
		if (match("ibs="))
		{
			ibs = number(BIG);
			continue;
		}
		if (match("obs="))
		{
			obs = number(BIG);
			continue;
		}
		if (match("cbs="))
		{
			cbs = number(BIG);
			continue;
		}
		if (match("bs="))
		{
			bs = number(BIG);
			continue;
		}
		if (match("if="))
		{
			ifile = string;
			continue;
		}
		if (match("of="))
		{
			ofile = string;
			continue;
		}

		if (match("skip="))
		{
			skip = number(BIG);
			continue;
		}
		if (match("iseek="))
		{
#ifdef __sgi
			iseekn = bignumber(BIGLL);
#else
			iseekn = number(BIG);
#endif /* __sgi */
			continue;
		}
		if (match("oseek="))
		{
#ifdef __sgi
			oseekn = bignumber(BIGLL);
#else
			oseekn = number(BIG);
#endif /* __sgi */
			continue;
		}
		if (match("seek="))		/* retained for compatibility */
		{
#ifdef __sgi
			oseekn = bignumber(BIGLL);
#else
			oseekn = number(BIG);
#endif /* __sgi */
			continue;
		}
		if (match("count="))
		{
			count = number(BIG);
			continue;
		}
		if (match("files="))
		{
			files = number(BIG);
			continue;
		}
		if (match("conv="))
		{
			for (;;)
			{
				if (match(","))
				{
					continue;
				}
				if (*string == '\0')
				{
					break;
				}
				if (match("block"))
				{
					conv = BLOCK;
					continue;
				}
				if (match("unblock"))
				{
					conv = UNBLOCK;
					continue;
				}

				if (match("ebcdic"))
				{
					conv = EBCDIC;
					continue;
				}
				if (match("ibm"))
				{
					conv = IBM;
					continue;
				}
				if (match("ascii"))
				{
					conv = ASCII;
					continue;
				}
				if (match("lcase"))
				{
					cflag |= LCASE;
					continue;
				}
				if (match("ucase"))
				{
					cflag |= UCASE;
					continue;
				}
				if (match("swab"))
				{
					cflag |= SWAB;
					continue;
				}
				if (match("noerror"))
				{
					cflag |= NERR;
					continue;
				}
				if (match("ignerror"))
				{
					cflag |= IGNERR;
					badlimit = 0;
					continue;
				}
				if (match("sync"))
				{
					cflag |= SYNC;
					continue;
				}
                                if (match("notrunc"))
                                {
                                        cflag |= NOTRUNC;
                                        continue;
                                }

				goto badarg;
			}
			continue;
		}
		badarg:
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_BAD_ARGUMENT), string); 
		exit(2);
	}

	/* Perform consistency checks on options, decode strange conventions */

	if (bs)
	{
		ibs = obs = bs;
	}
	if ((ibs == 0) || (obs == 0))
	{
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_DD_BUFFER_SIZES_CANNOT)); 
		exit(2);
	}
	if (conv == COPY)
	{
		if (cbs != 0)
		{
			pfmt(stderr, MM_ERROR,
				PFMTTXT(_MSG_DD_CBS_MUST_BE_ZERO)); 
			exit(2);
		}
		if ((bs == 0) || (cflag&(LCASE|UCASE)))
		{
			conv = REBLOCK;
		}
	}
	if (cbs == 0)
	{
		switch (conv)
		{
		case BLOCK:
		case UNBLOCK:
			conv = REBLOCK;
			break;

		case ASCII:
			conv = NBASCII;
			break;

		case EBCDIC:
			conv = NBEBCDIC;
			break;

		case IBM:
			conv = NBIBM;
			break;
		}
	}

	/* Expand options into lower and upper case versions if necessary */

	switch (conv)
	{
	case REBLOCK:
		if (cflag&LCASE)
		{
			conv = LCREBLOCK;
		}
		else if (cflag&UCASE)
		{
			conv = UCREBLOCK;
		}
		break;

	case UNBLOCK:
		if (cflag&LCASE)
		{
			conv = LCUNBLOCK;
		}
		else if (cflag&UCASE)
		{
			conv = UCUNBLOCK;
		}
		break;

	case BLOCK:
		if (cflag&LCASE)
		{
			conv = LCBLOCK;
		}
		else if (cflag&UCASE)
		{
			conv = UCBLOCK;
		}
		break;

	case ASCII:
		if (cflag&LCASE)
		{
			conv = LCASCII;
		}
		else if (cflag&UCASE)
		{
			conv = UCASCII;
		}
		break;

	case NBASCII:
		if (cflag&LCASE)
		{
			conv = LCNBASCII;
		}
		else if (cflag&UCASE)
		{
			conv = UCNBASCII;
		}
		break;

	case EBCDIC:
		if (cflag&LCASE)
		{
			conv = LCEBCDIC;
		}
		else if (cflag&UCASE)
		{
			conv = UCEBCDIC;
		}
		break;

	case NBEBCDIC:
		if (cflag&LCASE)
		{
			conv = LCNBEBCDIC;
		}
		else if (cflag&UCASE)
		{
			conv = UCNBEBCDIC;
		}
		break;

	case IBM:
		if (cflag&LCASE)
		{
			conv = LCIBM;
		}
		else if (cflag&UCASE)
		{
			conv = UCIBM;
		}
		break;

	case NBIBM:
		if (cflag&LCASE)
		{
			conv = LCNBIBM;
		}
		else if (cflag&UCASE)
		{
			conv = UCNBIBM;
		}
		break;
	}

	/* Open the input file, or duplicate standard input */

	ibf = -1;
	if (ifile)
	{
		ibf = open(ifile, 0);
	}
#ifndef STANDALONE
	else
	{
		ifile = "";
		ibf = dup(0);
	}
#endif
	if (ibf == -1)
	{
		pfmt(stderr, MM_ERROR,
			PFMTTXT(_MSG_CANNOT_OPEN), ifile, 
			strerror(errno));
		exit(2);
	}

	/* Open the output file, or duplicate standard output */

	obf = -1;
	if (ofile)
	{
                obf = open(ofile,
                        O_WRONLY | O_CREAT | \
                                ((oseekn || (cflag & NOTRUNC)) ? 0 : O_TRUNC),
                        (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH));

	}
#ifndef STANDALONE
	else
	{
		ofile = null_string;	
		obf = dup(1);
	}
#endif
	if (obf == -1)
	{
		pfmt(stderr, MM_ERROR,
			PFMTTXT(_MSG_CANNOT_CREATE), ofile, 
			strerror(errno));
		exit(2);
	}

	/* Expand memory to get an input buffer */

	ibuf = (unsigned char *)malloc(ibs + 10);

	/* If no conversions, the input buffer is the output buffer */

	if (conv == COPY)
	{
		obuf = ibuf;
	}

	/* Expand memory to get an output buffer.  Leave enough room at the	*/
	/* end to convert a logical record when doing block conversions.	*/

	else
	{
		obuf = (unsigned char *)malloc(obs + cbs + 10);
	}
	if ((ibuf == (unsigned char *)NULL) || (obuf == (unsigned char *)NULL))
	{
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_NOT_ENOUGH_MEMORY), 
			strerror(errno));
		exit(2);
	}

	/* Enable a statistics message on SIGINT */

#ifndef STANDALONE
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	{
		(void)signal(SIGINT, term);
	}
#endif

	/* Skip input blocks */

	while (skip)
	{
		ibc = read(ibf, (char *)ibuf, ibs);
		if (ibc == (unsigned)-1)
		{
			if (badlimit && ++nbad > badlimit)
			{
				pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_SKIP_FAILED), 
					strerror(errno));
				exit(2);
			}
			else
			{
				pfmt(stderr, MM_ERROR,
					PFMTTXT(_MSG_DD_READ_ERROR_DURING), 
					strerror(errno));
			}
		}
		else
		{
			if (ibc == 0)
			{
				pfmt(stderr, MM_ERROR,
					PFMTTXT(_MSG_DD_CANNOT_SKIP_PAST), 
					strerror(errno));
				exit(3);
			}
			else
			{
				nbad = 0;
			}
		}
		skip--;
	}

	/* Seek past input blocks */

#ifdef __sgi
	if (iseekn && lseek64(ibf, ((off64_t) iseekn) * ((off64_t) ibs), 1) == -1)
#else
	if (iseekn && lseek(ibf, ((off_t) iseekn) * ((off_t) ibs), 1) == -1)
#endif /* __sgi */
	{
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_INPUT_SEEK_ERROR), 
			strerror(errno));
		exit(2);
	}

	/* Seek past output blocks */

#ifdef __sgi
	if (oseekn && lseek64(obf, ((off64_t) oseekn) * ((off64_t) obs), 1) == -1)
#else
	if (oseekn && lseek(obf, ((off_t) oseekn) * ((off_t) obs), 1) == -1)
#endif /* __sgi */
	{
		pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_OUTPUT_SEEK_ERROR), 
			strerror(errno));
		exit(2);
	}
        if (oseekn && (cflag & NOTRUNC) != NOTRUNC && ofile != null_string) {
#ifdef __sgi
                if (ftruncate64(obf, ((off64_t) oseekn) * ((off64_t) obs))
                                                                        == -1)
#else
                if (ftruncate(obf, ((off_t) oseekn) * ((off_t) obs)) == -1)
#endif /* __sgi */
                {
			/*
			 * Ignore errors from devices which don't
			 * support truncatation.
			 */
			if (errno != EINVAL) {
                        	(void)setcat("uxsgicore");
                        	_sgi_nl_usage(SGINL_NOSYSERR,CMD_LABEL,
                                	GETTXT(_MSG_MMX_dd_trucat_err), 
                                	ofile,
                                	strerror(errno));
                        	exit(2);
                	}
                }
        }

	/* Initialize all buffer pointers */

	skipf = 0;	/* not skipping an input line */
	ibc = 0;	/* no input characters yet */
	obc = 0;	/* no output characters yet */
	cbc = 0;	/* the conversion buffer is empty */
	op = obuf;	/* point to the output buffer */

	/* Read and convert input blocks until end of file(s) */

	for (;;)
	{
		if ((count == 0) || (nifr+nipr < count))
		{
			/* If proceed on error is enabled, zero the input buffer */

			if (cflag&(IGNERR|NERR))
			{
				ip = ibuf + ibs;
				c = ibs;
				if (c & 1)		/* if the size is odd, */
				{
					*--ip = 0;	/* clear the odd byte */
				}
				if (c >>= 1)		/* divide by two */
				{
					do {		/* clear two at a time */
						*--ip = 0;
						*--ip = 0;
					} while (--c);
				}
			}

			/* Read the next input block */

			ibc = read(ibf, (char *)ibuf, ibs);

			if ( !(I18N_SBCS_CODE) )
				*(ibuf+ibc) = 0; /* Put a null at the end of the block, assuming null cannot
						    occur in the second/third byte of a multibyte character */

			/* Process input errors */

			if (ibc == (unsigned)-1)
			{
				pfmt(stderr, MM_ERROR,
					PFMTTXT(_MSG_READ_ERROR_S), strerror(errno)); 
				if (   ((cflag&(IGNERR|NERR)) == 0)
				    || (badlimit && ++nbad > badlimit) )
				{
					while (obc)
					{
						(void)flsh();
					}
					term(2);
				}
				else
				{
					stats();
					if(cflag&IGNERR) {
						/* try to lseek over the bad read; assumes something
						 * like a disk, where this makes sense... */
						if(lseek64(ibf, ibs, SEEK_CUR) == -1)
							perror(GETTXT(_MSG_DD_COULDNT_SEEK_OVER_FAILED)); 
					}
					ibc = ibs;	/* mark as a full block */
				}
			}
			else
			{
				nbad = 0;
			}
		}

		/* Record count satisfied, simulate end of file */

		else
		{
			ibc = 0;
			files = 1;
		}

		/* Process end of file */

		if (ibc == 0)
		{
			switch (conv)
			{
			case UNBLOCK:
			case LCUNBLOCK:
			case UCUNBLOCK:
			case ASCII:
			case LCASCII:
			case UCASCII:

				/* Trim trailing blanks from the last line */

				if ((c = cbc) != 0)
				{
					do {
						if ((*--op) != ' ')
						{
							op++;
							break;
						}
					} while (--c);
					*op++ = '\n';
					obc -= cbc - c - 1;
					cbc = 0;

					/* Flush the output buffer if full */

					while (obc >= obs)
					{
						op = flsh();
					}
				}
				break;

			case BLOCK:
			case LCBLOCK:
			case UCBLOCK:
			case EBCDIC:
			case LCEBCDIC:
			case UCEBCDIC:
			case IBM:
			case LCIBM:
			case UCIBM:

				/* Pad trailing blanks if the last line is short */

				if (cbc)
				{
					obc += c = cbs - cbc;
					cbc = 0;
					if (c > 0)
					{
						/* Use the right kind of blank */

						switch (conv)
						{
						case BLOCK:
						case LCBLOCK:
						case UCBLOCK:
							ic = ' ';
							break;

						case EBCDIC:
						case LCEBCDIC:
						case UCEBCDIC:
							ic = atoe[' '];
							break;

						case IBM:
						case LCIBM:
						case UCIBM:
							ic = atoibm[' '];
							break;
						}

						/* Pad with trailing blanks */

						do {
							*op++ = ic;
						} while (--c);
					}
				}

				/* Flush the output buffer if full */

				while (obc >= obs)
				{
					op = flsh();
				}
				break;
			}

			/* If no more files to read, flush the output buffer */

			if (--files <= 0)
			{
				(void)flsh();
				term(0);	/* successful exit */
			}
			else
			{
				continue;	/* read the next file */
			}
		}

		/* Normal read, check for special cases */

		else if (ibc == ibs)
		{
			nifr++;		/* count another full input record */
		}
		else
		{
			nipr++;		/* count a partial input record */

			/* If `sync' enabled, pad nulls */

			if ((cflag&SYNC) && ((cflag&(IGNERR|NERR)) == 0))
			{
                                int pad_char = 0;

                                if(conv == BLOCK || conv == UNBLOCK)
                                        pad_char = ' ';

				c = ibs - ibc;
				ip = ibuf + ibs;
				do {
					*--ip = pad_char;
				} while (--c);
				ibc = ibs;
			}
		}

		/* Swap the bytes in the input buffer if necessary */

		if (cflag&SWAB)
		{
			ip = ibuf;
			if (ibc & 1)		/* if the byte count is odd, */
			{
				ip[ibc] = 0;	/* make it even, pad with zero */
			}
			c = (ibc + 1) >> 1;	/* compute the pair count */
			do {
				ic = *ip++;
				ip[-1] = *ip;
				*ip++ = ic;
			} while (--c);		/* do two bytes at a time */
		}

		/* Select the appropriate conversion loop */

		ip = ibuf;
		switch (conv)
		{

		/* Simple copy: no conversion, preserve the input block size */

		case COPY:
			obc = ibc;
			(void)flsh();
			break;

		/* Simple copy: pack all output into equal sized blocks */

		case REBLOCK:
		case LCREBLOCK:
		case UCREBLOCK:
		case NBASCII:
		case LCNBASCII:
		case UCNBASCII:
		case NBEBCDIC:
		case LCNBEBCDIC:
		case UCNBEBCDIC:
		case NBIBM:
		case LCNBIBM:
		case UCNBIBM:
			while ((c = ibc) != 0)
			{
				if (c > (obs - obc))
				{
					c = obs - obc;
				}
				ibc -= c;
				obc += c;
				switch (conv)
				{
				case REBLOCK:
					do {
						*op++ = *ip++;
					} while (--c);
					break;

				case LCREBLOCK:
					/* Convert lower case characters to upper case characters, 
						do not convert multibyte characters */
					do {
					     if ( I18N_SBCS_CODE )
					     {
						*op++ = utol[*ip++];
					     }
					     else
					     { 
						if( ncount == -1)  /* Process last iteration's pending bytes of a character */
						{
							memcpy( &old_string_buf[old_string_len], ip, MB_CUR_MAX); /* append new bytes to saved bytes */
							ncount = mblen( old_string_buf, MB_CUR_MAX); /* find mblen */
							temp_str_len = ncount-old_string_len; /* find the no of pending bytes */
							memcpy( op, ip, temp_str_len);	 /* Copy the pending bytes */
							op+=temp_str_len; /* Update counters and pointer */
							ip+=temp_str_len;
							c-= (temp_str_len-1);
							ncount = 0;
							continue;
						}
						ncount = mblen( (char *)ip, MB_CUR_MAX);
						switch( ncount)
						{
							case -1: /* Character is getting split at the block boundary */
								memcpy( op, ip, c); /* Copy the bytes of the character in the current block */
								memcpy( old_string_buf, ip, c); /* Save these bytes */
								old_string_len = c; /* Update counters and pointer */
								op+=c;
								ip+=c;
								c = 1;
								break;
							case 0: /* Process null characters */
								*op++ = *ip++;
								break;
							case 1: /* Process single byte characters */
								if(isascii(*ip))
									*op++ = tolower(*ip++);
								else
									*op++ = *ip++;
								break;
							default: /* Process multi byte characters */
								if( c >= ncount) /* There is enough space in the output buffer for the character */
								{
									memcpy( op, ip, ncount);
									op+=ncount;
									ip+=ncount;
									c-=(ncount-1);
								}
								else /* There is NOT enough space in the output buffer for the character */
								{
									ncount = -1;
									memcpy( op, ip, c);/* Copy the bytes in the current block */
									memcpy( old_string_buf, ip, c); /* Save these bytes */
									old_string_len = c; /* Update counters and pointer */
									op+=c;
									ip+=c;
									c = 1;
								}
								break;
						}
					     }
					} while (--c);
					break;

				case UCREBLOCK:
					/* Same as above case except toupper is used instead of tolower */
					do {
					     if ( I18N_SBCS_CODE )
					     {
						*op++ = ltou[*ip++];
					     }
					     else
					     {
						if( ncount == -1) 
						{
							memcpy( &old_string_buf[old_string_len], ip, MB_CUR_MAX);
							ncount = mblen( old_string_buf, MB_CUR_MAX);
							temp_str_len = ncount-old_string_len;
							memcpy( op, ip, temp_str_len);	
							op+=temp_str_len;
							ip+=temp_str_len;
							c-= (temp_str_len-1);
							ncount = 0;
							continue;
						}
						ncount = mblen( (char *)ip, MB_CUR_MAX);
						switch( ncount)
						{
							case -1:
								memcpy( op, ip, c);
								memcpy( old_string_buf, ip, c);
								old_string_len = c;
								op+=c;
								ip+=c;
								c = 1;
								break;
							case 0:
								*op++ = *ip++;
								break;
							case 1:
								if(isascii(*ip))
									*op++ = toupper(*ip++);
								else
									*op++ = *ip++;
								break;
							default:
								if( c >= ncount)
								{
									memcpy( op, ip, ncount);
									op+=ncount;
									ip+=ncount;
									c-=(ncount-1);
								}
								else
								{
									ncount = -1;
									memcpy( op, ip, c);
									memcpy( old_string_buf, ip, c);
									old_string_len = c;
									op+=c;
									ip+=c;
									c = 1;
								}
								break;
						}
					     } 
					} while (--c);
					break;

				case NBASCII:
					do {
						*op++ = etoa[*ip++];
					} while (--c);
					break;

				case LCNBASCII:
					do {
						*op++ = utol[etoa[*ip++]];
					} while (--c);
					break;

				case UCNBASCII:
					do {
						*op++ = ltou[etoa[*ip++]];
					} while (--c);
					break;

				case NBEBCDIC:
					do {
						*op++ = atoe[*ip++];
					} while (--c);
					break;

				case LCNBEBCDIC:
					do {
						*op++ = atoe[utol[*ip++]];
					} while (--c);
					break;

				case UCNBEBCDIC:
					do {
						*op++ = atoe[ltou[*ip++]];
					} while (--c);
					break;

				case NBIBM:
					do {
						*op++ = atoibm[*ip++];
					} while (--c);
					break;

				case LCNBIBM:
					do {
						*op++ = atoibm[utol[*ip++]];
					} while (--c);
					break;

				case UCNBIBM:
					do {
						*op++ = atoibm[ltou[*ip++]];
					} while (--c);
					break;
				}
				if (obc >= obs)
				{
					op = flsh();
				}
			}
			break;

		/* Convert from blocked records to lines terminated by newline */

		case UNBLOCK:
		case LCUNBLOCK:
		case UCUNBLOCK:
		case ASCII:
		case LCASCII:
		case UCASCII:
			while ((c = ibc) != 0)
			{
				if (c > (cbs - cbc))	/* if more than one record, */
				{
					c = cbs - cbc;	/* only copy one record */
				}
				ibc -= c;
				cbc += c;
				obc += c;
				switch (conv)
				{
				case UNBLOCK:
					do {
						*op++ = *ip++;
					} while (--c);
					break;

				case LCUNBLOCK:
					/* Convert lower case to upper case, do not convert multibyte characters */
					do {
					     if ( I18N_SBCS_CODE )
					     {
						*op++ = utol[*ip++];
					     }
					     else
					     {
						if( ncount == -1) /* Process last iteration's pending bytes of a character */
						{
							memcpy( &old_string_buf[old_string_len], ip, MB_CUR_MAX); /* append new bytes to saved bytes */
							ncount = mblen( old_string_buf, MB_CUR_MAX); /* find mblen */
							temp_str_len = ncount-old_string_len; /* find the no of pending bytes */
							if( cbs > temp_str_len) /* Enough space in Conversion block for character */
								memcpy( op, ip, temp_str_len); /* Copy the pending bytes */
							else                   /* NOT enough space in Conversion block for character */
								memset( op, ' ', temp_str_len);	/* Put spaces in the buffer and discard character*/
							op+=temp_str_len; /* Update counters and pointer */
							ip+=temp_str_len;
							c-= (temp_str_len-1);
							ncount = 0;
							continue;
						}
						ncount = mblen( (char *)ip, MB_CUR_MAX);
						switch( ncount)
						{
							case -1: /* Character is getting split at the block boundary */
								memcpy( op, ip, c);
								memcpy( old_string_buf, ip, c);
								old_string_len = c;
								op+=c;
								ip+=c;
								c = 1;
								break;
							case 0: /* Process null characters */
								*op++ = *ip++;
								break;
							case 1: /* Process single byte characters */
								if(isascii(*ip))
									*op++ = tolower(*ip++);
								else
									*op++ = *ip++;
								break;
							default: /* Process multi byte characters */
								if( c >= ncount)
								{
									memcpy( op, ip, ncount);
									op+=ncount;
									ip+=ncount;
									c-=(ncount-1);
								}
								else
								{
									ncount = -1;
									memcpy( op, ip, c);
									memcpy( old_string_buf, ip, c);
									old_string_len = c;
									op+=c;
									ip+=c;
									c = 1;
								}
								break;
						}
					     }
					} while (--c);
					break;

				case UCUNBLOCK:
					/* Same as above case except toupper is used instead of tolower */
					do {
					     if ( I18N_SBCS_CODE )
					     {
						*op++ = ltou[*ip++];
					     }
					     else
					     {
						if( ncount == -1) 
						{
							memcpy( &old_string_buf[old_string_len], ip, MB_CUR_MAX);
							ncount = mblen( old_string_buf, MB_CUR_MAX);
							temp_str_len = ncount-old_string_len;
							if( cbs > temp_str_len) 
								memcpy( op, ip, temp_str_len);	
							else
								memset( op, ' ', temp_str_len);	
							op+=temp_str_len;
							ip+=temp_str_len;
							c-= (temp_str_len-1);
							ncount = 0;
							continue;
						}
						ncount = mblen( (char *)ip, MB_CUR_MAX);
						switch( ncount)
						{
							case -1:
								memcpy( op, ip, c);
								memcpy( old_string_buf, ip, c);
								old_string_len = c;
								op+=c;
								ip+=c;
								c = 1;
								break;
							case 0:
								*op++ = *ip++;
								break;
							case 1:
								if(isascii(*ip))
									*op++ = toupper(*ip++);
								else
									*op++ = *ip++;
								break;
							default:
								if( c >= ncount)
								{
									memcpy( op, ip, ncount);
									op+=ncount;
									ip+=ncount;
									c-=(ncount-1);
								}
								else
								{
									ncount = -1;
									memcpy( op, ip, c);
									memcpy( old_string_buf, ip, c);
									old_string_len = c;
									op+=c;
									ip+=c;
									c = 1;
								}
								break;
						}
					     }
					} while (--c);
					break;

				case ASCII:
					do {
						*op++ = etoa[*ip++];
					} while (--c);
					break;

				case LCASCII:
					do {
						*op++ = utol[etoa[*ip++]];
					} while (--c);
					break;

				case UCASCII:
					do {
						*op++ = ltou[etoa[*ip++]];
					} while (--c);
					break;
				}

				/* Trim trailing blanks if the line is full */

				if (cbc == cbs)
				{
					c = cbs;	/* `do - while' is usually */
					do {		/* faster than `for' */
						if ((*--op) != ' ')
						{
							op++;
							break;
						}
					} while (--c);

					if( !(I18N_SBCS_CODE) ) 
					if( ncount == -1) /* Character is getting split at the block boundary */
					{
                                                if( cbs > old_string_len) /* The character MAY fit in the next block */
                                                { /* Keep the whole character for the next block by decrementing counters */
							ip -= old_string_len;
							ibc+=old_string_len;
							op -= old_string_len;
							obc-=old_string_len;
							ncount = 0;	
							old_string_len = 0;
                                                }
                                                else /* The buffer size is too small - discard the character */
						{
							op-=old_string_len;
							obc+=old_string_len; /* Update counters */
						}
					}

					*op++ = '\n';
					obc -= cbs - c - 1;
					cbc = 0;

					/* Flush the output buffer if full */

					while (obc >= obs)
					{
						op = flsh();
					}
				}
			}
			break;

		/* Convert to blocked records */

		case BLOCK:
		case LCBLOCK:
		case UCBLOCK:
		case EBCDIC:
		case LCEBCDIC:
		case UCEBCDIC:
		case IBM:
		case LCIBM:
		case UCIBM:
			while ((c = ibc) != 0)
			{
				int nlflag = 0;

				/* We may have to skip to the end of a long line */

				if (skipf)
				{
					do {
						if ((ic = *ip++) == '\n')
						{
							skipf = 0;
							c--;
							break;
						}
					} while (--c);
					if ((ibc = c) == 0)
					{
						continue;	/* read another block */
					}
				}

				/* If anything left, copy until newline */

				if (c > (cbs - cbc + 1))
				{
					c = cbs - cbc + 1;
				}
				ibc -= c;
				cbc += c;
				obc += c;

				switch (conv)
				{
				case BLOCK:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = ic;
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case LCBLOCK:
					/* Convert lower case to upper case, do not convert multibyte characters */
					do {
					     if ( I18N_SBCS_CODE )
					     {
						if ((ic = *ip++) != '\n')
						{
							*op++ = utol[ic];
						}
						else
						{
							nlflag = 1;
							break;
						}
					     }
					     else
					     {
						if( ncount == -1) /* Process last iteration's pending bytes of a character */
						{
							memcpy( &old_string_buf[old_string_len], ip, MB_CUR_MAX); /* append new bytes to saved bytes */
							ncount = mblen( old_string_buf, MB_CUR_MAX); /* find mblen */
							temp_str_len = ncount-old_string_len;  /* find the no of pending bytes */
							memcpy( op, ip, temp_str_len);	/* Copy the pending bytes */
							op+=temp_str_len; /* Update counters and pointer */
							ip+=temp_str_len;
							c-= (temp_str_len-1);
							ncount = 0;
							continue;
						}
						ncount = mblen( (char *)ip, MB_CUR_MAX);
						switch( ncount)
						{
							case -1: /* Character is getting split at the block boundary */
								memcpy( op, ip, c);
								memcpy( old_string_buf, ip, c);
								old_string_len = c;
								op+=c;
								ip+=c;
								c = 1;
								break;
							case 0: /* Process null characters, keep checking for newline */
								if ((ic = *ip++) != '\n')
								{
									*op++ = ic;
								}
								else
								{
									nlflag = 1;
								}
								break;
							case 1: /* Process single byte characters, keep checking for newline */
								if ((ic = *ip++) != '\n')
								{
									if(isascii(ic))
										*op++ = tolower(ic);
									else
										*op++ = ic;
								}
								else
								{
									nlflag = 1;
								}
								break;
							default: /* Process multi byte characters */
								if( c >= ncount)
								{
									memcpy( op, ip, ncount);
									op+=ncount;
									ip+=ncount;
									c-=(ncount-1);
								}
								else
								{
									ncount = -1;
									memcpy( op, ip, c);
									memcpy( old_string_buf, ip, c);
									old_string_len = c;
									op+=c;
									ip+=c;
									c = 1;
								}
								break;
						}
						if( nlflag == 1)
							break;
					     }
					} while (--c);
					break;

				case UCBLOCK:
					/* Same as above case except toupper is used instead of tolower */
					do {
					     if ( I18N_SBCS_CODE )
					     {
						if ((ic = *ip++) != '\n')
						{
							*op++ = ltou[ic];
						}
						else
						{
							nlflag = 1;
							break;
						}
					     }
					     else
					     {
						if( ncount == -1) 
						{
							memcpy( &old_string_buf[old_string_len], 
										    ip, MB_CUR_MAX);
							ncount = mblen( old_string_buf, MB_CUR_MAX);
							temp_str_len = ncount-old_string_len;
							memcpy( op, ip, temp_str_len);	
							op+=temp_str_len;
							ip+=temp_str_len;
							c-= (temp_str_len-1);
							ncount = 0;
							continue;
						}
						ncount = mblen( (char *)ip, MB_CUR_MAX);
						switch( ncount)
						{
							case -1:
								memcpy( op, ip, c);
								memcpy( old_string_buf, ip, c);
								old_string_len = c;
								op+=c;
								ip+=c;
								c = 1;
								break;
							case 0:
								if ((ic = *ip++) != '\n')
								{
									*op++ = ic;
								}
								else
								{
									nlflag = 1;
								}
								break;
							case 1:
								if ((ic = *ip++) != '\n')
								{
									if(isascii(ic))
										*op++ = toupper(ic);
									else
										*op++ = ic;
								}
								else
								{
									nlflag = 1;
								}
								break;
							default:
								if( c >= ncount)
								{
									memcpy( op, ip, ncount);
									op+=ncount;
									ip+=ncount;
									c-=(ncount-1);
								}
								else
								{
									ncount = -1;
									memcpy( op, ip, c);
									memcpy( old_string_buf, ip, c);
									old_string_len = c;
									op+=c;
									ip+=c;
									c = 1;
								}
								break;
						}
						if( nlflag == 1)
							break;
					     }
					} while (--c);
					break;

				case EBCDIC:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoe[ic];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case LCEBCDIC:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoe[utol[ic]];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case UCEBCDIC:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoe[ltou[ic]];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case IBM:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoibm[ic];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case LCIBM:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoibm[utol[ic]];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case UCIBM:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoibm[ltou[ic]];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;
				}

				/* If newline found, update all the counters and */
 				/* pointers, pad with trailing blanks if necessary */

				if (nlflag)
				{
					ibc += c - 1;
					obc += cbs - cbc;
					c += cbs - cbc;
					cbc = 0;
					if (c > 0)
					{
						/* Use the right kind of blank */

						switch (conv)
						{
						case BLOCK:
						case LCBLOCK:
						case UCBLOCK:
							ic = ' ';
							break;

						case EBCDIC:
						case LCEBCDIC:
						case UCEBCDIC:
							ic = atoe[' '];
							break;

						case IBM:
						case LCIBM:
						case UCIBM:
							ic = atoibm[' '];
							break;
						}

						/* Pad with trailing blanks */

						do {
							*op++ = ic;
						} while (--c);
					}
				}

				/* If not end of line, this line may be too long */

				else if (cbc > cbs)
				{
					skipf = 1;	/* note skip in progress */
					obc--;
					op--;

					if( !(I18N_SBCS_CODE) ) {
					if( ncount > 1 ) /* Not enough space in the buffer for the character */
					{
						memset( (op-(ncount-1)), ' ', (ncount-1)); /* Discard character and put space */
						ncount = 0;	
					}
					if( ncount == -1) /* Character is getting split at the block boundary */
					{
						memset( (op-(old_string_len-1)), ' ', (old_string_len-1));/*Discard bytes already copied and put space*/
						ncount = 0;	
						old_string_len = 0;
					}
					}

					cbc = 0;
					ntrunc++;	/* count another long line */
				}

				/* Flush the output buffer if full */

				while (obc >= obs)
				{
					op = flsh();
				}
			}
			break;
		}
	}
}

/* match ************************************************************** */
/*									*/
/* Compare two text strings for equality				*/
/*									*/
/* Arg:		s - pointer to string to match with a command arg	*/
/* Global arg:	string - pointer to command arg				*/
/*									*/
/* Return:	1 if match, 0 if no match				*/
/*		If match, also reset `string' to point to the text	*/
/*		that follows the matching text.				*/
/*									*/
/* ********************************************************************	*/

int match(s)
char *s;
{
	register char *cs;

	cs = string;
	while (*cs++ == *s)
	{
		if (*s++ == '\0')
		{
			goto true;
		}
	}
	if (*s != '\0')
	{
		return(0);
	}

true:
	cs--;
	string = cs;
	return(1);
}

#ifdef __sgi
/* bignumber ************************************************************* */
/*									*/
/* Convert a numeric arg to binary					*/
/*									*/
/* Arg:		big - maximum valid input number			*/
/* Global arg:	string - pointer to command arg				*/
/*									*/
/* Valid forms:	123 | 123k | 123w | 123b | 123*123 | 123x123		*/
/*		plus combinations such as 2b*3kw*4w			*/
/*									*/
/* Return:	converted number					*/
/*									*/
/* ********************************************************************	*/

unsigned long long bignumber(big)
long long big;
{
	register char *cs;
	long long n;
	long long cut = BIGLL / 10;	/* limit to avoid overflow */

	cs = string;
	n = 0;
	while ((*cs >= '0') && (*cs <= '9') && (n <= cut))
	{
		n = n*10 + *cs++ - '0';
	}
	for (;;)
	{
		switch (*cs++)
		{

		case 'k':
			n *= 1024;
			continue;

		case 'w':
			n *= 2;
			continue;

		case 'b':
			n *= DD_BSIZE;
			continue;

		case '*':
		case 'x':
			string = cs;
			n *= bignumber(BIGLL);

		/* Fall into exit test, recursion has read rest of string */
		/* End of string, check for a valid number */

		case '\0':
			if ((n > cut) || (n < 0))
			{
				pfmt(stderr, MM_ERROR,
					PFMTTXT(_MSG_ARG_OUTOFRANGE), 
					n);
				exit(2);
			}
			return(n);

		default:
			pfmt(stderr, MM_ERROR,
				PFMTTXT(_MSG_BAD_NUMERIC_ARGUMENT), string); 
			exit(2);
		}
	} /* never gets here */
}
#endif /* __sgi */
/* number ************************************************************* */
/*									*/
/* Convert a numeric arg to binary					*/
/*									*/
/* Arg:		big - maximum valid input number			*/
/* Global arg:	string - pointer to command arg				*/
/*									*/
/* Valid forms:	123 | 123k | 123w | 123b | 123*123 | 123x123		*/
/*		plus combinations such as 2b*3kw*4w			*/
/*									*/
/* Return:	converted number					*/
/*									*/
/* ********************************************************************	*/

unsigned int number(big)
long big;
{
	register char *cs;
	long n;
	long cut = BIG / 10;	/* limit to avoid overflow */

	cs = string;
	n = 0;
	while ((*cs >= '0') && (*cs <= '9') && (n <= cut))
	{
		n = n*10 + *cs++ - '0';
	}
	for (;;)
	{
		switch (*cs++)
		{

		case 'k':
			n *= 1024;
			continue;

		case 'w':
			n *= 2;
			continue;

		case 'b':
#ifdef __sgi
			n *= DD_BSIZE;
#else
			n *= BSIZE;
#endif /* __sgi */
			continue;

		case '*':
		case 'x':
			string = cs;
			n *= number(BIG);

		/* Fall into exit test, recursion has read rest of string */

		/* End of string, check for a valid number */

		case '\0':
			if ((n > cut) || (n < 0))
			{
				pfmt(stderr, MM_ERROR,
					PFMTTXT(_MSG_ARG_OUTOFRANGE), 
					n);
				exit(2);
			}
			return(n);

		default:
			pfmt(stderr, MM_ERROR,
				PFMTTXT(_MSG_BAD_NUMERIC_ARGUMENT), string); 
			exit(2);
		}
	} /* never gets here */
}

/* flsh *************************************************************** */
/*									*/
/* Flush the output buffer, move any excess bytes down to the beginning	*/
/*									*/
/* Arg:		none							*/
/* Global args:	obuf, obc, obs, nofr, nopr				*/
/*									*/
/* Return:	Pointer to the first free byte in the output buffer.	*/
/*		Also reset `obc' to account for moved bytes.		*/
/*									*/
/* ********************************************************************	*/

unsigned char *flsh()
{
	register unsigned char *op, *cp;
	register unsigned int bc;
	register unsigned int oc;

	if (obc)			/* don't flush if the buffer is empty */
	{
		if (obc >= obs)
		{
			oc = obs;
			nofr++;		/* count a full output buffer */
		}
		else
		{
			oc = obc;
			nopr++;		/* count a partial output buffer */
		}
		bc = write(obf, (char *)obuf, oc);
		if (bc != oc)
		{
			pfmt(stderr, MM_ERROR, PFMTTXT(_MSG_WRITE_ERROR), 
				strerror(errno));
			if (obc >= obs)
				nofr--;
			else
				nopr--;
			term(2);
		}
		obc -= oc;
		op = obuf;

		/* If any data in the conversion buffer, move it into the output buffer */

		if (obc)
		{
			cp = obuf + obs;
			bc = obc;
			do {
				*op++ = *cp++;
			} while (--bc);
		}
		return(op);
	}
	return(obuf);
}

/* term *************************************************************** */
/*									*/
/* Write record statistics, then exit					*/
/*									*/
/* Arg:		c - exit status code					*/
/*									*/
/* Return:	no return, calls exit					*/
/*									*/
/* ********************************************************************	*/

void
term(c)
int c;
{
	stats();
	exit(c);
}

/* stats ************************************************************** */
/*									*/
/* Write record statistics onto standard error				*/
/*									*/
/* Args:	none							*/
/* Global args:	nifr, nipr, nofr, nopr, ntrunc				*/
/*									*/
/* Return:	void							*/
/*									*/
/* ********************************************************************	*/

void stats()
{
	pfmt(stderr, MM_NOSTD, PFMTTXT(_MSG_DD_RECORDS_IN), nifr, nipr); 
	pfmt(stderr, MM_NOSTD, PFMTTXT(_MSG_DD_RECORDS_OUT), nofr, nopr); 
	if (ntrunc == 1)
		pfmt(stderr, MM_NOSTD, PFMTTXT(_MSG_DD_TRUNCATED_RECORD)); 
	else if (ntrunc > 1)
		pfmt(stderr, MM_NOSTD, PFMTTXT(_MSG_DD_FORMAT_U_TRUNCATED), ntrunc);
}
