/*
 * strtrns.c
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

      File : strtrns.c                                                          

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

**************************End of I18N File Header*****************************/

#ifdef __STDC__
	#pragma weak strtrns = _strtrns
#endif
#include "synonyms.h"

#include	<widec.h>
#include	<i18n_capable.h>

static wchar_t* WsAlloc(const char *);

#define RestoreMbstr(wsd, d) { \
n = (int)wcstombs(NULL, wsd, MB_CUR_MAX); \
n = (int)wcstombs(d, wsd, n); \
if(n <= 0) \
	return NULL; \
d[n] = '\0'; }

/*
	Copy `str' to `result' replacing any character found
	in both `str' and `old' with the corresponding character from `new'.
	Return `result'.
*/

char *
strtrns(register const char *str, const char *old,
	const char *new, char *result)
{
	register char *r;
	register const char *o;

	wchar_t  *wsstr, *wstmpstr, *wsold, *wsnew, *wsresult;
	register wchar_t	*wsr;
	register wchar_t	*wso;
	int	 n;

	/* I18NCAT_MB_WCHAR, I18NCAT_RELOP */
	if(I18N_SBCS_CODE || I18N_EUC_CODE)
	{
		for (r = result; *r = *str++; r++)
			for (o = old; *o; )
				if (*r == *o++) {
					*r = new[o - old -1];
					break;
				}
		return(result);
	}
	else{
		wsstr = WsAlloc(str);
		if(!wsstr)
			return NULL;
		wstmpstr = wsstr;

		wsold = WsAlloc(old);
		if(!wsold)
		{
			free(wsstr);
			return NULL;
		}

		wsnew = WsAlloc(new);
		if(!wsnew)
		{
			free(wsstr);
			free(wsold);
			return NULL;
		}

		wsresult = WsAlloc((const char* )result);
		if(!wsnew)
		{
			free(wsstr);
			free(wsold);
			free(wsnew);
			return NULL;
		}

		for (wsr = wsresult; *wsr = *wsstr++; wsr++)
			for (wso = wsold; *wso; )
				if (*wsr == *wso++) {
					*wsr = wsnew[wso - wsold -1];
					break;
				}
		RestoreMbstr(wsresult, result)

		free(wstmpstr);
		free(wsold);
		free(wsnew);
		free(wsresult);
		return(result);
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
