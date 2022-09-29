/*
 *               Copyright (C) 1997 Silicon Graphics, Inc.                     
 *                        
 *  These coded instructions, statements, and computer programs  contain
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and
 *  are protected by Federal copyright law.  They  may  not be disclosed
 *  to  third  parties  or copied or duplicated in any form, in whole or
 *  in part, without the prior written consent of Silicon Graphics, Inc.
 *                        
 *
 *  Filename: st_socket.c
 *  Description: socket-specific processing for the ST protocol.
 *
 */


#include "tcp-param.h"
#include "sys/param.h"
#include "sys/debug.h"
#include "sys/cmn_err.h"
#include "sys/cred.h"
#include "sys/domain.h"
#include "sys/errno.h"
#include "sys/file.h"
#include "sys/kthread.h"
#include "sys/mbuf.h"
#include "ksys/pid.h"
#include "ksys/vproc.h"
#include "sys/proc.h"
#include "sys/protosw.h"
#include "sys/signal.h"
#include "sys/socket.h"
#include "sys/socketvar.h"
#include "sys/strmp.h"
#include "sys/uio.h"
#include "sys/systm.h"
#include "sys/kmem.h"
#include "bstring.h"
#include "sys/sat.h"
#include "sys/capability.h"
#include "sys/atomic_ops.h"
#include "sys/sesmgr.h"

#include "sys/immu.h"


#include "sys/sysmacros.h"
#include "net/route.h"
#include "net/if.h"
#include "sys/types.h"
#include "sys/kmem.h"

#include "in.h"
#include "in_systm.h"
#include "ip.h"
#include "in_pcb.h"
#include "ip_var.h"
#include "ip_icmp.h"
#include "in_var.h"

#include "sys/cmn_err.h"
#include "sys/tcpipstats.h"

#include "st.h"
#include "st_var.h"
#include "st_macros.h"
#include "st_if.h"
#include "st_bufx.h"
#include "st_debug.h"


extern void sbdroprecord(struct sockbuf *sb);

static int xmitbreak = 1024;

int
st_sosend(register struct socket *so,
	  struct mbuf *nam,
	  register struct uio *uio,
	  int flags,
	  struct mbuf *rights)
{
	struct mbuf *top = 0;
	st_buf_t	payload;
	register int space;
	int len, rlen = 0, error = 0, first = 1;
	int intr = 1;		
	struct stpcb *sp = sotostpcb(so);
	st_tx_t		*tx;
	st_tid_t	tid = 0;
	int		i, unaligned = 0, iov_len;
	struct	uio	*aligned_uio = NULL;
	int		sprayed;

	

	/* need to figure out whtehr it can wait: see mwait in sosend */

	if (!(so->so_state & SS_WANT_CALL))
		KTOP_UPDATE_CURRENT_MSGSND(1);
	if (rights)
		rlen = rights->m_len;
#define	snderr(errno)	{ error = errno; goto release; }


	dprintf(10, ("send len %d, uio cnt %d, off %d in st_sosend\n",
		uio->uio_iov->iov_len, uio->uio_iovcnt,
		uio->uio_offset));

	/* set up for unnatural exit, and checking status */
	bzero(&payload, sizeof(st_buf_t));

	if(! uio->uio_resid)  {
		return	0;
	}

	if(uio->uio_iovcnt > 1)  {
		dprintf(30, ("st_sosend: multiple iovecs %d\n", 
			uio->uio_iovcnt));
	}
	
	tx = &(sp->tx[tid]);
 	sprayed = (tx->tx_spray_width > 1) ? 1 : 0;

	for(i = 0; i < uio->uio_iovcnt; i++)  {
		/* SHAC_1.0 WAR: following should be required for less
		*  than word alignment, but due to a SHAC bug, we need 
		*  DW/cacheline alignment under some cases: PV 637597
		*/
		/* if(((__psint_t) uio->uio_iov[i].iov_base) & 0x3) {
		** } */
		if(sprayed && (((__psint_t) uio->uio_iov[i].iov_base)
					& (CACHE_SLINE_SIZE - 1)))  {
			dprintf(0, ("Sprayed transfers must be "
					"cacheline aligned\n"));
			return EFAULT;
		}
		if((uio->uio_iov[i].iov_len <= 32 && 
			(((__psint_t) uio->uio_iov[i].iov_base) 
				& (CACHE_SLINE_SIZE - 1)))
		    || 
			(((__psint_t) uio->uio_iov[i].iov_base) 
					& 0x7))  {
		dprintf(10, ("st_sosend: user buffer (0x%x) len %d "
				"is misaligned\n",
				uio->uio_iov[i].iov_base, 
				uio->uio_iov[i].iov_len));
		unaligned = 1;
		break;
		}
	}

	if(! unaligned)  {
		aligned_uio = uio;
	}
	else {
		aligned_uio = kmem_zalloc(sizeof(uio_t), KM_SLEEP);
		ASSERT(aligned_uio);
		aligned_uio->uio_iov = kmem_zalloc(
			uio->uio_iovcnt * sizeof(iovec_t), KM_SLEEP);
		ASSERT(aligned_uio->uio_iov);
		aligned_uio->uio_iovcnt = uio->uio_iovcnt;
		aligned_uio->uio_resid = uio->uio_resid;
		for(i = 0; i < aligned_uio->uio_iovcnt; i++)  {
			iov_len = btoc(uio->uio_iov[i].iov_len) * NBPP;
			dprintf(30, ("st_sosend: iov_len is %d\n",
				iov_len));
			if(iov_len)  {
				aligned_uio->uio_iov[i].iov_base = 
				kmem_contig_alloc(iov_len, 
						ST_MAX_PAGE_SIZE, 0);
				ASSERT_ALWAYS(aligned_uio->uio_iov[i].iov_base);
				aligned_uio->uio_iov[i].iov_len = 
						uio->uio_iov[i].iov_len;
				dprintf(10, ("st_sosend: unaligned; base 0x%x, len %d\n",
				aligned_uio->uio_iov[i].iov_base,
				aligned_uio->uio_iov[i].iov_len));
				if(uio->uio_segflg == UIO_SYSSPACE) {
					bcopy(uio->uio_iov[i].iov_base, 
						aligned_uio->uio_iov[i]
							.iov_base, 
						uio->uio_iov[i].iov_len);
				}
				else {
					if(copyin(
					uio->uio_iov[i].iov_base,
					aligned_uio->uio_iov[i]
							.iov_base, 
					uio->uio_iov[i].iov_len))  {
						kmem_free(
						aligned_uio->uio_iov, 
						aligned_uio->uio_iovcnt 
						* sizeof(iovec_t));
						aligned_uio->uio_iovcnt 
								= 0;
						kmem_free(
						aligned_uio, 
						sizeof(uio_t));
						aligned_uio = NULL;
						dprintf(0, 
						("sosend: bad user addr\n"));
						return EFAULT;
					}
				}
				dprintf(30, ("st_sosend: copied %d bytes\n",
					uio->uio_iov[i].iov_len));
			}
		}
	}

	if ((error = sblock(&so->so_snd, NETEVENT_SODOWN, so, intr)) != 0)
		return (error);
	do {
		ASSERT(SOCKET_ISLOCKED(so));
		if (so->so_state & SS_CANTSENDMORE)
			snderr(EPIPE);
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;			/* ??? */
			goto release;
		}
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			if (so->so_proto->pr_flags & PR_CONNREQUIRED)
				snderr(ENOTCONN);
			if (nam == 0 &&
			    (so->so_proto->pr_flags & PR_DESTADDROPT) == 0)
				snderr(EDESTADDRREQ);
		}
		if (flags & MSG_OOB)
			space = 1024;
		else {
			space = sbspace(&so->so_snd);
			if (space <= rlen ||
			   (sosendallatonce(so) &&
				space < aligned_uio->uio_resid + rlen) ||
			   (aligned_uio->uio_resid >= xmitbreak && space < xmitbreak &&
			   so->so_snd.sb_cc >= xmitbreak &&
			   (so->so_state & SS_NBIO) == 0)) {
				if (so->so_state & SS_NBIO) {
					if (first)
						error = EWOULDBLOCK;
					goto release;
				}

				NETPAR(NETSCHED, NETSLEEPTKN, (char)&so->so_snd,
					NETEVENT_SODOWN,
                                       	NETCNT_NULL, NETRES_SBFULL);
				error = sbunlock_wait(&so->so_snd, so, intr);
				NETPAR(NETSCHED, NETWAKEUPTKN,
				    (char)&so->so_snd, NETEVENT_SODOWN,
				    NETCNT_NULL, NETRES_SBFULL);
				if (error)
					return (error);
				error = sblock(&so->so_snd,
				       NETEVENT_SODOWN, so, intr);
				if (error)
					return (error);
				continue;
			}
		}

		/*
		 * The hold count keeps a socket from being closed by a
		 * protocol (on MPs).  Below the socket lock is dropped
		 * while doing uiomove and associated bookkeepping.  This
		 * is done because uiomove could fault and could wait a
		 * very long time (nfs); the protocol timeout routines
		 * become hung as they need to grab socket locks to do
		 * timeout function processing.
		 */
		so->so_holds++;
		ASSERT_ALWAYS(SOCKET_ISLOCKED(so));
		SOCKET_UNLOCK(so);

		space -= rlen;
		while (space > 0) {	/* for rolling pin */
			struct iovec *iov;
			iov = aligned_uio->uio_iov;
			
			len = iov->iov_len;
			if(len <= 0) {
				aligned_uio->uio_iov++;
				if (--aligned_uio->uio_iovcnt == 0) {
					ASSERT(aligned_uio->uio_resid == 0);
					break;
				}
				continue;
			}


			payload.bufx_flags = BUF_ADDR;
			payload.payload_len = aligned_uio->uio_resid;
			
			payload.uio = aligned_uio;

			space -= len;
			if (error)
				goto holdoff;
			if (aligned_uio->uio_resid <= 0)
				break;
		}

		SOCKET_LOCK(so);
		so->so_holds--;

		if (so->so_state & SS_CANTSENDMORE)
			snderr(EPIPE);
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;			/* ??? */
			goto release;
		}
		NETPAR(NETFLOW, NETFLOWTKN, NETPID_NULL,
			 NETEVENT_SODOWN, len, NETRES_NULL);
		if (!(flags & MSG_OOB) && _SESMGR_PUT_SAMP(so, &top, nam) != 0)
			goto release;
		if(payload.payload_len > 0)  {
			top = (struct mbuf *) &payload;
			error = (*so->so_proto->pr_usrreq)(so,
		    		(flags & MSG_OOB) ? 
						PRU_SENDOOB : PRU_SEND,
		    				top, nam, rights);
			ASSERT_ALWAYS(SOCKET_ISLOCKED(so));
			payload.test_bufaddr = NULL;
			payload.st_addr_bufx.bufaddr = NULL;
			payload.uio = NULL;
			if (so->so_error) {
				dprintf(5, ("Got error %d from PRU_SEND\n",
					so->so_error));
				error = so->so_error;
				so->so_error = 0;
				goto release;
			}
		}

		if(!error)  {
			aligned_uio->uio_resid -= payload.payload_len;
			aligned_uio->uio_offset += payload.payload_len;
		}


		rights = 0;
		rlen = 0;
		top = 0;
		first = 0;
		if (error)
			break;
	} while (aligned_uio->uio_resid);

release:
	ASSERT(SOCKET_ISLOCKED(so));

	if (payload.st_addr_bufx.bufaddr) {
		payload.st_addr_bufx.bufaddr = NULL;
		payload.payload_len = 0;
	}


	if(tx->tx_buf.bufx_flags != BUF_NONE)  {
		dprintf(20, ("Cancelling timer and "
			"tearing down in PRU_SEND \n"));
		st_cancel_timer(sp, TID_TO_TIMER_ID(tid));
		if(st_teardown_buf(sp, &tx->tx_buf, 
				(u_char) ST_BUFX_ALLOW_SEND, tid)) {
			cmn_err(CE_PANIC, "st_teardown_buf failed\n");
		}

		bzero(&(tx->tx_buf), sizeof(tx->tx_buf));
	}
	
	if(aligned_uio && aligned_uio != uio)  {
		uio->uio_resid = aligned_uio->uio_resid;
		dprintf(30, ("Releasing uio; %d iovs\n", 
			aligned_uio->uio_iovcnt));
		for(i = 0; i < aligned_uio->uio_iovcnt; i++)  {
			dprintf(30, ("iov %d, len %d released\n",
				i, aligned_uio->uio_iov[i].iov_len));
			iov_len = NBPP * btoc(
				aligned_uio->uio_iov[i].iov_len);
			if(iov_len)  {
				kmem_contig_free(
					aligned_uio->uio_iov[i].iov_base, 
							iov_len);
			}
			aligned_uio->uio_iov[i].iov_base = NULL;
		}
		aligned_uio->uio_resid = 0;
		kmem_free(aligned_uio->uio_iov, 
			aligned_uio->uio_iovcnt * sizeof(iovec_t));
		aligned_uio->uio_iovcnt = 0;
		kmem_free(aligned_uio, sizeof(uio_t));
		aligned_uio = NULL;
		dprintf(30, ("Released aligned_uio\n"));
	}

	sbunlock(&so->so_snd, NETEVENT_SODOWN, so);

	/* Cause SIGPIPE to be sent on return */
	if (error == EPIPE) {
		dprintf(10, ("Recv: uio err set to EPIPE\n"));
		uio->uio_sigpipe = 1;
	}

	NETPAR(error ? NETFLOW : 0,
	       NETDROPTKN, NETPID_NULL, NETEVENT_SODOWN,
	       uio->uio_resid, NETRES_ERROR);

	return (error);

holdoff:
	SOCKET_LOCK(so);
	so->so_holds--;
	goto release;
}


int
st_soreceive(register struct socket *so,
	  struct mbuf **aname,
	  register struct uio *uio,
	  int *flagsp,
	  struct mbuf **rightsp)
{
	register int len, error = 0, offset;
	struct protosw *pr = so->so_proto;
	int mflipped = 0;
	int orig_resid = uio->uio_resid;
	int flags;
	int truncated = 0;
	int intr = 1;	/* if 1, allow sblock to be interrupted */
	struct stpcb *sp = sotostpcb(so);
	st_rx_t		*rx;
	static int	intf_registered = 0;
	st_tid_t	rid;
	sthdr_t		sth;
	int		i, unaligned = 0, iov_len;
	struct	uio	*aligned_uio = NULL;
	uint		bufsize = (sp->s_vcd.vc_lbufsize)? 
				sp->s_vcd.vc_lbufsize : ST_LOG_BUFSZ;
	int		sprayed;


	NETPAR(NETSCHED, NETEVENTKN, NETPID_NULL,
		 NETEVENT_SOUP, NETCNT_NULL, NETRES_SYSCALL);
	if (rightsp)
		*rightsp = 0;
	if (aname)
		*aname = 0;
	if (flagsp) {
		flags = *flagsp;
	} else {
		flags = 0;
	}

	if(uio && uio->uio_iovcnt > 1)  {
		dprintf(30, ("st_sorecv: multiple iovecs %d\n", 
			uio->uio_iovcnt));
	}
	for(i = 0; i < uio->uio_iovcnt; i++)  {
		/* SHAC_1.0 WAR
		*  Dest should be able to take care of byte-alignment,
		*  but due to a SHAC bug, we need the following
		*  arrangement for now:
		*  if the SHAC TX block has to make > 1 STU: i.e.,
		*    	if(dest_bufsize < length, or max_stu < length)
		*   		dest_offset must be dw aligned
		*    	else 
		*		dest_offset can be byte aligned
		*/
			
		if((uio->uio_iov[i].iov_len >
			(1 << max(sp->s_vcd.vc_lmaxstu, bufsize)))
					&& 
			(((__psint_t) uio->uio_iov[i].iov_base) & 0x7)) {
			dprintf(10, ("st_sorecv: user buffer (0x%x), len %d "
				"is misaligned\n",
				uio->uio_iov[i].iov_base,
				uio->uio_iov[i].iov_len));
			unaligned = 1;
		}
	}

	if(! unaligned)  {
		aligned_uio = uio;
	}
	else {
		aligned_uio = kmem_zalloc(sizeof(uio_t), KM_SLEEP);
		ASSERT(aligned_uio);
		aligned_uio->uio_iov = kmem_zalloc(
			uio->uio_iovcnt * sizeof(iovec_t), KM_SLEEP);
		ASSERT(aligned_uio->uio_iov);
		aligned_uio->uio_iovcnt = uio->uio_iovcnt;
		aligned_uio->uio_resid = uio->uio_resid;
		for(i = 0; i < aligned_uio->uio_iovcnt; i++)  {
			iov_len = btoc(uio->uio_iov[i].iov_len) * NBPP;
			dprintf(30, ("st_sorec: iov_len is %d\n",
				iov_len));
			if(iov_len)   {
				aligned_uio->uio_iov[i].iov_base = 
					kmem_contig_alloc(iov_len, 
						ST_MAX_PAGE_SIZE, 0);
				ASSERT_ALWAYS(aligned_uio->uio_iov[i].iov_base);
				aligned_uio->uio_iov[i].iov_len = 
						uio->uio_iov[i].iov_len;
				dprintf(0, ("st_recv: misaligned; base 0x%x, len %d\n",
				aligned_uio->uio_iov[i].iov_base,
				aligned_uio->uio_iov[i].iov_len));
			}
		}
	}

	if ((error = sblock(&so->so_rcv, NETEVENT_SOUP, 
					so, intr)) != 0) {
		printf("returning short-cct %d\n", error);
		return error;
	}


	if(! intf_registered)  {
		if(st_findroute(so)) {
			cmn_err(CE_PANIC, "Error in finding "
				"route from receiver\n");
		}
		intf_registered = 1;
	}
	
	ASSERT_ALWAYS(SOCKET_ISLOCKED(so));
	rid = sp->s_ridalloc(sp);
	dprintf(30, ("rid obtained in st_sorecv is %d\n", rid));
	if(rid != 0)  {
#		if 0
		/* 
		* do we want to send ENOTCONN when other end has died? 
		*/
		dprintf(0, ("Bad rid %d, returning ENOTCONN\n", rid));
		error = ENOTCONN;
#		endif /* 0 */
		goto release;
	}

	/* ASSERT_ALWAYS(rid == 0); */
	rx = &(sp->rx[rid]);
	rx->rx_spray_width = rx->rx_spray_width ?
						rx->rx_spray_width : 1;
	dprintf(30, ("rid %u is %u at 0x%x\n", 
		rid, rx->rx_rid, &(rx->rx_rid)));
	sprayed = (rx->rx_spray_width > 1) ? 1 : 0;

	for(i = 0; i < uio->uio_iovcnt; i++)  {
		if(sprayed && (((__psint_t) uio->uio_iov[i].iov_base)
					& (CACHE_SLINE_SIZE - 1)))  {
			dprintf(0, ("Sprayed transfers must be "
				"cacheline aligned\n"));
			error = EFAULT;
			goto ridfree_and_release;
		}
	}

	/* allocate ST_RX_MX_ENTRIES Mx entries or however many are available */
	for (i = 0; i < ST_RX_MX_ENTRIES; i++) {
		rx->rx_mx[i] = sp->s_R_Mx_alloc(sp);
		if (rx->rx_mx[i] == (u_int16_t) INVALID_R_Mx)
			break;
	}
	rx->rx_num_mx = i;
	if (rx->rx_num_mx == 0) {
		dprintf(0, ("No Mx's available, tearing down read\n"));
		error = ENOMEM;
		goto ridfree_and_release;
	}

	rx->rx_buf.uio = aligned_uio;
	error = st_setup_buf(sp, &rx->rx_buf, aligned_uio,
				(u_char) ST_BUFX_ALLOW_RECV, rid);
	if(error) {
		dprintf(0, ("Bad status from st_setup_buf\n"));
		goto ridfree_and_release;
	}


	if (flags & MSG_DONTWAIT)
		flags &= ~MSG_WAITALL;	/* conflicting options */
	if (flags & _MSG_NOINTR)
		intr = 2;
	if (flags & MSG_OOB) {
		cmn_err(CE_PANIC, "out-of-band ST not implemented yet\n");
	}


	dprintf(30, ("Before restart, %d data bytes left\n", 
		so->so_rcv.sb_cc));

restart:
	if (so->so_state & SS_CANTRCVMORE) {
#		if 0
		/* 
		* do we want to send EPIPE when the other end has 
		* gone away in the middle of a read() ?
		*/
		dprintf(10, ("recv: err set to EPIPE\n"));
		error = EPIPE;
#		endif /* 0 */
		goto ridfree_and_release;
	}

	/* ASSERT((signed)so->so_rcv.sb_cc >= 0); */
	if((signed)so->so_rcv.sb_cc < 0)  {
		cmn_err(CE_PANIC, "so_rcv.sb_cc (0x%x) -ve: %d\n",
			&(so->so_rcv.sb_cc), so->so_rcv.sb_cc);
	}
	if (so->so_rcv.sb_cc == 0) {
		if (so->so_error) {
			/* don't give error now if any data has been moved */
			if (orig_resid == aligned_uio->uio_resid) {
				error = so->so_error;
				so->so_error = 0;
			}
			goto ridfree_and_release;
		}
		if ((so->so_state & SS_ISCONNECTED) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED)) {
			dprintf(10, ("STP: ~SS_ISCONNECTED in recv\n"));
			error = ENOTCONN;
			goto ridfree_and_release;
		}
		if (so->so_state & SS_CANTRCVMORE) {
			dprintf(10, ("STP: SS_CANTRCVMORE in recv\n"));

			goto ridfree_and_release;
		}
		if (aligned_uio->uio_resid == 0)
			goto ridfree_and_release;
		if ((so->so_state & SS_NBIO) || (flags & MSG_DONTWAIT)) {
			error = EWOULDBLOCK;
			goto ridfree_and_release;
		}

		NETPAR(NETSCHED, NETSLEEPTKN, (char)&so->so_rcv,
			 NETEVENT_SOUP, NETCNT_NULL, NETRES_SBEMPTY);

		if(KX_OUTSTANDING(sp))  {
			/* we have an RTS waiting */
			st_tid_t	kid = 0;
			sthdr_rts_t	*rts = (sthdr_rts_t *) &sth;

			dprintf(30, ("freeing kid \n"));
			ASSERT_ALWAYS(SOCKET_ISLOCKED(so));
			sp->s_kidfree(sp, kid, &sth);

			if(st_data_fsm(&sp, ST_RTS, &sth, rid)) {
				dprintf(0, ("st_data_fsm returned bad "
					"status in st_soreceive\n"));
				error = EPROTO;
				goto ridfree_and_release;
			}
		}

		dprintf(30, ("Going into sbunlock_wait, ONE \n"));
		error = sbunlock_wait(&so->so_rcv, so, intr);
		dprintf(30, ("After sbunlock_wait ONE, so_rcv.sb_cc %d, error %d \n",
			so->so_rcv.sb_cc, error));


		if (error)
			goto ridfree_and_release;

		NETPAR(NETSCHED, NETWAKEUPTKN, (char)&so->so_rcv,
			 NETEVENT_SOUP, NETCNT_NULL, NETRES_SBEMPTY);

		dprintf(30, ("Going into sblock, so_rcv.sb_cc %d \n",
			so->so_rcv.sb_cc));
		error = sblock(&so->so_rcv, NETEVENT_SOUP, so, intr);
		dprintf(30, ("After sblock, so_rcv.sb_cc %d, error %d\n",
			so->so_rcv.sb_cc, error));

		dprintf(30, ("Cancelling timer and tearing down in recv\n"));
		st_cancel_timer(sp, RID_TO_TIMER_ID(rid));
		if(st_teardown_buf(sp, &rx->rx_buf, 
				(u_char) ST_BUFX_ALLOW_RECV, rid)) {
			cmn_err(CE_PANIC, "st_teardown_buf failed\n");
		}
		rx->rx_buf.uio = NULL;
		rx->rx_state = STP_READY_FOR_RTS;


		if (error) {
			goto ridfree_and_release;
		}
		goto restart;
	}

	/*
	 * For streams interface, this routine MAY be called from a 	
	 * service routine, which means NO u area, OR the wrong one.
	 */
	if (!(so->so_state & SS_WANT_CALL)) 
		KTOP_UPDATE_CURRENT_MSGRCV(1);
	  {
		if (pr->pr_flags & PR_ADDR)  {
			cmn_err(CE_PANIC, "Shouldn't come here: "
			"ST is a connection-oriented protocol\n");
		}

		len = so->so_rcv.sb_cc;
		if(len != aligned_uio->uio_resid)  {
			dprintf(5, ("read for %u bytes, %u satisfied\n",
				aligned_uio->uio_resid, len));
		}

		ASSERT(aligned_uio->uio_iovcnt >= 1);
		so->so_state &= ~SS_RCVATMARK;
		if (so->so_oobmark && len > so->so_oobmark - offset)
			len = so->so_oobmark - offset;
		/*
	 	* the holdcount keeps a socket from being closed by a protocol
	 	* while executing the loop below
	 	*/
		so->so_holds++;
		ASSERT_ALWAYS(SOCKET_ISLOCKED(so));
		SOCKET_UNLOCK(so); 	

		if (len > rx->rx_buf.payload_len)  {
			cmn_err(CE_WARN, "recvd too much (%u bytes, not  %u)\n",
				len, rx->rx_buf.payload_len);
			/* len = rx->rx_buf.payload_len; */
		}
		NETPAR(NETFLOW, NETFLOWTKN, NETPID_NULL,
	       		NETEVENT_SOUP, (int)len, NETRES_NULL);

		error = len;
		aligned_uio->uio_resid -= len;
		aligned_uio->uio_offset += len;
		dprintf(30, ("recvd %d bytes, resid %d, err %d\n",
			len, aligned_uio->uio_resid, error));

		SOCKET_LOCK(so);
		so->so_holds--;


		ST_DUMP_PAYLOAD_PREFIX(aligned_uio);


		if(rx->hdr_for_RSR)  {
			sthdr_t		*sth = mtod(
					rx->hdr_for_RSR, sthdr_t *);
			sthdr_data_t	*data = &(sth->sth_data);
			

			dprintf(10, ("Checking st_ack_last_recv from st_sorecv\n"));
			if((data->OpFlags & ST_FLAG_MASK & ST_SENDSTATE)
				&& st_ack_last_recv(sp, rid, data->Sync))  {
				cmn_err(CE_PANIC, "Couldn't ACK ST recv\n");
			}
			m_freem(rx->hdr_for_RSR);
			rx->hdr_for_RSR = NULL;
		}
		else {
			dprintf(10, ("WARN: no RSR-hds seen at sorecv exit\n"));
		}


		bzero(&(rx->rx_buf), sizeof(rx->rx_buf));

		so->so_rcv.sb_cc -= len;
		if(rx->rx_buf.payload_len > len) {
			truncated = 1;
		}


		ASSERT(mflipped == 0);
		if (so->so_oobmark) {
			if ((flags & MSG_PEEK) == 0) {
				so->so_oobmark -= len;
				if (so->so_oobmark == 0) {
					so->so_state |= SS_RCVATMARK;
					goto uio_done;
				}
			} else
				offset += len;
		}
	}	


uio_done:
	if(truncated && (pr->pr_flags & PR_ATOMIC) && flagsp) {
		*flagsp = MSG_TRUNC;
	}

	if ((flags & MSG_PEEK) == 0) {
		if (pr->pr_flags & PR_ATOMIC)
			(void) sbdroprecord(&so->so_rcv);
		if (pr->pr_flags & PR_WANTRCVD && so->so_pcb)
			st_usrreq(so, PRU_RCVD, (struct mbuf *)0,
			    (struct mbuf *)0, (struct mbuf *)0);
		if (error == 0 && rightsp && *rightsp &&
		    pr->pr_domain->dom_externalize) {
			error = (*pr->pr_domain->dom_externalize)(*rightsp);
			orig_resid = aligned_uio->uio_resid;	/* keep this error */
		}
	}

	if ((flags & MSG_WAITALL) && aligned_uio->uio_resid > 0 && error == 0)
		goto restart;

ridfree_and_release:
	if(!SOCKET_ISLOCKED(so))  {
		SOCKET_LOCK(so);
	}

	if(so->so_rcv.sb_cc)  {
		dprintf(30, ("recv buf had %d unread bytes\n",
			so->so_rcv.sb_cc));
		so->so_rcv.sb_cc = 0;
	}
	
	ASSERT_ALWAYS(SOCKET_ISLOCKED(so));
	dprintf(30, ("Releasing rid %d\n", rid));
	ASSERT_ALWAYS(0 == sp->s_ridfree(sp, rid));

release:
	ASSERT_ALWAYS(SOCKET_ISLOCKED(so));
	sbunlock(&so->so_rcv, NETEVENT_SOUP, so);
	NETPAR(error ? NETFLOW : 0,
	       NETDROPTKN, NETPID_NULL,
	       NETEVENT_SOUP, NETCNT_NULL, NETRES_ERROR);

	if(aligned_uio && aligned_uio != uio)  {
		uio->uio_resid = aligned_uio->uio_resid;
		dprintf(30, ("st_sorec: Releasing uio; %d iovs\n", 
			aligned_uio->uio_iovcnt));
		for(i = 0; i < aligned_uio->uio_iovcnt; i++)  {
			if(uio->uio_segflg == UIO_SYSSPACE) {
				bcopy(aligned_uio->uio_iov[i].iov_base,
					uio->uio_iov[i].iov_base, 
					uio->uio_iov[i].iov_len);
			}
			else {
				if(copyout(
					 aligned_uio->uio_iov[i]
						.iov_base, 
					uio->uio_iov[i].iov_base,
					uio->uio_iov[i].iov_len)) {
					aligned_uio->uio_resid = 0;
					kmem_free(aligned_uio->uio_iov, 
						aligned_uio->uio_iovcnt 
						* sizeof(iovec_t));
					aligned_uio->uio_iovcnt = 0;
					kmem_free(aligned_uio, sizeof(uio_t));
					aligned_uio = NULL;
					dprintf(0, ("sorecv: bad user addr\n"));
					return EFAULT;
				}
			}
			dprintf(30, ("st_sorec: copied %d bytes\n",
				uio->uio_iov[i].iov_len));
			dprintf(30, ("st_sorec: iov %d, len %d released\n",
				i, aligned_uio->uio_iov[i].iov_len));
			iov_len = NBPP * btoc(
				aligned_uio->uio_iov[i].iov_len);
			if(iov_len)  {
				kmem_contig_free(
					aligned_uio->uio_iov[i].iov_base, 
								iov_len);
			}
			aligned_uio->uio_iov[i].iov_base = NULL;
		}
		aligned_uio->uio_resid = 0;
		kmem_free(aligned_uio->uio_iov, 
			aligned_uio->uio_iovcnt * sizeof(iovec_t));
		aligned_uio->uio_iovcnt = 0;
		kmem_free(aligned_uio, sizeof(uio_t));
		aligned_uio = NULL;
		dprintf(30, ("st_sorec: Released aligned_uio\n"));
	}

	/* if data has been moved, return it, give the error next time */

	if (orig_resid > uio->uio_resid)
		return 0;
	else  {
		dprintf(5, ("Returning %d from st_soreceive\n", error));
		return (error);
	}
}



int
st_sodisconnect(register struct socket *so, int	free_tids)
{
	struct stpcb *sp = sotostpcb(so);

	if (so->so_rcv.sb_cc) {
	  so->so_rcv.sb_cc = 0;
	}

	if (so->so_snd.sb_cc) {
	  so->so_rcv.sb_cc = 0;
	}


	if (sp->s_vc_state == STP_VCS_CONNECTED) {
		stvc_output(sp, ST_RDISCONNECT, INVALID_TID);
		sp->s_vc_state = STP_VCS_RDSENT;
		stvc_isdisconnecting(sp);
		if(free_tids)  {
			ridfree_all(sp);
			iidfree_all(sp);
			if(KX_OUTSTANDING(sp)) {
				kidfree_all(sp);
			}
		}
		dprintf(20, ("Waking up read/writers in st_sodisconnect\n"));
		SOCKET_SBWAIT_WAKEALL(&so->so_snd);
		SOCKET_SBWAIT_WAKEALL(&so->so_rcv);
		return 0;
	}
	else {
		dprintf(0, ("ST VC is in state %s\n",
			st_decode_state(sp->s_vc_state)));
	}

	return -1;
}


/* ARGSUSED */
int
st_sopoll(struct socket *so, short events, int anyyet, short *reventsp,
			struct pollhead **phpp, unsigned int *genp)
{
	register unsigned revents;
	/* struct stpcb *sp = sotostpcb(so); */

	NETSPL_DECL(s1)

#define sbselqueue(sb) (sb)->sb_flags |= SB_SEL


	dprintf(10, ("Inside st_sopoll, events 0x%x\n", events));
 
	/* we can set phpp outside lock, saves two arg restores */
	if (!anyyet) {
		*phpp = &so->so_ph;
		*genp = POLLGEN(&so->so_ph);
		dprintf(30, ("gen is: %u \n", *genp));
	}


	dprintf(10, ("st_soreadable: %d (so_rcv.sb_cc %u, "
			"recv 0x%x, rec | err 0x%x, KX_OUT 0x%x)\n",
		st_soreadable(so), so->so_rcv.sb_cc, 
		so->so_state & SS_CANTRCVMORE, 
		((so->so_state & SS_CANTRCVMORE) | so->so_error),
		KX_OUTSTANDING(sotostpcb(so))));

	dprintf(10, ("st_sowriteable: %d (state 0x%x, flag 0x%x, "
			"iscon: 0x%x, csend 0x%x)\n",
		st_sowriteable(so), so->so_state, 
		so->so_proto->pr_flags,
		so->so_state & SS_ISCONNECTED,
		so->so_state & SS_CANTSENDMORE));

	dprintf(10, ("csend | error 0x%x\n", 
		((so->so_state & SS_CANTSENDMORE) | so->so_error)));


	revents = events;
	SOCKET_LOCK(so);
	SOCKET_QLOCK(so, s1);
	if ((revents & (POLLIN|POLLRDNORM)) && !st_soreadable(so)) {
		dprintf(10, ("Clearing revents POLLIN and POLLRDNORM); "
			"selQ so_rcv\n"));
		revents &= ~(POLLIN|POLLRDNORM);
		sbselqueue(&so->so_rcv);
	}
	if ((revents & (POLLPRI|POLLRDBAND))
	    && !(so->so_oobmark | (so->so_state & SS_RCVATMARK))) {
		dprintf(10, ("Clearing revents POLLPRI and POLLRDBAND); "
			"selQ so_rcv\n"));
		revents &= ~(POLLPRI|POLLRDBAND);
		sbselqueue(&so->so_rcv);
	}
	if ((revents & POLLCONN) && so->so_qlen == 0) {
		dprintf(10, ("Clearing revents POLLCONN; "
			"selQ so_rcv\n"));
		revents &= ~POLLCONN;
		sbselqueue(&so->so_rcv);
	}
	if ((revents & POLLOUT) && !sowriteable(so)) {
		dprintf(10, ("Clearing revents POLLOUT; "
			"selQ so_snd\n"));
		revents &= ~POLLOUT;
		sbselqueue(&so->so_snd);
	}
	SOCKET_QUNLOCK(so, s1);
	SOCKET_UNLOCK(so);
	*reventsp = revents;
	return 0;
}
