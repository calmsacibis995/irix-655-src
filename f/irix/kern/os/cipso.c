/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

#ident	"$Revision: 1.35 $"
#include <sys/types.h> 
#include <sys/mbuf.h>
#include <sys/tcp-param.h>
#include <net/if.h>
#include <net/route.h>
#include <sys/buf.h>
#include <sys/cipso.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/cred.h>
#include <sys/errno.h>
#include <sys/mac_label.h>
#include <sys/kabi.h>
#include <sys/protosw.h>
#include <sys/sat.h>
#include <sys/so_dac.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <netinet/ip_secopts.h>
#include <netinet/in.h>
#include <net/ksoioctl.h>
#include <sys/xlate.h>
#include <sys/capability.h>
#include <sys/var.h>
#include <sys/kmem.h>
#include "os/proc/pproc_private.h"	/* XXX bogus */

#include <net/if_sec.h>
#include <sys/sesmgr.h>
 
extern cipso_enabled;
extern tcp_mtudisc;
extern mac_label *mac_high_low_lp;
extern mac_label *mac_low_high_lp;

#if _MIPS_SIM == _ABI64
extern int irix5_to_ifsec_xlate(enum xlate_mode, void *, int, xlate_info_t *);
extern int ifsec_to_irix5_xlate(void *, int, xlate_info_t *);
#endif

/* Zone for cipso_proc structs */
static struct zone *cipso_proc_zone;

void
cipso_init()
{
	cipso_enabled = 1;
	tcp_mtudisc = 0;

	/* Initialize the zone for the cipso_proc structures */
	cipso_proc_zone = kmem_zone_init(sizeof(cipso_proc_t), "Cipso proc");
}

void
cipso_proc_init(struct proc * p, struct proc * parent)
{
	struct cipso_proc * old;
	struct cipso_proc * new;

	ASSERT(p->p_cipso_proc == NULL);

	new = kmem_zone_alloc(cipso_proc_zone, KM_SLEEP);

	if (parent == NULL) {
		ASSERT(p->p_pid == 0);
		new->soacl = NULL;
	} else {
		ASSERT(p != parent);

		old = parent->p_cipso_proc;
		ASSERT(old != NULL);

		if (old->soacl != NULL) {
			new->soacl = kern_malloc(sizeof (struct soacl));
			bcopy((caddr_t)old->soacl,
				(caddr_t)new->soacl,
				 sizeof(struct soacl));
		} else
			new->soacl = NULL;
	}
	p->p_cipso_proc = new;
}

void
cipso_proc_exit(struct proc * p)
{
	struct cipso_proc * cp = p->p_cipso_proc;

	if (cp->soacl != NULL) {
		kern_free(cp->soacl);
		cp->soacl = NULL;
	}
	kmem_zone_free(cipso_proc_zone, cp);
	p->p_cipso_proc = NULL;

}

void
cipso_socket_init(struct proc * p, struct socket * so)
{

	struct cipso_proc * cp = p->p_cipso_proc;
	struct sesmgr_cipso * sp;
	cred_t *cr = get_current_cred();

	ASSERT(cp != NULL);
	if (so->so_sesmgr_data == NULL) {
		sp = kern_malloc(sizeof(struct sesmgr_cipso));
		so->so_sesmgr_data = (caddr_t)sp;
	} else
		sp = (struct sesmgr_cipso *)so->so_sesmgr_data;

	sp->sm_sendlabel = sp->sm_label = cr->cr_mac;
	sp->sm_rcvuid = cr->cr_uid;
	sp->sm_uid = cr->cr_uid;
	sp->sm_soacl = (struct soacl *)mcb_get(M_WAIT, sizeof *sp->sm_soacl, MT_SOACL);
	if (cp->soacl == NULL) {
		/*
		 * If u_soacl not yet allocated, allocate
		 * and set default values before copying it.
		 */
		cp->soacl = kern_malloc(sizeof(struct soacl));
		cp->soacl->so_uidlist[0] = cr->cr_uid;
		cp->soacl->so_uidcount = 1;
	}
	bcopy(cp->soacl, sp->sm_soacl, sizeof(struct soacl));
}


/*
 * Simply print a message to the effect that CIPSO is enabled.
 */
void
cipso_confignote(void)
{
	cmn_err(CE_CONT, "Internet Protocol Security Options Enabled.\n");
}

/* Cloned from sort_shorts in ../bsd/netinet/ip_sec_if.c.
 * Sort an array of integers into ascending order.  
 * Based on Algorithm 201, ACM by J. Boothroyd (1963).
 * Also known as a "shell" sort.
 * Chosen for compactness more than speed.
 */
static void
sort_uid_list(
	uid_t * ints,
	int     count)
{
	register uid_t * p1;
	register uid_t * p2;
	register uid_t       middle;
	uid_t *          start;
	uid_t *          limit;

	middle = count;
	for (;;) {
		if ( (middle >>= 1) == 0 )
			return;
		limit = &ints[ count - middle ];
		for ( start = ints; start < limit; ++start ) {
			register int tmp1;
			register int tmp2;

			p1 = start;
			p2 = p1 + middle;	
			while ( p1 >= ints && (tmp1 = *p1) > (tmp2 = *p2) ) {
				*p1 = tmp2; *p2 = tmp1;
				p2 = p1;
				p1 -= middle;
			} 
		}
	}
	/* NOTREACHED */
}

/* 
 * If count is 0, set the ACL to users's uid only.  If count is
 * neither 0 nor WILDACL, sort the uids. Return 0 if so_uidcount is
 * bad, otherwise return 1.
 */
int
soacl_ok(struct soacl *aclp)
{
    if ( ((aclp->so_uidcount > SOACL_MAX)||(aclp->so_uidcount < 0 )) &&
	(aclp->so_uidcount != WILDACL) ) {
	return(0);
    } 
    if (!aclp->so_uidcount) {
	aclp->so_uidlist[0] = get_current_cred()->cr_uid;
	aclp->so_uidcount = 1;		
    }
    if ( (aclp->so_uidcount > 1) && (aclp->so_uidcount != WILDACL) ) 
	sort_uid_list(aclp->so_uidlist, aclp->so_uidcount);
    return(1);
}

/*
 * Return 1 if uid is Superuser, if soacl is wild, or if 
 * uid is in the soacl.  Otherwise, return 0.
 */
int 
dac_allowed(uid_t uid,
	    struct soacl *aclp)
{
    register int count;
    
    if ( (uid == 0) || (aclp->so_uidcount == WILDACL) )
	return(1);
    
    for (count = 0; count < aclp->so_uidcount; count++) {
	if ( aclp->so_uidlist[count] == uid )
	    return (1);
	if ( aclp->so_uidlist[count] > uid )
	    break;
    }
    return(0);
}

int
getpsoacl(struct soacl *usr_acl, rval_t *rvp)
{
	struct cipso_proc * cp = curprocp->p_cipso_proc;

	ASSERT(cp != NULL);

	/* If never allocated, allocate & set "default".*/
	if ( cp->soacl == NULL ) {
        	cp->soacl = kern_malloc(sizeof(struct soacl));
		cp->soacl->so_uidlist[0] = get_current_cred()->cr_uid;
		cp->soacl->so_uidcount = 1;
	}

	if ( copyout((void*)cp->soacl, (void*)usr_acl,
				sizeof(struct soacl)) ) {
		return EFAULT;
	}

	if (cp->soacl->so_uidcount == WILDACL)
		rvp->r_val1 = NULL;
	else
		rvp->r_val1 = cp->soacl->so_uidcount;

	return(0);
}

int
setpsoacl(struct soacl *usr_acl)
{
        struct cipso_proc * cp = curprocp->p_cipso_proc;
        struct soacl * tmpsoacl;

	ASSERT(cp != NULL);

        /* Copy into temp structure */
        tmpsoacl = kern_malloc(sizeof(struct soacl));
        if ( copyin((void*)usr_acl, (void*)tmpsoacl, sizeof(struct soacl)) ) {
                return EFAULT;
        }

        /* Is it ok */
        if ( !soacl_ok(tmpsoacl) ) {
                kern_free(tmpsoacl);
                return EINVAL;
        }

        /* XXX glowell: do we need second copy ? */
        /* it was never allocated */
        if ( cp->soacl == NULL )
                cp->soacl = kern_malloc(sizeof(struct soacl));

        bcopy( (caddr_t)tmpsoacl, (caddr_t)cp->soacl,
                                 sizeof (struct soacl) );
        kern_free(tmpsoacl);
        return(0);

}

int
siocgetlabel(struct socket *so, 
	     caddr_t addr) 
{ 
	struct mac_label * ml = sesmgr_get_label(so);
	return(copyout((caddr_t)ml, addr, mac_size(ml)) );
}

int
siocsetlabel(struct socket *so,
	     caddr_t addr) 
{ 
	mac_label *new_label;
	int error = 0;

	if (!_CAP_ABLE(CAP_NETWORK_MGT)) 
		error = EPERM;
	else {
		if (so->so_rcv.sb_cc) 		/* data in buffer */
			error = EISCONN;
		else {
			if ((error = mac_copyin_label((mac_label *)addr,
							&new_label)) == 0) {
				error = (*so->so_proto->pr_usrreq)(so,
						PRU_SOCKLABEL, 
						(struct mbuf *)0, 
						(struct mbuf *) new_label, 
						(struct mbuf *)0);
			}
		}
	}

	/* generate audit record, ut_scallargs is file descriptor */
	_SAT_BSDIPC_MAC_CHANGE(*curuthread->ut_scallargs, so,
				new_label, error);
	return(error);
}

int
siocgetacl(struct socket *so, 
	     caddr_t out_acl, rval_t *rvp)
{
	struct soacl * soacl = sesmgr_get_soacl(so);
	bcopy( soacl, out_acl, sizeof(struct soacl) );
	if (soacl->so_uidcount == WILDACL) 
		rvp->r_val1 = NULL;
	else
		rvp->r_val1 = soacl->so_uidcount;
	return (0);
}   

int
siocsetacl(struct socket *so, struct soacl *in_acl)
{
	int error = 0;

	if ( soacl_ok(in_acl) ) /* Sorts, returns 0 if ACL invalid. */
		sesmgr_set_soacl(so,in_acl);
	else
		error = EINVAL;
	_SAT_BSDIPC_DAC_CHANGE(*curuthread->ut_scallargs, so, error);
	return(error);
}   

int
siocgetuid(struct socket *so, uid_t *out_uidp, rval_t *rvp)
{
	*out_uidp = rvp->r_val1 = sesmgr_get_uid(so);
	return (0);
}

int
siocsetuid(struct socket *so,
	   uid_t *in_uidp)
{
	int error = 0;

	if (!_CAP_ABLE(CAP_NETWORK_MGT)) 
		error = EPERM;
	else 
		sesmgr_set_uid(so, *in_uidp);
	_SAT_BSDIPC_DAC_CHANGE(*curuthread->ut_scallargs, so, error);
	return(error);
}

int
siocgetrcvuid(struct socket *so,
uid_t *out_uidp, rval_t *rvp)
{
	*out_uidp = rvp->r_val1 = sesmgr_get_rcvuid(so);
	return (0);
}

int
siocsifuid(struct ifnet *ifp, struct ifreq *ifr)
{
	int error = 0;

	if (!_CAP_ABLE(CAP_NETWORK_MGT)) {
		error = EPERM;
		goto done;
	}

	if (ifr->ifr_len != sizeof(uid_t)) {
		error = EINVAL;
		goto done;
	}

	error = copyin(ifr->ifr_base, (void *)&ifp->if_sec->ifs_uid,
			sizeof(uid_t));
	if (error)
		goto done;

	done:
	/*
	 * generate audit record, u_ap is file descriptor
	_SAT_BSDIPC_IF_SETUID(*curprocp->p_exception->u_ap, so, ifr,
				ifp->if_sec->ifs_uid, (error ? 0 : 1) );
	 */

	return (error);
}
 
int
siocgifuid(struct ifnet *ifp, struct ifreq *ifr)
{
	if (ifr->ifr_len != sizeof(uid_t))
		return EINVAL;

	return copyout((void *)&ifp->if_sec->ifs_uid, ifr->ifr_base,
			sizeof(uid_t));
}

int
siocgiflabel(struct ifnet *ifp, 
	     struct ifreq *ifr)
{
	int error = 0;
	ifsec_t sec;
	u_char abi = get_current_abi();
	int sizeok = 0;

	/* interface may not have been initialized yet */
	if (!ifp->if_sec) {
		return (ENOTCONN);
	}
	if (!ifp->if_sec->ifs_label_min || !ifp->if_sec->ifs_label_max) {
		return (ENOTCONN);
	}

	if (!ifr->ifr_base || !ifr->ifr_len)
		return 0;

#if _MIPS_SIM == _ABI64
	switch (abi) {
	case ABI_IRIX5:
	case ABI_IRIX5_N32:
		sizeok = (ifr->ifr_len == sizeof(irix5_ifsec_t));
		break;
	case ABI_IRIX5_64:
		sizeok = (ifr->ifr_len == sizeof(ifsec_t));
		break;
	}
#else
	sizeok = (ifr->ifr_len == sizeof(ifsec_t));
#endif
	if (!sizeok)
		return EINVAL;

	error = COPYIN_XLATE(ifr->ifr_base, &sec, sizeof(ifsec_t),
				irix5_to_ifsec_xlate, abi, 1);
	if (error) {
		return error;
	}

	if ( (copyout((caddr_t)ifp->if_sec->ifs_label_max,
			(caddr_t)sec.ifs_label_max,
			mac_size(ifp->if_sec->ifs_label_max)))
	 ||  (copyout((caddr_t)ifp->if_sec->ifs_label_min,
			(caddr_t)sec.ifs_label_min,
			mac_size(ifp->if_sec->ifs_label_min))) )
		return EFAULT;

	sec.ifs_authority_max = ifp->if_sec->ifs_authority_max;
	sec.ifs_authority_min = ifp->if_sec->ifs_authority_min;
	sec.ifs_doi           = ifp->if_sec->ifs_doi;
	sec.ifs_idiom         = ifp->if_sec->ifs_idiom;

	error = XLATE_COPYOUT(&sec, ifr->ifr_base, sizeof(ifsec_t),
		ifsec_to_irix5_xlate, abi, 1);

	return(error);
}

int
siocsiflabel(struct ifnet *ifp, struct ifreq *ifr, struct socket *so)
{
	mac_label *new_label;
	int error = 0;
	ifsec_t sec;
	u_char abi = get_current_abi();
	int sizeok = 0;

	/* interface may not have been initialized yet */
	if (!ifp->if_sec) {
		ifp->if_sec = kmem_zalloc(sizeof(struct ifsec), KM_NOSLEEP);
	}

	if (!_CAP_ABLE(CAP_NETWORK_MGT)) {
		error = EPERM;
		goto done;
	}

	if (!ifr->ifr_base || !ifr->ifr_len)
		return 0;

#if _MIPS_SIM == _ABI64
	switch (abi) {
	case ABI_IRIX5:
	case ABI_IRIX5_N32:
		sizeok = (ifr->ifr_len == sizeof(irix5_ifsec_t));
		break;
	case ABI_IRIX5_64:
		sizeok = (ifr->ifr_len == sizeof(ifsec_t));
		break;
	}
#else
	sizeok = (ifr->ifr_len == sizeof(ifsec_t));
#endif
	if (!sizeok)
		return EINVAL;

	error = COPYIN_XLATE(ifr->ifr_base, &sec, sizeof(ifsec_t),
			irix5_to_ifsec_xlate, abi, 1);
	if (error)
		return error;

	if ( (error = mac_copyin_label(sec.ifs_label_max,
					&new_label)) == 0 ) {
		sec.ifs_label_max = new_label;
		if ( (error = mac_copyin_label(sec.ifs_label_min,
						&new_label)) == 0 ) {
			sec.ifs_label_min = new_label;
			/* Invoke the network policy module */

			if ( if_security_invalid(&sec) ) 
				error = EINVAL;	
			else {
				/*ifp->if_sec = sec;*//* structure assignment */
				ifp->if_sec->ifs_label_max = sec.ifs_label_max;
				ifp->if_sec->ifs_label_min = sec.ifs_label_min;
				ifp->if_sec->ifs_doi       = sec.ifs_doi; 	
				ifp->if_sec->ifs_idiom     = sec.ifs_idiom;
				/* flush the label cache */
				m_freem(ifp->if_sec->ifs_lbl_cache);
				ifp->if_sec->ifs_lbl_cache = (struct mbuf *)0;
			}
		}	    
	}

	if (!error)
		error = XLATE_COPYOUT(&sec, ifr->ifr_base, sizeof(ifsec_t),
					ifsec_to_irix5_xlate, abi, 1);
done:

	/* generate audit record, u_ap is file descriptor */
	_SAT_BSDIPC_IF_SETLABEL(*curuthread->ut_scallargs, so,
				ifr, error );
	return (error);
}

void
set_lo_secattr(struct ifnet *ifp)
{
	/* interface may not have been initialized yet */
	if (!ifp->if_sec) {
		ifp->if_sec = kmem_zalloc(sizeof(struct ifsec), KM_NOSLEEP);
	}

	ifp->if_sec->ifs_label_max = mac_high_low_lp;
	ifp->if_sec->ifs_label_min = mac_low_high_lp;
	ifp->if_sec->ifs_doi       = 3; 	
	ifp->if_sec->ifs_idiom     = IDIOM_SGIPSO2;
}

void
svr4_tcl_endpt_init(tcl_endpt_t	* te) 
{
	struct cipso_proc * cp = curprocp->p_cipso_proc;
	cred_t *cr = get_current_cred();

	ASSERT(cp != NULL);

	te->te_soacl = kern_malloc(sizeof(struct soacl));
	ASSERT(te->te_soacl != NULL);
	if (cp->soacl == NULL) {       
		/* 
		 * If u_soacl not yet allocated, allocate 
		 * and set default values before copying it.
		 */
		cp->soacl = kern_malloc(sizeof(struct soacl));
		cp->soacl->so_uidlist[0] = cr->cr_uid;
		cp->soacl->so_uidcount = 1;
	}
	bcopy(cp->soacl, te->te_soacl, sizeof(struct soacl));
	te->te_souid = cr->cr_uid;
	te->te_mac_label = cr->cr_mac;
}

void
svr4_tco_endpt_init(tco_endpt_t	* te) 
{
	struct cipso_proc * cp = curprocp->p_cipso_proc;
	cred_t *cr = get_current_cred();

	ASSERT(cp != NULL);

	te->te_soacl = kern_malloc(sizeof(struct soacl));
	ASSERT(te->te_soacl != NULL);
	if (cp->soacl == NULL) {       
		/* 
		 * If u_soacl not yet allocated, allocate 
		 * and set default values before copying it.
		 */
		cp->soacl = kern_malloc(sizeof(struct soacl));
		cp->soacl->so_uidlist[0] = cr->cr_uid;
		cp->soacl->so_uidcount = 1;
	}
	bcopy(cp->soacl, te->te_soacl, sizeof(struct soacl));
	te->te_souid = cr->cr_uid;
	te->te_mac_label = cr->cr_mac;
}

void
svr4_tcoo_endpt_init(tcoo_endpt_t * te) 
{
	struct cipso_proc * cp = curprocp->p_cipso_proc;
	cred_t *cr = get_current_cred();

	ASSERT(cp != NULL);

	te->te_soacl = kern_malloc(sizeof(struct soacl));
	ASSERT(te->te_soacl != NULL);
	if (cp->soacl == NULL) {       
		/* 
		 * If u_soacl not yet allocated, allocate 
		 * and set default values before copying it.
		 */
		cp->soacl = kern_malloc(sizeof(struct soacl));
		cp->soacl->so_uidlist[0] = cr->cr_uid;
		cp->soacl->so_uidcount = 1;
	}
	bcopy(cp->soacl, te->te_soacl, sizeof(struct soacl));
	te->te_souid = cr->cr_uid;
	te->te_mac_label = cr->cr_mac;
}

/*
 *  Place holder for recvlumsg until it is implemented under
 *  the new session manager.
 */
/*ARGSUSED*/
int
recvlumsg(struct recvlumsga *a, rval_t *rvp)
{
	rvp->r_val1 = 0;
	return 0;
}

/*ARGSUSED*/
int
if_security_invalid(ifsec_t *sec)
{
	return 0;
}
