#ident "$Revision: 1.11 $"

#include <bstring.h>
#include <sys/cipso.h>
#include "sys/param.h"
#include "sys/debug.h"
#include "sys/mbuf.h"
#include "sys/errno.h"
#include "sys/mac_label.h"

#include "net/if.h"
#include "in.h"
#include "in_systm.h"		/* needed for ip.h" */
#include "ip.h"
#include "ip_var.h"		/* needed for MAX_IPOPTLEN */
#include "sys/sat.h"

#include "ip_secopts.h"

#define NONE_VALID	0
#define BSO_VALID	1
#define CIPSO_VALID	2
#define ESO_VALID	BSO_VALID

/*
 * This table relates the valid security options to the interface idiom.
 */
STATIC_PRF
u_char if_policy[ IDIOM_MAX + 1 ] = {
	NONE_VALID, 		/* IDIOM_MONO		*/
	BSO_VALID, 		/* IDIOM_BSO_TX		*/
	BSO_VALID, 		/* IDIOM_BSO_RX		*/
	BSO_VALID,		/* IDIOM_BSO_REQ	*/
	CIPSO_VALID,		/* IDIOM_CIPSO		*/
	CIPSO_VALID,		/* IDIOM_SGIPSO		*/
	CIPSO_VALID,		/* IDIOM_TT1		*/
	CIPSO_VALID,		/* IDIOM_CIPSO2	       	*/
	CIPSO_VALID,		/* IDIOM_SGIPSO2       	*/	
	CIPSO_VALID,		/* IDIOM_SGIPSOD       	*/
	CIPSO_VALID,		/* IDIOM_SGIPSO2_NO_UID	*/	
	CIPSO_VALID,		/* IDIOM_TT1_CIPSO2	*/
};

/* values used in the following table:
 *	0	illegal
 *	1	Reserved 1	(see RFC 1108)
 *	2	Unclassified	
 *	3	Reserved 2
 *	4	Reserved 3
 *	5	Confidential
 *	6	Secret
 *	7	Top Secret
 *	8	Reserved 4
 */
STATIC_PRF
u_char ripso_lvl_valid[256] = {
	0, 8, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 7, 0, 0,

	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 6, 0,   0, 0, 0, 0,
	0, 0, 0, 0,   0, 0, 4, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,

	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	0, 0, 0, 0,   0, 0, 5, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 2,   0, 0, 0, 0,
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,

	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   3, 0, 0, 0,
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	0, 1, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0
};

/* This table is the one that will be replaced with customer-selected
 * values.  It has 8 values, corresponding to the 8 legal levels (1-8)
 * defined in the previous table.  These are the default values that
 * the customer gets unless he choses to replace them.
 */
STATIC_PRF
u_char level_rtoi_tab[8] = {
	0, 1<<5, 2<<5, 3<<5, 4<<5, 5<<5, 6<<5, 255	/* default values */
};

/*
 * search interface's label cache for match on external representation.
 * NOTE: must be called at splnet or eqivalent.
 */
STATIC_PRF
struct mbuf *
if_lbl_string_search( 
	struct ifnet *  ifp,		/* interface with cache 	*/
	u_char *        cp,		/* ptr to option string		*/
	int             optlen)		/* length of entire option	*/
{
	struct mbuf *   m;

	/* search cache of cipso options */
	for ( m = ifp->if_sec->ifs_lbl_cache; m; m = m->m_next ) {
		if ( optlen == m->m_len
		&& ! bcmp( cp, mtod( m, caddr_t), optlen)) {
			/* found matching RIPSO/CIPSO label in cache */
			break;
		}
	}
	return m;
}

/* The BSO and ESO options are *not* mutually exclusive.  The label produced
 * by processing a BSO option may be modified by an ESO option, and vice-versa.
 * The BSO code below will need to be modified when ESO code is added.
 * Returns 1 if caller should place label into cache, zero otherwise.
 * NOTE: must be called at splnet or eqivalent.
 */
STATIC_PRF
mac_label *
ip_rcv_bso( 
	register u_char *    cp,
	struct ifnet *       ifp,
	mac_label *          prev_lbl)
{
	register mac_label * ripso_lbl;
	struct mbuf *        m;
	label_cache_t *      entry;
	int                  optlen;	/* length of entire option	*/
	int                  level;	/* internal sensitivity level 	*/
	int                  auth;
	ssize_t		     lbl_size;

	optlen = cp[IPOPT_OLEN];
	if ( optlen != BSO_MIN_LEN ) {
		return (mac_label *)0;	/* error */
	}
	auth  = cp[ RIPSOPT_AUTH ];
	level = ripso_lvl_valid[ cp[ RIPSOPT_LVL ]];

	if ( ifp->if_sec->ifs_authority_min & ~auth 
	||   auth & ~ifp->if_sec->ifs_authority_max 
	||   level == 0 ) {
		return (mac_label *)0;	/* error */
	}
	m = if_lbl_string_search( ifp, cp, optlen );
	if ( m != 0 ) {
		return ((label_cache_t *)m->m_dat)->lc_int;
	} 

	lbl_size  = sizeof( *ripso_lbl );
	ripso_lbl = (mac_label *)kern_malloc( lbl_size );
	ASSERT( ripso_lbl );
	if ( ! prev_lbl ) 
		prev_lbl = ifp->if_sec->ifs_label_max;
	lbl_size  = mac_size( prev_lbl );
	bcopy( prev_lbl, ripso_lbl, lbl_size );

	ripso_lbl->ml_msen_type = MSEN_TCSEC_LABEL;
	ripso_lbl->ml_level     = level_rtoi( level - 1 );
	if ( mac_invalid( ripso_lbl ) )  {
		kern_free( ripso_lbl );
		return (mac_label *)0;	/* no cookie */
	}

	ripso_lbl               = mac_add_label( ripso_lbl );

	/* construct a new cache entry and add entry to cache */
	if ((m = m_get(M_DONTWAIT, MT_SOOPTS)) != 0) {
		entry = (label_cache_t *)m->m_dat;
		entry->lc_int = ripso_lbl;
		m->m_off += sizeof( entry->lc_int );
		m->m_len = optlen;
		bcopy( cp, entry->lc_ext, optlen );

		m->m_next         = ifp->if_sec->ifs_lbl_cache;
		ifp->if_sec->ifs_lbl_cache = m;
	}

	return ripso_lbl;
}

/*ARGSUSED*/
STATIC_PRF
mac_label *
ip_rcv_eso( 
	u_char *             cp,
	struct ifnet *       ifp,
	mac_label *          prev_lbl)
{
	register mac_label * ripso_lbl = 0;

	/* <<< Flesh this in later, when eso is fully specified >>> */
	return ripso_lbl;
}

/* The following table is used to map DOI-defined tag-type numbers into
 * SGI-defined tag type numbers.  The index is DOI-tag-type minus 128.
 * The cell value is used to determine how to interpret the tag.
 * The values in this table may be re-defined by the system administrator. 
 * The value "0" means unknown tag type.
 */
STATIC_PRF
u_char internal_tag_type[128] = {
	SGIPSO_BITMAP, SGIPSO_ENUM, SGIPSO_SPECIAL, SGIPSO_DAC,
	                0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

/*
 * Parse the security options in a received CIPSO datagram.
 * Return the internal mac_label that matches, or 0 if none or invalid.
 * Expects either APR 91 (CIPSO) or JAN 92 (CIPSO2) RFC format,
 * depending on ifp->idiom.
 * NOTE: must be called at splnet or eqivalent.
 */
STATIC_PRF
mac_label *
ip_rcv_cipso( 
	register u_char *    cp,
	struct ifnet *       ifp,
	mac_label *          cipso_lbl)
{
	register u_char *    tp;	/* tag pointer */
	register u_short *   up;	/* for filling cats into label */
	struct mbuf *        m;
	label_cache_t *      entry;
	u_int                doi;	/* value from option		*/
	int                  cnt;	
	int                  idiom;	/* interface idiom              */
	int                  lbl_len;	/* allocation length of mac_label */
	int                  optlen;	/* length of entire option	*/
	int                  tag;	/* tag type value		*/
	int                  taglen;	/* length of tag		*/
	int                  cipstag_lvl;    /* CIPSO or CIPSO2 format */
	int                  cipstag_bitmap; /* CIPSO or CIPSO2 format */
	u_short              enums[ CIPSO_MAX_ENUM ]; /* aligned array  */

	if ( cipso_lbl )
		return (mac_label *)0;	/* don't do this twice */
	optlen = cp[IPOPT_OLEN];
	if ( optlen < CIPSO_MIN_LEN || optlen > CIPSO_MAX_LEN )
		return (mac_label *)0;	/* no cookie */

	idiom = ifp->if_sec->ifs_idiom;
	if ( dacful(idiom) ) {
	    /*
	     * Subtract DAC tag length from option length
	     * so lbl_string_search() and the rest of this
	     * function never see the DAC tag.
	     */
	    optlen -= SGIPSO_DAC_LEN;
	}

	if ( ((long)(&cp[CIPSOPT_DOI]) & 3) == 0 ) {
		doi = *(u_int *)(&cp[CIPSOPT_DOI]);
	} else {
		bcopy( &cp[CIPSOPT_DOI], &doi, sizeof doi );
	}
	doi = ntohl(doi);
	if ( doi == 0 || doi != ifp->if_sec->ifs_doi )
		return (mac_label *)0;	/* no cookie */

	m = if_lbl_string_search( ifp, cp, optlen );
	if ( m != 0 ) {
		entry = (label_cache_t *)m->m_dat;
		return entry->lc_int;
	} 

	/* no match, construct new cache entry */
	if ((m = m_get(M_DONTWAIT, MT_SOOPTS)) == 0)
		return (mac_label *)0;	/* no cookie */
	entry = (label_cache_t *)m->m_dat;
	entry->lc_int = 0;
	m->m_off += sizeof( entry->lc_int );
	m->m_len = optlen;
	bcopy( cp, entry->lc_ext, optlen );

	/* allocate new internal label */
	lbl_len = sizeof( mac_label );
	cipso_lbl = (mac_label *)kern_malloc( lbl_len );
	ASSERT( cipso_lbl );
	bzero( cipso_lbl, lbl_len );

	/* 
	 * Adjust expected offsets for either CIPSO2 or 
	 * CIPSO tag format, depending on idiom. 
	 */
	if ( cipso2(idiom)) {
	    cipstag_lvl = CIPS2TAG_LVL;
	    cipstag_bitmap = CIPS2TAG_BITMAP;
	} else {
	    cipstag_lvl = CIPSTAG_LVL;
	    cipstag_bitmap = CIPSTAG_BITMAP;
	}
	/* fill in new label */
	tp = &cp[CIPSOPT_TAG];
	up = cipso_lbl->ml_list;
	for ( cnt = optlen - CIPSOPT_TAG; cnt > 0; cnt-=taglen, tp+=taglen ) {
		tag    = tp[ CIPSTAG_VAL ];
		taglen = tp[ CIPSTAG_LEN ];
		if ( taglen > cnt )
			goto bad;
		if ( tag < 128 ) {	/* CIPSO WG-defined tag type */
		    if ( cipso2(idiom) && (tp[ CIPS2TAG_ALIGN ] != 0) )
			goto bad;
		    if ( cipso_lbl->ml_divcount )
			goto bad;	/* can't add divs after cats */
		    switch ( tag ) {
			
		    case 1: {	/* CIPSO TAG TYPE 1 */
			register u_char * bp;
			register int      bits;
			register int      bit;
			int               bytes;
			int               catno = 0;

			if  (taglen < cipstag_bitmap )
			    goto bad;
			cipso_lbl->ml_msen_type = MSEN_TCSEC_LABEL;
			cipso_lbl->ml_level     = level_xtoi( tp[cipstag_lvl], 
							      doi);
			bp    = &tp[ cipstag_bitmap ];
			bytes = taglen - cipstag_bitmap;
			for ( ++bytes; --bytes; catno += 8 )  {
				bits = *bp++;
				for ( bit = 0; bits; ++bit ) {
					if ( bits & 0x80 ) {
						*up++ = category_xtoi( 
							catno + bit, doi );
					}
					bits = bits << 1 & 0xff;
				}
			}
			cipso_lbl->ml_catcount += up - cipso_lbl->ml_list;
			break;
		    }

		    case 2: {	/* CIPSO TAG TYPE 2 */
			register u_short * bp;
			register int       cats;

			if ( taglen & 1 
			|| taglen < CIPSTAG_ENUMS )
				goto bad;
			if ( (! cipso2(idiom)) && 
			     ( tp[ CIPSTAG_FLAGS ] & CIPSFLAG_EXCL ) ){
				/* Exclusionary tag type never implemented. */
				goto bad;
			}
			cats = (taglen - CIPSTAG_ENUMS) / sizeof( u_short );
			bp = (u_short *)(&tp[ CIPSTAG_ENUMS ]);
			if ( (long)bp & 1 ) {
				/* copy array of enums to align */
				bcopy( bp, enums, cats * sizeof( u_short ));
				bp = enums;
			}
			cipso_lbl->ml_msen_type = MSEN_TCSEC_LABEL;
			cipso_lbl->ml_level     = level_xtoi( tp[cipstag_lvl], 
							      doi);
			cipso_lbl->ml_catcount += cats;
			for ( ++cats; --cats; )
				*up++ = category_xtoi( ntohs( *bp++ ), doi);
			break;
		    }

		    default:
			    goto bad;
		    }
		} else if (sgipso(idiom) ) {
		    /* DOI-defined tag type, > 127 */
		    switch ( internal_tag_type[ tag - 128 ] ) {

		    case SGIPSO_BITMAP: {
			register u_char * bp;
			register int      bits;
			register int      bit;
			int               bytes;
			int               divno = 0;

			if ( cipso2(idiom) && (tp[CIPS2TAG_ALIGN] != 0) )
			    goto bad;
			if ( taglen < cipstag_bitmap )
				goto bad;
			cipso_lbl->ml_mint_type = MINT_BIBA_LABEL;
			cipso_lbl->ml_grade     = tp[ cipstag_lvl ];
			bp    = &tp[ cipstag_bitmap ];
			bytes = taglen - cipstag_bitmap;
			for ( ++bytes; --bytes; divno += 8 )  {
				bits = *bp++;
				for ( bit = 0; bits; ++bit ) {
					if ( bits & 0x80 ) {
						*up++ = division_xtoi( 
							divno + bit, doi );
					}
					bits = bits << 1 & 0xff;
				}
			}
			cipso_lbl->ml_divcount += (up - cipso_lbl->ml_list)
						  - cipso_lbl->ml_catcount;
			break;
		    }

		    case SGIPSO_ENUM: {
			register u_short * bp;
			register int       divs;

			if ( taglen & 1 || taglen < CIPSTAG_ENUMS )
				goto bad;
			if ( cipso2(idiom) ) {
			    if ( tp[CIPS2TAG_ALIGN] != 0) 
				goto bad;
			} else {
			    if (tp[ CIPSTAG_FLAGS ])
			    goto bad;
			}
			divs = (taglen - CIPSTAG_ENUMS) / sizeof( u_short );
			bp = (u_short *)(&tp[ CIPSTAG_ENUMS ]);
			if ( (long)bp & 1 ) {
				/* copy array of enums to align */
				bcopy( bp, enums, divs * sizeof( u_short ) );
				bp = enums;
			}
			cipso_lbl->ml_mint_type = MINT_BIBA_LABEL;
			cipso_lbl->ml_grade     = tp[ cipstag_lvl ];
			cipso_lbl->ml_divcount += divs;
			for ( ++divs; --divs; )
				*up++ = division_xtoi( ntohs( *bp++ ), doi);
			break;
		    }

		    case SGIPSO_SPECIAL: 
			/* get special ml_msen_type and ml_mint_type values */
			if ( taglen != SGIPSO_SPECIAL_LEN
			||   tp[ CIPSTAG_MSEN ] < MSEN_MIN_LABEL_NAME
			||   tp[ CIPSTAG_MSEN ] > MSEN_MAX_LABEL_NAME
			||   tp[ CIPSTAG_MINT ] < MINT_MIN_LABEL_NAME
			||   tp[ CIPSTAG_MINT ] > MINT_MAX_LABEL_NAME )
				goto bad;
			cipso_lbl->ml_msen_type = tp[ CIPSTAG_MSEN ];
			cipso_lbl->ml_mint_type = tp[ CIPSTAG_MINT ];
			break;

		    default:
			/*
			 * the default action, for unknown DOI-tag types,
			 * is to ignore them.
			 */
			 break;
		    }
		}
	  }
	/* 
	 * If not SGIPSO, then copy MINT information from the
	 * interface's high label into the label being built. 
	 */
	if ( (! sgipso(idiom)) || (cipso_lbl->ml_mint_type == 0) ) {
		register mac_label * if_highlbl;

		if_highlbl = ifp->if_sec->ifs_label_max;
		if ( cipso_lbl->ml_divcount + if_highlbl->ml_catcount >
		     MAC_MAX_SETS )
			goto bad;	/* punt */
		cipso_lbl->ml_mint_type = if_highlbl->ml_mint_type;
		if ((cipso_lbl->ml_divcount = if_highlbl->ml_divcount) != 0) {
		bcopy(if_highlbl->ml_list + if_highlbl->ml_catcount,
		      cipso_lbl->ml_list  + cipso_lbl->ml_catcount,
		      cipso_lbl->ml_divcount * sizeof(cipso_lbl->ml_list[0]));
		}
	}

	/* 
	 * now sort categories and divisions
	 */
	if ( cipso_lbl->ml_catcount > 1 )
		sort_shorts( &cipso_lbl->ml_list[ 0 ], 
			      cipso_lbl->ml_catcount );
	if ( cipso_lbl->ml_divcount > 1 )
		sort_shorts( &cipso_lbl->ml_list[ cipso_lbl->ml_catcount ],
			      cipso_lbl->ml_divcount );

	/* test for validity of label just built before adding to cache */
	if ( mac_invalid( cipso_lbl ) )  
		goto bad;	/* invalid label */

	entry->lc_int = cipso_lbl = mac_add_label( cipso_lbl );

	/* put new entry on chain */
	m->m_next         = ifp->if_sec->ifs_lbl_cache;
	ifp->if_sec->ifs_lbl_cache = m;

	return cipso_lbl;

bad:
	if ( m ) {
		m_freem( m );	
	}
	if ( cipso_lbl ) {
		kern_free( cipso_lbl );
	}
	return (mac_label *)0;	/* no cookie */
}

/*
 * Find DAC tag and fill in uidp. Return 1 on success, 0 on error.
 */
int
ip_recvdac(
	u_char * cp,                 
        uid_t *  uidp)   /* filled in by this function */
{
        u_char * tp;   /* pointer to tag */
	int len;       /* length of whole option */
        int tag;       /* tag type value       */
        int taglen;    /* length of tag	       */
	register int cnt;

	tp = &cp[CIPSOPT_TAG];
	len = cp[CIPSTAG_LEN];
	for ( cnt = len-CIPSOPT_TAG; cnt > 0; cnt-=taglen, tp+=taglen ) {
	    tag    = tp[ CIPSTAG_VAL ];
	    taglen = tp[ CIPSTAG_LEN ];
	    if (  internal_tag_type[ tag - 128 ] == SGIPSO_DAC  )
		break;
	}
	if ( internal_tag_type[ tag -128 ] != SGIPSO_DAC )
	    return(0);
	*uidp = 0;
	/* 
	 * Assumptions: (1) We are big endian. (2) uid_t 
	 * is twice the size of a short. 
	 */
	bcopy(&tp[CIPSTAG_UID], ((ushort *)uidp) + 1, sizeof(ushort));
	return(1);
}
/*
 * Look for security options in IP header.
 * Return pointer to corresponding internal mac_label, NULL otherwise.
 * Make third arg point to uid.
 */
mac_label *
ip_recvlabel(
	struct mbuf *        m,
	struct ifnet *       ifp,
        uid_t *              uidp)     /* filled in by this function */
{
	register u_char *    cp;
	struct ip *          ip;
	mac_label *          ripso_lbl   = 0;
	mac_label *          cipso_lbl   = 0;
	mac_label *          result      = 0;
	int                  sec_options = 0;
	int                  count_bso   = 0;
	int                  count_eso   = 0;
	int                  count_cipso = 0;
	int                  cnt;
	int                  opt;
	int                  optlen;

    	ASSERT(cipso_enabled);
	/* 
	 * Initialize *uidp to interface's dacless default uid. 
	 */
	*uidp = ifp->if_sec->ifs_uid;  
	if ( ifp->if_sec->ifs_label_max == 0 )
		return (mac_label *)0;	/* no cookie */
	ip = mtod(m, struct ip *);
	cp = (u_char *)(ip + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= 0 || optlen > cnt) {
				goto bad;	/* bad noogies */
			}
		}
		switch (opt) {

		case IPOPT_SECURITY:
			++sec_options;
			if ( count_bso++ != 0
			||   if_policy[ ifp->if_sec->ifs_idiom ] != BSO_VALID )
				break;
			ripso_lbl = ip_rcv_bso( cp, ifp, ripso_lbl );
			break;

		case IPOPT_ESO:
			++sec_options;
			if ( count_eso++ != 0 
			||   if_policy[ ifp->if_sec->ifs_idiom ] != ESO_VALID )
				break;
			ripso_lbl = ip_rcv_eso( cp, ifp, ripso_lbl );
			break;

		case IPOPT_CIPSO:
			++sec_options;
			if ( count_cipso++ != 0 
			||   if_policy[ ifp->if_sec->ifs_idiom ] != CIPSO_VALID )
				break;
			if ( dacful(ifp->if_sec->ifs_idiom) ) { 
			    /*
			     * Default uid is replaced by true uid. 
			     */
			    if ( ip_recvdac(cp, uidp) == 0 ) {
				/* Should audit missing DAC tag */
				return((mac_label *)0);
			    }
			}
			cipso_lbl = ip_rcv_cipso( cp, ifp, cipso_lbl );
			break;
		default:
			break;

		}
	}

	/* parsed all the options, now see if policy permits this */
	switch ( ifp->if_sec->ifs_idiom ) {

	case IDIOM_CIPSO2:
	case IDIOM_SGIPSO2:
	case IDIOM_SGIPSO2_NO_UID:
	case IDIOM_TT1_CIPSO2:
	case IDIOM_CIPSO:
	case IDIOM_SGIPSOD:
	case IDIOM_SGIPSO:
	case IDIOM_TT1:
		if ( sec_options == 1 && count_cipso == 1 ) {
			result = cipso_lbl;
		} 
		break;

	case IDIOM_BSO_RX:
	case IDIOM_BSO_REQ:
		if ( count_bso == 1 && count_eso < 2 
		&& (sec_options - count_eso) == 1 ) {
			result = ripso_lbl;
		} 
		break;

	case IDIOM_BSO_TX:
		if ( count_bso == 1 && count_eso < 2 
		&& (sec_options - count_eso) == 1 ) {
			result =  ripso_lbl;
		} 
		/* FALL THROUGH */

	case IDIOM_MONO:
	default:
		if ( sec_options == 0 ) {
			_SAT_BSDIPC_RANGE( ifp, ip, *uidp, ifp->if_sec->ifs_label_max, 
				SAT_BSDIPC_RX_OK, 0 );
			return ifp->if_sec->ifs_label_max;
		}
		break;

	}
	if ( result ) {
		int ok;

		ok = mac_inrange( ifp->if_sec->ifs_label_min, result, 
				  ifp->if_sec->ifs_label_max );
		_SAT_BSDIPC_RANGE( ifp, ip, *uidp, result, 
				  ok ? SAT_BSDIPC_RX_OK : SAT_BSDIPC_RX_RANGE,
				  0);
		return ( ok ? result : (mac_label *)0 );
	}
bad:
	_SAT_BSDIPC_MISSING( ifp, ip, 0 );
	return ((mac_label *)0 );
}
