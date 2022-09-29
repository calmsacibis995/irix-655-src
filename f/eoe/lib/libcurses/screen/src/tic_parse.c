/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/tic_parse.c	1.16"
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
 *	comp_parse.c -- The high-level (ha!) parts of the compiler,
 *			that is, the routines which drive the scanner,
 *			etc.
 *
 *   $Log: tic_parse.c,v $
 *   Revision 1.5  1998/09/18 19:47:22  sherwood
 *   SJIS feature for 6.5.2f
 *
 *   Revision 1.6  1998/09/16 02:31:57  rkm
 *   i18n message cleanup
 *
 *   Revision 1.5  1998/01/30 02:42:42  ktill
 *   merged HCL changes
 *
 *   Revision 1.4.18.2  1998/01/09 06:07:50  himanshu
 *   modified for comment ending '/' in file header.
 *
 * Revision 1.4.18.1  1997/10/31  08:46:01  scm
 * I18N changes for SJIS/BIG5 support.
 *
 * Revision 1.4.18.2  1997/10/04  19:22:08  rajkr
 * New Coding and Cataloguing Changes
 *
 * Revision 1.4  1993/09/09  00:03:04  igehy
 * Converted to 64-bit.
 *
 * Revision 1.3  1993/08/04  22:29:37  wtk
 * Fullwarn of libcurses
 *
 * Revision 1.2  1991/12/09  16:35:01  daveh
 * Merge in long filename fixes and symlink capability from IRIX,
 *
 * Revision 2.1  82/10/25  14:45:43  pavel
 * Added Copyright Notice
 *
 * Revision 2.0  82/10/24  15:16:39  pavel
 * Beta-one Test Release
 *
 * Revision 1.3  82/08/23  22:29:39  pavel
 * The REAL Alpha-one Release Version
 *
 * Revision 1.2  82/08/19  19:09:53  pavel
 * Alpha Test Release One
 *
 * Revision 1.1  82/08/12  18:37:12  pavel
 * Initial revision
 *
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include "curses_inc.h"
#include "compiler.h"
#include "object.h"
#include "limits.h"


#include <locale.h>
#include <nl_types.h>
#include <msgs/uxeoe.h>
extern nl_catd catd;


extern	char check_only;
extern	char *progname;
extern	int symlinks;

struct	use_item;
static 	void enqueue(long);
static 	void dequeue(struct use_item *);
static 	void change_cancellations(short[]);
static	void dump_structure(short[], short[], short[]);
static	void init_structure(short[], short[], short[]);
static	void elim_cancellations(short[], short[], short[]);
static	int save_str(char *);
static	int invalid_term_name(char *);
static	int do_entry(struct use_item *);
static	int write_object(FILE *, short[], short[], short[]);
static	int handle_use(struct use_item *, long, short[], short[], short[]);

char 	*string_table;
int	next_free;	/* next free character in string_table */
unsigned int	table_size = 0; /* current string_table size */
short	term_names;	/* string table offset - current terminal */
int	part2 = 0;	/* set to allow old compiled defns to be used */
int	complete = 0;	/* 1 if entry done with no forward uses */

struct use_item
{
	long	offset;
	struct use_item	*fptr, *bptr;
};

struct use_header
{
	struct use_item	*head, *tail;
};

struct use_header	use_list = {NULL, NULL};
int			use_count = 0;

/*
 *  The use_list is a doubly-linked list with NULLs terminating the lists:
 *
 *	   use_item    use_item    use_item
 *	  ---------   ---------   ---------
 *	  |       |   |       |   |       |   offset
 *        |-------|   |-------|   |-------|
 *	  |   ----+-->|   ----+-->|  NULL |   fptr
 *	  |-------|   |-------|   |-------|
 *	  |  NULL |<--+----   |<--+----   |   bptr
 *	  ---------   ---------   ---------
 *	  ^                       ^
 *	  |  ------------------   |
 *	  |  |       |        |   |
 *	  +--+----   |    ----+---+
 *	     |       |        |
 *	     ------------------
 *	       head     tail
 *	          use_list
 *
 */


/*
 *	compile()
 *
 *	Main loop of the compiler.
 *
 *	get_token()
 *	if curr_token != NAMES
 *	    err_abort()
 *	while (not at end of file)
 *	    do an entry
 *
 */

void
compile(void)
{
	char			line[1024];
	int			token_type;
	struct use_item	*ptr;
	int			old_use_count;

	token_type = get_token();

	if (token_type != NAMES)
	    err_abort("File does not start with terminal names in column one");

	while (token_type != EOF)
	    token_type = do_entry((struct use_item *) NULL);

	DEBUG(2, CATGETS(catd, _MSG_TIC_START_USE), "");

	for (part2=0; part2<2; part2++) {
	    old_use_count = -1;
	DEBUG(2, CATGETS(catd, _MSG_TIC_PART), part2);
	    while (use_list.head != NULL  &&  old_use_count != use_count)
	    {
		old_use_count = use_count;
		for (ptr = use_list.tail; ptr != NULL; ptr = ptr->bptr)
		{
		    fseek(stdin, ptr->offset, 0);
		    reset_input();
		    if ((token_type = get_token()) != NAMES)
			syserr_abort("Token after a seek not NAMES");
		    (void) do_entry(ptr);
		    if (complete)
			dequeue(ptr);
		}

		for (ptr = use_list.head; ptr != NULL; ptr = ptr->fptr)
		{
		    fseek(stdin, ptr->offset, 0);
		    reset_input();
		    if ((token_type = get_token()) != NAMES)
			syserr_abort("Token after a seek not NAMES");
		    (void) do_entry(ptr);
		    if (complete)
			dequeue(ptr);
		}

		DEBUG(2, CATGETS(catd, _MSG_TIC_END_ENQ), "");
	    }
	}

	if (use_list.head != NULL && !check_only)
	{
	fprintf(stderr, CATGETS(catd, _MSG_TIC_LOOP));

	    for (ptr = use_list.head; ptr != NULL; ptr = ptr->fptr)
	    {
		fseek(stdin, ptr->offset, 0);
		fgets(line, 1024, stdin);
		fprintf(stderr, "%s", line);
	    }

	    exit(1);
	}
}

void
dump_list(char *str)
{
	struct use_item *ptr;
	char line[512];

	fprintf(stderr, 
		CATGETS(catd, _MSG_TIC_DUMP_LIST), 
		str);
	for (ptr = use_list.head; ptr != NULL; ptr = ptr->fptr)
	{
		fseek(stdin, ptr->offset, 0);
		fgets(line, 1024, stdin);
		fprintf(stderr, "ptr %x off %d bptr %x fptr %x str %s",
		ptr, ptr->offset, ptr->bptr, ptr->fptr, line);
	}
	fprintf(stderr, "\n");
}

/*
 *	int
 *	do_entry(item_ptr)
 *
 *	Compile one entry.  During the first pass, item_ptr is NULL.  In pass
 *	two, item_ptr points to the current entry in the use_list.
 *
 *	found-forward-use = FALSE
 *	re-initialise internal arrays
 *	save names in string_table
 *	get_token()
 *	while (not EOF and not NAMES)
 *	    if found-forward-use
 *		do nothing
 *	    else if 'use'
 *		if handle_use() < 0
 *		    found-forward-use = TRUE
 *          else
 *	        check for existance and type-correctness
 *	        enter cap into structure
 *	        if STRING
 *	            save string in string_table
 *	    get_token()
 *      if ! found-forward-use
 *	    dump compiled entry into filesystem
 *
 */

static int
do_entry(struct use_item *item_ptr)
{
	long					entry_offset;
	register int				token_type;
	register struct name_table_entry	*entry_ptr;
	int					found_forward_use = FALSE;
	short					Booleans[MAXBOOLS],
						Numbers[MAXNUMS],
						Strings[MAXSTRINGS];

	init_structure(Booleans, Numbers, Strings);
	complete = 0;
	term_names = (short) save_str(curr_token.tk_name);
	DEBUG(2, CATGETS(catd, _MSG_TIC_STARTING), curr_token.tk_name);
	entry_offset = curr_file_pos;

	for (token_type = get_token();
		token_type != EOF  &&  token_type != NAMES;
		token_type = get_token())
	{
	    if (found_forward_use)
		/* do nothing */ ;
	    else if (strcmp(curr_token.tk_name, "use") == 0)
	    {
		if (handle_use(item_ptr, entry_offset,
					Booleans, Numbers, Strings) < 0)
		    found_forward_use = TRUE;
	    }
	    else
	    {
		entry_ptr = find_entry(curr_token.tk_name);

		if (entry_ptr == NOTFOUND) {
		    warning(CATGETS(catd, _MSG_TIC_UNKNOWN_CAP),
			  curr_token.tk_name); 
		    continue;
		}


		if (token_type != CANCEL
					&&  entry_ptr->nte_type != token_type)
		    warning(CATGETS(catd, _MSG_TIC_WRONG_CAP),
			    curr_token.tk_name);
		switch (token_type)
		{
		    case CANCEL:
			switch (entry_ptr->nte_type)
			{
			    case BOOLEAN:
				Booleans[entry_ptr->nte_index] = -2;
				break;

			    case NUMBER:
				Numbers[entry_ptr->nte_index] = -2;
				break;

			    case STRING:
				Strings[entry_ptr->nte_index] = -2;
				break;
			}
			break;

		    case BOOLEAN:
			if (Booleans[entry_ptr->nte_index] == 0)
				Booleans[entry_ptr->nte_index] = TRUE;
			break;

		    case NUMBER:
			if (Numbers[entry_ptr->nte_index] == -1)
				Numbers[entry_ptr->nte_index] =
				(short) curr_token.tk_valnumber;
			break;

		    case STRING:
			if (Strings[entry_ptr->nte_index] == -1)
				Strings[entry_ptr->nte_index] =
				(short) save_str(curr_token.tk_valstring);
			break;

		    default:
			warning(CATGETS(catd, _MSG_TIC_UNKNOWN_TOKEN));
			panic_mode(',');
			continue;
		}
	    } /* end else cur_token.name != "use" */

	} /* endwhile (not EOF and not NAMES) */

	if (found_forward_use)
	    return(token_type);

	dump_structure(Booleans, Numbers, Strings);

	complete = 1;
	return(token_type);
}

/*
    Change all cancellations to a non-entry.
    For booleans, @ -> false
    For nums, @ -> -1
    For strings, @ -> -1

    This only has to be done for entries which
    have to be compatible with the pre-Vr3 format.
*/
#ifndef NOCANCELCOMPAT
static void
elim_cancellations(short Booleans[], short Numbers[], short Strings[])
{
	register int i;
	for (i=0; i < BoolCount; i++)
	{
	    if (Booleans[i] == -2)
		Booleans[i] = FALSE;
	}

	for (i=0; i < NumCount; i++)
	{
	    if (Numbers[i] == -2)
		Numbers[i] = -1;
	}

	for (i=0; i < StrCount; i++)
	{
	    if (Strings[i] == -2)
		Strings[i] = -1;
	}
}
#endif /* NOCANCELCOMPAT */
/*
    Change the cancellation signal from the -2 used internally to
    the 2 used within the binary.
*/

static void
change_cancellations(short Booleans[])
{
	register int i;
	for (i=0; i < BoolCount; i++)
	{
	    if (Booleans[i] == -2)
		Booleans[i] = 2;
	}

}

/*
 *	enqueue(offset)
 *
 *      Put a record of the given offset onto the use-list.
 *
 */

static void
enqueue(long offset)
{
	struct use_item	*item;

	item = (struct use_item *) malloc(sizeof(struct use_item));

	if (item == NULL)
	    syserr_abort("Not enough memory for use_list element");

	item->offset = offset;

	if (use_list.head != NULL)
	{
	    item->bptr = use_list.tail;
	    use_list.tail->fptr = item;
	    item->fptr = NULL;
	    use_list.tail = item;
	}
	else
	{
	    use_list.tail = use_list.head = item;
	    item->fptr = item->bptr = NULL;
	}

	use_count ++;
}

/*
 *	dequeue(ptr)
 *
 *	remove the pointed-to item from the use_list
 *
 */

void
dequeue(struct use_item *ptr)
{
	if (ptr->fptr == NULL)
	    use_list.tail = ptr->bptr;
	else
	    (ptr->fptr)->bptr = ptr->bptr;

	if (ptr->bptr == NULL)
	    use_list.head = ptr->fptr;
	else
	    (ptr->bptr)->fptr = ptr->fptr;

	use_count --;
}

/*
 *	invalid_term_name(name)
 *
 *	Look for invalid characters in a term name. These include
 *	space, tab and '/'.
 *
 *	Generate an error message if given name does not begin with a
 *	digit or letter, then exit.
 *
 *	return TRUE if name is invalid.
 *
 */

static int
invalid_term_name(register char *name)
{
	int error = 0;
	if (! isdigit(*name)  &&  ! islower(*name) && ! isupper(*name))
	    error++;

	for ( ; *name ; name++)
	    if (isalnum(*name))
		continue;
	    else if (isspace(*name) || (*name == '/'))
		return 1;
	if (error)
	{
	    fprintf(stderr, 	
		    CATGETS(catd, _MSG_TIC_LINE_ILL_TTY), 
		    progname, 
		    curr_line, 
		    name);
	    fprintf(stderr,
			CATGETS(catd, _MSG_TIC_TTY_NAME));
	    exit(1);
	}
	return 0;
}

/*
 *	dump_structure()
 *
 *	Save the compiled version of a description in the filesystem.
 *
 *	make a copy of the name-list
 *	break it up into first-name and all-but-last-name
 *	if necessary
 *	    clear CANCELS out of the structure
 *	creat(first-name)
 *	write object information to first-name
 *	close(first-name)
 *      for each valid name
 *	    link to first-name
 *
 */

static void
dump_structure(short Booleans[], short Numbers[], short Strings[])
{
	struct stat	statbuf;
	FILE		*fp;
	char		name_list[1024];
	register char	*first_name, *other_names, *cur_name;
	char		filename[128];
	char		linkname[128];
	size_t		len;
	int		alphastart = 0;

	strcpy(name_list, term_names + string_table);
	DEBUG(7, CATGETS(catd, _MSG_TIC_NAME_LIST), name_list);

	first_name = name_list;
	/* Set othernames to 1 past first '|' in the list. */
	/* Null out that '|' in the process. */
	other_names = strchr(first_name, '|');
	if (other_names)
	    *other_names++ = '\0';

	if (invalid_term_name(first_name))
	    warning(CATGETS(catd, _MSG_TIC_BADNAME),
			first_name );


	DEBUG(7, CATGETS(catd, _MSG_TIC_FIRSTNAME), first_name);
	DEBUG(7, CATGETS(catd, _MSG_TIC_OTHER_NAMES), other_names ? other_names : "NULL");

	if((len = strlen(first_name)) > 100)
	    warning(CATGETS(catd, _MSG_TIC_LONGNAME),
		    first_name);
	else if (len == 1)
	    warning(CATGETS(catd, _MSG_TIC_SHORTNAME),
		    first_name);
			

	check_dir(first_name[0]);

	sprintf(filename, "%c/%s", first_name[0], first_name);

	if (strlen(filename) > NAME_MAX )
		warning(CATGETS(catd, _MSG_TIC_TRUNCNAME),
			filename, filename);
	if (stat(filename, &statbuf) >= 0  &&  statbuf.st_mtime >= start_time)
	{
	    warning(CATGETS(catd, _MSG_TIC_MULTIDEF),
			first_name);
			
	    fprintf(stderr, 
			CATGETS(catd, _MSG_TIC_ENTRY_USE),
			(unsigned) term_names + string_table);
	}

	if (!check_only) {
		unlink(filename);
		fp = fopen(filename, "w");
		if (fp == NULL)
		{
		    perror(filename);
		    syserr_abort(CATGETS(catd, _MSG_TIC_CANT_OPEN2),
				 destination, filename);
		}
		DEBUG(1, CATGETS(catd, _MSG_TIC_CREATED), filename);
	}
	else 
	DEBUG(1, CATGETS(catd, _MSG_TIC_WHCR), filename);

#ifndef NOCANCELCOMPAT
	/* if there is no '+' in the name, eliminate */
	/* cancellation markings. */
	if (strchr(first_name, '+') == 0)
		elim_cancellations(Booleans, Numbers, Strings);
	else
#endif /* NOCANCELCOMPAT */
		change_cancellations(Booleans);

	if (!check_only) {
		if (write_object(fp, Booleans, Numbers, Strings) < 0)
		{
		    syserr_abort(CATGETS(catd, _MSG_TIC_WRITE_ERROR),
				 destination, filename);
		}
		fclose(fp);
	}

	alphastart = isalpha(first_name[0]);

	while (other_names)
	{
	    cur_name = other_names;
	    other_names = strchr(cur_name, '|');
	    if (other_names)
		*other_names++ = '\0';
	    if (*cur_name == '\0')
		continue;

	    if ((len = strlen(cur_name)) > 100)
	    {
			warning(
				CATGETS(catd, _MSG_TIC_TTY_TOOLONG),
				cur_name);
			continue;
	    }
	    else if (len == 1)
	    {
		warning(
			CATGETS(catd, _MSG_TIC_TTY_TOOSHORT), 
			first_name);
		continue;
	    }

	    if (invalid_term_name(cur_name))
	    {
		if (other_names)
		    warning(
			CATGETS(catd, _MSG_TIC_BAD_TTY), 
			cur_name);
		continue;
	    }

	    check_dir(cur_name[0]);

	    sprintf(linkname, "%c/%s", cur_name[0], cur_name);

	if (strlen(linkname) > NAME_MAX )

		warning(
			CATGETS(catd, _MSG_TIC_LONG_LINK), 
			linkname, linkname);
	    alphastart |= isalpha(cur_name[0]);

	    if (strcmp(first_name, cur_name) == 0)
	    {
		warning(
			CATGETS(catd, _MSG_TIC_TERM_SYNONYM), 
			first_name);
	    }
	    else  {
		if (!check_only) {
			if (stat(linkname, &statbuf) >= 0  &&
				statbuf.st_mtime >= start_time) {
				warning(
					CATGETS(catd, _MSG_TIC_MULTIDEF), 
					cur_name);
			    fprintf(stderr, 
				    CATGETS(catd, _MSG_TIC_ENTRY_USE),
				    (unsigned) term_names + string_table);
			}
			unlink(linkname);
#ifdef _SGI_SOURCE
			{
			if (symlinks) {
			    char fname[256];
			    if ( *linkname != *filename ) {
			        strcpy(fname, "../");
			        strcat(fname, filename);
			        }
			    else
				strcpy(fname, filename+2);
			    if (symlink(fname, linkname) < 0)
			        syserr_abort(
					     CATGETS(catd, _MSG_TIC_LINK_ERROR),
							fname, linkname);
			    }
			else
			if (link(filename, linkname) < 0)
			    syserr_abort(
				CATGETS(catd, _MSG_TIC_LINK_ERROR),
				filename,
				linkname);
			}
#else
			    syserr_abort(
				CATGETS(catd, _MSG_TIC_LINK_ERROR),
				filename,
				linkname);
#endif	/* if sgi sources */
			DEBUG(1, CATGETS(catd, _MSG_TIC_LINKED), linkname);
		}
		else 
		DEBUG(1, CATGETS(catd, _MSG_TIC_WHCRL), linkname);

	    }
	}

	if (!alphastart)
	{
	    warning(CATGETS(catd, _MSG_TIC_SYN_BEGIN));
	}
}

/*
 *	int
 *	write_object(fp, Booleans, Numbers, Strings)
 *
 *	Write out the compiled entry to the given file.
 *	Return 0 if OK or -1 if not.
 *
 */

#define swap(x)		(((x >> 8) & 0377) + 256 * (x & 0377))

#define might_swap(x)	(must_swap()  ?  swap(x)  :  (x))


int
write_object(FILE *fp, short Booleans[], short Numbers[], short Strings[])
{
	struct header	header;
	char		*namelist;
	short		namelen;
	char		zero = '\0';
	register int	i;
	char		cBooleans[MAXBOOLS];
	register int	l_next_free;

	namelist = term_names + string_table;
	namelen = (short) (strlen(namelist) + 1);

	l_next_free = next_free;
	if (l_next_free % 256 == 255)
	    l_next_free++;

	if (must_swap())
	{
	    header.magic = swap(MAGIC);
	    header.name_size = (short) swap(namelen);
	    header.bool_count = (short) swap(BoolCount);
	    header.num_count = (short) swap(NumCount);
	    header.str_count = (short) swap(StrCount);
	    header.str_size = (short) swap(l_next_free);
	}
	else
	{
	    header.magic = MAGIC;
	    header.name_size = namelen;
	    header.bool_count = (short) BoolCount;
	    header.num_count = (short) NumCount;
	    header.str_count = (short) StrCount;
	    header.str_size = (short) l_next_free;
	}

	for (i=0; i < BoolCount; i++)
		cBooleans[i] = (char) Booleans[i];

	if (fwrite(&header, sizeof(header), 1, fp)
							!= 1 ||
	    fwrite(namelist, sizeof(char), (size_t) namelen, fp)
							!= (size_t) namelen ||
	    fwrite(cBooleans, sizeof(char), (size_t) BoolCount, fp)
							!= (size_t) BoolCount)
	    return(-1);

	if ((namelen+BoolCount) % 2 != 0  &&  fwrite(&zero, sizeof(char), 1, fp) != 1)
	    return(-1);

	if (must_swap())
	{
	    for (i=0; i < NumCount; i++)
		Numbers[i] = (short) swap(Numbers[i]);
	    for (i=0; i < StrCount; i++)
		Strings[i] = (short) swap(Strings[i]);
	}

	if (fwrite((char *)Numbers, sizeof(short), (size_t) NumCount, fp)
							!= (size_t)NumCount ||
	    fwrite((char *)Strings, sizeof(short), (size_t) StrCount, fp)
							!= (size_t)StrCount ||
	    fwrite(string_table, sizeof(char), (size_t) l_next_free, fp)
							!= (size_t)l_next_free)
	    return(-1);

	return(0);
}

/*
 *	int
 *	save_str(string)
 *
 *	copy string into next free part of string_table, doing a realloc()
 *	if necessary.  return offset of beginning of string from start of
 *	string_table.
 *
 */

int
save_str(char *string)
{
	int	old_next_free;

	/* Do not let an offset be 255. It reads as -1 in Vr2 binaries. */
	if (next_free % 256 == 255)
		next_free++;

	old_next_free = next_free;

	if (table_size == 0)
	{
	    if ((string_table = malloc(1024)) == NULL)
		syserr_abort(CATGETS(catd, _MSG_TIC_NO_MEMORY));
	    table_size = 1024;
	    DEBUG(5, CATGETS(catd, _MSG_TIC_INIT_TABLE),table_size);
	}

	while (table_size < (size_t) next_free + strlen(string))
	{
	    if ((string_table = realloc(string_table, table_size + 1024))
								== NULL)
		syserr_abort(CATGETS(catd, _MSG_TIC_NO_MEMORY));
	    table_size += 1024;
	    DEBUG(5, CATGETS(catd, _MSG_TIC_EXT_TABLE), table_size);
	}

	strcpy(&string_table[next_free], string);
	DEBUG(7, CATGETS(catd, _MSG_TIC_SAVE_STR), string);
	DEBUG(7, CATGETS(catd, _MSG_TIC_AT_LOC), next_free);
	next_free += strlen(string) + 1;

	return(old_next_free);
}

/*
 *	init_structure(Booleans, Numbers, Strings)
 *
 *	Initialise the given arrays
 *	Reset the next_free counter to zero.
 *
 */

static void
init_structure(short Booleans[], short Numbers[], short Strings[])
{
	int	i;

	for (i=0; i < BoolCount; i++)
	    Booleans[i] = FALSE;

	for (i=0; i < NumCount; i++)
	    Numbers[i] = -1;

	for (i=0; i < StrCount; i++)
	    Strings[i] = -1;

	next_free = 0;
}

/*
**	int
**	handle_use(item_ptr, entry_offset, Booleans, Numbers, Strings)
**
**	Merge the compiled file whose name is in cur_token.valstring
**	with the current entry.
**
**		if it's a forward use-link
**	    	    if item_ptr == NULL
**		        queue it up for later handling
**	            else
**		        ignore it (we're already going through the queue)
**	        else it's a backward use-link
**	            read in the object file for that terminal
**	            merge contents with current structure
**
**	Returned value is 0 if it was a backward link and we
**	successfully read it in, -1 if a forward link.
*/

static int
handle_use(struct use_item *item_ptr, long entry_offset,
	short Booleans[], short Numbers[], short Strings[])
{
	struct _bool_struct	use_bools;
	struct _num_struct	use_nums;
	struct _str_struct	use_strs;
	struct stat	statbuf;
	char		filename[50];
	int             i;
	char  *UB = &use_bools._auto_left_margin;/* first bool */
	short *UN = &use_nums._columns;		 /* first num */
	char **US = &use_strs.strs._back_tab;	 /* first str */

	if (invalid_term_name(curr_token.tk_valstring))
	    warning(CATGETS(catd, _MSG_TIC_BAD_TERMNAME), 
		    curr_token.tk_valstring);

	sprintf(filename, "%c/%s", curr_token.tk_valstring[0],
						     curr_token.tk_valstring);

	if (stat(filename, &statbuf) < 0  ||  part2==0 && statbuf.st_mtime < start_time)
	{
	    DEBUG(2, CATGETS(catd, _MSG_TIC_FORWARD_USE), 
		  curr_token.tk_valstring);

	    if (item_ptr == NULL)
	    {
			DEBUG(2, CATGETS(catd, _MSG_TIC_ENQD), "");
			enqueue(entry_offset);
	    }
	    else
		DEBUG(2, CATGETS(catd, _MSG_TIC_SKIPPED), "");

	    return(-1);
	}
	else
	{
	    DEBUG(2, CATGETS(catd, _MSG_TIC_BACK_USE), 
		  curr_token.tk_valstring);
	    if (read_entry(filename, &use_bools, &use_nums, &use_strs) < 0)
		syserr_abort("Error in re-reading compiled file %s", filename);

	    for (i=0; i < BoolCount; i++)
	    {
		if (Booleans[i] == FALSE)
		    if (UB[i] == TRUE)		/* now true */
			Booleans[i] = TRUE;
		    else if (UB[i] > TRUE)	/* cancelled */
			Booleans[i] = -2;
	    }

	    for (i=0; i < NumCount; i++)
	    {
		if (Numbers[i] == -1)
		    Numbers[i] = UN[i];
	    }

	    for (i=0; i < StrCount; i++)
	    {
		if (Strings[i] == -1)
		    if (US[i] == (char *) -1)
			Strings[i] = -2;
		    else if (US[i] != (char *) 0)
			Strings[i] = (short) save_str(US[i]);
	    }

	}
	return(0);
}
