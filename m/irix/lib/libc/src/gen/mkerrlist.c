/* $Revision: 1.5 $
 *
 * File: mkerrlist.c
 * Usage: % mkerrlist <error_list_file>
 *
 * This command take an input file and generates the following files:
 *	(1) sys error input file for the msg catalog generation tool
 *	(2) sgi error input file for the msg catalog generation tool
 *	(3) .c file for errno msg array sys_errlist[] (back compat)
 *	(4) .c file for errno xlate table and errno msg array
 */

static char rcsid[] = "mkerrlist.c $Revision: 1.5 $";

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <stdlib.h>

#define	SGIBASE		1000	/* for sgi added error numbers */
#define	MAX_SYS_NERR	152	/* back compat: index into sys_errlist */

/*
 * Global definitions:
 */

char *progname;			/* program name for error messages */
char *configfile;		/* file w/ (errno,label,string) entries */

static const char prefix[] = "MMX_SYS_";	/* label prefix */ 

/*
 * Files generated by this tool.
 */
#define	SYSERRMSG	"uxsyserr_msg.src"	/* input to gen catalog */
#define	SGIERRMSG	"uxsgierr_msg.src"	/* input to gen catalog */
#define ERRLIST_C	"errlst.c"		/* .c for back compat */
#define NEW_LIST_C	"new_list.c"		/* .c for new msgs */

/*
 * Message catalogs
 */
#define	SYSERR		"uxsyserr"	/* standard SVR4 libc msg catalog */
#define	SGIERR		"uxsgierr"	/* sgi libc msg catalog */
#define	CATNO_SYSERR	0x0000
#define	CATNO_SGIERR	0x1<<16
#define	CATNO_MASK	CATNO_SYSERR | CATNO_SGIERR

#define	MAX_LINE	256
char line[MAX_LINE];		/* to read each entry in the err list table */

struct errtable {
	int cat;		/* which msg catalog: uxsyserr=0, uxsgierr=1 */
	int index;		/* entry into the message file */
	char *msg;		/* message string */
};

struct errtable *syserrtab;	/* generic system errors */
struct errtable *sgierrtab;	/* sgi specific errors */

int	max_syserr = 0;		/* highest system error number encountered */
int	max_sgierr = 0;		/* highest sgi error number encountered */

FILE	*fconfig = NULL;
FILE	*fsyserr = NULL;
FILE	*fsgierr = NULL;
FILE	*ferrlst = NULL;
FILE	*fnewlst = NULL;


FILE *
efopen(char *file, char *mode)	/* fopen file, die if can't */
{
	FILE *fp;

	if ((fp = fopen(file, mode)) != NULL)
		return fp;
	fprintf(stderr, "%s: can't open file %s mode %s\n",
		progname, file, mode);
	exit(1);
	/*NOTREACHED*/
}	/* efopen() */


char *
nextline(FILE *fd)
{
	char *cp;

	if (fgets(line, sizeof (line), fd) == NULL)
		return ((char *)0);
	cp = index(line, '\n');
        if (cp)
		*cp = '\0';
	return (line);
}	/* nextline() */


/*
 * Skip over white space to the next token. If <single> is true
 * then terminate the token at the next white space after the token.
 */
char *
skip(char **cpp, int single)
{
	register char *cp = *cpp;
	char *start;

	while (*cp == ' ' || *cp == '\t')
		cp++;
	if (*cp == '\0') 
		return ((char *)0);
	start = cp;
	if (single == 0)
		return (start);
	while (*cp && *cp != ' ' && *cp != '\t')
		cp++;
	if (*cp != '\0')
		*cp++ = '\0';
	*cpp = cp;
	return (start);
}	/* skip() */


void
openfiles(void)
{
	fconfig = efopen(configfile, "r");
	fsyserr = efopen(SYSERRMSG, "w");
	fsgierr = efopen(SGIERRMSG, "w");
	ferrlst = efopen(ERRLIST_C, "w");
	fnewlst = efopen(NEW_LIST_C, "w");

	/*
	 * The first 10 lines of each catalog src file are
	 * reserved for comments and ident stuff. So src must
	 * write 10 lines at first, containing only a colon ':'.
	 * These 10 lines will be removed during build by the makefile.
	 */
	fprintf(fsyserr, ":\n:\n:\n:\n:\n:\n:\n:\n:\n:\n");
	fprintf(fsgierr, ":\n:\n:\n:\n:\n:\n:\n:\n:\n:\n");
}	/* openfiles() */


void
getmaxerrs(void)
{
	char *cp;
	int error;
	int linenumber = 0;

	max_syserr = 0;
	max_sgierr = SGIBASE;

	while (cp = nextline(fconfig)) {
		linenumber++;
		if (*cp == '#' || *cp == '\0')	/* ignore comments and \n */
			continue;
		if (sscanf(cp, "%d", &error) == 0) {
			fprintf(stderr, "%s: line %d - bad error number\n",
				progname, linenumber);
			continue;
		}
		if ((max_syserr < error) && (error < SGIBASE))
			max_syserr = error;
		else if (error > max_sgierr)
			max_sgierr = error;
	}
}	/* getmaxerrs() */


/*
 * Read error number configuration and create the internal tables. 
 * Also write the message into its respective catalog input file. 
 */
void
maketables(void)
{
	int size, error;
	int idx, catno;
	int sys_index, sgi_index;
	int linenumber = 0;
	char *cat;
	char *cp;
	char *label;
	char *msg;

	/*
	 * Create the tables, minimum size is 1.
	 */
	size = (max_syserr + 1) * sizeof(struct errtable);
	syserrtab = (struct errtable *) malloc (size);
	bzero(syserrtab, size);

	size = (max_sgierr - SGIBASE + 1) * sizeof(struct errtable);
	sgierrtab = (struct errtable *) malloc (size);
	bzero(sgierrtab, size);

	/*
	 * First message in each catalog is for "Unknown error".
	 * Catalog index starts at 1.
	 */
	sys_index = sgi_index = 1;
	fprintf(fsyserr, "%s%s:%s\n", prefix, "UNKNOWNERR", "Unknown error");
	fprintf(fsgierr, "%s%s:%s\n", prefix, "UNKNOWNERR", "Unknown error");

	/*
	 * For system catalog, second message is COLON ": "
	 * and third message is ERROR0 "Error 0".
	 */
	sys_index++;
	fprintf(fsyserr, "%s%s:%s\n", prefix, "COLON", ": ");
	sys_index++;
	fprintf(fsyserr, "%s%s:%s\n", prefix, "ERROR0", "Error 0");
	syserrtab[0].index = sys_index;
	syserrtab[0].msg = strdup("Error 0");
	
	/*
	 * Scan the formated error list input file. Build the two internal
	 * tables and create the input file for the message catalog generation
	 * tool.
	 */
	while (cp = nextline(fconfig)) {
		linenumber++;
		if (*cp == '#' || *cp == '\0')  /* ignore comments and \n */
			continue;
		if (sscanf(cp, "%d", &error) == 0)	/* skip bad lines */
			continue;
		skip(&cp, 1);			/* skip over error number */
		if (NULL == (cat = skip(&cp, 1))) {
			fprintf(stderr, "%s: line %d - no message catalog\n",
				progname, linenumber);
			continue;
		}
		if (NULL == (label = skip(&cp, 1))) {
			fprintf(stderr, "%s: line %d - no label\n",
				progname, linenumber);
			continue;
		}
		if (NULL == (msg = skip(&cp, 0))) {
			fprintf(stderr, "%s: line %d - no message string\n",
				progname, linenumber);
			continue;
		}
		/*
		 * Write into the respective generated message catalog
		 * input file.
		 */
		if (!strcmp(cat, SYSERR)) {
			catno = CATNO_SYSERR;
			idx = ++sys_index;
			fprintf(fsyserr, "%s%s:%s\n", prefix, label, msg);
		} else {
			catno = CATNO_SGIERR;
			idx = ++sgi_index;
			fprintf(fsgierr, "%s%s:%s\n", prefix, label, msg);
		}
		/*
		 * Update the corresponding internal table.
		 *
		 * No check is made to determine if this new message
		 * superceded a previous message. The last one wins.
		 * If a message is superceded, the pointer to that message
		 * is lost without an attempt to free the allocate string.
		 * This should be okay as the memory is returned when
		 * the program exits.
		 */
		if (error < SGIBASE) {
			syserrtab[error].cat = catno;
			syserrtab[error].index = idx;
			syserrtab[error].msg = strdup(msg);
		} else {
			sgierrtab[error-SGIBASE].cat = catno;
			sgierrtab[error-SGIBASE].index = idx;
			sgierrtab[error-SGIBASE].msg = strdup(msg);
		}
	}
}	/* maketables() */


void
addcopyright(FILE * fp)
{
	fprintf(fp, "/**************************************************************************\n");
	fprintf(fp, " *                                                                        *\n");
	fprintf(fp, " *               Copyright (C) 1992 Silicon Graphics, Inc.                *\n");
	fprintf(fp, " *                                                                        *\n");
	fprintf(fp, " *  These coded instructions, statements, and computer programs  contain  *\n");
	fprintf(fp, " *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *\n");
	fprintf(fp, " *  are protected by Federal copyright law.  They  may  not be disclosed  *\n");
	fprintf(fp, " *  to  third  parties  or copied or duplicated in any form, in whole or  *\n");
	fprintf(fp, " *  in part, without the prior written consent of Silicon Graphics, Inc.  *\n");
	fprintf(fp, " *                                                                        *\n");
	fprintf(fp, " **************************************************************************/\n\n");

	fprintf(fp, "#ident\t\"@(#)libc-port:gen/%s\"\n\n", rcsid);

}	/* addcopyright() */


void
makecfiles(void)
{
	int i, idx, max_sys_nerr = 0;
	FILE *fp;

	/*
	 * File errlst.c: sys_errlist for backwards compatibility
	 */
	fp = ferrlst;
	addcopyright(fp);
	fprintf(fp, "/*LINTLIBRARY*/\n");
	fprintf(fp, "#ifdef __STDC__\n");
	fprintf(fp, "\t#pragma weak sys_errlist = _sys_errlist\n");
	fprintf(fp, "\t#pragma weak sys_nerr = _sys_nerr\n");
	fprintf(fp, "#endif\n");
	fprintf(fp, "#include \"synonyms.h\"\n\n");

	fprintf(fp, "const char * sys_errlist[] = {\n");
	fprintf(fp, "\t\"Error 0\",\n");
	for (i = 1; i <= max_syserr && i < MAX_SYS_NERR; i++) {
		if (syserrtab[i].msg) {
			fprintf(fp, "\t\"%s\",\n", syserrtab[i].msg);
			max_sys_nerr = i;
		} else
			fprintf(fp, "\t\"Error %d\",\n", i);
	}
	fprintf(fp, "};\n\n");
	fprintf(fp, "const int sys_nerr = %d;\n\n", max_sys_nerr+1);

	/*
	 * File new_list.c: Error number to message index translation tables.
	 */
	fp = fnewlst;
	addcopyright(fp);
	fprintf(fp, "/*\n");
	fprintf(fp, " * Translation table for getting a message id from an\n");
	fprintf(fp, " * errno. This table corresponds to \"sys_errlist\".\n");
	fprintf(fp, " * As is the case with \"sys_errlist\", the errno must\n");
	fprintf(fp, " * be less than \"sys_nerr\".\n");
	fprintf(fp, " * Message id 1 is always for an unknown error.\n");
	fprintf(fp, " *\n");
	fprintf(fp, " * In most cases the message id is for the SVR4 message\n");
	fprintf(fp, " * catalog \"uxsyserr\". However, when the id masked with\n");
	fprintf(fp, " * CATNO_SGIERR is true, the id corresponds to the message\n");
	fprintf(fp, " * catalog \"uxsyserr\".\n");
	fprintf(fp, " */\n\n");
	fprintf(fp, "#define	CATNO_SYSERR	0x0000\n");
	fprintf(fp, "#define	CATNO_SGIERR	0x1<<16\n");
	fprintf(fp, "#define	CATNO_MASK	CATNO_SYSERR|CATNO_SGIERR\n\n");
	fprintf(fp, "const int _sys_index[] = {\n");
	for (i = 0; i <= MAX_SYS_NERR; i++) {
		if (syserrtab[i].index == 0)
			fprintf(fp, "\t1,\t\t/* Error %d */\n", i);
		else if (syserrtab[i].cat == CATNO_SYSERR)
			fprintf(fp, "\t%d,\n", syserrtab[i].index);
		else
			fprintf(fp, "\tCATNO_SGIERR | %d,\t/* Use message catalog %s */\n",
				syserrtab[i].index, SGIERR);
	}
	fprintf(fp, "}; /* _sys_index[] */\n\n\n");

	fprintf(fp, "struct errtable {\n");
	fprintf(fp, "\tint\terrnum;\n");
	fprintf(fp, "\tint\tmsgidx;\n");
	fprintf(fp, "\tchar\t*msg;\n");
	fprintf(fp, "};\n\n");

	fprintf(fp, "/*\n");
	fprintf(fp, " * Message table for system errno > \"sys_nerr\".\n");
	fprintf(fp, " */\n");
	fprintf(fp, "const struct errtable _sys_errtable[] = {\n");
	for (idx = MAX_SYS_NERR; idx <= max_syserr; idx++) {
		if (syserrtab[idx].index)
			fprintf(fp, "\t{%d,\t%d,\t\"%s\"},\n",
			idx, syserrtab[idx].index, syserrtab[idx].msg);
	}
	fprintf(fp, "\t{0,\t0,\t0},\n");
	fprintf(fp, "};\n\n");

	fprintf(fp, "/*\n");
	fprintf(fp, " * Message table for Irix errors.\n");
	fprintf(fp, " */\n");
	fprintf(fp, "const struct errtable _sgi_errtable[] = {\n");
	for (i = SGIBASE; i <= max_sgierr; i++) {
		idx = i - SGIBASE;
		if (sgierrtab[idx].index)
			fprintf(fp, "\t{%d,\t%d,\t\"%s\"},\n",
			i, sgierrtab[idx].index, sgierrtab[idx].msg);
	}
	fprintf(fp, "\t{0,\t0,\t0},\n");
	fprintf(fp, "};\n\n");

	fprintf(fp, "const int _sys_num_err = %d;\t\t/* high sys errno */\n",
		max_syserr);
	fprintf(fp, "const int _sgi_num_err = %d;\t\t/* high sgi errno */\n",
		max_sgierr);
}	/* makecfiles() */


int
main (int argc, char **argv)
{
	progname = argv[0];
	configfile = argv[1];

	openfiles();
	getmaxerrs();		/* get highest system and sgi error number */
	rewind(fconfig);
	maketables();		/* create system and sgi errno tables */
	makecfiles();		/* generate .c file for libc */
	exit(0);
	/*NOTREACHED*/
}