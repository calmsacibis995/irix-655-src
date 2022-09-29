/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:shlib/adjust.c	1.2.4.1"

/*
 *   ADJUST.C
 *
 *   Programmer:  D. A. Lambeth
 *
 *        Owner:  D. A. Lambeth
 *
 *         Date:  April 17, 1980
 *
 *
 *
 *   CHATTRIB (NODE, NEWATTS)
 *
 *        Give NODE the attribute(s) NEWATTS, and change its
 *        value to conform to the new attributes.
 *
 *
 *
 *   See Also:  TYPESET(I)
 */


#include     "sh_config.h"
#include     "name.h"

extern void free();
#ifdef apollo
    extern void	ev_$delete_var();
    extern void	ev_$set_var();
#endif /* apollo */


/*
 *   NAM_NEWYTPE (NODE, NEWATTS)
 *
 *        struct namnod *NODE;
 *
 *        int NEWATTS;
 *
 *	  int size;
 *
 *   Give NODE the attributes NEWATTS, and change its current
 *   value to conform to NEWATTS.  The SIZE of left and right
 *   justified fields may be given.
 */

void nam_newtype (node, newatts, size)
struct namnod *node;
unsigned int newatts;
{
	register wchar_t *sp;
	register wchar_t *cp = 0L;
	register struct namnod *np = node;
	register unsigned int n;
	struct Namaray *ap = 0;
	int oldsize,oldatts,savedot;

#ifdef NAME_SCOPE
	if(np->value.namflg&N_CWRITE)
		np = nam_copy(np,1);
#endif /* NAME_SCOPE */
	/* handle attributes that do not change data separately */
	n = np->value.namflg;
#ifdef apollo
	if(((n^newatts)&N_EXPORT))
	/* record changes to the environment */
	{
		short namlen = wcslen(np->namid);
		if(n&N_EXPORT)
			ev_$delete_var(np->namid,&namlen);
		else
		{
			wchar_t *vp = nam_strval(np);
			short vallen = wcslen(vp);
			ev_$set_var(np->namid,&namlen,vp,&vallen);
		}
	}
#endif /* apollo */
	if((size==0||(n&N_INTGER)) && ((n^newatts)&~NO_CHANGE)==0)
	{
		if(size)
			np->value.namsz = size;
		np->value.namflg = newatts;
		return;
	}
	/* for an array, change all the elements */
	if(nam_istype(np,N_ARRAY))
	{
		ap = array_ptr(np);
		savedot = ap->cur[0];
		ap->cur[0] = 0;
		oldsize = np->value.namsz;
		oldatts = np->value.namflg;
	}
again:
	if (sp = nam_strval (np))
 	{
		cp = (wchar_t *)malloc (sizeof(wchar_t)*((n=wcslen(sp))+1));
		wcscpy (cp, sp);
		if(ap)	/* make sure array won't be deleted, just element */
			ap->nelem++;
		nam_free(np);
		if(ap)
			ap->nelem--;
		if(size==0 && (newatts&(N_LJUST|N_RJUST|N_ZFILL)))
			size = n;
	}
	else
		nam_free(np);
	np->value.namsz = size;
	np->value.namflg &= ~(N_LJUST|N_RJUST|N_ZFILL|N_LTOU|N_UTOL|N_HOST|N_EXPORT|N_RDONLY);
	np->value.namflg |= newatts;
	if (cp != 0L)
	{
		nam_fputval (np, cp);
		free(cp);
	}
	if(ap)
	{
		while((n= ++ap->cur[0]) < ap->maxi)
		{
			if(ap->val[n])
			{
				np->value.namsz = oldsize;
				np->value.namflg = oldatts;
				goto again;
			}
		}
		ap->cur[0] = savedot;
	}
	return;
}
