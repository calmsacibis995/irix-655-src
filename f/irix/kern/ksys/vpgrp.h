/**************************************************************************
 *									  *
 * 		 Copyright (C) 1995-1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

#ident "$Id: vpgrp.h,v 1.26 1999/05/14 20:13:13 lord Exp $"

#ifndef _KSYS_VPGRP_H_
#define _KSYS_VPGRP_H_

#include <ksys/behavior.h>
#include <ksys/kqueue.h>
#include <sys/space.h>

/*
 * Vpgrp structure.
 */
typedef struct vpgrp {
	kqueue_t	vpg_queue;		/* hash queue link */
	pid_t		vpg_pgid;		/* vpgrp pgrp identifier */
	pid_t		vpg_sid;		/* vsession identifier */
	int		vpg_refcnt;		/* vpgrp reference count */
	lock_t		vpg_refcnt_lock;	/* lock for ref. count */
	bhv_head_t	vpg_bhvh;		/* Behavior head */
} vpgrp_t;

#define VPRGP_NULL	((vpgrp_t *)0)

#define	VPGRP_LOOKUP(pgid)	vpgrp_lookup(pgid)
#define	VPGRP_CREATE(pgid, sid)	vpgrp_create(pgid, sid)
#define VPGRP_HOLD(v)		vpgrp_ref(v)
#define VPGRP_RELE(v)		vpgrp_unref(v)

extern void	vpgrp_init(void);
extern vpgrp_t	*vpgrp_lookup(pid_t);
extern vpgrp_t	*vpgrp_create(pid_t, pid_t);
extern void	vpgrp_ref(vpgrp_t *);
extern void	vpgrp_unref(vpgrp_t *);

struct proc;
struct pgrp;
struct cred;
struct k_siginfo;

typedef __uint32_t sequence_num_t;

typedef struct pgrp_ops {
	bhv_position_t	vpr_position;	/* position within behavior chain */

	void (*pgrp_getattr)
		(bhv_desc_t *b,		/* behavior */
		 pid_t *sid,		/* returns session id */
		 int *is_orphaned,	/* returns orphan status */
		 int *is_batch);	/* returns whether batch group */

	int  (*pgrp_join_begin)
		(bhv_desc_t *b);	/* behavior */

	void (*pgrp_join_end)
		(bhv_desc_t *b,		/* behavior */
		 struct proc *p,	/* process */
		 int attach);		/* contributes job control */

	void (*pgrp_leave)
		(bhv_desc_t *b,		/* behavior */
		 struct proc *p,	/* process */
		 int exitting);		/* is process exiting */

	void (*pgrp_detach)
		(bhv_desc_t *b,		/* behavior */
		 struct proc *p);	/* member process with parent exitting */

	void (*pgrp_linkage)
		(bhv_desc_t *b,		/* behavior */
		 struct proc *p,	/* member process with parent exitting */
		 pid_t ppgid,		/* parent's new pgrp id */
		 pid_t psid);		/* parent's new session id */

	int (*pgrp_sendsig)
		(bhv_desc_t *b,		/* behavior */
		 int sig,		/* signal number */
		 int options,		/* signaling options */
		 pid_t sid,		/* caller's session id */
		 struct cred *,		/* credentials of caller */
		 struct k_siginfo *);	/* signal info */

	sequence_num_t (*pgrp_sigseq)
		(bhv_desc_t *b);	/* behavior */

	int (*pgrp_sig_wait)
		(bhv_desc_t *b,		/* behavior */
		sequence_num_t seq);	/* signal sequence number */

	int (*pgrp_setbatch)
		(bhv_desc_t *b); 	/* set bit to batch */	
	int (*pgrp_clearbatch)
		(bhv_desc_t *b);	/* clear batch */

	int (*pgrp_nice)
		(bhv_desc_t *b,		/* behavior */
		 int flags,		/* SET or GET priority */
		 int *nice,		/* nice value to set */
		 int *cnt,		/* count of processes visited */
		 struct cred *);	/* Credentials of caller */

	/*
	 * The following operations are intended for internal use and should
	 * not be called above the vop interface.
	 */
	void (*pgrp_orphan)
		(bhv_desc_t *b,		/* behavior */
		 int is_orphan,		/* local orphan state of pgrp */
		 int is_exit);		/* orphan due process exit */

	void (*pgrp_anystop)
		(bhv_desc_t *b, 	/* behavior */
		 pid_t pid,		/* process id to be excluded */
		 int *is_stopped);	/* returns whether any member stopped*/

	void (*pgrp_membership)
		(bhv_desc_t *b, 	/* behavior */
		 int memcnt,		/* num member to be (only 0/1 used) */
		 pid_t sid);		/* session id */

	void (*pgrp_destroy)
		(bhv_desc_t *b); 	/* behavior */
} pgrp_ops_t;

/*
 * Signaling options for VPGRP_SENDSIG.
 */
#define VPG_SENDCONT	SIG_HUPCONT	/* SIGHUP/SIGCONT sequence */

/*
 * Flags for VPGRP_NICE:
 */
#define VPG_SET_NICE	0x1	/* Set nice val. of group */
#define VPG_GET_NICE	0x2	/* Get nice val. of group */

#define	vpg_fbhv	vpg_bhvh.bh_first	/* 1st behavior */
#define	vpg_ops		vpg_fbhv->bd_ops	/* ops for 1st behavior */
#define _VPGOP_(op, v)	(*((pgrp_ops_t *)(v)->vpg_ops)->op)

#define VPGRP_GETATTR(v, _sid, _is_orphaned, _is_b) \
	{ \
	_VPGOP_(pgrp_getattr, v) ((v)->vpg_fbhv, _sid, _is_orphaned, _is_b); \
	}

static __inline int
VPGRP_JOIN_BEGIN(
	vpgrp_t		*v)
{
	int	rv;
	rv = _VPGOP_(pgrp_join_begin, v) ((v)->vpg_fbhv);
	return rv;
}

#define VPGRP_JOIN_END(v, _p, _attach) \
	{ \
	_VPGOP_(pgrp_join_end, v) ((v)->vpg_fbhv, _p, _attach); \
	}

#define VPGRP_JOIN(v, _p, _attach) \
	{ \
	int	rv; \
	rv = _VPGOP_(pgrp_join_begin, v) ((v)->vpg_fbhv); \
	ASSERT(!rv); \
	_VPGOP_(pgrp_join_end, v) ((v)->vpg_fbhv, _p, _attach); \
	}

#define VPGRP_LEAVE(v, _p, _exiting) \
	{ \
	_VPGOP_(pgrp_leave, v) ((v)->vpg_fbhv, _p, _exiting); \
	}

#define VPGRP_DETACH(v, _p) \
	{ \
	_VPGOP_(pgrp_detach, v) ((v)->vpg_fbhv,  _p); \
	}

#define VPGRP_LINKAGE(v, _p, _ppgid, _psid) \
	{ \
	_VPGOP_(pgrp_linkage, v) ((v)->vpg_fbhv, _p, _ppgid, _psid); \
	}

#define VPGRP_ORPHAN(v, _state, _is_exit) \
	{ \
	_VPGOP_(pgrp_orphan, v) ((v)->vpg_fbhv, _state, _is_exit); \
	}

#define VPGRP_ANYSTOP(v, _pid, _is_stopped) \
	{ \
	_VPGOP_(pgrp_anystop, v) ((v)->vpg_fbhv, _pid, _is_stopped); \
	}

static __inline int
VPGRP_SENDSIG(
	vpgrp_t			*v,
	int			sig,
	int			options,
	pid_t			sid,
	struct cred		*credp,
	struct k_siginfo	*info)
{
	int	rv = 0;
	if (sig > 0) {
		rv = _VPGOP_(pgrp_sendsig, v)
				((v)->vpg_fbhv, sig, options, sid, credp, info);
	}
	return rv;
}

static __inline sequence_num_t
VPGRP_SIGSEQ(vpgrp_t *v)
{
	int	rv = 0;
	if (v != NULL) {
		rv = _VPGOP_(pgrp_sigseq, v) ((v)->vpg_fbhv);
	}
	return rv;
}

static __inline int
VPGRP_SIG_WAIT(vpgrp_t *v, int seq)
{
	int	rv;
	rv = _VPGOP_(pgrp_sig_wait, v) ((v)->vpg_fbhv, seq);
	return rv;
}

#define VPGRP_MEMBERSHIP(v, _memcnt, _sid) \
	{ \
	_VPGOP_(pgrp_membership, v) ((v)->vpg_fbhv, _memcnt, _sid); \
	}

#define VPGRP_DESTROY(v) \
	_VPGOP_(pgrp_destroy, v) ((v)->vpg_fbhv)

#define VPGRP_NICE(v, _flags, _nice, _cnt, _cred, _ret) \
	{ \
	_ret = _VPGOP_(pgrp_nice, v) ((v)->vpg_fbhv, _flags, _nice, _cnt, _cred); \
	}

static __inline int
VPGRP_SETBATCH(vpgrp_t *v)
{
	int rv;
	rv = _VPGOP_(pgrp_setbatch, v) ((v)->vpg_fbhv);
	return rv;
}

#define VPGRP_CLEARBATCH(v) \
	{ \
	_VPGOP_(pgrp_clearbatch, v) ((v)->vpg_fbhv); \
	}

#endif	/* _KSYS_VPGRP_H_ */
