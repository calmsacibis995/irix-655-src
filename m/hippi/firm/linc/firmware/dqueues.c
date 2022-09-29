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
 * dqueues.c - destination queue control commands
 *
 * $Revision: 1.9 $
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
  c2b - control to board - used to pass descriptors to dest linc
  
  Linc/Roadrunner Interface
  ----------------------------
  drr2l - dest roadrunner to linc descriptor queue
  

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
  
  There are also several special functions, depending on the queue:
  
  
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
  destination roadrunner to linc command queue  (drr2l)
*************************************************************/

drr2l_state_t	*DRR2L = 0;

void 
drr2l_init_mem(void) {
    
    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    DRR2L = (drr2l_state_t *)
	    heap_malloc(sizeof(drr2l_state_t), CACHED);
    dprintf(1, ("drr2l st\t= 0x%x\n", DRR2L));
    
    DRR2L->base = State->drr2l;
}

/* initialize drr2l state */
void 
drr2l_init_state(void) {
    volatile dst_rr2l_t *ip;
    u_int i;

    DRR2L->get = DRR2L->base;
    DRR2L->end = DRR2L->base + State->rr2l_len;
    DRR2L->st = DRRD_VB;		/* look for VB = 1 */

    /* init queue to invalid */
    i = ~DRRD_VB;
    for(ip = DRR2L->base; ip < DRR2L->end; ip++) {
#ifdef DEBUG
	ip->addr = (u_int *)0xffffffff;
	ip->len = 0xffffffff;
#endif
	ip->flag = i;
	i = ~i;
    }
}


/* returns null pointer if no valid drr2l's.
   checks validity bit. does wrap.
*/
volatile dst_rr2l_t *
drr2l_get(void) {
    volatile dst_rr2l_t *rr2l;

    if ( ((u_int)(DRR2L->get->flag) & DRRD_VB) == (DRR2L->st & DRRD_VB)) {
 	dprintf(1, ("drr2l_get: flag = 0x%x, addr = 0x%x, len = 0x%x\n", \
		    DRR2L->get->flag, DRR2L->get->addr, DRR2L->get->len));
	DRR2L->st =  (~DRR2L->st & DRRD_VB)
	               | (DRR2L->st & ~DRRD_VB);	/* toggle VB */
	rr2l = DRR2L->get++;

	if (DRR2L->get == DRR2L->end) {
	    DRR2L->get = DRR2L->base;
	}
	return(rr2l); 
    }
    
    else {
	return (NULL);
    }

}

/* put previous rr2l back at top of queue - used when an error
 * occured so can't process this one.
 */
void
drr2l_put_one_back(void) {
	DRR2L->st =  (~DRR2L->st & DRRD_VB)
	               | (DRR2L->st & ~DRRD_VB);	/* toggle VB */

	if (DRR2L->get == DRR2L->base) {
	    DRR2L->get = DRR2L->end - 1;
	}
	else 
	    DRR2L->get--;

}

/* Should modify this to only write when 1/2 of queue is consumed */


void 
update_dl2rr_datap(volatile u_int *dp, u_int len) {
  /* len in bytes */
  volatile u_int *ip;
  ip = dp + (len>>2);
  if (ip >= State->data_M_endp)
    ip -= State->data_M_len;

  State->dl2rr->data_ring_consumer = (u_int)K1_TO_PHYS((u_int)ip) |
      DST_RR_DATA_ATTR;

}

/* Should modify this to only write when 1/4 of queue is consumed */

void
update_dl2rr_descp(volatile dst_rr2l_t *rr2l) {
volatile dst_rr2l_t *ip;
  ip = rr2l + 1;
  if (ip >= State->drr2l + State->rr2l_len)
    ip -= State->rr2l_len;

  State->dl2rr->desc_ring_consumer = (u_int)K1_TO_PHYS((u_int)ip) 
      | DST_RR2L_DESC_ATTR;


}

