/*
 * bgets.c
 *
 *
 * Copyright 1991, Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
 * the contents of this file may not be disclosed to third parties, copied or
 * duplicated in any form, in whole or in part, without the prior written
 * permission of Silicon Graphics, Inc.
 *
 * RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software clause at DFARS 252.227-7013, and/or in similar or
 * successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
 * rights reserved under the Copyright Laws of the United States.
 */

#ident "$Revision: 1.5 $"


     /************************I18N File Header**********************************

      File : bgets.c                                                          

      Compatibility Options : No support/Improper support/EUC single byte/
                       EUC Multibyte/Big5-Sjis/Full multibyte/Unicode

      Old Compatibility : EUC single byte/EUC Multibyte

      Present Compatibility : EUC single byte/EUC multibyte/ Full multibyte

      Type of Application (Normal/Important/Critical) : Critical

      Optimization Level (EUC & Single byte/ Single byte/ Not optimized) : 
                                                       Single byte  

      Change History :
			 26 June 1997 : Ajay Pathak   

		Full multibyte support is implemented.

			 28 July 1997 : Ajay Pathak   
		Cahnges as per code review

     **********************End of I18N File Header*****************************/



/*
	read no more than <count> characters into <buf> from stream <fp>,
	stoping at any character slisted in <stopstr>.
	NOTE: This function will not work for multi-byte characters.
*/
#ifdef __STDC__
	#pragma weak bgets = _bgets
#endif
#include "synonyms.h"

#include <sys/types.h>
#include <stdio.h>
#include	<widec.h>
#include	<i18n_capable.h>

#define CHARS	256

static char	stop[CHARS];

static wchar_t	wStop[CHARS];

static int	first_bgets = 1;

char *
bgets(char *buf, register size_t count, FILE *fp, char *stopstr)
{
	register char	*cp;
	register int	c;
	register size_t i;

	wchar_t		wC;
	register int	nStopsz;
	int		n;
	int		j;
	int		nBreakflg = 0;


	/* clear and set stopstr array */
	if ((stopstr) || (first_bgets)) {
		for( cp = stop;  cp < &stop[CHARS]; )
			*cp++ = 0;
		first_bgets = 0;
	}

	/* I18NCAT_MB_WCHAR , I18NCAT_RELOP , I18NCAT_IO_UNFMT */
	if(I18N_SBCS_CODE)
	{
		if (stopstr)
			for( cp = stopstr;  *cp; )
				stop[(unsigned char)*cp++] = 1;
		i = 0;
		for( cp = buf;  ; ) {
			if(i++ == count) {
				*cp = '\0';
				break;
			}
			if( (c = getc(fp)) == EOF ) {
				*cp = '\0';
				if( cp == buf )
					cp = (char *) 0;
				break;
			}
			*cp++ = (char) c;
			if( stop[ c ] ) {
				*cp = '\0';
				break;
			}
		}

	}
	else{

		if (stopstr)
			n = (int)mbstowcs(wStop, stopstr, strlen(stopstr) + 1);
			if( n < 0)
				return NULL;
			nStopsz = (int)wslen(wStop);

		i = 0;
		for( cp = buf; !nBreakflg ; ) {


			if(i++ == count) {
				*cp = '\0';
				break;
			}

			if( (wC = getwc(fp)) == WEOF ) {
				*cp = '\0';

				if( cp == buf )
					cp = (char *) 0;
				break;
			}

			n = wctomb(cp, wC);
			if( n < 0)
				return NULL;

			cp += n ;
			for(j = 0; j < nStopsz; j++)
			{
				if(wStop[j] == wC ) {
					*cp = '\0';
					nBreakflg = 1;
					break;
				}
			}
		}
	}
	return  cp;
}
