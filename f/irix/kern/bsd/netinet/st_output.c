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
 *  Filename: st_output.c
 *  Description: routines that prepare a packet being sent using
 *		the ST protocol.
 *
 *  $Revision: 1.9 $
 *
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
#include "sys/errno.h"

#include "in.h"
#include "in_systm.h"
#include "ip.h"
#include "in_pcb.h"
#include "in_var.h"
#include "ip_icmp.h"

#include "sys/tcpipstats.h"
#include <sys/cmn_err.h>
#include "st.h"
#include "st_var.h"
#include "st_macros.h"
#include "st_bufx.h"
#include "st_debug.h"
#include "st_if.h"

#include <sys/ioccom.h>

#define	EARLY_SLOTS_NOTIFY

extern char *inet_ntoa(struct in_addr);
extern	char	st_debug_payload[];

int strt_old = 0; /* Old routing choice ? */

int
st_findroute(struct socket *so)
{

	register struct inpcb *inp = sotoinpcb(so);
	register struct stpcb *stp = sotostpcb(so);
	int error = 0;
	struct sockaddr_in *dst = (struct sockaddr_in *) &stp->s_dst;
	struct route *ro;
	
	DPRINTF(ST_DEBUG_ENTRY,("ENTRY: st_findroute()\n"));
	ASSERT(SOCKET_ISLOCKED(so));

	/* ST has no "addressing" but coexists in the IP namespace.
	 *
	 * Thus we mask ourselves as in the INET family to find our device,
	 * but then use PF_STP on the socket dest addr to differentiate 
	 * ourselves to the ifp_output routine. 
	 *
	 * It makes no sense to modify ifconfig, etc. to allow for
	 * PF_STP.  STP has no addressing based on the specification.
	 */
	if (NULL == stp->s_ifp) {
		struct ifaddr *ifa;
		struct in_ifaddr *ia;
		
		dst->sin_family = AF_INET;
		dst->sin_addr = inp->inp_iap.iap_faddr;

		dprintf(30, ("finding route to 0x%x \n", 
			     dst->sin_addr));

		if (strt_old) {		
			if (((ifa = ifa_ifwithdstaddr((struct sockaddr *) dst)) == 0) 
			    && ((ifa = ifa_ifwithnet((struct sockaddr *) dst)) == 0)) {
				IPSTAT(ips_noroute);
				return ENETUNREACH;
			}
		} else {
			struct ifaddr *ifa2;
			if (((ifa2 = ifa_ifwithdstaddr((struct sockaddr *) dst)) == 0) 
			    && ((ifa2 = ifa_ifwithnet((struct sockaddr *) dst)) == 0)) {
				dprintf(5, ("st_findroute: not connected directly!\n"));
			}
			/*
			 * If routing to interface only, short circuit routing lookup.
			 * Route packet.
			 */
			ro=&inp->inp_route;
			ROUTE_RDLOCK();
			if (ro == NULL) { 
			  ro = (struct route *) malloc (sizeof(struct route));
			  bzero((caddr_t)ro, sizeof (*ro));
			} 
			ro->ro_dst = stp->s_dst;
	
			if (!ro->ro_rt)
				rtalloc(ro); /* !! */
			if (ro->ro_rt->rt_flags & RTF_GATEWAY) {
			} 
			dprintf(5, ("final dest: %s    s_dst: %s  gtwy: %s\n",
				    inet_ntoa(((struct sockaddr_in *) &ro->ro_dst)->sin_addr),
				    inet_ntoa(((struct sockaddr_in *) &stp->s_dst)->sin_addr),
				    inet_ntoa(((struct sockaddr_in *) ro->ro_rt->rt_gateway)->sin_addr)));
			
			ASSERT_ALWAYS((&inp->inp_route)->ro_rt); 
			ASSERT_ALWAYS((&inp->inp_route)->ro_rt->rt_ifa); 
			ifa = (&inp->inp_route)->ro_rt->rt_ifa;

			if (ifa != ifa2) printf(" ifa 0x%x != ifa2 0x%x \n",ifa,ifa2);
			dprintf(5, ("st_output:OUT faddr: %x fport: %d laddr: %x lport: %d;\n",
				    inp->inp_iap.iap_faddr,    inp->inp_iap.iap_fport,
				    inp->inp_iap.iap_laddr,    inp->inp_iap.iap_lport));

			ROUTE_UNLOCK();
		}

		dst->sin_family = PF_STP;
		
		ia = (struct in_ifaddr *) ifa->ifa_start_inifaddr;
		stp->s_ifp = ia->ia_ifp; 

		dprintf(20, ("st_findroute: in_ifaddr 0x%x if: %s%d\n", 
			     ia, ia->ia_ifp->if_name, ia->ia_ifp->if_unit));
		
		(*stp->s_ifp->if_ioctl)
			(stp->s_ifp, STP_SIOC_GETSTIFNET, 
							&stp->s_stifp);
		if(stp->s_stifp == NULL)  {
		   	printf("st_ifnet is NULL\n");
		    	return EPROTONOSUPPORT;		
		}
	}
	return error;
}


int
st_send_data(struct stpcb *sp, sthdr_t *sth, st_tid_t tid)
{
	sthdr_cts_t     *cts = &(sth->sth_cts);
	sthdr_data_t    *data = NULL;
	st_tx_t		*tx;
	int		offset, cts_len;
	int		error = 0;
	struct mbuf	*m0;
	uint		bufxnum;
	int		frag_size, num_bufxes;
	int		need_RSR = 0;
	struct	socket	*so    = stpcbtoso(sp);


	ASSERT_ALWAYS(tid == 0);
	tx = &(sp->tx[tid]);
	ASSERT_ALWAYS(tx->tx_buf.bufx_flags & BUF_BUFX);
	ASSERT_ALWAYS(tx->tx_spray_width);

	tx->tx_num_cts_seen++;
	offset = cts->B_num * (1 << cts->Blocksize);
	cts_len = min((tx->tx_len_of_send - offset), 
						(1 << cts->Blocksize));

#undef	RS1_CHECK
#ifdef	RS1_CHECK
	printf("Asking for slots (RS1), for no reason\n");
	st_ask_for_slots(sp);
	ASSERT_ALWAYS(0 == WAIT_FOR_SLOTS(sp));
	printf("Done, asking for slots\n");
#endif	/* RS1_CHECK */

	dprintf(30, ("cts_len is %d [min (%d, %d)]\n", 
		cts_len, tx->tx_tlen, (1 << cts->Blocksize)));

	if(cts_len == (tx->tx_len_of_send - offset)) {
		need_RSR = (tx->tx_flags & TX_RTS_SENDSTATE)? 1 : 0;
		dprintf(10, ("need RSR %d; cts_len %d, tx_tlen %d, flags 0x%x \n",
			need_RSR, cts_len, tx->tx_tlen,
			tx->tx_flags));
	}

#ifdef	EARLY_SLOTS_NOTIFY
	if(SLOTS_ENABLED(sp) && 
		(sp->s_vcd.vc_rslots == 
				(sp->s_vcd.vc_max_rslots >> 1))) {
		need_RSR = 1;
		dprintf(10, ("need RSR for early-notify; cts_len %d, "
				"tx_tlen %d, rslots %d, max %d\n",
			cts_len, tx->tx_tlen, sp->s_vcd.vc_rslots,
			sp->s_vcd.vc_max_rslots));
	}
#endif	/* EARLY_SLOTS_NOTIFY */

	bufxnum = st_len_to_bufxnum(tx->tx_spray_width, offset,
			tx->tx_buf.st_addr_bufx.bufx_t.bufx,
					tx->tx_buf.bufx_cookie);

	dprintf(10, ("R_Bufx on CTS is: %u, bnum %u\n", 
		cts->R_Bufx, cts->B_num));

	dprintf(30, ("bufx obtained for xfer is: %d at len %u\n",
			bufxnum, offset));

	ASSERT(sp->s_vcd.vc_rslots >= ST_MIN_ALLOWED_SLOTS);
	
	/* slap on an ST hdr and send out the payload mbuf */
	m0 = m_getclr(M_DONTWAIT, MT_HEADER);
	if (NULL == m0) {
	  	return -1;
	}

	m0->m_len  = sizeof(struct st_io_s) + sizeof(if_st_tx_desc_t);
	m0->m_next = NULL;
	m0->m_off  = MMINOFF;
	data = (sthdr_data_t *) (mtod(m0, caddr_t) +
				 sp->s_stifp->xmit_desc_sthdr_off);
	data->OpFlags = ST_DATA | ST_INTERRUPT | ST_LAST;
	data->OpFlags |= sp->s_vcd.vc_out_vcnum;
	if(need_RSR)  {
		data->OpFlags |= ST_SENDSTATE;
	}
#ifdef	TEST_661337
	dprintf(0, ("Setting silent flag\n"));
	data->OpFlags |= ST_SILENT;
#endif	/* TEST_661337 */
	/* SHAC wants a zero STU-num, unless we are doing
	*  explicit tiling of our own */
	data->STU_num = 0;
	data->R_Port = cts->R_Port;
	data->I_Port = cts->I_Port;
	data->R_Key = sp->s_vcd.vc_rkey;
	data->R_Mx = cts->R_Mx;
	data->R_Bufx = cts->R_Bufx;
	data->R_Offset = cts->R_Offset;
	data->Sync = sp->s_vcd.vc_lsync;
	sp->s_vcd.vc_lsync = (sp->s_vcd.vc_lsync + 1) & 0x0ffff;
	data->B_num = cts->B_num;
	tx->tx_max_Bnum_sent = max(data->B_num, tx->tx_max_Bnum_sent);
	data->R_id = cts->R_id;
	data->Checksum = 0x0;

	dprintf(10, ("DATA: Rid: 0x%x; CTS: Rid 0x%x, Iid 0x%x\n",
		data->R_id, cts->R_id, cts->I_id));

	{
	st_macaddr_t *stmacp;
	if_st_tx_desc_t *ptxdesc;
	uint 	dest_bufsize = (sp->s_vcd.vc_rbufsize) ? 
			sp->s_vcd.vc_rbufsize : ST_LOG_BUFSZ;

	ptxdesc = (if_st_tx_desc_t *)(mtod(m0, caddr_t) +
			      sp->s_stifp->xmit_desc_ifhdr_off);

	dprintf(30, ("dest bufsz is %u (%u, %u) \n",
		dest_bufsize, sp->s_vcd.vc_rbufsize,
		ST_LOG_BUFSZ));
	/* TODO:  For now allow only on data VC per connection */
	ptxdesc->vc = sp->s_vcd.vc_out_vcnum;
	ptxdesc->flags = IF_ST_TX_ACK | IF_ST_USER_FIFO;
	/* ptxdesc->flags = IF_ST_TX_ACK; */
	ptxdesc->dst_bufsize = dest_bufsize;	
	ptxdesc->max_STU = sp->s_vcd.vc_rmaxstu;
	ptxdesc->len = cts_len;
	dprintf(30, ("ptx len is %u\n", cts_len));
	ptxdesc->token = 0xcafebabe;


	stmacp = (st_macaddr_t*)cts - 1;
	ptxdesc->flags |= IF_ST_TX_MAC_ADDR;
	ptxdesc->dst_MAC = stmacp->src_mac;
	dprintf(20, ("forcing D_ULA cookie to 0x%x\n", stmacp->src_mac));

	frag_size = st_bufx_to_frag_size(
			tx->tx_buf.st_addr_bufx.bufx_t.bufx,
				tx->tx_buf.bufx_cookie);
	if(cts_len % frag_size)  {
		num_bufxes = cts_len/frag_size + 1;
	}
	else {
		num_bufxes = cts_len/frag_size;
	}
	num_bufxes /= tx->tx_spray_width;


#	ifdef	DEBUG
	if(sp->s_stifp->if_st_get_port) {
		st_port_t	st_port;

		ASSERT_ALWAYS(0 == ((*sp->s_stifp->if_st_get_port)(
			sp->s_ifp, sp->s_vcd.vc_lport, &st_port)));
		/* ASSERT_ALWAYS(bufxnum >= st_port.bufx_base
				&& bufxnum <= (st_port.bufx_base + 
						st_port.bufx_range)); */
		if(bufxnum < st_port.bufx_base ||
			(bufxnum + num_bufxes - 1) > 
			(st_port.bufx_base + st_port.bufx_range)) {
			cmn_err(CE_PANIC, "Bad bufx range "
				"[%d, %d] (frag sz %d) in "
				"st_output; port base %d (0x%x), "
				"port range %d on port %d\n",
				bufxnum, 
				bufxnum + num_bufxes - 1,
					frag_size,
					st_port.bufx_base,
					st_port.bufx_base,
					st_port.bufx_range,
					sp->s_vcd.vc_lport);
		}
		dprintf(20, ("bufxnum is in port range\n"));
	}
#	endif	/* DEBUG */


	ptxdesc->bufx = bufxnum;
	ptxdesc->offset = tx->tx_buf.st_addr_bufx.bufx_t.offset;
	ptxdesc->addr = NULL;

  	dprintf(10, ("st_send_data from: bufx: 0x%x "
			"until bufx  0x%x, "
			"off 0x%x, len %u, VC %d; RSR %d \n",
	      	ptxdesc->bufx, ptxdesc->bufx + num_bufxes - 1,
		ptxdesc->offset, ptxdesc->len,
		(int) ptxdesc->vc, need_RSR));
	dprintf(10, ("dest bufsz is %u (%u, %u) \n",
		dest_bufsize, sp->s_vcd.vc_rbufsize,
		ST_LOG_BUFSZ));
	}

	STSTAT_ADD(stps_datatxtotal, cts_len);
	dprintf(10, ("st_send_data to: R_bufx: 0x%x (%d) "
			" R_offset: 0x%x\n",
		    data->R_Bufx, data->R_Bufx));


#ifdef	ST_HDR_DUMP
	dprintf(0, ("Dumping hdr in send_data\n"));
#endif 	/* ST_HDR_DUMP */
	ST_DUMP_HDR((sthdr_t *) data);

	error = st_output(m0, sp->s_so);
	st_set_timer(sp, TID_TO_TIMER_ID(tid), TX_TIMEOUT_VAL);

	if(! error)  {
		tx->tx_tlen -= cts_len;
		dprintf(10, ("port %d sent len %d, BNum %d (max %d) "
			"(offset %u) to Port %d, need_RSR %d\n",
			sp->s_inp->inp_lport,  cts_len, cts->B_num, 
			tx->tx_max_Bnum_required, offset,
			sp->s_inp->inp_fport, need_RSR));
	}
	else {
		dprintf(0, ("Port %d couldn't send PAYLOAD: len %d to Port %d\n",
			sp->s_inp->inp_lport, cts_len,
			sp->s_inp->inp_fport));
	}		

	/* ASSERT_ALWAYS((int) tx->tx_tlen >= 0); */
	if((int) tx->tx_tlen < 0)  {
		printf("tx_tlen %d cts len %d, max bnum %d, data-bnm %d,\n",
			(int) tx->tx_tlen, cts_len, tx->tx_max_Bnum_sent,
			data->B_num);
		st_dump_data_CTS_tab(tx);
		cmn_err(CE_PANIC, "tx_tlen negative (%d); cts len %d, "
			"max sent bnum %d, data-bnm %d, "
			"max expected bnum %d, num CTS seen %d\n",
			(int) tx->tx_tlen, cts_len, tx->tx_max_Bnum_sent,
			data->B_num, tx->tx_max_Bnum_required,
			tx->tx_num_cts_seen);
	}

	
	if(tx->tx_tlen == 0) {
		if(tx->tx_ACKed && 
				tx->tx_max_Bnum_sent == tx->tx_acked_Bnum)  {
			dprintf(0, ("Already-ACKed tx (ACK-Bnum %d)\n",
				tx->tx_acked_Bnum));
			dprintf(0, ("Waking up so 0x%x, buf 0x%x\n",
				so, so->so_snd));
			/* sowwakeup(sp->s_so, NETEVENT_STPPUP); */
			SOCKET_SBWAIT_WAKEALL(&so->so_snd);
		}
		else {
			dprintf(30, ("TX len drop to 0; "
				"will change state at RSR\n"));
		}
	}
	else {
		tx->tx_state = STP_DATA_SEND;
	}

#if	0
	if(need_RSR)  {
		dprintf(0, ("Asking for RSR\n"));
		st_ask_for_ack(sp, tid);
	}
#endif	/* 0 */

	return error;
}


int
st_output(struct mbuf *m0, struct socket *so)
{
  	register struct stpcb  	*stp = sotostpcb(so);
	sthdr_t			*sth;
	sthdr_data_t    	*data;
	ushort			opcode, OpFlags;
  	int                     error = 0;
	int			i;
	struct sockaddr_in      *dst = (struct sockaddr_in *) &stp->s_dst;

  	DPRINTF(ST_DEBUG_ENTRY,("ENTRY: st_output()\n"));
  	ASSERT(SOCKET_ISLOCKED(so));

  	if (NULL == stp->s_ifp) {
	  	dprintf(20, ("No route for PCB 0x%x...\n", stp));
	  	error = st_findroute(so);
	
	  	if (error) {
			dprintf(0, ("st_findroute returned error\n"));
	  		return error;
	  	}
  	}

  	STSTAT(stps_txtotal);
  
  	DPRINTF(ST_DEBUG_ENTRY,("ENTRY: st_output() outputting\n"));    

#ifdef	ST_HDR_DUMP
	dprintf(0, ("Dump header in st_output\n"));
#endif	/* ST_HDR_DUMP */
	sth = (sthdr_t *) ((mtod(m0, caddr_t) +
			stp->s_stifp->xmit_desc_sthdr_off));
	ST_DUMP_HDR(sth);

	data = (sthdr_data_t *) sth;
	OpFlags = data->OpFlags;
	opcode = OpFlags & ST_OPCODE_MASK;

	if(SLOTS_ENABLED(stp) && CONSUMES_SLOT(OpFlags)) {
		if(stp->s_vcd.asked_for_slots)  {
			ASSERT_ALWAYS(0 == WAIT_FOR_SLOTS(stp));
		}
		if(OUT_OF_SLOTS(stp)) {
			st_ask_for_slots(stp);
			ASSERT_ALWAYS(0 == WAIT_FOR_SLOTS(stp));
			if(OUT_OF_SLOTS(stp)) {
				dprintf(0, ("No slots after sync-up; "
					"dropping %s packet\n",
					st_decode_opcode(opcode)));
				return -1;
			}
		}
		stp->s_vcd.vc_rslots--;
		/* CHECK_SLOTS(stp); */
		if(! GOOD_SLOTS(stp))  {
			SLOTS_PANIC(stp);
		}
		dprintf(4, ("Op %s; rslots dec to %d\n",
			st_decode_opcode(opcode),
			stp->s_vcd.vc_rslots));
	}

	/* assume an error returned in a transient condition due to backed
	 * up tx credits, so immediately retry rather than drop.
	 */
#define MAX_RETRIES 20
	for (i = 0; i < MAX_RETRIES; i++) {
		error = (*stp->s_ifp->if_output)(
				stp->s_ifp, m0, &stp->s_dst, NULL);
		
		if (!error)  {
			DPRINTF(ST_DEBUG_ENTRY,("ENTRY: st_output() done outputting\n"));
			break;
		}
		else {
			DPRINTF(ST_DEBUG_ENTRY,("st_output if_output returned %d\n", error));
		}
	}

	if (i != 0)
		dprintf(0, ("st_output if_output retried %d times\n", i));
#undef MAX_RETRIES

#ifdef	EARLY_SLOTS_NOTIFY
	if(opcode != ST_RS && opcode != ST_RCONNECT
			&& (SLOTS_ENABLED(stp) &&
			(stp->s_vcd.vc_rslots 
				== (stp->s_vcd.vc_max_rslots >> 1)))) {
		dprintf(10, ("Slots are low (%d), but not at limit; "
			"Asking for slots without waiting\n",
			stp->s_vcd.vc_rslots));
		st_ask_for_slots(stp);
		/* don't wait for slots */
		/* ASSERT_ALWAYS(0 == WAIT_FOR_SLOTS(stp)); */
	}
#endif 	/* EARLY_SLOTS_NOTIFY */


  	return error;
}
