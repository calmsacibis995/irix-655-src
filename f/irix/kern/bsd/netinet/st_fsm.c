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
 *  Filename: st_fsm.c
 *  Description: Finite State Machines for Virtual connection and 
 *			Data Transfer in the ST protocol.
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


void
st_data_init_tx_state(st_tx_t *tx, st_state_t state)
{
	tx->tx_state = state;
}

void
st_data_init_rx_state(st_rx_t *rx, st_state_t state)
{
	rx->rx_state = state;
}


int
stc_hdr_template(struct stpcb *sp, ushort OpFlags, sthdr_t *sth,
							short tid)
{
	int error = 0;
	ushort	opcode = OpFlags & ST_OPCODE_MASK;
	ushort	flags = OpFlags & ST_FLAG_MASK;

	dprintf(10, ("stc_hdr_template: opcode is 0x%x (%s), tid %d\n", 
		opcode, st_decode_opcode(opcode), tid));

	ASSERT_ALWAYS(SOCKET_ISLOCKED(sp->s_so));
	switch(opcode) {
	case ST_RCONNECT:
		{
			sthdr_rc_t *rc = &(sth->sth_rc);
			uint	bufsize = (sp->s_vcd.vc_lbufsize)?
				sp->s_vcd.vc_lbufsize : ST_LOG_BUFSZ;
			u_char	spray = (sp->tx[0].tx_spray_width) ?
					(sp->tx[0].tx_spray_width) : 1;
				
			
			rc->OpFlags     = ST_RCONNECT | ST_INTERRUPT | ST_ORDER; 
			if ( sp->s_flags & (STP_SF_BYPASS|STP_SF_USERVISSLOTS) ) {
			    /* do some sanity checks */
			    if ( sp->s_vcd.vc_vslots == 0xffff ) {
				/* always honor "don't care" */
				rc->I_Slots = sp->s_vcd.vc_vslots;
			    } else if ( sp->s_vcd.vc_vslots < sp->s_vcd.vc_true_max_lslots ) {
				/* ok to expose fewer slots than ddq size */
				/* conditional is there just in case a 0 sneaks in */
				rc->I_Slots = (sp->s_vcd.vc_vslots) ?
				    sp->s_vcd.vc_vslots : sp->s_vcd.vc_true_max_lslots;
			    } else {
				/* show how many we really have */
				rc->I_Slots = sp->s_vcd.vc_true_max_lslots;
			    }
			} else if ( sp->s_flags & STP_SF_BYPASS ) {
			    /* bypass does not send -1 by default */
			    rc->I_Slots = sp->s_vcd.vc_true_max_lslots;
			} else {
			    rc->I_Slots = (sp->s_vcd.vc_max_lslots) ?
				sp->s_vcd.vc_max_lslots : -1;
			}
			/* copy exported value back to vcd */
			sp->s_vcd.vc_vslots = rc->I_Slots;

			dprintf(20, ("RC: I_Slots set to 0x%x (%d)\n", 
				rc->I_Slots, rc->I_Slots));
			rc->R_Port      = sp->s_vcd.vc_rport;
			rc->I_Port      = sp->s_vcd.vc_lport;
			rc->I_Bufsize   = log2(spray) + bufsize;
			dprintf(30, ("I_Bufsize set to %u in RC\n",
				rc->I_Bufsize));
			rc->I_Key       = sp->s_vcd.vc_lkey;
			rc->I_Max_Stu   = min(sp->s_vcd.vc_lmaxstu, rc->I_Bufsize);
			rc->EtherType   = sp->s_vcd.vc_ethertype;
			break;
		}
	case ST_CANSWER:
		{
			sthdr_ca_t *ca = &(sth->sth_ca);
			uint	bufsize = (sp->s_vcd.vc_lbufsize)?
				sp->s_vcd.vc_lbufsize : ST_LOG_BUFSZ;
			u_char	spray = (sp->rx[0].rx_spray_width) ?
					(sp->rx[0].rx_spray_width) : 1;
			
			ca->OpFlags     = ST_CANSWER | ST_INTERRUPT | ST_ORDER;
			if ( sp->s_flags & (STP_SF_BYPASS|STP_SF_USERVISSLOTS) ) {
			    /* do some sanity checks */
			    if ( sp->s_vcd.vc_vslots == 0xffff ) {
				/* always honor "don't care" */
				ca->R_Slots = sp->s_vcd.vc_vslots;
			    } else if ( sp->s_vcd.vc_vslots < sp->s_vcd.vc_true_max_lslots ) {
				/* ok to expose fewer slots than ddq size */
				/* conditional is there just in case a 0 sneaks in */
				ca->R_Slots = (sp->s_vcd.vc_vslots) ?
				    sp->s_vcd.vc_vslots : sp->s_vcd.vc_true_max_lslots;
			    } else {
				/* show how many we really have */
				ca->R_Slots = sp->s_vcd.vc_max_lslots;
			    }
			} else if ( sp->s_flags & STP_SF_BYPASS ) {
			    /* bypass does not send -1 by default */
			    ca->R_Slots = sp->s_vcd.vc_true_max_lslots;
			} else {
			    ca->R_Slots = (sp->s_vcd.vc_max_lslots) ?
				sp->s_vcd.vc_max_lslots : -1;
			}
			/* copy exported value back to vcd */
			sp->s_vcd.vc_vslots = ca->R_Slots;

			dprintf(20, ("CA: R_Slots set to 0x%x (%d)\n", 
				ca->R_Slots, ca->R_Slots));
			ca->I_Key       = sp->s_vcd.vc_rkey;
			ca->I_Port      = sp->s_vcd.vc_rport;
			ca->R_Port      = sp->s_vcd.vc_lport;
			ca->R_Bufsize   = log2(spray) + bufsize;
			dprintf(30, ("R_Bufsize set to %u in CA\n",
				ca->R_Bufsize));
			ca->R_Key       = sp->s_vcd.vc_lkey;
			ca->R_Max_Stu   = min(sp->s_vcd.vc_lmaxstu,  ca->R_Bufsize);
			break;
		}
	case ST_RDISCONNECT:
		{
			sthdr_rd_t *rd = &(sth->sth_rd);
			
			rd->OpFlags     = ST_RDISCONNECT | ST_INTERRUPT;
			rd->R_Port      = sp->s_vcd.vc_rport;
			rd->I_Port      = sp->s_vcd.vc_lport;
			rd->R_Key       = sp->s_vcd.vc_rkey;
			rd->I_Key       = sp->s_vcd.vc_lkey;
			break;
		}
	case ST_DANSWER:
		{
			sthdr_da_t *da = &(sth->sth_da);
			
			da->OpFlags     = ST_DANSWER | ST_INTERRUPT;
			da->I_Port      = sp->s_vcd.vc_rport;
			da->R_Port      = sp->s_vcd.vc_lport;
			da->R_Key       = sp->s_vcd.vc_lkey;	
			da->I_Key       = sp->s_vcd.vc_rkey;	
			break;
		}
	case ST_DCOMPLETE:
		{
			sthdr_dc_t *dc = &(sth->sth_dc);
			
			dc->OpFlags     = ST_DCOMPLETE | ST_INTERRUPT;
			dc->R_Port      = sp->s_vcd.vc_rport;
			dc->I_Port      = sp->s_vcd.vc_lport;
			dc->R_Key       = sp->s_vcd.vc_rkey;	
			dc->I_Key       = sp->s_vcd.vc_lkey;	
			break;
		}
	case ST_RTS:
		{
			sthdr_rts_t	*rts = &(sth->sth_rts);
			st_tx_t		*tx;
			u_short		cts_req;

			dprintf(15, ("stc_hdr_template: forming ST_RTS\n"));
			if(tid != 0)  {
				dprintf(0, ("ST_RTS: Got non-zero tid\n"));
			}
			tx = &(sp->tx[tid]);
			cts_req = (tx->tx_ctsreq) ?
				tx->tx_ctsreq : ST_NUM_CTS_OUTSTANDING;
			rts->OpFlags    = ST_RTS | ST_INTERRUPT | ST_SENDSTATE;
			tx->tx_flags |= TX_RTS_SENDSTATE;

			
			ASSERT_ALWAYS(sp->s_vcd.vc_out_vcnum);
			if (sp->s_vcd.vc_out_vcnum) 
			  	rts->OpFlags |= sp->s_vcd.vc_out_vcnum;

			rts->CTS_req    = cts_req;
			dprintf(10, ("CTS_req set to %d in RTS\n",
				rts->CTS_req));
			rts->R_Port     = sp->s_vcd.vc_rport;
			rts->I_Port     = sp->s_vcd.vc_lport;
			rts->R_Key      = sp->s_vcd.vc_rkey;	
			rts->Max_Block  = (sp->blocksize)? 
					sp->blocksize : BLOCK_SIZE;
			dprintf(10, ("RTS max block made %u\n",
						rts->Max_Block));
			rts->tlen 	= tx->tx_tlen;
			rts->I_id 	= tx->tx_iid;
			break;
		}
	case ST_CTS:
		{
			sthdr_cts_t	*cts = &(sth->sth_cts);
			st_rx_t		*rx;
			uint		payload_offset;
			extern	volatile  char     st_R_Mx_tab[];

			/* we don't do asynch xfer yet */
			ASSERT_ALWAYS(tid == 0);
			ASSERT_ALWAYS(rx_is_valid(sp, tid));
			rx = &(sp->rx[tid]);
			cts->OpFlags    = ST_CTS | ST_INTERRUPT;
			cts->I_Port     = sp->s_vcd.vc_rport;
			cts->R_Port     = sp->s_vcd.vc_lport;
			cts->I_Key      = sp->s_vcd.vc_rkey;	
			cts->R_Mx       = rx->rx_mx_tmp;
			/* ASSERT_ALWAYS(GOODMX(cts->R_Mx)); */
			if(!GOODMX(cts->R_Mx))  {
				cmn_err(CE_PANIC, "Bad MX %d (%d); tab 0x%x\n",
				cts->R_Mx, cts->R_Mx >> 3,
				st_R_Mx_tab[cts->R_Mx >> 3]);
			}
			cts->Blocksize  = rx->rx_blocksize;
			dprintf(10, ("CTS blocksz made %u\n",
						cts->Blocksize));
			cts->B_num  	= rx->cur_bnum;
			dprintf(30, ("CTS bnum %d\n", cts->B_num));
			ASSERT_ALWAYS(! IS_SET_RX_CTS(rx, cts->B_num));
			SET_RX_CTS_TAB(rx, cts->B_num);
			ASSERT_ALWAYS(IS_SET_RX_CTS(rx, cts->B_num));
			rx->cur_bnum++;
			dprintf(30, ("CTS, bnum %d\n", cts->B_num));
			cts->R_Offset 	= 
				rx->rx_buf.st_addr_bufx.bufx_t.offset;
			if(0 == cts->B_num) {
				rx->rx_first_offset = cts->R_Offset;
			}
			cts->F_Offset  	= rx->rx_first_offset;;
			payload_offset 	= cts->B_num * (1 << cts->Blocksize);
			cts->R_Bufx     = 
				st_len_to_bufxnum(
				rx->rx_spray_width, payload_offset,
				rx->rx_buf.st_addr_bufx.bufx_t.bufx,
				rx->rx_buf.bufx_cookie);
			dprintf(10, ("CTS: R_Bufx is %d\n", cts->R_Bufx));
			ASSERT(cts->R_Bufx != -1);
			cts->I_id 	= rx->rx_iid;
			cts->R_id 	= rx->rx_rid;
			dprintf(20, ("CTS: rx_iid %u, rx_rid %u at 0x%x\n",
				rx->rx_iid, rx->rx_rid,
				&(rx->rx_rid)));
			break;
		}
	case ST_RANSWER:
		{
			sthdr_ra_t	*ra = &(sth->sth_ra);
			st_kx_t		*kx;
			st_rx_t		*rx = &(sp->rx[tid]);

			if(tid != 0)  {
				dprintf(0, ("ST_RANSWER: Got non-zero tid\n"));
			}
			kx = &(sp->kx[tid]);
			ra->OpFlags    = ST_RANSWER | flags | ST_INTERRUPT;
			ra->I_Port     = sp->s_vcd.vc_rport;
			ra->R_Port     = sp->s_vcd.vc_lport;
			ra->I_Key      = sp->s_vcd.vc_rkey;	
			if(!(flags & ST_REJECT))  {
				ra->I_id       = kx->saved_rts.I_id;
			}
			dprintf(20, ("marking I_id of RANSWER as %d\n",
				ra->I_id));
			/* ASSERT_ALWAYS((flags & ST_REJECT) || ra->I_id); */
			if(!((flags & ST_REJECT) || ra->I_id)) {
				dprintf(0, ("Bad I_id in RANSWER\n"));
				st_dump_hdr(sth);
				dprintf(0, ("Kx is:\n"));
				st_dump_kx(kx);
				st_dump_pcb(sp);
				st_dump_rx(rx);
				ASSERT_ALWAYS(0);
			}
			break;
		}
	default:
		cmn_err(CE_WARN, "Invalid opcode %s (0x%x) in stc_hdr_template\n",
			st_decode_opcode(opcode), opcode);
		error = -1; /* Doh! */
	}
	
	return(error);
}


int
st_rs_template(struct stpcb *sp, ushort OpFlags, sthdr_t *sth, 
							uchar_t type)
{
	int		error = 0;
	ushort	opcode = OpFlags & ST_OPCODE_MASK;
	st_tid_t	rid;
	st_rx_t		*rx;

	switch (opcode) {
	default: {
		error = -1;
		cmn_err(CE_WARN, "Invalid opcode %s in st_rs_template\n",
			st_decode_opcode(opcode));
		break;
	}
	case ST_RS: {
		if(type > 3)  {
			error = -1;
			cmn_err(CE_WARN, "Invalid type %u in opcode %s of st_rs_template\n",
				type, st_decode_opcode(opcode));
		}
		else {
			sthdr_rs1_t *rs = &(sth->sth_rs1);

			rs->OpFlags     = ST_RS | ST_INTERRUPT;
			rs->R_Port      = sp->s_vcd.vc_rport;
			rs->I_Port      = sp->s_vcd.vc_lport;
			rs->R_Key       = sp->s_vcd.vc_rkey;	
			rs->Sync 	= sp->s_vcd.vc_lsync;
			sp->s_vcd.vc_lsync = 
				(sp->s_vcd.vc_lsync + 1) & 0x0ffff;
	
			switch (type)  {
				default: {
					cmn_err(CE_PANIC, "Unknown type\n");
					break;
				}
				case ST_STATE_SLOTS:  {
					rs->minus_one = -1;
					break;
				}
				case ST_STATE_XFER:  {
					sthdr_rs2_t *rs2 = 
						(sthdr_rs2_t *) rs;
					rs2->minus_one = -1;
					break;
				}
				case ST_STATE_BLOCK: {
					break;
				}
			}
		}
		break;
	}

	case ST_RSR: {
		sthdr_rsr1_t *rsr = &(sth->sth_rsr1);

		rsr->OpFlags     = ST_RSR | ST_INTERRUPT;
		dprintf(20, ("max lslots %d, lslots %d, R_slots %d\n",
			sp->s_vcd.vc_max_lslots,
			sp->s_vcd.vc_lslots, rsr->R_Slots)); 
		if(SLOTS_ENABLED(sp))  {
			rsr->R_Slots	 = sp->s_vcd.vc_lslots;
			sp->s_vcd.vc_lslots = 0;
			dprintf(15, ("Returning %d slots in RSR-%d, Sync %d\n",
				rsr->R_Slots, type, rsr->Sync));
		}
		else {
			rsr->R_Slots	 = -1;
		}

		rsr->I_Port      = sp->s_vcd.vc_rport;
		rsr->R_Port      = sp->s_vcd.vc_lport;
		rsr->I_Key       = sp->s_vcd.vc_rkey;	

		switch (type)  {
			default: {
				cmn_err(CE_PANIC, "Unknown type\n");
				break;
			}
			case ST_STATE_SLOTS:  {
				sthdr_rsr1_t *rsr1 =
					(sthdr_rsr1_t *) rsr;

				rsr1->minus_one = -1;
				break;
			}
			case ST_STATE_XFER:  {
				sthdr_rsr2_t *rsr2 = 
					(sthdr_rsr2_t *) rsr;

				rid = 0;
				rx = &(sp->rx[rid]);
				rsr2->I_id = rx->rx_iid;
				rsr2->minus_one = -1;
				rsr2->B_seq = get_max_set_bnum(rx);
				break;
			}
			case ST_STATE_BLOCK: {
				sthdr_rsr3_t *rsr3 = 
					(sthdr_rsr3_t *) rsr;

				rid = 0;
				rx = &(sp->rx[rid]);
				rsr3->I_id = rx->rx_iid;
				rsr3->B_num = -1;
				rsr3->B_seq = get_max_set_bnum(rx);
				break;
			}
		}
		break;
	}

	}


	return error;
}


/*
 * This function is expensive.
 *
 * Since CM is not in the performance path, it is wise
 * to enumerate all possible state transitions for now 
 * anyway...
 *
 * Moreover -- there is only a single entry into this
 * fsm:  This should be protected by a socket lock
 * in the kernel.
 *
 */
int
stc_vc_fsm(struct stpcb **spp, ushort OpFlags, sthdr_t *sth,
	   struct inaddrpair *iap) 
{
extern	uint    SHAC_bufx_lows[], SHAC_bufx_highs[];
	int error = 0;
	struct stpcb *sp = *spp;
	ushort	opcode = OpFlags & ST_OPCODE_MASK;

	dprintf(30, ("st_vc_fsm: state is (%s), opcode %s (%s) \n", 
		st_decode_state(sp->s_vc_state),
		st_decode_opcode(opcode), st_decode_opcode(OpFlags)));

	switch(sp->s_vc_state) {
	case STP_VCS_CLOSED:
		switch(opcode) {
		case ST_VOP_ULISTEN:
			sp->s_vc_state = STP_VCS_LISTEN;
			break;
		case ST_VOP_UUNLISTEN:
		case ST_RCONNECT:
		case ST_CANSWER:
		case ST_RDISCONNECT:
		case ST_DANSWER:
		case ST_DCOMPLETE:
		case ST_VOP_UCONNECT:
			stvc_output(sp, ST_RCONNECT, INVALID_TID);
			sp->s_vc_state = STP_VCS_RCSENT;
			break;
		case ST_VOP_UDISCONNECT:
		default:
			DPANIC("stvc_input: rcv\n");
			error = 1;
		}
		break;

	case STP_VCS_LISTEN:
		switch(opcode) {
		case ST_RCONNECT:
			{
				sthdr_rc_t *rc = &(sth->sth_rc);
				int set_gsn_port = 1; 
				st_port_t st_port;

				sp = stvc_newconn(sp, iap);
				*spp = sp;
				
				sp->s_vcd.vc_max_rslots  = 
				sp->s_vcd.vc_rslots  	 = rc->I_Slots;
				sp->s_vcd.vc_true_max_rslots = rc->I_Slots;
				ASSERT_ALWAYS(sp->s_vcd.vc_lport == rc->R_Port);
				sp->s_vcd.vc_rport        =  rc->I_Port;
				sp->s_vcd.vc_rbufsize     =  rc->I_Bufsize;
				sp->s_vcd.vc_rkey         =  rc->I_Key;
				sp->s_vcd.vc_rmaxstu      =  rc->I_Max_Stu;
				sp->s_vcd.vc_ethertype    =  rc->EtherType;
				
				if(sp->s_keyalloc(sp, (ushort) ST_CANSWER)) {
					cmn_err(CE_PANIC,
						"Can't allocate key in ST_CANSWER\n");
				}

				if (NULL == sp->s_ifp) 
				  if( error = st_findroute(sp->s_so)) 
				    return error;


				if (sp->s_stifp->if_st_get_port) {
				  error = (*sp->s_stifp->
					   if_st_get_port)(sp->s_ifp,
							   sp->s_vcd.vc_lport,
							   &st_port);

				  if (st_port.key != 0) 
				    set_gsn_port = 0;
				  else
				    bzero(&st_port, sizeof(st_port));
				}

				if (sp->s_stifp->if_st_set_port &&
				    set_gsn_port) {
					uint	bufsize = 
					(sp->s_vcd.vc_lbufsize) ?
						sp->s_vcd.vc_lbufsize : 
							ST_LOG_BUFSZ;
					u_char	spray = 
					(sp->tx[0].tx_spray_width) ?
					(sp->tx[0].tx_spray_width) : 1;
					uint		num_bufxes;
					
					/* TODO: Find correct values for this */
					bzero(&st_port, sizeof(st_port_t));
					st_port.bufx_base  = 
						SHAC_bufx_lows[spray];
					/* Need to say the # of bufxes 
					 * we need (not the # of valid
					 * bufx avaiable) */
					num_bufxes = SHAC_bufx_highs[spray]
						- st_port.bufx_base + 1;
					st_port.bufx_range  = 
							num_bufxes;
					dprintf(30, ("FSM set_port: "
					     "bufx_range set to (0x%x), "
					     " start 0x%x, spray %d\n",
						st_port.bufx_range, 
						st_port.bufx_base,
						spray));
					st_port.src_bufsize = 
						log2(spray) + bufsize;
					dprintf(10, ("bufsz set to %d in set_port\n",
						st_port.src_bufsize));

					/* make sure this mirrors the code in stvc_connect */
					if ( sp->s_flags & STP_SF_BYPASS ) {
					    	st_port.vc_fifo_credit[0] = 
							VC0_FIFO_CREDITS;
					    	st_port.vc_fifo_credit[1] = 
							VC1_FIFO_CREDITS;
					    	st_port.vc_fifo_credit[2] = 
							VC2_FIFO_CREDITS;
					    	st_port.vc_fifo_credit[3] = 
							VC3_FIFO_CREDITS;
						ASSERT_ALWAYS(
							sp->s_vcd.vc_max_lslots % 16
						);
						if(sp->s_flags & STP_SF_USERSLOTS ) {
					    		st_port.ddq_size =
							    128 * sp->s_vcd.vc_max_lslots;
							sp->s_vcd.vc_true_max_lslots =
							    sp->s_vcd.vc_max_lslots;
						} else {
							st_port.ddq_size = 128 * ST_DDQ_NUM_SLOTS;
							sp->s_vcd.vc_true_max_lslots =
							    ST_DDQ_NUM_SLOTS;
						}
					    	st_port.ddq_addr = NULL;
					    	st_port.key = sp->s_vcd.vc_lkey;
					} else {
					    	st_port.vc_fifo_credit[0] = 
							VC0_FIFO_CREDITS;
					    	st_port.vc_fifo_credit[1] = 
							VC1_FIFO_CREDITS;
					    	st_port.vc_fifo_credit[2] = 
							VC2_FIFO_CREDITS;
					    	st_port.vc_fifo_credit[3] = 
							VC3_FIFO_CREDITS;
						ASSERT_ALWAYS(
						    sp->s_vcd.vc_max_lslots % 16);
						/** CHANGE this  
					    	st_port.ddq_size = 
						    128 * 
						    sp->s_vcd.vc_max_lslots;
						sp->s_vcd.vc_true_max_lslots =
						    sp->s_vcd.vc_max_lslots;
						**/
					    	st_port.ddq_size = 
						    128 * 
						    ST_DDQ_NUM_SLOTS;
						sp->s_vcd.vc_true_max_lslots =
						    ST_DDQ_NUM_SLOTS;

					    	st_port.ddq_addr = NULL;
					    	st_port.key = sp->s_vcd.vc_lkey;
					}

					if (sp->s_stifp->if_st_clear_port) {
					  error = (*sp->s_stifp->
						   if_st_clear_port)(
								     sp->s_ifp,
								     sp->s_vcd.vc_lport);
					  if (error) {
					    dprintf(0, ("ST_RCONNECT: "
							"clear_port\n"));
					    return error;
					  }
					}  

					error = (*sp->s_stifp->
						if_st_set_port)(
							sp->s_ifp,
						       	sp->s_vcd.
							vc_lport,
							&st_port);

					if(error)  {
						dprintf(0, ("ST_RCONNECT: "
							"set_port "
							"erred\n"));
						return error;
					}
				}

				stvc_output(sp, ST_CANSWER, INVALID_TID);
				sp->s_vc_state = STP_VCS_CONNECTED;
				STSTAT(stps_connects);
				stvc_isconnected(sp);
#undef RS1_CHECK
#ifdef  RS1_CHECK
				printf("Asking for RS1 after CANSWER, "
					"for no reason\n");
				st_ask_for_slots(sp);
				ASSERT_ALWAYS(0 == WAIT_FOR_SLOTS(sp));
				printf("Done, with slots after CANSWER\n");
#endif  /* RS1_CHECK */
				break;
			}

		case ST_CANSWER:
		case ST_RDISCONNECT:
		case ST_DANSWER:
		case ST_DCOMPLETE:
		case ST_VOP_UUNLISTEN:
			dprintf(20, ("Closing VC (CANS), sp 0x%x\n", sp)); 
			sp->s_vc_state = STP_VCS_CLOSED;
			break;
		case ST_VOP_UCONNECT:
		case ST_VOP_UDISCONNECT:
		default:
			cmn_err(CE_WARN, "Invalid transition in stc_vc_fsm VCS_LISTEN: 0x%x\n",
			       opcode);
		}
		break;
		
	case STP_VCS_RCSENT:
		switch(opcode) {
		case ST_CANSWER:
			{
				sthdr_ca_t *ca;
				sp->s_vc_state = STP_VCS_CONNECTED;
				STSTAT(stps_connects);
				
				ca = &(sth->sth_ca);
				
				sp->s_vcd.vc_max_rslots =
				sp->s_vcd.vc_rslots  =  ca->R_Slots;
				sp->s_vcd.vc_true_max_rslots = ca->R_Slots;
				ASSERT_ALWAYS(sp->s_vcd.vc_lport == ca->I_Port);
				sp->s_vcd.vc_rport        =  ca->R_Port;
				sp->s_vcd.vc_rbufsize     =  ca->R_Bufsize;
				sp->s_vcd.vc_rkey         =  ca->R_Key;
				sp->s_vcd.vc_rmaxstu      =  ca->R_Max_Stu;
				stvc_isconnected(sp);
				break;
			}
		case ST_RCONNECT:
		case ST_RDISCONNECT:
		case ST_DANSWER:
		case ST_DCOMPLETE:
		case ST_VOP_ULISTEN:
		case ST_VOP_UCONNECT:
		case ST_VOP_UDISCONNECT:
		default:
			cmn_err(CE_WARN, "Invalid transition in stc_vc_fsm VCS_RCSENT: 0x%x\n",
			       opcode);
		}
		break;
	case STP_VCS_CONNECTED:
		switch(opcode) {
		case ST_RDISCONNECT:
			stvc_output(sp, ST_DANSWER, INVALID_TID);
			sp->s_vc_state = STP_VCS_DASENT;
			stvc_isdisconnecting(sp);
			break;
			
		case ST_RS: {
			sthdr_rs1_t	*rs1 = &(sth->sth_rs1);

			ASSERT_ALWAYS(rs1->minus_one == -1);
#			ifdef	ST_HDR_DUMP
			dprintf(30, ("RS1 in stc_vc_fsm\n"));
#			endif	/* ST_HDR_DUMP */
			ST_DUMP_HDR(sth);
			/* remote side wants to sync up */
			error = st_sync_up(sp, rs1);
			break;
			}

		case ST_RSR: {
			dprintf(10, ("RSR in stc_vc_fsm\n"));
			error = st_process_RSR(sp, sth);
			break;
			}

		case ST_RCONNECT:
		case ST_CANSWER:
		case ST_DANSWER:
		case ST_DCOMPLETE:
		case ST_VOP_ULISTEN:
		case ST_VOP_UCONNECT:
		case ST_VOP_UDISCONNECT:
		default:
			cmn_err(CE_WARN, "Invalid transition in stc_vc_fsm VCS_CONNECTED: 0x%x\n",
			       opcode);
		}
		break;
	case STP_VCS_RDSENT:
		switch(opcode) {
		case ST_DANSWER:
			{
				stvc_output(sp, ST_DCOMPLETE, INVALID_TID);
				dprintf(20, ("Disconnecting VC (DANS), sp 0x%x\n", 
					sp)); 
				sp->s_vc_state = STP_VCS_DISCONNECTED;
				stvc_isdisconnected(sp);
				break;
			}
		case ST_RCONNECT:
		case ST_CANSWER:
		case ST_RDISCONNECT:
			{
				stvc_output(sp, ST_DANSWER, INVALID_TID);
				sp->s_vc_state = STP_VCS_DASENT;
				stvc_isdisconnecting(sp);
				break;
			}
		case ST_DCOMPLETE:
		case ST_VOP_ULISTEN:
		case ST_VOP_UCONNECT:
		case ST_VOP_UDISCONNECT:
		default:
			cmn_err(CE_WARN, "Invalid transition in stc_vc_fsm VCS_RDSENT: 0x%x\n",
			       opcode);
		}
		break;
	case STP_VCS_DASENT:

		switch(opcode) {
		case ST_DCOMPLETE:
			{
				dprintf(20, ("Disconnecting VC (DCOMP), sp 0x%x\n",
					sp));
				sp->s_vc_state = STP_VCS_DISCONNECTED;
				stvc_isdisconnected(sp);
				break;
			}
		case ST_RCONNECT:
		case ST_CANSWER:
		case ST_RDISCONNECT:
			{
				stvc_output(sp, ST_DANSWER, INVALID_TID);
				sp->s_vc_state = STP_VCS_DASENT;
				stvc_isdisconnecting(sp);
				break;
			}
		case ST_DANSWER:
			{
				stvc_output(sp, ST_DCOMPLETE, INVALID_TID);
				dprintf(20, ("Disconnecting VC (DANS), sp 0x%x\n",
					sp));
				sp->s_vc_state = STP_VCS_DISCONNECTED;
				stvc_isdisconnected(sp);
				break;
			}
		case ST_VOP_ULISTEN:
		case ST_VOP_UCONNECT:
		case ST_VOP_UDISCONNECT:
		default:
			cmn_err(CE_WARN, "Invalid transition in stc_vc_fsm VCS_DASENT: 0x%x\n",
			       opcode);
		}
		break;

	default:
		cmn_err(CE_WARN, "Invalid state! %s in ST-VC-setup\n", 
			st_decode_state(sp->s_vc_state));
		break;
	}

	
	if(sp)  {
		dprintf(30, ("st_vc_fsm exited: state is (%s)\n", 
		st_decode_state(sp->s_vc_state)));
	}
	else {
		dprintf(30, ("st_vc_fsm: vc torn down at exit\n")); 
	}


	return(error);
}




static int
st_data_tx_fsm(struct stpcb **spp, ushort OpFlags, sthdr_t *sth, 
							short tid)
{
	int 		error = 0;
	struct stpcb *sp = *spp;
	st_tx_t		*tx = NULL;
	st_state_t	state;
	ushort	opcode = OpFlags & ST_OPCODE_MASK;

	ASSERT_ALWAYS(tid == 0);
	tx = &(sp->tx[tid]);
	state = tx->tx_state;

	switch(state) {
		default: {
			dprintf(0, ("Invalid state %s st_data_tx_fsm\n",
		       		st_decode_state(state)));
			error = 1;
			break;
		}

		case STP_RTS_PINNED: {
		    switch(opcode) {
			default:
				{
				dprintf(0, ("Invalid opcode %s in STP_RTS_PINNED\n",
		       			st_decode_opcode(opcode)));
				error = 1;
				break;
				}

			case ST_CTS:
				{
				sthdr_cts_t	*cts = &(sth->sth_cts);

				dprintf(30, ("Got CTS 0x%x; parms: DP %d, SP %d, BlkSz %d, BNum %d; tlen %d\n",
					cts, cts->I_Port, cts->R_Port,
					cts->Blocksize, cts->B_num,
					tx->tx_tlen));
				tx->tx_rid = cts->R_id;
				ASSERT_ALWAYS(! IS_SET_TX_CTS(
					tx, cts->B_num));
				error = st_send_data(sp, sth, tid);
				SET_TX_CTS_TAB(tx, cts->B_num);
				ASSERT_ALWAYS(IS_SET_TX_CTS(
					tx, cts->B_num));
#undef	RS3_CHECK
#ifdef	RS3_CHECK
				printf("Asking for RS3 at CTS, "
					"for no reason\n");
				st_ask_for_block_info(sp, 0, tid);
				printf("Done, ask for RS3 at CTS\n");
#endif  /* RS3_CHECK */

				break;
				}

			case ST_RANSWER:
				{
				tx->tx_state = STP_RA_RECEIVED;
				dprintf(10, ("Got back an RA; opflags 0x%x\n",
					opcode));
				error = st_retire_write(sp, sth, tid);
				break;
				}

			case ST_RS:
				{
				sthdr_rs1_t	*rs1 = &(sth->sth_rs1);

				ASSERT_ALWAYS(rs1->minus_one == -1);
				/* remote side wants to sync up */
				error = st_sync_up(sp, rs1);
				break;
				}

			case ST_RSR:
				{
				/* remote side sent reply to local RS */
				error = st_process_RSR(sp, sth);
				break;
				}
			}
			break;
		}


		case STP_DATA_SEND: {
			switch(opcode) {
			default:
				{
				dprintf(0, ("Bad opcode %s in state %s "
					"of st_data_tx_fsm\n",
			       		st_decode_opcode(opcode),
					st_decode_state(state)));
				error = 1;
				break;
				}

			case ST_CTS:
				{
				sthdr_cts_t	*cts = &(sth->sth_cts);

				dprintf(30, ("Got CTS 0x%x; parms: DP %d, SP %d, BlkSz %d, BNum %d; tlen %d\n",
					cts, cts->I_Port, cts->R_Port,
					cts->Blocksize, cts->B_num,
					tx->tx_tlen));
				ASSERT_ALWAYS(! IS_SET_TX_CTS(
					tx, cts->B_num));
				if(cts->B_num <= tx->tx_max_Bnum_sent)  {
					dprintf(5, ("CTS %d outa order "
						"max %d, diff %d, "
						"max_OUTSTAND %d\n",
					cts->B_num, tx->tx_max_Bnum_sent,
					max(cts->B_num, tx->tx_max_Bnum_sent)
					- 
					min(cts->B_num, tx->tx_max_Bnum_sent),
					ST_NUM_CTS_OUTSTANDING));
				}
				error = st_send_data(sp, sth, tid);
				SET_TX_CTS_TAB(tx, cts->B_num);
				ASSERT_ALWAYS(IS_SET_TX_CTS(
					tx, cts->B_num));
#undef	RS3_CHECK
#ifdef	RS3_CHECK
				printf("Asking for RS3 at CTS, "
					"for no reason\n");
				st_ask_for_block_info(sp, 0, tid);
				printf("Done, ask for RS3 at CTS\n");
#endif  /* RS3_CHECK */
				break;
				}

			case ST_RS:
				{
				sthdr_rs1_t	*rs1 = &(sth->sth_rs1);

				ASSERT_ALWAYS(rs1->minus_one == -1);
#				ifdef	ST_HDR_DUMP
				dprintf(0, ("In st_data_tx_fsm\n"));
#				endif	/* ST_HDR_DUMP */
				ST_DUMP_HDR(sth);
				/* remote side wants to sync up */
				error = st_sync_up(sp, rs1);
				break;
				}

			case ST_RSR:
				{
				/* remote side sent reply to local RS */
				error = st_process_RSR(sp, sth);
				break;
				}
			}
			break;
		}

		case STP_READY_FOR_RTS: {
			switch(opcode) {
			default: 
				{
				dprintf(0, ("Bad opcode %s in state %s "
					"of st_data_tx_fsm\n",
			       		st_decode_opcode(opcode),
					st_decode_state(state)));
				error = 1;
				break;
				}

			case ST_RS: {
				sthdr_rs1_t	*rs1 = &(sth->sth_rs1);
	
				ASSERT_ALWAYS(rs1->minus_one == -1);
#				ifdef	ST_HDR_DUMP
				dprintf(0, ("RS1 in stc_vc_fsm\n"));
#				endif	/* ST_HDR_DUMP */
				ST_DUMP_HDR(sth);
				/* remote side wants to sync up */
				error = st_sync_up(sp, rs1);
				break;
				}

			case ST_RSR: {
				dprintf(10, ("RSR; st_data_tx_fsm\n"));
				error = st_process_RSR(sp, sth);
				break;
				}
			}
			break;
		}

	}   /* end of switch(state) */

	return error;
}

static int
st_data_rx_fsm(struct stpcb **spp, ushort OpFlags, sthdr_t *sth, 
							int tid)
{
	int 		error = 0;
	struct stpcb *sp = *spp;
	st_rx_t	 	*rx = NULL;
	st_state_t	state;
	ushort	opcode = OpFlags & ST_OPCODE_MASK;

	ASSERT_ALWAYS(tid == 0);
	rx = &(sp->rx[tid]);
	state = rx->rx_state;

	switch(state) {
		default: {
			dprintf(0, ("Bad state %s (0x%x) in st_data_rx_fsm\n",
		       		st_decode_state(state), state));
			st_dump_pcb(sp);
			st_dump_rx(rx);
			printf("\nHDR dump:\n");
			st_dump_hdr(sth);
			error = 1;
			break;
		}

		case STP_READY_FOR_RTS: {
		switch(opcode) {
			default: {
				dprintf(0, ("Invalid opcode in STP_READY_FOR_RTS 0x%x (%s)\n",
			       		opcode, st_decode_opcode(opcode)));
				error = 1;
				break;
			}

			case ST_RTS: {
				error = st_respond_to_RTS(sp, sth, tid);
				if(rx->rx_state != STP_DATA_RECV) {
					cmn_err(CE_WARN, 
					"State after RTS is %s\n",
					st_decode_state(rx->rx_state));
				}

				dprintf(10, ("DATA_fsm: new st: %s, "
					"opc %s \n",
					st_decode_state(rx->rx_state),
					st_decode_opcode(opcode)));

				break;
				}

			case ST_RS:
				{
				sthdr_rs1_t	*rs1 = &(sth->sth_rs1);
				sthdr_rs2_t	*rs2 = &(sth->sth_rs2);
				sthdr_rs3_t	*rs3 = &(sth->sth_rs3);

#				ifdef	ST_HDR_DUMP
				dprintf(0, ("In st_data_rx_fsm; RDY4RTS\n"));
#				endif	/* ST_HDR_DUMP */
				ST_DUMP_HDR(sth);
				if(rs1->minus_one == -1)  {
					/* remote side wants to sync up */
					error = st_sync_up(sp, rs1);
				}
				else if(rs2->minus_one == -1)  {
					error = st_ack_last_recv(
						sp, tid, rs2->Sync);
				}
				else {
					error = st_ack_a_block(
							 sp, tid, rs3);
				}
				break;
				}

			case ST_RSR:
				{
				/* remote side sent reply to local RS */
				error = st_process_RSR(sp, sth);
				break;
				}
			}
			break;
		}


		case STP_DATA_RECV: {
		switch(opcode) {
			default:
				{
				dprintf(0, ("Bad opcode %s in st_data_rx_fsm\n",
					st_decode_opcode(opcode)));
				dprintf(0, ("RX len left is %d\n",
					rx->rx_data_len));
				st_dump_pcb(sp);
				st_dump_rx(rx);
				printf("\nHDR dump:\n");
				st_dump_hdr(sth);
				error = 1;
				break;
				}

			case ST_DATA:
				{
				struct	mbuf	*payload;
				sthdr_data_t	*data = 
					(sthdr_data_t *) sth;
#undef RS1_CHECK
#ifdef  RS1_CHECK
				printf("Asking for RS1 at DATA, "
					"for no reason\n");
				st_ask_for_slots(sp);
				ASSERT_ALWAYS(0 == WAIT_FOR_SLOTS(sp));
				printf("Done, with slots at DATA\n");
#endif  /* RS1_CHECK */
				payload = rx->rx_buf.temp_mbuf;
				ASSERT_ALWAYS(payload != NULL);
				error = st_process_DATA(sp, sth, 
							tid, payload);
				rx->last_B_num = data->B_num;

				dprintf(30, ("ST_DATA: last_bnum set to %d\n",
					rx->last_B_num));

				dprintf(30, ("ST_DATA: new state is %s\n",
					st_decode_state(rx->rx_state)));

				break;
				}

			case ST_RS:
				{
				sthdr_rs1_t	*rs1 = &(sth->sth_rs1);
				sthdr_rs2_t	*rs2 = &(sth->sth_rs2);
				sthdr_rs3_t	*rs3 = &(sth->sth_rs3);

#				ifdef	ST_HDR_DUMP
				dprintf(0, ("In st_data_rx_fsm, DATRCV\n"));
#				endif	/* ST_HDR_DUMP */
				ST_DUMP_HDR(sth);
				if(rs1->minus_one == -1)  {
					/* remote side wants to sync up */
					error = st_sync_up(sp, rs1);
				}
				else if(rs2->minus_one == -1)  {
					error = st_ack_last_recv(
						sp, tid, rs2->Sync);
				}
				else {
					error = st_ack_a_block(
							sp, tid, rs3);
				}
				break;
				}

			case ST_RSR:
				{
				/* remote side sent reply to local RS */
				error = st_process_RSR(sp, sth);
				break;
				}
			}
			break;
		}

	}

	return error;
}

int
st_data_fsm(struct stpcb **spp, ushort OpFlags, sthdr_t *sth, int tid)
{
	struct stpcb *sp = *spp;
	ushort	opcode = OpFlags & ST_OPCODE_MASK;
	st_tx_t		*tx = &(sp->tx[tid]);
	st_rx_t		*rx = &(sp->rx[tid]);

	/* ASSERT_ALWAYS((STP_VCS_CONNECTED == sp->s_vc_state)
		|| (STP_VCS_RDSENT == sp->s_vc_state)
		|| (STP_VCS_DASENT == sp->s_vc_state)); */
	if(sp->s_vc_state != STP_VCS_CONNECTED
		&& sp->s_vc_state != STP_VCS_RDSENT
		&& sp->s_vc_state !=  STP_VCS_DASENT)  {
		cmn_err(CE_WARN, "State %s (opcode %s) in st_data_fsm\n",
			st_decode_state(sp->s_vc_state), 
			st_decode_opcode(opcode));
	}

	ASSERT_ALWAYS(tid == 0);

	/* 
	 *  Might have to choose the FSM based on opcode AND
	 *  current state at some point...
	*/
	if(IS_TX_FSM_OP(opcode))  {
		return st_data_tx_fsm(spp, OpFlags, sth, tid);
	} 
	else if(IS_RX_FSM_OP(opcode))  {
		return st_data_rx_fsm(spp, OpFlags, sth, tid);
	}
	else if(IS_SYNC_OP(opcode)) {
		if(opcode == ST_RSR)  {
			sthdr_rsr2_t	*rsr2 = (sthdr_rsr2_t *) sth;

			if(IS_TX_STATE(tx->tx_state)) {
				return st_data_tx_fsm(
					spp, OpFlags, sth, tid);
			}
			else if(IS_RX_STATE(rx->rx_state)) {
				return st_data_rx_fsm(
					spp, OpFlags, sth, tid);
			}
			else if(STP_VCS_CONNECTED == sp->s_vc_state)  {
				return stc_vc_fsm(&sp, OpFlags, sth, NULL);
			}
			else {
				cmn_err(CE_PANIC, "Unknown state "
					"[T: %s, R: %s] "
					"on i/p opcode %s\n",
				st_decode_state(tx->tx_state),
				st_decode_state(rx->rx_state),
				st_decode_opcode(opcode));
			}
		}
		else { /* opcode == RS guaranteed */
			if(IS_RX_STATE(rx->rx_state)) {
				dprintf(10, ("RX: RS in State %s\n",
					st_decode_state(
						rx->rx_state)));
				return st_data_rx_fsm(
					spp, OpFlags, sth, tid);
			}
			else if(IS_TX_STATE(tx->tx_state)) {
				dprintf(10, ("TX: RS in State %s\n",
					st_decode_state(
						tx->tx_state)));
				return st_data_tx_fsm(
					spp, OpFlags, sth, tid);
			}
			else if(STP_VCS_CONNECTED == sp->s_vc_state)  {
				return stc_vc_fsm(&sp, OpFlags, sth, NULL);
			}
			else {
				cmn_err(CE_PANIC, "Unknown state "
					"[T: %s, R: %s, VC: %s] "
					"on i/p opcode %s\n",
				st_decode_state(tx->tx_state),
				st_decode_state(rx->rx_state),
				st_decode_state(sp->s_vc_state),
				st_decode_opcode(opcode));
			}
		}
	}
	else {
		dprintf(0, ("Irrelevnt opcode %s in st_data_fsm\n",
			st_decode_opcode(opcode)));
		return	-1;
	}
	/* NOTREACHED */
}


int
st_tx_timer_expiry(struct stpcb *sp, uint tid)
{
	struct socket 	*so;
	st_tx_t		*tx = &(sp->tx[tid]);

	ASSERT(NULL != sp);
	so = stpcbtoso(sp);
	cmn_err(CE_WARN, 
		"Transmit timer went off; tearing down socket 0x%x\n",
			so);
	st_dump_pcb(sp);
	st_dump_tx(tx);
	
	return st_sodisconnect(so, 0);
}


int
st_rx_timer_expiry(struct stpcb *sp, uint rid)
{
	struct socket *so;
	st_rx_t		*rx = &(sp->rx[rid]);

	ASSERT(NULL != sp);
	so = stpcbtoso(sp);
	cmn_err(CE_WARN, 
		"Receive timer went off; tearing down socket 0x%x\n",
			so);
	st_dump_pcb(sp);
	st_dump_rx(rx);
			
	return st_sodisconnect(so, 0);
}

int
st_slots_timer_expiry(struct stpcb *sp)
{
	struct socket 	*so;
	st_rx_t		*rx = &(sp->rx[0]);
	st_kx_t		*kx = &(sp->kx[0]);
	st_tx_t		*tx = &(sp->tx[0]);

	ASSERT(NULL != sp);
	so = stpcbtoso(sp);
	cmn_err(CE_WARN, 
		"Slot timer went off; tearing down socket 0x%x\n", so);
	st_dump_pcb(sp);
	st_dump_tx(tx);
	st_dump_rx(rx);
	if(KX_OUTSTANDING(sp))  {
		st_dump_kx(kx);
	}
	
	return st_sodisconnect(so, 0);
}
