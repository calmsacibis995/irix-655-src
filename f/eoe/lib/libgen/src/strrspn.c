/*
 * strrspn.c
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

#ident "$Revision: 1.4 $"


/****************************I18N File Header**********************************

      File : strrspn.c                                                          

      Compatibility Options : No support/Improper support/EUC single byte/
                       EUC Multibyte/Big5-Sjis/Full multibyte/Unicode

      Old Compatibility : EUC single byte/EUC Multibyte

      Present Compatibility : EUC single byte/EUC multibyte/ Full multibyte

      Type of Application (Normal/Important/Critical) : Critical

      Optimization Level (EUC & Single byte/ Single byte/ Not optimized) : 
                                                       EUC & Single byte  

      Change History :
			 30/6/97 : Ajay Pathak   

		Full multibyte support is implemented.

			 28 July 1997 : Ajay Pathak   
		Cahnges as per code review.

**************************End of I18N File Header*****************************/

#ifdef __STDC__
	#pragma weak strrspn = _strrspn
#endif
#include "synonyms.h"

/*
	Trim trailing characters from a string.
	Returns pointer to the first character in the string
	to be trimmed (tc).
*/

#include	<string.h>

#include	<widec.h>
#include	<i18n_capable.h>

static wchar_t* WsAlloc(const char *);

char *
strrspn(const char *string, const char *tc)	/* tc: characters to trim */
{
	char	*p;

	wchar_t	*wp, *wstring, *wtc;
	int wplen;


	p = (char *)string + strlen( string );

	/* I18NCAT_MB_WCHAR, I18NCAT_STRING_OP */
	if(I18N_SBCS_CODE || I18N_EUC_CODE)
	{
		while( p != (char *)string )
			if( !strchr( tc, *--p ) )
				return  ++p;

		return  p;
	}
	else{
		wstring = WsAlloc(string);
		if(wstring == NULL)
			return NULL;

		wtc = WsAlloc(tc);
		if(wtc == NULL){
			free(wstring);
			return NULL;
		}
	
		wp = (wchar_t *)wstring + wslen(wstring);

		while( wp != (wchar_t *)wstring)
			if( !wschr(wtc, *--wp)) {
				wp++;
				wplen = (int)wcstombs(NULL, wp, MB_CUR_MAX);
				if( wplen < 0)
					return NULL;
				p -= wplen;
				free(wstring);
				free(wtc);
				return p;
			}	
			wplen = (int)wcstombs(NULL, wp, MB_CUR_MAX);
			if( wplen < 0)
				return NULL;
			p -= wplen;
			free(wstring);
			free(wtc);
			return p;
	}
}

     /**************************I18N Function Header**************************

      Purpose :	This function is added to allocte memory for wchar buffer and 
				convert a mbstring to wchar string and copy it into allocated 
				buffer.

      Parameters :	str -mbstring

      Return Values :	pointer to wchar buffer if success, or NULL if error 


     **********************End of I18N Function Header***********************/

static wchar_t* WsAlloc(const char *str)
{
	wchar_t		*wsstr;
	int		n;

	n = (int)strlen(str) + 1;
	wsstr = (wchar_t*)malloc((sizeof(wchar_t)) * n);
	if(!wsstr)
		return NULL;
	n = (int)mbstowcs(wsstr, str, n);
	if( n < 0)
	{
		free(wsstr);
		return NULL;
	}
	return (wchar_t *)wsstr;
}
