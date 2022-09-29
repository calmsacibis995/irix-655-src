#ident "$Revision: 1.8 $"
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <net/if.h>
struct mac_label;

/* 
 * Each interface has its own cache of mappings between internal (mac_label)
 * and external (CIPSO, or SGIIPSO) representations of labels.
 * This is the structure of label cache entries.
 * label_cache entries are kept in mbufs.
 * m_off points to lc_ext, and m_len is the length of the external form.
 */
typedef struct label_cache {
	struct mac_label *lc_int;	/* internal form of this label */
	char        lc_ext[1];		/* external (on wire) form of label */
} label_cache_t;

/*
 * Defines for RFC 1108 - DOD Security Option For IP (SOFTIP) (Nee: RIPSO)
 */
#define RIPSOPT_LVL	2	/* offset to level field	*/
#define RIPSOPT_AUTH	3	/* offset to authority field	*/
#define BSO_MIN_LEN	4	/* from RFC 1108 */
#define level_rtoi(n) level_rtoi_tab[ (n) ]
#define level_itor(n) level_itor_tab[ (n) ]

/*
 * Defines for APR 1991 RFC ??? Commercial IP Security Option (CIPSO) 
 */
#define CIPSO_MIN_LEN	 6	/* from draft RFC */
#define CIPSO_MAX_LEN	40	/* from draft RFC */
#define CIPSO_MAX_BITS 248	/* maximum bits in tag types 1 and 129  */
#define CIPSO_MAX_ENUM  15	/* maximum enumerated categories (TT 2) */
#define CIPSOPT_DOI	 2	/* offset to beginning of DOI field	*/
#define CIPSOPT_TAG  	 6	/* offset of first tag			*/

/*
 * Defines for APR 1991 RFC ??? Commercial IP Security Option 
 * (CIPSO) Tags (suboptions)
 */
#define CIPSTAG_VAL      0	/* offset of tag value within tag	*/
#define CIPSTAG_LEN      1	/* offset of tag length within tag      */
#define CIPSTAG_LVL      2	/* offset of tag level			*/
#define CIPSTAG_BITMAP   3	/* offset of tag type 1 bitmap of cats	*/
#define CIPSTAG_FLAGS    3	/* offset of tag type 2 flags           */
#define CIPSTAG_ENUMS    4	/* offset of tag type 2 enumerated cats */
#define CIPSFLAG_EXCL    1	/* mask for exclusion bit in tag type 2 */

/*
 * Additional defines for JAN 1992 RFC Commercial IP Security Option
 * (CIPSO2)
 */
#define CIPSO2_MAX_BITS 240	/* maximum bits in tag types 1 and 129  */
#define CIPS2TAG_ALIGN   2	/* offset of alignment octet		*/
#define CIPS2TAG_LVL     3	/* offset of tag level			*/
#define CIPS2TAG_BITMAP  4	/* offset of tag type 1 bitmap of cats	*/

/*
 * SGI's CIPSO extensions
 */
#define SGIPSO_BITMAP    1	/* mint flavored bitmamp tag value      */
#define SGIPSO_ENUM      2	/* mint flavored enumerated tag value   */
#define SGIPSO_SPECIAL   3	/* Special label type (high, low, equal)*/
#define SGIPSO_DAC       4	/* UID tag value                        */

#define SGIPSO_SPECIAL_LEN   4	/* Length of SGIPSO_SPECIAL tag		*/
#define CIPSTAG_MSEN     2	/* offset of msen_type in SPECIAL tag   */
#define CIPSTAG_MINT     3	/* offset of mint_type in SPECIAL tag   */

#define SGIPSO_DAC_LEN   4	/* Length of SGIPSO_DAC tag		*/
#define CIPSTAG_UID      2	/* offset of (ushort) uid in DAC tag    */

/* the following structures are useful only for creating CIPSO options,
 * since incoming options are not (in general) aligned properly.
 * These structures are based on the APR 1991 RFC (CIPSO).
 */
typedef struct	cipso_tt1 {	/* option containing CIPSO tag type 1	*/
	u_char	opttype;	/* option type				*/
	u_char	optlength;	/* option length			*/
	u_short	doih;		/* Most signficant 16 bits of DOI 	*/
	u_short	doil;		/* Least signficant 16 bits of DOI 	*/
	u_char	tagtype;	/* tag type == 1			*/
	u_char	taglength;	/* length of tag, in bytes		*/
	u_char	level;		/* sensitivity level - external rep.	*/
	u_char	bits[CIPSO_MAX_BITS/8]; /* big_endian category bit map 	*/
} cipso_tt1_t;

typedef struct	cipso_tt2 {	/* option containing CIPSO tag type 2	*/
	u_char	opttype;	/* option type				*/
	u_char	optlength;	/* option length			*/
	u_short	doih;		/* Most signficant 16 bits of DOI 	*/
	u_short	doil;		/* Least signficant 16 bits of DOI 	*/
	u_char	tagtype;	/* tag type == 2			*/
	u_char	taglength;	/* length of tag, in bytes		*/
	u_char	level;		/* sensitivity level - external rep.	*/
	u_char	flags;		/* flag bits				*/
	u_short	cats[CIPSO_MAX_ENUM]; /* enumerated list of categories	*/
} cipso_tt2_t;

/* Structures for creating CIPSO2 (Jan 92 RFC) options.
 */
typedef struct	cipso2_tt1 {	/* option containing CIPSO2 tag type 1	*/
	u_char	opttype;	/* option type				*/
	u_char	optlength;	/* option length			*/
	u_short	doih;		/* Most signficant 16 bits of DOI 	*/
	u_short	doil;		/* Least signficant 16 bits of DOI 	*/
	u_char	tagtype;	/* tag type == 1			*/
	u_char	taglength;	/* length of tag, in bytes		*/
	u_char	align;		/* alignment octet - new in CIPSO2      */
	u_char	level;		/* sensitivity level - external rep.	*/
	u_char	bits[CIPSO2_MAX_BITS/8]; /* big_endian category bit map */
} cipso2_tt1_t;

typedef struct	cipso2_tt2 {	/* option containing CIPSO2 tag type 2	*/
	u_char	opttype;	/* option type				*/
	u_char	optlength;	/* option length			*/
	u_short	doih;		/* Most signficant 16 bits of DOI 	*/
	u_short	doil;		/* Least signficant 16 bits of DOI 	*/
	u_char	tagtype;	/* tag type == 2			*/
	u_char	taglength;	/* length of tag, in bytes		*/
	u_char	align;		/* alignment octet - new in CIPSO2     	*/
	u_char	level;		/* sensitivity level - external rep.	*/
	u_short	cats[CIPSO_MAX_ENUM]; /* enumerated list of categories	*/
} cipso2_tt2_t;

/* SGIPSO DAC tag type
 */
typedef struct	dac_tt {
	u_char	tagtype;	/* type = external_tag_type[SGIPSO_DAC] */
	u_char	taglength;	/* length = SGIPSO_DAC_LEN		*/
	ushort	uid;		/* uid  		                */
} dac_tt_t;

/* 
 * These pseudo functions translate category numbers from external to
 * internal values, according to the Domain Of Interpretation (DOI).
 * This version reflects the fact that we only support one DOI.
 */
#define category_xtoi(n,doi) (n)
#define division_xtoi(n,doi) (n)
#define level_xtoi(n,doi)    (n)

/* 
 * These pseudo functions translate category numbers from internal to
 * external values, according to the Domain Of Interpretation (DOI).
 * This version reflects the fact that we only support one DOI.
 */
#define category_itox(n,doi) (n)
#define division_itox(n,doi) (n)
#define level_itox(n,doi)    (n)

#ifdef NOTYET
extern int           if_security_invalid( ifsec_t * );
#endif
extern struct mbuf * ip_strip_security( struct mbuf * );
extern void          sort_shorts( u_short *, int );

/* 
 * These pseudo functions evaluate specific idiom characteristics.
 */
#define dacful(n)	( (n==IDIOM_SGIPSO2) || (n==IDIOM_SGIPSOD) )
#define cipso2(n)	( (n==IDIOM_SGIPSO2) || (n==IDIOM_CIPSO2) || (n==IDIOM_SGIPSO2_NO_UID) || (n==IDIOM_TT1_CIPSO2) )
#define type_1_only(n)	( (n==IDIOM_TT1) || (n==IDIOM_TT1_CIPSO2) )
#define sgipso(n)	( (n==IDIOM_SGIPSO2) || (n==IDIOM_SGIPSO2_NO_UID) || (n==IDIOM_SGIPSOD) || (n==IDIOM_SGIPSO) )
#ifdef DEBUG
#define STATIC_PRF
#else
#define STATIC_PRF static
#endif
#ifdef __cplusplus
}
#endif
