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
/*
 * sbypass.c - source bypass.
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

extern state_t   	*State;


int sched_bp_rd(src_host_t *hostp, src_blk_t *blk) {
    /* return values:
     * 		0 = error occured
     *		non-zero = all okay
     */

    bp_job_state_t *jobp = &(hostp->job[hostp->cur_job]);
    int job = hostp->cur_job;
    
    hippi_bp_desc desc;
    int bufx_size = jobp->fm_entry_size;

    hippi_fp_t   *fp_hdr;
    bp_pkt_hdr_t *bp_hdr;
    u_int	*payloadp;

    int host_offset;
    int host_length;

    int dma2_length, dma1_length;
    dma_cmd_t dcmd;
    int i;

#ifdef NEVER
    /* this caused descriptor corruption */
    rgather((u_int*)&jobp->sdq_head->l[0], (u_int*)&desc.l[0]);
    rgather((u_int*)&jobp->sdq_head->l[1], (u_int*)&desc.l[1]);
#endif

    desc.i[0] = (u_int)jobp->sdq_head->i[0];
    desc.i[1] = (u_int)jobp->sdq_head->i[1];
    desc.i[2] = (u_int)jobp->sdq_head->i[2];
    desc.i[3] = (u_int)jobp->sdq_head->i[3];
    
    trace(TOP_BP_DESC,job, BP_SDQ_ENTRIES - (jobp->sdq_end - jobp->sdq_head),
	  desc.i[3],desc.i[1]);

    host_offset = desc.d.s_off<<2;	/* convert to bytes; */
    host_length = desc.d.len<<2; /* convert to bytes */

    
    trace(TOP_BP_DESC, 0, desc.d.S, host_length, State->nbpp);

    /* sanity check descriptor */
    if( desc.d.hostx >= BP_HOSTX_ENTRIES)
	goto drop_pkt_hostx;
    if(desc.d.X == 1 || desc.d.G == 1 || desc.d.Z != 0)
	goto drop_pkt_bad_opcode;
	
    /*multi-pkt can't have the special zero-length payload */
    if(desc.d.S == 0 && host_length == 0)
	goto drop_pkt_bad_len;

    /* process length*/
    if (desc.d.S == 0)  {	/* bulk move */
	host_length = bufx_size;
    }
    else if (host_length > State->nbpp)
	goto drop_pkt_bad_len;
    
    fp_hdr = (hippi_fp_t *)hostp->dp_put;
    bp_hdr = (bp_pkt_hdr_t *)( (caddr_t)fp_hdr + sizeof(hippi_fp_t));
    payloadp = (u_int *)( (caddr_t)bp_hdr + sizeof(bp_pkt_hdr_t)); 

    if (host_length != 0) {	/* something to DMA */

	/* We should never have an offset past the end of the buffer.
	* This isn't really the right error, but we don't have one for this. */
	if (host_offset > bufx_size)
	    goto drop_pkt_bad_len;
    
	if(desc.d.s_bufx >= BP_SFM_ENTRIES)
	    goto drop_pkt_bad_src_bufx;

	if ( (host_length << 29) >> 31)	/* must be long word aligned */
	    goto drop_pkt_bad_len;
    
	/*  can't move more than one bufx size of data */
	if(host_length > bufx_size) 
	    goto drop_pkt_bad_len;
    
	/* odd word offset fixup */
	if ( (host_offset<<29) >> 31) {	     
	    /* copy extra word at beginning and end */
	    dma1_length = host_length + 8; 
	    host_offset -= 4;	/*long word align */
	    payloadp--;		/* overwrite the padding word */
	}
	else 	/* even_word offset - nothing to fixup */
	    dma1_length = host_length;
    
	/* 2 bufx's or just 1? */
	dma2_length = host_offset + dma1_length - bufx_size;
	if (dma2_length <= 0) {	/* no second page to dma from */
	    dma2_length = 0;
	    store_bp_dma_status(DMA_ENABLE_SRC_DATA, 1, 
				desc.d.s_bufx, job);
	}

	else {			/* setup for 2nd page */
	    dma1_length -=  dma2_length;
	    if ((desc.d.s_bufx+1) > BP_SFM_ENTRIES)
		goto drop_pkt_bad_src_bufx;
	    store_bp_dma_status(DMA_ENABLE_SRC_DATA, 2, 
				desc.d.s_bufx, job);
	}

	dcmd.host_addr_hi = (hostp->freemap + 
		  (BP_SFM_ENTRIES*job + desc.d.s_bufx))->addr_hi;
	dcmd.host_addr_hi |= PPCIHI_ATTR_PREFETCH;

	dcmd.host_addr_lo = (hostp->freemap + 
		  (BP_SFM_ENTRIES*job + desc.d.s_bufx))->addr_lo;
	dcmd.host_addr_lo += host_offset;

	dcmd.brd_addr = (u_int)K1_TO_PHYS(payloadp);
	dcmd.flags = LINC_DTC_D64 | LINC_DTC_RD_CMD_READ;
	dcmd.len = dma1_length;

#ifdef DEBUG
	if(payloadp + (dma1_length>>2) >= hostp->endp)
	    ASSERT(0);
#endif

	trace(TOP_DMA0, T_DMA_BP_DATA, dcmd.brd_addr, dcmd.host_addr_lo, dcmd.flags | dcmd.len);
	
	dma_push_cmd0(&dcmd);	/* GO! */
#ifdef BRIDGE_B_WAR
	if ((dcmd.host_addr_hi & PPCIHI_ATTR_PREFETCH)
	    && (dcmd.host_addr_lo + (dcmd.len & 0x1ffff)) & (State->nbpp-1))
	    dma0_flush_prefetch();
#endif
    
	if (dma2_length > 0) {
	    dcmd.brd_addr += dcmd.len/4;
	    /* host offset always zero  */
	    dcmd.host_addr_hi = (hostp->freemap + 
		   (BP_SFM_ENTRIES*job + desc.d.s_bufx) + 1)->addr_hi;
	    dcmd.host_addr_hi |= PPCIHI_ATTR_PREFETCH;

	    dcmd.host_addr_lo = (hostp->freemap + 
		   (BP_SFM_ENTRIES*job + desc.d.s_bufx) + 1)->addr_lo;
	    dcmd.len = dma2_length;

	    trace(TOP_DMA0, T_DMA_BP_DATA, dcmd.brd_addr, dcmd.host_addr_lo, dcmd.flags | dcmd.len);

	    dma_push_cmd0(&dcmd); /* GO! */
#ifdef BRIDGE_B_WAR
	if ((dcmd.host_addr_hi & PPCIHI_ATTR_PREFETCH)
	    && (dcmd.host_addr_lo + (dcmd.len & 0x1ffff)) & (State->nbpp-1))
		dma0_flush_prefetch();
#endif
	}
    }

    /*  PACKET HEADER INITIALIZATION */

    /* setup FP hdr - set P=1, B=0, d2_offset = 0, d1 length */
    fp_hdr->hfp_ulp_id = hostp->bp_ulp;
    fp_hdr->hfp_flags = HFP_FLAGS_P;

    /* units in lw */
    fp_hdr->hfp_d1d2off = (sizeof(bp_pkt_hdr_t) >> 3) << HFP_D1SZ_SHFT; 
    fp_hdr->hfp_d2size = host_length;
  
    /* init Bypass header - opcode, vers, s_hostx, s_port */
    bp_hdr->i[0] = (desc.i[3] & BP_DESC_OPCODE_MASK)
	| (BP_VERSION << BP_PKT_VERSION_SHIFT)
	    | jobp->ack_hostport;
  
    /* init get_job, slot, d_hostx/port */
    bp_hdr->i[1] = desc.i[2];
  
    /* init pad, seqnum */
    bp_hdr->i[2] = desc.i[3];
  
    /* init s_bufx */
    bp_hdr->d.s_bufx = desc.d.s_bufx;
  
    /* init s_off */
    bp_hdr->d.s_off = desc.d.s_off;
  
    /* init d_bufx */
    bp_hdr->d.d_bufx = desc.d.d_bufx;

    /* init d_off */
    bp_hdr->d.d_off = desc.d.d_off;
  
    /* authentication words */
    bp_hdr->d.auth[0] = jobp->auth[0];
    bp_hdr->d.auth[1] = jobp->auth[1];
    bp_hdr->d.auth[2] = jobp->auth[2];
  
    /* checksum isn't used */
    bp_hdr->d.chksum = 0;
  
    /* scrub pad word */
#ifdef DEBUG
    bp_hdr->d.pad = 0;
#endif  

    /* Done with packet header creation.  */

    /* Clean up the descriptor */
    if(desc.d.S == 0 && desc.d.len != 1) { 
	/* multipkt, more micropkts to send */
	desc.d.len--;
	desc.d.F = 0;
	desc.d.s_bufx++;
	desc.d.d_bufx++;
	wgather((u_int*)&desc, (u_int*)jobp->sdq_head, sizeof(hippi_bp_desc));
    }
    else {
	hostp->bpstats->hst_s_bp_descs++;

	jobp->sdq_head->i[3] = -1;
	jobp->sdq_head++;
	if(jobp->sdq_head == jobp->sdq_end)
	    jobp->sdq_head -= BP_SDQ_ENTRIES;
    }
  
    hostp->msg_flags = SMF_PENDING;

    blk->flags = SBLK_SOC | SBLK_BP;
    blk->num_d2bs = 0;
    blk->dp = (u_int *)fp_hdr;	/* point at fp header */
    blk->fburst = 0;
    blk->ifield = hostp->hostx[job*BP_HOSTX_ENTRIES + desc.d.hostx]; 
    blk->len = host_length + sizeof(bp_pkt_hdr_t) + sizeof(hippi_fp_t); /*FP*/

    blk->tail_pad = 8;   /* pad 2 words to solve CPCI prefetch coherency */

    hostp->cksum_offs = -1;
#ifdef DEBUG
    hostp->chunks = hostp->chunks_left = 0;
#endif

    return( 1);
  
    /* ERROR RECOVERY */  
  
  drop_pkt_hostx:
    hostp->bpstats->hst_s_bp_desc_hostx_err++;
    goto drop_pkt;
  
  drop_pkt_bad_opcode:
    hostp->bpstats->hst_s_bp_desc_opcode_err++;
    goto drop_pkt;
  
  drop_pkt_bad_len:
    hostp->bpstats->hst_s_bp_desc_addr_err++;
    goto drop_pkt;
  
  drop_pkt_bad_src_bufx:
    hostp->bpstats->hst_s_bp_desc_bufx_err++;
    goto drop_pkt;
  
  drop_pkt:
    desc.i[3] = -1;
    hostp->bpstats->hst_s_bp_descs++;
    jobp->sdq_head->i[3] = -1;

    jobp->sdq_head++;
    if(jobp->sdq_head == jobp->sdq_end)
	jobp->sdq_head -= BP_SDQ_ENTRIES;

    /* reset dma status */
    store_bp_dma_status(DMA_OFF, 0, 0, 0);

    return(0);
  
}




