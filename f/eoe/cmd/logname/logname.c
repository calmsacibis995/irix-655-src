/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)logname:logname.c	1.4"	*/
#ident	"$Header: /proj/irix6.5f/isms/eoe/cmd/logname/RCS/logname.c,v 1.5 1998/09/18 19:47:22 sherwood Exp $"
/****************************I18N File Header*********************************
 * File                  : logname.c
 * Compatibility Options : No support/Improper support/EUC single byte/
 *                         EUC Multibyte/Big5-Sjis/Full multibyte/Unicode
 * Old Compatibility     : Full multibyte
 * Present Compatibility : Full multibyte
 * Type of Application (Normal/Important/Critical) : Normal
 * Optimization Level    :Not Applicable
 * Change History        :
 * 	27th January 98   HCL
 *	-	No I18N change required
 **************************End of I18N File Header****************************/

#include <stdio.h>
main() {
	char *name;

	name = cuserid((char *)NULL);
	if (name == NULL)
		return (1);
	(void) puts (name);
	return (0);
}
