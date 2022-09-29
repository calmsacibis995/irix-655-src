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
 *  Filename: st_debug.c
 *  Description: debugging routines for the ST protocol stack.
 *
 *  $Author: jgregor $
 *  $Date: 1999/04/30 21:36:25 $
 *  $Revision: 1.4 $
 *  $Source: /proj/irix6.5f/isms/irix/kern/bsd/netinet/RCS/st_debug.c,v $
 *
 */


#include 	"sys/param.h"
#include 	"sys/types.h"
#include	"sys/systm.h"
#include	"sys/uio.h"
#include	"sys/debug.h"
#include 	"sys/kmem.h"
#include 	"sys/cmn_err.h"
#include 	"sys/buf.h"
#include 	"sys/alenlist.h"


#include	"st.h"
#include	"st_var.h"
#include	"st_bufx.h"
#include	"st_debug.h"
#include	"st_macros.h"

/* int iSTDebugFlags = 0xFFFF; */
int iSTDebugFlags = 0x0;
int STDebugLevel = 0;

/* let the races begin! why do we care? */
char    st_debug_payload[1024];
char	st_flag_decoded[ST_TOT_FLAGS];	

char *
st_decode_opcode(unsigned short OpFlags)
{
	ushort		opcode;

	opcode = OpFlags & ST_OPCODE_MASK;
	switch(opcode) {
		default:
			return "UNKNOWN_OPCODE";
		case ST_RCONNECT:
			return "ST_RCONNECT";
		case ST_CANSWER:
			return "ST_CANSWER";
		case ST_RDISCONNECT:
			return "ST_RDISCONNECT";
		case ST_DANSWER:
			return "ST_DANSWER";
		case ST_DCOMPLETE:
			return "ST_DCOMPLETE";


		case ST_RMB:
			return "ST_RMB";
		case ST_MBA:
			return "ST_MBA";
		case ST_GET:
			return "ST_GET";
		case ST_RTS:
			return "ST_RTS";
		case ST_RANSWER:
			return "ST_RANSWER";
		case ST_RTR:
			return "ST_RTR";

		case ST_CTS:
			return "ST_CTS";
		case ST_DATA:
			return "ST_DATA";
		case ST_RS:
			return "ST_RS";
		case ST_RSR:
			return "ST_RSR";
		case ST_END:
			return "ST_END";
		case ST_END_ACK:
			return "ST_END_ACK";

		case ST_VOP_ULISTEN:
			return "ST_VOP_ULISTEN";
		case ST_VOP_UUNLISTEN:
			return "ST_VOP_UUNLISTEN";
		case ST_VOP_UCONNECT:
			return "ST_VOP_UCONNECT";
		case ST_VOP_UDISCONNECT:
			return "ST_VOP_UDISCONNECT";
	}
}

char *
st_decode_state(unsigned short state)
{
	switch(state) {
		default:
			return "UNKNOWN STATE";
		case STP_VCS_CLOSED:
			return "STP_VCS_[CLOSED/DISCONNECTED]";
		case STP_VCS_LISTEN:
			return "STP_VCS_LISTEN";
		case STP_VCS_RCSENT:
			return "STP_VCS_RCSENT";
		case STP_VCS_CONNECTED:
			return "STP_VCS_[CONNECTED/RDY_FOR_RTS]";
		case STP_VCS_RDSENT:
			return "STP_VCS_RDSENT";
		case STP_VCS_DASENT:
			return "STP_VCS_DASENT";


		case STP_SEND_RTS_PINNING:
			return "STP_SEND_RTS_PINNING";
		case STP_RTS_PINNED:
			return "STP_RTS_PINNED";
		case STP_CTS_PINNING:
			return "STP_CTS_PINNING";
		case STP_DATA_SEND:
			return "STP_DATA_SEND";
		case STP_INIT_END_TRANSFER:
			return "STP_INIT_END_TRANSFER";
		case STP_WAIT_ACK:
			return "STP_WAIT_ACK";

		case STP_RTS_RECEIVED:
			return "STP_RTS_RECEIVED";
		case STP_BUFF_PINNING:
			return "STP_BUFF_PINNING";
		case STP_BUFF_RECEIVED:
			return "STP_BUFF_RECEIVED";
		case STP_RECV_RTS_PINNING:
			return "STP_RECV_RTS_PINNING";
		case STP_DATA_RECV:
			return "STP_DATA_RECV";
		case STP_RESP_END_TRANSFER:
			return "STP_RESP_END_TRANSFER";
	}
}

char *
st_decode_flags(unsigned short flags)
{
	char	*fptr = st_flag_decoded;

	bzero(st_flag_decoded, ST_TOT_FLAGS);

	if(flags & ST_REJECT) {
		sprintf(fptr, "%c", 'R');
		fptr++;
	}
	if(flags & ST_LAST) {
		sprintf(fptr, "%c", 'L');
		fptr++;
	}
	if(flags & ST_ORDER) {
		sprintf(fptr, "%c", 'O');
		fptr++;
	}
	if(flags & ST_SENDSTATE) {
		sprintf(fptr, "%c", 'S');
		fptr++;
	}
	if(flags & ST_INTERRUPT) {
		sprintf(fptr, "%c", 'I');
		fptr++;
	}
	if(flags & ST_SILENT) {
		sprintf(fptr, "%c", 'T');
		fptr++;
	}
	if(flags & ST_NOP) {
		sprintf(fptr, "%s", "NOP");
		fptr += 3;
	}
	if(flags & ST_FetchInc) {
		sprintf(fptr, "%s", "FIC");
		fptr += 3;
	}
	if(flags & ST_FetchDec) {
		sprintf(fptr, "%s", "FDC");
		fptr += 3;
	}
	if(flags & ST_FetchClear) {
		sprintf(fptr, "%s", "FCL");
		fptr += 3;
	}
	if(flags & ST_FetchCmplete) {
		sprintf(fptr, "%s", "FCM");
		fptr += 3;
	}
	
	return	st_flag_decoded;
}


/* caller should hold socket lock */
void
st_dump_hdr(sthdr_t *sth)
{
	sthdr_rc_t	*rc = (sthdr_rc_t *) sth;
	ushort		opcode, flags;

	opcode = rc->OpFlags & ST_OPCODE_MASK;
	flags = rc->OpFlags & ST_FLAG_MASK;
	dprintf(30, ("ST Hdr dump from 0x%x (op 0x%x, flags 0x%x, OpFlags 0x%x):\n",
		sth, opcode, flags, rc->OpFlags));
	switch(opcode)  {
		default : {
			dprintf(0, ("\t Unknown format %s (0x%x)\n",
				st_decode_opcode(opcode), opcode));
			break;
		}
		case ST_RCONNECT: {
			sthdr_rc_t	*rc = (sthdr_rc_t *) sth;

			dprintf(0, ("\t Opcode: RCONNECT\n"));
			dprintf(0, ("\t Flags: %s\n", 
				st_decode_flags(flags)));
			dprintf(0, ("\t I_Slots: %u\n", rc->I_Slots));
			dprintf(0, ("\t R_Port: %u\n", rc->R_Port));
			dprintf(0, ("\t I_Port: %u\n", rc->I_Port));
			dprintf(0, ("\t Checksum: %u\n", rc->Checksum));
			dprintf(0, ("\t EtherType: 0x%x\n", rc->EtherType));
			dprintf(0, ("\t I_Bufsize: %u\n", rc->I_Bufsize));
			dprintf(0, ("\t I_Key: 0x%x\n", rc->I_Key));
			dprintf(0, ("\t I_Max_Stu: 0%u\n", rc->I_Max_Stu));
			break;
		}
		case ST_CANSWER: {
			sthdr_ca_t	*ca = (sthdr_ca_t *) sth;

			dprintf(0, ("\t Opcode: CANSWER\n"));
			dprintf(0, ("\t Flags: %s\n", 
				st_decode_flags(flags)));
			dprintf(0, ("\t R_Slots: %u\n", ca->R_Slots));
			dprintf(0, ("\t I_Port: %u\n", ca->I_Port));
			dprintf(0, ("\t R_Port: %u\n", ca->R_Port));
			dprintf(0, ("\t I_Key: 0x%x\n", ca->I_Key));
			dprintf(0, ("\t Checksum: %u\n", ca->Checksum));
			dprintf(0, ("\t R_Bufsize: %d\n", ca->R_Bufsize));
			dprintf(0, ("\t R_Key: 0x%x\n", ca->R_Key));
			dprintf(0, ("\t R_Max_Stu: 0%u\n", ca->R_Max_Stu));
			break;
		}
		case ST_RDISCONNECT: {
			sthdr_rd_t	*rd = (sthdr_rd_t *) sth;

			dprintf(0, ("\t Opcode: RDISCONNECT\n"));
			dprintf(0, ("\t Flags: %s\n", 
				st_decode_flags(flags)));
			dprintf(0, ("\t R_Port: %u\n", rd->R_Port));
			dprintf(0, ("\t I_Port: %u\n", rd->I_Port));
			dprintf(0, ("\t R_Key: 0x%x\n", rd->R_Key));
			dprintf(0, ("\t Checksum: %u\n", rd->Checksum));
			dprintf(0, ("\t I_Key: 0x%x\n", rd->I_Key));
			break;
		}
		case ST_DANSWER: {
			sthdr_da_t	*da = (sthdr_da_t *) sth;

			dprintf(0, ("\t Opcode: DANSWER\n"));
			dprintf(0, ("\t Flags: %s\n", 
				st_decode_flags(flags)));
			dprintf(0, ("\t I_Port: %u\n", da->I_Port));
			dprintf(0, ("\t R_Port: %u\n", da->R_Port));
			dprintf(0, ("\t I_Key: 0x%x\n", da->I_Key));
			dprintf(0, ("\t Checksum: %u\n", da->Checksum));
			dprintf(0, ("\t R_Key: 0x%x\n", da->R_Key));
			break;
		}
		case ST_DCOMPLETE: {
			sthdr_dc_t	*dc = (sthdr_dc_t *) sth;

			dprintf(0, ("\t Opcode: DCOMPLETE\n"));
			dprintf(0, ("\t Flags: %s\n", 
				st_decode_flags(flags)));
			dprintf(0, ("\t R_Port: %u\n", dc->R_Port));
			dprintf(0, ("\t I_Port: %u\n", dc->I_Port));
			dprintf(0, ("\t R_Key: 0x%x\n", dc->R_Key));
			dprintf(0, ("\t Checksum: %u\n", dc->Checksum));
			dprintf(0, ("\t I_Key: 0x%x\n", dc->I_Key));
			break;
		}
		case ST_RTR: {
			sthdr_rtr_t	*rtr = (sthdr_rtr_t *) sth;

			dprintf(0, ("\t Opcode: RTR (0x%x)\n",
				opcode));
			dprintf(0, ("\t Flags: %s\n", 
				st_decode_flags(flags)));
			dprintf(0, ("\t R_Port: %u\n", rtr->R_Port));
			dprintf(0, ("\t I_Port: %u\n", rtr->I_Port));
			dprintf(0, ("\t R_Key: 0x%x\n", rtr->R_Key));
			dprintf(0, ("\t Checksum: %u\n", rtr->Checksum));
			dprintf(0, ("\t T_len: %u\n", rtr->T_len));
			dprintf(0, ("\t I_id: 0x%x\n", rtr->I_id));
			break;
		}
		case ST_RANSWER: {
			sthdr_cts_t	*cts = (sthdr_cts_t *) sth;

			dprintf(0, ("\t Opcode: RANSWER\n"));
			dprintf(0, ("\t Flags: %s\n", 
				st_decode_flags(flags)));
			dprintf(0, ("\t Blocksize: %u\n", cts->Blocksize));
			dprintf(0, ("\t I_Port: %u\n", cts->I_Port));
			dprintf(0, ("\t R_Port: %u\n", cts->R_Port));
			dprintf(0, ("\t I_Key: 0x%x\n", cts->I_Key));
			dprintf(0, ("\t Checksum: %u\n", cts->Checksum));
			dprintf(0, ("\t I_id: %u\n", cts->I_id));
			break;
		}
		case ST_CTS: {
			sthdr_cts_t	*cts = (sthdr_cts_t *) sth;

			dprintf(0, ("\t Opcode: CTS\n"));
			dprintf(0, ("\t Flags: %s\n", 
				st_decode_flags(flags)));
			dprintf(0, ("\t Blocksize: %u\n", cts->Blocksize));
			dprintf(0, ("\t I_Port: %u\n", cts->I_Port));
			dprintf(0, ("\t R_Port: %u\n", cts->R_Port));
			dprintf(0, ("\t I_Key: 0x%x\n", cts->I_Key));
			dprintf(0, ("\t Checksum: %u\n", cts->Checksum));
			dprintf(0, ("\t R_Mx: %u\n", cts->R_Mx));
			dprintf(0, ("\t R_Bufx: 0x%x\n", cts->R_Bufx));
			dprintf(0, ("\t R_Offset: %u\n", cts->R_Offset));
			dprintf(0, ("\t F_Offset: %u\n", cts->F_Offset));
			dprintf(0, ("\t B_num: %u\n", cts->B_num));
			dprintf(0, ("\t I_id: %u\n", cts->I_id));
			dprintf(0, ("\t R_id: %u\n", cts->R_id));
			break;
		}
		case ST_RTS: {
			sthdr_rts_t	*rts = (sthdr_rts_t *) sth;

			dprintf(0, ("\t Opcode: RTS\n"));
			dprintf(0, ("\t Flags: %s\n", 
				st_decode_flags(flags)));
			dprintf(0, ("\t CTS_req: %u\n", rts->CTS_req));
			dprintf(0, ("\t R_Port: %u\n", rts->R_Port));
			dprintf(0, ("\t I_Port: %u\n", rts->I_Port));
			dprintf(0, ("\t R_Key: 0x%x\n", rts->R_Key));
			dprintf(0, ("\t Checksum: %u\n", rts->Checksum));
			dprintf(0, ("\t Max_Block: %u\n", rts->Max_Block));
			dprintf(0, ("\t tlen: %u\n", rts->tlen));
			dprintf(0, ("\t I_id: %u\n", rts->I_id));
			break;
		}
		case ST_DATA: {
			sthdr_data_t	*data = (sthdr_data_t *) sth;

			dprintf(0, ("\t Opcode: DATA\n"));
			dprintf(0, ("\t Flags: %s\n", 
				st_decode_flags(flags)));
			dprintf(0, ("\t STU_num: %u\n", data->STU_num));
			dprintf(0, ("\t R_Port: %u\n", data->R_Port));
			dprintf(0, ("\t I_Port: %u\n", data->I_Port));
			dprintf(0, ("\t R_Key: 0x%x\n", data->R_Key));
			dprintf(0, ("\t Checksum: %u\n", data->Checksum));
			dprintf(0, ("\t R_Mx: %u\n", data->R_Mx));
			dprintf(0, ("\t R_Bufx: 0x%x\n", data->R_Bufx));
			dprintf(0, ("\t R_Offset: %u\n", data->R_Offset));
			dprintf(0, ("\t Sync: %u\n", data->Sync));
			dprintf(0, ("\t B_num: %u\n", data->B_num));
			dprintf(0, ("\t R_id: %u\n", data->R_id));
			dprintf(0, ("\t Opaque: 0x%x\n", data->Opaque));
			break;
		}
		case ST_RSR: {
			sthdr_rsr1_t	*rsr1 = (sthdr_rsr1_t *) sth;
			sthdr_rsr2_t	*rsr2 = (sthdr_rsr2_t *) sth;
			sthdr_rsr3_t	*rsr3 = (sthdr_rsr3_t *) sth;
			
			dprintf(0, ("\t Opcode: RSR\n"));
			dprintf(0, ("\t Flags: %s\n", 
				st_decode_flags(flags)));
			dprintf(0, ("\t R_Slots: %d\n", rsr1->R_Slots));
			dprintf(0, ("\t I_Port: %d\n", rsr1->I_Port));
			dprintf(0, ("\t R_Port: %d\n", rsr1->R_Port));
			dprintf(0, ("\t I_Key: 0x%x\n", rsr1->I_Key));
			dprintf(0, ("\t Sync: %d\n", rsr1->Sync));
			
			if(rsr1->minus_one != (uint32_t) -1)  {
				dprintf(0, ("\t B_Seq: %d\n", 
						rsr2->B_seq));
				dprintf(0, ("\t I_id: %d\n", 
						rsr2->I_id));
				dprintf(0, ("\t R_id: %d\n", 
						rsr2->R_id));
				if(rsr2->minus_one == (uint32_t) -1) {
					dprintf(0, ("\t type: "
						"ST_STATE_XFER\n"));
				}
				else {
					dprintf(0, ("\t type: "
						"ST_STATE_BLOCK; "
						"B_num %d\n",
						rsr3->B_num));
				}
			}
			else  {
				dprintf(0, ("\t type: ST_STATE_SLOTS\n"));
			}

			break;
		}
		case ST_RS: {
			sthdr_rs1_t	*rs1 = (sthdr_rs1_t *) sth;
			sthdr_rs2_t	*rs2 = (sthdr_rs2_t *) sth;
			sthdr_rs3_t	*rs3 = (sthdr_rs3_t *) sth;
			
			dprintf(0, ("\t Opcode: RS\n"));
			dprintf(0, ("\t Flags: %s\n", 
				st_decode_flags(flags)));
			dprintf(0, ("\t R_Port: %d\n", rs1->R_Port));
			dprintf(0, ("\t I_Port: %d\n", rs1->I_Port));
			dprintf(0, ("\t R_Key: 0x%x\n", rs1->R_Key));
			dprintf(0, ("\t Sync: %d\n", rs1->Sync));
			
			if(rs1->minus_one != -1)  {
				dprintf(0, ("\t R_id: %d\n", 
						rs2->R_id));
				dprintf(0, ("\t I_id: %d\n", 
						rs2->I_id));
				if(rs2->minus_one == -1)  {
					dprintf(0, ("\t type: "
						"ST_STATE_XFER\n"));
				}
				else {
					dprintf(0, ("\t B_num: %d\n",
						rs3->B_num));
					dprintf(0, ("\t type: "
						"ST_STATE_BLOCK\n"));
				}
			}
			else {
				dprintf(0, ("\t type: "
					"ST_STATE_SLOTS\n"));
			}

			break;
		}
	}
}


/* caller should hold socket lock */
void
st_dump_pcb(struct stpcb *sp)
{
	printf("Dumping stpcb at 0x%x\n", sp);
	printf("state: %s\n", st_decode_state(sp->s_vc_state));
	printf("vcd:\n");
	printf("\t flags: %u\n", sp->s_vcd.vc_flags);
	printf("\t lport: %u\n",sp->s_vcd.vc_lport);
	printf("\t lkey: %u\n",sp->s_vcd.vc_lkey);
	printf("\t lmax_slots: %u\n",sp->s_vcd.vc_max_lslots);
	printf("\t lbufsz: %u\n",sp->s_vcd.vc_lbufsize);
	printf("\t lmaxstu: %u\n",sp->s_vcd.vc_lmaxstu);
	printf("\t lmaxblk: %u\n",sp->s_vcd.vc_lmaxblock);
	printf("\n");
	printf("\t rport: %u\n",sp->s_vcd.vc_rport);
	printf("\t rkey: %u\n",sp->s_vcd.vc_rkey);
	printf("\t rmax_slots: %u\n",sp->s_vcd.vc_max_rslots);
	printf("\t rbufsz: %u\n",sp->s_vcd.vc_rbufsize);
	printf("\t rmaxstu: %u\n",sp->s_vcd.vc_rmaxstu);
	printf("\t rmaxblk: %u\n",sp->s_vcd.vc_rmaxblock);
	printf("\n");
	printf("\t vc_ethertype: 0x%x\n",sp->s_vcd.vc_ethertype);
	printf("\n");
	printf("\t rslots: %u\n",sp->s_vcd.vc_rslots);
	printf("\t lslots: %u\n",sp->s_vcd.vc_lslots);
	printf("\t vslots: %u\n",sp->s_vcd.vc_vslots);
	printf("\t true_max_lslots: %u\n",sp->s_vcd.vc_true_max_lslots);
	printf("\t true_max_rslots: %u\n",sp->s_vcd.vc_true_max_rslots);
	printf("\t lsync: %u\n",sp->s_vcd.vc_lsync);
	printf("\n");
	printf("flags: 0x%x\n", sp->s_flags);
	printf("optimeout: %u\n", sp->s_optimeout);
	printf("numretries: %u\n", sp->s_numretries);
	printf("socket: 0x%x\n", sp->s_so);
	printf("inpcb: 0x%x\n", sp->s_inp);
	printf("ifnet: 0x%x\n", sp->s_ifp);
	printf("last_rx_mx_setup: %u\n", sp->last_rx_mx_setup);
	printf("last_rx_mx_torndown: %u\n", sp->last_rx_mx_torndown);
	printf("last_RSR_R_id_sent: %u\n", sp->last_RSR_R_id_sent);
	printf("last_RSR_I_id_sent: %u\n", sp->last_RSR_I_id_sent);
	printf("last_RSR_R_id_recvd: %u\n", sp->last_RSR_R_id_recvd);
	printf("last_RSR_I_id_recvd: %u\n", sp->last_RSR_I_id_recvd);
}


void
st_dump_tx(st_tx_t *tx)
{
	printf("Dumping tx at 0x%x\n", tx);
	printf("\t tx-state: %s\n", st_decode_state(tx->tx_state));
	printf("\t tx_tlen: %u\n", tx->tx_tlen);
	printf("\t tx_uio: 0x%x\n", tx->tx_uio);
	printf("\t tx_local_bufsize: %u\n", tx->tx_local_bufsize);
	printf("\t tx_remote_bufsize: %u\n", tx->tx_remote_bufsize);
	printf("\t tx_blocksize: %u\n", tx->tx_blocksize);
	printf("\t tx_ctsreq: %u\n", tx->tx_ctsreq);
	printf("\t tx_foffset: %u\n", tx->tx_foffset);
	printf("\t tx_spray_width: %u\n", tx->tx_spray_width);
	printf("\t tx_iid: %u\n", tx->tx_iid);
	printf("\t tx_rid: %u\n", tx->tx_rid);
}


void
st_dump_rx(st_rx_t *rx)
{
	int i;
	printf("Dumping rx at 0x%x\n", rx);
	printf("\t rx-state: %s\n", st_decode_state(rx->rx_state));
	printf("\t rx_tlen: %u\n", rx->rx_tlen);
	printf("\t rx_cts_len: %u\n", rx->rx_cts_len);
	printf("\t rx_data_len: %u\n", rx->rx_data_len);
	printf("\t rx_local_bufsize: %u\n", rx->rx_local_bufsize);
	printf("\t rx_remote_bufsize: %u\n", rx->rx_remote_bufsize);
	printf("\t rx_blocksize: %u\n", rx->rx_blocksize);
	printf("\t rx_spray_width: %u\n", rx->rx_spray_width);
	printf("\t rx_iid: %u\n", rx->rx_iid);
	printf("\t rx_rid: %u\n", rx->rx_rid);
	printf("\t rx_num_mx: %u\n", rx->rx_num_mx);
	printf("\t rx_mx_tmp = 0x%x\n", rx->rx_mx_tmp);
	printf("\t &rx_mx[0] = 0x%x\n", &(rx->rx_mx[0]));
	for (i = 0; i < rx->rx_num_mx; i++)
		printf("\t\trm_mx[%d] = %u\n", i, rx->rx_mx[i]);
	printf("\t rx_first_offset: %u\n", rx->rx_first_offset);
	printf("\t cur_bnum: %u\n", rx->cur_bnum);
	printf("\t last_B_num: %u\n", rx->last_B_num);
	printf("\t hdr_for_RSR: 0x%x\n", rx->hdr_for_RSR);
}



void
st_dump_kx(st_kx_t *kx)
{
	printf("KX state: %s\n", st_decode_state(kx->kx_state));
	st_dump_hdr((sthdr_t *) &(kx->saved_rts));
}



void
print_uio(struct uio *uio)
{
	int	i;

	ASSERT_ALWAYS(uio != NULL);
	dprintf(0, ("Uio 0x%x: iovcnt is %d, resid is %d\n", 
		uio, uio->uio_iovcnt, uio->uio_resid));
	for(i = 0; i < uio->uio_iovcnt; i++)  {
		dprintf(0, ("\t iov %d, base 0x%x, char-count %d\n", 
			i, uio->uio_iov[i].iov_base,
			uio->uio_iov[i].iov_len));
	}
}


/* caller should hold socket lock */
/* this is meant for in-kernel payload -- dumping a payload mbuf,
*  when debugging mbuf (Ethernet/loopback) based ST
*/
void
st_dump_payload(char *payload, uint char_count)
{
	dprintf(0, ("st_dump_payload: len %u (hdrsz %d)\n", 
		char_count, sizeof(struct st_io_s)));
	for(; char_count; char_count--)   {
		/* slow as shit; but who cares */
		dprintf(0, ("%c", *payload++));
	}
	dprintf(0, ("\n"));
}


/* caller should hold socket lock */
/* dump the first few bytes of a user's payload; 
*  payload should be in userland (i.e., DMA payload, as in HIPPI) */
void
st_dump_payload_prefix(uio_t *uio)
{
#	define		DUMP_SIZE	32	
	int		i, j, bytes_moved;
	char		payload[DUMP_SIZE];
	iovec_t		*iov;		

	for(i = 0; i < uio->uio_iovcnt; i++)  {
		iov = &(uio->uio_iov[i]);
		dprintf(0, ("iov %d, base 0x%x, size %u\n",
			i, iov->iov_base, iov->iov_len));
		bytes_moved = min(DUMP_SIZE, iov->iov_len);
		if(copyin(iov->iov_base, payload, bytes_moved) < 0) {
			cmn_err(CE_PANIC, 
				"Fault in st_dump_payload_prefix\n");
		}
		dprintf(0, ("payload-prefix (%u bytes): ", 
			bytes_moved));
		for(j = 0; j < bytes_moved; j++)  {
			dprintf(0, ("%c", payload[j]));
		}
		dprintf(0, ("\n"));
	}
}


void
st_dump(void (*printf)(char *, ...))
{
	printf("Dumping interfaces:\n");
	idbg_dump_interfaces(printf);

	printf("Dumping bufxes:\n");
	idbg_dump_bufxes(printf);
}


void
st_dump_bnum_tab(st_rx_t *rx)
{
	int	i;

	for(i = 0; i < 8 * BNUM_TAB_SIZE; i++)  {
		if(!( i % 8))  {
			printf("%d: ", i);
		}
		printf(" %d,", IS_SET_BNUM(rx, i));
		if((i % 8) == 7)  {
			printf("\n");
		}
	}
}

void
st_dump_CTS_tab(st_rx_t *rx)
{
	int	i;

	for(i = 0; i < 8 * BNUM_TAB_SIZE; i++)  {
		if(!( i % 8))  {
			printf("%d: ", i);
		}
		printf(" %d,", IS_SET_RX_CTS(rx, i));
		if((i % 8) == 7)  {
			printf("\n");
		}
	}
}

void
st_dump_data_CTS_tab(st_tx_t *tx)
{
	int	i;

	for(i = 0; i < 8 * BNUM_TAB_SIZE; i++)  {
		if(!( i % 8))  {
			printf("%d: ", i);
		}
		printf(" %d,", IS_SET_TX_CTS(tx, i));
		if((i % 8) == 7)  {
			printf("\n");
		}
	}
}
