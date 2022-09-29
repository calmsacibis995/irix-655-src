/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1997, Silicon Graphics, Inc                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/
/* destination bypass firmware */
/* dbypass.c 
 *
 *$Revision: 1.23 $
 */


#include <sys/types.h>
#include <sys/errno.h>

#include "r4650.h"
#include "sys/PCI/linc.h"
#include "hippi_sw.h"
#include "rdbg.h"
#include "lincutil.h"

extern state_t   *State;



int
sched_bp_wr(dst_host_t *hostp, ack_blk_t *ablk) {
  /* returns one of no errors, zero if packet was discarded */

  bp_pkt_hdr_t  *bp_hdrp;
  bp_pkt_hdr_t  bp_hdr;
  bp_port_state_t *port;
  bp_job_state_t  *job;
  bp_seq_num_t *seq_st;

  int bufx;
  int host_offset;
  u_int host_length;
  volatile u_int *payloadp;
  dma_cmd_t dcmd;

  int i;
  u_int *ip;

  /* remove FP header */
  /* bring it in using cache */

  ip = (u_int*)K1_TO_K0(hostp->dp_put+2);  
  if (ip >= (u_int*)K1_TO_K0(hostp->endp))
      ip -= hostp->data_M_len;

  bp_hdrp = (bp_pkt_hdr_t *)ip;

  if( (u_int)(bp_hdrp + 1) > (u_int)K1_TO_K0(hostp->endp)) {
      /* must copy header because it wraps */
      for(i = 0; i < sizeof(bp_pkt_hdr_t)/4; i++) {
	  bp_hdr.i[i] = *ip++;
	  if (ip == (u_int*)K1_TO_K0(hostp->endp))
	      ip = (u_int*)K1_TO_K0(hostp->basep); 
      }
      bp_hdrp = &bp_hdr;
  }

  if(bp_hdrp->d.vers != BP_PKT_VERS) /* bad header version */
    goto drop_bp_bad_vers;

  port = hostp->port+bp_hdrp->d.d_port; /* careful - could be out of bounds */
  if(bp_hdrp->d.d_port >= BP_MAX_PORTS /* port out of range or not enabled */
     || !(port->st & BP_PORT_VAL))
    goto drop_bp_bad_port;

  job = hostp->job + port->job;
  if ( !(hostp->job_vector>>(31-port->job) & 1)) /* job not enabled */
    goto drop_bp_bad_job;
  
  if (job->auth[0] != bp_hdrp->d.auth[0] || /* bad authentication */
      job->auth[1] != bp_hdrp->d.auth[1] ||
      job->auth[2] != bp_hdrp->d.auth[2]) 
    goto drop_bp_bad_auth;
  
  if (bp_hdrp->d.X || bp_hdrp->d.Z || bp_hdrp->d.G) /* unsupported opcodes */
    goto drop_bp_bad_opcode;

  seq_st = hostp->bpseqnum + (port->job*BP_MAX_SLOTS) + bp_hdrp->d.slot;

  if (ablk->d2_size != 0) {    /* have something to dma */
    host_offset = bp_hdrp->d.d_off << 2; /* xlate words to bytes */
    if ( !bp_hdrp->d.S) {	/* multi-packet */
      if(bp_hdrp->d.slot >= BP_MAX_SLOTS) 
	goto drop_bp_bad_seqnum;
      seq_st = hostp->bpseqnum + (port->job*BP_MAX_SLOTS) + bp_hdrp->d.slot;
      if (bp_hdrp->d.F) {
	if (! (seq_st->expseqnum == 0) || seq_st->expseqnum == -1) {
	  /* last block packet lost a microblock */
	  hostp->bpstats->hst_d_bp_seq_err++;
	}
	
	seq_st->maxseqnum = bp_hdrp->d.seqnum;
	seq_st->expseqnum = bp_hdrp->d.seqnum;
      }
      if(seq_st->expseqnum != bp_hdrp->d.seqnum)
	goto drop_bp_bad_seqnum;
      
      seq_st->expseqnum--;
    } /* finished multi-pkt processing */
    
    
    bufx = bp_hdrp->d.d_bufx;
    if(bufx >= BP_DFM_ENTRIES) /* bufx not in bounds */
      goto drop_bp_bad_bufx;

    /* both offset and length cacheline aligned */
    if( (host_offset & (NBPCL-1) )
	|| (ablk->d2_size & (NBPCL-1)) )
      goto drop_bp_bad_offset;

    /* offset is valid, check if length fits in bufx entry */
    if (host_offset + ablk->d2_size > job->fm_entry_size)
      goto drop_bp_bad_offset;

    /* store before xlate to avoid race conditions
     * must translate bufx to pfn everytime because driver
     * might have written dead page to freemap location
     */
    store_bp_dma_status(DMA_ENABLE_DST_DATA, 1, bufx, port->job);

    /* start DMA */
    /* get rid of FP hdr too */
    payloadp = hostp->dp_put + (((int)ablk->d1_size) >> 2) + 2;

    if (payloadp >= hostp->endp)
      payloadp -= hostp->data_M_len ;
 
    dcmd.host_addr_hi = (hostp->freemap + 
			 (BP_DFM_ENTRIES*port->job + bufx))->addr_hi;
    dcmd.host_addr_lo = (hostp->freemap + 
			 (BP_DFM_ENTRIES*port->job + bufx))->addr_lo;
    dcmd.host_addr_lo += host_offset;
    dcmd.brd_addr = (u_int)K1_TO_PHYS(payloadp);
    dcmd.len = ablk->d2_size;

    dcmd.flags = LINC_DTC_D64 | LINC_DTC_TO_PARENT;

    trace(TOP_DMA0, T_DMA_BP_DATA,
	  dcmd.brd_addr, dcmd.host_addr_lo, dcmd.flags | dcmd.len);

    dma_push_cmd0(&dcmd);	/* GO!*/
  }
	
  /* DMA has been launched (if needed) - now prep a descriptor */

  if (bp_hdrp->d.I && (bp_hdrp->d.S || 
		       (!bp_hdrp->d.S && seq_st->expseqnum == 0))) {
      port->int_cnt++;
      if ( !(port->st & BP_PORT_BLK_INT)) {
	  hostp->flags |= DF_BP_INTR_DESC;
	  hostp->cur_port = port;
	  hostp->dport = bp_hdrp->d.d_port;
      }
  }

  ablk->bp_job = port->job;

  if (bp_hdrp->d.D 
      && (bp_hdrp->d.S || (!bp_hdrp->d.S && seq_st->expseqnum == 0))) {
	    
      /* prepare a descriptor */
      /* copy header in reverse order for desc - do this to preserve
       * all possible header payload bits.
       */
		
      ablk->bpdesc.i[3] = bp_hdrp->i[0] & BP_PKT_OPCODE_MASK;

      ablk->bpdesc.d.d_bufx	= bp_hdrp->d.d_bufx;
      ablk->bpdesc.d.d_off	= bp_hdrp->d.d_off;

      ablk->bpdesc.d.s_bufx	= bp_hdrp->d.s_bufx;
      ablk->bpdesc.d.s_off	= bp_hdrp->d.s_off;

      ablk->bpdesc.d.get_job	= bp_hdrp->d.get_job;
      ablk->bpdesc.d.slot	= bp_hdrp->d.slot;
      ablk->bpdesc.d.hostx	= bp_hdrp->d.s_hostx;
      ablk->bpdesc.d.port	= bp_hdrp->d.s_port;

      ablk->bpdesc.d.len	= ablk->d2_size>>2; /* to words */

      if (ablk->bpdesc.d.len != 0) {
	  if( !ablk->bpdesc.d.S) {
	      ablk->bpdesc.d.len = seq_st->maxseqnum;
	      ablk->bpdesc.d.d_bufx = bp_hdrp->d.d_bufx - seq_st->maxseqnum +1;
	      ablk->bpdesc.d.s_bufx = bp_hdrp->d.s_bufx - seq_st->maxseqnum +1;
	  }
      }
      ablk->bp_desc_addr_lo = port->dq_base_lo + 
	port->dq_tail*sizeof(hippi_bp_desc);	/* no carry */
      ablk->bp_desc_addr_hi = port->dq_base_hi | PPCIHI_ATTR_BARRIER;
      
      port->dq_tail++;
      if (port->dq_tail >= port->dq_size)
	  port->dq_tail = 0;

      /* dma descriptor to host NOW */
      hostp->flags &= ~DF_BP_DESC;
      store_bp_dma_status(DMA_ENABLE_DST_DESC, 0, 0, 
			  ablk->bp_job);
      *State->bp_dst_desc = ablk->bpdesc;
      dcmd.host_addr_lo = ablk->bp_desc_addr_lo;
      dcmd.host_addr_hi = ablk->bp_desc_addr_hi | 
	                  PPCIHI_ATTR_BARRIER;
		    
      dcmd.brd_addr = (u_int)K1_TO_PHYS(State->bp_dst_desc);
      dcmd.len = sizeof(hippi_bp_desc);
      dcmd.flags = LINC_DTC_D64 | LINC_DTC_TO_PARENT;
      trace(TOP_DMA0, T_DMA_BP_DESC,
	    dcmd.brd_addr, dcmd.host_addr_lo, dcmd.flags | dcmd.len);

      dma_push_cmd0(&dcmd); /* GO! */
      hostp->bpstats->hst_d_bp_descs++;

  }


  /* remove from cache so next pkt not cached */
  if (bp_hdrp != &bp_hdr)
    inval_dcache(bp_hdrp, sizeof(bp_pkt_hdr_t)); 
  else {			/* wrap of header occured */
    inval_dcache((char*)(K1_TO_K0(hostp->dp_put)), sizeof(bp_pkt_hdr_t)); 
    inval_dcache((char*)(K1_TO_K0(hostp->basep)), sizeof(bp_pkt_hdr_t)); 
  }
  if (ablk->d2_size <= 256)	
    /*    trace(TOP_DMA0, T_DMA_BP_DATA, 0,0,0);*/
    while (!dma0_done()) 
      ;
  

  hostp->flags |= DF_PENDING;

/* bypass stuffs all of blk into dma queue, none left */
  hostp->cur_len = 0;		

  return(1);
  
  /* ERROR RECOVERY */  
  
	    drop_bp_bad_vers:
  hostp->bpstats->hst_d_bp_vers_err++;
  goto drop_dst_bp_pkt;

	    drop_bp_bad_port:
  hostp->bpstats->hst_d_bp_port_err++;;
  goto drop_dst_bp_pkt;

	    drop_bp_bad_job:
  hostp->bpstats->hst_d_bp_job_err++;
  goto drop_dst_bp_pkt;

	    drop_bp_bad_auth:
  hostp->bpstats->hst_d_bp_auth_err++;
  goto drop_dst_bp_pkt;

	    drop_bp_bad_opcode:
  hostp->bpstats->hst_d_bp_opcode_err++;
  goto drop_dst_bp_pkt;

	    drop_bp_bad_seqnum:
  hostp->bpstats->hst_d_bp_seq_err++;
  goto drop_dst_bp_pkt;

	    drop_bp_bad_bufx:
  hostp->bpstats->hst_d_bp_bufx_err++;
  goto drop_dst_bp_pkt;

	    drop_bp_bad_offset:
  hostp->bpstats->hst_d_bp_off_err++;
  goto drop_dst_bp_pkt;

	    drop_dst_bp_pkt:
  hostp->bpstats->hst_d_bp_packets++;
  
  /* reset dma status */
  store_bp_dma_status(DMA_OFF, 0, 0, 0);

  /* remove from cache so next pkt not cached */
  wbinval_dcache(bp_hdrp, sizeof(bp_pkt_hdr_t)); 

  return(0);

}
