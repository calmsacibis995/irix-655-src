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

#include <less.h>

#define	WHITESP(c)	((c)==' ' || (c)=='\t')

#include <i18n_capable.h>
#include <wchar.h>
#include <stdlib.h>
#define	W_WHITESP(c)	((c)==L' ' || (c)==L'\t')
wchar_t *wTagfile;
wchar_t *wTagpattern;

char *tagfile;
char *tagpattern;

static char *tags = "tags";

/*
 * Find a tag in the "tags" file.
 * Sets "tagfile" to the name of the file containing the tag,
 * and "tagpattern" to the search pattern which should be used
 * to find the tag.
 */
void
findtag(register char *tag)
{
	register char *p;
	register FILE *f;
	register int taglen;
	int search_char;
	static char tline[200];

	if ((f = fopen(tags, "r")) == NULL)
	{
		error(GETTXT(_MSG_MORE_NO_TAGS_FILE));
		tagfile = NULL;
		return;
	}

	taglen = strlen(tag);

	/*
	 * Search the tags file for the desired tag.
	 */
	while (fgets(tline, sizeof(tline), f) != NULL)
	{
		if (strncmp(tag, tline, taglen) != 0 || !WHITESP(tline[taglen]))
			continue;

		/*
		 * Found it.
		 * The line contains the tag, the filename and the
		 * pattern, separated by white space.
		 * The pattern is surrounded by a pair of identical
		 * search characters.
		 * Parse the line and extract these parts.
		 */
		tagfile = tagpattern = NULL;

		/*
		 * Skip over the whitespace after the tag name.
		 */
		for (p = tline;  !WHITESP(*p) && *p != '\0';  p++)
			continue;
		while (WHITESP(*p))
			p++;
		if (*p == '\0')
			/* File name is missing! */
			continue;

		/*
		 * Save the file name.
		 * Skip over the whitespace after the file name.
		 */
		tagfile = p;
		while (!WHITESP(*p) && *p != '\0')
			p++;
		*p++ = '\0';
		while (WHITESP(*p))
			p++;
		if (*p == '\0')
			/* Pattern is missing! */
			continue;

		/*
		 * Save the pattern.
		 * Delete any final "$" anchor from the pattern.
		 * Leave any initial anchor so the search can use it.
		 */
		search_char = *p++;
		tagpattern = p;
		while (*p != search_char && *p != '\0')
			p++;
		if (p[-1] == '$')
			p--;
		*p = '\0';

		(void)fclose(f);
		return;
	}
	(void)fclose(f);
	error(GETTXT(_MSG_MORE_NO_TAG_IN_FILE));
	tagfile = NULL;
}

/*
 * Search for a tag.
 * This is a stripped-down version of search().
 * We don't use search() for several reasons:
 *   -	We don't want to blow away any search string we may have saved.
 *   -	The various regular-expression functions (from different systems:
 *	regcmp vs. re_comp) behave differently in the presence of 
 *	parentheses (which are almost always found in a tag).
 */
int
tagsearch()
{
	off64_t pos, linepos;
	int linenum;
	char *pattern;
	int anchored = (tagpattern[0] == '^');

	/* Set up search for anchored (exact) match or just substring.
	 */
	pattern = anchored ? tagpattern : tagpattern + 1;

	pos = (off64_t)0;
	linenum = find_linenum(pos);

	for (;;)
	{
		/*
		 * Get lines until we find a matching one or 
		 * until we hit end-of-file.
		 */
		if (sigs)
			return (1);

		/*
		 * Read the next line, and save the 
		 * starting position of that line in linepos.
		 */
		linepos = pos;
		pos = forw_raw_line(pos);
		if (linenum != 0)
			linenum++;

		if (pos == NULL_POSITION)
		{
			/*
			 * We hit EOF without a match.
			 */
			error(GETTXT(_MSG_MORE_NO_TAG_FOUND));
			return (1);
		}

		/*
		 * If we're using line numbers, we might as well
		 * remember the information we have now (the position
		 * and line number of the current line).
		 */
		if (linenums)
			add_lnum(linenum, pos);

		/*
		 * Test the line to see if we have a match.
		 */
		if (anchored) {
			if (strcmp(line, pattern) == 0)
				break;
		} else if (strstr(line, pattern))
			break;
	}

	jump_loc(linepos);
	return (0);
}




void
wfindtag(register wchar_t *wTag)
{
	register wchar_t *wp;
	register FILE    *f;
	register int      taglen;
	         wchar_t  wSearch_char;
	static   wchar_t  wTline[ 200 ];
	static   char     mbTagfile[ 200 ];

	if ((f = fopen(tags, "r")) == NULL)
	{
		error(GETTXT(_MSG_MORE_NO_TAGS_FILE));
		wTagfile = NULL;
		tagfile = NULL;
		return;
	}
	

	taglen = wcslen(wTag);

	/*
	 * Search the tags file for the desired tag.
	 */
	while (fgetws(wTline, sizeof(wTline)/sizeof(wchar_t), f) != NULL)
	{
		if ( wcsncmp(wTag, wTline, taglen) != 0 || !W_WHITESP(wTline[taglen]) )
			continue;

		/*
		 * Found it.
		 * The line contains the tag, the filename and the
		 * pattern, separated by white space.
		 * The pattern is surrounded by a pair of identical
		 * search characters.
		 * Parse the line and extract these parts.
		 */
		wTagfile = wTagpattern = NULL;

		/*
		 * Skip over the whitespace after the tag name.
		 */
		for (wp = wTline;  !W_WHITESP(*wp) && *wp != L'\0';  wp++)
			continue;
		while (W_WHITESP(*wp))
			wp++;
		if (*wp == L'\0')
			/* File name is missing! */
			continue;

		/*
		 * Save the file name.
		 * Skip over the whitespace after the file name.
		 */
		wTagfile = wp;
		while (!W_WHITESP(*wp) && *wp != L'\0')
			wp++;
		*wp++ = L'\0';
		while (W_WHITESP(*wp))
			wp++;
		if (*wp == L'\0')
			/* Pattern is missing! */
			continue;

		/*
		 * Save the pattern.
		 * Delete any final "$" anchor from the pattern.
		 * Leave any initial anchor so the search can use it.
		 */
		wSearch_char = *wp++;
		wTagpattern = wp;
		while (*wp != wSearch_char && *wp != L'\0')
			wp++;
		if (wp[-1] == L'$')  
			wp--;
		*wp = L'\0';
		
		wcstombs( mbTagfile, wTagfile, (wcslen(wTagfile)+1) * MB_CUR_MAX );

		tagfile = mbTagfile;
		(void)fclose(f);
		return;
	}
	(void)fclose(f);
	error(GETTXT(_MSG_MORE_NO_TAG_IN_FILE));
	wTagfile = NULL;
	tagfile = NULL;
}



/*
 * Search for a tag.
 * This is a stripped-down version of search().
 * We don't use search() for several reasons:
 *   -	We don't want to blow away any search string we may have saved.
 *   -	The various regular-expression functions (from different systems:
 *	regcmp vs. re_comp) behave differently in the presence of 
 *	parentheses (which are almost always found in a tag).
 */

int
wtagsearch()
{
	off64_t pos, linepos;
	int linenum;
	wchar_t *wPattern;
	int anchored = (wTagpattern[0] == L'^');

	/* Set up search for anchored (exact) match or just substring.
	 */
	wPattern = anchored ? wTagpattern : wTagpattern + 1;

	pos = (off64_t)0;
	linenum = find_linenum(pos);

	for (;;)
	{
		/*
		 * Get lines until we find a matching one or 
		 * until we hit end-of-file.
		 */
		if (sigs)
			return (1);

		/*
		 * Read the next line, and save the 
		 * starting position of that line in linepos.
		 */
		linepos = pos;
		pos = wforw_raw_line(pos);
		if (linenum != 0)
			linenum++;

		if (pos == NULL_POSITION)
		{
			/*
			 * We hit EOF without a match.
			 */
			error(GETTXT(_MSG_MORE_NO_TAG_FOUND));
			return (1);
		}

		/*
		 * If we're using line numbers, we might as well
		 * remember the information we have now (the position
		 * and line number of the current line).
		 */
		if (linenums)
			add_lnum(linenum, pos);

		/*
		 * Test the line to see if we have a match.
		 */
		if (anchored) {
			if (wcscmp(wLine, wPattern) == 0)
				break;
		} else if (wcswcs(wLine, wPattern))
			break;
	}

	jump_loc(linepos);
	return (0);
}
