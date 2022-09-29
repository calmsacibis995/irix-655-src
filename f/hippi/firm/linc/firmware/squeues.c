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
 * squeues.c - queue control commands
 *
 * $Revision: 1.15 $
 *
 */




/*************************************************************
  General Queue Interfaces for HIPPI Linc firmware
  
  Linc has a ton of queues, going between the driver,
  linc, and roadrunner in each direction, for source
  and destination. Terminology is:
  
  Driver/Linc interface
  ----------------------------
  b2h - board to host
  d2b - data to board - used to pass descriptors to source linc
  
  Linc/Roadrunner Interface
  ----------------------------
  sl2rr - source linc to roadrunner descriptor queue
  srr2l - source roadrunner to linc ack queue
  
  Bypass User Space to Linc Queues
  --------------------------------
  sdq - source descriptor queue
  

  Queues are either simple queues or mirrored queues. 
  
  All queues have the following functionality:
  
  init_mem - setup all memory structures and pointers to structures.
  
  init_state - reset all internal state variables.
  
  get - get an element from the list.
  
  put - put an element to the list. Explicitly allows a push to 
  memory to occur.
  
  
  Mirrored queues also support updating the mirror:
  sync - synchronize the two images. internal state
  tells how much of queue to synchronize.
  
***********************************************************/

#include <sys/types.h>
#include <sys/errno.h>

#include "r4650.h"
#include "sys/PCI/linc.h"
#include "hippi_sw.h"

#include "rdbg.h"
#include "lincutil.h"

extern state_t	*State;



/*************************************************************
  source linc to roadrunner command queue 
*************************************************************/

sl2rr_state_t 	*SL2RR;
sl2rr_ack_t	*SL2RR_ACK;


void 
sl2rr_init_mem(void) {
    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    SL2RR = (sl2rr_state_t *)
	heap_malloc(sizeof(sl2rr_state_t), CACHED);
    dprintf(1, ("sl2rr st \t= 0x%x\n", SL2RR));

    
    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    SL2RR_ACK = (sl2rr_ack_t *)
	heap_malloc(sizeof(sl2rr_ack_t)*State->l2rr_len, CACHED);
    dprintf(1, ("sl2rr_ACK \t= 0x%x\n", SL2RR_ACK));

    SL2RR->base = State->sl2rr;
}

/* initialize sl2rr state */
void 
sl2rr_init_state(void) {
    int i;
    src_l2rr_t *ip;

    SL2RR->put = SL2RR->base;
    SL2RR->get = SL2RR->base;
    SL2RR->end = State->l2rr_len + SL2RR->base;

    SL2RR->total = 0;

    SL2RR->queued = 0;
    SL2RR->flags = SL2RR_VB;		/* init VB = 1 */

    /* init queue to invalid */
    i = ~SRRD_VB;
    for(ip = SL2RR->base; ip < SL2RR->end; ip++) {
#ifdef DEBUG
	ip->addr = (uint *)0xffffffff;
	ip->ifield = 0xffffffff;
	ip->pad = 0xffffffff;
#endif
	ip->op_len = i;
	i = ~i;
    }

#ifdef DEBUG
    for (i = 0; i < State->l2rr_len; i++) {
	SL2RR_ACK[i].dptr = (u_int *)0xffffffff;
    }
#endif
    
}


/***********************************************************************
sl2rr_put:
   checks for room, must set validity bit, insert wrap descriptor, 
   adds PFTCH, CHANNEL bits (31:26) to address.
   Checks that length is multiple of 1 KB or less than 1 KB (debug)
   
   returns 1 if all okay, 0 if push failed.
***********************************************************************/

int 
sl2rr_put(src_blk_t *blk) {
    u_int len;
    u_int *end_dp;
    int d2bs;
    u_int flags;

    src_blk_t lb = *blk;	/* local copy of blk so can fix-up flags, etc*/
    src_l2rr_t l2rr;
    sl2rr_ack_t *ackp;

    /* check that there's enough room for all of a block to be stuffed.
       1 l2rr for short burst (must not cross 64 KB boundary)
       3 l2rr for max length blk = 128 KB
    */
    if( (SL2RR->queued + 4) >= (State->l2rr_len-1)) { 
	/* enough room? - always fill to one less than size of queue*/
	dprintf(1, ("SL2RR QUEUE FULL!!!\n"));
	return(0);
    }

    ASSERT(((u_int)blk->dp & 3) == 0);	/* address is word aligned */
    ASSERT(((u_int)blk->len & 7) == 0); /* length is long word aligned  */

    dprintf(1, ("sl2rrput addr=0x%x, len=0x%x, NEOP=%d, NEOC=%d, dp=0x%x, tp=%d\n",
		SL2RR->put,
		blk->len, 
		(blk->flags & SBLK_NEOP) ? 1:0, 
		(blk->flags & SBLK_NEOC) ? 1:0,
		blk->dp, blk->tail_pad));
    
    if ( !(blk->flags & SBLK_BEGPC))
	SL2RR->flags &= ~SL2RR_PC;

    /* create l2rr */
    l2rr.ifield = lb.ifield;

    do {
	flags = lb.flags;

	/* check for short first burst */
	if (lb.fburst != 0) {	

	    len = lb.fburst;
	    end_dp = (u_int*)((u_int)lb.dp + len);
	    lb.fburst = 0;

	}
	else {
	    if ( (blk->flags & SBLK_REM)
		&& (lb.len > 1024))
		/* this is a block within a packet - to force flush of 
		 * Linc CPCI prefetch queue, use continuous prefetch
		 * on all but the last 1 KB - then use a prefetch of 1024.
		 * note that SBLK_REM will only be set if pkt is 1 KB multiple.
		 */
		len = lb.len - 1024;

	    else 
		len = lb.len;

	    end_dp = (u_int*)((u_int)lb.dp + len + lb.tail_pad);
	    if (end_dp >= State->data_M_endp)
		end_dp -= State->data_M_len;
	}

	ASSERT(((u_int)end_dp & 0x3) == 0);

	l2rr.op_len =  SRRD_LEN_MASK & len;
	if (len != lb.len)
	    flags |= SBLK_NEOC | SBLK_NEOP;
	else {			/* pick up NEOP/NEOC from original block */
	    flags &= ~SBLK_NEOC & ~SBLK_NEOP;
	    flags |= blk->flags & (SBLK_NEOC | SBLK_NEOP);
	}


	l2rr.op_len |= (flags & SBLK_NEOP) ? SRRD_MB : 0;
	l2rr.op_len |= (flags & SBLK_NEOC) ? SRRD_CC : 0;
	l2rr.op_len |= (flags & SBLK_SOC) ? 0 : SRRD_SI;
	if (len == 0) {
	    l2rr.op_len |= SRRD_DD;
	    l2rr.op_len &= ~SRRD_MB;
	    if (!(flags & SBLK_NEOC))
		l2rr.op_len &= ~SRRD_SI;
	}

	/* set perm conn if first time seen it */
	if ((flags & SBLK_BEGPC) && !(SL2RR->flags & SL2RR_PC)) {
	    l2rr.op_len |=  SRRD_PC;
	    SL2RR->flags |= SL2RR_PC;
	}

	/* add wrap-now flag if needed */
	l2rr.op_len |= (SL2RR->put == SL2RR->end - 1) ? SRRD_WN : 0;
	l2rr.op_len |= (SL2RR->flags & SRR2L_VB) ? SRRD_VB : 0;

	/* vary prefetch according to data length.
	 * see init_linc in common.c for doc.
	 */
	if (lb.len > 1024)
	    l2rr.addr = (u_int *)((u_int)K1_TO_PHYS(lb.dp) |
			       CPCI_ATTR_SDRAM | CPCI_ATTR_PFTCH_W(3));
	else if (lb.len == 1024)
	    l2rr.addr = (u_int *)((u_int)K1_TO_PHYS(lb.dp) |
			       CPCI_ATTR_SDRAM | CPCI_ATTR_PFTCH_W(2));
	else
	    l2rr.addr = (u_int *)((u_int)K1_TO_PHYS(lb.dp) |
			       CPCI_ATTR_SDRAM | CPCI_ATTR_PFTCH_W(1));

	/* GO! */
	dprintf(1, ("\t   addr=0x%x, len=0x%x, NEOP=%d, NEOC=%d\n",
		    lb.dp, len, flags & SBLK_NEOP, flags & SBLK_NEOC));

	trace (TOP_L2RR, (l2rr.op_len)>>24, SRRD_LEN_MASK & l2rr.op_len, l2rr.ifield, (u_int)l2rr.addr);
	ASSERT(((u_int)l2rr.addr & 0xffffff) < 0x03f0000);

	wgather((u_int *)&l2rr, (u_int *)SL2RR->put, sizeof(src_l2rr_t)); 

	/* setup info for ack and clean up blk struct */
        ackp = &SL2RR_ACK[SL2RR->put - SL2RR->base];

	ackp->dptr = end_dp;
	ackp->len = len;
	ackp->stack = lb.stack;
	ackp->flags = flags;
	ackp->tail_pad = lb.tail_pad;
	
	if (len != lb.len) {
	    /* didn't complete the block */
	    ackp->d2bs = 0;
	    lb.flags &= ~SBLK_BEGPC & ~SBLK_SOC;
	    lb.flags |= SBLK_NEOP | SBLK_NEOC;
	    lb.len -= len;
	    lb.dp += len/sizeof(u_int);
	    len = 0; /* make sure len != lb.len so we repeat loop */
	}
	else			/* completed blk */
	    ackp->d2bs = lb.num_d2bs;
	
	ASSERT(lb.len >= 0);

	/* clean up state for next round */
	SL2RR->flags =  (~SL2RR->flags  & SL2RR_VB)
	    | (SL2RR->flags & ~SL2RR_VB); /* toggle VB */

	if (++SL2RR->put == SL2RR->end) SL2RR->put = SL2RR->base;
	SL2RR->queued++;
	SL2RR->total++;

    } while (lb.len != len);	/* loop until blk completely transmitted */

    return(1);

}

/* sget_ack returns either the last unprocessed
 * ack, up to goal_l2rrp, or, if there are no unprocessed
 * acks, it returns the previous processed ack.
*/

sl2rr_ack_t *
sget_ack(src_l2rr_t *goal_l2rrp) {
    sl2rr_ack_t *getp;
    src_l2rr_t *goal = goal_l2rrp;
    int process = 0;

    ASSERT ( (goal_l2rrp < SL2RR->end) && (goal_l2rrp >= SL2RR->base));

    /* goal is incremented to account for queue size being len-1
     */
    goal++;
    if (goal >= SL2RR->end)
        goal = SL2RR->base;
    
    if ((SL2RR->put != SL2RR->get)
	&& (goal != SL2RR->get)) { /* not empty - return current get */
	getp = &SL2RR_ACK[SL2RR->get - SL2RR->base];

	/* if get = current goal, then
	   you've processed all available acks,
	   so don't increment get ptr 
	*/
	SL2RR->get++;
	if (SL2RR->get >= SL2RR->end)
	  SL2RR->get = SL2RR->base;
	SL2RR->queued--;
    }
    else {			/* empty - return previous get */
	if (SL2RR->get == SL2RR->base) 
	    getp = &SL2RR_ACK[State->l2rr_len-1];
	else
	    getp = &SL2RR_ACK[(SL2RR->get-1) - SL2RR->base];
    }

    dprintf(1, ("sget_ack: goal_l2rrp=0x%x, SL2RR->get=0x%x, ackp=0x%x, &l2rr=0x%x\n",
		goal_l2rrp, SL2RR->get, getp, State->sl2rr + (getp - SL2RR_ACK)));

    return(getp);
}


/*************************************************************
  source roadrunner to linc command queue  (srr2l)
*************************************************************/

srr2l_state_t	*SRR2L = 0;

void 
srr2l_init_mem(void) {
    
    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    SRR2L = (srr2l_state_t *)
	    heap_malloc(sizeof(srr2l_state_t), CACHED);
    dprintf(1, ("srr2l st\t= 0x%x\n", SRR2L));
	
    SRR2L->base = State->srr2l;
}

/* initialize srr2l state */
void 
srr2l_init_state(void) {
    volatile src_rr2l_t *ip;
    src_rr2l_t i;
    int j;

    SRR2L->get = SRR2L->base;
    SRR2L->end = SRR2L->base + State->rr2l_len;
    SRR2L->flags = SRR2L_VB;		/* look for VB = 1 */

    /* init queue to invalid */
    i = ~SRRS_VB;
    j = 0;
    for(ip = SRR2L->base; ip < SRR2L->end; ip++, j++) {
	*ip = i;
	i = ~i;
    }
}


/* returns null pointer if no valid srr2l's.
   checks validity bit. does wrap.
*/
src_rr2l_t 
srr2l_get(void) {
    src_rr2l_t rr2l;

    rr2l = *SRR2L->get;
    if ( (rr2l & SRRS_VB) && (SRR2L->flags & SRR2L_VB)
	 || (!(rr2l & SRRS_VB) && !(SRR2L->flags & SRR2L_VB))) {
	dprintf(1, ("srr2l_get: addr = 0x%x, desc = 0x%x\n", 
		    SRR2L->get, rr2l));
	SRR2L->flags =  (~SRR2L->flags & SRR2L_VB) 
	  | (SRR2L->flags & ~SRR2L_VB);	/* toggle VB */

	SRR2L->get++;
	if (SRR2L->get == SRR2L->end) 
	    SRR2L->get = SRR2L->base;
	return(rr2l);
    }
    
    else {
	return (NULL);
    }
}






