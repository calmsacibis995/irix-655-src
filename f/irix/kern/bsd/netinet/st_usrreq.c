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
 *  Filename: st_usrreq.c
 *  Description: usrreq for the ST protocol. 'Nuff said.
 */


#include "sys/param.h"
#include "sys/debug.h"
#include "sys/systm.h"
#include "sys/mbuf.h"
#include "sys/socket.h"
#include "sys/socketvar.h"
#include "sys/protosw.h"

#include "sys/sysmacros.h"
#include "sys/errno.h"
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




int 
st_usrreq(struct socket *so, int req, struct mbuf *m, 
				struct mbuf *nam, struct mbuf *rights)
{
  	struct inpcb *inp = sotoinpcb(so);
  	struct stpcb *sp;
  	int error = 0, status;

  
  	DPRINTF(ST_DEBUG_ENTRY, ("ENTRY: st_usrreq()\n"));
  
  	dprintf(30, ("req is %d in st_usrreq, socket 0x%x sock-state 0x%x\n", 
		req, so, so->so_state));

  	ASSERT(SOCKET_ISLOCKED(so));

  	if (inp !=NULL ) {
	  	sp = sotostpcb(so);
	  	stpcb_init(sp);
  	}
  
  	if (req == PRU_CONTROL) 
    		return (in_control(so, (__psint_t)m, (caddr_t)nam,
		       		(struct ifnet *)rights));
  	if ((req != PRU_SEND) && rights && rights->m_len) {
    		error = EINVAL;
    	goto release;
  	}
  	if (inp == NULL && req != PRU_ATTACH) {
    		error = EINVAL;
    		goto release;
  	}
  
  	switch (req) {
    
  		case PRU_ATTACH: 
	  		DPRINTF(ST_DEBUG_SETUP, ("SETUP: PRU_ATTACH\n"));
	 		 
	  		if (inp != NULL) {
		  		error = EISCONN;
		  		break;
	  		}
	  		error = stvc_attach(so);
	  		break;
  		case PRU_DETACH:
	  		DPRINTF(ST_DEBUG_SETUP, ("SETUP: PRU_DETACH\n"));

	  		if ((sp != NULL) && (sp->s_vc_state < STP_VCS_CONNECTED)) {
			  STSTAT(stps_closed);
		  		sp->s_vc_state = STP_VCS_CLOSED;
		  		stvc_detach(so);
		  		return 0;
	  		}
	 	 
	  		if ((so->so_state && SS_CLOSING) && (sp == 0)) {
		  		if (inp) {
			  		(void) in_pcbdetach(inp);
		  		}
		  		return 0;
	  		}
	  		break;
	
  		case PRU_BIND:
	  		DPRINTF(ST_DEBUG_SETUP, ("SETUP: PRU_BIND: 0x%x 0x%x\n",
				   inp, nam));
	  		error = stvc_bind(sp, nam);
	  		break;
       
  		case PRU_LISTEN:
	  		DPRINTF(ST_DEBUG_SETUP, ("SETUP: PRU_LISTEN\n"));

	  		error = stvc_listen(sp);
	  		break;
  
     		case PRU_CONNECT:
	     		DPRINTF(ST_DEBUG_SETUP, ("SETUP: PRU_CONNECT\n"));
			STSTAT(stps_connattempt);
			error = stvc_connect(sp, nam);
	     		break;
  		case PRU_CONNECT2:
	  		DPRINTF(ST_DEBUG_SETUP, ("SETUP: PRU_CONNECT2\n"));
	  		error = EOPNOTSUPP;
	  		break;
  		case PRU_DISCONNECT: 
			{
	  		DPRINTF(ST_DEBUG_SETUP, ("SETUP: PRU_DISCONNECT\n"));
	  		if (inp->inp_faddr.s_addr == INADDR_ANY) {
		  		error = ENOTCONN;
		  		break;
	  		}

			if(status = st_sodisconnect(so, 1)) {
				dprintf(10, ("st_sodisconnect returned %d\n",
					status));
			}

	  		break;
			}
	  
  		case PRU_ACCEPT:
	  		DPRINTF(ST_DEBUG_SETUP, ("SETUP: PRU_ACCEPT\n"));
			STSTAT(stps_accepts);

	  		{
		  		struct sockaddr_in *sin;

				sin = mtod(nam, struct sockaddr_in *);
		 		 
		  		dprintf(20, ("accept called, "
					     "socket 0x%x, state 0x%x\n", 
					     so, so->so_state));

		  		nam->m_len = sizeof(struct sockaddr_in);
		  		sin->sin_family = AF_INET;
		  		sin->sin_port = inp->inp_fport;
		  		sin->sin_addr = inp->inp_faddr;
		  		break;
	  		}
	  
  		case PRU_SHUTDOWN:
	  		socantsendmore(so);
	  		break;
	  
	  
  		case PRU_SEND:
			{
			int		tid;
			st_tx_t		*tx;
			st_buf_t	*payload;
			paddr_t		pbuf;
			st_ifnet_t	*stifp = (st_ifnet_t *)
						sp->s_stifp;

			payload = (st_buf_t *) m;
			if(payload->payload_len == 0)  {  
				dprintf(0, ("payload length zero\n"));
				break;
			}
			else {
				dprintf(20, ("payload length %u\n",
					payload->payload_len));
			}

			tid = sp->s_iidalloc(sp);
			if(tid != 0)  {
				error = ENOTCONN;
				break;
			}

			/* ASSERT_ALWAYS(tid == 0);  */

			tx = &(sp->tx[tid]);
			tx->tx_len_of_send = tx->tx_tlen = 
						payload->payload_len;
			tx->tx_buf.uio = payload->uio;
			tx->tx_spray_width = tx->tx_spray_width ?
						tx->tx_spray_width : 1;
			tx->tx_ACKed = 0;
			dprintf(30, ("tx_spray_width set to %d\n",
				tx->tx_spray_width));

			dprintf(30, ("payload len is %u in usrreq\n",
				tx->tx_tlen));

			if(status = st_setup_buf(sp, &tx->tx_buf, 
					tx->tx_buf.uio,
					(u_char) ST_BUFX_ALLOW_SEND,
								tid))  {
				cmn_err(CE_WARN, 
					"st_setup_buf error %d\n",
						status);
				error = status;
				break;
			}

			ST_DUMP_PAYLOAD_PREFIX(tx->tx_buf.uio);

			error = st_start_write(sp, tid);

			dprintf(20, ("PRU_SEND: sbwaiting so 0x%x, buf 0x%x \n", 
				so, so->so_snd));
			ASSERT_ALWAYS(SOCKET_ISLOCKED(so));
			SOCKET_MPUNLOCK_SBWAIT(so, &so->so_snd, PZERO+1);
			ASSERT_ALWAYS(!(SOCKET_ISLOCKED(so)));
			dprintf(20, ("PRU_SEND: after sbwait on sock 0x%x\n", so));
			SOCKET_LOCK(so);	/* st_sosend checks */
			
			dprintf(20, ("Cancelling timer and "
				"tearing down in PRU_SEND \n"));
			st_cancel_timer(sp, TID_TO_TIMER_ID(tid));
			if(st_teardown_buf(sp, &tx->tx_buf, 
					(u_char) ST_BUFX_ALLOW_SEND, 
								tid)) {
				cmn_err(CE_PANIC, "st_teardown_buf failed\n");
			}
			if(tx->tx_tlen && !so->so_error)  {
				so->so_error = EPIPE;
			}
			bzero(&(tx->tx_buf), sizeof(tx->tx_buf));

			dprintf(25, ("bufx %u freed\n", 
				tx->tx_buf.st_addr_bufx.bufx_t.bufx));

			tx->tx_state = STP_READY_FOR_RTS;

			dprintf(20, ("PRU_SEND: Locked sock 0x%x\n", so));
			ASSERT_ALWAYS(SOCKET_ISLOCKED(so));
			
			dprintf(30, ("Releasing tid %d\n", tid));
			ASSERT_ALWAYS(0 == sp->s_iidfree(sp, tid));

			break;
			}
	  
  		case PRU_ABORT:
		  soisdisconnected(so);
		  break;
	  
  		case PRU_SOCKADDR:
	  		in_setsockaddr(inp, nam);
	  		break;
	  
  		case PRU_PEERADDR:
	  		in_setpeeraddr(inp, nam);
	  		break;
	  
  		case PRU_SENSE:
	  		return(0);
       
  		case PRU_RCVD:
  		case PRU_RCVOOB:
			dprintf(30, ("PRU_RCV*:  Got a recv!\n"));
			break;

  		case PRU_SENDOOB:
  		case PRU_SOCKLABEL:
       			error = EOPNOTSUPP;
       			break;

  		case PRU_SLOWTIMO:
			dprintf(10, ("ST: Got a slowtimo!\n"));
			st_timers(sp, (__psint_t) nam);
			break;

  		default:
       			panic("st_usrreq");
     		}
release:
     return(error);
}


int
stvc_attach(struct socket *so) 
{
  	extern void m_pcbinc(int nbytes, int type);

  	register struct stpcb *sp;
  	struct inpcb *inp;
  	int error;
	
  	ASSERT(SOCKET_ISLOCKED(so));
	
  	if (error = in_pcballoc(so, &stpcb_head)) 
    		return error;
	
  	inp = sotoinpcb(so);

  	sp = kmem_zone_zalloc(stpcb_zone, KM_NOSLEEP);

  	if (sp == NULL) {
    		in_pcbdetach(inp);
    		/* Find error */
    		return(0);
  	}

  	inp->inp_ppcb = (caddr_t) sp;
  	sp->s_so    = so;
  	sp->s_inp = inp;
  	sp->s_vc_state = 0;
  	sp->s_ifp = NULL;
	sp->s_vcd.vc_lmaxstu = ST_MAX_STU_SZ;
	st_cancel_timers(sp);

  	soreserve(so, 16384, 16384);
  	so->so_options |= SO_DONTROUTE;
  
  	if (sp == 0) {
    		int nofd = so->so_state & SS_NOFDREF;
    
    		so->so_state &= ~SS_NOFDREF;
    		in_pcbdetach(inp);
    		so->so_state |= nofd;
    		return(ENOBUFS);
  	}
  	return(0);
}

int
stvc_detach(struct socket *so) 
{
  	struct inpcb *inp  = sotoinpcb(so);
  	struct stpcb  *stp  = sotostpcb(so);

	STSTAT(stps_closed);
  	in_pcbdisconnect(inp);
  	soisdisconnected(so);
	if(so->so_rcv.sb_cc)  {
		cmn_err(CE_WARN, "rcv buff has %u bytes; zeroing\n",
			so->so_rcv.sb_cc);
		so->so_rcv.sb_cc = 0;
	}
	if(so->so_snd.sb_cc)  {
		cmn_err(CE_WARN, "snd buff has %u bytes; zeroing\n",
			so->so_snd.sb_cc);
		so->so_snd.sb_cc = 0;
	}
  	sbrelease(&so->so_rcv);
  	sbrelease(&so->so_snd);
  	kmem_zone_free(stpcb_zone, stp);
	in_pcbdetach(inp);
  	return(0);
}    
  



int 
st_ctloutput(int op, struct socket *so, int level, int optname,
	struct mbuf **mp)
{
	int error = 0;
	struct inpcb *inp = sotoinpcb(so);
  	register struct stpcb *sp = NULL;
	register struct mbuf *m;

	ASSERT(SOCKET_ISLOCKED(so));
	sp = sotostpcb(so);
	if(sp == NULL) {
		return ECONNRESET;
	}
	if(level != IPPROTO_STP)  {
		return	EPROTO;
	}

	dprintf(30, ("st_ctloutput entered\n"));
	
	switch(op) {
	default:
		cmn_err(CE_WARN, "Unknown sockop (not set or get) in ST\n");
		error = EINVAL;
		break;


	case PRCO_SETOPT:
		m = *mp;

		switch(optname) {
			default: {
				dprintf(0, ("Unknown option in ST-setsockopt\n"));
				error = EINVAL;
				break;
			}

			/* kernel sockets */
			case ST_CTS_OUTSTD: {
				u_short	cts_req = *mtod(m, u_short *);

				if(! cts_req)  {
					error = EINVAL;
				}
				else {
					sp->tx[0].tx_ctsreq = cts_req;
					dprintf(10, ("cts_req set to %d\n",
						sp->tx[0].tx_ctsreq));
				}
				break;
			}

			case ST_BLKSZ: {
				u_short	blocksize = *mtod(m, u_short *);

				dprintf(10, ("Setting blksz as %u\n",
					blocksize));
				/* NBPP <= blksz <= max-pgsz (16M) */
				if(blocksize < 14 || blocksize > 24) { 
					error = EINVAL;
				}
				else {
					sp->blocksize = blocksize;
					dprintf(10, ("blksz set to %d\n",
						sp->blocksize));
				}

				break;
			}
				
			case ST_BUFSZ: {
				u_int	bufsize = *mtod(m, uint *);

				dprintf(10, ("Setting bufsz as %u\n",
					bufsize));
				if(bufsize < 14 || bufsize > 24) {
					error = EINVAL;
				}
				if(!(ALL_PAGE_SZ_MASK & 
						(1 << bufsize)))  { 
					dprintf(0, ("setsockopt: "
						"Bad bufsz 0x%x "
						"(Page mask 0x%x)\n",
						bufsize,
						ALL_PAGE_SZ_MASK));
					error = EINVAL;
				}
				else if(sp->s_vc_state != 
						STP_VCS_DISCONNECTED) {
					dprintf(0, ("ST doesn't allow "
						"bufsize change after "
						"connection setup\n"));
					error = EPROTO;
				}
				else {
					sp->s_vcd.vc_lbufsize = bufsize;
					dprintf(10, ("bufsz set to %d "
						"on sp 0x%x, vcd 0x%x\n",
						sp->s_vcd.vc_lbufsize,
						sp, &(sp->s_vcd)));
				}

				break;
			}
				
			case ST_OUT_VCNUM: {
				char	vcnum = *mtod(m, u_char *);

				if(vcnum < 1 || vcnum > 3) { 
					error = EINVAL;
				}
				else if(sp->s_vc_state != 
						STP_VCS_DISCONNECTED) {
					dprintf(0, ("ST doesn't allow "
						"output VC change after"
						" connection setup\n"));
					error = EPROTO;
				}
				else {
					sp->s_vcd.vc_out_vcnum = vcnum;
					dprintf(10, ("vcnum set to %d\n",
					(uint) sp->s_vcd.vc_out_vcnum));
				}

				break;
			}


			case ST_TX_SPRAY_WIDTH: {
				char	spray = *mtod(m, u_char *);
				st_tx_t	*tx = &(sp->tx[0]);

				if(sp->s_vc_state != 
						STP_VCS_DISCONNECTED 
					&&
				   tx->tx_state != STP_VCS_DISCONNECTED
					&&
				   tx->tx_state != STP_READY_FOR_RTS)  {
					dprintf(0, ("ST allows "
						"spray width change "
						"only when no data is "
						"currently being "
						"exchanged (%s, %s).\n",
					st_decode_state(sp->s_vc_state),
					st_decode_state(tx->tx_state)));
					error = EPROTO;
				}
				else if(spray != 1 && spray != 2 && 
					spray != 4 && spray != 6 &&
					spray != 8)  {
					dprintf(0, ("Bad spray %d\n",
						spray));
					error = EINVAL;
				}
				else {
					tx->tx_spray_width = spray;
					dprintf(10, ("Tx spray set to %d\n",
						tx->tx_spray_width));
				}
				break;
			}
				
			case ST_RX_SPRAY_WIDTH: {
				char	spray = *mtod(m, u_char *);
				st_rx_t	*rx = &(sp->rx[0]);

				if(sp->s_vc_state != 
						STP_VCS_DISCONNECTED 
					&&
				   rx->rx_state != STP_VCS_DISCONNECTED
					&&
				   rx->rx_state != STP_READY_FOR_RTS)  {
					dprintf(0, ("ST allows "
						"spray width change "
						"only when no data is "
						"currently being "
						"exchanged (%s, %s).\n",
					st_decode_state(sp->s_vc_state),
					st_decode_state(rx->rx_state)));
					error = EPROTO;
				}
				else if(spray != 1 && spray != 2 && 
					spray != 4 && spray != 6 &&
					spray != 8)  {
					dprintf(0, ("Bad spray %d\n",
						spray));
					error = EINVAL;
				}
				else {
					rx->rx_spray_width = spray;
					dprintf(10, ("Rx spray set to %d at 0x%x\n",
						rx->rx_spray_width,
						&(rx->rx_spray_width)));
				}

				break;
			}

			case ST_NUM_SLOTS: {
				u_short	num_slots = *mtod(m, u_short *);

				if(sp->s_vc_state != 
						STP_VCS_DISCONNECTED)  {
					dprintf(0, ("ST allows "
						"num slots change "
						"only before "
						"connection setup "
						"(%s).\n",
					st_decode_state(
						sp->s_vc_state)));
					error = EPROTO;
				}
				else if(! (num_slots % 16) ||
					num_slots < 
					2 * ST_MIN_ALLOWED_SLOTS) {
					dprintf(0, ("Bad num_slots %d\n",
						num_slots));
					error = EINVAL;
				}
				else {
					sp->s_vcd.vc_max_lslots = num_slots;
				}

				break;
			}

			case ST_ROTATE_DATA_VC: {
				sp->s_vcd.vc_flags |= STP_VCD_OUT_VC_ROT;
				break;
			}

			case ST_UNROTATE_DATA_VC: {
				sp->s_vcd.vc_flags &= ~(STP_VCD_OUT_VC_ROT);
				break;
			}


			/* begin bypass setsockopts */

			case ST_BYPASS:
			case ST_L_KEY:
			case ST_L_NUMSLOTS:
			case ST_V_NUMSLOTS:
			case ST_BUFX_ALLOC:
			case ST_BUFX_FREE:
			case ST_BUFX_MAP:
			case ST_BUFX_UNMAP: {
			    error = st_bypass_setopt(op,so,level,optname,mp);
			} break; /* bypass ops */

		}
		break;


	case PRCO_GETOPT:
		*mp = m = m_get(M_WAIT, MT_SOOPTS);
		m->m_len = sizeof(int);

		switch(optname) {
			default: {
				dprintf(0, ("Unknown option in ST-getsockopt\n"));
				error = EINVAL;
				break;
			}

			/* kernel sockets */
			case ST_CTS_OUTSTD:  {
				*mtod(m, ushort *) = sp->tx[0].tx_ctsreq;
				break;
			}
			case ST_BLKSZ: {
				*mtod(m, ushort *) = sp->blocksize;
				break;
			}
			case ST_BUFSZ: {
				*mtod(m, uint *) = sp->s_vcd.vc_lbufsize;
				break;
			}
			case ST_OUT_VCNUM: {
				*mtod(m, u_char *) = sp->s_vcd.vc_out_vcnum;
				break;
			}
			case ST_TX_SPRAY_WIDTH: {
				*mtod(m, u_char *) = sp->tx[0].
							tx_spray_width;
				break;
			}
			case ST_RX_SPRAY_WIDTH: {
				*mtod(m, u_char *) = sp->rx[0].
							rx_spray_width;
				break;
			} 
			case ST_NUM_SLOTS: {
				*mtod(m, uint *) = sp->s_vcd.
							vc_max_lslots;
				break;
			} 

			case ST_MEMALLOC_POOL: {
			  if (sp->s_ifp) 
			    *mtod(m, short *) = sp->s_ifp->if_unit;
			  else {
			    cmn_err(CE_WARN,
				    "Socket not connected.  Cannot return "
				    "MEMALLOC_POOL association.\n");
			    error = ENOTCONN;
			  }
			  break;
			}

			/* begin bypass getsockopts */

			case ST_BYPASS:
			case ST_L_KEY:
			case ST_R_KEY:
			case ST_L_PORT:
			case ST_R_PORT:
			case ST_L_NUMSLOTS:
			case ST_R_NUMSLOTS:
			case ST_V_NUMSLOTS:
			case ST_L_MAXSTU:
			case ST_R_MAXSTU:
			case ST_L_BUFSIZE:
			case ST_R_BUFSIZE: {
			    error = st_bypass_getopt(op,so,level,optname,mp);
			} break;

		}
		break;
	}

	if(op == PRCO_SETOPT && m)  {
		m_free(m);	/* does this sleep? */
	}

	return error;
}
