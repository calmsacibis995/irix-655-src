#ident "$Revision: 1.12 $"

#include <bstring.h>
#include "sys/param.h"
#include "sys/debug.h"
#include "sys/mbuf.h"
#include "sys/errno.h"
#include "sys/mac_label.h"
#include <sys/cipso.h>

#include "sys/tcpipstats.h"

#include "net/if.h"
#include "in.h"
#include "in_systm.h"		/* needed for ip.h" */
#include "ip.h"
#include "ip_var.h"		/* needed for MAX_IPOPTLEN & struct ipstat */
#include "ip_secopts.h"
#include "sys/sat.h"

#define MSEN 0                  /* Flag values for */
#define MINT 1                  /* cipso_add_msen_or_mint(). */
/*
 * This table maps from internal sensitivity level to BSO level.
 * A zero value in the table signifies an invalid BSO level.
 * This table is the inverse mapping of the table ripso_lvl_valid[].
 */
STATIC_PRF
u_char level_itor_tab[256] = {
	0xf1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	0xab, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,

	0xcc, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	0x66, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,

	0x96, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	0x5a, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,

	0x3D, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
	   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0x01
};

STATIC_PRF
u_char external_tag_type[] = { 0, 128, 129, 130, 131 };

/*
 * Separate IP header from old message into a new mbuf, 
 * and remove any old IP security options (e.g. RIPSO, CIPSO).
 * Assumes that the entire IP header, including any extant options, 
 * is all in one mbuf.
 */
struct mbuf *
ip_strip_security( 
	struct mbuf *        old )
{
	u_char *             cp;
	struct ip *          ip;
	struct mbuf *        new;
	caddr_t              to;
	int                  cnt;
	int                  hlen;
	int                  opt;
	int                  optlen;

	if ((new = m_get( M_DONTWAIT, MT_HEADER)) == 0)
		return new;	/* punt */;
	new->m_flags |= old->m_flags & M_COPYFLAGS;
	new->m_off = MMAXOFF - (sizeof(*ip) + MAX_IPOPTLEN );
	to = mtod(new, caddr_t);
	ip = mtod(old, struct ip *);
	bcopy( ip, to, sizeof(*ip) );
	new->m_len = sizeof(*ip);
	to += sizeof(*ip);

	hlen = ip->ip_hl << 2;
	cp      = (u_char *)(ip + 1);
	cnt = hlen - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			ASSERT( optlen > 0 && optlen <= cnt);
			/* A packet with bad options should never have 
			 * gotten this far.  It should have been caught
			 * in ip_intr() or ip_pcbopts().
			 */
		}
		switch (opt) {

		case IPOPT_NOP:
		case IPOPT_SECURITY:
		case IPOPT_ESO:
		case IPOPT_CIPSO:
			break;		/* don't copy these */

		default:
			bcopy( cp, to, optlen ); 
			to += optlen;
			break;
		}
	}
	old->m_off += hlen;
	old->m_len -= hlen;
	new->m_next = old;
	/* 
	 *  The options have been copied into the new mbuf.
	 *  No NOP or EOL options have been copied, hence the header
	 *  length may not be a multiple of 4.  NOPs and/or EOL will
	 *  have to be added later after any security options are added.
	 *  new->m_len contains the actual (possibly odd) header length,
	 *  to facilitate adding options later.
	 */
	new->m_len = to - mtod(new, caddr_t);
	return new;
}

/* 
 * search interface's label cache for entry matching given mac_label.
 * NOTE: must be called at splnet or eqivalent.
 */
STATIC_PRF
struct mbuf *
if_lbl_cache_search( 
	struct ifnet *  ifp,
	mac_label *     dlbl)
{
	register struct mbuf *   m;

	for ( m = ifp->if_sec->ifs_lbl_cache; m; m = m->m_next ) {
		if ( dlbl == ((label_cache_t *)m->m_dat)->lc_int ) {
			/* found cache entry */
			break;
		}
	}
	return m;
}

/*
 * add a cache_entry to the label cache for an interface.
 * NOTE: must be called at splnet or eqivalent.
 */
STATIC_PRF
void
if_lbl_cache_add( 
	struct ifnet * ifp,
	mac_label *    dlbl,
	struct mbuf *   m)
{
	label_cache_t * entry;

	entry = (label_cache_t *)m->m_dat;
	entry->lc_int = dlbl;

	/* put new entry on chain */
	m->m_next = ifp->if_sec->ifs_lbl_cache;
	ifp->if_sec->ifs_lbl_cache = m;
}


STATIC_PRF
struct mbuf *
ip_xmt_bso( 
	mac_label *       dlbl,
	int               authority)
{
	label_cache_t *   entry;
	struct mbuf *     m;
	int               bsolvl;

	if ( ! (bsolvl = level_itor( dlbl->ml_level )))
		return (struct mbuf *)0;

	if ((m = m_get(M_DONTWAIT, MT_SOOPTS)) == 0)
		return m;

	entry = (label_cache_t *)m->m_dat;
	m->m_off += sizeof( entry->lc_int );
	m->m_len = BSO_MIN_LEN;
	entry->lc_int = 0;
	entry->lc_ext[ IPOPT_OPTVAL ] = IPOPT_SECURITY;
	entry->lc_ext[ IPOPT_OLEN   ] = BSO_MIN_LEN;
	entry->lc_ext[ RIPSOPT_LVL  ] = bsolvl;
	entry->lc_ext[ RIPSOPT_AUTH ] = authority;
	return m;
}

/*
 * Create an mbuf in the format of a cache entry for a CIPSO option. 
 */
STATIC_PRF
struct mbuf *
cipso_start_buf( 
	u_long                   doi)	
{
	register struct mbuf *   m;
	register label_cache_t * entry;
	register cipso_tt1_t *   tt1;

	if ((m = m_getclr(M_DONTWAIT, MT_SOOPTS)) == 0)
		return m;	/* return null value */

	entry = (label_cache_t *)m->m_dat;
	entry->lc_int = 0;
	tt1   = (cipso_tt1_t *)entry->lc_ext;
	tt1->opttype   = IPOPT_CIPSO;
	tt1->optlength = CIPSOPT_TAG;
	tt1->doih      = htons( doi >> 16 );
	tt1->doil      = htons( doi & 0xffff );
	m->m_off += sizeof( entry->lc_int );
	m->m_len = tt1->optlength;
	return m;
}


/* 
 * Accepts an mbuf and adds a CIPSO2 msen tag or a SGIPSO2 mint tag 
 * to that mbuf.  Does NOT add the option to the cache.  If the
 * requested label component (msen or mint) can be represented, 
 * returns a pointer to the mbuf.  Otherwise, returns a null pointer.
 */
/*ARGSUSED*/
STATIC_PRF
struct mbuf *
cipso2_add_msen_or_mint( 
	struct mbuf *   m,
	mac_label *     dlbl,
	u_long          doi,
	int             idiom,
	int		lc_flag)      /* Label component: MSEN or MINT. */
{
	cipso2_tt1_t *   option;
	cipso2_tt1_t *   winner;
	int              count;      
	cipso2_tt1_t     tt1;
	cipso2_tt2_t     tt2;

	if ( ! m )
		return (struct mbuf *)0;	/* that was easy */
	if ( lc_flag == MSEN ) {
	        if ( (count = dlbl->ml_catcount) > CIPSO2_MAX_BITS) {
		     m_freem( m );
		     return (struct mbuf *)0;	/* no cookie */
	        }
	} else
	        count = dlbl->ml_divcount;
	bzero( &tt1, sizeof tt1 );
	if ( lc_flag == MSEN ) {
		tt1.tagtype   = 1;
		tt1.level     = level_itox( dlbl->ml_level, doi );
	} else {
		tt1.tagtype   = external_tag_type[ SGIPSO_BITMAP ];
		tt1.level     = dlbl->ml_grade;
	}
	tt1.taglength = CIPS2TAG_BITMAP;
	/* even if the internal categories are sorted, the external ones
	 * may not be in order, so we have to do this loop.
	 * If any category is > CIPSO2_MAX_BITS, then it can't be made into
	 * a tag type 1.
	 */
	if ( count > 0 && count <= CIPSO2_MAX_BITS ) {
		u_short *       this;
		u_short *       last;
		int             exbit;
		int             maxbit   = -1;

		tt1.taglength = MAX_IPOPTLEN + 1; /* to deal with errors */
		this = dlbl->ml_list;
		if ( lc_flag == MINT ) 
			this += dlbl->ml_catcount;
		last = this + ( count - 1 );
		do {
			if (lc_flag == MSEN) 
				exbit = category_itox( *this, doi );
			else
				exbit = *this;
			if ( exbit >= CIPSO2_MAX_BITS ) {
				maxbit = MAX_IPOPTLEN * 8;
				break;	/* no cookie */
			}
			if ( exbit > maxbit)
				maxbit = exbit;
			tt1.bits[ exbit >> 3 ] |= 0x80 >> (exbit & 7);
		} while ( ++this <= last );
		tt1.taglength = CIPS2TAG_BITMAP + ((maxbit + 8)/8);
	}

	/* construct an enumerated tag */
	bzero( &tt2, sizeof tt2 );
	if ( lc_flag == MSEN ) {
		tt2.tagtype   = 2;
		tt2.level     = level_itox( dlbl->ml_level, doi );
	} else {
		tt2.tagtype   = external_tag_type[ SGIPSO_ENUM ];
		tt2.level     = dlbl->ml_grade;
	}
	if ( type_1_only(idiom) || count > CIPSO_MAX_ENUM ) {
		tt2.taglength = MAX_IPOPTLEN + 1;
	} else if ( count == 0 ) {
		tt2.taglength = CIPSTAG_ENUMS;
	} else {
		u_short *       from;
		u_short *       to;
		u_short *       last;
		int             excat;
		int             prevcat  = -1;
		int		sorted   = 1;  

		/* 
		 * For Big Endian machines, take advantage of the fact that 
		 * Network Byte Order is Big Endian, do 2 passes.
		 *
		 * On other machines (little-endian, PDP-endian), do 3 passes:
		 * 1.  Convert from internal to external category numbers.
		 * 2.  Sort in ascending external order (if not already).
		 * 3.  Convert from host to network byte order.
		 */
		/* pass 1 */
		from = dlbl->ml_list;
		if ( lc_flag == MINT )
			from += dlbl->ml_catcount;
		last = from + ( count - 1 );
		to   = tt2.cats;
		do {
			if ( lc_flag == MSEN )
				excat = category_itox( *from, doi );
			else
				excat = *from;
			if ( excat <= prevcat )
				sorted = 0;
			*to++ = prevcat = excat;
		} while ( ++from <= last );
		/* pass 2 */
		if ( ! sorted ) {	/* Then sort 'em */
			sort_shorts( tt2.cats, count );
		}
#if BYTE_ORDER != BIG_ENDIAN
		/* pass 3 */
		to   = tt2.cats;
		last = to + count;
		do {
			*to = htons( *to );
		} while ( ++to < last );
#endif
		tt2.taglength = CIPSTAG_ENUMS + count * sizeof(u_short);
	}
	if ( m->m_len + tt1.taglength > MAX_IPOPTLEN ) {
		if ( m->m_len + tt2.taglength > MAX_IPOPTLEN ) {
			/* cannot represent mint in either tag type */
			m_freem( m );
			return (struct mbuf *)0;
		} 
		winner = (cipso2_tt1_t *)&tt2;
	} else if ( m->m_len + tt2.taglength > MAX_IPOPTLEN 
	       ||   tt2.taglength > tt1.taglength ) {
		winner = &tt1;
	} else {
		winner = (cipso2_tt1_t *)&tt2;
	}
	ASSERT(m->m_off + m->m_len + winner->taglength <= MMAXOFF);
	bcopy( &winner->tagtype, mtod(m, caddr_t) + m->m_len, 
		winner->taglength );
	option = mtod( m, cipso2_tt1_t *);
	option->optlength += winner->taglength;
	m->m_len    += winner->taglength;

	return m;
}

/*ARGSUSED*/
STATIC_PRF
struct mbuf *
cipso_add_msen_or_mint( 
	struct mbuf *   m,
	mac_label *     dlbl,
	u_long          doi,
	int             idiom,
	int		lc_flag)      /* Label component: MSEN or MINT. */
{
	cipso_tt1_t *   option;
	cipso_tt1_t *   winner;
	int             count;      
	cipso_tt1_t     tt1;
	cipso_tt2_t     tt2;

	if ( ! m )
		return (struct mbuf *)0;	/* that was easy */
	if ( lc_flag == MSEN ) {
	        if ( (count = dlbl->ml_catcount) > CIPSO_MAX_BITS) {
		     m_freem( m );
		     return (struct mbuf *)0;	/* no cookie */
	        }
	} else
	        count = dlbl->ml_divcount;
	bzero( &tt1, sizeof tt1 );
	if ( lc_flag == MSEN ) {
		tt1.tagtype   = 1;
		tt1.level     = level_itox( dlbl->ml_level, doi );
	} else {
		tt1.tagtype   = external_tag_type[ SGIPSO_BITMAP ];
		tt1.level     = dlbl->ml_grade;
	}
	tt1.taglength = CIPSTAG_BITMAP;

	/* even if the internal categories are sorted, the external ones
	 * may not be in order, so we have to do this loop.
	 * If any category is > CIPSO_MAX_BITS, then it can't be made into
	 * a tag type 1.
	 */
	if ( count > 0 && count <= CIPSO_MAX_BITS ) {
		u_short *       this;
		u_short *       last;
		int             exbit;
		int             maxbit   = -1;

		tt1.taglength = MAX_IPOPTLEN + 1; /* to deal with errors */
		this = dlbl->ml_list;
		if ( lc_flag == MINT ) 
			this += dlbl->ml_catcount;
		last = this + ( count - 1 );
		do {
			if (lc_flag == MSEN) 
				exbit = category_itox( *this, doi );
			else
				exbit = *this;
			if ( exbit >= CIPSO_MAX_BITS ) {
				maxbit = MAX_IPOPTLEN * 8;
				break;	/* no cookie */
			}
			if ( exbit > maxbit)
				maxbit = exbit;
			tt1.bits[ exbit >> 3 ] |= 0x80 >> (exbit & 7);
		} while ( ++this <= last );
		tt1.taglength = CIPSTAG_BITMAP + ((maxbit + 8)/8);
	}

	/* construct an enumerated tag */
	bzero( &tt2, sizeof tt2 );
	if ( lc_flag == MSEN ) {
		tt2.tagtype   = 2;
		tt2.level     = level_itox( dlbl->ml_level, doi );
	} else {
		tt2.tagtype   = external_tag_type[ SGIPSO_ENUM ];
		tt2.level     = dlbl->ml_grade;
	}
	if ( type_1_only(idiom) || count > CIPSO_MAX_ENUM ) {
		tt2.taglength = MAX_IPOPTLEN + 1;
	} else if ( count == 0 ) {
		tt2.taglength = CIPSTAG_ENUMS;
	} else {
		u_short *       from;
		u_short *       to;
		u_short *       last;
		int             excat;
		int             prevcat  = -1;
		int		sorted   = 1;  

		/* 
		 * For Big Endian machines, take advantage of the fact that 
		 * Network Byte Order is Big Endian, do 2 passes.
		 *
		 * On other machines (little-endian, PDP-endian), do 3 passes:
		 * 1.  Convert from internal to external category numbers.
		 * 2.  Sort in ascending external order (if not already).
		 * 3.  Convert from host to network byte order.
		 */
		/* pass 1 */
		from = dlbl->ml_list;
		if ( lc_flag == MINT )
			from += dlbl->ml_catcount;
		last = from + ( count - 1 );
		to   = tt2.cats;
		do {
			if ( lc_flag == MSEN )
				excat = category_itox( *from, doi );
			else
				excat = *from;
			if ( excat <= prevcat )
				sorted = 0;
			*to++ = prevcat = excat;
		} while ( ++from <= last );
		/* pass 2 */
		if ( ! sorted ) {	/* Then sort 'em */
			sort_shorts( tt2.cats, count );
		}
#if BYTE_ORDER != BIG_ENDIAN
		/* pass 3 */
		to   = tt2.cats;
		last = to + count;
		do {
			*to = htons( *to );
		} while ( ++to < last );
#endif
		tt2.taglength = CIPSTAG_ENUMS + count * sizeof(u_short);
	}

	if ( m->m_len + tt1.taglength > MAX_IPOPTLEN ) {
		if ( m->m_len + tt2.taglength > MAX_IPOPTLEN ) {
			/* cannot represent mint in either tag type */
			m_freem( m );
			return (struct mbuf *)0;
		} 
		winner = (cipso_tt1_t *)&tt2;
	} else if ( m->m_len + tt2.taglength > MAX_IPOPTLEN 
	       ||   tt2.taglength > tt1.taglength ) {
		winner = &tt1;
	} else {
		winner = (cipso_tt1_t *)&tt2;
	}

	ASSERT(m->m_off + m->m_len + winner->taglength <= MMAXOFF);
	bcopy( &winner->tagtype, mtod(m, caddr_t) + m->m_len, 
		winner->taglength );
	option = mtod( m, cipso_tt1_t *);
	option->optlength += winner->taglength;
	m->m_len    += winner->taglength;

	return m;
}


/*
 * Increment optlength field of option by SGIPSO_DAC_LEN, but don't
 * increment m->m_len at this time.
 */
STATIC_PRF
struct mbuf *
dac_fix_optlen(	struct mbuf *   m )
{
	cipso_tt1_t *   option;

	if ( ! m ) {
	    return (struct mbuf *)0;
	}
	if ( m->m_len + SGIPSO_DAC_LEN > MAX_IPOPTLEN ) {
	    m_freem( m );
	    return (struct mbuf *)0;
	}
	option = mtod( m, cipso_tt1_t *);
	option->optlength += SGIPSO_DAC_LEN;
	return m;		
}

STATIC_PRF
struct mbuf *
sgipso_add_special( 
	struct mbuf *   m,
	mac_label *     dlbl)
{
	u_char *        tp;	/* tag pointer */
	cipso_tt1_t *   option;

	if ( ! m )
		return m;		/* that was easy */
	if ( m->m_len + SGIPSO_SPECIAL_LEN > MAX_IPOPTLEN ) {
		/* won't fit */
		m_freem( m );
		return (struct mbuf *)0;
	} 
	ASSERT(m->m_off + m->m_len + SGIPSO_SPECIAL_LEN <= MMAXOFF);
	tp = mtod( m, u_char *) + m->m_len;
	tp[ CIPSTAG_VAL ]  = external_tag_type[ SGIPSO_SPECIAL ];
	tp[ CIPSTAG_LEN ]  = SGIPSO_SPECIAL_LEN;
	tp[ CIPSTAG_MSEN ] = dlbl->ml_msen_type;
	tp[ CIPSTAG_MINT ] = dlbl->ml_mint_type;

	option = mtod( m, cipso_tt1_t *);
	option->optlength += SGIPSO_SPECIAL_LEN;
	m->m_len    += SGIPSO_SPECIAL_LEN;

	return m;
}

/*
 * Create an mbuf, containing a cache entry ready to be inserted into the
 * cache for the interface ifp, containing the external representation of
 * the datagram label dlbl.  
 */
STATIC_PRF
struct mbuf *
if_lbl_create( 
	struct ifnet *  ifp,
	mac_label *     dlbl)
{
	struct mbuf *   m = 0;
	u_long          doi;
	int             idiom;
	int		msen_type;

	doi = ifp->if_sec->ifs_doi;
	switch ( idiom = (int)ifp->if_sec->ifs_idiom ) {

	case IDIOM_SGIPSO2:
	case IDIOM_SGIPSO2_NO_UID:
		m = cipso_start_buf( doi );
		if ((msen_type = dlbl->ml_msen_type) == MSEN_TCSEC_LABEL 
		||  msen_type == MSEN_MLD_LABEL ) {
			m = cipso2_add_msen_or_mint(m, dlbl, doi, idiom, MSEN);
		}
		if ( dlbl->ml_mint_type == MINT_BIBA_LABEL ) {
			m = cipso2_add_msen_or_mint( m, dlbl, 0, idiom, MINT );

		}
		if ( dlbl->ml_msen_type != MSEN_TCSEC_LABEL
		||   dlbl->ml_mint_type != MINT_BIBA_LABEL ) {
			m = sgipso_add_special( m, dlbl );
		}
		/* 
		 * For DACful idioms, Increment optlen now, although
		 * true length won't match it until later.
		 */
		if ( dacful(idiom) )
		    m = dac_fix_optlen(m);
		break;

	case IDIOM_SGIPSO: 
	case IDIOM_SGIPSOD: 
		m = cipso_start_buf( doi );
		if ((msen_type = dlbl->ml_msen_type) == MSEN_TCSEC_LABEL 
		||  msen_type == MSEN_MLD_LABEL ) {
			m = cipso_add_msen_or_mint(m, dlbl, doi, idiom, MSEN );
		}
		if ( dlbl->ml_mint_type == MINT_BIBA_LABEL ) {
			m = cipso_add_msen_or_mint( m, dlbl, 0, idiom, MINT );
		}
		if ( dlbl->ml_msen_type != MSEN_TCSEC_LABEL
		||   dlbl->ml_mint_type != MINT_BIBA_LABEL ) {
			m = sgipso_add_special( m, dlbl );
		}
		/* 
		 * For DACful idioms, Increment optlen now, although
		 * true length won't match it until later.
		 */
		if ( dacful(idiom) )
		    m = dac_fix_optlen(m);
		break;

	case IDIOM_CIPSO2: /* temp */ 
	case IDIOM_TT1_CIPSO2:
		if ( dlbl->ml_msen_type == MSEN_TCSEC_LABEL ) {
			m = cipso_start_buf( doi );
			m = cipso2_add_msen_or_mint(m, dlbl, doi, idiom, MSEN);
		}
		break;

	case IDIOM_CIPSO: 
	case IDIOM_TT1:
		if ( dlbl->ml_msen_type == MSEN_TCSEC_LABEL ) {
			m = cipso_start_buf( doi );
			m = cipso_add_msen_or_mint(m, dlbl, doi, idiom, MSEN);
		}
		break;

	case IDIOM_BSO_TX:
	case IDIOM_BSO_REQ:
	case IDIOM_BSO_RX:
		if ( dlbl->ml_msen_type == MSEN_TCSEC_LABEL ) {
			m = ip_xmt_bso( dlbl, ifp->if_sec->ifs_authority_max );
		}
		break;

	default:
		break;
	}
	return m;
}

struct mbuf *
if_dac_create( uid_t duid )
{
	struct mbuf *   m = 0;
	dac_tt_t    *   dact;

	if ((m = m_get(M_DONTWAIT, MT_SOOPTS)) == 0) {
		return m;	/* return null value */
	    }
	dact = mtod( m, dac_tt_t *);
        dact->tagtype = external_tag_type[ SGIPSO_DAC ];
	dact->taglength = SGIPSO_DAC_LEN;
	dact->uid = htons(duid);
	m->m_len = dact->taglength;
	return m;
}

/* ip_xmitlabel - translate internal label into external label
 * strategy:
 * 1. separate the ip header from the rest of the datagram, and 
 *    strip out any old security options from the header
 * 2. construct the option to be sent based on the policy of the outgoing
 *	interface.
 * 3. See if the option is too big for the ip header. If so, punt this packet.
 * 4. If not too big, combine into header.
 * If any errors occur, free the input mbuf chain (m), and return null pointer.
 * Otherwise return pointer to (probably modified) message.
 * NOTE: must be called at splnet or eqivalent.
 */
struct mbuf *
ip_xmitlabel(
	struct ifnet *       ifp,
	struct mbuf *        m,
	mac_label *          dlbl,
	uid_t                duid,	
	int *                error)
{
	struct ip *          ip;
	struct mbuf *        m2;
	u_char *             cp;
	int                  hlen;
	int                  idiom;
	int                  newhlen;
	int                  bodylen;
	int                  pad_bytes;

    	ASSERT(cipso_enabled);
    	ASSERT(ifp);
    	ASSERT(m);

	/* We can't do anything until the interface is labeled */
	if (!ifp->if_sec)
		goto no_cookie;
    	ASSERT(ifp->if_sec);

#ifndef notlikely
	ASSERT( m->m_len >= sizeof (struct ip) );
#else
	if (m->m_len < sizeof (struct ip) &&
	    (m = m_pullup(m, sizeof (struct ip))) == 0) {
		IPSTAT(ips_toosmall);
		goto no_cookie;
	}
#endif
	ip = mtod(m, struct ip *);
	hlen = ip->ip_hl << 2;
#ifndef notlikely
	ASSERT( hlen >= sizeof(struct ip) );
#else
	if (hlen < sizeof(struct ip)) {	/* minimum header length */
		IPSTAT(ips_badhlen);
		goto no_free_cookie;
	}
#endif
	if (hlen > m->m_len) {
		if ((m = m_pullup(m, hlen)) == 0) {
			IPSTAT(ips_badhlen);
			goto no_cookie;
		}
		ip = mtod(m, struct ip *);
	}

	idiom = (int)ifp->if_sec->ifs_idiom;
	if (! dacful(idiom) ) 		
		duid = -2;  	/* "nobody" for sat_bsdipc_range */

	if ( ifp->if_sec->ifs_label_max == 0 
	||   ! mac_inrange( ifp->if_sec->ifs_label_min, dlbl, ifp->if_sec->ifs_label_max ) ) {
		/* audit MAC enforcement */
		_SAT_BSDIPC_RANGE( ifp, ip, duid, dlbl, SAT_BSDIPC_TX_RANGE, 0);
		goto no_free_cookie;
	}

	bodylen = ip->ip_len - hlen;
	m2 = ip_strip_security( m );
	if ( m2 == 0 ) {
		/* failure (due to lack of mbufs) is not security relevant.
		 * so don't audit this path. 
		 */
		goto no_free_cookie;
	}
	m = m2;
	ip = mtod(m, struct ip *);
	newhlen = m->m_len;
	/* Now, the IP header, and any residual options, have been put into
	 * a separate mbuf at the front of the chain.  
	 * The length of that mbuf (m_len) is the length of the header.  
	 */

	/* Decide whether or not to send optional BSO headers.  */
	if ((idiom = (int)ifp->if_sec->ifs_idiom) == IDIOM_BSO_RX 
	&&  mac_equ( dlbl, ifp->if_sec->ifs_label_max ) ) 
		idiom = IDIOM_MONO;	/* don't send optional BSO header */
	else if ( idiom != IDIOM_MONO ) {
		m2 = if_lbl_cache_search( ifp, dlbl );
		if ( ! m2 ) {
			m2 = if_lbl_create( ifp, dlbl );
			if ( ! m2 ) {
				goto audit_no_free_cookie;
			}
			if_lbl_cache_add( ifp, dlbl, m2 );
		}

		newhlen += m2->m_len;
		if ( dacful(idiom) ) {		    
		    newhlen += SGIPSO_DAC_LEN;
		}
		if ( newhlen > sizeof(*ip) + MAX_IPOPTLEN 
		||   newhlen + bodylen > IP_MAXPACKET ) {
			/* it won't fit, so punt */
			goto audit_no_free_cookie;
		}
		bcopy(	mtod( m2, caddr_t), mtod( m, caddr_t) + m->m_len, 
			m2->m_len );
		if ( dacful(idiom) ) {
		    m->m_len  = newhlen - SGIPSO_DAC_LEN;
		    m2 = if_dac_create( duid );
		    if (m2 == 0) 
			goto no_free_cookie;
		    cp = mtod(m2, u_char *);
		    bcopy(mtod( m2, caddr_t), mtod( m, caddr_t) +
			  m->m_len, m2->m_len );
		    m_freem(m2);
		}
		m->m_len  = newhlen;
	}
	/* clean up message */
	if ( newhlen & 3 ) { /* need to pad out options */
		pad_bytes = 4 - ( newhlen & 3 );
		newhlen += pad_bytes;
		cp = (u_char *)m + m->m_off + m->m_len;
		while ( pad_bytes-- ) {
			*cp++ = IPOPT_EOL;
		}
		m->m_len  = newhlen;
	}
	ip->ip_hl  = newhlen >> 2;
	ip->ip_len = newhlen + bodylen;
	/* Audit MAC passage */
	_SAT_BSDIPC_RANGE( ifp, ip, duid, dlbl, SAT_BSDIPC_TX_OK, 0);
	return m;

audit_no_free_cookie:
	_SAT_BSDIPC_RANGE( ifp, ip, duid, dlbl, SAT_BSDIPC_TX_TOOBIG, 0);
	/* Audit that label couldn't be sent */
no_free_cookie:
	m_freem(m);
no_cookie:
	*error = EACCES;
	return (struct mbuf *)0;
}
