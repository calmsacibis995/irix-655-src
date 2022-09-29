/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/infocmp.c	1.14"
/*
    NAME
	infocmp - compare terminfo descriptions, or dump a terminfo
		  description

    AUTHOR
	Tony Hansen, February 23, 1984.
*/

#include "curses_inc.h"
#include "print.h"
#include <stdlib.h>
#include <locale.h>
#include <nl_types.h>
#include <msgs/uxeoe.h>
nl_catd catd;

/* data structures for this program */

struct boolstruct
    {
    char *infoname;			/* the terminfo capability name */
    char *capname;			/* the termcap capability name */
    char *fullname;			/* the long C variable name */
    char *secondname;			/* the use= terminal w/ this value */
    char val;				/* the value */
    char secondval;			/* the value in the use= terminal */
    char changed;			/* a use= terminal changed the value */
    char seenagain;			/* a use= terminal had this entry */
    };

struct numstruct
    {
    char *infoname;			/* ditto from above */
    char *capname;
    char *fullname;
    char *secondname;
    short val;
    short secondval;
    char changed;
    char seenagain;
    };

struct strstruct
    {
    char *infoname;			/* ditto from above */
    char *capname;
    char *fullname;
    char *secondname;
    char *val;
    char *secondval;
    char changed;
    char seenagain;
    };

static int boolcompare(struct boolstruct *, struct boolstruct *);
static int numcompare(struct numstruct *, struct numstruct *);
static int strcompare(struct strstruct *, struct strstruct *);

/* globals for this file */
char *progname;			/* argv[0], the name of the program */
static struct boolstruct *ibool;/* array of char information */
static struct numstruct *num;	/* array of number information */
static struct strstruct *str;	/* array of string information */
static char *used;		/* usage statistics */
static int numbools;		/* how many booleans there are */
static int numnums;		/* how many numbers there are */
static int numstrs;		/* how many strings there are */
#define TTYLEN 255
static char *firstterm;		/* the name of the first terminal */
static char *savettytype;	/* the synonyms of the first terminal */
static char _savettytype[TTYLEN];/* the place to save those names */
static int devnull;		/* open("/dev/null") for setupterm */
#define trace stderr		/* send trace messages to stderr */

/* options */
static int verbose = 0;		/* debugging printing level */
static int diff = 0;		/* produce diff listing, the default */
static int common = 0;		/* produce common listing */
static int neither = 0;		/* list caps in neither entry */
static int use = 0;		/* produce use= comparison listing */
static enum printtypes printing	/* doing any of above printing at all */
	= pr_none;
enum { none, by_database, by_terminfo, by_longnames, by_cap }
    sortorder = none;		/* sort the fields for printing */
static char *term1info, *term2info;	/* $TERMINFO settings */
static int Aflag = 0, Bflag = 0;	/* $TERMINFO was set with -A/-B */

#define EQUAL(s1,s2)	( ((s1 == NULL) && (s2 == NULL)) || \
			  ((s1 != NULL) && (s2 != NULL) && \
			  (strcmp(s1,s2) == 0) ) )

void
badmalloc(void)
{
    (void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_NO_MEMORY), progname);
    catclose(catd);
    exit (-1);
}

/*
    Allocate and initialize the global data structures and variables.
*/
void
allocvariables(int argc, int firstoptind)
{
    register int i, nullseen;

    /* find out how many names we are dealing with */
    for (numbools = 0; boolnames[numbools]; numbools++)
	;
    for (numnums = 0; numnames[numnums]; numnums++)
	;
    for (numstrs = 0; strnames[numstrs]; numstrs++)
	;

    if (verbose)
	{
	(void) fprintf (trace, CATGETS(catd,_MSG_INFOCMP_NUM_BOOLCAPS), numbools);
	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_NUM_NUMCAPS), numnums);
	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_NUM_STRCAPS), numstrs);
	
	}

    /* Allocate storage for the names and their values */
    ibool = (struct boolstruct  *) malloc ((unsigned) numbools * sizeof (struct boolstruct));
    num = (struct numstruct *) malloc ((unsigned) numnums * sizeof (struct numstruct));
    str = (struct strstruct *) malloc ((unsigned) numstrs * sizeof (struct strstruct));

    /* Allocate array to keep track of which names have been used. */
    if (use)
	used = (char *) malloc ((unsigned) (argc - firstoptind) * sizeof (char));

    if ((ibool == NULL) || (num == NULL) || (str == NULL) ||
	    (use && (used == NULL)))
	badmalloc();

    /* Fill in the names and initialize the structures. */
    nullseen = FALSE;
    for (i = 0; i < numbools; i++)
	{
	ibool[i].infoname = boolnames[i];
	ibool[i].capname = boolcodes[i];
	/* This is necessary until fnames.c is */
	/* incorporated into standard curses. */
	if (nullseen || (boolfnames[i] == NULL))
	    {
	    ibool[i].fullname = CATGETS(catd, _MSG_INFOCMP_UNKNOWN_BOOLEAN);
	    nullseen = TRUE;
	    }
	else
	    ibool[i].fullname = boolfnames[i];
	ibool[i].changed = FALSE;
	ibool[i].seenagain = FALSE;
	}
    nullseen = 0;
    for (i = 0; i < numnums; i++)
	{
	num[i].infoname = numnames[i];
	num[i].capname = numcodes[i];
	if (nullseen || (numfnames[i] == NULL))
	    {
	    ibool[i].fullname = CATGETS(catd, _MSG_INFOCMP_UNKNOWN_NUMBER);
	    nullseen = TRUE;
	    }
	else
	    num[i].fullname = numfnames[i];
	num[i].changed = FALSE;
	num[i].seenagain = FALSE;
	}
    nullseen = 0;
    for (i = 0; i < numstrs; i++)
	{
	str[i].infoname = strnames[i];
	str[i].capname = strcodes[i];
	if (nullseen || (strfnames[i] == NULL))
	    {
	    str[i].fullname = CATGETS(catd, _MSG_INFOCMP_UNKNOWN_STRING);
	    nullseen = TRUE;
	    }
	else
	    str[i].fullname = strfnames[i];
	str[i].changed = FALSE;
	str[i].seenagain = FALSE;
	}
}

/*
    Routines to be passed to qsort(3) for comparison of the structures.
*/
static int
boolcompare (struct boolstruct *a, struct boolstruct *b)
{
    switch ( (int) sortorder)
	{
	case (int) by_terminfo:	 return strcmp (a->infoname, b->infoname);
	case (int) by_cap:	 return strcmp (a->capname, b->capname);
	case (int) by_longnames: return strcmp (a->fullname, b->fullname);
	default:		 return 0;
	}
}

static int
numcompare (struct numstruct *a, struct numstruct *b)
{
    switch ( (int) sortorder)
	{
	case (int) by_terminfo:	 return strcmp (a->infoname, b->infoname);
	case (int) by_cap:	 return strcmp (a->capname, b->capname);
	case (int) by_longnames: return strcmp (a->fullname, b->fullname);
	default:		 return 0;
	}
}

static int
strcompare (struct strstruct *a, struct strstruct *b)
{
    switch ( (int) sortorder)
	{
	case (int) by_terminfo:	 return strcmp (a->infoname, b->infoname);
	case (int) by_cap:	 return strcmp (a->capname, b->capname);
	case (int) by_longnames: return strcmp (a->fullname, b->fullname);
	default:		 return 0;
	}
}

/*
    Sort the entries by their terminfo name.
*/
void
sortnames(void)
{
    if (sortorder != by_database)
	{
	qsort ((char *) ibool, (unsigned) numbools, 
		sizeof (struct boolstruct), (int(*)())boolcompare);
	qsort ((char *) num, (unsigned) numnums, 
		sizeof (struct numstruct), (int(*)())numcompare);
	qsort ((char *) str, (unsigned) numstrs, 
		sizeof (struct strstruct), (int(*)())strcompare);
	}
}

/*
    Print out a string, or "NULL" if it's not defined.
*/
void
PR (FILE *stream, register char *string)
{
    if (string == NULL)
    	(void) fprintf (stream, CATGETS(catd, _MSG_INFOCMP_NULLSTR));
    else
	tpr (stream, string);
}

/*
    Output the 'ko' termcap string. This is a list of all of the input
    keys that input the same thing as the corresponding output strings.    
*/
int kncounter;
char kobuffer[512];

char *
addko (char *output, char *input, char *koptr)
{
    char *inptr, *outptr, padbuffer[512];
    inptr = tgetstr (input, (char **)0);
    if (inptr == NULL)
        return koptr;
    outptr = tgetstr (output, (char **)0);
    if (outptr == NULL)
        return koptr;
    outptr = rmpadding (outptr, padbuffer, (int *) 0);
    if (strcmp (inptr, outptr) == 0)
        {
        *koptr++ = *output++;
	*koptr++ = *output++;
	*koptr++ = ',';
	kncounter++;
	}
    return koptr;
}

void
setupknko(void)
{
    char *koptr;
    
    kncounter = 0;
    koptr = kobuffer;

    koptr = addko("bs", "kb", koptr);	/* key_backspace */
    koptr = addko("bt", "kB", koptr);	/* key_btab */
    koptr = addko("cl", "kC", koptr);	/* key_clear */
    koptr = addko("le", "kl", koptr);	/* key_left */
    koptr = addko("do", "kd", koptr);	/* key_down */
    koptr = addko("nd", "kr", koptr);	/* key_right */
    koptr = addko("up", "ku", koptr);	/* key_up */
    koptr = addko("dc", "kD", koptr);	/* key_dc */
    koptr = addko("dl", "kL", koptr);	/* key_dl */
    koptr = addko("cd", "kS", koptr);	/* key_eos */
    koptr = addko("ce", "kE", koptr);	/* key_eol */
    koptr = addko("ho", "kh", koptr);	/* key_home */
    koptr = addko("st", "kT", koptr);	/* key_stab */
    koptr = addko("ic", "kI", koptr);	/* key_ic */
    koptr = addko("im", "kI", koptr);	/* key_ic */
    koptr = addko("al", "kA", koptr);	/* key_il */
    koptr = addko("sf", "kF", koptr);	/* key_sf */
    koptr = addko("ll", "kH", koptr);	/* key_ll */
    koptr = addko("sr", "kR", koptr);	/* key_sr */
    koptr = addko("ei", "kM", koptr);	/* key_eic */
    koptr = addko("ct", "ka", koptr);	/* key_catab */

    /* get rid of comma */
    if (koptr != kobuffer)
        *(--koptr) = '\0';
}

void
pr_kn(void)
{
    if (kncounter > 0)
        pr_number ((char *)0, "kn", (char *)0, kncounter);
}

void
pr_ko(void)
{
    if (kncounter > 0)
	pr_string ((char *)0, "ko", (char *)0, kobuffer);
}

void
pr_bcaps(void)
{
    char *retptr;
    char padbuffer[512];

    if (verbose)
    	(void) fprintf (trace, CATGETS(catd,_MSG_INFOCMP_LOOKAT), "'bs'");
    retptr = cconvert (rmpadding (cursor_left, padbuffer, (int *) 0));
    if (strcmp ("\\b", retptr) == 0)
	pr_boolean ((char *)0, "bs", (char *)0, 1);

    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_LOOKAT), "'pt'");
    retptr = cconvert (rmpadding (tab, padbuffer, (int *) 0));
    if (strcmp ("\\t", retptr) == 0)
	pr_boolean ((char *)0, "pt", (char *)0, 1);

    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_LOOKAT), "'nc'");
    retptr = cconvert (rmpadding (carriage_return, padbuffer, (int *) 0));
    if (strcmp ("\\r", retptr) != 0)
	pr_boolean ((char *)0, "nc", (char *)0, 1);

    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_LOOKAT), "'ns'");
    if (scroll_forward == NULL)
	pr_boolean ((char *)0, "ns", (char *)0, 1);

    /* Ignore "xr": Return acts like ce \r \n (Delta Data) */
}

void
pr_ncaps(void)
{
    char padbuffer[512];
    int padding;

    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_LOOKAT), "'ug'");
    /* Duplicate sg for ug: Number of blank chars left by us or ue */
    if (magic_cookie_glitch > -1)
	pr_number ((char *)0, "ug", (char *)0, magic_cookie_glitch);

    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_LOOKAT), "'dB'");
    /* Number of millisec of bs delay needed */
    (void) rmpadding (cursor_left, padbuffer, &padding);
    if (padding > 0)
	pr_number ((char *)0, "dB", (char *)0, padding);

    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_LOOKAT), "'dC'");
    /* Number of millisec of cr delay needed */
    (void) rmpadding (carriage_return, padbuffer, &padding);
    if (padding > 0)
	pr_number ((char *)0, "dC", (char *)0, padding);

    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_LOOKAT), "'dF'");
    /* Number of millisec of ff delay needed */
    (void) rmpadding (form_feed, padbuffer, &padding);
    if (padding > 0)
	pr_number ((char *)0, "dF", (char *)0, padding);

    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_LOOKAT), "'dN'");
    /* Number of millisec of nl delay needed */
    (void) rmpadding (cursor_down, padbuffer, &padding);
    if (padding > 0)
	pr_number ((char *)0, "dN", (char *)0, padding);

    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_LOOKAT), "'dT'");
    /* Number of millisec of tab delay needed */
    (void) rmpadding (tab, padbuffer, &padding);
    if (padding > 0)
	pr_number ((char *)0, "dT", (char *)0, padding);

    /* Handle "kn": Number of "other" keys */
    setupknko();
    pr_kn();
}

void
pr_scaps(void)
{
    char *retptr;
    char padbuffer[512];

    /* Backspace if not "^H" */
    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_LOOKAT), "'bc'");
    retptr = cconvert (rmpadding (cursor_left, padbuffer, (int *) 0));
    if (strcmp ("\\b", retptr) != 0)
	pr_string ((char *)0, "bc", (char *)0, cursor_left);

    /* Newline character (default "\n") */
    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_LOOKAT), "'nl'");
    retptr = cconvert (rmpadding (cursor_down, padbuffer, (int *) 0));
    if (strcmp ("\\n", retptr) != 0)
	pr_string ((char *)0, "nl", (char *)0, cursor_down);

    /* Handle "ko" here: Termcap entries for other non-function keys */
    pr_ko();

    /* Ignore "ma": Arrow key map, used by vi version 2 only */
}

/*
    Set up the first terminal and save the values from it.
*/
void
initfirstterm (register char *term)
{
    register int i;

    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_SETUPTERM), term);

    (void) setupterm (term, devnull, (int *) 0);

    /* Save the name for later use. */
    if (use)
	{
	register size_t length;
	savettytype = _savettytype;
	if ((length = strlen(ttytype)) >= TTYLEN)
	    {
	    savettytype = malloc (length);
	    if (savettytype == NULL)
		{
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_NO_MEMORY1), progname);
		(void) strncpy (_savettytype, ttytype, TTYLEN-1);
		_savettytype[TTYLEN - 1] = '\0';
		savettytype = _savettytype;
		}
	    }
	else
	    (void) strcpy (_savettytype, ttytype);
	}

    if (printing != pr_none)
	{
	pr_heading (term, ttytype);
	pr_bheading ();
	}

    /* Save the values for the first terminal. */
    for (i = 0; i < numbools; i++)
	{
	if ((ibool[i].val = (char) tgetflag (ibool[i].capname)) &&
	    printing != pr_none)
	    pr_boolean (ibool[i].infoname, ibool[i].capname, ibool[i].fullname, 1);
	if (verbose)
	    (void) fprintf (trace, "%s=%d.\n", ibool[i].infoname, ibool[i].val);
	}

    if (printing != pr_none)
	{
	if (printing == pr_cap)
	    pr_bcaps();
	pr_bfooting();
	pr_nheading();
	}

    for (i = 0; i < numnums; i++)
	{
	if (((num[i].val = (short) tgetnum (num[i].capname)) > -1) && 
	    printing != pr_none)
	    pr_number (num[i].infoname, num[i].capname, num[i].fullname, num[i].val);
	if (verbose)
	    (void) fprintf (trace, "%s=%d.\n", num[i].infoname, num[i].val);
	}

    if (printing != pr_none)
	{
	if (printing == pr_cap)
	    pr_ncaps();
	pr_nfooting();
	pr_sheading();
	}

    for (i = 0; i < numstrs; i++)
	{
	str[i].val = tgetstr (str[i].capname, (char **)0);
	if ((str[i].val != NULL) && printing != pr_none)
	    pr_string (str[i].infoname, str[i].capname, str[i].fullname, str[i].val);
	if (verbose)
	    {
	    (void) fprintf (trace, "%s='", str[i].infoname);
	    PR (trace, str[i].val);
	    (void) fprintf (trace, "'.\n");
	    }
	}

    if (printing == pr_cap)
	pr_scaps();

    if (printing != pr_none)
	pr_sfooting();
}

/*
    Set up the n'th terminal.
*/
void
check_nth_terminal (char *nterm, int n)
{
    register char boolval;
    register short numval;
    register char *strval;
    register int i;

    if (use)
	used[n] = FALSE;

    if (verbose)
    	(void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_ADDTERM), nterm);

    (void) setupterm (nterm, devnull, (int *) 0);

    if (printing != pr_none)
	{
	pr_heading (nterm, ttytype);
	pr_bheading ();
	}

    if (diff || common || neither)
	{
	if (Aflag && Bflag)
	    (void) printf (CATGETS(catd, _MSG_INFOCMP_COMP1),
	        firstterm, term1info, nterm, term2info);
	else if (Aflag)
	    (void) printf (CATGETS(catd, _MSG_INFOCMP_COMP2),
	        firstterm, term1info, nterm);
	else if (Bflag)
	    (void) printf (CATGETS(catd, _MSG_INFOCMP_COMP3),
	    firstterm, nterm, term2info);
	else
	    (void) printf (CATGETS(catd, _MSG_INFOCMP_COMP4), firstterm, nterm);
	(void) printf (CATGETS(catd, _MSG_INFOCMP_COMPBOOLS));
	}

    /* save away the values for the nth terminal */
    for (i = 0; i < numbools; i++)
	{
	boolval = (char) tgetflag (ibool[i].capname);
	if (use)
	    {
	    if (ibool[i].seenagain)
		{
		/* We do not have to worry about this impossible case
		** since booleans can have only two values: true and false.
		** if (boolval && (boolval != ibool[i].secondval))
		**  {
		**  (void) fprintf (trace, "use= order dependency found:\n");
		**  (void) fprintf (trace, "    %s: %s has %d, %s has %d.\n",
		**	ibool[i].capname, ibool[i].secondname,
		**	ibool[i].secondval, nterm, boolval);
		**  }
		*/
		}
	    else
		{
		if (boolval == TRUE)
		    {
		    ibool[i].seenagain = TRUE;
		    ibool[i].secondval = boolval;
		    ibool[i].secondname = nterm;
		    if (ibool[i].val != boolval)
			ibool[i].changed = TRUE;
		    else
			used[n] = TRUE;
		    }
		}
	    }
	if (boolval)
	    {
	    if (printing != pr_none)
		pr_boolean (ibool[i].infoname, ibool[i].capname, ibool[i].fullname, 1);
	    if (common && (ibool[i].val == boolval))
		(void) printf ("\t%s= T.\n", ibool[i].infoname);
	    }
	else if (neither && !ibool[i].val)
	    (void) printf ("\t!%s.\n", ibool[i].infoname);
	if (diff && (ibool[i].val != boolval))
	    (void) printf ("\t%s: %c:%c.\n", ibool[i].infoname,
		    ibool[i].val?'T':'F', boolval?'T':'F');
	if (verbose)
	    (void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_CHANGEDSEEN),
		    ibool[i].infoname, ibool[i].val, boolval,
		    ibool[i].changed, ibool[i].seenagain);
	}

    if (printing != pr_none)
	{
	if (printing == pr_cap)
	    pr_bcaps();
	pr_bfooting();
	pr_nheading();
	}

    if (diff || common || neither)
        (void) printf (CATGETS(catd, _MSG_INFOCMP_COMPNUMS));

    for (i = 0; i < numnums; i++)
	{
	numval = (short) tgetnum (num[i].capname);
	if (use)
	    {
	    if (num[i].seenagain)
		{
		if ((numval > -1) && (numval != num[i].secondval))
		    {
		    (void) fprintf (stderr,
			CATGETS(catd, _MSG_INFOCMP_USEORDER), progname);
		    (void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_HASHAS),
			    num[i].capname, num[i].secondname,
			    num[i].secondval, nterm, numval);
		    }
		}
	    else
		{
		if (numval > -1)
		    {
		    num[i].seenagain = TRUE;
		    num[i].secondval = numval;
		    num[i].secondname = nterm;
		    if ((numval > -1) && (num[i].val != numval))
			num[i].changed = TRUE;
		    else
			used[n] = TRUE;
		    }
		}
	    }
	if (numval > -1)
	    {
	    if (printing != pr_none)
		pr_number (num[i].infoname, num[i].capname, num[i].fullname, numval);
	    if (common && (num[i].val == numval))
		(void) printf ("\t%s= %d.\n", num[i].infoname, numval);
	    }
	else if (neither && (num[i].val == -1))
	    (void) printf ("\t!%s.\n", num[i].infoname);
	if (diff && (num[i].val != numval))
	    (void) printf ("\t%s: %d:%d.\n", num[i].infoname, num[i].val,
		    numval);
	if (verbose)
	    (void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_CHANGEDSEEN),
		    num[i].infoname, num[i].val, numval,
		    num[i].changed, num[i].seenagain);
	}

    if (printing != pr_none)
	{
	if (printing == pr_cap)
	    pr_ncaps();
	pr_nfooting();
	pr_sheading();
	}

    if (diff || common || neither)
        (void) printf (CATGETS(catd, _MSG_INFOCMP_COMPSTRS));

    for (i = 0; i < numstrs; i++)
	{
	strval = tgetstr (str[i].capname, (char **)0);
	if (use)
	    {
	    if (str[i].seenagain && (strval != NULL))
		{
		if (!EQUAL (strval, str[i].secondval))
		    {
		    (void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USEORDER1));
		    (void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_HASSTRING1),
			    str[i].capname, str[i].secondname);
		    PR (stderr, str[i].secondval);
		    (void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_HASSTRING2), nterm);
		    PR (stderr, strval);
		    (void) fprintf (stderr, "'.\n");
		    }
		}
	    else
		{
		if (strval != NULL)
		    {
		    str[i].seenagain = TRUE;
		    str[i].secondval = strval;
		    str[i].secondname = nterm;
		    if (!EQUAL (str[i].val, strval))
			str[i].changed = TRUE;
		    else
			used[n] = TRUE;
		    }
		}
	    }
	if (strval != NULL)
	    {
	    if (printing != pr_none)
		pr_string (str[i].infoname, str[i].capname, str[i].fullname, strval);
	    if (common && EQUAL (str[i].val, strval))
		{
		(void) printf ("\t%s= '", str[i].infoname);
		PR (stdout, strval);
		(void) printf ("'.\n");
		}
	    }
	else if (neither && (str[i].val == NULL))
	    (void) printf ("\t!%s.\n", str[i].infoname);
	if (diff && !EQUAL (str[i].val, strval))
	    {
	    (void) printf ("\t%s: '", str[i].infoname);
	    PR (stdout, str[i].val);
	    (void) printf ("','");
	    PR (stdout, strval);
	    (void) printf ("'.\n");
	    }
	if (verbose)
	    {
	    (void) fprintf (trace, "%s: '", str[i].infoname);
	    PR (trace, str[i].val);
	    (void) fprintf (trace, "':'");
	    PR (trace, strval);
	    (void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_CHANGEDSEEN1),
		    str[i].changed, str[i].seenagain);
	    }
	}

    if (printing == pr_cap)
	pr_scaps();

    if (printing != pr_none)
	pr_sfooting();
}

/*
    A capability gets an at-sign if it no longer exists, but
    one of the relative entries contains a value for it.
    It gets printed if the original value is not seen in ANY
    of the relative entries, or if the FIRST relative entry that has
    the capability gives a DIFFERENT value for the capability.
*/
void
dorelative (register int firstoptind, register int argc, register char **argv)
{
    register int i;

    /* turn off printing of termcap and long names */
    pr_init (pr_terminfo);

    /* print out the entry name */
    pr_heading ((char *)0, savettytype);

    pr_bheading();

    /* Print out all bools that are different. */
    for (i = 0; i < numbools; i++)
	if (!ibool[i].val && ibool[i].changed)
	    pr_boolean (ibool[i].infoname, (char *)0, (char *)0, -1);
	else if (ibool[i].val && (ibool[i].changed || !ibool[i].seenagain))
	    pr_boolean (ibool[i].infoname, (char *)0, (char *)0, 1);

    pr_bfooting();
    pr_nheading();

    /* Print out all nums that are different. */
    for (i = 0; i < numnums; i++)
	if (num[i].val < 0 && num[i].changed)
	    pr_number (num[i].infoname, (char *)0, (char *)0, -1);
	else if (num[i].val >= 0 && (num[i].changed || !num[i].seenagain))
	    pr_number (num[i].infoname, (char *)0, (char *)0, num[i].val);

    pr_nfooting();
    pr_sheading();

    /* Print out all strs that are different. */
    for (i = 0; i < numstrs; i++)
	if (str[i].val == NULL && str[i].changed)
	    pr_string (str[i].infoname, (char *)0, (char *)0, (char *)0);
	else if ((str[i].val != NULL) &&
		 (str[i].changed || !str[i].seenagain))
	    pr_string (str[i].infoname, (char *)0, (char *)0, str[i].val);

    pr_sfooting();

    /* Finish it up. */
    for (i = firstoptind; i < argc; i++)
	if (used[i - firstoptind])
	    (void) printf (CATGETS(catd, _MSG_INFOCMP_USE1), argv[i]);
	else
	    (void) fprintf (stderr,
		CATGETS(catd, _MSG_INFOCMP_USE2),
		progname, argv[i]);
}

void
setenv (register char *termNinfo)
{
    extern char **environ;
    static char *newenviron[2] = { 0, 0 };
    static size_t termsize = BUFSIZ;
    static char _terminfo[BUFSIZ];
    static char *terminfo = &_terminfo[0];
    register size_t termlen;

    if (termNinfo && *termNinfo)
	{
	if (verbose)
	    (void) fprintf (trace, CATGETS(catd, _MSG_INFOCMP_SETTERMINFO), termNinfo);
	termlen = strlen (termNinfo);
	if (termlen + 10 > termsize)
	    {
	    termsize = termlen + 20;
	    terminfo = (char *) malloc (termsize * sizeof (char));
	    }
	if (terminfo == (char *) NULL)
	    badmalloc();
	(void) sprintf (terminfo, CATGETS(catd, _MSG_INFOCMP_TERMINFOSTR), termNinfo);
	newenviron[0] = terminfo;
	}
    else
	newenviron[0] = (char *) 0;
    environ = newenviron;
}

main (int argc, char **argv)
{
    register int i, c, firstoptind;
    char *tempargv[2];
    char *term = getenv ("TERM");

    (void) setlocale(LC_ALL, "");
    catd = catopen("uxeoe", 0);

    term1info = term2info = getenv ("TERMINFO");
    progname = argv[0];

    /* parse options */
    while ((c = getopt (argc, argv, "ducnILCvV1rw:s:A:B:")) != EOF)
	switch (c)
	    {
	    case 'v':	verbose++;				break;
	    case '1':	pr_onecolumn(1);			break;
	    case 'w':	pr_width (atoi(optarg));		break;
	    case 'd':	diff++;					break;
	    case 'c':	common++;				break;
	    case 'n':	neither++;				break;
	    case 'u':	use++;					break;
	    case 'L':	pr_init (printing = pr_longnames);	break;
	    case 'I':	pr_init (printing = pr_terminfo);	break;
	    case 'C':	pr_init (printing = pr_cap);		break;
	    case 'A':	term1info = optarg; Aflag++;		break;
	    case 'B':	term2info = optarg; Bflag++;		break;
	    case 'r':	pr_caprestrict(0);			break;
	    case 's':
		if (strcmp(optarg, "d") == 0)
		    sortorder = by_database;
		else if (strcmp(optarg, "i") == 0)
		    sortorder = by_terminfo;
		else if (strcmp(optarg, "l") == 0)
		    sortorder = by_longnames;
		else if (strcmp(optarg, "c") == 0)
		    sortorder = by_cap;
		else
		    goto usage;
		break;
	    case 'V':
	    	(void) printf (CATGETS(catd, _MSG_INFOCMP_VERSION_INFO), progname, 
	    	"@(#)curses:screen/infocmp.c	  1.13");
	    	catclose(catd);
			exit (0);
	    case '?':
	    usage:
		(void) fprintf (stderr,
			CATGETS(catd, _MSG_INFOCMP_USAGE_STRING), 
			progname);
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_d));
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_u));
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_c));
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_n));
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_I));
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_C));
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_L));
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_1));
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_V));
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_v));
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_s));
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_A));
		(void) fprintf (stderr, CATGETS(catd, _MSG_INFOCMP_USAGE_B));
		catclose(catd);
		exit (-1);
	    }

    argc -= optind;
    argv += optind;
    optind = 0;

    /* Default to $TERM for -n, -I, -C and -L options. */
    /* This is done by faking argv[][], argc and optind. */
    if (neither && (argc == 0 || argc == 1))
	{
	if (argc == 0)
	    tempargv[0] = term;
	else
	    tempargv[0] = argv[optind];
	tempargv[1] = term;
	argc = 2;
	argv = tempargv;
	optind = 0;
	}

    else if ((printing != pr_none) && (argc == 0))
	{
	tempargv[0] = term;
	argc = 1;
	argv = tempargv;
	optind = 0;
	}

    /* Check for enough names. */
    if ((use || diff || common) && (argc <= 1))
	{
	(void) fprintf (stderr,
	    CATGETS(catd, _MSG_INFOCMP_TWOTERMS),
	    progname);
	goto usage;
	}

    /* Set the default of diff -d or print -I */
    if (!use && (printing == pr_none) && !common && !neither)
	{
	if (argc == 0 || argc == 1)
	    {
	    if (argc == 0)
		{
		tempargv[0] = term;
		argc = 1;
		argv = tempargv;
		optind = 0;
		}
	    pr_init (printing = pr_terminfo);
	    }
	else
	    diff++;
	}

    /* Set the default sorting order. */
    if (sortorder == none)
	switch ((int) printing)
	    {
	    case (int) pr_cap:		sortorder = by_cap; break;
	    case (int) pr_longnames:	sortorder = by_longnames; break;
	    case (int) pr_terminfo:
	    case (int) pr_none:		sortorder = by_terminfo; break;
	    }

    firstterm = argv[optind++];
    firstoptind = optind;

    allocvariables (argc, firstoptind);
    sortnames ();

    devnull = open("/dev/null", O_RDWR);
    setenv(term1info);
    initfirstterm (firstterm);
    setenv(term2info);
    for (i = 0; optind < argc; optind++, i++)
	check_nth_terminal (argv[optind], i);

    if (use)
	dorelative (firstoptind, argc, argv);
    catclose(catd);
    return 0;
}
