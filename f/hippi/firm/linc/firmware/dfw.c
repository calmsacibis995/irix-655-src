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
 * dfw.c - Destination firmware 
 *
 * $Revision: 1.62 $
 *
 */

#define HIPPI_DST_FW


#include <sys/types.h>

#include "r4650.h"
#include "sys/PCI/linc.h"
#include "hippi_sw.h"
#include "rdbg.h"
#include "lincutil.h"


extern state_t   	*State;

d2b_state_t 		*D2B;
b2h_state_t 		*B2H = 0;
wire_blk_t 		wblk;	/* wire block - block of data from wire_fsm */
ack_blk_t 		ablk;	/* acknowledge block - for feedback to wire 
				   fsm to free resource */
mbuf_state_t 		MBUF;
fpbuf_state_t		FPBUF;



/************************************************************
  fetch_fpbufs
************************************************************/

void
fetch_fpbufs(gen_host_t *hostp) {
    dma_cmd_t dcmd;
    stk_state_t *stackp = hostp->stackp;


    /* invalidate cached version of fpbufs */
    wbinval_dcache(FPBUF.basep, FPBUF_DMA_LEN);

    /* dma list of bufs locally */
    dcmd.flags = LINC_DTC_D64;
    dcmd.host_addr_hi = stackp->fpbuf_addr_hi;
    dcmd.host_addr_lo = stackp->fpbuf_addr_lo;
    dcmd.brd_addr = (u_int)K0_TO_PHYS(FPBUF.basep);

    dcmd.len = MIN(FPBUF_DMA_LEN, stackp->fpbuf_len);

    dprintf(5, ("fetching fpbufs\n"));
    
    trace(TOP_DMA1, T_DMA_GET_FPBUFS,
	  dcmd.brd_addr, dcmd.host_addr_lo, dcmd.flags | dcmd.len);

    dma_push_cmd1(&dcmd); /* GO! */

    FPBUF.num_valid = dcmd.len;
    FPBUF.get = FPBUF.basep;

    stackp->fpbuf_addr_lo += dcmd.len;
    
    while( !dma1_done())	/* wait for completion */
	;
			
}

/************************************************************
  fpbuf_fill_dma_pipe
************************************************************/

void
fpbuf_fill_dma_pipe(dst_host_t *hostp, ack_blk_t *ablk, int dmas_left) {
    dma_cmd_t dcmd;
    int i;
    int blen;

    dcmd.flags = LINC_DTC_D64 | LINC_DTC_TO_PARENT;

    /* stuff fpbufs. can't stuff dma queue to zero
     * because of 2nd dma on wrap
     */

    while ((hostp->cur_len > 0) && (dmas_left > 1) ) {
	if (FPBUF.num_valid == 0)
	    fetch_fpbufs(hostp);

	blen = MIN(FPBUF.get->c2b_param, hostp->cur_len);

	ASSERT(blen > 0);

	dcmd.host_addr_hi = (u_int)(FPBUF.get->c2b_addr>>32);
	dcmd.host_addr_lo = (u_int)FPBUF.get->c2b_addr;
	dcmd.brd_addr = (u_int)K1_TO_PHYS(hostp->dp_put);
	dcmd.len = blen;

	trace(TOP_DMA0, T_DMA_FP_D2,
	      dcmd.brd_addr, dcmd.host_addr_lo, dcmd.flags | dcmd.len);

	dprintf(1, ("hostp->cur_len = 0x%x, total_len = 0x%x\n",
		    hostp->cur_len, hostp->total_len));
	dma_push_cmd0(&dcmd);	/* GO! - could push 2 dma's*/

	hostp->dma_flags &= ~DDMA_START_PKT;
	dmas_left--;
	hostp->cur_len -= blen;

	dprintf(5, ("fpbuf_fill: cur_len = 0x%x, total_len = 0x%x\n", 
		    hostp->cur_len, hostp->total_len));

	hostp->dp_put += blen/4;	
	if (hostp->dp_put >= hostp->endp)
	    hostp->dp_put -= hostp->data_M_len;

	/* if it is not EOP and the last buffer did not get 
	 * fully used, then next time start where you left off.
	 */
	if ((blen != FPBUF.get->c2b_param)) {
	    FPBUF.get->c2b_addr += blen;
	    FPBUF.get->c2b_param -= blen;

	    dprintf(5, ("fill_dma_pipe: recycling FPBUF, get = 0x%x, num_val=0x%x\n",
			FPBUF.get, FPBUF.num_valid));
	}
	else {
	    FPBUF.get++;
	    FPBUF.num_valid -= sizeof(hip_c2b_t);
	    hostp->stackp->fpbuf_len -= sizeof(hip_c2b_t);
	    if (hostp->stackp->fpbuf_len == 0) /* out of bufs */
		break;

	}
    }

    /* if no more valid buffers and more at host, proactively fetch more */
    if ((FPBUF.num_valid == 0) && (hostp->stackp->fpbuf_len != 0))
	fetch_fpbufs(hostp);


}

/************************************************************
  copy_blk_wtoa
************************************************************/

void
copy_blk_wtoa(wire_blk_t *wblk, ack_blk_t *ablk, 
	      gen_host_t *hostp, u_int stack) {
    /* pick up info on packet/blockm, release DW machine
     * to parse next header, and stuff the dma engine quueue
     */
    ablk->flags = wblk->flags;
    ablk->avail = wblk->avail + hostp->rem_len;
    hostp->dp_put = ablk->dp = wblk->dp - hostp->rem_len/4;
    if (ablk->dp < hostp->basep) {
	ablk->dp += hostp->data_M_len;
	hostp->dp_put = ablk->dp;
    }

    dprintf(1, ("copy_blk: dp_put = 0x%x\n", hostp->dp_put));

    if (hostp->flags & DF_NEOP) { /* already copied most state */
	hostp->total_len += wblk->avail; /* don't count remainder here */
	ablk->d2_offset = 0;
	ablk->d1_size = 0;
    }
    else {
	ablk->ulp = wblk->ulp;
	ablk->d2_size = wblk->d2_size;
	ablk->d2_offset = wblk->d2_offset;
	ablk->d1_size = wblk->d1_size;
	ablk->pad = wblk->pad;
	ablk->bp_job = 0;

	/* sanity check - if EOP here don't let D2 length 
	 * be longer than what we've got */
	if (ablk->flags & DBLK_EOP) {
	  if (ablk->d2_size > (ablk->avail- ablk->d2_offset)) {
		ablk->d2_size = ablk->avail - ablk->d2_offset;
	  }
	}
	hostp->total_len = ablk->avail - ablk->d2_offset;

	if (stack < HIP_N_STACKS) {
	    hostp->stack = stack;
	    hostp->stackp = &hostp->fpstk[stack];
	}
	else
	    hostp->stack = HIP_N_STACKS;
    }

    if (ablk->avail & 0x4) {	/* odd word aligned - put off til next blk */
	ASSERT((ablk->flags & DBLK_EOP) == 0);
	dprintf(1, ("ROUNDING LENGTH DOWN\n"));
        hostp->rem_len = 4;
	ablk->avail -= 4;
    }
    else
	hostp->rem_len = 0;

    /* We've just copied the struct so free up the wire to bring in the next. */
    wblk->flags = DBLK_NONE;
}

/************************************************************
  sched_fphdr_wr
************************************************************/

void
sched_fphdr_wr(gen_host_t *hostp, 
	     ack_blk_t *ablk) {
	     
    dma_cmd_t dcmd;
    hip_b2h_t b2h;
    
    if (hostp->stack != HIP_STACK_RAW) {
	/* dma fp header to host */
	dcmd.flags = LINC_DTC_D64 | LINC_DTC_TO_PARENT;
	dcmd.host_addr_hi = hostp->stackp->hdr_addr_hi;
	dcmd.host_addr_lo = hostp->stackp->hdr_addr_lo;
	dcmd.brd_addr = (u_int)K1_TO_PHYS(hostp->dp_put);
	dcmd.len = ablk->d1_size + 8;

	trace(TOP_DMA0, T_DMA_FP_D1,
	      dcmd.brd_addr, dcmd.host_addr_lo, dcmd.flags | dcmd.len);

	hostp->stackp->flags &= ~FP_STK_HDR_VAL;
	dma_push_cmd0(&dcmd);	/* GO! */
    }

    /* dma HIP_B2H_IN */
    /* stuff b2h_s with length of init buffer that was dma'ed
     *  stuff b2h_l with length of D2 that's available 
     *      (doesn't have to be accurate)
     */

    b2h.b2h_op = HIP_B2H_IN | hostp->stack;

    if (hostp->stack == HIP_STACK_RAW) {
	b2h.b2h_l = 128*1024;
	b2h.b2hu.b2hu_s = 0;
    }
    else {
	b2h.b2h_l = ablk->d2_size;
	b2h.b2hu.b2hu_s = (ablk->d1_size + 8);
    }

    b2h_queue(&b2h);
    b2h_push(hostp);
    
    hostp->flags |= DF_HOST_KNOWS;

    hostp->cur_len = ablk->avail - ablk->d2_offset;
    hostp->dp_put += ablk->d2_offset>>2; 
    if (hostp->dp_put >= hostp->endp)
      hostp->dp_put -= hostp->data_M_len;
}


/************************************************************
  init_mbufs
************************************************************/

void
init_mbufs(void) {
    /* throw out m_bufs */
    MBUF.sm_mbuf_basep = State->sm_buf;
    MBUF.lg_mbuf_basep = State->lg_buf;

    MBUF.sm_put = MBUF.sm_mbuf_basep;
    MBUF.sm_get = MBUF.sm_mbuf_basep;
    MBUF.lg_get = MBUF.lg_mbuf_basep;
    MBUF.lg_put = MBUF.lg_mbuf_basep;

    /* throw out fp bufs */
    FPBUF.basep = State->fpbuf;
    FPBUF.num_valid = 0;
    FPBUF.get = FPBUF.basep;
    
}

/************************************************************
  enough_mbufs
************************************************************/

int
enough_mbufs(int num_lg, int num_sm) {
    int answer;

    answer = (num_lg <= (HIP_MAX_BIG + (MBUF.lg_put - MBUF.lg_get))
	      %HIP_MAX_BIG);
    answer &= (num_sm <= (HIP_MAX_SML + (MBUF.sm_put - MBUF.sm_get))
	       %HIP_MAX_SML);

    return(answer);
}

/************************************************************
  put_sm_mbuf
************************************************************/

void 
put_sm_mbuf(u_int addr_lo, u_int addr_hi, int len) {

    /* better not be full */
    ASSERT(HIP_MAX_SML > 
	   ((HIP_MAX_SML + MBUF.sm_put - MBUF.sm_get)%HIP_MAX_SML));

    MBUF.sm_put->addr_lo = addr_lo;
    MBUF.sm_put->addr_hi = addr_hi;
    MBUF.sm_put->len = len;
    if (++MBUF.sm_put >= (MBUF.sm_mbuf_basep+HIP_MAX_SML))
	MBUF.sm_put = MBUF.sm_mbuf_basep;
}


/************************************************************
  put_lg_mbuf
************************************************************/

void 
put_lg_mbuf(u_int addr_lo, u_int addr_hi, int len) {
    /* better not be full */
    ASSERT(HIP_MAX_BIG > 
	   (HIP_MAX_BIG + (MBUF.lg_put - MBUF.lg_get))%HIP_MAX_BIG);

    MBUF.lg_put->addr_lo = addr_lo;
    MBUF.lg_put->addr_hi = addr_hi;
    MBUF.lg_put->len = len;

    if (++MBUF.lg_put >= (MBUF.lg_mbuf_basep+HIP_MAX_BIG))
	MBUF.lg_put = MBUF.lg_mbuf_basep;
}

/************************************************************
  mbuf_fill_dma_pipe
************************************************************/

void
mbuf_fill_dma_pipe(dst_host_t *hostp, int dmas_left) {
    dma_cmd_t dcmd;
    int i;
    int blen;

    if (hostp->dma_flags & DDMA_START_PKT)
	dcmd.flags = LINC_DTC_D64 
	    | LINC_DTC_TO_PARENT;
    else
	dcmd.flags = LINC_DTC_D64 
	    | LINC_DTC_TO_PARENT
	    | LINC_DTC_CHAIN_CS;


    /* stuff small mbufs. can't stuff dma queue to zero
     * because of 2nd dma on wrap
     */
    while (hostp->cur_sm_buf_len > 0 && dmas_left > 1) { 

	blen = MIN(MBUF.sm_get->len, hostp->cur_sm_buf_len);

	dcmd.host_addr_hi = MBUF.sm_get->addr_hi;
	dcmd.host_addr_lo = MBUF.sm_get->addr_lo;
	dcmd.brd_addr = (u_int)K1_TO_PHYS(hostp->dp_put);

	if ((hostp->cur_sm_buf_len <= MBUF.sm_get->len) 
	    && (hostp->cur_lg_bufs == 0))
	    /* end of packet - save checksum */
	    dcmd.flags |= LINC_DTC_SAVE_CS;
	
	dcmd.len = blen;

	trace(TOP_DMA0, T_DMA_IP_SMBUF,
	      dcmd.brd_addr, dcmd.host_addr_lo, dcmd.flags | dcmd.len);


	dma_push_cmd0(&dcmd); /* GO! */
	hostp->dma_flags &= ~DDMA_START_PKT;
	dcmd.flags |= LINC_DTC_CHAIN_CS;

	/* decrement dma's, length, 
	 * inc data ptr, wrap dataptr,inc mbuf get ptr*/

	dmas_left--;
	hostp->cur_sm_buf_len -= blen;
	hostp->cur_len -= blen;
	hostp->dp_put += blen/4;	
	if (hostp->dp_put >= hostp->endp)
	    hostp->dp_put -= hostp->data_M_len;

	MBUF.sm_get++;
	if (MBUF.sm_get >= (MBUF.sm_mbuf_basep+HIP_MAX_SML))
	    MBUF.sm_get = MBUF.sm_mbuf_basep;

    }

    
    /*stuff large mbufs */
    while ((hostp->cur_lg_bufs > 0) && (dmas_left > 1)) {
	blen = MIN(MBUF.lg_get->len, hostp->cur_len);

	dcmd.host_addr_hi = MBUF.lg_get->addr_hi;
	dcmd.host_addr_lo = MBUF.lg_get->addr_lo;
	dcmd.brd_addr = (u_int)K1_TO_PHYS(hostp->dp_put);
	dcmd.len = blen;

	if (hostp->cur_lg_bufs == 1)
	    /* end of packet - save checksum */
	    dcmd.flags |= LINC_DTC_SAVE_CS;

	trace(TOP_DMA0, T_DMA_IP_LGBUFS,
	      dcmd.brd_addr, dcmd.host_addr_lo, dcmd.flags | dcmd.len);

	dma_push_cmd0(&dcmd);	/* GO! */
	hostp->dma_flags &= ~DDMA_START_PKT;
	dcmd.flags |= LINC_DTC_CHAIN_CS;

	dmas_left--;
	hostp->cur_lg_bufs--;
	hostp->cur_len -= blen;
	hostp->dp_put += blen/4;	
	if (hostp->dp_put >= hostp->endp)
	    hostp->dp_put -= hostp->data_M_len;

	MBUF.lg_get++;
	if (MBUF.lg_get >= (MBUF.lg_mbuf_basep+HIP_MAX_BIG))
	    MBUF.lg_get = MBUF.lg_mbuf_basep;

    }
}

/************************************************************
  sched_le_wr
************************************************************/

int
sched_le_wr(gen_host_t *hostp, ack_blk_t *ablk) {
    int num_sm_mbufs;
    
    /* le can not span multiple blocks
     * size how many small and large mbufs we need 
     * using tail fill 
     */

    /* avail might have extra pad because IP is byte aligned length */
    ablk->avail = ablk->d2_size+ablk->d2_offset;

    if (ablk->avail & 0x3) {	/* make word in length */
	ablk->avail = (ablk->avail & ~0x3) + 4;
    }

    hostp->cur_len = ablk->avail;

    if (hostp->cur_len <= 3*( MBUF.sm_get->len)){ 
	/* less than three sm_bufs, just use sm mbufs */
	hostp->cur_sm_buf_len = hostp->cur_len;
	hostp->cur_lg_bufs = 0;
    }

    else { /* use tail fill algorithm */

	hostp->cur_sm_buf_len = hostp->cur_len%MBUF.lg_get->len;
	hostp->cur_lg_bufs = hostp->cur_len/MBUF.lg_get->len;

	if (hostp->cur_sm_buf_len > MBUF.sm_get->len*3) { 
	    /* too many small mbufs - move it to large mbuf */
	    hostp->cur_sm_buf_len = 0;
	    if(hostp->cur_lg_bufs*MBUF.lg_get->len < hostp->cur_len)
		hostp->cur_lg_bufs++;
	}
	else if (hostp->cur_sm_buf_len < MIN_FP_LE_SNAP_IP_LEN) {
	    hostp->cur_sm_buf_len = MIN_FP_LE_SNAP_IP_LEN;
	    if ( (hostp->cur_len - hostp->cur_sm_buf_len)  
		 <= ((hostp->cur_lg_bufs - 1 )*MBUF.lg_get->len))
		/* pull out one large mbuf */
		hostp->cur_lg_bufs--;
	}
    }

    num_sm_mbufs = hostp->cur_sm_buf_len/MBUF.sm_get->len;
    if (num_sm_mbufs*MBUF.sm_get->len < hostp->cur_sm_buf_len)
	num_sm_mbufs++;

    ablk->le_sm_buf_len = hostp->cur_sm_buf_len;
    ablk->le_lg_bufs = hostp->cur_lg_bufs;

    dprintf(1, ("sched_le_wr: le_sm_buf_len = 0x%x, le_lg_bufs = 0x%x, len = 0x%x\n",
		ablk->le_sm_buf_len, ablk->le_lg_bufs, hostp->cur_len));
    

    /* if not enough mbufs, drop packet */
    if ( !enough_mbufs(hostp->cur_lg_bufs, num_sm_mbufs)) {
	/* not enough mbufs - drop packet */
	State->stats->df.hip_s.ledrop++;
	return(0);
    }

    hostp->dma_flags = DDMA_START_PKT | DDMA_CHAIN_CS;

    mbuf_fill_dma_pipe(hostp, MAX_DMAS_ENQUEUED);

    if (hostp->cur_len > 0)
	hostp->flags |= DF_STUFFING | DF_PENDING;
    else
	hostp->flags |= DF_PENDING;

    return(1);
}

/************************************************************
  process_d2bs
************************************************************/

void
process_d2bs(gen_host_t *hostp) {
    gen_d2b_t *d2bp;

    while (d2bp = d2b_get()) {
	int stack = d2bp->c2b_op & HIP_C2B_STMASK;

	switch (d2bp->c2b_op & HIP_C2B_OPMASK)
	    {
	      case HIP_C2B_SML:
		if (stack == HIP_STACK_LE) { /* mbuf for LE stack */
		    put_sm_mbuf((u_int)d2bp->c2b_addr, 
				(u_int)(d2bp->c2b_addr>>32),
				d2bp->c2b_param);

		}
		else {		/* header buf for FP stack */
		    dprintf(1, (">>>>>>>>got d2b: FPHDR_VAL, addr = 0x%x,%x, len=0x%x\n",
				(u_int)(d2bp->c2b_addr>>32), 
				(u_int)d2bp->c2b_addr,
				d2bp->c2b_param));
		    trace(TOP_D2B, T_D2B_FPHDR, 0xffffff & (u_int)d2bp, (u_int)d2bp->c2b_addr, 
			  *(u_int*)&d2bp->c2b_param);
		    hostp->fpstk[stack].flags |= FP_STK_HDR_VAL;
		    hostp->fpstk[stack].hdr_addr_hi = 
			                        (u_int)(d2bp->c2b_addr>>32);
		    hostp->fpstk[stack].hdr_addr_lo = (u_int)d2bp->c2b_addr;
		}
		break;
	      case HIP_C2B_BIG:
		put_lg_mbuf((u_int)d2bp->c2b_addr, 
			    (u_int)(d2bp->c2b_addr>>32),
				    d2bp->c2b_param);

		break;
	      case HIP_C2B_WRAP:
		/* no longer used, just ignore - length is in hcmd_init */
		break;

	      case HIP_C2B_READ:
		dprintf(1, ("got d2b: FP_STK_FPBUF_VAL, addr = 0x%x,%x, len = 0x%x\n",
			(u_int)(d2bp->c2b_addr>>32), 
			(u_int)d2bp->c2b_addr,
			d2bp->c2b_param));
		trace(TOP_D2B, T_D2B_FPBUF,  0xffffff & (u_int)d2bp, (u_int)d2bp->c2b_addr, 
			  *(u_int*)&d2bp->c2b_param);
		hostp->fpstk[stack].flags |= FP_STK_FPBUF_VAL;
		hostp->fpstk[stack].fpbuf_addr_hi = (u_int)(d2bp->c2b_addr>>32);
		hostp->fpstk[stack].fpbuf_addr_lo = (u_int)d2bp->c2b_addr;
		hostp->fpstk[stack].fpbuf_len = d2bp->c2b_param;

		break;

	      default:
		assert(0);
	    }
    }
}

/************************************************************
  dma_cleanup
************************************************************/

int 
dma_cleanup(dst_host_t *hostp, ack_blk_t *ablk) {
    /* cleans up after a dma transfer
     * clears DF_PENDING, which allows ablk struct to be used again
     */

    hostp->flags &= ~DF_PENDING;

    switch(hostp->st) 
	{
	  case DH_BP:

	    if ( (hostp->job_vector>>(31 - ablk->bp_job) & 1)) { 
		/* job still enabled? */
		if (hostp->flags & DF_BP_INTR_DESC) {
		    /* send a b2h telling driver to interrupt dest process */
		    /* must not send a b2h every time the intr bit is set or
		     * will overflow b2h queue.
		     */
		    hip_b2h_t b2h;
			
		    hostp->flags &= ~DF_BP_INTR_DESC;
		    b2h.b2h_op = HIP_B2H_BP_PORTINT;
		    b2h.b2hu.b2h_bp_portint.portid = hostp->dport;
		    b2h.b2h_l = hostp->cur_port->int_cnt;
		    b2h_queue(&b2h);
		    b2h_push(hostp);
		  
		    hostp->cur_port->st |= BP_PORT_BLK_INT;

		    while ( !dma0_done()) ;
		}

		trace(TOP_DMA0, T_DMA_BP_DESC,0,0,0);
	    }
	    else
		hostp->bpstats->hst_d_bp_job_err++;

	    store_bp_dma_status(DMA_OFF, 0, 0, 0);

	    hostp->st = DH_IDLE;

	    trace(TOP_HFSM_ST, hostp->st, (hostp->flags<<16) | hostp->dma_flags,
		  (hostp->rem_len<<24) | hostp->cur_len,  
		  (hostp->cur_sm_buf_len<<16) | hostp->cur_lg_bufs);
	    hostp->bpstats->hst_d_bp_packets++;
	    
	    /* just count user payload in stats */
	    dprintf(5, ("dma_cleanup: proc'ed 0x%x b\n", ablk->avail));
	    hostp->bpstats->hst_d_bp_byte_count += ablk->avail;
 	    hostp->dma_flags = 0;

	    break;
	  
	  case DH_LE:
	    {
		hip_b2h_t b2h;
		u_int checksum;
		int len;

		dprintf(1, ("getting checksum\n"));
		
		/* dma IN_DONE to host */
		checksum = LINC_READREG(LINC_DMA_RES_CHECKSUM_0);

		ASSERT((LINC_READREG(LINC_DMA_CONTROL_STATUS_0) 
			& LINC_DCSR_NUM_CHK_MASK) == 0);
		
		checksum += checksum>>16;
		checksum += checksum>>16;

		b2h.b2h_op = HIP_B2H_IN_DONE | hostp->stack;
		b2h.b2h_pages = ablk->le_lg_bufs;
		b2h.b2h_words = ablk->le_sm_buf_len>>2; /* to words */

		len = ablk->avail;

		/* xlate to words, starting at bit 16 */
		b2h.b2h_l = (len>>2)<<16; 

		b2h.b2h_l |= checksum & 0xffff;

		ASSERT( ((b2h.b2h_pages*16384)+b2h.b2h_words*4) >=  ((b2h.b2h_l>>16)*4));
		b2h_queue(&b2h);
		b2h_push(hostp);
		hostp->st = DH_IDLE;
		
		trace(TOP_HFSM_ST, hostp->st, (hostp->flags<<16) | hostp->dma_flags,
		      (hostp->rem_len<<24) | hostp->cur_len,  
		      (hostp->cur_sm_buf_len<<16) | hostp->cur_lg_bufs);
		dprintf(5, ("dma_cleanup: LE sent to host, %d bytes\n", len));
		hostp->dma_flags = 0;
		break;
	    }

	  case DH_FP:
	    {
		hip_b2h_t b2h;

		/* State table of what could be happening:
		 * EOP	len=0	bufs=0	action
		 *  F	F	F	can't occur - dma_cleanup won't be entered
		 *  T	F	F	can't occur
		 *
		 *  F	F	T	b2h | MORE
		 *  T	F	T	b2h | MORE
		 * 
		 *  F	T	F	end of blk, got bufs, wait for next blk
		 *  F	T	T	b2h | MORE
		 *  T	T	F	b2h
		 *  T	T	T	b2h
		 */
		dprintf(1, ("dma_cleanup: cur_len=0x%x, EOP=%d, fpbufs=0x%x\n",
		       hostp->cur_len, (ablk->flags & DBLK_EOP) ? 1:0,
		       hostp->stackp->fpbuf_len))

		if (  (ablk->flags & DBLK_EOP)
		    || (hostp->stackp->fpbuf_len == 0)) {
		    
		    /* send a b2h to host because:
		     *   - out of buffers to dma into
		     *   - end of packet and all dma's are done.
		     */
		    
		    hostp->stackp->flags &= ~FP_STK_FPBUF_VAL;/* flush rest of bufs */
		    FPBUF.num_valid = 0;
		    b2h.b2h_op = HIP_B2H_IN_DONE | hostp->stack;
		    b2h.b2h_l = hostp->total_len - hostp->cur_len - hostp->rem_len;

		    if ((hostp->cur_len != 0) || !(ablk->flags & DBLK_EOP)) {
			/* either ran out of buffers for this block
			 * or haven't seen EOP so complete with MORE 
			 */
			b2h.b2h_s = B2H_ISTAT_MORE;

			/* reset len to remainder */
			hostp->total_len = hostp->cur_len + hostp->rem_len; 
			hostp->st = DH_WAIT_FPBUF;
		    }
		    else {	/* EOP and done with this block */
			ASSERT(hostp->rem_len == 0);
			hostp->flags &= ~(DF_NEOP | DF_HOST_KNOWS);
			b2h.b2h_s = 0;
			hostp->st = DH_IDLE;
		    }
		    trace(TOP_HFSM_ST, hostp->st, (hostp->flags<<16) | hostp->dma_flags,
			  (hostp->rem_len<<24) | hostp->cur_len,  
			  (hostp->cur_sm_buf_len<<16) | hostp->cur_lg_bufs);
		    dprintf(1, ("dma_cleanup: pushing b2h, EOP = %d, MORE = %d, len = 0x%x\n",
				(ablk->flags & DBLK_EOP) ? 1 : 0,
				(b2h.b2h_s & B2H_ISTAT_MORE) ? 1 : 0,
				b2h.b2h_l));
		    b2h_queue(&b2h);
		    b2h_push(hostp);
		}
		else {
		    /* not-end-of-packet, end of this block, but still have fp bufs */

		    trace(TOP_HFSM_ST, hostp->st, (hostp->flags<<16) | hostp->dma_flags,
			  (hostp->rem_len<<24) | 
			  (hostp->total_len - hostp->cur_len - hostp->rem_len),
			  (hostp->cur_sm_buf_len<<16) | hostp->cur_lg_bufs);
		    dprintf(1, ("dma_cleanup: done w/ blk, total_len = 0x%x\n",
				hostp->cur_len, hostp->total_len));
		}
	    }
	    break;


	  case DH_WAIT_FPBUF:
	    /* occurs when a zero length D2 is received - just recycle the data buffer */
	    hostp->st = DH_IDLE;
	    hostp->flags &= ~DF_HOST_KNOWS;
	    break;

	  case DH_IDLE:
	    break;
	    
	  default:
	    assert(0);
	    
	}

    /* give credits back to roadrunner */
    if (hostp->cur_len == 0) {
        update_dl2rr_datap(ablk->dp, ablk->avail + ablk->pad);
	ablk->flags = DBLK_NONE;
    }
}


/************************************************************

  host_fsm
************************************************************/

void
host_fsm(dst_host_t *hostp, 
	 wire_blk_t *wblk, 
	 ack_blk_t *ablk) {
    int stack;


    /* sets DF_PENDING if a block is launched to host
     * sets DF_STUFFING if transfer couldn't be completely inserted
     * into the DMA fifo - i.e. there is more to put into the FIFO
     */

#ifdef DEBUG
    /* discard block if error and not in middle of packet */
    if (wblk->flags & DBLK_ERR_MASK)
	ASSERT(hostp->flags & DF_NEOP);
#endif

    switch(hostp->st) 
	{
	  case DH_IDLE:


	    /* Bypass Packet */
	    if(hostp->job_vector && (wblk->ulp == hostp->bp_ulp)) {	
		dprintf(1, ("host_fsm: Bypass pkt\n"));

		copy_blk_wtoa(wblk, ablk, hostp, HIP_N_STACKS);
		if (sched_bp_wr(hostp, ablk)) {
		    hostp->st = DH_BP;
		    trace(TOP_HFSM_ST, hostp->st, (hostp->flags<<16) | hostp->dma_flags,
			  (hostp->rem_len<<24) | hostp->cur_len,  
			  (hostp->cur_sm_buf_len<<16) | hostp->cur_lg_bufs);
		    return;
		}

		/* error occured */
		break;
	    }

	    else if ( (wblk->ulp == HIPPI_ULP_LE) 
		     && (State->flags & FLAG_ENB_LE)) { 
		/* LE packet (really IP) */

		dprintf(1, ("host_fsm: IP pkt\n"));
		copy_blk_wtoa(wblk, ablk, hostp, HIP_STACK_LE);
		if (sched_le_wr(hostp, ablk)) {
		    hostp->st = DH_LE;
		    trace(TOP_HFSM_ST, hostp->st, (hostp->flags<<16) | hostp->dma_flags,
			  (hostp->rem_len<<24) | hostp->cur_len,  
			  (hostp->cur_sm_buf_len<<16) | hostp->cur_lg_bufs);

		    return;
		}

		/* error occured */
		break;

	    }
	    else {
		/* might be an FP packet */

		stack = (State->flags & FLAG_HIPPI_PH) ? 
		    HIP_STACK_RAW :  hostp->ulptostk[wblk->ulp ];

		copy_blk_wtoa(wblk, ablk, hostp, stack);

		/* PH packet or FP on an enabled ulp */
		if ((hostp->stack == HIP_STACK_RAW)
		    || ( (hostp->stack != HIP_N_STACKS)
			&& (hostp->stackp->flags & FP_STK_ENABLED))) {

		    dprintf(1, ("host_fsm: got FP/PH pkt\n"));

		    if ( (hostp->stack != HIP_STACK_RAW)
 			&& !(hostp->stackp->flags & FP_STK_HDR_VAL)) {
			/* FP stack is enabled, but no header buf is
			 * available. should already be in queue, so
			 * synchronously dma it down.
			 */
			dprintf(1, ("host_fsm: to DH_WAIT_FPHDR\n"));
			hostp->st = DH_WAIT_FPHDR;
			trace(TOP_HFSM_ST, hostp->st, (hostp->flags<<16) | hostp->dma_flags,
			      (hostp->rem_len<<24) | hostp->cur_len,  
			      (hostp->cur_sm_buf_len<<16) | hostp->cur_lg_bufs);
			goto host_fsm_need_fphdr;
		    }
host_fsm_got_fp_hdr:

		    /* FP/PH stack is enabled and a header buffer is val */
		    sched_fphdr_wr(hostp, ablk);
		    
		    if (ablk->d2_size == 0 ||
			!(hostp->stackp->flags & FP_STK_FPBUF_VAL)) { 
			/* if d2_size == 0 then no more to dma - wait for completion.
			 *       (note that PH has d2_size = avail)
			 * or there is no buffer available to dma into
			 */
			hostp->st = DH_WAIT_FPBUF;
			if (ablk->d2_size == 0)
			    hostp->flags |= DF_PENDING;
			trace(TOP_HFSM_ST, hostp->st, (hostp->flags<<16) | hostp->dma_flags,
			      (hostp->rem_len<<24) | hostp->cur_len,  
			      (hostp->cur_sm_buf_len<<16) | hostp->cur_lg_bufs);
			return;
		    }

		    /* D2 size is non-zero and buffers are available */
		    hostp->st = DH_FP;

		    trace(TOP_HFSM_ST, hostp->st, (hostp->flags<<16) | hostp->dma_flags,
			  (hostp->rem_len<<24) | hostp->cur_len,  
			  (hostp->cur_sm_buf_len<<16) | hostp->cur_lg_bufs);

		    goto host_fsm_fp;
		}
	    }


	    /* discard packet - unknown ulp  */
	    hostp->stats->df.hip_s.badulps++;
	    break;


	  host_fsm_need_fphdr:
	  case DH_WAIT_FPBUF:
	  case DH_WAIT_FPHDR:

	    d2b_sync_update(D2B_DMA_LEN);
	    process_d2bs(hostp);
	    if (hostp->stackp->flags & FP_STK_ENABLED) {

		if  ( (hostp->st == DH_WAIT_FPHDR)
		     && (hostp->stackp->flags & FP_STK_HDR_VAL)) {
		    /* got buf to dma header into! */
		    hostp->st = DH_IDLE;
		    trace(TOP_HFSM_ST, hostp->st, (hostp->flags<<16) | hostp->dma_flags,
			  (hostp->rem_len<<24) | hostp->cur_len,  
			  (hostp->cur_sm_buf_len<<16) | hostp->cur_lg_bufs);
		    goto host_fsm_got_fp_hdr;
		}

		if ((hostp->st == DH_WAIT_FPBUF)
		    && (hostp->stackp->flags & FP_STK_FPBUF_VAL)) {
		    hostp->st = DH_FP;
		    trace(TOP_HFSM_ST, hostp->st, (hostp->flags<<16) | hostp->dma_flags,
			  (hostp->rem_len<<24) | hostp->cur_len,  
			  (hostp->cur_sm_buf_len<<16) | hostp->cur_lg_bufs);
		    /* if processed all of current blk, wait for a new one. */
		    if (hostp->cur_len == 0)
		      return;
		    goto host_fsm_fp;
		}
		return;

	    }

	    /* stack was disabled while waiting for header buf */
	    /* discard packet - unknown ulp  */
	    hostp->stats->df.hip_s.badulps++;
	    break;


	  host_fsm_fp:
	  case DH_FP:
	    /* we have a ptr to fpbufs to dma data into */

	    if (ablk->flags & DBLK_READY) { /* still working on ablk */
		dprintf(1, ("--- Start BLK: clen=0x%x,avail=0x%x\n", hostp->cur_len, ablk->avail));
    
		/* since sched_fphdr could have stuffed two dma's, max can stuff now is 2 */
		fpbuf_fill_dma_pipe(hostp, ablk, 2);

		/* if more to dma, and more bufs avail, then continue stuffing dma queue */
		hostp->flags |= ((hostp->cur_len > 0) && (hostp->stackp->fpbuf_len != 0))
		    ? DF_STUFFING | DF_PENDING : DF_PENDING;

		if ( !(ablk->flags & DBLK_EOP))
		    hostp->flags |= DF_NEOP; /* not-end-of-pkt */
		return;
	    }

	    /* prior pkt done, can process new one */
	    ASSERT(wblk->flags & DBLK_READY);
	    copy_blk_wtoa(wblk, ablk, hostp, hostp->stack);
	    hostp->cur_len = ablk->avail;

	    if (hostp->stackp->flags & FP_STK_ENABLED) {
		if ( !(ablk->flags & DBLK_ERR_MASK)) { /* not an error */
		    dprintf(1, ("--- Start BLK: clen=0x%x,avail=0x%x\n", 
				hostp->cur_len, ablk->avail));
		    
		    /* since sched_fphdr could have stuffed two dma's, max can stuff now is 2 */
		    fpbuf_fill_dma_pipe(hostp, ablk, 2);
		    
		    /* if more to dma, and more bufs avail, then continue stuffing dma queue */
		    hostp->flags |= ((hostp->cur_len > 0) && (hostp->stackp->fpbuf_len != 0))
			? DF_STUFFING | DF_PENDING : DF_PENDING;
		    return;
		}
		
		ASSERT(hostp->flags & DF_HOST_KNOWS);
		/*  wire error and host knows about cur packet!!!
		 * complete read with an error - dma up status and reset fsm
		 */
		{
		    hip_b2h_t b2h;
		    /* dma IN_DONE to host */
		    
		    b2h.b2h_op = HIP_B2H_IN_DONE | hostp->stack;
		    b2h.b2h_l = ablk->flags>>DBLK_ERR_MASK_SHIFT;
		    
		    b2h.b2h_l = 1<<31; /* make it negative */
		    b2h.b2h_l |= (ablk->flags & DRRD_NO_PKT_RCV) ?      B2H_IERR_NO_PKT_RCV : 0;
		    b2h.b2h_l |= (ablk->flags & DRRD_NO_BURST_RCV) ?    B2H_IERR_NO_BURST_RCV : 0;
		    b2h.b2h_l |= (ablk->flags & DRRD_READY_ERR) ?       B2H_IERR_READY : 0;
		    b2h.b2h_l |= (ablk->flags & DRRD_LINKRDY_ERR) ?     B2H_IERR_LINKLOST_ERR : 0;
		    b2h.b2h_l |= (ablk->flags & DRRD_FLAG_ERR) ?        B2H_IERR_FLAG_ERR : 0;
		    b2h.b2h_l |= (ablk->flags & DRRD_FRAMING_ERR) ?     B2H_IERR_FRAMING : 0;
		    b2h.b2h_l |= (ablk->flags & DRRD_BURST_SIZE_MASK) ? B2H_IERR_ILBURST : 0;
		    b2h.b2h_l |= (ablk->flags & DRRD_ERR_ST_MASK) ?     B2H_IERR_STATE : 0;
		    b2h.b2h_l |= (ablk->flags & DRRD_LLRC_ERR) ?        B2H_IERR_LLRC : 0;
		    b2h.b2h_l |= (ablk->flags & DRRD_PAR_ERR) ?         B2H_IERR_PARITY : 0;
		    
		    b2h_queue(&b2h);
		    b2h_push(hostp);
		}
	    }
	    else		/* stack was disabled in middle of pkt */
		hostp->stats->df.hip_s.badulps++;
	    break;
    
	  case DH_FEOP:
	    /* copying blk in prep for throwing it away */
	    copy_blk_wtoa(wblk, ablk, hostp, hostp->stack);
	    break;

	  case DH_LE:
	  default:
	    assert(0);
	    break;
	}

    /* if we got to here, the packet was discarded */

    if (ablk->flags & DBLK_EOP) {
        hostp->rem_len = 0;
	hostp->st = DH_IDLE;
    }
    else
	hostp->st = DH_FEOP;

    trace(TOP_HFSM_ST, hostp->st, (hostp->flags<<16) | hostp->dma_flags,
	  (hostp->rem_len<<24) | hostp->cur_len,  
	  (hostp->cur_sm_buf_len<<16) | hostp->cur_lg_bufs);
    /* reset FPBUF state */
    hostp->flags &= ~(DF_NEOP | DF_HOST_KNOWS);
    FPBUF.num_valid = 0;
    hostp->stackp->flags &=  ~FP_STK_FPBUF_VAL;

    dprintf(1, ("host_fsm: DISCARD pkt, ulp = 0x%x, addr = 0x%x, d1=0x%x\n", 
		ablk->ulp, ablk->dp, ablk->d1_size));
    update_dl2rr_datap(ablk->dp, ablk->avail + ablk->pad);
    ablk->flags = DBLK_NONE;
    hostp->cur_len = 0;

}


/************************************************************
  wire_inc_err
************************************************************/

void
wire_inc_err(dst_rr2l_t *rr2l, u_int bytes) {
    u_int flags = rr2l->flag;
    char c=0, p=0,b=0;

    if (rr2l->flag & DRRD_EOP) { /* error state only valid if EOP */
	if (flags & DRRD_PAUSE_NO_DESC)
	    /* no idea what to increment here */
	    ;

	if (flags & DRRD_PAUSE_NO_BUFF)
	    /* no idea what to increment here */
	    ;

	if (flags & DRRD_READY_ERR)  {
	    State->stats->df.hip_s.rdy_err++;
	}

	if (flags & DRRD_NO_BURST_RCV) {
	    State->stats->df.hip_s.bad_pkt_st_err++;
	    State->stats->df.hip_s.resvd++;
	    c++; p++;
	}

	if ((flags & DRRD_FRAMING_ERR) || (flags & DRRD_ERR_ST_MASK))
	    State->stats->df.hip_s.frame_state_err++;

	if (flags & DRRD_PAR_ERR) {
	    State->stats->df.hip_s.par_err++;
	    p++; c++; b++;
	}

	if (flags & DRRD_LLRC_ERR) {
	    State->stats->df.hip_s.llrc++;
	    p++; c++; b++;
	}

	if (flags & DRRD_NO_PKT_RCV) {
	    State->stats->df.hip_s.nullconn++;
	    c++;
	}

	if (flags & DRRD_LINKRDY_ERR)
	    State->stats->df.hip_s.pkt_lnklost_err++;

	if (flags & DRRD_FLAG_ERR)
	    State->stats->df.hip_s.flag_err++;

	if (flags & DRRD_BURST_SIZE_MASK) {
	    State->stats->df.hip_s.illbrst++;
	    p++; c++; b++;
	}

	/* clean up packet, connection, and byte count */
	if (!p) {
	    State->stats->hst_d_packets--;
	}
	if (!c) {
	    State->stats->hst_d_conns--;
	}
	if (!b) { /* must deal with carry */
	    if (State->stats->df.hip_s.numbytes_lo == 0)
		State->stats->df.hip_s.numbytes_hi--;
	    State->stats->df.hip_s.numbytes_lo -= bytes;
	}

    }
}

/************************************************************
  extract_fp_data
************************************************************/

int
extract_fp_data(dst_wire_t *wirep, wire_blk_t *wblk, dst_rr2l_t *rr2l) {
    hippi_fp_t *fp;

    if(wblk->flags == DBLK_NONE) {
	wblk->avail = 0;
	if(State->flags & FLAG_HIPPI_PH)
	    wblk->bytes_needed = 0;
	else
	    wblk->bytes_needed = sizeof(hippi_fp_t) + sizeof(u_int);
    }

    if (rr2l->len >= wblk->bytes_needed) {
	wblk->bytes_needed = 0;
	if( !(wblk->flags & DBLK_ADDR_VALID)) 
	    wblk->dp = (uint *)PHYS_TO_K1(rr2l->addr);

	if(rr2l->flag & DRRD_IFP || (wblk->flags & DBLK_IFIELD)) {
	    wblk->ifield = *wblk->dp++;
	    if (wblk->dp >= State->data_M_endp)
		wblk->dp -= State->data_M_len;
	    /* avail is a running total */
	    wblk->avail += rr2l->len - 4;	/* remove ifield */
	}
	else
	    wblk->avail += rr2l->len;


	/* rr added a 4 byte pad at end? */
	wblk->pad =  (rr2l->flag & DRRD_LAST_WORD_ODD) ? 4 : 0; 

	if( !(State->flags & FLAG_HIPPI_PH)) {
	    u_int *ip;
	    /* not PH - FP field is valid */
	    fp = (hippi_fp_t *)wblk->dp;
	    wblk->ulp = fp->hfp_ulp_id;

	    if (fp->hfp_flags & HFP_FLAGS_P) {
		wblk->flags |=  DBLK_P_BIT;
		wblk->d1_size = (fp->hfp_d1d2off & HFP_D1SZ_MASK);
	    }
	    else
		wblk->d1_size = 0;

	    /* d2_offset is from start of fp */
	    wblk->d2_offset = (fp->hfp_d1d2off & HFP_D2OFF_MASK) 
		               + wblk->d1_size + 8;

	    wblk->flags |= (fp->hfp_flags & HFP_FLAGS_B) ? DBLK_B_BIT : 0;

	    ip = (u_int*)(wblk->dp + 1);
	    if (ip >= State->data_M_endp)
		ip = (u_int*)State->data_M;

	    wblk->d2_size = *ip & 0x7fffffff;

	    wblk->flags |= DBLK_FP_VALID;
	}
	else { /* in PH mode */
	    wblk->d1_size = 0;
	    wblk->d2_size = wblk->avail;
	    wblk->d2_offset = 0;
	}

	return(0);
    }

    if ( !(wblk->flags & DBLK_ADDR_VALID)) { /* first time seen this packet */
	wblk->flags |= DBLK_ADDR_VALID;
	wblk->dp = (uint *)PHYS_TO_K1(rr2l->addr);
	wblk->avail = rr2l->len;
	wblk->flags |= (rr2l->flag & DRRD_IFP) ? DBLK_IFIELD : 0;
    }

    wblk->bytes_needed -= rr2l->len;

    return (1);
	
}

/************************************************************
  wire_fsm
************************************************************/

void 
wire_fsm(dst_wire_t *wirep, wire_blk_t *wblk) {
    /* this state machine is made more perverse because we can never assume
       an end of connection unless an error occurs. Thus the main states
       are NEOP (not end of packet) and NEOC (not end of connection).
       */
    dst_rr2l_t rr2l;
    volatile dst_rr2l_t *rr2lp;
    u_int tmp;

wire_fsm_try_again:
    rr2lp =  drr2l_get(); 
    if(rr2lp) {			/* got a new descriptor */
	rr2l = *rr2lp;
	switch(wirep->st)
	    {
	      case DW_NO_FP:
		/* this is a halfway state - blk doesn't really get 
		 * processed until next time through switch. 
		 * Fixes up rr2l (removes length of FP hdr)
		 *  and then goes to appropriate FSM
		 */
		tmp = State->stats->df.hip_s.numbytes_lo>>31; 

		State->stats->df.hip_s.numbytes_lo += rr2l.len;
		if (State->stats->df.hip_s.numbytes_lo>>31 == 0 & tmp) {
		    /* 31st bit went from 1 to 0 - must carry to hi word */
		    State->stats->df.hip_s.numbytes_hi++;
		}

		extract_fp_data(wirep, wblk, &rr2l);

		if(rr2l.flag & DRRD_EOP) {
		    if(rr2l.flag & DRRD_ERR_MASK) { /* error occured */
			wirep->st = DW_NEOC;
			wire_inc_err(&rr2l, rr2l.len);
			/* give credits back to roadrunner */
			update_dl2rr_datap(wblk->dp, 
					   wblk->avail + wblk->pad);
			wblk->flags = DBLK_NONE;
			trace(TOP_WFSM_ST, wirep->st, (u_int)rr2l.addr, rr2l.len, rr2l.flag);
			break;
		    }
		    wirep->st = DW_NEOC;
		    if(wblk->bytes_needed == 0) 
			wblk->flags |= DBLK_READY | DBLK_EOP;
		    else {
			/* didn't get a full fp header, throw it away */
			if (wblk->bytes_needed != 0)
			    update_dl2rr_datap(wblk->dp, 
					       sizeof(hippi_fp_t) 
					       + sizeof(u_int)
					       - wblk->bytes_needed);
			State->stats->df.hip_s.bad_pkt_st_err++;
			wblk->flags = DBLK_NONE;
		    }
		    break;
		}
		else if (wblk->bytes_needed != 0) /* still not enough header */
		    goto wire_fsm_try_again;


		/* got enough header, not end of packet */
		wirep->st = DW_NEOP;

		trace(TOP_WFSM_ST, wirep->st, (u_int)rr2l.addr, rr2l.len, rr2l.flag);

		/* this rr2l didn't have EOP, wait for next rr2l to get EOP */
		goto wire_fsm_try_again;


	      case DW_NEOP:
		{
		    /* get sign bit */
		    tmp = State->stats->df.hip_s.numbytes_lo>>31; 

		    State->stats->df.hip_s.numbytes_lo += rr2l.len;

		    if (State->stats->df.hip_s.numbytes_lo>>31 == 0 & tmp) {
			/* 31st bit went from 1 to 0 - must carry to hi word */
			State->stats->df.hip_s.numbytes_hi++;
		    }

		    if(rr2l.flag & DRRD_IFP) { 
			/* unexpected start of packet - can't clean up
			 * byte/pkt/conn counter because not enough info
			 */
			State->stats->df.hip_s.frame_state_err++;
			
			if (wirep->blks_sent != 0) {
			    /* missed an EOP on prior packet - must recover 
			     * by sending dummy blk to host_fsm to 
			     * clear state.
			     * Process this rr2l later
			     */
			    dprintf(1, ("wire_fsm: ERROR: Unexpect pkt, flgs = 0x%x\n",
					rr2l.flag));
			    drr2l_put_one_back();
			    wblk->flags = DBLK_READY | DBLK_ERR_SEQ;
			    trace(TOP_WFSM_ST, wirep->st, (u_int)rr2l.addr, rr2l.len, rr2l.flag);
			    break;
			}
			else { 
			    /* didn't pass a blk to host_fsm, so don't need 
			     * to clean up
			     */
			    State->stats->hst_d_conns++;
			    State->stats->hst_d_packets++;
			}
		    }

		    if(wirep->blks_sent != wirep->old_blks_sent) {
			/* new block of same packet */
			wblk->dp = (u_int*)PHYS_TO_K1(rr2l.addr);
			wblk->d2_offset = 0;
			wblk->d1_size = 0;
			wirep->old_blks_sent = wirep->blks_sent;
			wblk->avail = rr2l.len;
		    }
		    else {
			wblk->avail += rr2l.len;
			if (State->flags & FLAG_HIPPI_PH)
			    wblk->d2_size += rr2l.len;
		    }

		    if(rr2l.flag & DRRD_EOP) {
			if(rr2l.flag & DRRD_ERR_MASK) { /* error occured */
			    wirep->st = DW_NEOC;
			    wire_inc_err(&rr2l, rr2l.len);
			    /* give credits back to roadrunner */
			    update_dl2rr_datap(wblk->dp, 
					       wblk->avail + wblk->pad);
			    if (wirep->blks_sent) {
				/* already sent a block to host_fsm, must
				 *  send error block
				 */

				/* lump errors that should never occur
				 * as being a sequence error 
				 */
				wblk->flags |= (rr2l.flag & (DRRD_PAUSE_NO_DESC
							     | DRRD_PAUSE_NO_BUFF
							     | DRRD_NO_PKT_RCV))
				    ? DBLK_ERR_SEQ : 0;

				wblk->flags |= (rr2l.flag & ( DRRD_READY_ERR
							     | DRRD_NO_BURST_RCV 
							     | DRRD_FRAMING_ERR 
							     | DRRD_ERR_ST_MASK))
				    ? DBLK_ERR_SEQ : 0;

				wblk->flags |= (rr2l.flag & DRRD_PAR_ERR)
				    ? DBLK_ERR_PARITY : 0;

				wblk->flags |= (rr2l.flag & DRRD_LLRC_ERR)
				    ? DBLK_ERR_LLRC : 0;

				wblk->flags |= (rr2l.flag & DRRD_LINKRDY_ERR)
				    ? DBLK_SDIC : 0;

				wblk->flags |= (rr2l.flag & DRRD_FLAG_ERR)
				    ? DBLK_ERR_SYNC : 0;

				wblk->flags |= (rr2l.flag & DRRD_BURST_SIZE_MASK)
				    ? DBLK_ILBURST : 0;

				wblk->flags |= DBLK_READY | DBLK_EOP;
			    }
			    else
				wblk->flags = DBLK_NONE;

			    trace(TOP_WFSM_ST, wirep->st, (u_int)rr2l.addr, rr2l.len, rr2l.flag);
			    break;
			}
			wirep->st = DW_NEOC;
			wblk->flags |= DBLK_READY | DBLK_EOP;

			dprintf(1, ("wire_fsm: NEOP add 0x%x\n", rr2l.len));
			if (rr2l.flag & DRRD_LAST_WORD_ODD) 
			    wblk->pad = 4; /* rr added a 4 byte pad at end */
		    }
		     /* not EOP - but sufficient to pass on? */
		    else if (wblk->avail >= State->blksize) {
			dprintf(1, ("wire_fsm: blk done\n"));
			wblk->flags |= DBLK_READY;
			wirep->blks_sent++;
		    }
		    else
			goto wire_fsm_try_again;


		}
		trace(TOP_WFSM_ST, wirep->st, (u_int)rr2l.addr, rr2l.len, rr2l.flag);
			
		break;

	      case DW_NEOC:	
		/* not end of connection - could be a new connection.
		 * it's at least a new packet
		 */
		{
		    /* get sign bit */
		    tmp = State->stats->df.hip_s.numbytes_lo>>31; 

		    State->stats->hst_d_packets++;
		    wirep->blks_sent = 0;
		    wirep->old_blks_sent = 0;

		    if ((rr2l.flag & DRRD_IFP)) { /* new connection */
			State->stats->hst_d_conns++;

			/* don't count ifield */
			State->stats->df.hip_s.numbytes_lo += rr2l.len-4; 
		    }
		    else
			State->stats->df.hip_s.numbytes_lo += rr2l.len;

		    if (State->stats->df.hip_s.numbytes_lo>>31 == 0 & tmp) {
			/* 31st bit went from 1 to 0 - must carry to hi word */
			State->stats->df.hip_s.numbytes_hi++;
		    }

		    if (extract_fp_data(wirep, wblk, &rr2l))
			/* not all FP header present */
			wirep->st = DW_NO_FP;

		    if(rr2l.flag & DRRD_EOP) {
			if(rr2l.flag & DRRD_ERR_MASK) { /* error occured */
			    dprintf(1, ("wire_fsm: RR ERROR, flags = 0x%x\n", 
					rr2l.flag));
			    wirep->st = DW_NEOC;
			    wblk->flags = DBLK_NONE;
			    wire_inc_err(&rr2l, wblk->avail);
			    update_dl2rr_datap(wblk->dp, 
					       wblk->avail + wblk->pad);
			    trace(TOP_WFSM_ST, wirep->st, (u_int)rr2l.addr, rr2l.len, rr2l.flag);
			    break;
			}

			if (wirep->st != DW_NO_FP)
			    wblk->flags |= DBLK_READY | DBLK_EOP;

			else {	/* not enough for a valid fp - null packet */
			    wirep->st = DW_NEOC;
			    if (wblk->bytes_needed != 0)
				update_dl2rr_datap(wblk->dp, 
						   sizeof(hippi_fp_t) 
						   + sizeof(u_int)
						   - wblk->bytes_needed);
			    State->stats->df.hip_s.bad_pkt_st_err++;
			    wblk->flags = DBLK_NONE;
			}
			  
		    }
		    else  if (wirep->st != DW_NO_FP)  {
			/* not end of packet, but have complete FP hdr */
			wirep->st = DW_NEOP;

			/* not EOP - but sufficient to pass on? */
			if (wblk->avail >= State->blksize) {
			    wblk->flags |= DBLK_READY;
			    wirep->blks_sent++;
			}
			else
			    goto wire_fsm_try_again;

		    }

		    trace(TOP_WFSM_ST, wirep->st, (u_int)rr2l.addr, rr2l.len, rr2l.flag);
		    break;
		}

	      default:
		assert(0);
	    }
	/* should only be called at the end of a packet or an error */
	update_dl2rr_descp(rr2lp);
    }

}



/************************************************************
  check_and_fill_pipe
************************************************************/

void
check_and_fill_pipe(dst_host_t *hostp, ack_blk_t *blk) {
    int avail;

    /* if room in the dma quue for another descriptor... */
    avail = MAX_DMAS_ENQUEUED -  
	LINC_DCSR_VAL_DESC_R(LINC_READREG(LINC_DMA_CONTROL_STATUS_0));
    if (avail > 1) {
	/* need at least 2 dmas to cover wrap condition */
	switch (hostp->st) 
	    {
	      case DH_LE:
		mbuf_fill_dma_pipe(hostp, avail);
		break;

	      case DH_FP:
		fpbuf_fill_dma_pipe(hostp, blk, avail);
		if (hostp->stackp->fpbuf_len == 0)
		    hostp->flags &= ~DF_STUFFING;
		break;
			   
	      case DH_IDLE:
	      case DH_WAIT_FPBUF:
	      case DH_BP:
		assert(0);
		break;
	    }

    }

    if (hostp->cur_len <= 0 ) 
	hostp->flags &= ~DF_STUFFING;
}

/************************************************************
  main_loop 
************************************************************/

void 
main_loop (dst_host_t *hostp, dst_wire_t *wirep) {
    /* handoff of packets from wire to host is handled with 2 structs
     * the wire block and the ack block struct. The wire block takes
     * a block of data off the wire (out of the wire fsm) and moves
     * it to the host fsm. The host fsm copies the info into the ack 
     * struct, frees up the wire_blk, and starts transfering the 
     * data block to the host. The wblk is full of data when DBLK_READY
     * is asserted. The ablk is full of data when DF_PENDING is asserted.
     * There is more data to dma in the block if DF_STUFFING is asserted.
     */

    int hcmd_cntr = 0;

    wblk.flags = 0;
    ablk.flags = 0;
    hostp->rem_len = 0;
    
    FPBUF.num_valid = 0;
    FPBUF.basep = State->fpbuf;

    timer_set(&State->b2h_timer);

    while (1) { 

	if ((hcmd_cntr & HCMD_COUNT_MASK) == 0) { /*deal with hcmd's */
	    if(State->old_cmdid != State->hcmd->cmd_id)
		process_hcmd(hostp, wirep);
	}
	hcmd_cntr++;
	

	/* process extremely rare timer events */
	if (State->flags & (FLAG_CHECK_RR_EN |
			   FLAG_PUSH_TO_OPP |
			   FLAG_CHECK_OPP_EN))
	    update_slow_state();

	/* periodic update of d2b queue if idle  */
	if ( !(hostp->flags & DF_STUFFING)){
#ifdef USE_MAILBOX

	    if (!(State->flags & FLAG_ASLEEP) && !(State->flags & FLAG_D2B_POLL_EN) &&
		(LINC_READREG(LINC_MAILBOX_STATUS) & (1 << HIP_DST_D2B_RDY_MBOX))) {
	      
		*(volatile __uint64_t *)PHYS_TO_K1(LINC_MAILBOX(HIP_DST_D2B_RDY_MBOX));
	      
		/* setup polling and set to timeout immediately */
		State->flags |= FLAG_D2B_POLL_EN;
		State->poll_timer = 0;
		D2B->poll_timeout = 0;
	    }
	    
	    if ((State->flags & FLAG_D2B_POLL_EN) && 
		timer_expired(State->poll_timer, D2B->poll_timeout)) {
	      
		wait_usec(1);
		d2b_sync_update(D2B_DMA_LEN);
	      
		if (!d2b_check()) {
		    D2B->poll_timeout = (D2B->poll_timeout >= D2B_POLL_TIMER) ? 
			D2B_POLL_TIMER : ++D2B->poll_timeout * 2;
				
		    timer_set(&State->poll_timer);
		}
		else {
		    process_d2bs(hostp);
		    State->flags &= ~FLAG_D2B_POLL_EN;
		}
	    }

#else /* !USE_MAILBOX */

	    if ( ( !(State->flags & FLAG_ASLEEP)
		   && timer_expired(State->poll_timer, C2B_POLL_TIMER))
		 || (State->flags & FLAG_D2B_POLL_EN)) {
		timer_set(&State->poll_timer);
		d2b_sync_update(D2B_DMA_LEN);
		process_d2bs(hostp);
	    }

#endif
	}


	/* don't even look at queue if prior blk not processed */
	if( !(wblk.flags & DBLK_READY))
	    wire_fsm(wirep, &wblk);

	/* wblk available and ack blk been processed */
	if( !(hostp->flags & DF_PENDING)) {
	    /* we're not in middle of transfer to host */
	    if ( (wblk.flags & DBLK_READY) 
		  || (hostp->st == DH_WAIT_FPBUF) 
		  || (hostp->st == DH_WAIT_FPHDR)) {
		/* either new block, or waiting for fp buffers */
		host_fsm(hostp, &wblk, &ablk);
	    }
	}

	/* in middle of packet transfer? */
	if (hostp->flags & DF_PENDING) {
	    if(hostp->flags & DF_STUFFING)
		check_and_fill_pipe(hostp, &ablk); 

	    else if (dma0_done())
		/* dma just completed - clean up state */
		dma_cleanup(hostp, &ablk);
	}

	
	if ( (State->flags & FLAG_NEED_HOST_INTR)
	    && !(State->flags & FLAG_BLOCK_INTR)) {
	    intr_host();
	}
	    
	/* collect interrupts for a timer interval */
	if ( (State->flags & FLAG_BLOCK_INTR)
	    && timer_expired(State->b2h_timer, B2H_TIMER)) {

	    /* if interrupt not cleared, don't allow another to be posted */
	    if (LINC_READREG(LINC_HISR) & LINC_HISR_TO_HOST_INT_0)
		timer_set(&State->b2h_timer);
	    else
		State->flags &= ~FLAG_BLOCK_INTR;
	}
    }
}
