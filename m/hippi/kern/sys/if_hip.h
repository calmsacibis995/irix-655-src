/*
 * HIPPI board, if_hip network interface.
 *
 * Copyright 1994 Silicon Graphics, Inc.  All rights reserved.
 *
 */

#ifndef __IF_HIP_H__
#define __IF_HIP_H__

#ifndef __NETINET_IN_H__
#include "netinet/in.h"
#include "sys/socket.h"
#endif

#ifndef	FALSE
#   define	FALSE	0
#endif

#ifndef	TRUE
#   define	TRUE	!(FALSE)
#endif


#ident "$Revision: 1.14 $"

#define IFHIP_NAME		"hip"		/* interface name */
#define IFHIP_DEFAULT_MTU	65280		/* See RFC 1374 */

/*
 * TCP window sizes
 */

#define HIPPI_DEFAULT_SENDSPACE	512*1024
#define HIPPI_DEFAULT_RECVSPACE	512*1024

/*
 * Debugging suport Macros and defines
 */

#ifdef IFHIP_DEBUG
extern int ifhip_debug;
/* trace/debug printing based on ifhip_debug ONLY */
#define dprintf(lvl, x)	   if (ifhip_debug>=lvl) { printf x; }

/* trace/debug printing based on ifhip_debug AND the interface dbug flag */
#define IFDPRINTF(ihi, x)  if (0!=((ihi->if_flags&IFF_DEBUG)||ifhip_debug>0)) { printf x;}
#define IFDEBUG(ihi)	   (0!=((ihi)->hi_if.if_flags&IFF_DEBUG)||ifhip_debug>0)
#else   /* NO IFHIP_DEBUG */
#define IFDEBUG(ihi)	   (0!=((ihi)->hi_if.if_flags&IFF_DEBUG))
#define IFDPRINTF(ihi, x)  if (0!=(ihi->if_flags&IFF_DEBUG)) { printf x;}
#define dprintf(lvl, x)
#endif	/* IFHIP_DEBUG */

#define NOCHKSUM_SUMOFF 0xFFFF /* sumoff to use when we don't want the FW to
				* to insert the checksum anywhere
				*/

#define HIPPI_IP_SIZE   4 /* sizeof( struct in_addr)  = 4 */
#define HIPPI_ULA_SIZE  6
#define HIPPI_SWADDR_S  3
#define HIPPI_IFLD_SIZE 4

/******************************************** 
 *   ---- Data structures definitions  ----
 ********************************************/
typedef unsigned char hippi_ula_t[HIPPI_ULA_SIZE];

typedef struct hippi_le {
	u_char fcwm;		/* Forwarding class, Width, M_Type */
#define HLE_FC_MASK	0xE0
#define HLE_FC_SHIFT	5
#define HLE_W		0x10
#define HLE_MT_MASK	0x0F
#define HLE_MT_SHIFT	0
 /* HIPPI-LE Spec: section "6.1.1 LE_Header" */
#define MAT_DATA		0	/* data carrying PDU */
#define MAT_ARP_REQUEST		1	/* HIPPI ARP request */
#define MAT_ARP_REPLY		2	/* HIPPI ARP response */
#define MAT_S_REQUEST		3	/* switch address discovery request */
#define MAT_S_RESPONSE		4	/* switch address discovery response */
/* RFC 1374:  5-11 Reserved by ANSI comittee, 12-15 Locally Assigned  */
	u_char	dest_swaddr[3];	/* dest switch address */
	u_char	swat;		/* switch address types */
#define HLE_DAT_MASK	0xF0
#define HLE_DAT_SHIFT	4
#define HLE_SAT_MASK	0x0F
#define HLE_SAT_SHIFT	0

	u_char	src_swaddr[3];	/* source switch address */
	u_short	resvd;
	hippi_ula_t	dest;		/* Destination IEEE address */
	u_short	local_admin;
	hippi_ula_t	src;		/* Source IEEE address */
} hippi_le_t;

/* RFC 1374: IEEE 802.2 LLC- The IEEE 802.2 LLC Header shall begin in 
 *      the firstoctet of the HIPPI-FP D2_Area.
 *   SSAP (8 bits) = 170 (dec).  DSAP (8 bits) = 170 (decimal).
 *   CTL (8 bits)  = 3 (Unnumbered Information).
 */

typedef struct hippi_snap {
	u_char	ssap, dsap;		/* XXX: check these */
	u_char	type;
	u_char	org[3];
	u_short	ethertype;
} hippi_snap_t;


/* Size of header passed to snoop_input() */
#define IFHIP_SNHDR_LEN	(sizeof(hippi_i_t) \
			 +sizeof(struct hippi_fp) \
			 +sizeof(struct hippi_le))


typedef struct I_fplesnap {
  hippi_i_t         I;		/* I-field to use */
  struct hippi_fp   fp;         /* ulp_id, flags, d1d2off, d2size    */
  struct hippi_le   le;         /* fcwm,dest-,src_sw_addr,dula,lcl_admin,s_ula */
  struct hippi_snap snap;
} I_fplesnap_t;

typedef struct fplesnap {
  struct hippi_fp   fp;         /* ulp_id, flags, d1d2off, d2size    */
  struct hippi_le   le;         /* fcwm,dest-,src_sw_addr,dula,lcl_admin,s_ula */
  struct hippi_snap snap;
} fplesnap_t;

#define HIPPI_CCI = 0x07
#define HIPPI_CONST_CCI = 0x01
#define HIPPI_IMASK = 0xFFFFFF

typedef struct hip_address {
  union I_field {
	hippi_i_t	I;         /* 32 bit (4 byte) I-field to use */
    struct partial_i {
	  u_char        cci;       /* Connection control info see HIPPI-SC */  
	  u_char        swaddr[3]; /* switch address aprt of the ifield*/
	} fields;
  } i_field;
#define hi_I    i_field.I
#define fields  i_field.fields

  hippi_ula_t	        ula;	   /* 48 bit (6 byte) ULA */
} hip_address_t;

/* HIPPI ARP table entry.
 */
typedef struct harptab {
	struct in_addr	ht_iaddr;	/* internet address */
	hippi_i_t	ht_I;		/* I-field to use */
	hippi_ula_t	ht_ula;		/* ULA */
	u_char		ht_flags;	/* flags */
} harptab_t;

#define NULLADDR_TYPE   0               /* No address specified. */
#define HARPTAB_SIZE	211		/* max entries, prime number */

#define HTF_PERM	0x01		/* a permanent entry */
#define HTF_SRCROUTE	0x02		/* using source routing */

/* HIPPI ARP request structure.
 * These are used with the interface-specific ioctl's to
 * manipulate HIPPI ARP tables.
 */
struct harpreq {
	struct sockaddr	harp_pa;	/* protocol address */
	struct sockaddr	harp_ula;	/* hardware ula */
	unsigned int	harp_swaddr;	/* switch address */
	u_char		harp_flags;
};

/* These, unlike ARP requests, are interface specific.
 * You must have a raw socket attached to the interface.
 */
#define SIOCSHARP	_IOW('P',  1, struct harpreq)	/* set entry */
#define SIOCGHARP	_IOWR('P', 2, struct harpreq)	/* get entry */
#define SIOCDHARP	_IOW('P',  3, struct harpreq)	/* delete entry */
#define SIOCGHARPTBL	_IO('P',  4)			/* get entire table */

#ifndef XIO_HIPPI	/* these defines moved to hippi_firm.h 
			   to be shared by 4640 fw. */
#define HIP_MAX_BIG	128		/* clusters in pool */
#define HIP_MAX_SML	64		/* little mbufs in pool */
#define HIP_BIG_SIZE	NBPP		/* size of big mbufs */
#define HIP_SML_SIZE	MLEN

#define IFHIP_MAX_OUTQ 	50         	/* max output queue length. */
/* max mbuf's chained on xmit */
#define IFHIP_MAX_MBUF_CHAIN	((131072/NBPP)-2)
#define IFHIP_OMBUF_ALIGN	7	/* mbuf tail addr align mask */

#else /* XIO_HIPPI */

#define IFHIP_OMBUF_ALIGN	3	/* mbuf tail addr align mask */

#endif /* XIO */

#ifdef _KERNEL

/* Per device variables for if_hip interfaces
 */
typedef struct ifhip_vars {
	struct ifnet	hi_if;		/* struct ifnet */
	struct rawif	hi_rawif;	/* raw interface */

        hip_address_t   hi_haddr;       /* Save this cards hippi address */

#ifdef XIO_HIPPI
	hippi_vars_t	*hps_devp;
	mutex_t		hi_mbuf_mutex;	/* if_hip */
#endif
	struct mbuf	*hi_in_smls[HIP_MAX_SML];
	int		hi_in_sml_num;
	int		hi_in_sml_h;
	int		hi_in_sml_t;
	struct mbuf	*hi_in_bigs[HIP_MAX_BIG];
	int		hi_in_big_num;
	int		hi_in_big_h;
	int		hi_in_big_t;
} ifhip_vars_t;

#ifndef XIO_HIPPI
extern ifhip_vars_t ifhip_device[];
#endif
extern int 	ifhip_cksum;		/* from master.d/if_hip */
extern int 	ifhip_mtusize;		/* from master.d/if_hip */

extern lock_t	ifhip_harplock;		/* spin-lock for I-field table */
extern harptab_t harptab[];		/* IP to I-field table */

/* Prototypes for HIPPI-LE layer hooks */

void	ifhip_attach( hippi_vars_t * );
void	ifhip_le_odone( hippi_vars_t *, volatile struct hip_d2b_hd *, int );
int	ifhip_fillin( hippi_vars_t * );
void	ifhip_le_input( hippi_vars_t *, volatile struct hip_b2h * );
void	ifhip_shutdown( hippi_vars_t * );

#endif /* _KERNEL */

#endif /* __IF_HIP_H__ */
