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
 * dma.c
 *
 * $Revision: 1.27 $
 */

#include <sys/types.h>
#include <sys/errno.h>

#include "r4650.h"
#include "sys/PCI/linc.h"
#include "hippi_sw.h"
#include "rdbg.h"
#include "lincutil.h"

extern state_t   	*State;

#ifdef PEER_TO_PEER_DMA_WAR
void update_local_side(void);
#endif

/**********************************************************************
  dmaintr - should never get one
**********************************************************************/

void
dmaintr(void) {

#ifndef NO_BREAKPOINTS
    breakpoint();
#endif

    assert(0);
}


/**********************************************************************
  dma_push_cmd0
**********************************************************************/

void
dma_push_cmd0(dma_cmd_t *cmd) {

    dprintf(1, ("dma_push_cmd0: h_add=0x%x,%x, loc=0x%x, len/fl = 0x%x\n",
		cmd->host_addr_hi, cmd->host_addr_lo, cmd->brd_addr, 
		cmd->flags | cmd->len));

    if (cmd->flags & LINC_DTC_TO_PARENT) { /* a write to host */

	if (cmd->brd_addr + cmd->len > K1_TO_PHYS((u_int)State->hostp->endp)) { 
	    /* buffer would wrap */
	    int save_cs = cmd->flags & LINC_DTC_SAVE_CS;
	    int first_dma = (u_int)K1_TO_PHYS(State->hostp->endp) 
		                    - cmd->brd_addr;

	    cmd->flags &= ~LINC_DTC_SAVE_CS;

	    trace(TOP_DMA0,T_DMA_MISC,
		  cmd->brd_addr, cmd->host_addr_lo, cmd->flags | first_dma);
	    LINC_WRITEREG( LINC_DMA_HIGH_PPCI_ADDR_0, cmd->host_addr_hi);
	    LINC_WRITEREG( LINC_DMA_LOW_PPCI_ADDR_0, cmd->host_addr_lo );
	    LINC_WRITEREG( LINC_DMA_BUFMEM_ADDR_0, cmd->brd_addr );
	    LINC_WRITEREG( LINC_DMA_TRANSFER_CONTROL_0, cmd->flags | first_dma);

	    cmd->host_addr_lo += first_dma;
	    cmd->brd_addr = (u_int)K1_TO_PHYS(State->hostp->basep);
	    cmd->len -= first_dma;
	    cmd->flags |= LINC_DTC_CHAIN_CS;
	    

#ifdef BRIDGE_B_WAR
	    if(cmd->host_addr_lo & 0x4)	{
		/* odd host address - must clean up host address
		 * by sending odd word as D32
		 */

		/* make sure don't overflow dma fifo - wait for room for 2*/
		while(LINC_DCSR_VAL_DESC_R(LINC_READREG(
						  LINC_DMA_CONTROL_STATUS_0)) > 2)
		    continue;

		trace(TOP_DMA0,T_DMA_MISC, 
		      State->zero, 0x3fff0000, 8 | cmd->flags);

		/* send a D32 bit to clear the linc state */
		LINC_WRITEREG( LINC_DMA_HIGH_PPCI_ADDR_0, 0);
		LINC_WRITEREG( LINC_DMA_LOW_PPCI_ADDR_0, 0x3fff0000);
		LINC_WRITEREG( LINC_DMA_BUFMEM_ADDR_0, State->zero);
		LINC_WRITEREG( LINC_DMA_TRANSFER_CONTROL_0, 8 | cmd->flags);

		if((cmd->len == 4) && save_cs)
		    cmd->flags |= LINC_DTC_SAVE_CS;

		trace(TOP_DMA0,T_DMA_MISC,
		      cmd->brd_addr, cmd->host_addr_lo, LINC_DTC_TO_PARENT | 4);
		/* send a single word to get host address back long word aligned */
		LINC_WRITEREG( LINC_DMA_HIGH_PPCI_ADDR_0, cmd->host_addr_hi);
		LINC_WRITEREG( LINC_DMA_LOW_PPCI_ADDR_0, cmd->host_addr_lo );
		LINC_WRITEREG( LINC_DMA_BUFMEM_ADDR_0, cmd->brd_addr );
		LINC_WRITEREG( LINC_DMA_TRANSFER_CONTROL_0, 4 | cmd->flags);

		cmd->len -= 4;
		cmd->brd_addr += 4;
		cmd->host_addr_lo += 4;

		/* this routine must exit w/ max 2 dma's pushed on queue */
		while( LINC_DCSR_VAL_DESC_R(LINC_READREG(
						  LINC_DMA_CONTROL_STATUS_0)) > 1) {
		    continue;
		}
		
		if (cmd->len == 0)
		    return;
		
	    }
#endif
	    if (save_cs)
		cmd->flags |= LINC_DTC_SAVE_CS;
	    
	    dprintf(1, ("fpbuf sent in two dma's 1=%d, 2=%d\n",
			first_dma, cmd->len));
	}
    }

    LINC_WRITEREG( LINC_DMA_HIGH_PPCI_ADDR_0, cmd->host_addr_hi);
    LINC_WRITEREG( LINC_DMA_LOW_PPCI_ADDR_0, cmd->host_addr_lo );
    LINC_WRITEREG( LINC_DMA_BUFMEM_ADDR_0, cmd->brd_addr );
    LINC_WRITEREG( LINC_DMA_TRANSFER_CONTROL_0, cmd->flags | cmd->len );

}


/**********************************************************************
  dma_push_cmd1
**********************************************************************/

void
dma_push_cmd1(dma_cmd_t *cmd) {
    u_int flags;

    if ( !(cmd->flags & LINC_DTC_TO_PARENT)) 
	dprintf(1, ("dma_push_cmd1: h_add=0x%x,%x, loc=0x%x, len/fl = 0x%x\n",
		    cmd->host_addr_hi, cmd->host_addr_lo, cmd->brd_addr, 
		    cmd->flags | cmd->len));

    LINC_WRITEREG( LINC_DMA_HIGH_PPCI_ADDR_1, 
		  (u_int)cmd->host_addr_hi);
    LINC_WRITEREG( LINC_DMA_LOW_PPCI_ADDR_1, 
		  (u_int)(cmd->host_addr_lo) );
    LINC_WRITEREG( LINC_DMA_BUFMEM_ADDR_1, cmd->brd_addr );

    LINC_WRITEREG( LINC_DMA_TRANSFER_CONTROL_1, cmd->flags | cmd->len );
}


/**********************************************************************
  dma0_flush_prefetch
**********************************************************************/

void
dma0_flush_prefetch(void) {

    dprintf(1, ("dma_flush_prefetch\n"));

    trace(TOP_DMA0, T_DMA_MISC, 
	  State->zero,
	  0x3fff0000,
	  8 | LINC_DTC_TO_PARENT | LINC_DTC_D64 | LINC_DTC_CHAIN_CS);
    
    LINC_WRITEREG( LINC_DMA_HIGH_PPCI_ADDR_0, 0);
		  
    LINC_WRITEREG( LINC_DMA_LOW_PPCI_ADDR_0, 0x3fff0000);
    LINC_WRITEREG( LINC_DMA_BUFMEM_ADDR_0, State->zero);

    LINC_WRITEREG( LINC_DMA_TRANSFER_CONTROL_0, 
		  8 | LINC_DTC_TO_PARENT | LINC_DTC_D64 | LINC_DTC_CHAIN_CS);
}


/**********************************************************************
  dma1_flush_prefetch
**********************************************************************/

void
dma1_flush_prefetch(void) {

    trace(TOP_DMA1, T_DMA_MISC, 
	  State->zero,
	  0x3fff0000,
	  8 | LINC_DTC_TO_PARENT | LINC_DTC_D64 | LINC_DTC_CHAIN_CS);
    
    LINC_WRITEREG( LINC_DMA_HIGH_PPCI_ADDR_1, 0);
		  
    LINC_WRITEREG( LINC_DMA_LOW_PPCI_ADDR_1, 0x3fff0000);
    LINC_WRITEREG( LINC_DMA_BUFMEM_ADDR_1, State->zero);

    LINC_WRITEREG( LINC_DMA_TRANSFER_CONTROL_1, 
		  8 | LINC_DTC_TO_PARENT | LINC_DTC_D64 | LINC_DTC_CHAIN_CS);
}


/**********************************************************************
  sync_opposite
**********************************************************************/

void
sync_opposite(void) {
    int i;
    /* sync with opposite linc */

    /*first wait to try to avoid a race condition with opposite */
    for(i = 0; i < 1000000; i++)
      ;
    
    /* update opposite side */
    LINC_WRITEREG( LINC_INDIRECT_DMA_WRITE_DATA_HIGH, State->local_st->cnt);
    LINC_WRITEREG(  LINC_INDIRECT_DMA_WRITE_DATA_LOW, State->local_st->flags);

    LINC_WRITEREG( LINC_INDIRECT_DMA_CSR,
		  LINC_ICSR_TO_PARENT );

#ifdef PEER_TO_PEER_DMA_WAR
    LINC_WRITEREG( LINC_INDIRECT_DMA_HIGH_ADDR, State->opposite_addr_hi);
    LINC_WRITEREG( LINC_INDIRECT_DMA_LOW_ADDR, State->opposite_addr_lo);
#else
    LINC_WRITEREG( LINC_INDIRECT_DMA_HIGH_ADDR, 0 );
    LINC_WRITEREG( LINC_INDIRECT_DMA_LOW_ADDR, State->opposite_addr);
#endif

    /* wait for opposite to sync */
    i = 100000;
    while((State->opposite_st->cnt == State->opposite_cnt) && i > 0) {
        i--;
#ifdef PEER_TO_PEER_DMA_WAR
	if (i%100 == 0)
	    update_local_side();
#endif
    }
    if(i == 0) {
        dprintf(1, ("sync_opposite: SYNC FAILED\n"));
        breakpoint();
    }

}


/**********************************************************************
  idma_write
**********************************************************************/

int 
idma_write(u_int addr_hi, u_int addr_lo, 
	   u_int data_hi, u_int data_lo, u_int addr_flags) {
    u_int icsr;

    ASSERT(!((LINC_READREG(LINC_INDIRECT_DMA_CSR)&LINC_ICSR_BUSY)));

    LINC_WRITEREG( LINC_INDIRECT_DMA_WRITE_DATA_HIGH, data_hi);
    LINC_WRITEREG(  LINC_INDIRECT_DMA_WRITE_DATA_LOW, data_lo);

    LINC_WRITEREG( LINC_INDIRECT_DMA_CSR,
		   LINC_ICSR_USE_D64 |
		  LINC_ICSR_TO_PARENT );
    LINC_WRITEREG( LINC_INDIRECT_DMA_HIGH_ADDR,
		  addr_hi | addr_flags);
    LINC_WRITEREG( LINC_INDIRECT_DMA_LOW_ADDR, addr_lo); /* GO ! */

}


/**********************************************************************
  idma_read
**********************************************************************/

int 
idma_read(u_int addr_hi, u_int addr_lo, 
	  u_int laddr,
	  int len,
	  u_int addr_flags) {
    u_int rd_size;
    int i = 0;

    switch(len) {
      case 4:			/* length in bytes */
	rd_size = 0;
	break;
      case 8:
	rd_size = 1;
	break;
      case 64:
	rd_size = 2;
	break;
      case 128:
	rd_size = 3;
	break;
      default:
	assert(0);
    }

    ASSERT( !(LINC_READREG(LINC_INDIRECT_DMA_CSR) & LINC_ICSR_BUSY));
    ASSERT(len == 4 | len == 8 | len == 64 | len == 128);
    ASSERT((addr_lo & 0x7) == 0);
    

    LINC_WRITEREG( LINC_INDIRECT_DMA_CSR,
		  ((rd_size == 0) ?  0 : LINC_ICSR_USE_D64) |
		  LINC_ICSR_RD_CMD_READ |
		  LINC_ICSR_PPCI_REQ |
		  LINC_ICSR_RD_SIZE_W(rd_size));
	

    LINC_WRITEREG( LINC_INDIRECT_RD_DATA_POINTER, (u_int)laddr);

    LINC_WRITEREG( LINC_INDIRECT_DMA_HIGH_ADDR,
		  addr_hi | addr_flags );

    dprintf(10, ("idma_read: host: 0x%x,%x local: 0x%x, len 0x%x\n",
		addr_hi,
		addr_lo,
		laddr,
		rd_size));
    
    LINC_WRITEREG( LINC_INDIRECT_DMA_LOW_ADDR, addr_lo);

#ifdef BRIDGE_B_WAR
    while( !idma_done()) 	/* wait for idle */
	assert(i++ < 10000000);

    LINC_WRITEREG( LINC_INDIRECT_DMA_CSR,
		  LINC_ICSR_USE_D64 |
		  LINC_ICSR_PPCI_REQ |
		  LINC_ICSR_TO_PARENT);
    
    LINC_WRITEREG( LINC_INDIRECT_DMA_WRITE_DATA_HIGH, 0);
    LINC_WRITEREG(  LINC_INDIRECT_DMA_WRITE_DATA_LOW, 0);

    LINC_WRITEREG( LINC_INDIRECT_DMA_HIGH_ADDR, 0);
		  
    LINC_WRITEREG( LINC_INDIRECT_DMA_LOW_ADDR, 0x3fff0000);
#endif

}


#ifdef PEER_TO_PEER_DMA_WAR
/********************************************************************
 * update_local_side
 * This function brings down the other side's info from the host mem.
 * This is only necessary if peer to peer dma is not functioning. The
 * naming is rather strange. This updates the local copy of the state
 * for the opposite side.
 *
 ********************************************************************/

void
update_local_side(void) {
    int i = 0;

    idma_read(State->local_addr_hi,
	      State->local_addr_lo,
	      (u_int)State->opposite_st,
	      sizeof(opposite_t),
	      0);

    while( !idma_done()) 	/* wait for idle */
	assert(i++ < 10000000);
}
#endif /* PEER_TO_PEER_DMA_WAR */


/**********************************************************************
  update_opposite_side
**********************************************************************/

void
update_opposite_side(void) {
    int i = 0;
    
#ifdef PEER_TO_PEER_DMA_WAR
    /* if we're not using peer to peer dma push local info out to host
     * and pull down opposite side info 
     */

    idma_write(State->opposite_addr_hi,
	       State->opposite_addr_lo,
	       State->local_st->cnt, 
	       State->local_st->flags,
	       0);
#else /* !PEER_TO_PEER_DMA_WAR */
    idma_write(0,
	       State->opposite_addr,
	       State->local_st->cnt, 
	       State->local_st->flags,
	       0);
#endif

    State->local_st->cnt++;

    while( !idma_done()) 	/* wait for idle */
	assert(i++ < 10000000);

#ifdef PEER_TO_PEER_DMA_WAR
    update_local_side();
#endif
}






