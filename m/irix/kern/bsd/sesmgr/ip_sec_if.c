#ident "$Revision: 1.9 $"

#include "sys/param.h"
#include "sys/debug.h"
#include "sys/mbuf.h"
#include "sys/errno.h"
#include "sys/mac_label.h"
#include "sys/sesmgr.h"

#include <net/if.h>
#include "netinet/in.h"
#include "netinet/in_systm.h"	/* needed for ip.h" */
#include "netinet/ip.h"
#include "netinet/ip_var.h"	/* needed for MAX_IPOPTLEN & struct ipstat */
#include "netinet/ip_secopts.h"

/*
 * returns 1 if label is ok for an interface, 0 otherwise 
 */
STATIC_PRF
int
if_label_ok( mac_label *lbl )
{
	switch ( lbl->ml_msen_type ) {
	case MSEN_ADMIN_LABEL:
	case MSEN_HIGH_LABEL:
	case MSEN_LOW_LABEL:
	case MSEN_MLD_HIGH_LABEL:
	case MSEN_MLD_LABEL:
	case MSEN_MLD_LOW_LABEL:
	case MSEN_TCSEC_LABEL:
		break;

	case MSEN_EQUAL_LABEL:
	case MSEN_UNKNOWN_LABEL:
	default:
		return 0;
	}

	switch ( lbl->ml_mint_type ) {
	case MINT_BIBA_LABEL:
	case MINT_HIGH_LABEL:
	case MINT_LOW_LABEL:
		break;

	case MINT_EQUAL_LABEL:
	default:
		return 0;
	}
	return 1;
}

#ifdef NOTYET
/*
 *  check interface security configuration for validity.
 *  Returns 0 if OK, non-zero if invalid configuration.
 */
int
if_security_invalid( register ifsec_t *ifs )
{
	int result = 0;

    	ASSERT(sesmgr_enabled);
	if (! if_label_ok( ifs->ifs_label_max )
	||  ! if_label_ok( ifs->ifs_label_min )
	||  ! mac_dom( ifs->ifs_label_max, ifs->ifs_label_min ) )
		result = 1;
	else switch ( ifs->ifs_idiom ) {

	case IDIOM_MONO:
		result = ! mac_equ( ifs->ifs_label_max, ifs->ifs_label_min );
		break;

	case IDIOM_BSO_TX:
	case IDIOM_BSO_RX:
	case IDIOM_BSO_REQ:
		/* Disallow multi-byte authority values, which have LSB==1.
		 * Require maximum authority to dominate minimum authority.
		 * Require min and max labels to have same categories.
		 */
		result = (ifs->ifs_authority_min | ifs->ifs_authority_max) & 1
		      || (ifs->ifs_authority_min & ~ifs->ifs_authority_max)
		      || !mac_mint_equ( ifs->ifs_label_max, ifs->ifs_label_min)
		      || !mac_cat_equ( ifs->ifs_label_max, ifs->ifs_label_min);
		break;

	case IDIOM_CIPSO2:
	case IDIOM_TT1_CIPSO2:
	case IDIOM_CIPSO:
	case IDIOM_TT1:
		result = ( ifs->ifs_doi == 0 )
		      || !mac_mint_equ( ifs->ifs_label_max, ifs->ifs_label_min);
		break;

	case IDIOM_SGIPSO2:
	case IDIOM_SGIPSO2_NO_UID:
	case IDIOM_SGIPSOD:
	case IDIOM_SGIPSO:
		result = ( ifs->ifs_doi == 0 );
		break;

	default:
		result = 1;
		break;
	}
	return result;

}
#endif

/* sort an array of short integers into ascending order.  
 * Based on Algorithm 201, ACM by J. Boothroyd (1963).
 * Also known as a "shell" sort.
 * Chosen for compactness more than speed.
 */
void
sort_shorts( u_short *shorts, int count )
{
	register u_short * p1;
	register u_short * p2;
	register int       middle;
	u_short *          start;
	u_short *          limit;

	middle = count;
	for (;;) {
		if ( (middle >>= 1) == 0 )
			return;
		limit = &shorts[ count - middle ];
		for ( start = shorts; start < limit; ++start ) {
			register int tmp1;
			register int tmp2;

			p1 = start;
			p2 = p1 + middle;	
			while ( p1 >= shorts && (tmp1 = *p1) > (tmp2 = *p2) ) {
				*p1 = tmp2; *p2 = tmp1;
				p2 = p1;
				p1 -= middle;
			} 
		}
	}
	/* NOTREACHED */
}

