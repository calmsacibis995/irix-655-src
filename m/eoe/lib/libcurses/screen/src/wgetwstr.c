/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/wgetwstr.c	1.2"
#include	"curses_inc.h"
#define		LENGTH	256

/* This routine gets a string starting at (_cury, _curx) */

int
wgetwstr(WINDOW *win, wchar_t *str)
{
    return ((wgetnwstr(win, str, LENGTH) == ERR) ? ERR : OK);
}

int
wgetnwstr(register WINDOW *win, wchar_t	*str, register int n)
{
    register	int	cpos = 0, ch;
    register	wchar_t	*cp = str;
    int			i, total = 0;
    char		myerase, mykill;
    char		rownum[LENGTH], colnum[LENGTH], length[LENGTH];
    int			doecho = SP->fl_echoit;
    int			savecb = cur_term->_fl_rawmode;
    int			savsync, savimmed, savleave;

#ifdef	DEBUG
    if (outf)
	fprintf(outf, "doecho %d, savecb %d\n", doecho, savecb);
#endif	/* DEBUG */


    myerase = erasechar();
    mykill = killchar();
    if (!savecb)
	cbreak();

    if (doecho)
    {
	SP->fl_echoit = FALSE;
	savsync = win->_sync;
	savimmed = win->_immed;
	savleave = win->_leave;
	win->_immed = win->_sync = win->_leave = FALSE;
	(void) wrefresh(win);
	if (n > LENGTH)
	    n = LENGTH;
    }
    n--;

    while (cpos < n)
    {
	if (doecho)
	{
	    rownum[cpos] = (char) win->_cury;
	    colnum[cpos] = (char) win->_curx;
	}

	ch = wgetwch(win);

	if ((ch == ERR) || (ch == '\n') || (ch == '\r') || (ch == KEY_ENTER))
	    break;
	if ((ch == myerase) || (ch == KEY_LEFT) || (ch == KEY_BACKSPACE) ||
	    (ch == mykill))
	{
	    if (cpos > 0)
	    {
		if (ch == mykill)
		{
		    i = total;
		    total = cpos = 0;
		    cp = str;
		}
		else
		{
		    cp--;
		    cpos--;
		    if (doecho)
			total -= (i = length[cpos]);
		}
		if (doecho)
		{
		    (void) wmove(win, rownum[cpos], colnum[cpos]);
		    /* Add the correct amount of blanks. */
		    for ( ; i > 0; i--)
			(void) waddch (win, ' ');
		    /* Move back after the blanks are put in. */
		    (void) wmove(win, rownum[cpos], colnum[cpos]);
		    /* Update total. */
		    (void) wrefresh(win);
		}
	    }
	    else
		if (doecho)
		    beep();
	}
	else
	    if ((KEY_MIN <= ch) && (ch <= KEY_MAX))
		beep();
	    else
	    {
		*cp++ = ch;
		if (doecho)
		{
		    /* Add the length of the */
		    /* character to total. */
#ifdef	_WCHAR16
		    if ((ch & H_EUCMASK) != H_P00)
#else	/*_WCHAR16*/
		    if ((ch & EUCMASK) != P00)
#endif	/*_WCHAR16*/
			length[cpos] = (char) wcscrw(ch);
		    else if (ch >= ' ')
			length[cpos] = 1;
		    else
			if (ch == '\t')
			    length[cpos] = (char) (TABSIZE - (colnum[cpos] % TABSIZE));
			else
			    length[cpos] = 2;
		    total += length[cpos];
		    (void) wechowchar(win, (chtype) ch);
		}
		cpos++;
	    }
    }

    *cp = '\0';

    if (!savecb)
	nocbreak();
    /*
     * The following code is equivalent to waddch(win, '\n')
     * except that it does not do a wclrtoeol.
     */
    if (doecho)
    {
	SP->fl_echoit = TRUE;
	win->_curx = 0;
	if (win->_cury + 1 > win->_bmarg)
	    (void) wscrl(win, 1);
	else
	    win->_cury++;

	win->_sync = (char) savsync;
	win->_immed = (char) savimmed;
	win->_leave = (char) savleave;
	(void) wrefresh(win);
    }

    return (ch);
}
