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

#ifndef __T6RHDB_H__
#define __T6RHDB_H__

#ifdef __cplusplus
extern "C" {
#endif

#ident	"$Revision: 1.2 $"

#include <limits.h>		/* For NGROUPS_MAX */
#include <netinet/in.h>		/* For struct in_addr */

/*
 * Session Manager IDs.  Identifies the protocol used
 * to communicate with a host.
 */
typedef enum t6rhdb_smm_id {
	T6RHDB_SMM_INVALID=0,
	T6RHDB_SMM_SINGLE_LEVEL,
	T6RHDB_SMM_MSIX_1_0,
	T6RHDB_SMM_MSIX_2_0,
	T6RHDB_SMM_TSIX_1_0,
	T6RHDB_SMM_TSIX_1_1
} t6rhdb_smm_id_t;

/*
 *  IP Security Options.  NLM is a historical
 *  reference to the TSIX spec.
 */
typedef enum t6rhdb_nlm_id {
	T6RHDB_NLM_INVALID=0,
	T6RHDB_NLM_UNLABELED,
	T6RHDB_NLM_CIPSO_1,
	T6RHDB_NLM_CIPSO_2,
	T6RHDB_NLM_CIPSO_3,
	T6RHDB_NLM_CIPSO_5,
	T6RHDB_NLM_RIPSO_BSO,
	T6RHDB_NLM_RIPSO_ESO,
	T6RHDB_NLM_SGIPSO,
	T6RHDB_NLM_SGIPSO2
} t6rhdb_nlm_id_t;

/*
 *  Flags.  Indicates which attributes are mandatory
 *  on packets received from a host.
 */
typedef enum t6rhdb_flag_id {
	T6RHDB_FLG_INVALID=0,
	T6RHDB_FLG_IMPORT,
	T6RHDB_FLG_EXPORT,
	T6RHDB_FLG_DENY_ACCESS,
	T6RHDB_FLG_MAND_SL,
	T6RHDB_FLG_MAND_INTEG,
	T6RHDB_FLG_MAND_ILB,
	T6RHDB_FLG_MAND_PRIVS,
	T6RHDB_FLG_MAND_LUID,
	T6RHDB_FLG_MAND_IDS,
	T6RHDB_FLG_MAND_SID,
	T6RHDB_FLG_MAND_PID,
	T6RHDB_FLG_MAND_CLEARANCE
} t6rhdb_flag_id_t;

#define T6RHDB_MASK(value)          ((unsigned int) (1<<(value)))

/*
 *  Vendor ID for tweaking the protocol to
 *  account for different interpretations of 
 *  the TSIX spec.
 */
typedef enum t6rhdb_vendor_id {
	T6RHDB_VENDOR_INVALID=0,
	T6RHDB_VENDOR_UNKNOWN,
	T6RHDB_VENDOR_SUN,
	T6RHDB_VENDOR_HP,
	T6RHDB_VENDOR_IBM,
	T6RHDB_VENDOR_CRAY,
	T6RHDB_VENDOR_DG,
	T6RHDB_VENDOR_HARRIS
} t6rhdb_vendor_id_t;

/*
 *  Buffer for returning statistics about the
 *  remote host data base.
 */
typedef struct t6rhdb_rstat {
	int host_cnt;
	int host_size;
	int profile_cnt;
	int profile_size;
} t6rhdb_rstat_t;

#if defined(_KERNEL)
#include <sys/hashing.h>

/*
 * This structure is the in-kernel representation
 * of a t6rhdb_host_buf_t.
 */
typedef struct t6rhdb_kern_buf {
	int		hp_smm_type;
	int		hp_nlm_type;
	int		hp_auth_type;
	int		hp_encrypt_type;
	int 		hp_attributes;
	int		hp_flags;
	int		hp_cache_size;
	int		hp_host_cnt;
	cap_set_t	hp_def_priv;
	cap_set_t	hp_max_priv;
	mac_b_label *	hp_def_sl;
	mac_b_label *	hp_min_sl;
	mac_b_label *	hp_max_sl;
	mac_b_label *	hp_def_integ;
	mac_b_label *	hp_min_integ;
	mac_b_label *	hp_max_integ;
	mac_b_label *	hp_def_ilb;
	mac_b_label *	hp_def_clearance;
	uid_t		hp_def_sid;		/* Session ID */
	uid_t		hp_def_uid;
	uid_t		hp_def_luid;		/* Login or Audit ID */
	gid_t		hp_def_gid;
	int		hp_def_grp_cnt;
	gid_t		hp_def_groups[NGROUPS_MAX];
} t6rhdb_kern_buf_t;

/*
 *  For fast lookup, host addresses are stored in a small hash
 *  table with a linked list to resolve collisions.
 */
#define T6RHDB_TABLE_SIZE	256
#define T6RHDB_REFCNT_MASK	0x7fffffff
#define T6RHDB_REFCNT_BAD	0x80000000

typedef struct t6rhdb_host_hash {
	struct hashbucket		hh_hash;
	t6rhdb_kern_buf_t		hh_profile;
	struct in_addr			hh_addr;
	int				hh_flags;
	unsigned int			hh_refcnt;
	lock_t				hh_lock;
} t6rhdb_host_hash_t;
#endif

/*
 *  This structure is used to pass a host security profile
 *  into the kernel.  The structure is followed by a list
 *  of the applicable IP addresses, the sensitivity ranges and the
 *  integrity label ranges.
 */
typedef struct t6rhdb_host_buf {
	int		hp_smm_type;
	int		hp_nlm_type;
	int		hp_auth_type;
	int		hp_encrypt_type;
	int 		hp_attributes;
	int		hp_flags;
	int		hp_cache_size;	
	int		hp_host_cnt;
	cap_set_t	hp_max_priv;
	mac_b_label	hp_def_sl;
	mac_b_label	hp_min_sl;
	mac_b_label	hp_max_sl;
	mac_b_label	hp_def_integ;
	mac_b_label	hp_min_integ;
	mac_b_label	hp_max_integ;
	mac_b_label	hp_def_ilb;
	mac_b_label	hp_def_clearance;
	uid_t		hp_def_sid;		/* Session ID */
	uid_t		hp_def_uid;
	uid_t		hp_def_luid;		/* Login or Audit ID */
	gid_t		hp_def_gid;
	int		hp_def_grp_cnt;
	gid_t		hp_def_groups[NGROUPS_MAX];
	cap_set_t	hp_def_priv;
} t6rhdb_host_buf_t;

/*
 *  Library Interfaces.
 */
#if defined(_KERNEL)
extern int t6rhdb_put_host(size_t, caddr_t, rval_t *);
extern int t6rhdb_get_host(struct in_addr *, size_t, caddr_t, rval_t *);
extern int t6rhdb_stat(caddr_t data);
extern int t6rhdb_flush(struct in_addr *, rval_t *);
extern void t6rhdb_init(void);
extern int t6findhost(struct in_addr *, int, t6rhdb_kern_buf_t *);
#else
int t6rhdb_put_host(size_t, caddr_t);
int t6rhdb_get_host(struct in_addr *, size_t, caddr_t);
int t6rhdb_stat(caddr_t);
int t6rhdb_flush(const struct in_addr *);
#endif

#ifdef __cplusplus
}
#endif

#endif	/* __T6RHDB_H__ */
