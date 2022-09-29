/*
 * strecpy.c
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


/****************************I18N File Header**********************************

      File : strecpy.c                                                          

      Compatibility Options : No support/Improper support/EUC single byte/
                       EUC Multibyte/Big5-Sjis/Full multibyte/Unicode

      Old Compatibility : EUC single byte/EUC Multibyte

      Present Compatibility : EUC single byte/EUC multibyte/ Full multibyte

      Type of Application (Normal/Important/Critical) : Critical

      Optimization Level (EUC & Single byte/ Single byte/ Not optimized) : 
                                                       EUC & Single byte  

      Change History :
			 30 June 1997 : Ajay Pathak   

		Full multibyte support is implemented.

			 28 July 1997 : Ajay Pathak   
		Cahnges as per code review.

**************************End of I18N File Header*****************************/

#ifdef __STDC__
	#pragma weak strecpy = _strecpy
	#pragma weak streadd = _streadd
#endif
#include "synonyms.h"

/*
	strecpy(output, input, except)
	strecpy copys the input string to the output string expanding
	any non-graphic character with the C escape sequence.
	Esacpe sequences produced are those defined in "The C Programming
	Language" pages 180-181.
	Characters in the except string will not be expanded.
	Returns the first argument.

	streadd( output, input, except )
	Identical to strecpy() except returns address of null-byte at end
	of output.  Useful for concatenating strings.
*/

#include	<ctype.h>
#include	<string.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<widec.h>
#include	<i18n_capable.h>

#define RestoreMbstr(wsd, d) { \
n = (int)wcstombs(NULL, wsd, 2); \
n = (int)wcstombs(d, wsd, n); \
if(n <= 0) \
	return NULL; \
d[n] = '\0'; }

char *streadd(char *, const char *, const char *);


char *
strecpy(char *pout, const char *pin, const char *except)
{
	(void)streadd( pout, pin, except );
	return  pout;
}


char *
streadd(register char *pout, register const char *pin, const char *except)
{
	register unsigned	c;

	wchar_t  *wspout, *wstmppout, *wspin, *wstmppin, *wsexcept;
	wchar_t  wc;
	char	 tmpOctalstr[4];	/* since three octal byte has to be stored */
	wchar_t  wtmpOctalstr[4];
	int	 n;


	/* I18NCAT_MB_WCHAR, I18NCAT_STRING_OP, I18NCAT_IO_FMT */
	if(I18N_SBCS_CODE || I18N_EUC_CODE)
	{
		while( c = *pin++ ) {
			if( !isprint(c) && (!except || !strchr(except, (int) c)) ) {
				*pout++ = '\\';
				switch( c ) {
				case '\n':
					*pout++ = 'n';
					continue;
				case '\t':
					*pout++ = 't';
					continue;
				case '\b':
					*pout++ = 'b';
					continue;
				case '\r':
					*pout++ = 'r';
					continue;
				case '\f':
					*pout++ = 'f';
					continue;
				case '\v':
					*pout++ = 'v';
					continue;
				case '\007':
					*pout++ = 'a';
					continue;
				case '\\':
					continue;
				default:
					(void)sprintf( pout, "%.3o", c );
					pout += 3;
					continue;
				}
			}
			if ( c == '\\' && (!except || !strchr(except, (int) c)) )
				*pout++ = '\\';
			*pout++ = (char) c;
		}
		*pout = '\0';
	}
	else{
		wspout = wspin  = NULL;
		if( pout != NULL)
		{
			n = (int)strlen(pout) + 1;
			wspout = (wchar_t*) malloc((sizeof(wchar_t)) * n);
			if(!wspout)
				return NULL;
			memset(wspout,0,sizeof(wchar_t)*n);
			wstmppout = wspout;
		}
		if( pin != NULL)
		{
			n = (int)strlen(pin) + 1;
			wspin = (wchar_t*) malloc((sizeof(wchar_t)) * n);
			if(!wspin)
				return NULL;
			memset(wspin,0,sizeof(wchar_t)*n);

			n = (int)mbstowcs(wspin, pin, n);
			if( n < 0)
			{
				free(wspout);
				free(wspin);
				return NULL;
			}
			wstmppin = wspin;
		}
		if( except != NULL)
		{
			n = (int)strlen(except) + 1;
			wsexcept = (wchar_t*) malloc((sizeof(wchar_t)) * n);
			if(!wsexcept)
				return NULL;

			n = (int)mbstowcs(wsexcept, except, n);
			if( n < 0)
			{
				free(wspout);
				free(wspin);
				free(wsexcept);
				return NULL;
			}
		}
		while( wc = *wspin++ ) {

			if( !iswprint(wc) && (!wsexcept || !wschr(wsexcept, (int) wc)) ) {
				*wspout++ = L'\\';
				switch( wc ) {
				case L'\n':
					*wspout++ = L'n';
					continue;
				case L'\t':
					*wspout++ = L't';
					continue;
				case L'\b':
					*wspout++ = L'b';
					continue;
				case L'\r':
					*wspout++ = L'r';
					continue;
				case L'\f':
					*wspout++ = L'f';
					continue;
				case L'\v':
					*wspout++ = L'v';
					continue;
				case L'\007':
					*wspout++ = L'a';
					continue;
				case L'\\':
					continue;
				default:
					n =wctomb(tmpOctalstr, wc);
					if(n ==1)
					{
						(void)sprintf( tmpOctalstr, "%.3o", (int)wc );

						n = (int)mbstowcs(wtmpOctalstr, tmpOctalstr, strlen(tmpOctalstr) + 1);
						if( n < 0)
							return NULL;

						wscat(wspout, wtmpOctalstr);
						wspout += 3;
						continue;
					}
					else { 
						wspout--;
						break; 
					}
				}
			}
			if ( wc == L'\\' && (!wsexcept || !wschr(wsexcept, (int) wc)) )
				*wspout++ = L'\\';
			*wspout++ = (wchar_t) wc;
		}
		*wspout = L'\0';

		RestoreMbstr(wstmppout, pout)
		free(wstmppin);
		free(wstmppout);
		free(wsexcept);
	}
	return  (pout);
}
