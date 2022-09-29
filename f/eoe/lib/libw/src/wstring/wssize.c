/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libw:port/wstring/wssize.c	1.1.1.2"

/***********************I18N File Header******************************
File                  : wssize.c

Compatibility Options : No support/Improper support/ EUC single byte/
                        EUC multibyte/Sjis-Big5/Full multibyte/Unicode

Old Compatibility     : EUC single byte/EUC multibyte

Present Compatibility : EUC single byte/EUC multibyte/Sjis

Type of Application (Normal/Important/Critical) : Critical

Optimization Level (EUC & Single byte/Single byte/Not optimized)
                      : EUC & Single byte

Change History        : Sept 19, 1997          HCL
                        Support for SJIS set has been added. Currently
	MASK for SJIS is not known, So Values are hard coded.

************************End of I18N File Header**********************/

/*
 * Wssize returns number of bytes in EUC for wchar_t string.
 */

#include <widec.h>
#include <sys/euc.h>
#include <locale.h>
#include <i18n_capable.h>
#include "pcode.h"

#ifdef _WCHAR16
# define MY_EUCMASK	H_EUCMASK
# define MY_P11		H_P11
# define MY_P01		H_P01
# define MY_P10		H_P10
# define MY_SHIFT	8
#else
# define MY_EUCMASK	EUCMASK
# define MY_P11		P11
# define MY_P01		P01
# define MY_P10		P10
# define MY_SHIFT	7
#endif
#define	SJIS_LOCALE	"ja_JP.SJIS"

int
_wssize(register wchar_t *s)
{
	register wchar_t wchar;
	register int size;
	char	*pLocale;

	 /* I18NCAT_STRING_OP, I18NCAT_OTHER */
	if(I18N_SBCS_CODE || I18N_EUC_CODE)
	{
		for (size = 0;;)	/* don't check for bad sequences */
		{
			switch ((wchar = *s++) & MY_EUCMASK)
			{
			default:
				if (wchar == 0)
					return size;
				size++;
				continue;
			case MY_P11:
				size += eucw1;
				continue;
			case MY_P01:
				size++;
#ifdef _WCHAR16
				if (wchar < 0240 || !multibyte && wchar < 0400)
					break;
#endif
				size += eucw2;
				continue;
			case MY_P10:
				size++;
				size += eucw3;
				continue;
			}
		}
	}
	else{
/*
 * REVISIT
 * Date:24 Sep 1997 
 * Since MASK for SJIS is not known, we are checking for locale. Once, we
 * get the MASK this block of code should be replaced by SJIS and BIG5 MASK
 */
		pLocale = setlocale(LC_ALL, NULL);

		if(!strcmp(pLocale, SJIS_LOCALE))
			return ((int)wcstombs(NULL, s, 3));

		for (size = 0;;)	/* don't check for bad sequences */
		{
			switch ((wchar = *s++) & MY_EUCMASK)
			{
			default:
				if (wchar == 0)
					return size;
				size++;
				continue;
			case MY_P11:
				size += eucw1;
				continue;
			case MY_P01:
				size++;
#ifdef _WCHAR16
				if (wchar < 0240 || !multibyte && wchar < 0400)
					break;
#endif
				size += eucw2;
				continue;
			case MY_P10:
				size++;
				size += eucw3;
				continue;
			}
		}
	}
}
