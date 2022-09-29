/*
 * bufsplit.c
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

      File : bufsplit.c                                                          

      Compatibility Options : No support/Improper support/EUC single byte/
                       EUC Multibyte/Big5-Sjis/Full multibyte/Unicode

      Old Compatibility : EUC single byte/EUC Multibyte

      Present Compatibility : EUC single byte/EUC multibyte/ Full multibyte

      Type of Application (Normal/Important/Critical) : Critical

      Optimization Level (EUC & Single byte/ Single byte/ Not optimized) : 
                                                       EUC & Single byte  

      Change History :
			 23 June 1997 : Ajay Pathak   

		Full multibyte support is implemented.

			 28 July 1997 : Ajay Pathak   
		Cahnges as per code review

     **********************End of I18N File Header*****************************/



/*
	split buffer into fields delimited by tabs and newlines.
	Fill pointer array with pointers to fields.
	Return the number of fields assigned to the array[].
	The remainder of the array elements point to the end of the buffer.
  Note:
	The delimiters are changed to null-bytes in the buffer and array of
	pointers is only valid while the buffer is intact.
*/

#ifdef __STDC__
	#pragma weak bufsplit = _bufsplit
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <string.h>
#include	<widec.h>
#include	<i18n_capable.h>

static wchar_t* WsAlloc(const char *);

static char	*bsplitchar = "\t\n";	/* characters that separate fields */

size_t
bufsplit(register char *buf, size_t dim, char *array[])
  /* register char	*buf;		-- input buffer */
  /* size_t		dim;		-- dimension of the array */
{
	register size_t numsplit;
	register size_t i;
	wchar_t		*wsbuf, *wstmpbuf;
	wchar_t		*wsbsplitchar;
	wchar_t		**wsarray, **wstmparray;
	int		n;

	if( !buf )
		return 0;
	if ( !dim ^ !array)
		return 0;
	if( buf  &&  !dim  &&  !array ) {
		bsplitchar = buf;
		return 1;
	}
	numsplit = 0;

	/* I18NCAT_MB_WCHAR , I18NCAT_STRING_OP, I18NCAT_CHAR_ARITH, I18NCAT_RELOP*/
	if(I18N_SBCS_CODE || I18N_EUC_CODE)
	{

		while ( numsplit < dim ) {
			array[numsplit] = buf;
			numsplit++;
			buf = strpbrk(buf, bsplitchar);
			if (buf)
				*(buf++) = '\0';
			else
				break;
			if (*buf == '\0') {
				break;
			}
		}
		buf = strrchr( array[numsplit-1], '\0' );
		for (i=numsplit; i < dim; i++)
			array[i] = buf;

	}
	else{	
		wsbuf = WsAlloc((const char *)buf);
		if(wsbuf == NULL)
			return -1;
		wstmpbuf = wsbuf;
		
		wsbsplitchar = WsAlloc((const char *)bsplitchar);
		if(wsbsplitchar == NULL)
		{
			free(wstmpbuf);
			return -1;
		}

		wsarray = (wchar_t **)malloc((sizeof(wchar_t)) * dim);
		if(wsarray == NULL)
		{
			free(wstmpbuf);
			free(wsbsplitchar);
			return -1;
		}
		wstmparray = wsarray;

		while ( numsplit < dim ) {
			wsarray[numsplit] = wsbuf;
			numsplit++;

			wsbuf = wspbrk(wsbuf, wsbsplitchar);
			if (wsbuf)
			{		
				*(wsbuf++) = L'\0';
			}
			else
				break;

			if (*wsbuf == L'\0') {
				break;
			}
		}

		wsbuf = wcsrchr( wsarray[numsplit-1], L'\0' );

		for (i=numsplit; i < dim; i++)
		{
			wsarray[i] = wsbuf;
		}
		for(i =0; i < dim; i++)
		{

		
			n = (int)wcstombs(NULL, wsarray[i], MB_CUR_MAX);
			if ( n < 0)
				break; 
			array[i] = (char *)malloc(n+1);

			n = (int)wcstombs(array[i], wsarray[i], n+1);
			if ( n < 0)
				break; 
		}
		free(wstmpbuf);
		free(wstmparray);
		free(wsbsplitchar);
	}

	return numsplit;
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
