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
/* Source firmware */
#define HIPPI_SRC_FW


#include <sys/types.h>

#include "r4650.h"
#include "sys/PCI/linc.h"
#include "hippi_sw.h"
#include "rdbg.h"
#include "lincutil.h"


extern state_t   	*State;
extern sl2rr_ack_t 	*SL2RR_ACK;
extern srr2l_state_t	*SRR2L;
extern sl2rr_state_t	*SL2RR;

src_blk_t blk;
d2b_state_t 		*D2B;
b2h_state_t 		*B2H = 0;




#define d2b_enough(num) ((((D2B->end_valid - D2B->get) & (LOCAL_D2B_LEN-1)) >= num) \
			 ? 1 : 0)

/************************************************************
  launch_d2b_dma
************************************************************/

void
launch_d2b_dma(hip_d2b_t *get, 
	       src_host_t *hostp, 
	       src_blk_t *blk) {
    /* stateless - deal with a single dma. Use flags
       to direct what to do 
    */

    int len = get->sg.len;	/* length and DMA flags */
    u_int host_addr_hi;
    u_int host_addr_lo;
    u_int local_addr;

    if (len > HOST_LINESIZE)
	host_addr_hi = (u_int)(get->sg.addr>>32) | PPCIHI_ATTR_PREFETCH;
    else
	host_addr_hi = (u_int)(get->sg.addr>>32);

    host_addr_lo = (u_int)get->sg.addr;

    /* don't chain checksum if start of packet */
    len |= (blk->dma_flags & (SDMA_START_PKT)) ? 0 : LINC_DTC_CHAIN_CS;
    len |= (blk->dma_flags & (SDMA_SAVE_CS)) ? LINC_DTC_SAVE_CS : 0;
    len |= LINC_DTC_D64 | LINC_DTC_RD_CMD_READ_MULTIPLE;

#ifndef BRIDGE_B_WAR
    /* chain buffer address? */
    if ( blk->dma_flags & SDMA_START_PKT ) {
	local_addr = (u_int)K1_TO_PHYS(hostp->dp_put);
	LINC_WRITEREG( LINC_DMA_BUFMEM_ADDR_0, local_addr);
    }
    else {
	len |= LINC_DTC_CHAIN_BA;
#ifdef DEBUG
	local_addr = 0xbeadface;
#endif
    }

#else 
    local_addr = (u_int)K1_TO_PHYS(hostp->dp_put);
    LINC_WRITEREG( LINC_DMA_BUFMEM_ADDR_0, local_addr);
#endif /* BRIDGE_B_WAR */

    /* len and address must be word aligned */
    ASSERT( ((len & 0x3) == 0) && ((get->sg.addr & 0x3) == 0));

    dprintf(1, ("d2b dma: host:0x%x,%x local:0x%x  len: 0x%x\n",
		host_addr_hi,
		host_addr_lo, 
		local_addr,
		len));
    
    trace(TOP_DMA0, T_DMA_D2B_DATA, local_addr, host_addr_lo, len);
    
    LINC_WRITEREG( LINC_DMA_HIGH_PPCI_ADDR_0, host_addr_hi);
    LINC_WRITEREG( LINC_DMA_LOW_PPCI_ADDR_0, host_addr_lo);
    LINC_WRITEREG( LINC_DMA_TRANSFER_CONTROL_0, len ); /* GO! */

#ifdef BRIDGE_B_WAR
    if ((host_addr_hi & PPCIHI_ATTR_PREFETCH)
	&& (host_addr_lo + (len & 0x1ffff)) & (State->nbpp-1))
	/* last dma used prefetch and did not end on a page boundary */
	dma0_flush_prefetch();
#endif
    
    hostp->dp_put += get->sg.len>>2; /* can't wrap */

}

/************************************************************
  sched_d2b_rd
************************************************************/

void
sched_d2b_rd(src_host_t *hostp, src_blk_t *blk, int dmas_left) {
/*  This routine understands how to deal with multiple chunks within
 *  a single block. It assumes the DMA FIFO is empty, which allows it
 *  to stuff 4 chunks.
*/

    /* if odd word aligned, round addr down and length up 
     * and increment header pad 
     *
     * loop shoving dma commands until queue is full or no more 
     */

    hip_d2b_t *d2b_dp;

    while ( (hostp->chunks > 0) 
#ifdef BRIDGE_B_WAR		
	   /* adds a 2nd dma to invalidate bridge prefetch buffer */
	   && (dmas_left > 1)
#else
	   && (dmas_left > 0)
#endif
	   ){
	if ((d2b_dp = d2b_get()) == 0) {
	    d2b_sync_update(hostp->chunks);
	    d2b_dp = d2b_get();
	    ASSERT(d2b_dp);
	}

	blk->len += d2b_dp->sg.len;


	if (hostp->chunks_left == 0 && hostp->chunks == 1) {
	    blk->dma_flags |= (hostp->msg_flags & SMF_CS_VALID) 
		? SDMA_SAVE_CS : 0;

	    /* fix odd byte length */
	    if (d2b_dp->sg.len & 3) {
		dprintf(1, ("\t ADD TAIL PAD, len = 0x%x rounded to 0x%x\n", 
			    d2b_dp->sg.len,  4 + (d2b_dp->sg.len & ~3)));
		d2b_dp->sg.len = 4 + (d2b_dp->sg.len & ~3);
	    }
	}
#ifdef DEBUG
	else {
	    if (hostp->chunks_left == 0)
		ASSERT((d2b_dp->sg.len & 3) == 0);
	}
#endif
	
	launch_d2b_dma(d2b_dp, hostp, blk);

#ifdef BRIDGE_B_WAR
        dmas_left -= 2;
#else
	dmas_left -= 1;
#endif
	hostp->chunks--;
	blk->dma_flags &= ~SDMA_START_PKT;

    }

}




/************************************************************
  dma_cleanup - cleanup after a dma 
************************************************************/

int 
dma_cleanup(src_host_t *hostp, src_blk_t *blk) {
    int pad;

    hostp->msg_flags |= SMF_READY;	/* long word aligned */
    hostp->msg_flags &= ~SMF_PENDING;

    switch(hostp->st) {
      case SH_BP_ACTIVE:
	  
	dprintf(1, ("dma_cleanup: BP_ACTIVE\n"));
	store_bp_dma_status(DMA_OFF,0, 0, 0);
	hostp->st = SH_IDLE;

	/* two word pad at end to cover bypass odd word overspray 
	 * and for CPCI prefetch flush
	 */
	hostp->dp_put += (blk->len + blk->tail_pad) >> 2;
	if(hostp->dp_put >= hostp->endp)
	    hostp->dp_put -= hostp->data_M_len;

	break;

      case SH_D2B_ACTIVE:

	/* padded start of dma xfer to a long word,
	 * clean it up if necessary
	 */


	if (hostp->chunks_left == 0)	{
	    /* processed all data d2bs in block - we know that chunks == 0*/

	    blk->num_d2bs = 1;

	    /* set flags to what d2b originally stated */
	    blk->flags &= ~(SBLK_NEOP | SBLK_NEOC);
	    blk->flags |= (hostp->msg_flags & SMF_NEOC) ? SBLK_NEOC : 0;
	    blk->flags |= (hostp->msg_flags & SMF_NEOP) ? SBLK_NEOP : 0;

	    if ( !(hostp->msg_flags & SMF_NEOP)) { /* end of packet */

		/* merge checksum into packet */
		if ((hostp->msg_flags & SMF_CS_VALID)) {
		    unsigned short *cksump;
		    u_int checksum;

		    checksum = LINC_READREG(LINC_DMA_RES_CHECKSUM_0);

		    ASSERT((LINC_READREG(LINC_DMA_CONTROL_STATUS_0) 
			    & LINC_DCSR_NUM_CHK_MASK) == 0);

		    dprintf(1, ("host chksum 0x%x, dma checksum = 0x%x\n", 
				*(unsigned short *)( (char*)blk->dp  
						    + hostp->cksum_offs), 
				checksum));
		    checksum = (checksum & 0xffff) + (checksum>>16);
		    checksum = (checksum & 0xffff) + (checksum>>16);

		    checksum = 0xffff & ~checksum;
	    

		    dprintf (1, ("final checksum= 0x%x\n", checksum));


		    /* blocks can't wrap in buffer - so don't need wrap check
		     */
		    cksump = (unsigned short *)
			( (char*)blk->dp  + hostp->cksum_offs);

		    *cksump = (unsigned short)checksum;
		
		    hostp->msg_flags &= ~SMF_CS_VALID;
		    /*		    chksum(blk->dp, blk->len>>2);*/
		}
	    }

	    /* clean up the packet. This entails:
	     *   1) remove ifield from header
	     *   2) round length to long word
	     *   3) clear tail padding to zeros
	     *   4) if rounding of length pads data past hostp->dp, 
	     *      inc hostp->dp
	     *   5) add tail_pad if needed to make 
	     *      blk->dp + len + pad = hostp->dp_put
	     */

	    /* get ifield from packet, if needed */
	    if (hostp->msg_flags & SMF_NEED_IFIELD) {
		blk->ifield = *(u_int*)(blk->dp++); 
		blk->len -= sizeof(u_int);
		hostp->msg_flags &= ~SMF_NEED_IFIELD;
	    }

	    if (blk->len & 3) {
		dprintf(1, ("padding w/ zero, addr = 0x%x, len = %d\n",
			    (char*)blk->dp + blk->len, 4 - (blk->len & 3)));
		while (blk->len & 3) { /* must clean up bytes at end */ 
		    char *i = (char*)blk->dp + blk->len;
		    dprintf(1, ("%x", *i));
		    *i = 0;
		    blk->len++;
		}
		dprintf(1, ("\n"));
	    }
	    
	    if (blk->len & 4) { /* now clean up word if needed */
		dprintf(1, ("padding w/ 4 bytes, addr = 0x%x\n", 
			    blk->dp + blk->len));
		*(blk->dp + (blk->len>>2)) = 0;
		blk->len += 4;
		hostp->dp_put++;
	    }

	    hostp->msg_flags &= ~SMF_PENDING;

	    if ( !(hostp->msg_flags & SMF_NEOC)) {
		/* end of connection, go to idle */
		hostp->st = SH_IDLE;
		hostp->rem_len = 0;
	    }
	    else {		/* continue connection */
		hostp->st = SH_D2B_IDLE;
		if (hostp->msg_flags & SMF_NEOP) {
		    int no_fburst_len;
		    /* not end of packet, so must round to 1 KB boundary */
		    no_fburst_len = blk->len - blk->fburst;
		    hostp->rem_len = no_fburst_len & (1024-1);
		    if (hostp->rem_len)
			/* make it 1KB multiple */
			blk->len = blk->fburst + (no_fburst_len & ~(1024-1)); 

		    hostp->dp_put = blk->dp + blk->len/sizeof(u_int);
		    dprintf(1, ("dma_cleanup: done w/ blk, but NEOP\N"));
		    dprintf(1, ("\tchk_left = %d, chunks = %d, rem_len = %d, dp=0x%x\n", 
				hostp->chunks_left, hostp->chunks, hostp->rem_len,
				hostp->dp_put));
		}
	    }
	}
	else {			/* more blocks to process */
	    int no_fburst_len;

	    /* get ifield from packet, if needed */
	    if (hostp->msg_flags & SMF_NEED_IFIELD) {
		blk->ifield = *(u_int*)(blk->dp); 
		blk->dp++;	/* can't wrap - dp is fp header now */
		blk->len -= sizeof(u_int);
		hostp->msg_flags &= ~SMF_NEED_IFIELD;
	    }

	    /* checksum won't work across block because can't wrap it back in
	       after packet is sent.
	       */
	    hostp->chunks = MIN(hostp->chunks_left, State->blksize);
	    hostp->chunks_left -= hostp->chunks;

	    /* setup remainder */
	    no_fburst_len = blk->len - blk->fburst;
	    hostp->rem_len = no_fburst_len & (1024-1);
	    if (hostp->rem_len)
		/* make it 1KB multiple */
		blk->len = blk->fburst + (no_fburst_len & ~(1024-1)); 

	    hostp->dp_put = blk->dp + blk->len/sizeof(u_int);

	    /* hand off block as middle of packet */
	    blk->flags |= SBLK_NEOP | SBLK_NEOC;
	    dprintf(1, ("dma_cleanup: \tchk_left = %d, chunks = %d, rem_len = %d\n", 
			hostp->chunks_left, hostp->chunks, hostp->rem_len));
	}

	/* handle flush of CPCI prefetch by either:
	 * a) flush prefetch by making next packet be
	 * 	put in non-contigous mem (tail_pad)
	 * b) if there is a remainder, cause creation of 2 l2rr's to be
	 * 	sent - first uses continuous prefetch for all
	 *	but last 1 KB of message. Last l2rr doesn't
	 *	use persistent prefetch and prefetch is exact length,
	 *	so it will be flushed on completion.
	 */

	if (hostp->rem_len == 0) {
	    /* put tail pad to ensure CPCI prefetch is flushed for next pkt */
	    blk->tail_pad = 8;
	    hostp->dp_put += 2;
	    if(hostp->dp_put >= hostp->endp) {
		hostp->dp_put -= hostp->data_M_len;
	    }
	}
	else {
	    blk->flags |= SBLK_REM;
	    blk->tail_pad = 0;
	}

	break;

      case SH_BP_FULL:
      case SH_D2B_FULL:
      case SH_IDLE:
      default:
	/* should never occur in this routine */
	assert(0);
    }
    
}


/************************************************************
  buf_room_left
************************************************************/

int
buf_room_left(volatile u_int *put, volatile u_int *get, int len) {
    /* len = bytes */
    int room;

    room = (State->data_M_len - (put - get))%State->data_M_len;

    if (room == 0) room = State->data_M_len;

    return (len < room*4);

}

/************************************************************
  find_bp_desc
************************************************************/

int
find_bp_desc(src_host_t *hostp, src_blk_t *blk) {
    /* return 
     *  0 = no desc found
     *  1 = desc found, packet launched
     *  2 = desc found, but data queue is full
    */
    int i;
    volatile hippi_bp_desc *sdq_head;  

    /* see if any sdq have work to be done */
    for (i = 0; i < BP_MAX_JOBS; i++) { 
	/* take one turn through all jobs, 
	   start where last left off (cur_job) 
	   */
	if( (0x1<<(31-hostp->cur_job) & hostp->job_vector)) {

	    sdq_head = hostp->job[hostp->cur_job].sdq_head;
	    if (sdq_head->i[3] != HIP_BP_DQ_INV) { /* valid descriptor? */

		/* room for max length message? */
		if( !buf_room_left(hostp->dp_put, 
				  hostp->dp_noack, MAX_LEN_BP_PKT))
		    return(2);	/* buffer full - retry later */

		if( (hostp->endp - hostp->dp_put) 
		       < (MAX_LEN_BP_PKT>>2)) {
		    /* can't wrap a block */
		    hostp->dp_put = hostp->basep;
			
		    if( !buf_room_left(hostp->dp_put, 
				       hostp->dp_noack, MAX_LEN_BP_PKT))
			return(2);	/* buffer full - retry later */
		}

		if ( !sched_bp_rd(hostp, blk))
			return(0); /* error occured */

		if (++hostp->cur_job == BP_MAX_JOBS)
		    hostp->cur_job = 0;

		return(1);
	    }
	}  
	if (++hostp->cur_job == BP_MAX_JOBS)
	    hostp->cur_job = 0;
    }
    return (0);
}

/************************************************************
  process_d2b_hdr
************************************************************/

void
process_d2b_hdr(src_host_t *hostp, src_blk_t *blk, hip_d2b_t *hdrp) {
    /* return 
     *  1 = desc found, packet launched
     *  2 = desc found, but data queue is full
    */
    ASSERT(hdrp->hd.flags & HIP_D2B_RDY);

    hostp->msg_flags = (hdrp->hd.flags & HIP_D2B_NEOC) ? SMF_NEOC : 0;
    hostp->msg_flags |= (hdrp->hd.flags & HIP_D2B_NEOP) ? SMF_NEOP : 0;
    hostp->msg_flags |= (hdrp->hd.flags & HIP_D2B_IFLD) ? SMF_NEED_IFIELD : 0;
    hostp->msg_flags |= (hdrp->hd.sumoff != 0xffff ) && (hdrp->hd.sumoff != 0 )
	?  SMF_CS_VALID : 0;

    /* these four lines are repeated in host_fsm with state SH_D2B_ACTIVE */
    blk->dp = hostp->dp_put;
    blk->len = hostp->rem_len;
    hostp->dp_put += blk->len/sizeof(u_int);
    hostp->rem_len = 0;

    blk->stack = hdrp->hd.stk;
    blk->fburst = hdrp->hd.fburst;
    blk->num_d2bs = 0;

    blk->flags = (hdrp->hd.flags & HIP_D2B_NACK) ? SBLK_NACK : 0;
    blk->flags |= (hdrp->hd.flags & HIP_D2B_BEGPC) ? SBLK_BEGPC : 0;
    blk->flags |= (hdrp->hd.flags & HIP_D2B_IFLD) ? SBLK_SOC : 0;
    blk->dma_flags = 0;
    blk->tail_pad = 0;

    hostp->cksum_offs = hdrp->hd.sumoff;

    trace(TOP_D2B, T_D2B_HEAD, 0, *(u_int*)hdrp, *((u_int*)hdrp + 1));

    if (hdrp->hd.chunks != 0) {
	if (hostp->cksum_offs != 0xffff) {
	    hostp->chunks = hdrp->hd.chunks;
	    hostp->chunks_left = 0;
	}
	else {
	    hostp->chunks = MIN(hdrp->hd.chunks, State->blksize);
	    hostp->chunks_left = hdrp->hd.chunks - hostp->chunks;
	}

	dprintf(1, (" >>>>>>>>>   proc_d2b: chks_left=%d, chunks = %d>>>>>\n", 
		    hostp->chunks_left, hostp->chunks));

	blk->dma_flags |= SDMA_START_PKT;
	blk->dma_flags |= (hostp->msg_flags & SMF_CS_VALID) 
	    ? SDMA_CHAIN_CS : 0;
    }
    else {	/* dummy descriptor - just pass it through */
	hostp->msg_flags |= SMF_READY | SMF_PENDING;
	hostp->msg_flags &=  ~SMF_CS_VALID;
    }
}

/************************************************************************** 
 * setup_d2b_rd
 *
 * This function returns a 1 if the data buffer has been successfully setup
 * for the dma of the d2b data. It will fail if there is not enough room in
 * the buffer for the complete contiguous block including the possible 
 * remainder from the last block. This assumes the blk->len is set to the
 * remainder length and blk->dp is set to the start of the remainder.
 **************************************************************************/

int
setup_d2b_rd(src_host_t *hostp, src_blk_t *blk) {

        /* We don't add the blk->len in this check because the remainder has
	 * already been written to the buffer and the hostp->dp_put advanced past
	 * it. We add 8 just to be safe. */
	if( !buf_room_left(hostp->dp_put, 
			  hostp->dp_noack, 
			  hostp->chunks*State->nbpp + 8))
	    return(0);		/* FULL */
	    

	/* might just need to wrap to start of buffer */
	if ( (hostp->endp - hostp->dp_put) < (hostp->chunks*(State->nbpp>>2))) {
	    /* Check if we can fit everything in at the top of the buffer.
	     * We add the blk->len here because we have to copy it on wrap .*/
	    if( !buf_room_left(hostp->basep, 
			       hostp->dp_noack, 
			       hostp->chunks*State->nbpp + blk->len + 8))
		return(0);	/* full */
	    else {
		int i;
		volatile u_int *ipf = blk->dp;
		volatile u_int *ipto = hostp->basep;
		
		/* first copy over remainder from last block */
		for (i = 0; i < blk->len/sizeof(u_int); i++) {
		    *ipto++ = *ipf++;
		}
		hostp->dp_put = ipto;
		blk->dp = hostp->basep;
	    }
	}
	
	/* get the sucker! */
	sched_d2b_rd(hostp, blk, MAX_DMAS_ENQUEUED - 
		     LINC_DCSR_VAL_DESC_R(LINC_READREG( LINC_DMA_CONTROL_STATUS_0)));

	hostp->msg_flags |= (hostp->chunks != 0) ?
	    SMF_MIDDLE | SMF_PENDING : SMF_PENDING;

	return 1;
}


/* Main finite state machine that controls transfers between the 
   host and the board. 

   Two protocol interfaces are supported - bypass and d2b's 
      (IP is a subset of d2b's)
   For d2b:
     1) find a valid d2b
     2) subdivide the header d2b into chunks_left and chunks (of blocks)
     3) get enough d2b's to do first set of chunks
     4) launch up to 4 dma's, one per chunk
     5) use check_and_fill to fill the dma pipeline until chunks == 0
     6) wait until all dma's are done
     7) in dma_cleanup clean up state for pass off to wire_fsm
     8) wire_fsm sends blk out the door. Stores enough state for later ack
     9) process_acks finds acks for each block, increments counters
        posts odones to host.

     In the case of multiple blocks, after wire_fsm frees the block structure
     the host_fsm will stay in D2B_ACTIVE until all blocks have been processed.

   For bypass, there all transfer by definition are one block.


   Some Wierd Cases:
   Data in PH is odd num words - must align next packet on long word boundary
   Must deal with short-burst-first or short-burst-last by stuffing a 
     separate sl2rr for each. All other sl2rr must be an integral num of 1KB.
   Must pad in data buffer one long word between each set of data described
     by one sl2rr.
   Must break data up if croses a 64 KB boundary.
   must break data up into multiple rr2b if crosses end of buffer, and make
      sure each is a multiple of 1 KB!!
   Translate between d2b concept of NEOP/NEOC and sl2rr SAME_IFIELD, 
      CONT_CONNECTION.
   Turn on permanent connection when driver tells it to
   Send a dummy desc to rr when driver sends dummy d2b to end PC (len = 0)
   Must deal with data buffer wrap by hand.

*/

/************************************************************
  host_fsm
************************************************************/

void
host_fsm(src_host_t *hostp, src_blk_t *blk) {
    
    int i;
    hip_d2b_t *hdrp;
    static int pkt_arb = 1;
    
    switch(hostp->st) 
	{
	  case SH_IDLE:
	  case SH_D2B_IDLE:
	    if (pkt_arb) {
		/* if we're in SH_D2B_IDLE don't check for BP pkts */
		if (!(hostp->st == SH_D2B_IDLE) && 
		    hostp->job_vector && (i = find_bp_desc(hostp, blk))) {
		
		    if (i == 1)	/* launched a packet */
			hostp->st = SH_BP_ACTIVE; 
		    else 		/* == 2 */
			/* could have launched, but data buffer full */
			hostp->st = SH_BP_FULL; 
		}
		else if (hostp->flags & SF_NEW_D2B) {
		    /* check d2b's */
		    if (hdrp = d2b_get()) {
			process_d2b_hdr(hostp, blk, hdrp);
			i = setup_d2b_rd(hostp, blk);
			if (i == 1)
			    hostp->st = SH_D2B_ACTIVE;
			else { /* FULL */
			    hostp->st = SH_D2B_FULL;
			    trace(TOP_HFSM_ST, hostp->st,
				  hostp->flags<<16 | hostp->msg_flags,
				  hostp->chunks<<16 | hostp->chunks_left,
				  hostp->cur_job<<16 | hostp->rem_len);
			}
			if ( !d2b_enough(hostp->chunks)) 
			    d2b_sync_update(hostp->chunks);
		    }
		    else		/* no more valid d2b's */
			hostp->flags &= ~SF_NEW_D2B;
		}		    
	    }
	    else {
		/* check d2b's */
		if (hostp->flags & SF_NEW_D2B) {
		    if (hdrp = d2b_get()) {
			process_d2b_hdr(hostp, blk, hdrp);
			i = setup_d2b_rd(hostp, blk);
			if (i == 1)
			    hostp->st = SH_D2B_ACTIVE;
			else { /* FULL */
			    hostp->st = SH_D2B_FULL;
			    trace(TOP_HFSM_ST, hostp->st,
				  hostp->flags<<16 | hostp->msg_flags,
				  hostp->chunks<<16 | hostp->chunks_left,
				  hostp->cur_job<<16 | hostp->rem_len);
			}
			if ( !d2b_enough(hostp->chunks)) 
			    d2b_sync_update(hostp->chunks);
		    }
		    else		/* no more valid d2b's */
			hostp->flags &= ~SF_NEW_D2B;
		}
		/* if we're in SH_D2B_IDLE don't check for BP pkts */
		else if (!(hostp->st == SH_D2B_IDLE) && 
			 hostp->job_vector && (i = find_bp_desc(hostp, blk))) {
		    /* check for bp packet */
		    if (i == 1)	/* launched a packet */
			hostp->st = SH_BP_ACTIVE; 
		    else 		/* == 2 */
			/* could have launched, but data buffer full */
			hostp->st = SH_BP_FULL; 
		}
	    }

	    /* switch which packet type we look for first */
	    pkt_arb = !pkt_arb;
	    break;
	    
	  case SH_D2B_ACTIVE:	/* start of a new block (multiple blocks) */
	    /* checksum can not work across multiple blocks because can't
	       merge it back into block already transmitted
	       */

	    dprintf(1, ("host_fsm: SH_D2B_ACTIVE\n"));

	    /* these four lines are repeated in process_d2b_hdr */
	    blk->dp = hostp->dp_put;
	    blk->len = hostp->rem_len;
	    hostp->dp_put += blk->len/sizeof(u_int);
	    hostp->rem_len = 0;

	    blk->fburst = 0;
	    blk->dma_flags = 0;
	    blk->flags &= ~SBLK_SOC;

	  case SH_D2B_FULL:

	    if (setup_d2b_rd(hostp, blk))
		hostp->st = SH_D2B_ACTIVE;
	    else {
		/* Not enough room in the data buffer. */

		hostp->st = SH_D2B_FULL; /* in case we're here from SH_D2B_ACTIVE */

		/* If we haven't got them all, go get the rest of the d2bs for 
		this transfer. */
		if ( !d2b_enough(hostp->chunks))
		    d2b_sync_update(hostp->chunks);

		trace(TOP_HFSM_ST, hostp->st, hostp->flags<<16 | hostp->msg_flags,
		      hostp->chunks<<16 | hostp->chunks_left,
		      hostp->cur_job<<16 | hostp->rem_len);
	    }

	    break;

	  case SH_BP_ACTIVE:
	    /* no action - should never hit this state in this routine*/
	    assert(0);
	    break;

	  case SH_BP_FULL:
	    if (hostp->job_vector && (i = find_bp_desc(hostp, blk))) {
		
		if (i == 1)
		    hostp->st = SH_BP_ACTIVE; /* dma was launched */
		else 		/* could have launched, but no buffer room */
		    hostp->st = SH_BP_FULL; 
		    
		break;
	    }
	    else {
		dprintf(1, ("host_fsm: WARN - drop bp pkt: job dis or err\n"));
		hostp->st = SH_IDLE;
	    }

	    break;

	  default:
	    dprintf(1, ("ERROR: invalid hostp state, %d\n", hostp->st));
	    assert(0);
	}
    
}

/************************************************************
  wire_fsm
************************************************************/

int
wire_fsm(src_wire_t *wirep, src_blk_t *blk) {

    trace(TOP_SBLK, blk->flags, blk->len, 
	  (blk->stack<<24) | blk->num_d2bs, (blk->fburst<<24) | ((u_int)blk->dp & 0xffffff));
    
    switch(wirep->st) {
      case SH_IDLE:

	if(sl2rr_put(blk)) {
	    if (blk->flags & SBLK_BEGPC)
		wirep->flags = SBLK_BEGPC;
	    
	    if (blk->flags & SBLK_NEOP) 
		wirep->st = SW_NEOP;
	    else if ((blk->flags & SBLK_NEOC)
		     || (wirep->flags & SBLK_BEGPC))
		wirep->st = SW_NEOC;
	    trace(TOP_WFSM_ST, wirep->st, wirep->flags,0,0);
	    return(1);
	}
	break;

      case SW_NEOC:
	if(sl2rr_put(blk)) {
	    
	    if (blk->flags & SBLK_NEOP) 
		wirep->st = SW_NEOP;
	    else if (! ((blk->flags & SBLK_NEOC)
			|| (wirep->flags & SBLK_BEGPC)))
		wirep->st = SW_IDLE;
	    
	    trace(TOP_WFSM_ST, wirep->st, wirep->flags,0,0);
	    return(1);
	}

	break;

      case SW_NEOP:
	if(sl2rr_put(blk)) {
	    
	    if ( !(blk->flags & SBLK_NEOP)) {
		if ((blk->flags & SBLK_NEOC)
		    || (wirep->flags & SBLK_BEGPC))
		    wirep->st = SW_NEOC;
	    
		else 
		    wirep->st = SW_IDLE;
	    }
	    trace(TOP_WFSM_ST, wirep->st, wirep->flags,0,0);
	    return(1);
	}
	break;


      default:

	dprintf(1, ("ERROR: unknown wire_fsm state\n"));
	assert(0);
    }
    return(0);

}

/************************************************************
  process_acks
************************************************************/

void 
process_acks(src_host_t *hostp, src_wire_t *wirep) {
    src_rr2l_t rr2l;
    src_l2rr_t *l2rrp;
    sl2rr_ack_t *ackp;
    sl2rr_ack_t *last_ackp = 0;
    int b2h_full = 0;
    char p=1, c=1, b=1;		/* increment appropriate stats */
    static u_int last_err;	/* last error seen */

    while (( (rr2l = srr2l_get()) != 0) && !b2h_full) {

	l2rrp = (src_l2rr_t *)PHYS_TO_K1(rr2l & SRRS_ADDR_MASK);
	/* free up all resources associated with rr2l */

	if (rr2l & SRRS_OP_MASK) {
	    /* rr will never ack errors across a packet boundary */
	    u_int err = rr2l & SRRS_OP_MASK;
	    dprintf(1, ("process_acks: ERROR OCCURRED: rr2l = 0x%x\n", rr2l));
	    
	    /* inc error stats */
	    if ( !(err == SRRS_OP_DESC_FLUSHED)) { 
		/* if not previously reported, increment errors */
		last_err = err;
		p--; c--;
		switch (err)
		    {
		      case SRRS_OP_CONN_TIMEO:
			State->stats->sf.hip_s.timeo++;
			b--;
			break;
		      case SRRS_OP_DST_DISCON:
			State->stats->sf.hip_s.connls++;
			break;
		      case SRRS_OP_CONN_REJ:
			State->stats->sf.hip_s.rejects++;
			b--;
			break;
		      case SRRS_OP_SRC_PERR:
			State->stats->sf.hip_s.par_err++;
			b--;
			break;
		      default:	/* should never occur */
			b--;
		}
	    }
	}    
	else {			/* no error - clean things up */
	    last_err = 0;
	}
	
	while ( (ackp = sget_ack(l2rrp)) != last_ackp) {
	    /* get sign bit */
	    u_int tmp = State->stats->sf.hip_s.numbytes_lo>>31; 
	
	    /* sequence through all sl2rr ack descriptors to this point.
	       Might cause multiple transitions in the state machine.
	       */
	    if (ackp->d2bs != 0 && !(ackp->flags & SBLK_NACK)) {
		hip_b2h_t b2h;
		/* push b2h odones to host */
		dprintf(1, ("process_acks: queueing an odone\n"));

		b2h.b2h_op = HIP_B2H_ODONE | (ackp->stack & HIP_B2H_STMASK);

		b2h.b2h_ndone = ackp->d2bs;

		switch (last_err & SRRS_OP_MASK) {
		  case SRRS_OP_CONN_TIMEO:
		    b2h.b2h_ostatus = B2H_OSTAT_TIMEO; break;

		  case SRRS_OP_DST_DISCON:
		    b2h.b2h_ostatus = B2H_OSTAT_CONNLS; break;

		  case SRRS_OP_CONN_REJ:
		    b2h.b2h_ostatus = B2H_OSTAT_REJ; break;

		  case SRRS_OP_SRC_PERR:
		    b2h.b2h_ostatus = B2H_OSTAT_SPAR; break;

		  case SRRS_OP_XMIT_OK:
		    b2h.b2h_ostatus = B2H_OSTAT_GOOD; break;
		    
		  default:
		    assert(0);
		}

		b2h_full = b2h_queue(&b2h);

	    }
	    
	    hostp->dp_noack = ackp->dptr;
	    ASSERT(hostp->dp_noack >= hostp->basep);
	    ASSERT(hostp->dp_noack < hostp->endp);

	    switch (wirep->ack_st) {
	      case SH_IDLE:
		if (ackp->flags & SBLK_BEGPC)
		    wirep->ack_flags = SBLK_BEGPC;
	    
		if (ackp->flags & SBLK_NEOP) {
		    dprintf(1, ("pa: to NEOP\n"));
		    wirep->ack_st = SW_NEOP;
		}
		else if ((ackp->flags & SBLK_NEOC) 
			 || (wirep->ack_flags & SBLK_BEGPC)) {
		    dprintf(1, ("pa: to NEOC\n"));
		    wirep->ack_st = SW_NEOC;

		}
		if (ackp->len != 0 || wirep->ack_st != SW_IDLE) { /* dummy descriptor */
		    if (c) State->stats->hst_s_conns++;
		    if (p) State->stats->hst_s_packets++;
		    if (b) {
			State->stats->sf.hip_s.numbytes_lo += ackp->len;
			if (State->stats->sf.hip_s.numbytes_lo>>31 == 0 & tmp) {
			    /* 31st bit went from 1 to 0 - must carry */
			    State->stats->sf.hip_s.numbytes_hi++;
			}
		    }
			

		    if( ackp->flags & SBLK_BP) { /* bypass */
			if (p) State->bpstats->hst_s_bp_packets++;
			if (b) State->bpstats->hst_s_bp_byte_count += ackp->len;
		    }
		}


		break;

	      case SW_NEOC:
		if (ackp->flags & SBLK_BEGPC)
		    wirep->ack_flags = SBLK_BEGPC;
	    
		if (ackp->flags & SBLK_NEOP) { /* partial packet */
		    dprintf(1, ("pa: to NEOP\n"));
		    wirep->ack_st = SW_NEOP;
		}
		else {		/* not a partial packet */
		    if (! ((ackp->flags & SBLK_NEOC) 
			   || (wirep->ack_flags & SBLK_BEGPC))) {
			dprintf(1, ("pa: to IDLE\n"));
			wirep->ack_st = SW_IDLE;
		    }
		    else
			dprintf(1, ("pa: to NEOC\n"));
		}

		if (c && (ackp->flags & SBLK_SOC))
		    State->stats->hst_s_conns++;

		if (ackp->len != 0 || wirep->ack_st != SW_NEOC) { /* dummy descriptor */
		    if (p) State->stats->hst_s_packets++;
		    if (b) {
			State->stats->sf.hip_s.numbytes_lo += ackp->len;
			if (State->stats->sf.hip_s.numbytes_lo>>31 == 0 & tmp) {
			    /* 31st bit went from 1 to 0 - must carry */
			    State->stats->sf.hip_s.numbytes_hi++;
			}
		    }

		    if( ackp->flags & SBLK_BP) { /* bypass */
			if (p) State->bpstats->hst_s_bp_packets++;
			if (b) State->bpstats->hst_s_bp_byte_count += ackp->len;
		    }
		}
		
		trace(TOP_RR2L, wirep->ack_st, wirep->ack_flags, rr2l, 0);

		break;

	      case SW_NEOP:
	    
		if (ackp->flags & SBLK_BEGPC)
		    wirep->ack_flags = SBLK_BEGPC;
	    
		if ( !(ackp->flags & SBLK_NEOP)) {
		    if ((ackp->flags & SBLK_NEOC)
			|| (wirep->ack_flags & SBLK_BEGPC)) {
			dprintf(1, ("pa: to NEOC\n"));
			wirep->ack_st = SW_NEOC;
		    }
		    
		    else {
			dprintf(1, ("pa: to IDLE,swire st = %d\n", wirep->st));
			wirep->ack_st = SW_IDLE;
		    }
		}
		else 
		    dprintf(1, ("pa: to NEOP\n"));
		
		if (ackp->flags & SBLK_SOC) { 
		    /* previous error, but proceed */
		    if (c) State->stats->hst_s_conns++;
		    if (p) {
			State->stats->hst_s_packets++;
			if( ackp->flags & SBLK_BP) /* bypass */
			    State->bpstats->hst_s_bp_packets++;
		    }
		}
		if (b) {
		    State->stats->sf.hip_s.numbytes_lo += ackp->len;
		    if (State->stats->sf.hip_s.numbytes_lo>>31 == 0 & tmp) {
			/* 31st bit went from 1 to 0 - must carry */
			State->stats->sf.hip_s.numbytes_hi++;
		    }
		    
		    if( ackp->flags & SBLK_BP) { /* bypass */
			State->bpstats->hst_s_bp_byte_count += ackp->len;
		    }
		}

		trace(TOP_RR2L, wirep->ack_st, wirep->ack_flags, rr2l, 0);
		break;

	      default:
		dprintf(1, ("ERROR: unknown process_ack state\n"));
		assert(0);
	    }
	    last_ackp = ackp;
	    dprintf(1, ("last_ackp = 0x%x\n", last_ackp));
	}
    }
}

/************************************************************
  check_and_fill_pipe
************************************************************/

void
check_and_fill_pipe(src_host_t *hostp, src_blk_t *blk) {

    /* could optimize this further to not do anything until DMA_INT 
       is asserted */

    switch(hostp->st) 
	{
	  case SH_D2B_ACTIVE:
	    {

		ASSERT(hostp->chunks != 0);
		sched_d2b_rd(hostp, blk,
			     MAX_DMAS_ENQUEUED - 
			     LINC_DCSR_VAL_DESC_R(LINC_READREG( LINC_DMA_CONTROL_STATUS_0)));

		if (hostp->chunks == 0)
		    hostp->msg_flags &= ~SMF_MIDDLE;

		break;
	    }

	  case SH_D2B_FULL:
	  case SH_IDLE:
	  case SH_BP_FULL:
	  case SH_BP_ACTIVE:
	  default:
	    assert(0);
	    break;
	}
}

/************************************************************
  main_loop
************************************************************/

void 
main_loop(src_host_t *hostp, src_wire_t *wirep) {


    int hcmd_cntr = 0;		/* hcmd loop counter */
    int proc_ack_cntr = 0;

    blk.flags = 0;

    timer_set(&State->b2h_timer);
    timer_set(&State->sleep_timer);
    timer_set(&State->poll_timer);

    while (1) { 
	
	if ((hcmd_cntr & HCMD_COUNT_MASK) == 0) { /*deal with hcmd's */
	    if(State->old_cmdid != State->hcmd->cmd_id)
		process_hcmd(hostp, wirep);
	}
	hcmd_cntr++;


	/* process extremely rare timer events */
	if (State->flags & (FLAG_CHECK_RR_EN |
			   FLAG_PUSH_TO_OPP |
			   FLAG_CHECK_OPP_EN)) {
	    update_slow_state();

	}

	if(  !(hostp->msg_flags & SMF_MIDDLE) && 
	     !(hostp->msg_flags & SMF_READY)) {
	    /* periodic update of d2b queue if idle  */
	    if (hostp->st == SH_IDLE || hostp->st == SH_D2B_IDLE) {
#ifdef USE_MAILBOX
		if (!(State->flags & FLAG_ASLEEP) && !(State->flags & FLAG_D2B_POLL_EN) && 
		    (LINC_READREG(LINC_MAILBOX_STATUS) & (1 << HIP_SRC_D2B_RDY_MBOX))) {
		    
		    *(volatile __uint64_t *)PHYS_TO_K1(LINC_MAILBOX(HIP_SRC_D2B_RDY_MBOX));

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

			timer_set(&State->sleep_timer);
			timer_set(&State->poll_timer);
		    }
		    else {
			hostp->flags |= SF_NEW_D2B;
			State->flags &= ~FLAG_D2B_POLL_EN;
		    }
		}
#else /* !USE_MAILBOX */

		if ( !(State->flags & FLAG_ASLEEP)
		    && timer_expired(State->poll_timer, D2B_POLL_TIMER)) {
		    d2b_sync_update(D2B_DMA_LEN);
		    timer_set(&State->sleep_timer);
		    timer_set(&State->poll_timer);
		    hostp->flags |= SF_NEW_D2B;
		}
#endif
	    }
	}

	if(  ! (hostp->msg_flags & (SMF_PENDING | SMF_READY))) {
	    host_fsm(hostp, &blk);
	}
	    

	/* in middle of packet transfer? */
	if(hostp->msg_flags & SMF_PENDING) {
	    if (hostp->msg_flags & SMF_MIDDLE) {
		check_and_fill_pipe(hostp, &blk); 
	    }
	    if ( !(hostp->msg_flags & SMF_MIDDLE) && 
		(hostp->st != SH_IDLE) && 
		dma0_done())
		  
		dma_cleanup(hostp, &blk);
	}


	/* send packets to wire */
 	if(hostp->msg_flags & SMF_READY) {
	    if (wire_fsm(wirep, &blk)) {
		/* free if sl2rr_put completes */
		hostp->msg_flags &= ~SMF_ST_MASK;
		blk.flags &= ~(SBLK_REM |  SBLK_SOC);
	    }
	}
  
	/* check if roadrunner is done sending any packets */

	if (proc_ack_cntr%PROC_ACKS_COUNT == 0) {
	    /*deal with processing ack queue */
	    process_acks(hostp, wirep);
	}
        proc_ack_cntr++;


	/* push b2h's to host */
	if (B2H->queued != 0)
	    b2h_push(hostp);		

	if ( (State->flags & FLAG_NEED_HOST_INTR)
	    && !(State->flags & FLAG_BLOCK_INTR))
	    intr_host();

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



