/**************************************************************************
 *									  *
 * 		 Copyright (C) 1994-1997 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ifndef __SM_PRIVATE_H__
#define __SM_PRIVATE_H__

#ident "$Id: sm_private.h,v 1.4 1997/10/04 03:37:45 bitbug Exp $"

#include <sys/acl.h>
#include <sys/mac.h>
#include <sys/capability.h>
#include <sys/t6attrs.h>
#include <sys/vsocket.h>
/***
#include <sys/t6samp.h>
#include <sys/sesmgr_samp.h>
****/


#ifdef _KERNEL

/*
 *  This structure contains the default attributes that may be
 *  set on an interface, host, or socket.  The TSIX attributes
 *  that are not currently supported by Trusted Irix are not
 *  present in the structure.
 */
typedef struct t6default_attrs {
	t6mask_t	dflt_attrs;	/* Mask indicates which attrs present */
	msen_t		dflt_msen;
	mint_t		dflt_mint;
	uid_t		dflt_sid;
	msen_t		dflt_clearance; 
	acl_t		dflt_acl;
	cap_t		dflt_privs;
	uid_t		dflt_audit_id;
	uid_t		dflt_uid;
	gid_t		dflt_gid;
	t6groups_t *	dflt_groups;
} t6default_attrs_t;

/*
 *  Each structure containing session manager attributes begins with
 *  int32 type indicating the set of attributes described.
 */
#define T6IF_ATTR	2	/* Interface ATTRS */
#define T6RH_ATTR	3	/* Remote Host Attributes */
#define T6SO_ATTR	4	/* Socket Attributes */

/* Flags */
#define IPSOPT_NONE	0
#define IPSOPT_BSO	0x01
#define IPSOPT_ESO	0x02
#define IPSOPT_CIPSO	0x04
#define IPSOPT_IPOUT	0x80		/* IP Options Only, free when done */

/* Atrributes Mask */
#define IPSOPT_MSEN	0x01;
#define IPSOPT_MINT	0x02;
#define IPSOPT_UID	0x04;

/* 
 * Structure for holding samp ids.
 */
typedef struct t6ids {
	uid_t		uid;
	gid_t		gid;
	t6groups_t	groups;
} t6ids_t;

typedef struct soattr {
	t6mask_t	sa_mask;
	msen_t		sa_msen;
	mint_t		sa_mint;
	uid_t		sa_sid;
	msen_t		sa_clearance;
	cap_set_t	sa_privs;
	uid_t		sa_audit_id;
	t6ids_t		sa_ids;
} soattr_t;

typedef struct ipsec {
	/* XXX following should be header struct */
	int			sm_protocol_id;
	
	/* Session Manager Flags */
	int			sm_cap_net_mgt;	/* indicates privileged
						   process */
	int			sm_new_attr;
	u_int			sm_samp_seq;
	t6mask_t		sm_mask;	/* endpoint-default mask */
	size_t			sm_samp_cnt;	/* number of bytes till next */
						/* samp header (TCP)         */

	/* Cipso compatibility stuff */
	mac_t			sm_label;	/* dominates all data
						   rcvd on socket */
	mac_t			sm_sendlabel;	/* label sent by udp_output */
	struct soacl *		sm_soacl;	/* ACL on socket */
	uid_t			sm_uid;		/* uid sent */
	uid_t			sm_rcvuid;	/* uid received - set by
						   sonewconn */

	/* IP Security Options */
	short			sm_ip_flags;
	short			sm_ip_mask;	/* currently unused */
	mac_t			sm_ip_lbl;
	uid_t			sm_ip_uid;
	struct ifnet *		sm_ip_ifp;

	/*
	 * This set of attributes can be set by processes
	 * with appropriate privilege and are sent, on a
	 * per-message basis, instead of the corresponding
	 * default attribute or process attribute.
	 */
	soattr_t		sm_msg;

	/*
	 * Default Attributes.  These attributes can be set by 
	 * processes with appropriate privilege and are sent instead
	 * of the corresponding process attribute.
	 */
	soattr_t		sm_dflt;

	/*
	 *  This set of attributes are the composite of the received
	 *  attributes and any defaults specified for the interface or
	 *  for the remote host.
	 */
	soattr_t		sm_rcv;

	/*
	 *  This set of attributes are the last sent attributes.  A copy
	 *  is saved so that any changes in process attributes since the
	 *  last send can be detected and only the changed attribute sent.
	 */
	soattr_t		sm_snd;		/* last sent message attrs */

	/* data to be sent when connection is established */
	struct mbuf *		sm_conn_data;
} ipsec_t;

/*
 * This structure holds the security attributes of a network
 * interface.  For short term compatiblity the original SGIPSO2
 * attributes are maintained as well.
 */
typedef struct t6if_attrs {
	int			ifs_type;	/* Attribute type = T6IF_ATTR */
	t6mask_t		ifs_mand_attrs;
	msen_t			ifs_min_msen;
	msen_t  		ifs_max_msen;
	mint_t			ifs_min_mint;
	mint_t  		ifs_max_mint;
	t6default_attrs_t	ifs_defaults;

	/*  The following fields are duplicated from the old structure
	 *  until the cipso code is changed to use the fields.
	 */
	mac_label * ifs_label_max;	/* dominates all dgrams on if   */
	mac_label * ifs_label_min;	/* dominated by all if dgrams 	*/
	__uint32_t  ifs_doi;		/* domain of interpretation	*/
	u_char	    ifs_authority_max;	/* maximum authority allowed	*/
	u_char	    ifs_authority_min;	/* minimum authority permitted	*/
	u_char	    ifs_reserved;	/* must be zero until defined	*/
	u_char	    ifs_idiom;		/* security idiom (see below)	*/
	struct mbuf *ifs_lbl_cache;	/* label mapping cache	*/
	uid_t 	ifs_uid;		/* default uid for dacless if */
} t6if_attrs_t;

int sesmgr_getsock(int fd, struct socket **sop, struct vsocket **);

#endif  /* _KERNEL */

#endif	/* __SM_PRIVATE_H__ */
