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
 *  Routines to maintain security attributes on the
 *  network interface.
 */

#ident	"$Revision: 1.4 $"
#include <bstring.h>
#include <sys/types.h> 
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/xlate.h>
#include <net/if.h>

#include <sys/mac.h>
#include <sys/mac_label.h>
#include <sys/sat.h>
#include "sys/sesmgr_if.h"
#include "sm_private.h"

extern mac_label *mac_high_low_lp;
extern mac_label *mac_low_high_lp;

/**************************************************************************
 *  Routines for labeling network interfaces.
 **************************************************************************/

#if _MIPS_SIM == _ABI64

/*ARGSUSED*/
static t6ifreq_t *
irix5_to_t6ifreq(t6ifreq_t *user_ifr, t6ifreq_t *native_ifr)
{
	irix5_t6ifreq_t *i5_ifr;
	char abi = get_current_abi();

	if (ABI_IS_IRIX5_64(abi))
		return user_ifr;

	ASSERT(ABI_IS_IRIX5(abi) || ABI_IS_IRIX5_N32(abi));

	i5_ifr = (irix5_t6ifreq_t *)user_ifr;

	bcopy(i5_ifr->ifsec_name, native_ifr->ifsec_name,
		sizeof(native_ifr->ifsec_name));
	native_ifr->ifsec_mand_attrs = i5_ifr->ifsec_mand_attrs;
	native_ifr->ifsec_min_msen = (msen_t)(__psunsigned_t)
		i5_ifr->ifsec_min_msen;
	native_ifr->ifsec_max_msen = (msen_t)(__psunsigned_t)
		i5_ifr->ifsec_max_msen;
	native_ifr->ifsec_min_mint = (mint_t)(__psunsigned_t)
		i5_ifr->ifsec_min_mint;
	native_ifr->ifsec_max_mint = (mint_t)(__psunsigned_t)
		i5_ifr->ifsec_max_mint;

	native_ifr->ifsec_dflt.dflt_attrs =
		i5_ifr->ifsec_dflt.dflt_attrs;
	native_ifr->ifsec_dflt.dflt_msen = (msen_t)(__psunsigned_t)
		i5_ifr->ifsec_dflt.dflt_msen;
	native_ifr->ifsec_dflt.dflt_mint = (mint_t)(__psunsigned_t)
		i5_ifr->ifsec_dflt.dflt_mint;
	native_ifr->ifsec_dflt.dflt_sid =
		i5_ifr->ifsec_dflt.dflt_sid;
	native_ifr->ifsec_dflt.dflt_clearance = (msen_t)(__psunsigned_t)
		i5_ifr->ifsec_dflt.dflt_clearance;
	native_ifr->ifsec_dflt.dflt_acl = (acl_t)(__psunsigned_t)
		i5_ifr->ifsec_dflt.dflt_acl;
	native_ifr->ifsec_dflt.dflt_privs = (cap_t)(__psunsigned_t)
		i5_ifr->ifsec_dflt.dflt_privs;
	native_ifr->ifsec_dflt.dflt_audit_id =
		i5_ifr->ifsec_dflt.dflt_audit_id;
	native_ifr->ifsec_dflt.dflt_uid =
		i5_ifr->ifsec_dflt.dflt_uid;
	native_ifr->ifsec_dflt.dflt_groups = (t6groups_t *)(__psunsigned_t)
		i5_ifr->ifsec_dflt.dflt_groups;
	native_ifr->ifsec_dflt.dflt_gid =
		i5_ifr->ifsec_dflt.dflt_gid;

	return native_ifr;
}

static t6ifreq_t *
t6ifreq_to_irix5(t6ifreq_t *native_ifr, t6ifreq_t *user_ifr)
{
	irix5_t6ifreq_t *i5_ifr;
	int abi = get_current_abi();

	if (ABI_IS_IRIX5_64(abi))
		return native_ifr;

	ASSERT(ABI_IS_IRIX5(abi) || ABI_IS_IRIX5_N32(abi));

	i5_ifr = (irix5_t6ifreq_t *)user_ifr;

	bcopy(native_ifr->ifsec_name, i5_ifr->ifsec_name,
		sizeof(i5_ifr->ifsec_name));
	i5_ifr->ifsec_mand_attrs =
		native_ifr->ifsec_mand_attrs;
	i5_ifr->ifsec_min_msen = (app32_ptr_t)(__psunsigned_t)
		native_ifr->ifsec_min_msen;
	i5_ifr->ifsec_max_msen = (app32_ptr_t)(__psunsigned_t)
		native_ifr->ifsec_max_msen;
	i5_ifr->ifsec_min_mint = (app32_ptr_t)(__psunsigned_t)
		native_ifr->ifsec_min_mint;
	i5_ifr->ifsec_max_mint = (app32_ptr_t)(__psunsigned_t)
		native_ifr->ifsec_max_mint;

	i5_ifr->ifsec_dflt.dflt_attrs =
		native_ifr->ifsec_dflt.dflt_attrs;
	i5_ifr->ifsec_dflt.dflt_msen = (app32_ptr_t)(__psunsigned_t)
		native_ifr->ifsec_dflt.dflt_msen;
	i5_ifr->ifsec_dflt.dflt_mint = (app32_ptr_t)(__psunsigned_t)
		native_ifr->ifsec_dflt.dflt_mint;
	i5_ifr->ifsec_dflt.dflt_sid =
		native_ifr->ifsec_dflt.dflt_sid;
	i5_ifr->ifsec_dflt.dflt_clearance = (app32_ptr_t)(__psunsigned_t)
		native_ifr->ifsec_dflt.dflt_clearance;
	i5_ifr->ifsec_dflt.dflt_acl = (app32_ptr_t)(__psunsigned_t)
		native_ifr->ifsec_dflt.dflt_acl;
	i5_ifr->ifsec_dflt.dflt_privs = (app32_ptr_t)(__psunsigned_t)
		native_ifr->ifsec_dflt.dflt_privs;
	i5_ifr->ifsec_dflt.dflt_audit_id =
		native_ifr->ifsec_dflt.dflt_audit_id;
	i5_ifr->ifsec_dflt.dflt_uid =
		native_ifr->ifsec_dflt.dflt_uid;
	i5_ifr->ifsec_dflt.dflt_groups = (app32_ptr_t)(__psunsigned_t)
		native_ifr->ifsec_dflt.dflt_groups;
	i5_ifr->ifsec_dflt.dflt_gid =
		native_ifr->ifsec_dflt.dflt_gid;

	return user_ifr;
}

#endif /* _MIPS_SIM == _ABI64) */

int
sesmgr_siocgiflabel(struct ifnet *ifp, caddr_t data)
{
	int		error = 0;
	t6mask_t	defaults;
	t6if_attrs_t	*if_attrs;
	t6default_attrs_t *dif;

        t6ifreq_t *t6ifr = (t6ifreq_t *)data;
#if _MIPS_SIM == _ABI64
        t6ifreq_t native_t6ifr;
#endif

	/* Interface may not have been initialized yet */
	if ((if_attrs = ifp->if_sec) == NULL) {
		bzero(t6ifr, sizeof(t6ifr));
		error = ENOTCONN;
		goto done;
	}

	XLATE_FROM_IRIX5(irix5_to_t6ifreq, t6ifr, &native_t6ifr);

	/* Mandatory Attributes List */
	t6ifr->ifsec_mand_attrs = if_attrs->ifs_mand_attrs;

	/* Minimum Sensitivity Label. */
	if (if_attrs->ifs_min_msen == NULL)
		t6ifr->ifsec_min_msen = NULL;
	else {
		ASSERT(msen_valid(if_attrs->ifs_min_msen));
		if (copyout(if_attrs->ifs_min_msen, t6ifr->ifsec_min_msen,
				msen_size(if_attrs->ifs_min_msen))) {
			error = EFAULT;
			goto done;
		}
	}

	/* Maximum Sensitivity Label. */
	if (if_attrs->ifs_max_msen == NULL)
		t6ifr->ifsec_max_msen = NULL;
	else {
		ASSERT(msen_valid(if_attrs->ifs_min_msen));
		if (copyout(if_attrs->ifs_max_msen, t6ifr->ifsec_max_msen,
				msen_size(if_attrs->ifs_max_msen))) {
			error = EFAULT;
			goto done;
		}
	}

	/* Minimum Integrity Label. */
	if (if_attrs->ifs_min_mint == NULL)
		t6ifr->ifsec_min_mint = NULL;
	else {
		ASSERT(mint_valid(if_attrs->ifs_min_mint));
		if (copyout(if_attrs->ifs_min_mint, t6ifr->ifsec_min_mint,
				mint_size(if_attrs->ifs_min_mint))) {
			error = EFAULT;
			goto done;
		}
	}

	/* Maximum Integrity Label. */
	if (if_attrs->ifs_max_mint == NULL)
		t6ifr->ifsec_max_mint = NULL;
	else {
		ASSERT(mint_valid(if_attrs->ifs_max_mint));
		if (copyout(if_attrs->ifs_max_mint, t6ifr->ifsec_max_mint,
				mint_size(if_attrs->ifs_max_mint))) {
			error = EFAULT;
			goto done;
		}
	}

	/* Any defaults */
	defaults = t6ifr->ifsec_dflt.dflt_attrs =
		if_attrs->ifs_defaults.dflt_attrs;
	if (defaults == T6M_NO_ATTRS)
		goto done;
	dif  = &if_attrs->ifs_defaults;

	/* Default Sensitivity Label */
	if (defaults & T6M_SL) {
		ASSERT(msen_valid(dif->dflt_msen));
		if (copyout(dif->dflt_msen, t6ifr->ifsec_dflt.dflt_msen,
				msen_size(dif->dflt_msen)) != 0) {
			error = EFAULT;
			goto done;
		}
	}

	/* Default Integrity Label */
	if (defaults & T6M_INTEG_LABEL) {
		ASSERT(mint_valid(dif->dflt_mint));
		if (copyout(dif->dflt_mint, t6ifr->ifsec_dflt.dflt_mint,
				mint_size(dif->dflt_mint)) != 0) {
			error = EFAULT;
			goto done;
		}
	}

	/* Default Clearance */
	if (defaults & T6M_CLEARANCE) {
		ASSERT(msen_valid(dif->dflt_clearance));
		if (copyout(dif->dflt_clearance, t6ifr->ifsec_dflt.dflt_clearance,
				msen_size(dif->dflt_clearance)) != 0) {
			error = EFAULT;
			goto done;
		}
	}

	/* Default ACL */
	if (defaults & T6M_ACL) {
		ASSERT(dif->dflt_acl != NULL);
		if (copyout(dif->dflt_acl, t6ifr->ifsec_dflt.dflt_acl,
				sizeof(struct acl)) != 0) {
			error = EFAULT;
			goto done;
		}
	}

	/* Default Privs */
	if (defaults & T6M_PRIVILEGES) {
		ASSERT(dif->dflt_privs != NULL);
		if (copyout(dif->dflt_privs, t6ifr->ifsec_dflt.dflt_privs,
				sizeof(struct cap_set)) != 0) {
			error = EFAULT;
			goto done;
		}
	}

	/* Default Session ID */
	if (defaults & T6M_SESSION_ID) {
		t6ifr->ifsec_dflt.dflt_sid = dif->dflt_sid;
	}

	/* Default Audit ID */
	if (defaults & T6M_AUDIT_ID) {
		t6ifr->ifsec_dflt.dflt_audit_id = dif->dflt_audit_id;
	}

	/* Default UID */
	if (defaults & T6M_UID) {
		t6ifr->ifsec_dflt.dflt_uid = dif->dflt_uid;
	}

	/* Default GID */
	if (defaults & T6M_GID) {
		t6ifr->ifsec_dflt.dflt_gid = dif->dflt_gid;
	}

	/* Default Groups */
	if (defaults & T6M_GROUPS) {
		ASSERT(dif->dflt_groups != NULL);
		if (copyout(dif->dflt_groups, t6ifr->ifsec_dflt.dflt_groups,
			    sizeof(t6groups_t)) != 0) {
			error = EFAULT;
			goto done;
		}
	}

done:
	XLATE_TO_IRIX5(t6ifreq_to_irix5, (t6ifreq_t *)data, t6ifr);
	return(error);
}

/*ARGSUSED*/
sesmgr_siocsiflabel(struct ifnet *ifp, caddr_t data)
{
	int error = 0;
	t6if_attrs_t *if_attrs;
	msen_t tmp_msen;
	mint_t tmp_mint;
	t6mask_t defaults;
	t6default_attrs_t *dif;
        t6ifreq_t *t6ifr = (t6ifreq_t *)data;
#if _MIPS_SIM == _ABI64
        t6ifreq_t native_t6ifr;
#endif

	/* Do we have appropriate privilege */
	if (!_CAP_ABLE(CAP_NETWORK_MGT)) {
		error = EPERM;
		goto done;
	}

	XLATE_FROM_IRIX5(irix5_to_t6ifreq, t6ifr, &native_t6ifr);

	/* interface may not have been initialized yet */
	if (!ifp->if_sec) {
		if_attrs = kmem_zalloc(sizeof(t6if_attrs_t), KM_NOSLEEP);
		ifp->if_sec = if_attrs;
		ASSERT(ifp->if_sec != NULL);
	} else
		if_attrs = ifp->if_sec;

	/* Mandatory Attributes List */
	if_attrs->ifs_mand_attrs = t6ifr->ifsec_mand_attrs;

	/* Minimum Sensitivity Label */
	if (t6ifr->ifsec_min_msen != NULL) {
		if ((error = msen_copyin_label(
				t6ifr->ifsec_min_msen, &tmp_msen)) != 0)
			goto done;
		if_attrs->ifs_min_msen = tmp_msen;
	}

	/* Maximum Sensitivity Label */
	if (t6ifr->ifsec_max_msen != NULL) {
		if ((error = msen_copyin_label(
				t6ifr->ifsec_max_msen, &tmp_msen)) != 0)
			goto done;
		if_attrs->ifs_max_msen = tmp_msen;
	}

	/* Minimum Integrity Label */
	if (t6ifr->ifsec_min_mint != NULL) {
		if ((error = mint_copyin_label(
				t6ifr->ifsec_min_mint, &tmp_mint)) != 0)
			goto done;
		if_attrs->ifs_min_mint = tmp_mint;
	}

	/* Maximum Integrity Label */
	if (t6ifr->ifsec_max_mint != NULL) {
		if ((error = mint_copyin_label(
				t6ifr->ifsec_max_mint, &tmp_mint)) != 0)
			goto done;
		if_attrs->ifs_max_mint = tmp_mint;
	}

	/* Compatibility Fields */
	if_attrs->ifs_label_max = mac_from_msen_mint(
					if_attrs->ifs_max_msen,
					if_attrs->ifs_max_mint);
	if_attrs->ifs_label_min = mac_from_msen_mint(
					if_attrs->ifs_min_msen,
					if_attrs->ifs_min_mint);
	if_attrs->ifs_doi       = 3; 	
	if_attrs->ifs_idiom     = IDIOM_SGIPSO2;
	if_attrs->ifs_lbl_cache = (struct mbuf *)0;

	/* Any defaults */
	if ((defaults = t6ifr->ifsec_dflt.dflt_attrs) == T6M_NO_ATTRS)
		goto done;
	if_attrs->ifs_defaults.dflt_attrs = defaults;
	dif  = &if_attrs->ifs_defaults;

	/* Default Sensitivity Label */
	if (defaults & T6M_SL) {
		if (error = msen_copyin_label(t6ifr->ifsec_dflt.dflt_msen, &tmp_msen))
			goto done;
		dif->dflt_msen = tmp_msen;
	}

	/* Default Integrity Label */
	if (defaults & T6M_INTEG_LABEL) {
		if (error = mint_copyin_label(t6ifr->ifsec_dflt.dflt_mint, &tmp_mint))
			goto done;
		dif->dflt_mint = tmp_mint;
	}

	/* Default Clearance */
	if (defaults & T6M_CLEARANCE) {
		if (error = msen_copyin_label(t6ifr->ifsec_dflt.dflt_clearance, &tmp_msen))
			goto done;
		dif->dflt_clearance = tmp_msen;
	}

	/* Default ACL */
	if (defaults & T6M_ACL) {
		struct acl *tmp_acl = kern_malloc(sizeof(*tmp_acl));
		if (copyin((caddr_t) t6ifr->ifsec_dflt.dflt_acl, (caddr_t) tmp_acl,
			   sizeof(*tmp_acl))) {
			error = EFAULT;
			goto done;
		}
		dif->dflt_acl = tmp_acl;
	}

	/* Default Privs */
	if (defaults & T6M_PRIVILEGES) {
		struct cap_set *tmp_cap = kern_malloc(sizeof(*tmp_cap));
		if (copyin((caddr_t) t6ifr->ifsec_dflt.dflt_privs, (caddr_t) tmp_cap,
			   sizeof(*tmp_cap))) {
			error = EFAULT;
			goto done;
		}
		dif->dflt_privs = tmp_cap;
	}

	/* Default Session ID */
	if (defaults & T6M_SESSION_ID) {
		dif->dflt_sid = t6ifr->ifsec_dflt.dflt_sid;
	}

	/* Default Audit ID */
	if (defaults & T6M_AUDIT_ID) {
		dif->dflt_audit_id = t6ifr->ifsec_dflt.dflt_audit_id;
	}

	/* Default UID */
	if (defaults & T6M_UID) {
		dif->dflt_uid = t6ifr->ifsec_dflt.dflt_uid;
		/* XXX remove when compatibility not needed */
		if_attrs->ifs_uid = t6ifr->ifsec_dflt.dflt_uid;
	}

	/* Default GID */
	if (defaults & T6M_GID) {
		dif->dflt_gid = t6ifr->ifsec_dflt.dflt_gid;
	}

	/* Default Groups */
	if (defaults & T6M_GROUPS) {
		t6groups_t *grp = kmem_alloc(sizeof(*grp), KM_NOSLEEP);
		if (copyin(t6ifr->ifsec_dflt.dflt_groups, grp, sizeof(*grp))) {
			error = EFAULT;
			goto done;
		}
		dif->dflt_groups = grp;
	}

done:
	/* generate audit record, *u.u_ap is file descriptor */
	/*_SAT_BSDIPC_IF_SETLABEL( *u.u_ap, so, ifr, error );*/ /* SAT_XXX */
	return (error);
}

void
set_lo_secattr(struct ifnet *ifp)
{
	t6if_attrs_t *if_attrs;

	/* Interface may not have been initialized yet */
	if (!ifp->if_sec) {
		if_attrs = kmem_zalloc(sizeof(t6if_attrs_t), KM_NOSLEEP);
		ifp->if_sec = if_attrs;
	} else
		if_attrs = ifp->if_sec;

	if_attrs->ifs_min_msen = msen_from_mac(mac_low_high_lp);
	if_attrs->ifs_min_mint = mint_from_mac(mac_low_high_lp);

	if_attrs->ifs_max_msen = msen_from_mac(mac_high_low_lp);
	if_attrs->ifs_max_mint = mint_from_mac(mac_high_low_lp);

	if_attrs->ifs_label_max = mac_high_low_lp;
	if_attrs->ifs_label_min = mac_low_high_lp;
	if_attrs->ifs_doi       = 3; 	
	if_attrs->ifs_idiom     = IDIOM_SGIPSO2;
	if_attrs->ifs_lbl_cache = (struct mbuf *) 0;
}

/**************************************************************************
 *  Policy Routines
 **************************************************************************/

/*ARGSUSED*/
int
sesmgr_if_range_check(struct ifnet *ifp, msen_t *msen_lbl)
{
	return 0;
}

/**************************************************************************
 *  Access routines.
 **************************************************************************/

/*ARGSUSED*/
int
sesmgr_if_get_msen(struct ifnet *ifp, msen_t *msen_lbl)
{
	return 0;
}

/*ARGSUSED*/
int
sesmgr_if_set_msen(struct ifnet *ifp, msen_t msen_lbl)
{
	return 0;
}

/*ARGSUSED*/
int
sesmgr_if_get_mint(struct ifnet *ifp, mint_t *mint_lbl)
{
	return 0;
}

/* ARGSUSED*/
int
sesmgr_if_set_mint(struct ifnet *ifp, mint_t mint_lbl)
{
	return 0;
}
