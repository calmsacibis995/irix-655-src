/**************************************************************************
 *									  *
 * 		 Copyright (C) 1995, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/* 
 *  Routines to maintain IP security attributes.
 */

#ident	"$Revision: 1.3 $"
#include <bstring.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/tcp-param.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/tcpipstats.h>

#include <sys/sat.h>
#include <sys/mac.h>
#include <sys/mac_label.h>
#include <sys/sesmgr.h>
#include <sys/capability.h>
#include <sys/t6rhdb.h>
#include <sys/t6satmp.h>
#include "sesmgr_if.h"
#include "sm_private.h"

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>

extern mac_label * ip_recvlabel(struct mbuf *, struct ifnet *, uid_t *);
extern struct mbuf *ip_xmitlabel(struct ifnet *, struct mbuf *, mac_label *,
	uid_t, int *);

int
sesmgr_ip_options (struct mbuf *m, struct ifnet *ifp, struct ipsec **attrs)
{
	register struct ip *ip = mtod (m, struct ip *);
	register u_char *cp;
	int opt;
	int optlen;
	int cnt;
	struct ipsec *ip_attrs;

	*attrs = NULL;

	/* Drop everything until interface labeled */
	ASSERT (ifp != NULL);
	if (ifp->if_sec == NULL) {
		return 1;
	}

	/* Allocate a buffer to hold the security attributes */
	ip_attrs = sesmgr_soattr_alloc (M_DONTWAIT);
	if (ip_attrs == NULL) {
		return 1;
	}
	ip_attrs->sm_ip_ifp = ifp;

	/* Parse the IP Security Options. */
	cp = (u_char *) (ip + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);

	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;

		if (opt == IPOPT_NOP)
			optlen = 1;
		else
		{
			optlen = cp[IPOPT_OLEN];
			if (optlen <= 0 || optlen > cnt)
				goto bad;
		}

		switch (opt)
		{
			default:
				break;

			case IPOPT_SECURITY:	/* RIPSO BSO RFC 1108 */
				ip_attrs->sm_ip_flags |= IPSOPT_BSO;
				break;

			case IPOPT_ESO:	/* RIPSO ESO RFC 1108 */
				ip_attrs->sm_ip_flags |= IPSOPT_ESO;
				break;

			case IPOPT_CIPSO:	/* CIPSO and SGI extensions */
				ip_attrs->sm_ip_flags |= IPSOPT_CIPSO;
				break;
		}
	}

	if (ip_attrs->sm_ip_flags) {
		mac_t lbl;
		uid_t uid;

		/* XXX glowell need to check if unlabeled packets are *
		 * permitted on this interface */
		lbl = ip_recvlabel (m, ifp, &uid);
		if (lbl == NULL) {
			sesmgr_soattr_free (ip_attrs);
			return 1;
		}
		ASSERT (!mac_invalid (lbl));
		ip_attrs->sm_ip_lbl = lbl;
		ip_attrs->sm_ip_uid = uid;
	} else {
		/* No options, use defaults on interface */
		/* XXX should use rhost as well */
		ip_attrs->sm_ip_uid = ifp->if_sec->ifs_defaults.dflt_uid;
		ip_attrs->sm_ip_lbl = mac_from_msen_mint (
					ifp->if_sec->ifs_defaults.dflt_msen,
					ifp->if_sec->ifs_defaults.dflt_mint);
		if (ip_attrs->sm_ip_lbl == NULL) {
			sesmgr_soattr_free (ip_attrs);
			return 1;
		}
		ip_attrs->sm_ip_flags |= IPSOPT_CIPSO;
	}

	/* XXX security policy here */
	*attrs = ip_attrs;
	return 0;
bad:
	/* We silently drop bad packets.  The spec calls for optionally
	 * returning ICMP errors. */
	/* XXX Audit record.  Bad or Missing IP security options */
	IPSTAT (ips_badoptions);
	sesmgr_soattr_free (ip_attrs);
	return 1;
}

struct mbuf *
sesmgr_ip_output (struct ifnet *ifp, struct mbuf *m, struct ipsec *ipsec,
		  struct sockaddr_in *nhost, int *error)
{
	struct ip *ip;
	t6rhdb_kern_buf_t hsec;	/* Destination Host */
	t6rhdb_kern_buf_t nsec;	/* Next Hop Host */
	uid_t ip_uid;
	mac_t ip_lbl;

	ASSERT (ifp != NULL);
	ASSERT (m != NULL);
	ASSERT (error != NULL);

	/* Check for a labeled interface */
	if (ifp->if_sec == NULL) {
		*error = ENETUNREACH;
		if (ipsec != NULL && (ipsec->sm_ip_flags & IPSOPT_IPOUT))
			sesmgr_soattr_free (ipsec);
		return NULL;
	}

	ASSERT (ifp->if_sec != NULL);
	ASSERT (ifp->if_sec->ifs_label_max != NULL);

	/* Lookup destination host */
	ip = mtod (m, struct ip *);
	ASSERT (ip != NULL);
	if (!t6findhost (&ip->ip_dst, 0, &hsec)) {
		*error = ENETUNREACH;
		if (ipsec && (ipsec->sm_ip_flags & IPSOPT_IPOUT))
			sesmgr_soattr_free (ipsec);
		return NULL;
	}
	if (hsec.hp_smm_type == T6RHDB_SMM_TSIX_1_1) {
		if (sesmgr_satmpd_started () == 0) {
			*error = ENETUNREACH;
			if (ipsec && (ipsec->sm_ip_flags & IPSOPT_IPOUT))
				sesmgr_soattr_free (ipsec);
			return NULL;
		}
	}


	/* Lookup next hop host */
	if (nhost->sin_addr.s_addr == ip->ip_dst.s_addr)
		nsec = hsec;
	else {
		if (!t6findhost (&nhost->sin_addr, 0, &nsec)) {
			*error = ENETUNREACH;
			if (ipsec && (ipsec->sm_ip_flags & IPSOPT_IPOUT))
				sesmgr_soattr_free (ipsec);
			return NULL;
		}
	}

	/* 
	 *  We only skip the ip options iff both the destination host and  
	 *  the next hop host are un-labeled.
	 */
	if (hsec.hp_nlm_type == T6RHDB_NLM_UNLABELED &&
	    nsec.hp_nlm_type == T6RHDB_NLM_UNLABELED) {
		if (ipsec && (ipsec->sm_ip_flags & IPSOPT_IPOUT))
			sesmgr_soattr_free (ipsec);
		return m;
	}

	if (ipsec != NULL) {
		/* Use Label that was passed in.  If it's an in-transit
		 * packet, free options when done */
		if (ipsec->sm_ip_flags & IPSOPT_CIPSO) {
			ASSERT (!mac_invalid (ipsec->sm_ip_lbl));
			ip_uid = ipsec->sm_ip_uid;
			ip_lbl = ipsec->sm_ip_lbl;
		} else {
			soattr_t *sa_msg = &ipsec->sm_msg;
			soattr_t *sa_dflt = &ipsec->sm_dflt;

			if ((sa_msg->sa_mask & T6M_SL) &&
			    (sa_msg->sa_mask & T6M_INTEG_LABEL)) {
				ip_lbl = mac_from_msen_mint (sa_msg->sa_msen,
							     sa_msg->sa_mint);
				ASSERT (!mac_invalid (ip_lbl));
			} else if ((sa_dflt->sa_mask & T6M_SL) &&
				   (sa_dflt->sa_mask & T6M_INTEG_LABEL)) {
				ip_lbl = mac_from_msen_mint (sa_dflt->sa_msen,
							     sa_dflt->sa_mint);
				ASSERT (!mac_invalid (ip_lbl));
			} else {
				ASSERT (!mac_invalid (ipsec->sm_sendlabel));
				ip_lbl = ipsec->sm_sendlabel;
			}

			if (sa_msg->sa_mask & T6M_UID)
				ip_uid = sa_msg->sa_ids.uid;
			else if (sa_dflt->sa_mask & T6M_UID)
				ip_uid = sa_dflt->sa_ids.uid;
			else
				ip_uid = ipsec->sm_uid;
		}
		if (ipsec->sm_ip_flags & IPSOPT_IPOUT)
			sesmgr_soattr_free (ipsec);
		ASSERT (!mac_invalid (ip_lbl));
	} else {
		/* No options, use defaults on interface */
		/* XXX should use rhost as well */
		ip_uid = ifp->if_sec->ifs_defaults.dflt_uid;
		ip_lbl = mac_from_msen_mint (
					ifp->if_sec->ifs_defaults.dflt_msen,
					ifp->if_sec->ifs_defaults.dflt_mint);
		if (ip_lbl == NULL) {
			return NULL;
		}
		ASSERT (!mac_invalid (ip_lbl));
	}

	ASSERT (!mac_invalid (ip_lbl));
	ip_lbl = mac_add_label (ip_lbl);	/* XXX redundant ??? */
	return ip_xmitlabel (ifp, m, ip_lbl, ip_uid, error);
}

#if 0
static int
dac_allowed(uid_t uid, struct soacl *aclp)
{
	int count;
    
	/*
	 * XXX the following test for root needs to be replaced
	 * by a test for CAP_DAC_OVERRIDE at some point.
	 */
	if (uid == 0)
		return(1);

	if (aclp == NULL || aclp->so_uidcount == WILDACL)
		return(1);
    
	for (count = 0; count < aclp->so_uidcount; count++) {
		if (aclp->so_uidlist[count] == uid)
			return (1);
		if (aclp->so_uidlist[count] > uid)
			break;
	}
	return(0);
}
#endif

int
sesmgr_ip_check (struct socket *so, struct ipsec *ipsec)
{
	int ip_ok = 0;
	struct ipsec *soattr = so->so_sesmgr_data;

	ASSERT(soattr != NULL);
	ASSERT(ipsec != NULL);

	/* if process is unprivileged */
	if (soattr->sm_cap_net_mgt != 1) {
		/* check MAC */
		if (mac_invalid(soattr->sm_label) ||
		    mac_invalid(ipsec->sm_ip_lbl))
			goto done;
		if (!mac_equ(soattr->sm_label, ipsec->sm_ip_lbl))
			goto done;

#if 0
		/* check DAC */
		ip_ok = dac_allowed(ipsec->sm_ip_uid, soattr->sm_soacl);
		if (ip_ok == 0)
			goto done;
#endif
	}

	/* success */
	ip_ok = 1;
	soattr->sm_ip_flags = (ipsec->sm_ip_flags & ~IPSOPT_IPOUT);
	soattr->sm_ip_mask = ipsec->sm_ip_mask;
	soattr->sm_ip_lbl = ipsec->sm_ip_lbl;
	soattr->sm_ip_uid = ipsec->sm_ip_uid;
done:
	return (!ip_ok);
}
