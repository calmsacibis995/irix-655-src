/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/tic_scan.c	1.12"
/*********************************************************************
*                         COPYRIGHT NOTICE                           *
**********************************************************************
*        This software is copyright (C) 1982 by Pavel Curtis         *
*                                                                    *
*        Permission is granted to reproduce and distribute           *
*        this file by any means so long as no fee is charged         *
*        above a nominal handling fee and so long as this            *
*        notice is always included in the copies.                    *
*                                                                    *
*        Other rights are reserved except as explicitly granted      *
*        by written permission of the author.                        *
*                Pavel Curtis                                        *
*                Computer Science Dept.                              *
*                405 Upson Hall                                      *
*                Cornell University                                  *
*                Ithaca, NY 14853                                    *
*                                                                    *
*                Ph- (607) 256-4934                                  *
*                                                                    *
*                Pavel.Cornell@Udel-Relay   (ARPAnet)                *
*                decvax!cornell!pavel       (UUCPnet)                *
*********************************************************************/

/*
 *	comp_scan.c --- Lexical scanner for terminfo compiler.
 *
 *   $Log: tic_scan.c,v $
 *   Revision 1.4  1998/09/18 19:47:22  sherwood
 *   SJIS feature for 6.5.2f
 *
 *   Revision 1.5  1998/09/16 02:32:01  rkm
 *   i18n message cleanup
 *
 *   Revision 1.4  1998/01/30 02:43:09  ktill
 *   merged HCL changes
 *
 *   Revision 1.3.18.1  1997/10/31 08:46:05  scm
 *   I18N changes for SJIS/BIG5 support.
 *
 * Revision 1.3.18.2  1997/10/04  19:22:30  rajkr
 * New Coding and Cataloguing Changes
 *
 * Revision 1.3  1993/09/09  00:03:11  igehy
 * Converted to 64-bit.
 *
 * Revision 1.2  1993/08/04  22:29:43  wtk
 * Fullwarn of libcurses
 *
 * Revision 1.1  1991/12/06  14:06:20  daveh
 *
 * Revision 2.1  82/10/25  14:45:55  pavel
 * Added Copyright Notice
 *
 * Revision 2.0  82/10/24  15:17:12  pavel
 * Beta-one Test Release
 *
 * Revision 1.3  82/08/23  22:30:03  pavel
 * The REAL Alpha-one Release Version
 *
 * Revision 1.2  82/08/19  19:10:06  pavel
 * Alpha Test Release One
 *
 * Revision 1.1  82/08/12  18:37:46  pavel
 * Initial revision
 *
 *
 */


#include <stdio.h>
#include <ctype.h>
#include "curses_inc.h"
#include "compiler.h"

#include <locale.h>
#include <nl_types.h>
#include <msgs/uxeoe.h>

extern nl_catd catd;

#define iswhite(ch)	(ch == ' '  ||  ch == '\t')

static void 	backspace(void);
static int	next_char(void);
static int 	trans_string(char *);
static int	first_column;		/* See 'next_char()' below */



/*
 *	int
 *	get_token()
 *
 *	Scans the input for the next token, storing the specifics in the
 *	global structure 'curr_token' and returning one of the following:
 *
 *		NAMES		A line beginning in column 1.  'name'
 *				will be set to point to everything up to
 *				but not including the first comma on the line.
 *		BOOLEAN		An entry consisting of a name followed by
 *				a comma.  'name' will be set to point to the
 *				name of the capability.
 *		NUMBER		An entry of the form
 *					name#digits,
 *				'name' will be set to point to the capability
 *				name and 'valnumber' to the number given.
 *		STRING		An entry of the form
 *					name=characters,
 *				'name' is set to the capability name and
 *				'valstring' to the string of characters, with
 *				input translations done.
 *		CANCEL		An entry of the form
 *					name@,
 *				'name' is set to the capability name and
 *				'valnumber' to -1.
 *		EOF		The end of the file has been reached.
 *
 */

int
get_token(void)
{
	int		number;
	int		type;
	register int	ch;
	static char	buffer[1024];
	register char	*ptr;
	int		dot_flag = FALSE;

	while ((ch = next_char()) == '\n' || (isascii(ch) && iswhite(ch)))
	    ;

	if (ch == EOF)
	    type = EOF;
	else
	{
	    if (ch == '.')
	    {
		dot_flag = TRUE;

		while ((ch = next_char()) == ' '  ||  ch == '\t')
		    ;
	    }

	    if (! isascii(ch) || ! isalnum(ch))
	    {
		 warning(CATGETS(catd, _MSG_TIC_ILLCHAR), ch);
		 panic_mode(',');
	    }

	    ptr = buffer;
	    if (ch != '\n') *(ptr++) = (char) ch;

	    if (first_column)
	    {
		while ((ch = next_char()) != ',' && ch != '\n' && ch != EOF)
		    *(ptr++) = (char) ch;

		if (ch == EOF)
		    err_abort("Premature EOF");
		else if (ch == '\n') {
		    warning(CATGETS(catd, _MSG_TIC_NEWLINE_TERM));
		    panic_mode(',');
		}

		*ptr = '\0';
		curr_token.tk_name = buffer;
		type = NAMES;
	    }
	    else
	    {
		ch = next_char();
		while (isascii(ch) && isalnum(ch))
		{
		    *(ptr++) = (char) ch;
		    ch = next_char();
		}

		*ptr++ = '\0';
		switch (ch)
		{
		    case ',':
			curr_token.tk_name = buffer;
			type = BOOLEAN;
			break;

		    case '@':
			if (next_char() != ',')
			    warning(CATGETS(catd, _MSG_TIC_MISSING_COMMA));
			curr_token.tk_name = buffer;
			type = CANCEL;
			break;

		    case '#':
			number = 0;
			if ((ch = next_char()) == ',') 
			    warning(CATGETS(catd, _MSG_TIC_MISSING_NUM));
			backspace();
			if ((ch = next_char()) == '0')
			    {
			    if ((ch = next_char()) == 'x' || ch == 'X')
				{
				while (isascii(ch = next_char()) &&
				       isxdigit(ch))
				    {
				    number *= 16;
				    if (isdigit(ch))
					number += ch - '0';
				    else if (ch >= 'a' && ch <= 'f')
					number += 10 + ch - 'a';
				    else
					number += 10 + ch - 'A';
				    }
				}
			    else
				{
				backspace();
				while ((ch = next_char()) >= '0' &&
				       ch <= '7')
				    number = number * 8 + ch - '0';
				}
			    }
			else
			    {
			    backspace();
			    while (isascii(ch = next_char()) &&
				   isdigit(ch))
				number = number * 10 + ch - '0';
			    }
			if (ch != ',')
			    warning("Missing comma");
			curr_token.tk_name = buffer;
			curr_token.tk_valnumber = number;
			type = NUMBER;
			break;

		    case '=':
			ch = trans_string(ptr);
			if (ch != NULL && ch != ',')
			    warning(CATGETS(catd, _MSG_TIC_MISSING_COMMA));
			if (ch == NULL)
				warning(CATGETS(catd, _MSG_TIC_NULL_STR_VAL));
			curr_token.tk_name = buffer;
			curr_token.tk_valstring = ptr;
			type = STRING;
			break;

		    default:
		 	warning(CATGETS(catd, _MSG_TIC_ILLCHAR), ch);
		}
	    } /* end else (first_column == FALSE) */
	} /* end else (ch != EOF) */

	if (dot_flag == TRUE)
	    DEBUG(8, "Commented out ", "");

	if (debug_level >= 8)
	{
	    fprintf(stderr, "Token: ");
	    switch (type)
	    {
		case BOOLEAN:
		    fprintf(stderr, 
			    CATGETS(catd, _MSG_TIC_BOOL_MSG),
			    curr_token.tk_name);
		    break;

		case NUMBER:
		    fprintf(stderr, 
				CATGETS(catd, _MSG_TIC_NUM_MSG),
				curr_token.tk_name, 
				curr_token.tk_valnumber);
		    break;

		case STRING:
		    fprintf(stderr, 
				CATGETS(catd, _MSG_TIC_STR_MSG),
				curr_token.tk_name, 
				curr_token.tk_valstring);
		    break;

		case CANCEL:
		    fprintf(stderr, 
			    CATGETS(catd, _MSG_TIC_CANCEL_MSG),
			    curr_token.tk_name);
		    break;

		case NAMES:

		    fprintf(stderr, 
			    CATGETS(catd, _MSG_TIC_NAME_MSG),
			    curr_token.tk_name);
	
		    break;

		case EOF:

		    fprintf(stderr, 
			    CATGETS(catd, _MSG_TIC_EOF_MSG));
		    break;

		default:

		    warning(CATGETS(catd, _MSG_TIC_BADTOKEN));
	    }
	}

	if (dot_flag == TRUE)		/* if commented out, use the next one */
	    type = get_token();

	return(type);
}



/*
 *	int
 *	next_char()
 *
 *	Returns the next character in the input stream.  Comments and leading
 *	white space are stripped.  The global state variable 'firstcolumn' is
 *	set TRUE if the character returned is from the first column of the input
 * 	line.  The global variable curr_line is incremented for each new line.
 *	The global variable curr_file_pos is set to the file offset of the
 *	beginning of each line.
 *
 */

int	curr_column = -1;
char	line[1024];

static int
next_char(void)
{
	char	*rtn_value;

	if (curr_column < 0  ||  curr_column > 1023  ||
			    line[curr_column] == '\0')
	{
	    do
	    {
		curr_file_pos = ftell(stdin);

		if ((rtn_value = fgets(line, 1024, stdin)) == NULL)
		    return (EOF);
		curr_line++;
	    } while (line[0] == '#');

	    curr_column = 0;
	    while (isascii(line[curr_column]) && iswhite(line[curr_column]))
		curr_column++;
	}

	if (curr_column == 0  &&  line[0] != '\n')
	    first_column = TRUE;
	else
	    first_column = FALSE;

	return (line[curr_column++]);
}

static void
backspace(void)
{
	curr_column--;

	if (curr_column < 0)
	    syserr_abort( CATGETS(catd, _MSG_TIC_BACKSPACED_OFF));
}



/*
 *	reset_input()
 *
 *	Resets the input-reading routines.  Used after a seek has been done.
 *
 */

void
reset_input(void)
{
	curr_column = -1;
}



/*
 *	int
 *	trans_string(ptr)
 *
 *	Reads characters using next_char() until encountering a comma, a new
 *	entry, or end-of-file.  The returned value is the character which caused
 *	reading to stop.  The following translations are done on the input:
 *
 *		^X  goes to  ctrl-X (i.e. X & 037)
 *		{\E,\n,\r,\b,\t,\f}  go to
 *			{ESCAPE,newline,carriage-return,backspace,tab,formfeed}
 *		{\^,\\}  go to  {carat,backslash}
 *		\ddd (for ddd = up to three octal digits)  goes to
 *							the character ddd
 *
 *		\e == \E
 *		\0 == \200
 *
 */

static int
trans_string(char *ptr)
{
	register int	count = 0;
	int		number;
	register int	i;
	register int	ch;

	while ((ch = next_char()) != ','  &&  ch != EOF && !first_column)
	{
	    if (ch == '^')
	    {
		ch = next_char();
		if (ch == EOF)
		    err_abort( CATGETS(catd, _MSG_TIC_PREMATURE_EOF));

		if (!isascii(ch) || ! isprint(ch))
		{
		    warning( CATGETS(catd, _MSG_TIC_ILL_CTRL_CHAR), ch );
		}

		if (ch == '@')
		    *(ptr++) = 0200;
		else
		    *(ptr++) = (char) (ch & 037);
	    }
	    else if (ch == '\\')
	    {
		ch = next_char();
		if (ch == EOF)
		    err_abort( CATGETS(catd, _MSG_TIC_PREMATURE_EOF));

		if (ch >= '0'  &&  ch <= '7')
		{
		    number = ch - '0';
		    for (i=0; i < 2; i++)
		    {
			ch = next_char();
			if (ch == EOF)
		    	    err_abort( CATGETS(catd, _MSG_TIC_PREMATURE_EOF));

			if (ch < '0'  ||  ch > '7')
			{
			    backspace();
			    break;
			}

			number = number * 8 + ch - '0';
		    }

		    if (number == 0)
			number = 0200;
		    *(ptr++) = (char) number;
		}
		else
		{
		    switch (ch)
		    {
			case 'E':
			case 'e':	*(ptr++) = '\033';	break;

			case 'l':
			case 'n':	*(ptr++) = '\n';	break;

			case 'r':	*(ptr++) = '\r';	break;

			case 'b':	*(ptr++) = '\010';	break;

			case 's':	*(ptr++) = ' ';		break;

			case 'f':	*(ptr++) = '\014';	break;

			case 't':	*(ptr++) = '\t';	break;

			case '\\':	*(ptr++) = '\\';	break;

			case '^':	*(ptr++) = '^';		break;

			case ',':	*(ptr++) = ',';		break;

			case ':':	*(ptr++) = ':';		break;

			default:
			    warning(CATGETS(catd, _MSG_TIC_ILL_CHAR_SEQ),ch);
			    *(ptr++) = (char) ch;
		    } /* endswitch (ch) */
		} /* endelse (ch < '0' ||  ch > '7') */
	    } /* end else if (ch == '\\') */
	    else
	    {
		if (ch != '\n') *(ptr++) = (char) ch;
	    }

	    count ++;

	    if (count > 1000)
		warning(CATGETS(catd, _MSG_TIC_LONG_STR_COMMA));
	} /* end while */

	if (ch == EOF)
	    warning(CATGETS(catd, _MSG_TIC_PREMATURE_EOF_COMMA));
	/* start of new description */
	else if (first_column)
	{
	    backspace();
	    warning(CATGETS(catd, _MSG_TIC_MISSING_COMMA));
	    /* pretend we did get a comma */
	    ch = ',';
	}

	*ptr = '\0';

	if (count == 0) return(NULL);
	return(ch);
}

/*
 * Panic mode error recovery - skip everything until a "ch" is found.
 */

void
panic_mode(register int ch)
{
	register int c;

	for (;;) {
		c = next_char();
		if (c == ch)
			return;
		if (c == EOF)
			return;
	}
}