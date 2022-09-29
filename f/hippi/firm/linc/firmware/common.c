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
 * common.c - tools used by source and destination firmware 
 *
 * $Revision: 1.65 $
 *
 */

#include <sys/types.h>
#include <sys/errno.h>

#include "r4650.h"
#include "sys/PCI/linc.h"
#include "hippi_sw.h"
#include "rdbg.h"
#include "lincutil.h"
#include "bypass.h"


#ifdef HIPPI_SRC_FW

#include "rr_src_fw.h"
extern srr2l_state_t	*SRR2L;
extern sl2rr_state_t 	*SL2RR;
extern sl2rr_ack_t	*SL2RR_ACK;
extern src_blk_t blk;

#else /* HIPPI_DST_FW */

#include "rr_dst_fw.h"
extern drr2l_state_t	*DRR2L;
extern wire_blk_t 	wblk;	
extern ack_blk_t 	ablk;	
extern mbuf_state_t 	MBUF;
extern fpbuf_state_t	FPBUF;

#endif /* HIPPI_SRC_FW */

extern state_t   	*State;

extern d2b_state_t 	*D2B;
extern b2h_state_t 	*B2H;

extern trace_t *Tracep;
extern trace_t *Trace_base;

#ifdef USE_TIMERS
/* Variables needed for light weight timers. */
hip_timer_t hip_timers[NUM_TIMERS];
#endif

/************************************************************
  chksum
************************************************************/

unsigned int 
chksum(volatile unsigned int *a, int len) {

    /* a points to the beginning of the FP field of a packet. 
     *  len is the length in
     * words for the total packet.
     */
    unsigned int checksum = 0;
    u_int phdr[3];
    int i = 0;

    a += 10;			/* point to start of IP header */
    if (a >= State->data_M_endp)
      a -= State->data_M_len;

    len -= 10;


    /* get tcp segment length  = IP len - IP hdr len */
    /* 24-2 xlates to bytes */
    phdr[2] = (*a & 0xffff) - ( (*a & 0xf000000) >> 24-2); 

    a += 2;
    if (a >= State->data_M_endp)
      a -= State->data_M_len;

    /* get proto */
    phdr[2] |= (*a & 0x00ff0000);

    a++;
    if (a >= State->data_M_endp)
      a -= State->data_M_len;

    /* source IP addresses */
    phdr[0] = *a++;
    if (a >= State->data_M_endp)
      a -= State->data_M_len;

    /* dest IP addr */
    phdr[1] = *a++;
    if (a >= State->data_M_endp)
      a -= State->data_M_len;

    while ( i < len) {
    
	checksum += (*a & 0xffff) + ((*a >>16) & 0xffff);
	/*	printf("\tchksm: 0x%x + 0x%x = 0x%x\n", 
	       (a[i] & 0xffff),
	       ((a[i]>>16) & 0xffff),
	       (a[i] & 0xffff) + ((a[i]>>16) & 0xffff));
	printf("\tchksm: checksum = 0x%x\n", checksum);
	*/
	checksum = (checksum & 0xffff) + (checksum>>16);
	i++;
	a++;
	if (a >= State->data_M_endp)
	  a = State->data_M;
    }
    /*    printf("\tchksum cleanup: 0x%x + 0x%x = 0x%x\n",
	   checksum & 0xffff, checksum>>16, 
	   (checksum & 0xffff) + (checksum>>16));*/

    checksum = (checksum & 0xffff) + (checksum>>16);

    /*    printf("\tchksum cleanup: 0x%x + 0x%x = 0x%x\n",
	   checksum & 0xffff, checksum>>16, 
	   (checksum & 0xffff) + (checksum>>16));*/

    checksum = (checksum & 0xffff) + (checksum>>16);
    if (checksum != 0)
      breakpoint();
    return(checksum);
}


/*************************************************************
  board to host queue commands (B2H) 
  *************************************************************/

void 
b2h_init_mem(void) {

    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    B2H = (b2h_state_t *)
	heap_malloc(sizeof(b2h_state_t), CACHED);
    dprintf(1, ("b2h st \t\t= 0x%x\n", B2H));
    B2H->basep = State->b2h;
}


/************************************************************
  b2h_init_state
************************************************************/

void 
b2h_init_state(u_int hostp_lo, u_int hostp_hi, int len) {

    B2H->seqnum = 1;
    B2H->queued = 0;

    B2H->put = B2H->basep;
    B2H->endp = B2H->basep + B2H_SIZE;

    B2H->hostp_lo = hostp_lo;
    B2H->hostp_hi = hostp_hi;
    B2H->host_off = 0;
    B2H->host_end = len * sizeof(hip_b2h_t);

}

/************************************************************
  b2h_queue
************************************************************/

int
b2h_queue( hip_b2h_t *b2h) {
    /* return 1 if queue is fairly full - must leave slop because source
     * can continue to stuff b2h's - src fw must finish acking all
     * d2b's for a single rr2l ack descriptor.
     */

    hip_b2h_t *put = B2H->put + B2H->queued;

    if (put >= B2H->endp)
	put -= B2H_SIZE;

    b2h->b2h_sn = B2H->seqnum++;
    *put = *b2h;

    B2H->queued++;
    if (B2H->queued >= B2H_SIZE/2)
	return 1;
    return 0;
}

/************************************************************
  intr_host
************************************************************/

void
intr_host(void) {

    /* add a 1 usec delay to ensure min deassertion time */
    wait_usec(1);

    trace(TOP_MISC,T_MISC_INTR, 0, 0, 0);

    LINC_WRITEREG( LINC_HISR, 
		   LINC_HISR_TO_HOST_INT_W(1));
	 
#ifdef RINGBUS_WAR
     (void)LINC_READREG( LINC_SDRAM_ADDR );
#endif

    State->flags |= FLAG_BLOCK_INTR;
    State->flags &= ~FLAG_NEED_HOST_INTR;
    timer_set(&State->b2h_timer);

}

/************************************************************
  b2h_push
************************************************************/

void
b2h_push(gen_host_t *hostp) {
    dma_cmd_t dcmd;
    int max_local;
    int max_host;
    int len;

    len = B2H->queued * sizeof(hip_b2h_t);

    while (len > 0) {		

	max_local = (B2H->endp - B2H->put) * sizeof(hip_b2h_t);
	max_host = (B2H->host_end - B2H->host_off);

	dcmd.len = MIN(max_local, len);
	dcmd.len = MIN(max_host, dcmd.len);
	dcmd.flags = LINC_DTC_D64 | LINC_DTC_TO_PARENT;
	dcmd.host_addr_lo = B2H->hostp_lo + B2H->host_off;
	dcmd.brd_addr = (u_int)K1_TO_PHYS(B2H->put);
	 
	/* flush data from cache */
	wbinval_dcache((u_int*)B2H->put, dcmd.len);

#ifdef HIPPI_SRC_FW

	/* don't need barrier for source */
	dcmd.host_addr_hi = B2H->hostp_hi;

	/* dma1 is only used by b2h_push */
	ASSERT(LINC_DCSR_VAL_DESC_R(LINC_READREG( 
						 LINC_DMA_CONTROL_STATUS_1)) == 0);
	/* GO! - dma1 doesn't flush dma0's prefetch queue*/

	trace(TOP_DMA_B2H, 0,
	      dcmd.brd_addr, dcmd.host_addr_lo, dcmd.flags | dcmd.len);

	dma_push_cmd1(&dcmd);	

	while( !dma1_done())
	    continue;

#else  /* HIPPI_DST_FW */

	dcmd.host_addr_hi = B2H->hostp_hi | PPCIHI_ATTR_BARRIER;

	/* routine produces a max of 3 dma commands.
	 * since source side has exclusive use of DMA1 for b2h's,
	 * only need to make sure enough room at destination
	 */
	while(LINC_DCSR_VAL_DESC_R(LINC_READREG( 
						LINC_DMA_CONTROL_STATUS_0)) > 3)
	    continue;

	trace(TOP_DMA_B2H, 0,
	      dcmd.brd_addr, dcmd.host_addr_lo, dcmd.flags | dcmd.len);

	/* GO! - dma0 ensures coherency*/	
	dma_push_cmd0(&dcmd);	

	while( !dma0_done())
	    continue;

#endif

	B2H->put += dcmd.len/sizeof(hip_b2h_t);
	if (B2H->put >= B2H->endp)
	    B2H->put = B2H->basep;

	B2H->host_off += dcmd.len;
	if (B2H->host_off >= B2H->host_end)
	    B2H->host_off = 0;

	len -= dcmd.len;
    }

    B2H->queued = 0;
    State->flags |= FLAG_NEED_HOST_INTR;
}


/*************************************************************
  data to board command queue (D2B) - used primarily to send 
  buffers to the source engine 
  *************************************************************/

void 
d2b_init_mem(void) {
    
    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    D2B = (d2b_state_t *)heap_malloc(sizeof(d2b_state_t), CACHED);
    dprintf(1, ("d2b st\t= 0x%x\n", D2B));
    D2B->basep = State->d2b;
}

    
/*************************************************************
  initialize d2b state 
  *************************************************************/

void 
d2b_init_state(u_int hostp_lo, u_int hostp_hi, int len) {
    int i;
    
    D2B->hostp_lo = hostp_lo;

    /* clear attributes set by driver - set our own */
    D2B->hostp_hi = (hostp_hi & 0xf0ffffff);

    D2B->host_off = 0;
    D2B->host_end = len * sizeof(gen_d2b_t);
    
    D2B->endp = D2B->basep + LOCAL_D2B_LEN;
    D2B->get = D2B->basep;
    
    /* sets start of invalid sequence */
#ifdef HIPPI_SRC_FW
    D2B->get->hd.flags = HIP_D2B_BAD; 
#else /* HIPPI_DST_FW */
    D2B->get->c2b_op = HIP_C2B_EMPTY; 
#endif

    D2B->end_valid = D2B->get;
    D2B->st_in_cache = D2B->basep;
    wbinval_dcache(D2B->st_in_cache, sizeof(hip_d2b_t));
		 
    
    /* scrub rest of stucture */
    D2B->flags = 0;
}

/*************************************************************
  synchronously update local copy of d2b (data to board)
  *************************************************************/

int 
d2b_sync_update(int num) {
    int i = 0;
    int len_eocl;		/* length to end of cacheline */
    int len;
    u_int localp;

    len_eocl = HOST_LINESIZE - ((u_int)D2B->get & (HOST_LINESIZE-1));
    localp = (u_int)K1_TO_PHYS(D2B->get) & ~(HOST_LINESIZE-1);
    if (len_eocl > 64) { /* transfer full cache-line */
	len = 128;
    }
    else {			/* transfer half cache-line */
	len = 64;
	localp |= HOST_LINESIZE>>1; /* upper half of line */
    }

    D2B->end_valid = (gen_d2b_t*)PHYS_TO_K0(localp + len);

    /* invalidate anything in cache */
    if (D2B->get > D2B->st_in_cache)
	inval_dcache(D2B->st_in_cache, 
		     ((u_int)D2B->get - (u_int)D2B->st_in_cache));
    else { 
	inval_dcache(D2B->st_in_cache, 
		     ((u_int)D2B->endp - (u_int)D2B->st_in_cache));
	inval_dcache(D2B->basep, 
		     ((u_int)D2B->get - (u_int)D2B->basep));
    }

/*    trace(TOP_IDMA, T_DMA_D2B_DESC, localp, D2B->hostp_lo + D2B->host_off, len);*/

    idma_read(D2B->hostp_hi, 
	      D2B->hostp_lo + D2B->host_off,
	      localp,
	      len,
	      0);

    D2B->st_in_cache = D2B->get;

    while( !idma_done()) 	/* wait for idle */
	assert(i++ < 1000000000);

}

#ifdef USE_MAILBOX

/* This returns a 1 if there is something on the d2b queue to be processed */
int
d2b_check(void) {

#ifdef HIPPI_SRC_FW
    if ((D2B->get == D2B->end_valid) || (D2B->get->hd.flags == HIP_D2B_BAD)) {
	return(0);
    }
#else  /* HIPPI_DST_FW */
    if ((D2B->get == D2B->end_valid) || 
	((D2B->get->c2b_op & HIP_C2B_OPMASK) == HIP_C2B_EMPTY)) {
	return(0);
    }
#endif
    else
	return 1;

}

#endif /* USE_MAILBOX */


/*************************************************************
  get d2b's from host
  *************************************************************/

gen_d2b_t*
d2b_get(void) {
    gen_d2b_t *tmp;

#ifdef HIPPI_SRC_FW
    if ((D2B->get == D2B->end_valid) 
	|| (D2B->get->hd.flags == HIP_D2B_BAD)) {
	return(0);
    }
#else  /* HIPPI_DST_FW */
    if ((D2B->get == D2B->end_valid) 
	|| ((D2B->get->c2b_op & HIP_C2B_OPMASK) == HIP_C2B_EMPTY)) {
	return(0);
    }
#endif
	
    tmp = D2B->get;
    D2B->get++;
    if (D2B->get == D2B->endp) {
	D2B->get = D2B->basep;
	D2B->end_valid = D2B->basep;
    }

    if ((u_int)D2B->get % (HOST_LINESIZE>>1) == 0) {
	D2B->host_off += HOST_LINESIZE>>1;
	if (D2B->host_off >= D2B->host_end)
	    D2B->host_off = 0;
    }
    return (tmp);

}

/************************************************************
  go_to_sleep
************************************************************/

void
go_to_sleep(void) {
    int i = 0;

    /* tell host asleep */
    idma_write(State->sleep_addr_hi, 
	       State->sleep_addr_lo, 
	       HIPFW_FLAG_SLEEP,
	       D2B->host_off,
	       PPCIHI_ATTR_BARRIER);

    State->flags |= FLAG_NEED_HOST_INTR;
    State->flags |= FLAG_ASLEEP;

    while( !idma_done()) 	/* wait for idle */
	assert(i++ < 10000000);
}


/************************************************************
  check_rr_alive
************************************************************/

void
check_rr_alive(void) {
    int i = 100;

    for (i = 100; (State->rr_mem->rr_gca.cmd.mb.mailbox[31] == 0) &&  (i > 0); ) 
        i--;

    if (i == 0) {
        dprintf(1, ("roadrunner is DEAD\n"));
	State->flags &= ~FLAG_RR_UP;
	die(CDIE_RR_ERR, CDIE_AUX_RR_FW_DEAD);
	
    }
    else
	State->flags |= FLAG_RR_UP;
    
    State->rr_mem->rr_gca.cmd.mb.mailbox[31] = 0;
}

/************************************************************
  store_bp_dma_status
************************************************************/

void 
store_bp_dma_status(enum dma_status_t type, 
		    int num_bufx, 
		    int base_bufx, 
		    int job) {
    u_int status;

    switch(type) 
	{
	  case DMA_ENABLE_SRC_DATA:
	    status =  0x1 << HIPPIBP_DMA_ACTIVE_SHIFT;
	    status |= HIPPIBP_DMA_CLIENT_SFM << HIPPIBP_DMA_CLIENT_SHIFT;
	    if (num_bufx == 2) 
		status |= 0x1 << HIPPIBP_DMA_2PG_SHIFT;
	    status |= job << HIPPIBP_DMA_JOB_SHIFT;
	    status |= base_bufx << HIPPIBP_DMA_PGX_SHIFT;
	    break;

	  case DMA_ENABLE_DST_DATA:
	    status =  0x1 << HIPPIBP_DMA_ACTIVE_SHIFT;
	    status |= HIPPIBP_DMA_CLIENT_DFM << HIPPIBP_DMA_CLIENT_SHIFT;
	    if (num_bufx == 2) 
		status |= 0x1 << HIPPIBP_DMA_2PG_SHIFT;
	    status |= job << HIPPIBP_DMA_JOB_SHIFT;
	    status |= base_bufx << HIPPIBP_DMA_PGX_SHIFT;
	    break;

	  case DMA_ENABLE_DST_DESC:
	    status =  0x1 << HIPPIBP_DMA_ACTIVE_SHIFT;
	    status |= HIPPIBP_DMA_CLIENT_PORTMAP << HIPPIBP_DMA_CLIENT_SHIFT;
	    status |= job << HIPPIBP_DMA_JOB_SHIFT;
	    break;
      
	  case DMA_OFF:
	    status = 0;
	    break;
	}
    *(State->dma_statusp) = status;

}

/************************************************************
  set_leds
************************************************************/

void 
set_leds(u_int timer_ticks) {
    u_int opp_flags;
    int goodness;
    int sig_det;
    volatile char *bbpal_misc = (volatile char*)PHYS_TO_K1(LINC_BBPAL_MISC);

    /* Check everyone is alive */

    if(State->flags & FLAG_GOT_INIT) {

#ifdef HIPPI_SRC_FW
	sig_det = (*bbpal_misc & LINC_BB_MISC_SIG_DET);
	/* set pkt LED */
	if(timer_ticks % LED_PKT_MIN_ON == 0) {

	    if(State->leds & LED_SRC_PKT) /* pkt led is on - turn it off */
	        State->leds &= ~LED_SRC_PKT;

	    else if(State->old_byte_cnt != State->stats->sf.hip_s.numbytes_lo) {
	        State->old_byte_cnt = State->stats->sf.hip_s.numbytes_lo;
		State->leds |= LED_SRC_PKT;
	    }
	}

	/* GLINK is up if:
	 * light is on (LINC_BB_MISC_SIG_DET)
	 * destination has sent status with:
	 *    flag synched & link_ready & 0H8_synched
	 */
	opp_flags = (u_int)State->opposite_st->flags;
	goodness = sig_det
	    && (opp_flags & OPP_FLAG_SYNC)
	    && (opp_flags &  OPP_FLAG_LNK_READY)
	    && (opp_flags & OPP_FLAG_OH8SYNC);
	
	if (goodness) { /*link is up */
	    State->flags |= FLAG_GLINK_UP;
	    State->leds |= LED_LINK_OK;
	    State->cur_link_errcnt = 0;
	}
	else {			/* not up */
	    if (State->flags & FLAG_GLINK_UP) { /* just turned off */
		State->stats->sf.hip_s.glink_err++;
	    }
	    State->flags &= ~FLAG_GLINK_UP;
	    State->leds &= ~LED_LINK_OK;
	    if (sig_det) {
		/* drop through and reset the first time we see the glink
		 * out of sync, reset again only if the first didn't fix it
		 * within the GLINK_RESET_THRESHOLD
		 */
		if ((State->cur_link_errcnt++ == 0) | 
		    (State->cur_link_errcnt > GLINK_RESET_THRESHOLD)) {
		    State->cur_link_errcnt = 0;
		    State->stats->sf.hip_s.glink_resets++;

		    *bbpal_misc = LINC_BB_MISC_GRESET;
		    wait_usec(1);
		    *bbpal_misc = 0;
		    wait_usec(1);

		    /* enable glink and loopback*/
		    if (State->flags & FLAG_LOOPBACK) 
		        *bbpal_misc = LINC_BB_MISC_TDAV | LINC_BB_MISC_LB;
		    else
		        *bbpal_misc = LINC_BB_MISC_TDAV;
		}
	    }
	}

#else  /* HIPPI_DST_FW */

	/* set pkt LED */
	if(timer_ticks % LED_PKT_MIN_ON == 0) {

	    if(State->leds & LED_DST_PKT) /* pkt led is on - turn it off */
	        State->leds &= ~LED_DST_PKT;
	
	    else if(State->old_byte_cnt != State->stats->df.hip_s.numbytes_lo) {
	        State->old_byte_cnt = State->stats->df.hip_s.numbytes_lo;
		State->leds |= LED_DST_PKT;
	    }
	}

	if (State->flags & FLAG_LOOPBACK) {
	    /* fast alive flicker if loopback */
	    if((timer_ticks % LED_ALIVE_FAST_TICK) == 0)
		State->leds ^= LED_ALIVE;
	}
	else
	    /* slow alive flicker if not in loopback */
	    if((timer_ticks % LED_ALIVE_SLOW_TICK) == 0)
		State->leds ^= LED_ALIVE;
#endif

	State->leds &= ~(LED_ERROR | LED_ERR_CODE);
	
    }

    else {			/* board has not been initialized */
        if(timer_ticks%LED_NO_INIT_TICK == 0) /* rapid blink */
	    State->leds ^= 0xa;
    }
	    
    LINC_WRITEREG(LINC_LED, ~State->leds);
}


/************************************************************
  init_linc
************************************************************/

void
init_linc() {
    u_int i;
    
    /* Reset byte-bus errors & Speed up PROM accesses  */
    LINC_WRITEREG( LINC_BBCSR,
		  LINC_BBCSR_EN_ERR | /* RW1C */    
		  LINC_BBCSR_RST_ERR | /* RW1C */	
		  LINC_BBCSR_PROM_SZ_ERR | /* RW1C */	
		  LINC_BBCSR_PAR_ERR | /* RW1C */	
		  LINC_BBCSR_WR_TO | /* RW1C */
		  LINC_BBCSR_PULS_WID_W( 0x0f ) | 
		  LINC_BBCSR_SELF_TIMED |		
		  LINC_BBCSR_A_TO_CS_W( 0 ) | 	
		  LINC_BBCSR_CS_TO_EN_W( 0 ) | 	
		  LINC_BBCSR_EN_WID_W( 0x04 ) | 	
		  LINC_BBCSR_CS_TO_A_W( 0 ) |
		  LINC_BBCSR_BBUS_EN);

    /* Set up mailboxes. */
    LINC_WRITEREG( LINC_MAILBOX_BASE_ADDRESS, MAILBOXES );

    /* Initialize IDMA. */
    LINC_WRITEREG( LINC_INDIRECT_DMA_CSR,
		  LINC_ICSR_FLUSH | LINC_ICSR_ERROR );
	
    /* Initialize DMA. */
    LINC_WRITEREG( LINC_DMA_CONTROL_STATUS_0,
		  LINC_DCSR_RESET );
    LINC_WRITEREG( LINC_DMA_CONTROL_STATUS_0,
		  LINC_DCSR_DONE_INT_MASK |
		  LINC_DCSR_DONE_INT | /* RW1C */
		  LINC_DCSR_ERROR | /* RW1C */
		  LINC_DCSR_EN_DMA );

    LINC_WRITEREG( LINC_DMA_CONTROL_STATUS_1,
		  LINC_DCSR_RESET );
    LINC_WRITEREG( LINC_DMA_CONTROL_STATUS_1,
		  LINC_DCSR_DONE_INT_MASK |
		  LINC_DCSR_DONE_INT | /* RW1C */
		  LINC_DCSR_ERROR | /* RW1C */
		  LINC_DCSR_EN_DMA );

    /*init PPCI interface */
    LINC_WRITEREG(LINC_PCHDR,
		  LINC_PCHDR_COUNT_W(0x1f));
	
    /* Clear BOOTING bit & Initialize CPCI */
    
    i = LINC_READREG(LINC_LCSR) & ~LINC_LCSR_BOOTING;

    LINC_WRITEREG( LINC_LCSR, i | LINC_LCSR_RESET_CPCI);
#ifdef RINGBUS_WAR
     (void)LINC_READREG( LINC_SDRAM_ADDR );
#endif


    wait_usec(1);

    LINC_WRITEREG( LINC_LCSR, i);
#ifdef RINGBUS_WAR
     (void)LINC_READREG( LINC_SDRAM_ADDR );
#endif


    LINC_WRITEREG( LINC_CHILD_PCI_CSR,
		  LINC_CCSR_PAR_CHK_EN | LINC_CCSR_PAR_RESP_EN |
		  LINC_CCSR_SERR_EN |
		  LINC_CCSR_MEM_SPACE_EN | LINC_CCSR_MASTER_EN |
		  LINC_CCSR_OPT_GRANT |
		  LINC_CCSR_NO_WRT_FLUSH | LINC_CCSR_ARB_EN_W(3) );
    /* Source Prefetch:
     * prefetching is done for data and descriptors:
     *
     *   data = 1024 KB+ to (SRC_BLK_SIZE - 1 KB) (i.e. one hippi burst)
     * 		use continuous mode - this will always prefetch
     *		speculatively 128 bytes (32 w) in advance with a persistent buf.
     *
     *   data = 256 - 1 KB use prefetch = 512 B (128 words). Must evenly divide
     *		into 1024 so that no prefetch is left at end of 1024 bytes.
     *		This allows multi-block packets to be consecutive in memory without
     *		persistent read-ahead getting in the way (Use persistent prefetch=11
     *		for all block except last 256 words. Slice that off as a seperate
     *		l2rr descriptor with pretch=10 such that nothing is left in the
     *		prefetch after 256 words.
     *
     *	 data = < 256 B use prefetch = 128 (32 words).
     *
     *   desc = prefetch 64 bytes (16 words) (4 descriptors)
     */

    /* Destination Prefetch just for l2rr descriptor:
     * prefetching = 2 words - exact length of 4640 get pointers.
     */

#ifdef HIPPI_SRC_FW
    LINC_WRITEREG( LINC_CPCI_PREFETCH_CSR,
		  LINC_CPCSR_PF_LTH2_W(128) |
		  LINC_CPCSR_PF_LTH1_W(32) |
		  LINC_CPCSR_PF_LTH0_W(16) );
#else /* HIPPI_DST_FW */
    LINC_WRITEREG( LINC_CPCI_PREFETCH_CSR,
		  LINC_CPCSR_PF_LTH2_W(128) |
		  LINC_CPCSR_PF_LTH1_W(32) |
		  LINC_CPCSR_PF_LTH0_W(2) );
#endif

}


/************************************************************
  heap_malloc
************************************************************/

caddr_t 
heap_malloc(int size, heap_malloc_type type) {

  State->heap += size;
  ASSERT(K1_TO_PHYS(State->heap) < SDRAM_END);

  if (type == CACHED) 
    return ((caddr_t)K1_TO_K0 (State->heap) - size);
  else
    return ((caddr_t)State->heap - size);
}


/************************************************************
  init_mem -  Init memory pointers, but don't set any State->
************************************************************/

void 
init_mem(void) {
    u_int *ip;

    /* make heap uncacheable by default */
    State->heap = ALIGN((caddr_t) K0_TO_K1(FIRM_HEAPSTART), DCACHE_LINESIZE);
    dprintf(1, ("heap \t\t= 0x%x\n", State->heap));

    wbinval_dcache((u_int*)K1_TO_K0(State->heap), 
		   SDRAM_END - (u_int)K1_TO_PHYS(State->heap));

    /* init good parity to rest of memory */
    for (ip=(u_int*)K1_TO_K0(State->heap); 
	 ip < (u_int *)PHYS_TO_K0(SDRAM_END); ip++)
        *ip = 0xdeadbeef;

    wbinval_dcache((u_int*)K1_TO_K0(State->heap), 
		   SDRAM_END - (u_int)K1_TO_PHYS(State->heap));

    /* Driver has HCMD, stats, bpconfig hard coded!!
       don't change this order without synching with the driver.
    */

    State->hcmd = (hip_hc_t *)PHYS_TO_K1(HCMD_BASE);
    dprintf(1, ("hcmd \t\t= 0x%x\n", State->hcmd));
    State->stats = (hippi_stats_t *)PHYS_TO_K1(STATS_BASE);
    dprintf(1, ("stats \t\t= 0x%x\n", State->stats))
    State->bpconfig = (hip_bp_fw_config_t *)PHYS_TO_K1(BPCONFIG_BASE);
    dprintf(1, ("bpconfig \t= 0x%x\n", State->bpconfig));

    State->mb = (volatile uint64_t *)PHYS_TO_K1(MAILBOXES);

    /* keep this up front - must be same for both source and dest */

    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    State->opposite_st = (volatile opposite_t *)
	heap_malloc(sizeof(opposite_t), UNCACHED);
    dprintf(1, ("opposite_st \t= 0x%x\n", State->opposite_st));

    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    State->hostp = (gen_host_t *)
	heap_malloc(sizeof(gen_host_t), CACHED);
    dprintf(1, ("hostp \t\t= 0x%x\n", State->hostp));

    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    State->wirep = (gen_wire_t *)
	heap_malloc(sizeof(gen_wire_t), CACHED);
    dprintf(1, ("wirep \t\t= 0x%x\n", State->wirep));

    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    State->local_st = (opposite_t *)
	heap_malloc(sizeof(opposite_t), CACHED);
    dprintf(1, ("local_st \t= 0x%x\n", State->local_st));

    State->heap = ALIGN( State->heap, DCACHE_LINESIZE);
    State->bpstats = (hippibp_stats_t *)
	heap_malloc(sizeof(hippibp_stats_t), CACHED);
    dprintf(1, ("bpstats \t= 0x%x\n", State->bpstats));

    State->heap = ALIGN( State->heap, DCACHE_LINESIZE);
    State->job = (bp_job_state_t *)
	heap_malloc(sizeof(bp_job_state_t)*BP_MAX_JOBS, CACHED);
    dprintf(1, ("job \t\t= 0x%x\n", State->job));

#ifdef BRIDGE_B_WAR

    /* this is required for the flush of the prefetch queue */
    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    State->zero = (u_int) heap_malloc(sizeof(u_int), UNCACHED);
    dprintf(1, ("State->zero \t= 0x%x\n", State->zero));

    *(u_int*)State->zero = 0;
    *(u_int*)(State->zero + 4) = 0;
    
    State->zero = (u_int)K1_TO_PHYS(State->zero);

#endif

#ifdef HIPPI_SRC_FW		
    State->sblk = &blk;

    State->heap = ALIGN( State->heap, DCACHE_LINESIZE);
    State->l2rr_len = SRC_L2RR_SIZE-1;
    State->sl2rr = (src_l2rr_t *)
	heap_malloc(sizeof(src_l2rr_t)*State->l2rr_len, UNCACHED);
    dprintf(1, ("sl2rr \t\t= 0x%x\n", State->sl2rr));

    /* 64 KB alignment */
    assert( ((u_int)State->sl2rr & ~0xffff) 
	   == (((u_int)State->sl2rr+State->l2rr_len) & ~0xffff));


    /* must have odd length */
    assert(State->l2rr_len & 0x1);

    State->heap = ALIGN( State->heap, DCACHE_LINESIZE);
    State->rr2l_len = SRC_RR2L_SIZE-1;

    State->srr2l = (src_rr2l_t *)
	heap_malloc(sizeof(src_rr2l_t)*State->rr2l_len, UNCACHED);
    dprintf(1, ("srr2l \t\t= 0x%x\n", State->srr2l));

    assert( ((u_int)State->srr2l & ~0xffff) == 
	   ( ((u_int)State->srr2l+State->rr2l_len) & ~0xffff));
    
    /* must have odd length */
    assert(State->rr2l_len & 0x1);

    srr2l_init_mem();
    sl2rr_init_mem();

    State->sl2rr_ack = SL2RR_ACK;
    State->sl2rr_state = SL2RR;
    State->srr2l_state = SRR2L;

    State->heap = ALIGN( State->heap, HOST_LINESIZE );
    State->freemap = (volatile freemap_t *)
	heap_malloc(sizeof(freemap_t)*BP_SFM_ENTRIES*BP_MAX_JOBS, UNCACHED);
    dprintf(1, ("freemap \t= 0x%x\n", State->freemap));

    State->heap = ALIGN( State->heap, HOST_LINESIZE );
    State->hostx = (u_int *) 
	heap_malloc(sizeof(u_int)*BP_HOSTX_ENTRIES*BP_MAX_JOBS, CACHED);
    dprintf(1, ("hostx \t\t= 0x%x\n", State->hostx));

    State->heap = ALIGN(State->heap, 16384);
    State->sdq  = (hippi_bp_desc *)
	heap_malloc(sizeof(hippi_bp_desc)*BP_SDQ_ENTRIES*BP_MAX_JOBS, 
		    UNCACHED);
    dprintf(1, ("sdq \t\t= 0x%x\n", State->sdq));
    
#else /* HIPPI_DST_FW */

    State->ablk = &ablk;
    State->wblk = &wblk;
    State->mbuf_state = &MBUF;
    State->fpbuf_state = &FPBUF;

    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    State->bp_dst_desc = (hippi_bp_desc *)
	heap_malloc(sizeof(hippi_bp_desc), UNCACHED);
    dprintf(1, ("bp_dst_desc \t= 0x%x\n", State->bp_dst_desc));

    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    State->sm_buf = (mbuf_t *)
	heap_malloc(sizeof(mbuf_t)*(HIP_MAX_SML+1), UNCACHED);
    dprintf(1, ("sm_mbuf \t= 0x%x\n", State->sm_buf));

    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    State->lg_buf = (mbuf_t *)
	heap_malloc(sizeof(mbuf_t)*(HIP_MAX_BIG+1), UNCACHED);
    dprintf(1, ("lg_mbuf \t= 0x%x\n", State->lg_buf));

    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    State->fpbuf = (hip_c2b_t *)
	heap_malloc(FPBUF_DMA_LEN, CACHED);
    dprintf(1, ("fpbuf \t= 0x%x\n", State->fpbuf));
    
    State->heap = ALIGN( State->heap, DCACHE_LINESIZE);
    State->l2rr_len = 0;		/* not needed */
    State->dl2rr = (dst_l2rr_t *)
	heap_malloc(sizeof(dst_l2rr_t), UNCACHED);
    dprintf(1, ("dl2rr \t\t= 0x%x\n", State->dl2rr));

    /*  end of the ring must be on a 64 KB boundary */
    State->rr2l_len = DST_RR2L_SIZE-1;

    State->heap = (caddr_t)(((u_int)State->heap & ~0xffff) 
				 + (u_int)0x10000);
    State->drr2l = (dst_rr2l_t*)State->heap - State->rr2l_len;
    dprintf(1, ("drr2l \t\t= 0x%x\t end = 0x%x\n", 
		State->drr2l, State->drr2l + State->rr2l_len));

    assert( ((u_int)State->drr2l & ~0xffff) + 0x10000 ==
	   ((u_int)(State->drr2l+State->rr2l_len) & ~0xffff));

    /* must have odd length */
    assert( (State->rr2l_len & 0x1));

    drr2l_init_mem();
    State->drr2l_state = DRR2L;

    State->heap = ALIGN( State->heap, HOST_LINESIZE );
    State->freemap = (volatile freemap_t *)
	heap_malloc(sizeof(freemap_t)*BP_DFM_ENTRIES*BP_MAX_JOBS, UNCACHED);
    dprintf(1, ("freemap \t= 0x%x\n", State->freemap));

    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    State->bpseqnum = (bp_seq_num_t *)
	heap_malloc(sizeof(bp_seq_num_t) 
		    * BP_MAX_SLOTS * BP_MAX_JOBS, CACHED);
    dprintf(1, ("bpseqnum \t= 0x%x\n", State->bpseqnum));

    State->heap = ALIGN(State->heap, DCACHE_LINESIZE);
    State->port = (bp_port_state_t *)
	heap_malloc(sizeof(bp_port_state_t)*BP_MAX_PORTS, CACHED); 
    dprintf(1, ("port \t\t= 0x%x\n", State->port));

#endif /* HIPPI_DST_FW */

    State->heap = ALIGN( State->heap, HOST_LINESIZE );
    State->d2b = (gen_d2b_t *)
	heap_malloc(sizeof(gen_d2b_t)*(LOCAL_D2B_LEN), CACHED);
    dprintf(1, ("d2b \t\t= 0x%x\n", State->d2b));

    d2b_init_mem();
    State->d2b_state = D2B;


    heap_malloc(C2B_D2B_DEAD_ZONE, CACHED);

    State->heap = ALIGN( State->heap, DCACHE_LINESIZE);
    State->b2h = (hip_b2h_t *) heap_malloc(sizeof(hip_b2h_t)*B2H_SIZE, CACHED);
    dprintf(1, ("b2h \t\t= 0x%x\n", State->b2h));

    b2h_init_mem();
    State->b2h_state = B2H;

    /* what's left of the heap goes to the data ring buffer */

    State->heap = ALIGN( State->heap, 128);
    State->data_M = (u_int *) State->heap;

    /* roadrunner firmware requires end of buffer be on 64 KB boundary,
       but prefetch requires a barrier of 128 bytes at the end,
       so loose 64 KB instead.
    */

    
    State->data_M_len = (SDRAM_END - (int)K1_TO_PHYS(State->data_M) - 64*1024)/4;
    State->data_M_endp = State->data_M + State->data_M_len;

    assert( ((u_int)(State->data_M_endp) & 0xffff) == 0);

    /* sufficient buffer space? */
    assert(State->data_M_len > 1024*128*3/4);

    dprintf(1, ("data_M \t\t= 0x%x, endp = 0x%x, len = 0x%x\n", 
		State->data_M, State->data_M_endp, State->data_M_len));

    State->rr_mem = (rr_pci_mem_t *) K0_TO_K1(LINC_CPCI_PIO_ADDR);
    dprintf(1, ("rr_mem mem \t= 0x%x\n", State->rr_mem));
    
    State->rr_config = (pci_cfg_hdr_t *)
	K0_TO_K1(LINC_CPCI_CONFIG_ADDR+RR_CONFIG_OFFSET);
    dprintf(1, ("rr_config \t= 0x%x\n", State->rr_config));
	    
}

/************************************************************
  test_rr_sram 
************************************************************/

void
test_rr_sram (volatile rr_pci_mem_t *pci) {
    int i;
    u_int j;

    /* test all of roadrunner memory */
    /* basic data path stuck at ones test */
    pci->win_base_reg = 0;
    for (i = 0; i < RR_SRAM_SIZE/RR_MEMWIN_SIZE; i++) {
	for (j = 0; j < RR_MEMWIN_SIZE; j++)
	    pci->rr_sram_window[j] = 0xaaaa5555;
	pci->win_base_reg += RR_MEMWIN_SIZE*4;
    }

    /* verify */
    pci->win_base_reg = 0;
    for (i = 0; i < RR_SRAM_SIZE/RR_MEMWIN_SIZE; i++) {
	for (j = 0; j < RR_MEMWIN_SIZE; j++) {
	    if (pci->rr_sram_window[j] != 0xaaaa5555) {
		dprintf(1, ("test_rr_sram: FAILED addr = 0x%x, data = 0x%x\n",
			    pci->win_base_reg+(j*4), pci->rr_sram_window[j]));
		die(CDIE_RR_ERR, CDIE_AUX_RR_SSRAM_DATA1); 
	    }
	}
	pci->win_base_reg += RR_MEMWIN_SIZE*4;
    }

    pci->win_base_reg = 0;
    for (i = 0; i < RR_SRAM_SIZE/RR_MEMWIN_SIZE; i++) {
	for (j = 0; j < RR_MEMWIN_SIZE; j++)
	    pci->rr_sram_window[j] = 0x5555aaaa;
	pci->win_base_reg += RR_MEMWIN_SIZE*4;
    }

    /* verify */
    pci->win_base_reg = 0;
    for (i = 0; i < RR_SRAM_SIZE/RR_MEMWIN_SIZE; i++) {
	for (j = 0; j < RR_MEMWIN_SIZE; j++) {
	    if (pci->rr_sram_window[j] != 0x5555aaaa) {
		dprintf(1, ("test_rr_sram: FAILED addr = 0x%x, data = 0x%x\n",
			    pci->win_base_reg+(j*4), pci->rr_sram_window[j]));
		die(CDIE_RR_ERR, CDIE_AUX_RR_SSRAM_DATA2); 
	    }
	}
	pci->win_base_reg += RR_MEMWIN_SIZE*4;
    }

    /* basic address bits check */
    pci->win_base_reg = 0;
    pci->win_data_reg = 0;
    for (i = 0x4; i != RR_SRAM_SIZE; i = i<<1) {
	pci->win_base_reg = i;
	pci->win_data_reg = i;
    }

    /* verify */
    pci->win_base_reg = 0;
    if(pci->win_data_reg != 0) die(CDIE_RR_ERR, CDIE_AUX_RR_SSRAM_ADDR); 
    for (i = 0x4; i != RR_SRAM_SIZE; i = i<<1) {
	pci->win_base_reg = i;
	if (pci->win_data_reg != i) die(CDIE_RR_ERR, CDIE_AUX_RR_SSRAM_ADDR); 

    }
}


/************************************************************
  rr_pkt_accept
************************************************************/

void 
rr_pkt_accept(int flag) {
    u_int i, j;

    if (flag) {			/* enable reception of pkts */
	State->rr_mem->rr_gca.cmd.mb.mailbox[31] = RR_STATUS_ACCEPT_CONN;
    }
    else {				/* disable reception */
	/* this has the side effect of also clearing receive errors*/
	State->rr_mem->rr_gca.cmd.mb.mailbox[31] = RR_STATUS_REJECT_CONN;
    }

    for (i = 0; i < 1000; i++) {
	j = State->rr_mem->rr_gca.cmd.mb.mailbox[31];
	if (j == RR_STATUS_INIT_SUCCESS || j == RR_STATUS_GLINK_DOWN)
	    break;
    }
    if (i == 1000)
        die(CDIE_RR_ERR, CDIE_AUX_RR_FW_DEAD);
      
}

/************************************************************
  init_roadrunner
************************************************************/

void
init_roadrunner(void) {
    volatile rr_pci_mem_t *pci;
    volatile pci_cfg_hdr_t *pci_cfg;

    int i, j;
    u_int ui;
    volatile u_int vui;

    volatile src_l2rr_t l2rr;
    volatile src_rr2l_t *rr2l;


    pci_cfg = State->rr_config;
    pci = State->rr_mem;

    /* check vendor/device ID */
    State->hcmd->sign = HIP_SIGN_RR_VEND;
    dprintf(1, ("init_roadrunner: reading from CPCI config space = 0x%x\n", 
		pci_cfg));
    
    for (i = 0; 
	 (  (LINC_PCVEND_DEV_ID_R(pci_cfg->dev_vend_id) != RR_PCI_DEVICE_ID)
	    || (LINC_PCVEND_VEN_ID_R(pci_cfg->dev_vend_id) != RR_PCI_VENDOR_ID))
	 && i < 10; i++)
	;

    if (i == 10)
        die(CDIE_RR_CFG_ERR, CDIE_AUX_RR_CFG_VEND);

    /* wait for BIST to be okay */

    State->hcmd->sign = HIP_SIGN_RR_BIST;
    dprintf(1, ("init_roadrunner:  testing BIST \n"));

    pci_cfg->misc_host_ctrl_reg = RR_HARDRESET | RR_HALT ;

    for (i = 0; (LINC_PCHDR_BIST_R(pci_cfg->bhlc.i) != RR_BIST_MEM_64K) 
	         && i < 10000; i++)
	;

    if (i == 10000)
        die(CDIE_RR_CFG_ERR, CDIE_AUX_RR_CFG_BIST);

    /* setup memory window */
    State->hcmd->sign = HIP_SIGN_RR_WIN;
    dprintf(1, ("init_roadrunner: testing window size \n"));

    pci_cfg->bar0 = 0xffffffff;

    /* waste some time */
    for(i = 0; i < 1000; i++)
	;

    ui = pci_cfg->bar0;

    for (i = 0; i < 32; i++) {
	if ((ui >> i) & 0x1)	/* find first non-zero */
	    break;
    }
    if (i != 12)  
        die(CDIE_RR_CFG_ERR, CDIE_AUX_RR_CFG_WIN);  /* should be 4KB Window */

    /* clear status and enable dma, mem, and error reporting */
    pci_cfg->stat_cmd = LINC_PCCSR_DEVSEL_TIMING_W(RR_PCISTAT_MED_TIME)
	| LINC_PCCSR_SERR_EN | LINC_PCCSR_PAR_RESP_EN 
	| LINC_PCCSR_MASTER_EN | LINC_PCCSR_MEM_SPACE | 0xffff0000;
    
    pci_cfg->bar0 = RR_PCI_MEM_BASE;

    /* memory space is enabled */
    /* make sure processor is halted, and then test memory */
    if ( !(pci->misc_host_ctrl_reg & RR_HALTED)) 
	die(CDIE_RR_CFG_ERR, CDIE_AUX_RR_CFG_HALT);

    State->hcmd->sign = HIP_SIGN_RR_MEM;
    dprintf(1, ("init_roadrunner: testing roadrunner memory \n"));
    test_rr_sram(pci);

    State->hcmd->sign = HIP_SIGN_RR_BOOT;
    {    /* download roadrunner firmware through memory space window */
	int size;
	u_int *fw;

#ifdef HIPPI_SRC_FW
	pci->win_base_reg = rr_src_fw_base;
	size = rr_src_fw_size;
	fw = rr_src_fw;
#else
	pci->win_base_reg = rr_dst_fw_base;
	size = rr_dst_fw_size;
	fw = rr_dst_fw;
#endif

	for (i = 0; i < (size/RR_MEMWIN_SIZE)+1; i++) {
	    int j, j_max;
	    if (size - i*RR_MEMWIN_SIZE == 0) /* reqd because of remainder */
		break;
	    if (size - i*RR_MEMWIN_SIZE < RR_MEMWIN_SIZE)
		j_max = (size - i*RR_MEMWIN_SIZE);
	    else
		j_max = RR_MEMWIN_SIZE;
	    
	    for (j = 0; j < j_max; j++) {
		pci->rr_sram_window[j] = fw[i*RR_MEMWIN_SIZE + j];
	    }
	    pci->win_base_reg += RR_MEMWIN_SIZE*4;
	}
	dprintf(1, ("init_roadrunner: Download of RR firmware complete\n"));
    }

#ifdef HIPPI_SRC_FW
    /* initialize command/ack queues */
    sl2rr_init_state();
    srr2l_init_state();

    /* setup gen control area with pointers to regions in 
       SDRAM memory plus stimeout. This must happen
       after download of code.
    */
    pci->rr_gca.cmd.mb.s.timeo = HIPPI_DEFAULT_TIMEO;
    pci->rr_gca.cmd.mb.s.PH_on = 0;
    pci->rr_gca.cmd.mb.s.retry_count = 0;

    /* see init_linc for how prefetch is used */
    pci->rr_gca.cmd.l2rr = K1_TO_PHYS((u_int)State->sl2rr) 
	| SRC_L2RR_DESC_ATTR;

    pci->rr_gca.cmd.rr2l = K1_TO_PHYS((u_int)State->srr2l) 
	| SRC_RR2L_DESC_ATTR;
    pci->rr_gca.cmd.rr2l_end = K1_TO_PHYS(State->srr2l + State->rr2l_len) 
	| SRC_RR2L_DESC_ATTR;

    /* reset the program counter */
    pci->prog_counter_reg = rr_src_startPC;

#else /* HIPPI_DST_FW */

    drr2l_init_state();

    update_dl2rr_datap(State->data_M, 0);
    update_dl2rr_descp(State->drr2l-1); /* func automagically inc's by one */

    pci->rr_gca.cmd.mb.d.timeo = HIPPI_DEFAULT_TIMEO;
    pci->rr_gca.cmd.mb.d.PH_on = 0;

    pci->rr_gca.cmd.l2rr = 0;	/* only valid for source */

    /* see init_linc for how prefetch is used */
    pci->rr_gca.cmd.rr2l = K1_TO_PHYS((u_int)State->drr2l) 
	| DST_RR2L_DESC_ATTR;
    pci->rr_gca.cmd.rr2l_end = K1_TO_PHYS(State->drr2l + State->rr2l_len)
	| DST_RR2L_DESC_ATTR;

    pci->rr_gca.cmd.ring_consumers = K1_TO_PHYS((u_int)State->dl2rr) 
	| DST_L2RR_DESC_ATTR;

    pci->rr_gca.cmd.b_data_buff = K1_TO_PHYS((u_int)State->data_M)
	| DST_RR_DATA_ATTR;
    pci->rr_gca.cmd.b_data_buff_end = K1_TO_PHYS(State->data_M + State->data_M_len)
	| DST_RR_DATA_ATTR;

    /* reset the program counter */
    pci->prog_counter_reg = rr_dst_startPC;

#endif /* HIPPI_SRC_FW */

    /* prep sequence numbers in cmd regions */
    pci->rr_gca.cmd.cmd.i[0] = 0;
    pci->rr_gca.cmd.ack_data.i[0] = 0;

    /* restart roadrunner processor */
    pci->rr_gca.cmd.mb.mailbox[31] = RR_STATUS_DEAD;
    pci->misc_host_ctrl_reg = CLEAR_PCI_INTR;

    dprintf(1, ("init_rr: - waiting for RR to boot\n"));

    /* wait for rr firmware to boot */
    for (i = 0; (pci->rr_gca.cmd.mb.mailbox[31] == RR_STATUS_DEAD) 
	         && (i < 10000); i++)
	;

    if (i == 10000)
	die(CDIE_RR_ERR, CDIE_AUX_RR_FW_BOOT);

    dprintf(1, ("init_roadrunner: roadrunner UP\n"));
}

/************************************************************
  set_state_flags
************************************************************/

void 
set_state_flags(gen_host_t *hostp, u_int host_flags, u_int *flags) {
    char *bbpal_misc = (char*)PHYS_TO_K1(LINC_BBPAL_MISC);

    if (host_flags & HIP_FLAG_ACCEPT){ 
	if ( !(*flags & FLAG_ACCEPT)) {	/* first time enabled */
	    *flags |= FLAG_ACCEPT;

#ifdef HIPPI_SRC_FW
	    if (host_flags & HIP_FLAG_LOOPBACK) {
	        /* enable glink and loopback*/
	        State->flags |= FLAG_LOOPBACK;
		*bbpal_misc = LINC_BB_MISC_TDAV | LINC_BB_MISC_LB;
	    }
	    else
	        /* enable glink */
	        *bbpal_misc = LINC_BB_MISC_TDAV;
#else  /* HIPPI_DST_FW */
	    if (host_flags & HIP_FLAG_LOOPBACK)
	        /* enable glink and loopback*/
	        State->flags |= FLAG_LOOPBACK;

	    rr_pkt_accept(1);
#endif
	}
    }
    else  {			/* disable interface */
	*flags &= ~FLAG_ACCEPT;

#ifdef HIPPI_DST_FW
	rr_pkt_accept(0);
#endif
    }    


#ifdef HIPPI_DST_FW
    if (host_flags & HIP_FLAG_IF_UP) { /* enable LE interface */
	if ( !(*flags & FLAG_ENB_LE)) { /* not already enabled */
	    *flags |= FLAG_ENB_LE;
	    hostp->fpstk[HIP_STACK_LE].flags = FP_STK_ENABLED;
	    hostp->ulptostk[HIPPI_ULP_LE] = HIP_STACK_LE;
	}
    }
    else {			/* disable LE interface */
	*flags &= ~FLAG_ENB_LE;
	init_mbufs();
    }
#endif	

}

void
init_trace(void) {

    Trace_base = (trace_t *)PHYS_TO_K1(LINC_TRACE_BASE);
    for (Tracep = Trace_base; Tracep < Trace_base+LINC_TRACE_BUF_SIZE; 
	 Tracep++) {
	Tracep->l[0] = 0;
	Tracep->l[1] = 0;
    }
    Tracep = Trace_base;
    State->traceput = &Tracep;
    State->trace_basep = Trace_base;
    
}

void ltrace(char op, u_int arg0, u_int arg1, u_int arg2, u_int arg3) {
    Tracep->s.op = op;
/*    Tracep->arg0 |= (get_r4k_count() >> (32-TRACE_OP_SHIFT)) & ~TRACE_OP_MASK;*/
    Tracep->s.time = get_r4k_count() >> TOP_TIME_SHIFT;

    Tracep->s.arg0 = arg0;
    Tracep->s.arg1 = arg1;
    Tracep->s.arg2 = arg2;
    Tracep->s.arg3 = arg3;

    Tracep++;
    if (Tracep >= Trace_base + LINC_TRACE_BUF_SIZE)
	Tracep = Trace_base;
}


/************************************************************
  hcmd_init
************************************************************/

void
hcmd_init(gen_host_t *hostp, gen_wire_t *wirep) {
    int i;

#define HINIT State->hcmd->arg.init

    dprintf(1, ("fw: got HCMD_INIT\n"));
    init_trace();
    trace(TOP_MISC, T_MISC_INIT, 0, 0, 0);

    State->nbpp = HINIT.host_nbpp_mlen & ~HIP_INIT_MLEN_MASK;

    /* setup pointers to host structures */
    b2h_init_state(HINIT.b2h_buf, HINIT.b2h_buf_hi, HINIT.b2h_len);

    /* initialize watchdog timers */
    timer_set(&State->poll_timer);
    timer_set(&State->sleep_timer);

    State->sleep_addr_lo = (u_int)HINIT.b2h_sleep;
    State->sleep_addr_hi = (u_int)(HINIT.b2h_sleep>>32);

#ifdef PEER_TO_PEER_DMA_WAR
    /* This field is not used with this WAR so init it to 0. */
    State->opposite_addr = 0;
#else
    /* We're not using the WAR fields so init them to 0. */
    State->local_addr_hi = 0;
    State->local_addr_lo = 0;
    State->opposite_addr_hi = 0;
    State->opposite_addr_lo = 0;

    State->opposite_addr = HINIT.peer_pcibase + K1_TO_PHYS(State->opposite_st);

    dprintf(1, ("opposite_addr = 0x%x\n", State->opposite_addr));

#ifndef DEBUG
    sync_opposite();
#endif
#endif
    
    State->flags |= FLAG_OPPOSITE_UP | FLAG_GOT_INIT;
    dprintf(1, ("hcmd_init: synched w/ opposite linc\n"));
    
#ifdef HIPPI_SRC_FW
      
#ifdef PEER_TO_PEER_DMA_WAR
    /* If we're using host mem, init addresses */
    State->local_addr_hi = HINIT.src_msg_area_hi;
    State->local_addr_lo = HINIT.src_msg_area_lo;

    dprintf(1, ("local_addr_hi: 0x%x, lo: 0x%x\n",
		State->local_addr_hi,State->local_addr_lo));

    State->opposite_addr_hi = HINIT.dst_msg_area_hi;
    State->opposite_addr_lo = HINIT.dst_msg_area_lo;

    dprintf(1, ("opposite_addr_hi: 0x%x, lo: 0x%x\n",
		State->opposite_addr_hi,State->opposite_addr_lo));

#ifndef DEBUG
    sync_opposite();
#endif
#endif

    /*  because fp is one d2b, will never get SRC_BLK_SIZE len blks */
    State->blksize = SRC_BLK_SIZE/State->nbpp; 
    ASSERT(State->blksize != 0);
    
    d2b_init_state(HINIT.d2b_buf, HINIT.d2b_buf_hi, HINIT.d2b_len);

    hostp->st = SH_IDLE;
    hostp->cur_job = 0;
    hostp->msg_flags = 0;
    hostp->rem_len = 0;
    hostp->dp_noack = hostp->dp_put;

    /* init wire state */
    wirep->st = SW_IDLE;
    wirep->ack_st = SW_IDLE;
    wirep->flags = 0;
    wirep->ack_flags = 0;


    /* init src desc queues */
    {
	u_int *ip = ((u_int *)State->sdq) + 3; /* invalidate last word */

	for (i = 0; i < BP_SDQ_ENTRIES*BP_MAX_JOBS*sizeof(hippi_bp_desc)/4; 
	            i += sizeof(hippi_bp_desc)/4) {
	    *ip = HIP_BP_DQ_INV;
	    ip += 4;
	}
	  
    }
      
#else  /* HIPPI_DST_FW */

#ifdef PEER_TO_PEER_DMA_WAR
    /* If we're using host mem, init addresses */
    State->local_addr_hi = HINIT.dst_msg_area_hi;
    State->local_addr_lo = HINIT.dst_msg_area_lo;

    dprintf(1, ("local_addr_hi: 0x%x, lo: 0x%x\n",
		State->local_addr_hi,State->local_addr_lo));

    State->opposite_addr_hi = HINIT.src_msg_area_hi;
    State->opposite_addr_lo = HINIT.src_msg_area_lo;

    dprintf(1, ("opposite_addr_hi: 0x%x, lo: 0x%x\n",
		State->opposite_addr_hi,State->opposite_addr_lo));

#ifndef DEBUG
    sync_opposite();
#endif
#endif

    State->blksize = DST_BLK_SIZE;
    ASSERT(State->blksize != 0);
    
    d2b_init_state(HINIT.c2b_buf, HINIT.c2b_buf_hi, HINIT.c2b_len);

    drr2l_init_state();
      
    hostp->st = DH_IDLE;
    hostp->flags = 0;
    hostp->stack = HIP_N_STACKS;
      
    wirep->st = DW_NEOC;

    /* throw out m_bufs */
    init_mbufs();

    /* invalid stack structure */
    for (i = 0; i < HIP_N_STACKS+1; i++)
	hostp->fpstk[i].flags = 0;
      
    /* invalidate ulp to stack lookups */
    for (i = 0; i < 256; i++)
	hostp->ulptostk[i] = HIP_N_STACKS;

#endif /* HIPPI_DST_FW */	  

    hostp->dp_put	= hostp->basep;

    hostp->job_vector = 0;

    /* zero out statistics */
    bzero((void *)hostp->stats, sizeof(hippi_stats_t));
    bzero((void *)hostp->bpstats, sizeof(hippibp_stats_t));

    wbinval_dcache(hostp->bpstats, sizeof(hippibp_stats_t));
    wbinval_dcache(hostp->stats, sizeof(hippi_stats_t));
    
    /* check that roadrunner is up - this is a quick sanity check
     * before going into normal mechanism (intr handler sets flag, 
     * update_slow_state checks for death).
     */
    if (State->rr_mem->rr_gca.cmd.mb.mailbox[31] == RR_STATUS_DEAD)
	die(CDIE_RR_ERR, CDIE_AUX_RR_FW_DEAD);

    State->flags |= FLAG_RR_UP;

    State->flags &= ~FLAG_ASLEEP;
    set_state_flags(hostp, HINIT.iflags, &State->flags);

}


/************************************************************
  hcmd_params
************************************************************/
void
hcmd_params(gen_host_t *hostp) {

  dprintf(1, ("fw: got HCMD_PARAMS\n"));

#define HPARAMS State->hcmd->arg.params
    set_state_flags(hostp, HPARAMS.flags, &State->flags);

#ifdef HIPPI_SRC_FW
    State->rr_mem->rr_gca.cmd.mb.s.timeo = HPARAMS.timeo*1000;
#endif

#undef HPARAMS
}

/************************************************************
  hcmd_bp_job
************************************************************/
void
hcmd_bp_job(gen_host_t *hostp, gen_wire_t *wirep) {
    bp_seq_num_t *slotp;
    bp_job_state_t *jobp;
    int i;

#define HCMD_JOB  ((hip_bp_hc_t *)State->hcmd)->arg.bp_job

    dprintf(1, ("HCMD_BP_JOB: jb=%d, auth=0x%x,%x,%x, ackhst=%d, ackport=%d\n",
		HCMD_JOB.job, 
		HCMD_JOB.auth[0], HCMD_JOB.auth[1], HCMD_JOB.auth[2],
	    HCMD_JOB.ack_host, HCMD_JOB.ack_port));

    jobp = &(hostp->job[HCMD_JOB.job]);
    if (HCMD_JOB.enable) {
	hostp->job_vector |= (0x80000000 >> HCMD_JOB.job); /* enable job */
	jobp->fm_entry_size = HCMD_JOB.fm_entry_size;
	jobp->auth[0] = HCMD_JOB.auth[0];
	jobp->auth[1] = HCMD_JOB.auth[1];
	jobp->auth[2] = HCMD_JOB.auth[2];

	/*setup ack host and port for direct insertion into packet */
	jobp->ack_hostport = (BP_ACK_HOST_MASK & HCMD_JOB.ack_host) <<  BP_ACK_HOST_SHIFT;
	jobp->ack_hostport |=  BP_ACK_PORT_MASK & HCMD_JOB.ack_port; 

#ifdef HIPPI_SRC_FW
	jobp->sdq_head = hostp->sdq + ((int)HCMD_JOB.job*BP_SDQ_ENTRIES);
	jobp->sdq_end = jobp->sdq_head + BP_SDQ_ENTRIES;

#endif /* HIPPI_SRC_FW */
    }
    else {			/* disable job */
	hostp->job_vector &= ~(0x80000000 >> HCMD_JOB.job); /* disable job */

#ifdef HIPPI_SRC_FW
	jobp->sdq_head = hostp->sdq + ((int)HCMD_JOB.job * BP_SDQ_ENTRIES);
	for (i = 0; i < BP_SDQ_ENTRIES; i++) {
	    jobp->sdq_head->i[3] = HIP_BP_DQ_INV;
	    jobp->sdq_head++;
	}
	jobp->sdq_head = hostp->sdq + ((int)HCMD_JOB.job * BP_SDQ_ENTRIES);
	/* flush data cache of hostx */
	inval_dcache(hostp->hostx + ((int)HCMD_JOB.job*BP_HOSTX_ENTRIES),
		     BP_HOSTX_ENTRIES*sizeof(u_int));

#else /* HIPPI_DST_FW */

	slotp = hostp->bpseqnum + (HCMD_JOB.job*BP_MAX_SLOTS);
	for (i = 0; i < BP_MAX_SLOTS; i++) { /* invalid slot info */
	  (slotp+i)->expseqnum = 0;

	}
#endif /* HIPPI_SRC_FW */

    }
#undef HCMD_JOB
}


/************************************************************
  hcmd_bp_port
************************************************************/
void
hcmd_bp_port(gen_host_t *hostp, gen_wire_t *wirep) {

    bp_port_state_t *port_p;

    dprintf(1, ("fw: got HCMD_BP_PORT\n"));

#ifdef HIPPI_DST_FW

#define HCMD_PORT  ((hip_bp_hc_t *)State->hcmd)->arg.bp_port
    port_p = &(hostp->port[HCMD_PORT.port]); 
    if(HCMD_PORT.ux.s.opcode != HIP_BP_PORT_DISABLE) {
	port_p->st		= BP_PORT_VAL;
	port_p->job		= HCMD_PORT.job;

	/* size of host buffer */
	port_p->d_data_size	= hostp->job[port_p->job].fm_entry_size; 

	/* setup descriptor queue state */
	port_p->dq_base_lo	= HCMD_PORT.ddq_lo;
	port_p->dq_base_hi	= HCMD_PORT.ddq_hi;
	port_p->dq_size		= HCMD_PORT.ddq_size/sizeof(hippi_bp_desc);
	port_p->dq_tail		= 0;

	port_p->int_cnt		= 0;
    }
    else {			/* disable port */
	port_p->st		= BP_PORT_INVAL;
    }
#undef HCMD_PORT
#endif /* HIPPI_DST_FW */
}


/************************************************************
  hcmd_bp_conf
************************************************************/

void
hcmd_bp_conf(gen_host_t *hostp) {
    /*disable all jobs */
    hostp->job_vector = 0;
    hostp->bp_ulp = ((hip_bp_hc_t *)State->hcmd)->arg.bp_conf.ulp;

    dprintf(1, ("HCMD_BP_CONF: bp_ulp = 0x%x\n", hostp->bp_ulp));
}

/************************************************************
  hcmd_portint_ack
************************************************************/

void
hcmd_bp_portint_ack(gen_host_t *hostp) {
    bp_port_state_t *port_p;

    dprintf(1, ("fw: got HCMD_BP_PORTINT_ACK\n"));

#ifdef HIPPI_DST_FW     
#define HCMD_INTACK  ((hip_bp_hc_t *)State->hcmd)->arg.bp_portint_ack
    port_p = &hostp->port[HCMD_INTACK.portid];
    if (port_p->int_cnt == HCMD_INTACK.cookie) {
	/* no new interrupts, so clear state */
	port_p->st &= ~BP_PORT_BLK_INT;
    }

    else {			/* new interrupts received, post them */
	hip_b2h_t b2h;

	/* wait for room */
	while(LINC_DCSR_VAL_DESC_R(LINC_READREG(
						LINC_DMA_CONTROL_STATUS_0)) > 2)
	    continue;

	b2h.b2h_op = HIP_B2H_BP_PORTINT;
	b2h.b2hu.b2h_bp_portint.portid = 
	    (u_short)HCMD_INTACK.portid;
	b2h.b2h_l = port_p->int_cnt;
	b2h_queue(&b2h);
    }
#undef HCMD_INTACK
	
#endif /* HIPPI_DST_FW */
}

/************************************************************
  hcmd_wakeup
************************************************************/

void
hcmd_wakeup(void) {
    int old_debug = debug;

    dprintf(1, ("fw: got HCMD_WAKEUP\n"));
    State->flags &= ~FLAG_ASLEEP;

}

/************************************************************
  hcmd_status
************************************************************/

void
hcmd_status(gen_host_t *hostp) {
    u_int tmp1, tmp2;
    char *bbpal_misc = (char*)PHYS_TO_K1(LINC_BBPAL_MISC);

    dprintf(1, ("fw: got HCMD_STATUS\n"));

    /* set flags to XIO */
    hostp->stats->hst_flags = HST_XIO;
#ifdef HIPPI_SRC_FW
    State->stats->hst_flags |=  (*bbpal_misc & LINC_BB_MISC_SIG_DET)
	    ? HST_FLAG_DST_SIG_DET : 0;

    /* read interface signal state from roadrunner */
    tmp1 = State->rr_mem->xmit_state_reg;
    State->stats->hst_flags |= (tmp1 & RR_XMIT_REQUEST) 
	? HST_FLAG_SRC_REQOUT : 0;
    State->stats->hst_flags |= (tmp1 & RR_XMIT_CONN) ? HST_FLAG_SRC_CONIN : 0;

    /* make sure rr internal optical link between dest/src are sync'ed */
    tmp1 = State->rr_mem->recv_state_reg & RR_RCV_FLAG_SYNC;
    tmp2 = State->rr_mem->hippi_ovrhd_reg & 0x1;

    State->stats->hst_flags |= (State->flags & FLAG_LOOPBACK) 
	? HST_FLAG_LOOPBACK : 0;

    State->stats->sf.hip_s.xmit_retry = State->rr_mem->rr_gca.cmd.mb.s.retry_count;

#else  /* HIPPI_DST_FW */

    State->stats->hst_flags |= (State->flags & FLAG_ACCEPT) 
	? HST_FLAG_DST_ACCEPT:0;

    /* read interface signal state from roadrunner */
    tmp1 = State->rr_mem->recv_state_reg;
    State->stats->hst_flags |= (tmp1 & RR_RCV_PKT) ? HST_FLAG_DST_PKTIN : 0;
    State->stats->hst_flags |= (tmp1 & RR_RCV_REQUEST) ? HST_FLAG_DST_REQIN : 0;

    State->stats->hst_flags |= (tmp1 & RR_RCV_LNK_READY) 
	? HST_FLAG_DST_LNK_RDY : 0;
    State->stats->hst_flags |= (tmp1 & RR_RCV_FLAG_SYNC) 
	? HST_FLAG_DST_FLAG_SYNC : 0;

    tmp1 = State->rr_mem->hippi_ovrhd_reg;
    State->stats->hst_flags |= (tmp1 & RR_OVRHD_OH8_SYNC) 
	? HST_FLAG_DST_OH8_SYNC : 0;

#endif
    
    wbinval_dcache(hostp->stats, sizeof(hippi_stats_t));
    
}


/************************************************************
  hcmd_sdqhead
************************************************************/

void
hcmd_sdqhead(gen_host_t *hostp) {
#ifdef HIPPI_SRC_FW
  int job = ((hip_bp_hc_t *)State->hcmd)->arg.sdqhead_jobnum;
  bp_job_state_t *jobp;

  dprintf(1, ("hcmd_sdqhead, job = %d\n", job));

  jobp = &(hostp->job[job]);
  
  ((hip_bp_hc_t *)State->hcmd)->res.sdqhead = (u_int)jobp->sdq_head;
#endif

}

/************************************************************
  hcmd_assign_ulp
************************************************************/

#ifdef HIPPI_DST_FW
void
hcmd_assign_ulp(dst_host_t *hostp, int enable) {

#define HCMD_ASS  State->hcmd->arg.cmd_data[0]
    
    trace(TOP_MISC, T_MISC_ULP, enable, HCMD_ASS & 0xffff,
	  (HCMD_ASS & ~0xffff)>>16);

    if (enable) {
	hostp->fpstk[HCMD_ASS & 0xffff].flags |= FP_STK_ENABLED;
	hostp->ulptostk[(HCMD_ASS & ~0xffff)>>16] = HCMD_ASS & 0xffff;
	if((HCMD_ASS & 0xffff) == HIP_STACK_RAW)
	    State->flags |= FLAG_HIPPI_PH;
    }
    else {
	hostp->fpstk[HCMD_ASS & 0xffff].flags = 0;
	hostp->ulptostk[(HCMD_ASS & ~0xffff) >> 16] = HIP_N_STACKS;
	if((HCMD_ASS & 0xffff) == HIP_STACK_RAW)
	    State->flags &= ~FLAG_HIPPI_PH;
	
    }
#undef HCMD_ASS
}
#endif


/************************************************************
  process_hcmd
************************************************************/

int 
process_hcmd(gen_host_t *hostp,gen_wire_t *wirep) {

  int i, j, err;
  u_int *ip;


  switch(State->old_cmdid = State->hcmd->cmd)
    {
    case HCMD_NOP:			/* 00	do nothing */
      break;

    case HCMD_INIT:			/* 01	set parameters */
      hcmd_init(hostp, wirep);
      break;

    case HCMD_PARAMS:			/* set operational flags/params */
      hcmd_params(hostp);
      break;

    case HCMD_EXEC:			/* 02	execute downloaded code */
      dprintf(1, ("fw: got HCMD_EXEC\n"));
      break;

    case HCMD_WAKEUP:			/* 04	card has things to do */
      hcmd_wakeup();
      break;

    case HCMD_ASGN_ULP:			/* 05	assign stack to ULP */
      dprintf(1, ("fw: got HCMD_ASSN_ULP\n"));
#ifdef HIPPI_DST_FW
      hcmd_assign_ulp(hostp, 1);
#endif
      break;

    case HCMD_DSGN_ULP:			/* 06	assign stack to ULP */
      dprintf(1, ("fw: got HCMD_DSGN_ULP\n"));
#ifdef HIPPI_DST_FW
      hcmd_assign_ulp(hostp, 0);
#endif
      break;

    case HCMD_STATUS:			/* 07	update status flags */
      hcmd_status(hostp);
      break;

    case HCMD_BP_STATUS:		/* 09	update status flags */
      dprintf(1, ("fw: got HCMD_BPSTATUS\n"));
      State->bpstats->hst_bp_job_vec = hostp->job_vector;
      wbinval_dcache(hostp->bpstats, sizeof(hippibp_stats_t));
      break;

    case HCMD_BP_JOB:		        /* 08   define a bypass job */
      hcmd_bp_job(hostp, wirep);
      break;
	  
     case HCMD_BP_PORT:			/* define a bypass port */
      hcmd_bp_port(hostp, wirep);
      break;

    case HCMD_BP_CONF:		/* configure the bypass */
      hcmd_bp_conf(hostp);
      break;

    case HCMD_BP_PORTINT_ACK:	/* ack for a port interrupt */
      hcmd_bp_portint_ack(hostp);
      break;

    case HCMD_BP_SDQHEAD:	/* get source descriptor queue head pointer */
      hcmd_sdqhead(hostp);
      break;
      
    default:
      dprintf(1, ("fw: got HCMD_UNKNOWN\n"));
      /* ERROR */
      break;
  }
  State->old_cmdid = State->hcmd->cmd_id;
  State->hcmd->cmd_ack = State->hcmd->cmd_id;
  
}


/************************************************************
  init_board
************************************************************/

void
init_board(gen_host_t *hostp, gen_wire_t *wirep) {

    int i, err;
    int *ip;

    State->hcmd->sign = HIP_SIGN_C_INIT;
    State->hcmd->vers = FW_VERSION_NUM;
    State->flags = FLAG_ASLEEP;
    State->leds = 0x0;
    State->cur_link_errcnt = 0;

    init_roadrunner();

    State->flags |= FLAG_RR_UP;

    /* setup bypass configuration region */
    State->bpconfig->num_jobs = BP_MAX_JOBS;
    State->bpconfig->num_ports = BP_MAX_PORTS;
    State->bpconfig->dfl_base = 0;
    State->bpconfig->dfl_size = 0;

    State->dma_statusp = &(State->bpconfig->dma_status);

    bzero(hostp, sizeof(gen_host_t));
    hostp->stats = State->stats;
    hostp->bpstats = State->bpstats;
    hostp->job = State->job;
    hostp->freemap = State->freemap;
    hostp->basep = State->data_M;
    hostp->data_M_len = State->data_M_len;
    hostp->endp = State->data_M_endp;
    hostp->flags = 0;

#ifdef HIPPI_SRC_FW
    hostp->hostx = State->hostx;
    hostp->sdq   = State->sdq;

    wirep->st   = SW_IDLE;
    wirep->ack_st = SW_IDLE;

    State->bpconfig->dfm_base = 0;
    State->bpconfig->dfm_size = 0;
    State->bpconfig->hostx_base = K0_TO_K1((u_int)hostp->hostx) 
	- (u_int)State->hcmd;
    State->bpconfig->hostx_size = BP_HOSTX_ENTRIES * sizeof(u_int);
    State->bpconfig->sfm_base = (u_int)hostp->freemap - (u_int)State->hcmd;
    State->bpconfig->sfm_size = BP_SFM_ENTRIES * sizeof(freemap_t);
    State->bpconfig->sdq_base = (u_int)hostp->sdq -  (u_int)State->hcmd;
    State->bpconfig->sdq_size = BP_SDQ_ENTRIES *  sizeof(hippi_bp_desc);
    State->bpconfig->bpjob_base = K0_TO_K1((u_int)hostp->job) 
	-  (u_int)State->hcmd;
    State->bpconfig->bpjob_size = sizeof(bp_job_state_t);
#else
    hostp->bpseqnum = State->bpseqnum;
    {
	bp_seq_num_t *slotp = hostp->bpseqnum;
        
	for (i = 0; i < BP_MAX_SLOTS*BP_MAX_JOBS; i++) { 
	    /* invalidate slot info */
	    (slotp+i)->expseqnum = 0;
	}
    }

    hostp->port = State->port;

    wirep->st   = DW_NEOC;

    State->bpconfig->dfm_base = K0_TO_K1((u_int)hostp->freemap) 
	- (u_int)State->hcmd;
    State->bpconfig->dfm_size = BP_DFM_ENTRIES * sizeof(freemap_t);
    State->bpconfig->hostx_base = 0;
    State->bpconfig->hostx_size = 0;
    State->bpconfig->sfm_base = 0;
    State->bpconfig->sfm_size = 0;
    State->bpconfig->sdq_base = 0;
    State->bpconfig->sdq_size = 0;
    State->bpconfig->bpjob_base = 0;
    State->bpconfig->bpjob_size = 0;
#endif /* HIPPI_SRC_FW */

    State->bpconfig->bpstat_base = K0_TO_K1((u_int)hostp->bpstats) 
	-  (u_int)State->hcmd;
    State->bpconfig->bpstat_size = sizeof(hippibp_stats_t);
    *State->dma_statusp = 0; /* no dma is active */
    
    dprintf(2, ("bpconfig: num_jobs\t0x%x\n", State->bpconfig->num_jobs));
    dprintf(2, ("bpconfig: num_ports\t0x%x\n", State->bpconfig->num_ports));
    dprintf(2, ("bpconfig: hostx_base\t0x%x\n", State->bpconfig->hostx_base));
    dprintf(2, ("bpconfig: hostx_size\t0x%x\n", State->bpconfig->hostx_size));
    dprintf(2, ("bpconfig: dfl_base\t0x%x\n", State->bpconfig->dfl_base));
    dprintf(2, ("bpconfig: dfl_size\t0x%x\n", State->bpconfig->dfl_size));
    dprintf(2, ("bpconfig: sfm_base\t0x%x\n", State->bpconfig->sfm_base));
    dprintf(2, ("bpconfig: sfm_size\t0x%x\n", State->bpconfig->sfm_size));
    dprintf(2, ("bpconfig: dfm_base\t0x%x\n", State->bpconfig->dfm_base));
    dprintf(2, ("bpconfig: dfm_size\t0x%x\n", State->bpconfig->dfm_size));
    dprintf(2, ("bpconfig: bpstat_base\t0x%x\n", State->bpconfig->bpstat_base));
    dprintf(2, ("bpconfig: bpstat_size\t0x%x\n", State->bpconfig->bpstat_size));
    dprintf(2, ("bpconfig: sdq_base\t0x%x\n", State->bpconfig->sdq_base));
    dprintf(2, ("bpconfig: sdq_size\t0x%x\n", State->bpconfig->sdq_size));
    dprintf(2, ("bpconfig: bpjob_base\t0x%x\n", State->bpconfig->bpjob_base));
    dprintf(2, ("bpconfig: bpjob_size\t0x%x\n", State->bpconfig->bpjob_size));
    dprintf(2, ("bpconfig: dma_status\t0x%x\n", State->bpconfig->dma_status));

    
    for (i = 0; i < 11; i++) {	/* clean out unused region */
	State->bpconfig->reserved[i] = 0;
    }
    
    dprintf(1, ("init_board: finished bypass init\n"));
    /* initialize firmware state */
    State->old_cmdid = 0;
    State->hcmd->cmd_id = 0;

    /* init opposite state counters */
    State->opposite_st->cnt = 0;
    State->opposite_st->flags = 0;

    State->local_st->cnt = 1;
    State->local_st->flags = 0;
    State->opposite_cnt = 0;

    /* store signature for host */
    State->hcmd->sign = HIP_SIGN;
    
    dprintf(1, ("init_board: sign = %x, at 0x%x\n", 
		HIP_SIGN, &(State->hcmd->sign)));
    
}


/************************************************************
  update_slow_state
************************************************************/
void
update_slow_state() {
    char *bbpal_misc = (char*)PHYS_TO_K1(LINC_BBPAL_MISC);

    if (State->flags & FLAG_CHECK_RR_EN) {
	State->flags &= ~FLAG_CHECK_RR_EN;
	check_rr_alive();
    }

    if (State->flags & FLAG_PUSH_TO_OPP) {
	State->flags &= ~FLAG_PUSH_TO_OPP;

#ifdef HIPPI_SRC_FW
	State->local_st->flags =  (*bbpal_misc & LINC_BB_MISC_SIG_DET) ?
	    OPP_SIG_DETECT : 0;
#else	
	State->local_st->flags = 
	    ((State->rr_mem->recv_state_reg & RR_RCV_FLAG_SYNC) ? OPP_FLAG_SYNC : 0)
	    | ((State->rr_mem->recv_state_reg & RR_RCV_LNK_READY) ? OPP_FLAG_LNK_READY: 0)
	    | ((State->rr_mem->hippi_ovrhd_reg & RR_OVRHD_OH8_SYNC) ? OPP_FLAG_OH8SYNC : 0);
#endif
	update_opposite_side();
    }
	    
#ifndef DEBUG
    if (State->flags & FLAG_CHECK_OPP_EN) {
	State->flags &= ~FLAG_CHECK_OPP_EN;
	if(State->opposite_st->cnt != State->opposite_cnt) {
	    State->opposite_cnt = State->opposite_st->cnt;
	    State->flags |= FLAG_OPPOSITE_UP;
	}
	else {
	    State->flags &= ~FLAG_OPPOSITE_UP;
	    dprintf(1, ("OPPOSITE SIDE'S DEADMAN WENT OFF\n"));
	    die(CDIE_OPP_DEAD, 0);
	}
    }
#endif

}

