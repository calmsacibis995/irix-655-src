/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *  1.37 88/02/08 
 */


/*
 * rpc_prot.c
 *
 * This set of routines implements the rpc message definition,
 * its serializer and some common rpc utility routines.
 * The routines are meant for various implementations of rpc -
 * they are NOT for the rpc client or rpc service implementations!
 * Because authentication stuff is easy and is part of rpc, the opaque
 * routines are also in this program.
 */

#ifdef _KERNEL
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "types.h"
#include "xdr.h"
#include "auth.h"
#include "clnt.h"
#include "rpc_msg.h"
#include "netinet/in.h"	/* for htonl, nested in IXDR_PUT_LONG */

#else
#ifdef __STDC__
	#pragma weak xdr_opaque_auth = _xdr_opaque_auth
	#pragma weak xdr_des_block = _xdr_des_block
	#pragma weak xdr_accepted_reply = _xdr_accepted_reply
	#pragma weak xdr_rejected_reply = _xdr_rejected_reply
	#pragma weak xdr_replymsg = _xdr_replymsg
	#pragma weak xdr_callhdr = _xdr_callhdr
#endif
#include "synonyms.h"
#include <sys/param.h>
#include <rpc/rpc.h>
#endif

static void	accepted(enum accept_stat, struct rpc_err *);
static void	rejected(enum reject_stat, struct rpc_err *);

/* * * * * * * * * * * * * * XDR Authentication * * * * * * * * * * * */

/*
 * XDR an opaque authentication struct
 * (see auth.h)
 */
bool_t
xdr_opaque_auth(register XDR *xdrs, register struct opaque_auth *ap)
{

	if (xdr_enum(xdrs, &(ap->oa_flavor)))
		return (xdr_bytes(xdrs, &ap->oa_base,
			&ap->oa_length, MAX_AUTH_BYTES));
	return (FALSE);
}

/*
 * XDR a DES block
 */
bool_t
xdr_des_block(register XDR *xdrs, register des_block *blkp)
{
	return (xdr_opaque(xdrs, (caddr_t)blkp, sizeof(des_block)));
}

/* * * * * * * * * * * * * * XDR RPC MESSAGE * * * * * * * * * * * * * * * */

/*
 * XDR the MSG_ACCEPTED part of a reply message union
 */
bool_t 
xdr_accepted_reply(register XDR *xdrs, register struct accepted_reply *ar)
{

	/* personalized union, rather than calling xdr_union */
	if (! xdr_opaque_auth(xdrs, &(ar->ar_verf)))
		return (FALSE);
	if (! xdr_enum(xdrs, (enum_t *)&(ar->ar_stat)))
		return (FALSE);
	switch (ar->ar_stat) {

	case SUCCESS:
		return ((*(bool_t (*)(XDR *, char *))
			(ar->ar_results.proc))(xdrs, ar->ar_results.where));
	
	case PROG_MISMATCH:
		if (! xdr_u_long(xdrs, &(ar->ar_vers.low)))
			return (FALSE);
		return (xdr_u_long(xdrs, &(ar->ar_vers.high)));
	}
	return (TRUE);  /* TRUE => open ended set of problems */
}

/*
 * XDR the MSG_DENIED part of a reply message union
 */
bool_t 
xdr_rejected_reply(register XDR *xdrs, register struct rejected_reply *rr)
{

	/* personalized union, rather than calling xdr_union */
	if (! xdr_enum(xdrs, (enum_t *)&(rr->rj_stat)))
		return (FALSE);
	switch (rr->rj_stat) {

	case RPC_MISMATCH:
		if (! xdr_u_long(xdrs, &(rr->rj_vers.low)))
			return (FALSE);
		return (xdr_u_long(xdrs, &(rr->rj_vers.high)));

	case AUTH_ERROR:
		return (xdr_enum(xdrs, (enum_t *)&(rr->rj_why)));
	}
	return (FALSE);
}

static struct xdr_discrim reply_dscrm[3] = {
	{ (int)MSG_ACCEPTED, (xdrproc_t) xdr_accepted_reply },
	{ (int)MSG_DENIED, (xdrproc_t) xdr_rejected_reply },
	{ __dontcare__, NULL_xdrproc_t } };

/*
 * XDR a reply message
 */
bool_t
xdr_replymsg(register XDR *xdrs, register struct rpc_msg *rmsg)
{
#ifdef _KERNEL
/* trade space for time in the kernel only */
	register xdr_long_t *buf;
	register struct accepted_reply *ar;
	register struct opaque_auth *oa;

	if (xdrs->x_op == XDR_ENCODE &&
	    rmsg->rm_reply.rp_stat == MSG_ACCEPTED &&
	    rmsg->rm_direction == REPLY &&
	    (buf = XDR_INLINE(xdrs, 6 * BYTES_PER_XDR_UNIT +
	    rmsg->rm_reply.rp_acpt.ar_verf.oa_length)) != NULL) {
		IXDR_PUT_LONG(buf, rmsg->rm_xid);
		IXDR_PUT_ENUM(buf, rmsg->rm_direction);
		IXDR_PUT_ENUM(buf, rmsg->rm_reply.rp_stat);
		ar = &rmsg->rm_reply.rp_acpt;
		oa = &ar->ar_verf;
		IXDR_PUT_ENUM(buf, oa->oa_flavor);
		IXDR_PUT_LONG(buf, oa->oa_length);
		if (oa->oa_length) {
			bcopy(oa->oa_base, (caddr_t)buf, oa->oa_length);
			buf += (oa->oa_length +
				BYTES_PER_XDR_UNIT - 1) /
				sizeof (xdr_long_t);
		}
		/*
		 * stat and rest of reply, copied from xdr_accepted_reply
		 */
		IXDR_PUT_ENUM(buf, ar->ar_stat);
		switch (ar->ar_stat) {

		case SUCCESS:
			return ((*(bool_t (*)(XDR *, char *))
			    (ar->ar_results.proc))(xdrs, ar->ar_results.where));
	
		case PROG_MISMATCH:
			if (! xdr_u_long(xdrs, &(ar->ar_vers.low)))
				return (FALSE);
			return (xdr_u_long(xdrs, &(ar->ar_vers.high)));
		}
		return (TRUE);
	}
	if (xdrs->x_op == XDR_DECODE &&
	    (buf = XDR_INLINE(xdrs, 3 * BYTES_PER_XDR_UNIT)) != NULL) {
		rmsg->rm_xid = IXDR_GET_LONG(buf);
		rmsg->rm_direction = IXDR_GET_ENUM(buf, enum msg_type);
		if (rmsg->rm_direction != REPLY) {
			return (FALSE);
		}
		rmsg->rm_reply.rp_stat = IXDR_GET_ENUM(buf, enum reply_stat);
		if (rmsg->rm_reply.rp_stat != MSG_ACCEPTED) {
			if (rmsg->rm_reply.rp_stat == MSG_DENIED) {
				return (xdr_rejected_reply(xdrs,
					&rmsg->rm_reply.rp_rjct));
			}
			return (FALSE);
		}
		ar = &rmsg->rm_reply.rp_acpt;
		oa = &ar->ar_verf;
		buf = XDR_INLINE(xdrs, 2 * BYTES_PER_XDR_UNIT);
		if (buf != NULL) {
			oa->oa_flavor = IXDR_GET_ENUM(buf, enum_t);
			oa->oa_length = IXDR_GET_LONG(buf);
		} else {
			if (xdr_enum(xdrs, &oa->oa_flavor) == FALSE ||
			    xdr_u_int(xdrs, &oa->oa_length) == FALSE) {
				return (FALSE);
			}
		}
		if (oa->oa_length) {
			if (oa->oa_length > MAX_AUTH_BYTES) {
				return (FALSE);
			}
			if (oa->oa_base == NULL) {
				oa->oa_base = (caddr_t)
					mem_alloc(oa->oa_length);
			}
			buf = XDR_INLINE(xdrs, RNDUP(oa->oa_length));
			if (buf == NULL) {
				if (xdr_opaque(xdrs, oa->oa_base,
				    oa->oa_length) == FALSE) {
					return (FALSE);
				}
			} else {
				bcopy((caddr_t)buf, oa->oa_base, oa->oa_length);
			}
		}
		/*
		 * stat and rest of reply, copied from
		 * xdr_accepted_reply
		 */
		if (! xdr_enum(xdrs, (enum_t *)&ar->ar_stat)) {
			return(FALSE);
		}
		switch (ar->ar_stat) {

		case SUCCESS:
			return ((*(bool_t (*)(XDR *, char *))
			    (ar->ar_results.proc))(xdrs, ar->ar_results.where));

		case PROG_MISMATCH:
			if (! xdr_u_long(xdrs, &(ar->ar_vers.low)))
				return (FALSE);
			return (xdr_u_long(xdrs, &(ar->ar_vers.high)));
		}
		return (TRUE);
	}
#endif /* _KERNEL */
	if (
	    xdr_u_long(xdrs, &(rmsg->rm_xid)) && 
	    xdr_enum(xdrs, (enum_t *)&(rmsg->rm_direction)) &&
	    (rmsg->rm_direction == REPLY) )
		return (xdr_union(xdrs, (enum_t *)&(rmsg->rm_reply.rp_stat),
		   (caddr_t)&(rmsg->rm_reply.ru), reply_dscrm, NULL_xdrproc_t));
	return (FALSE);
}


/*
 * Serializes the "static part" of a call message header.
 * The fields include: rm_xid, rm_direction, rpcvers, prog, and vers.
 * The rm_xid is not really static, but the user can easily munge on the fly.
 */
bool_t
xdr_callhdr(register XDR *xdrs, register struct rpc_msg *cmsg)
{

	cmsg->rm_direction = CALL;
	cmsg->rm_call.cb_rpcvers = RPC_MSG_VERSION;
	if (
	    (xdrs->x_op == XDR_ENCODE) &&
	    xdr_u_long(xdrs, &(cmsg->rm_xid)) &&
	    xdr_enum(xdrs, (enum_t *)&(cmsg->rm_direction)) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_rpcvers)) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_prog)) )
	    return (xdr_u_long(xdrs, &(cmsg->rm_call.cb_vers)));
	return (FALSE);
}

/* ************************** Client utility routine ************* */

static void
accepted(register enum accept_stat acpt_stat, register struct rpc_err *error)
{

	switch (acpt_stat) {

	case PROG_UNAVAIL:
		error->re_status = RPC_PROGUNAVAIL;
		return;

	case PROG_MISMATCH:
		error->re_status = RPC_PROGVERSMISMATCH;
		return;

	case PROC_UNAVAIL:
		error->re_status = RPC_PROCUNAVAIL;
		return;

	case GARBAGE_ARGS:
		error->re_status = RPC_CANTDECODEARGS;
		return;

	case SYSTEM_ERR:
		error->re_status = RPC_SYSTEMERROR;
		return;

	case SUCCESS:
		error->re_status = RPC_SUCCESS;
		return;
	}
	/* something's wrong, but we don't know what ... */
	error->re_status = RPC_FAILED;
	error->re_lb.s1 = (long)MSG_ACCEPTED;
	error->re_lb.s2 = (long)acpt_stat;
}

static void 
rejected(register enum reject_stat rjct_stat, register struct rpc_err *error)
{

	switch (rjct_stat) {

	case RPC_MISMATCH:
		error->re_status = RPC_VERSMISMATCH;
		return;

	case AUTH_ERROR:
		error->re_status = RPC_AUTHERROR;
		return;
	}
	/* something's wrong, but we don't know what ... */
	error->re_status = RPC_FAILED;
	error->re_lb.s1 = (long)MSG_DENIED;
	error->re_lb.s2 = (long)rjct_stat;
}

/*
 * given a reply message, fills in the error
 */
void
_seterr_reply(register struct rpc_msg *msg, register struct rpc_err *error)
{

	/* optimized for normal, SUCCESSful case */
	switch (msg->rm_reply.rp_stat) {

	case MSG_ACCEPTED:
		if (msg->acpted_rply.ar_stat == SUCCESS) {
			error->re_status = RPC_SUCCESS;
			return;
		};
		accepted(msg->acpted_rply.ar_stat, error);
		break;

	case MSG_DENIED:
		rejected(msg->rjcted_rply.rj_stat, error);
		break;

	default:
		error->re_status = RPC_FAILED;
		error->re_lb.s1 = (long)(msg->rm_reply.rp_stat);
		break;
	}
	switch (error->re_status) {

	case RPC_VERSMISMATCH:
		error->re_vers.low = msg->rjcted_rply.rj_vers.low;
		error->re_vers.high = msg->rjcted_rply.rj_vers.high;
		break;

	case RPC_AUTHERROR:
		error->re_why = msg->rjcted_rply.rj_why;
		break;

	case RPC_PROGVERSMISMATCH:
		error->re_vers.low = msg->acpted_rply.ar_vers.low;
		error->re_vers.high = msg->acpted_rply.ar_vers.high;
		break;
	}
}
