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

#ifndef _FIRMWARE_H
#define _FIRMWARE_H

/*
 * fw.h
 *
 * Copyright 1995, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON
 * GRAPHICS, INC.
 */

#ident "$Revision: 1.47 $"

#ifdef DEBUG
#define dprintf(lvl, x)	if (debug>=lvl) { printf x; }
#else
#define dprintf(lvl, x)
#endif	/* DEBUG */

#ifdef TRACE
#define trace(op, a0, a1, a2, a3) ltrace(op, a0, a1, a2, a3)
#else
#define trace(op, a0, a1, a2, a3)
#endif /* TRACE */

/* align x to boundary y */
#define ALIGN(x,y) ((caddr_t)(((u_long)(x)+(y)-1) & ~((y)-1)))

#define MAX_LEN_BP_PKT 16*1024+sizeof(bp_pkt_hdr_t) + sizeof(hippi_fp_t) + 8

#define ABSOLUTE(x) ((x>0) ? x : -x)

#define dma0_done() ( !(LINC_READREG( LINC_DMA_CONTROL_STATUS_0) \
	      & (LINC_DCSR_VAL_DESC_MASK | LINC_DCSR_BUSY)) )

#define dma1_done() ( !(LINC_READREG( LINC_DMA_CONTROL_STATUS_1) \
	      & (LINC_DCSR_VAL_DESC_MASK | LINC_DCSR_BUSY)) )

#define idma_done() ( !(LINC_READREG( LINC_INDIRECT_DMA_CSR ) \
	      & LINC_ICSR_BUSY))


/************************************************************
  HIPPI-FP and other protocol stuff
************************************************************/


#define		HIPPI_ULP_MI	2
#define		HIPPI_ULP_LE	4
#define		HIPPI_ULP_IPIM	6
#define		HIPPI_ULP_IPIS	7

#define		IP_PROTO_TCP	6
#define		IP_PROTO_UDP	17

/* driver requires LE/SNAP/IP to be in one mbuf */
#define 	MIN_FP_LE_SNAP_IP_LEN 60

/* ifhip parameter definitions */

/*  max D2_size in HIPPI-LE */
#define		MAX_LE_D2SIZE	(65535+8)	


/*
 * host-to-board control requests (C2B)
 */

/* XXX: longer? */
#define		MAX_OUTQ	25 


/*************************************************************
  Constants to set up basic memory structure
*************************************************************/
#define SDRAM_SIZE	0x400000

#define SDRAM_END	LINC_SDRAM_ADDR+SDRAM_SIZE

#define FIRM_PROMSTART		0xbfc00800

#define VECT_START		0x80000000
#define VECT_SIZE		0x280		/* R4650 vects in bufspace */


/* Buffer SDRAM will hold text, data, bss.
 */
#define FIRM_START		0x80000000
#define FIRM_BSS		0x80014000
#define FIRM_SIZE		0x00020000	/* 128K text+data+bss+stack */

#define FIRM_EFRAME		(LINC_SDRAM_ADDR+FIRM_SIZE)
#define FIRM_STACK		(FIRM_START+FIRM_SIZE-32) /* starting SP */

#define MAILBOXES		(FIRM_EFRAME+EFRAMESZ)

/* HCMD_BASE MUST BE ON A CACHELINE BOUNDARY */
#define HCMD_BASE		(MAILBOXES + 0x100)

/* hardcoded offset to write sign value when in assembly (lincprom boot code) */
#define HCMD_SIGN_OFFSET	(22*4)

#if (HCMD_BASE & (128-1)) != 0
#error "HCMD region not on a cache line boundary"
#endif

#define STATS_BASE		(HCMD_BASE + sizeof(struct hip_hc))
#define BPCONFIG_BASE		(STATS_BASE + sizeof(hippi_stats_t))
#define OPPOSITE_BASE		(BPCONFIG_BASE + \
				 sizeof(struct hip_bp_fw_config))
#define LINC_TRACE_BASE		(((OPPOSITE_BASE + sizeof(opposite_t)) \
				  & ~(DCACHE_LINESIZE-1)) + DCACHE_LINESIZE)
#define STATE_BASE		(LINC_TRACE_BASE + sizeof(trace_t)*LINC_TRACE_BUF_SIZE)
#define FIRM_HEAPSTART		(( (STATE_BASE + sizeof(state_t)) \
				  &  ~(DCACHE_LINESIZE-1)) + DCACHE_LINESIZE)
#define FIRM_HEAPSIZE		(SDRAM_SIZE-FIRM_HEAPSTART-LINC_SDRAM_ADDR)

#define HIPPI_PERMINFO_ADDR	0x1fc04000

#define RR_CONFIG_OFFSET	0x800
#define RR_PCI_MEM_BASE 	0
#define RR_SRAM_SIZE		1024*256

#define FW_VERSION_NUM  	0x1
#define ICACHE_LINESIZE		32
#define HOST_LINESIZE 		128
#define NBPP			16384


/* size various buffers */
/* local copies of host structures */
#define SRC_L2RR_SIZE	 256	/* must be even - actual size is one less */
#define SRC_RR2L_SIZE	 256     /* must be even - actual size is one less */

#if (SRC_L2RR_SIZE != SRC_RR2L_SIZE)
#error "SRC_L2RR_SIZE must equal SRC_RR2L_SIZE"
#endif


/* because a single block can generate up to 5 l2rr's, 
 * must size the queue large
 *  enough. For better performance should have at least two to stuff
 */
#if (SRC_L2RR_SIZE < 16)
#error "SRC_L2RR_SIZE must be at least 16"
#endif

#define DST_RR2L_SIZE	 256	/* must be even - actual size is one less */

/* data reads on channel 0, descr read/writes on channel 1 */
/* descr reads use prefetch = 0 */
/* data attr are set on the fly */
/* see init_linc in common.c for how prefetch is used */
#define SRC_RR2L_DESC_ATTR	(CPCI_ATTR_SDRAM | CPCI_ATTR_CHANNEL_1)
#define SRC_L2RR_DESC_ATTR	 (CPCI_ATTR_SDRAM | CPCI_ATTR_CHANNEL_1 \
				  | CPCI_ATTR_PFTCH_W(0))

/* data writes desc writes on channel 0, descr reads on channel 1 */
/* descr prefetch = 0 */
#define DST_L2RR_DESC_ATTR	(CPCI_ATTR_SDRAM | CPCI_ATTR_CHANNEL_1 \
				 | CPCI_ATTR_PFTCH_W(0))
#define DST_RR2L_DESC_ATTR	(CPCI_ATTR_SDRAM | CPCI_ATTR_CHANNEL_0)
#define DST_RR_DATA_ATTR	(CPCI_ATTR_SDRAM | CPCI_ATTR_CHANNEL_0)

/* flow control of insertion of b2h's (b2h_queue) is done on source, not on
 * dst. Dst is effectively flow controlled to the max number of mbufs,
 * which is far less than B2H_SIZE.
 */

#define B2H_SIZE         256


/*************************************************************
   Extra LINC stuff, support for ByteBus PAL
************************************************************ */


#define CPCI_ATTR_CHANNEL_0		0x00000000
#define CPCI_ATTR_CHANNEL_1		0x40000000

#define LINC_BBPAL_BYPASS0	0x07e00000
#define LINC_BBPAL_BYPASS1	0x07e00004
#define LINC_BBPAL_STATUS	0x07e00008
#define LINC_BBPAL_MISC		0x07e0000c
#define LINC_BBPAL_DIAG_SEL	0x07e00010

#define LINC_BB_DIAG_TL_DIAG_SEL_MASK 	0x00000038
#define LINC_BB_DIAG_RL_DIAG_SEL_MASK 	0x00000007

#define LINC_BB_MISC_DIAG_SEL_EN	0x00000020
#define LINC_BB_MISC_SIG_DET		0x00000008
#define LINC_BB_MISC_GRESET		0x00000004
#define LINC_BB_MISC_LB			0x00000002
#define LINC_BB_MISC_TDAV		0x00000001

#define LINC_BB_STATUS_BB_ADD_ERR	0x00000001

#define LINC_BB_BYPASS1_BYP_PEND	0x00000002
#define LINC_BB_BYPASS1_BYP_INT		0x00000001

#define LINC_BB_BYPASS0_BYP_PEN		0x00000002
#define LINC_BB_BYPASS0_BYP_INT		0x00000001



#define LINC_WRITEDMAREG( pi, r, v ) \
		LINC_WRITEREG( pi->dmaregs+(r), (v) )
#define LINC_READDMAREG( pi, r ) \
		LINC_READREG( pi->dmaregs+(r) )


/* PPCI address attributes (as bits in upper 32-bits of address).
 */
#define PPCIHI_ATTR_TARGETID(x)	((x)<<(60-32))
#define PPCIHI_ATTR_PREFETCH	(1<<(59-32))
#define PPCIHI_ATTR_PRECISE	(1<<(58-32))
#define PPCIHI_ATTR_VIRTUAL_REQ	(1<<(57-32))
#define PPCIHI_ATTR_BARRIER	(1<<(56-32))
#define PPCIHI_ATTR_REM_MAP(x)	((x)<<(48-32))

#define CPCI_MAX_PREFETCH       128


/*************************************************************
  Timer constants
************************************************************ */

/* Timer constants for remote debug
 */
#define DEBUG_BAUD		9600
#define DEBUG_TMR_HZ		(5*DEBUG_BAUD)
#define DEBUG_TMR_TICK		(CPU_HZ/2/DEBUG_TMR_HZ)

#define TMR_HZ          50	/* timer interrupt hz */
#define TMR_TICK	(CPU_HZ/2/TMR_HZ)

/* host control queue timer functions (d2b, c2b) */
/* units of microseconds */
#define D2B_POLL_TIMER		100
#define SRC_SLEEP_TIMER		10000
#define C2B_POLL_TIMER		100
#define DST_SLEEP_TIMER		100000

#define B2H_TIMER		100

/* deadman timers  - in units of 1/TMR_HZ seconds */
#define CHECK_RR_TIMER		1
#define PUSH_TO_OPP_TIMER	5 /* timer to push state to source */

/* timer to update state variables for opposite linc */
#define CHECK_OPP_TIMER	 	(4*PUSH_TO_OPP_TIMER) 

/*roadrunner default time out value */
#define HIPPI_DEFAULT_TIMEO	20*250000 /* roughly 5 secs */

/* mask  for number of main-loop counts before hcmd interface is checked */
#define HCMD_COUNT_MASK		0xff

/* number of main-loop counts before source checks rr2l acks */
#define PROC_ACKS_COUNT		10


/*************************************************************
  Misc Constants
************************************************************ */

/* SRC_BLK_SIZE controls how messages are divided into blocks. FW transfers
   packets >= SRC_BLK_SIZE as multiple packets with NEOP asserted.
   Note that firmware enforces a single block no matter packet length
   if checksum is valid.
*/
#define SRC_BLK_SIZE	(1024*64) 
#define DST_BLK_SIZE	(1024*64)



/*************************************************************
  LED constants
************************************************************ */

#define LED_LINK_OK	8
#define LED_SRC_PKT	2
#define LED_DST_PKT	8
#define LED_ALIVE	2

#define LED_ERROR	4
#define LED_ERR_CODE	1

/* led timing, in units of 1/TMR_HZ*/
#define LED_PKT_MIN_ON	1

#define LED_NO_INIT_TICK  	25
#define LED_ALIVE_SLOW_TICK 	50
#define LED_ALIVE_FAST_TICK 	13

/* number of times in led processing subroutine with glink dead
 * before it gets reset. Units of 1/TMR_HZ.
 */
#define GLINK_RESET_THRESHOLD 50



/*************************************************************
*************************************************************
  Beginning of C defines and structs
*************************************************************
*************************************************************/

#if defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS)
#include "hippibp.h"		/* MUST BE FIRST! - get hippibp stat struct */
#include "hippi.h"		/* get hippi stat struct */
#include "hippi_firm.h"		/* get hip_hc */
#include "hippibp_firm.h"	/* bypass driver/firmware interface structs */
#include "rrunner.h"		/* roadrunner interface definition */
#include "bypass.h"
#include "hip_errors.h"


/* define the host to board interface as d2b's or c2b's depending on 
   whether you're the source or destination 
   */
#ifdef HIPPI_SRC_FW
typedef hip_d2b_t gen_d2b_t;
#else /*HIPPI_DST_FW*/
typedef hip_c2b_t gen_d2b_t;
#endif

/* must be a multiple of a cache-line length (32 bytes) */
#define LINC_TRACE_BUF_SIZE 256

/* some standard formats:
 * DMA's:
 *	op = which dma engine
 *	arg0 = type of dma
 *	arg1 = local addr
 *	arg2 = host lo_addr
 *	arg3 = len | dma_flags
*/


typedef union {
    u_int      i[4];
    __uint64_t l[2];
    struct {
	u_int op	: 4;
#define TOP_DMA0 	1    /* dma format: type, local_addr, host_addr_lo, len */
#define TOP_DMA1 	2
#define TOP_IDMA 	3
#define TOP_DMA_B2H	4

#define TOP_MISC	5    /* format: depends */

#define TOP_WBLK 	6    /* format: flags,  avail, ulp<<24 | d2_size, dp  */
#define TOP_SBLK 	7    /* format: flags,  len,   stack<<24 | num_d2bs, fburst<<24 | dp */
#define TOP_L2RR	8    /* src format: op, len,   ifield,            addr*/
			     /* dst format: 0,  0,     desc_ring_consumer, data_ring_consumer */
#define TOP_RR2L	9    /* src format: ack_state, ack_flags, rr2l,          0 */
			     /* dst format: 0,  len,   flag,              addr */

#define TOP_HFSM_ST	10   /* src format: state,  flags<<16 | msg_flags, chunks<<16|chunks_left, cur_job | rem_len*/
			     /* dst format: state,  flags<<16 | dma_flags, rem_len<<24|cur_len, cur_sm_buf_len<<16|cur_lg_bufs */

#define TOP_WFSM_ST	11   /* src format: state,  flags, 0, 0  */
			     /* dst format: state,  rr2l.addr, rr2l.len, rr2l.flag */

#define TOP_BP_DESC	12   /* format:  job, entry_num, desc.i[3], desc.i[1]*/
#define TOP_D2B		13   /* format: type, d2bp, hdrp_lo_word, hdrp_hi_word */

#define TOP_PKT_DROP	14   /* format:  0, reason*/

	u_int time	: 28;
/* shift r4640 timer to fit */
#define TOP_TIME_SHIFT 4
	    

	u_int arg0	: 8;

/* destination qualifiers */
#define T_DMA_MISC		0

#define T_DMA_BP_DATA		1
#define T_DMA_FP_D1 		2
#define T_DMA_FP_D2  		3
#define T_DMA_FP_DESCHDR	4 
#define T_DMA_FP_DESCD2		5
#define T_DMA_BP_DESC		6
#define T_DMA_GET_FPBUFS	7
#define T_DMA_IP_SMBUF		8
#define T_DMA_IP_LGBUFS		9
#define T_DMA_D2B_DATA		10
#define T_DMA_D2B_DESC		11

#define T_D2B_HEAD		0
#define T_D2B_FPHDR		1 /* type, 0, host_addr, len */
#define T_D2B_FPBUF		2 /* type, 0, host_addr, len */

#define T_MISC_INIT   	1
#define T_MISC_INTR	2	/* intr to host */
#define T_MISC_LEDS	3	/* set_led routine was entered */
#define T_MISC_ULP	4	/* assign and deassign of ulp */
#define T_MISC_DIE	5	/* in DIE routine- a2 = major, a3 = aux_data */
#define T_TIMER_INFO	6	/* write out timer info */
#define T_TIMER_ERR	7	/* a timer error occurred */


	u_int arg1	: 24;
	u_int arg2;
	u_int arg3;
    } s;
} trace_t;

#ifdef USE_TIMERS
/* These timers should be used for light weight performance timings. Setting
 * these timers is lighter weight than traces. These macros allow you to end
 * a timing and then write out the trace at some point later on. Very little
 * error checking is done so care should be taken that the timers are started
 * and ended correctly. */

#define NUM_TIMERS 8
typedef struct hip_timer_s {
    u_int start;
    u_int end;
    u_char set;
} hip_timer_t;

extern hip_timer_t hip_timers[];

#define TIMER_START(num) if (num < NUM_TIMERS) { \
			     hip_timers[num].start = get_r4k_count() >> TOP_TIME_SHIFT;\
			     hip_timers[num].set++;}
#define TIMER_END(num) if (num < NUM_TIMERS) {\
			   hip_timers[num].end = get_r4k_count() >> TOP_TIME_SHIFT;\
			   hip_timers[num].set++;}
#define TIMER_WRITE(num) if (num < NUM_TIMERS) {\
                             if  (hip_timers[num].set%2 == 0) {\
                                  ltrace(TOP_MISC, T_TIMER_INFO, num, \
				         hip_timers[num].end - hip_timers[num].start,\
				         hip_timers[num].set);\
                                         hip_timers[num].set = 0;\
                             }\
                             else {\
				  ltrace(TOP_MISC, T_TIMER_ERR, num, \
				         hip_timers[num].end - hip_timers[num].start,\
				         hip_timers[num].set);\
                                         hip_timers[num].set = 0;\
			   }}

#else /* !USE_TIMERS */
#define TIMER_START(num)
#define TIMER_END(num)
#define TIMER_WRITE(num)
#endif

/*************************************************************
  C2B/D2B constants/structs
  State structs for queues between host and board
  and opposite state update.
*************************************************************/
 
#define D2B_DMA_LEN		4 /* default num d2b's to DMA at a time */

/* number of d2b's stored locally - sized to support a 4 MB write
 without wrapping the queue - must be a power of 2 */
#define LOCAL_D2B_LEN		(2 * (4*1024*1024/16384))

#if (LOCAL_D2B_LEN < (4*1024*1024/16384 + 8)) /* FP allows posting of 4MB  */
#error "LOCAL_D2B_LEN too small"
#endif

#if (LOCAL_D2B_LEN & (HOST_LINESIZE-1))
#error "LOCAL_D2B_LEN not host cacheline aligned in length"
#endif

/* number of c2b's stored locally */
#define LOCAL_C2B_LEN		((4*1024*1024/16384) + 8)
#if (LOCAL_C2B_LEN < (4*1024*1024/16384))
#error "LOCAL_C2B_LEN too small"
#endif

/* dead zone after d2b and c2b queue because idma rounds up in length */
#define C2B_D2B_DEAD_ZONE	64



typedef struct {
    /* size of local and host copy is the same */
  int		seqnum;
  int		queued;		/* number b2h's queued */

  hip_b2h_t	*basep;		/* base of local version */
  hip_b2h_t	*endp;		/* end of local version */
  hip_b2h_t	*put;		/* ptr to place needing push to host */

  u_int	hostp_lo;		/* pointer to host buffer */
  u_int	hostp_hi;
  u_int host_off;		/* offset to push location, bytes */
  u_int host_end;		/* offset to end, bytes */

} b2h_state_t;


typedef struct {
    u_int	flags;

#ifdef USE_MAILBOX
    u_int	poll_timeout; /* timeout value in usec */
#endif

    u_int	hostp_lo; /* ptr to base of host buffer */
    u_int	hostp_hi;
    u_int	host_off;	/* offset from base in bytes */
    u_int	host_end;	/* offset from base to end */

    gen_d2b_t	*basep; /* local versions of host struct */
    gen_d2b_t	*endp; /* local versions of host struct */
    gen_d2b_t	*get; /* local versions of host struct */

    
    gen_d2b_t	*st_in_cache;	/* start of portion in 4640 cache */
    gen_d2b_t	*end_valid;	/* d2b's accurate to here - one past end of 
				   what's in 4640 cache*/
}  d2b_state_t;



/* DMA Structures */
typedef struct {
  u_int	host_addr_lo;
  u_int	host_addr_hi;
  u_int	brd_addr;
  int		len;
  int		flags;		/* flags defined in linc.h */
} dma_cmd_t;


/* If the size of this struct changes the PEER_TO_PEER_DMA_WAR will break
   because the size is hard coded in the driver. */
typedef struct {
    u_int cnt;
    u_int flags;
#define OPP_FLAG_LNK_READY	(1<<0) /* sent by dest */
#define OPP_FLAG_SYNC		(1<<1) /* sent by dest */
#define OPP_FLAG_OH8SYNC	(1<<2) /* sent by dest */
#define OPP_SIG_DETECT		(1<<3) /* sent by source */

} opposite_t;



/*************************************************************
  Source constants/structs
*************************************************************/
 
/* max number of desc in dma command queue */
#define MAX_DMAS_ENQUEUED 4

enum src_wire_state_t {
  SW_IDLE,
  SW_NEOC,
  SW_NEOP,
  SW_ERR
};

typedef struct {
    enum src_wire_state_t 	st;
    u_int	 		flags; /* defined by SBLK flags 
					  BEGPC is now PERM_CONN */

    /* ack state is exactly the same as wire state, just delayed a little */
    enum src_wire_state_t	ack_st;
    u_int			ack_flags;

}  src_wire_t;

typedef struct {
    u_int 		st;

    u_int		flags;
#define SRR2L_VB 	(1<<0)
    volatile src_rr2l_t *base;
    
    volatile src_rr2l_t	*end;
    volatile src_rr2l_t	*get;

} srr2l_state_t;


typedef struct {
    u_int flags;
#define SL2RR_VB 	(1<<0)
#define SL2RR_PC	(1<<1)
    int queued;			/* number queued in ring */

    src_l2rr_t *base;
    src_l2rr_t *put;
    src_l2rr_t *get;
    src_l2rr_t *end;

    int total;
} sl2rr_state_t;

typedef struct {
  u_int		*dptr;		/* ptr to one past end of data */
  u_int 	len;		/* in bytes */
  u_char	d2bs;		/* number of d2bs this packet completes */
  u_char	stack;		/* FP stack index */
  u_short	flags;		/* same flags as SBLK_* */
  u_int		tail_pad;
} sl2rr_ack_t;


enum src_host_state_t { 
    SH_IDLE,			/* anyone can grab interface */
    SH_D2B_ACTIVE,		/* FP or LE owns interface, active msg */
    SH_D2B_IDLE,		/* FP owns interface, but no active msg */
    SH_D2B_FULL,		
    SH_BP_ACTIVE,		/* BP owns interface, active msg */
    SH_BP_FULL
};

/* source Host state structure */
typedef struct {
    enum src_host_state_t	st;         /* finite state machine state */
    u_int	flags;
#define SF_NEW_D2B	(1<<2)	/* possibly new d2b's to check */

    u_int 	msg_flags;	/* flags for the current (multi) block */
#define SMF_READY	(1<<0)	/* blk ready for wire fsm */
#define SMF_PENDING	(1<<1)	/* block is pending */
#define SMF_MIDDLE	(1<<2)      /* DMA done interrupt enabled */

#define SMF_ST_MASK	7	/* mask for main state flags */

#define SMF_NEED_IFIELD (1<<3)	
#define SMF_NEOP	(1<<4)
#define SMF_NEOC	(1<<5)
#define SMF_CS_VALID	(1<<8)	/* checksum processing enabled */


    short			chunks;
    short			chunks_left;

    volatile u_int		*dp_put;    /* where to put next blk */
    volatile u_int		*dp_noack;  /* no ack has been sent for data
					       dp_noack through dp_put */

    int				rem_len;    /* rem len, bytes, for prev blk */

    int				cksum_offs; /* if msb=1 (neg.), don't put chksum in pkt */

    volatile u_int 		*basep;	    /* base of data region */
    volatile u_int		*endp;
    
    int				data_M_len; /* len of data buffer in words */

    /* bypass state */
    int				cur_job;
    int 			job_vector;
    u_int			bp_ulp;
    
    /* statistics structures */
    hippi_stats_t 		*stats;
    hippibp_stats_t 		*bpstats;

    volatile freemap_t		*freemap;
    bp_job_state_t		*job;
    u_int			*hostx;
    volatile hippi_bp_desc	*sdq;
} src_host_t;


/*************************************************************
  Destination constants/structs
*************************************************************/

/*  128 bursts is our threshold to start pushing data to host */
#define DST_MAX_RDYS	128	/*  maximum ready's outstanding */
#define DST_OP_THRESHOLD 	128
#define DST_OP_RDY_BURSTS 	980 /*  init burst counter to this */


typedef struct {
    u_int 		st;
    /* see dst_rr2l_t for bit definitions */

    volatile dst_rr2l_t	*base;
    volatile dst_rr2l_t	*end;
    volatile dst_rr2l_t	*get;

} drr2l_state_t;


typedef struct {
    u_int	flags;	
#define FP_STK_ENABLED (1<<0)
#define FP_STK_HDR_VAL (1<<1)
#define FP_STK_FPBUF_VAL (1<<2)

    u_int	hdr_addr_hi;
    u_int	hdr_addr_lo;
    u_int	fpbuf_addr_hi;
    u_int	fpbuf_addr_lo;
    u_int	fpbuf_len;	/* length in bytes of c2b's of readlist */
} stk_state_t;

typedef struct {
    u_int	addr_hi;
    u_int	addr_lo;
    u_int	len;
} mbuf_t;

typedef struct {
    /* mbuf management */
    mbuf_t*			sm_put; /* put ptr to small mbufs */
    mbuf_t*			sm_get; /* get ptr to small mbufs */
    mbuf_t*			lg_put; /* put ptr to large mbufs */
    mbuf_t*			lg_get; /* get ptr to large mbufs */

    mbuf_t			*sm_mbuf_basep;
    mbuf_t			*lg_mbuf_basep;

} mbuf_state_t;

/* number of c2b's from FPBUF (a.k.a. READLIST) list to
 * dma down at a time (in bytes). 
 * Must be an integral number of 4640 cache lines.
 */

#define FPBUF_DMA_LEN	(8*16)


#if ((FPBUF_DMA_LEN % DCACHE_LINESIZE) != 0)
#error "FPBUF_DMA_LEN must be an integral multiple of cache lines"
#endif


typedef struct {
    int				num_valid; /* num fp bufs left */
    hip_c2b_t			*get; /* ptr to current buf to grab */
    hip_c2b_t			*basep;

} fpbuf_state_t;


enum dst_host_state_t {
    DH_IDLE, 
    DH_WAIT_FPHDR,		/* wait for header buf for fp */
    DH_WAIT_FPBUF,		/* wait for list of fp buffers */
    DH_FP,			/* dma'ing fp data */
    DH_LE,			/* dma'ing LE data */
    DH_BP,			/* dma'ing bypass data */
    DH_FEOP			/* DEST "find end-of-pkt" - if FP disabled
				 * in middle of packet, find the end
				 */
};

typedef struct {
    enum dst_host_state_t st;	/* state of finite state machine */
    int		flags;		/* see dwire state */

#define DF_NEOP		(1<<0)	/* not end of packet */
#define DF_HOST_KNOWS	(1<<1)	/* host knows about current pkt */
#define DF_BP_INTR_DESC (1<<2)  /* bypass pkt in process needs host intr */
#define DF_BP_DESC	(1<<3)
#define DF_NEW_D2B	(1<<4)	/* new d2b's to process */

#define DF_PENDING	(1<<5)	/* dma of block to host is pending */
#define DF_STUFFING 	(1<<6)	/* middle of a block - still stuffing dma's*/


    char	dma_flags;	/* flags to help in dma control */
/* flags on a per block basis */
#define DDMA_START_PKT 	(1<<0)
#define DDMA_CHAIN_CS	(1<<1)
#define DDMA_SAVE_CS	(1<<2)

    volatile u_int		*dp_put;    /* see src_host_t */
    volatile u_int 		*basep;	
    volatile u_int		*endp;

    int				data_M_len; /* len of data buffer in words */

    u_int 			stack;
    stk_state_t 		*stackp;

    int				total_len; /* for FP. length not sent 
					    * to host - could span
					    * multiple blocks */
    int 			rem_len; /* length carried over from
					  * prior block */
    int 			cur_len; /* remaining len of blk to dma
					  * used by both fp and le
					  */
    /* used for just le */
    int				cur_sm_buf_len;	/*rem len of blk for sm bufs */
    int				cur_lg_bufs; /* remaining lg bufs to dma */

    /* bypass state */
    int 			job_vector;
    bp_port_state_t		*cur_port;
    int				dport;
    u_int			bp_ulp;
    

    /* statistics structures */
    hippi_stats_t 		*stats;
    hippibp_stats_t 		*bpstats;

    volatile freemap_t		*freemap;
    bp_job_state_t 		*job;
    bp_port_state_t		*port;
    bp_seq_num_t		*bpseqnum;
    
    stk_state_t			fpstk[HIP_N_STACKS+1];/* +1 for invalid stack*/

    char			ulptostk[256];


} dst_host_t;

enum dst_wire_state_t {
  DW_NEOP,
  DW_NEOC,
  DW_NO_FP			/* not enough of blk to extract FP */
};

typedef struct {
  enum dst_wire_state_t 	st;
  int 				blks_sent; /* num blks sent on cur pkt */
  int				old_blks_sent; 

} dst_wire_t;

#ifdef HIPPI_SRC_FW
typedef src_host_t gen_host_t;
typedef src_wire_t gen_wire_t;
#else /* HIPPI_DST_FW */
typedef dst_host_t gen_host_t;
typedef dst_wire_t gen_wire_t;
#endif


/*************************************************************
  Block structures/constants - defines one unit of data
  transfered between host and wire finite state machines.
*************************************************************/

/* info handed to swire machine after shost is complete. */
typedef struct {
    volatile u_int	*dp;		/* pointer to data */

/* SBLK flags are used by swire fsm and by process_acks fsm */
    short	flags;		
#define SBLK_NEOP	(1<<0)	/* not end of packet */
#define SBLK_NEOC	(1<<1)	/* not end of connection */
#define SBLK_NACK	(1<<2)	/* do not ACK */
#define SBLK_BEGPC	(1<<3)	/* begin permanent connection */

#define SBLK_SOC	(1<<4)	/*  start of connection */
#define SBLK_BP		(1<<5)	/* credit stats for bypass */
#define SBLK_REM	(1<<6)	/* this part of a continguous chunk
				 * of data in SDRAM, so must
				 * explicitly cause flush of 
				 * CPCI prefetch buffer at end
				 * of roadrunner transfer
				 */

    char	dma_flags;	/* flags to help in dma control */
/* flags on a per block basis */
#define SDMA_START_PKT 	(1<<0)
#define SDMA_CHAIN_CS	(1<<1)
#define SDMA_SAVE_CS	(1<<2)
#define SDMA_INT_DONE	(1<<3)

    char	stack;		/* stack that this applies to */
    char	num_d2bs;	/* num d2b's this represents */

    char 	tail_pad;	/* padding added after data in bytes
				   (not part of pkt) */
    int		fburst;		/* length of short first burst */
    int		ifield;		/* ifield to be sent */
    int		len;		/* length of message 
				    = ifield, FP, D1, D2, pad if LE odd byte */
}   src_blk_t;


typedef struct {
    volatile u_int	*dp;
    int 	flags;

#define DBLK_NONE	(0)

#define DBLK_READY	(1<<0)
#define DBLK_EOP	(1<<1)
#define DBLK_IFIELD	(1<<2)

#define DBLK_FP_VALID	(1<<4)
#define DBLK_ADDR_VALID (1<<5)
#define DBLK_P_BIT	(1<<6)
#define DBLK_B_BIT	(1<<7)

/* these are derived from b2h bit values */
#define DBLK_ERR_PARITY	(0x01<<8)
#define DBLK_ERR_LLRC	(0x02<<8)
#define DBLK_ERR_SEQ	(0x04<<8)
#define DBLK_ERR_SYNC	(0x08<<8)
#define DBLK_ILBURST	(0x10<<8)
#define DBLK_SDIC	(0x20<<8)

#define DBLK_ERR_MASK	(0xff00)
#define DBLK_ERR_MASK_SHIFT 8

    int		bytes_needed;	/* bytes needed for rest of ifield/fp */

    u_int	ifield;
    u_int	ulp;
    int		avail;		/* in bytes */
    int		d2_size;	/* in bytes */
    int		d2_offset;	/* in bytes */
    int		d1_size;	/* in bytes */
    int 	pad;		/* in bytes */
}  wire_blk_t;


typedef struct {
    volatile u_int	*dp;
    int 	flags;		/* this is a copy of wire blk flags */

    /* if multiple blocks make up the same packet, 
     * ulp, d2_size stay the same. On the second blk
     * avail is actual bytes available, d1_size = 0, d2_offset = 0;
     */

    u_int	ulp;
    int		d2_size;	/* in bytes */
    int		avail;		/* in bytes */
    int		d2_offset;	/* in bytes */
    int		d1_size;	/* in bytes */

    int		le_sm_buf_len;	/* in bytes */
    int		le_lg_bufs;	/* number large buffers in the packet */

    int 	pad;		/* in bytes */
    u_int	bp_desc_addr_hi;
    u_int	bp_desc_addr_lo;
    int		bp_job;
    hippi_bp_desc	bpdesc;
    
} ack_blk_t;




/************************************************************
  Main State Structure
************************************************************/

typedef struct {
    /* Main state structure for interface (source or destination) */
    u_int				flags; /* various state */
#define FLAG_ACCEPT		(1<<0) /* board is accepting conn's */
#define FLAG_ENB_LE		(1<<1) /* board is accepting HIPPI-LE */
#define FLAG_HIPPI_PH		(1<<2) /* don't try to parse headers*/

#define FLAG_NEED_HOST_INTR	(1<<4) /* need to interrupt host */
#define FLAG_BLOCK_INTR		(1<<5) /* block interrupts to host */
#define FLAG_ASLEEP		(1<<6) /* board is asleep */

#define FLAG_CHECK_OPP_EN	(1<<8) /* update local state of opp. linc */
#define FLAG_CHECK_RR_EN	(1<<9) /* push state to opposite linc */
#define FLAG_PUSH_TO_OPP	(1<<10) /* push state to opposite linc */
#define FLAG_D2B_POLL_EN	(1<<11) /* enable polling of data to brd queue */

#define FLAG_GLINK_UP    	(1<<12) /* hippi glink is up */
#define FLAG_RR_UP		(1<<13) /* roadrunner is up */
#define FLAG_OPPOSITE_UP	(1<<14) /* opposite linc is up */

#define FLAG_GOT_INIT		(1<<16) /* enable intr check for opp. linc being up */ 
#define FLAG_LOOPBACK		(1<<17) /* internal loopback enabled */
#define FLAG_FATAL_ERROR	(1<<18) /* got fatal error - controls leds*/

    
    int				blksize; /* size of a blk - src units = # data d2b 
					  *                 dst units = bytes
					  */

    bp_job_state_t		*job;
    volatile freemap_t		*freemap;
    u_int			*hostx;
    hippi_bp_desc		*sdq;
    bp_port_state_t		*port;
    bp_seq_num_t		*bpseqnum;

    volatile struct hip_hc	*hcmd;
    struct hippi_stats		*stats;
    struct hip_bp_fw_config 	*bpconfig;
    struct hippibp_stats	*bpstats;

    u_int			*dma_statusp;
    u_int			zero;

    gen_host_t 			*hostp;
    gen_wire_t			*wirep;

    /* board data buffers */
    int				l2rr_len;
    int				rr2l_len;
    src_l2rr_t			*sl2rr; /* control to roadrunner */
    volatile src_rr2l_t		*srr2l; /* control from roadrunner */

    dst_l2rr_t			*dl2rr; /* control to roadrunner */
    volatile dst_rr2l_t		*drr2l; /* control from roadrunner */

    gen_d2b_t	 		*d2b;
    hip_b2h_t			*b2h;


    mbuf_t			*sm_buf;
    mbuf_t			*lg_buf;
    hip_c2b_t			*fpbuf;

    volatile rr_pci_mem_t	*rr_mem;
    volatile pci_cfg_hdr_t 	*rr_config;

    volatile u_int		*data_M; /* data buffer base*/
    volatile u_int 		*data_M_endp;
    int				data_M_len;

    volatile uint64_t		*mb; /* mailboxes */

    hippi_bp_desc 		*bp_dst_desc; /* desc to send to host */

    u_int			sleep_addr_lo;
    u_int			sleep_addr_hi;

    u_int			old_byte_cnt;

    volatile opposite_t		*opposite_st; /* opposite linc's state */
    opposite_t			*local_st; /* local linc's state */

    int				opposite_cnt;
    u_int			opposite_addr;

    /* These next 4 addresses are only used with the PEER_TO_PEER_DMA_WAR. */
    u_int			opposite_addr_hi;
    u_int			opposite_addr_lo;
    u_int			local_addr_hi;
    u_int			local_addr_lo;

    u_int			cur_link_errcnt;

    u_int			old_cmdid;
    int				nbpp; /* number bytes per page */
					  
    int				poll_timer;
    int				sleep_timer;
    int				b2h_timer;

    u_int			leds;

    caddr_t			heap; /* buf space */

    trace_t			*trace_basep;
    trace_t			**traceput;
    src_blk_t			*sblk; /* source block */
    wire_blk_t			*wblk; /* dest wire block */
    ack_blk_t			*ablk; /* dest ack block */
    
    sl2rr_state_t 		*sl2rr_state;
    sl2rr_ack_t			*sl2rr_ack;
    srr2l_state_t		*srr2l_state;
    drr2l_state_t		*drr2l_state;
    mbuf_state_t 		*mbuf_state;
    fpbuf_state_t		*fpbuf_state;

    b2h_state_t 		*b2h_state;
    d2b_state_t 		*d2b_state;

    int				max_loop_time;
    u_int			loop_timer;

} state_t;




/************************************************************
  intr.c
************************************************************/

extern void	init_intrs(void);
extern u_int	timer_ticks;


/************************************************************
  locore.s
************************************************************/

extern void	exception(void);


/************************************************************
  dma.c
************************************************************/

void dma0_flush_prefetch(void);
void dma1_flush_prefetch(void);
void dma_push_cmd0(dma_cmd_t *);
void dma_push_cmd1(dma_cmd_t *);
void sync_opposite(void);
void dmaintr(void);
int idma_write(u_int addr_hi, u_int addr_lo, 
	       u_int data_hi, u_int data_lo, u_int addr_flags);
int idma_read(u_int addr_hi, u_int addr_lo, 
	      u_int laddr, int len, u_int addr_flags);

#define HIP_IS_SRC 1
#define HIP_IS_DST 0

/*************************************************************
  errors.c 
*************************************************************/

extern void	die( int code, int auxdata );
extern void	fatal_ppci_dma_err(void);
extern void	fatal_cisr_errs(u_int linc_cisr);


/*************************************************************
  sfw.c or dfw.c 
*************************************************************/

void main_loop (gen_host_t *hostp, gen_wire_t *wirep);
void init_mbufs(void);
/*************************************************************
  common.c
*************************************************************/

void ltrace(char op, u_int arg0, u_int arg1, u_int arg2, u_int arg3);
void dump_state_to_mem(void);
u_int chksum(volatile u_int *a, int len);
int get_loop_time(void);
void store_bp_dma_status(enum dma_status_t type, 
		    int num_bufx, 
		    int base_bufx, 
		    int job);

void update_opposite_side(void);
void check_rr_alive(void);
void	init_linc(void);

void wait_usec(int i);

void go_to_sleep(void);

void set_leds(u_int timer_ticks);

void init_mem(void);

void update_slow_state(void);

int process_hcmd(gen_host_t *hostp, gen_wire_t *wirep);

void init_board(gen_host_t *hostp, gen_wire_t *wirep);

void init_roadrunner(void);

typedef enum {
    CACHED,
    UNCACHED
}    heap_malloc_type;

caddr_t heap_malloc(int size, heap_malloc_type type);

void b2h_init_mem(void);
void b2h_init_state(u_int hostp_lo, u_int hostp_hi, int len);
void intr_host(void);

int b2h_queue(hip_b2h_t *b2h);

void b2h_push(gen_host_t *hostp);


/* control to board interface (C2B). Used by the destination
   to receive buffer pools from the host 
*/

void c2b_init_mem(void);
void c2b_init_state(u_int hostp_lo, u_int hostp_hi);
hip_c2b_t  *c2b_get(int force);
#define C2B_GET_FORCE 1
#define C2B_GET_SOON  0


/* data to board command queue (D2B) - used primarily to send 
 buffers to the source engine 
*/

void d2b_init_mem(void);
void d2b_init_state(u_int hostp_lo, u_int hostp_hi, int len);
int d2b_sync_update (int num);
#ifdef USE_MAILBOX
int d2b_check(void);
#endif
gen_d2b_t * d2b_get(void);

/*************************************************************
  sbypass.c
*************************************************************/
int sched_bp_rd(src_host_t *shost, src_blk_t *blk) ;


/*************************************************************
  dbypass.c
*************************************************************/
int sched_bp_wr(dst_host_t *hostp, ack_blk_t *hdp);


/*************************************************************
  {s,d}queues.c 
*************************************************************/

/*  source roadrunner to linc command queue */
void srr2l_init_mem(void);
void srr2l_init_state(void);
src_rr2l_t srr2l_get(void);

/*  source linc to roadrunner command queue */
void sl2rr_init_mem(void);
void sl2rr_init_state(void);
int  sl2rr_put(src_blk_t *blk);
sl2rr_ack_t * sget_ack(src_l2rr_t *);


/*  destination roadrunner to linc command queue */
void drr2l_init_mem(void);
void drr2l_init_state(void);
volatile dst_rr2l_t *drr2l_get(void);
void drr2l_put_one_back(void);
void update_dl2rr_datap(volatile u_int *ip, u_int len);
void update_dl2rr_descp(volatile dst_rr2l_t *ip);


extern int debug;


#endif /* C || C++ */
#endif /*  _FIRMWARE_H */

