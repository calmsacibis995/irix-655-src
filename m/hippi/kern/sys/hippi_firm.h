/**************************************************************************
 *									  *
 * 		 Copyright (C) 1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
/*
 * hippi_firm.h
 *
 *	Header file for interface between xtalk HIPPI-Serial card driver
 *	and the 4640 firmware.
 */

#ifndef __HIPPI_FIRM_H
#define __HIPPI_FIRM_H
#ident	"$Revision: 1.31 $    $Date: 1998/01/16 23:30:23 $"
/************************************************************
 *							    *
 *        HIPPI Board/Host Interface description            *
 *							    *
 ************************************************************/


#define LINC_SDRAM_SIZE	    0x400000	/* 4MB */

/* 
 * size of mbufs used for IP stack.
 */
#define HIP_MAX_BIG	128		/* clusters in pool */
#define HIP_MAX_SML	64		/* little mbufs in pool */
#define HIP_BIG_SIZE	NBPP		/* size of big mbufs */
#define HIP_SML_SIZE	MLEN

#define IFHIP_MAX_OUTQ 	50         	/* max output queue length. */
/* max mbuf's chained on xmit */
#define IFHIP_MAX_MBUF_CHAIN	((131072/NBPP)-2)


/* PCI Definitions */
typedef struct {
    uint_t	dev_vend_id;
#define RR_PCI_VENDOR_ID 	0x120F
#define RR_PCI_DEVICE_ID 	0x0001

    uint_t	stat_cmd;
#define RR_PCISTAT_MED_TIME	01

    uint_t	cc_rev;

    union {
	uint_t i;
	struct {
	    uchar_t	bist;
	    uchar_t	hdr_type;
	    uchar_t	lat_timer;
	    uchar_t	cache_line_size;
	} s;
    } bhlc;
    
    uint_t	bar0;
    uint_t	bar1;
    uint_t	bar2;
    uint_t	bar3;
    uint_t	bar4;
    uint_t	bar5;
    uint_t	dont_care[5];

    uchar_t	max_lat;
    uchar_t	min_gnt;
    uchar_t	int_pin;
    uchar_t	int_line;

    uint_t misc_host_ctrl_reg;	    /* offset 0x40 */
    uint_t misc_local_ctrl_reg;	    /* offset 0x44 */
    uint_t prog_counter_reg;	   /* offset 0x48 */
    uint_t breakpoint_reg;	   /* offset 0x4C */
    uint_t res2;	    /* offset 0x50, reserved for timer bits 62-32 */
    uint_t timer_reg;	    /* offset 0x54, timer, incr's every 0.97 usec */
    uint_t timerref_reg;    /* offset 0x58, "timer reference", timer 
			     * event is generated when TimerReg's value
			     * matches TimerRefReg's. */

    uint_t res3;	    /* offset 0x5C */

    uint_t event_reg;	    /* offset 0x60, Event Register */
    uint_t mbox_event_reg;    /* offset 0x64 */
    uint_t win_base_reg;    /* offset 0x68 */
    uint_t win_data_reg;    /* offset 0x6C */
    uint_t recv_state_reg;  /* offset 0x70 */
    uint_t hippi_ovrhd_reg; /* offset 0x78 */

    uint_t ext_serial_reg;  /* offset 0x7C */

    /* -------- Host DMA Control registers (0x80 - 0xCF) -------- */
    uint_t dma_write_host_hi;	/* offset 0x80 */
    uint_t dma_write_host_lo;	/* offset 0x84 */
    uint_t res6[2];

    uint_t dma_read_host_hi;	/* offset 0x90 */
    uint_t dma_read_host_lo;	/* offset 0x94 */
    uint_t res8;
    uint_t dma_read_len;	/* offset 0x9C */

    uint_t dma_write_state;	/* offset 0xA0 */
    uint_t dma_write_local;	/* offset 0xA4 */
    uint_t res9;
    uint_t dma_write_len;	/* offset 0xAC */

    uint_t dma_read_state;	/* offset 0xB0 */
    uint_t dma_read_local;	/* offset 0xB4 */

    uint_t tcpchksum;		/* offset 0xB8 */
    uint_t res10;		/* offset 0xBC */

    /* -------- Local Memory Config Registers (0xC0 - 0xFF) -------- */
    uint_t local_mem_cfg_regs[16];

} pci_cfg_hdr_t;


/* Opcodes for the hip_hc cmd.
 * Range 0-19 reserved for hps_
 *       20-39 reserved for hippibp
 */
#define HCMD_NOP	0	/* do nothing */
#define HCMD_INIT	1	/* set parameters */
#define HCMD_EXEC	2	/* execute downloaded code */
#define HCMD_PARAMS	3	/* set operational flags/params */
#define HCMD_WAKEUP	4	/* card has things to do */
#define HCMD_ASGN_ULP	5	/* assign stack to ULP */
#define HCMD_DSGN_ULP	6	/* deassign stack to ULP */
#define HCMD_STATUS	7	/* update status flags */

/*
 * This struct serves as replacement for the SLEEP msg in the Challenge
 * HIPPI driver. If the 4640 is going to stop DMA-polling the host-to-board
 * queue it will DMA into this struct first to let us know where it stopped 
 * polling and send an interrupt. Driver clears the sleep flag before 
 * waking the board up.
 */
typedef struct hipfw_sleep {
    __uint32_t	index;	/* of _next_ ring element to be 
			   processed when awakened. */
    __uint32_t	flags;
#define HIPFW_FLAG_SLEEP	1
} hipfw_sleep_t;


/* 
 * This struct must be kept in sync with the struct hip_bp_hc used
 * by the bypass driver code. 
 * 
 * Additions may be freely made to the respective unions as long as 
 * the sizes fit within the place holders (stretchers?)
 *  	__uint32_t	cmd_data[16];
 * and
 *  	__uint32_t	cmd_res[16];
 *
 * !! DO NOT CHANGE ANY OF THE OTHER FIELDS!!
 */ 
/* Firmware undertakes to make this struct begin on a double-word
 * alignment.
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * firm/linc/include/hippi_sw.h has the offset to the sign word 
 * hardcoded!! Change it if you change where sign is!!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
typedef struct hip_hc {

    /* host-written/board-read portion of the communcations area
     */
    __uint32_t	cmd;                    /* one of the hip_cmd enum */
    __uint32_t	cmd_id;                 /* ID used by board to ack cmd */

    union {
	__uint32_t	cmd_data[20];	/* minimize future version problems */

	__uint32_t	    exec;	/* goto downloaded code */

	struct {
		__uint32_t  flags;	/* operational flags */
#define HIP_FLAG_ACCEPT		0x01	/* accepting connections */
#define	HIP_FLAG_IF_UP 		0x02	/* accept HIPPI-LE network traffic */
#define	HIP_FLAG_NODISC		0x04	/* no disconnect on parity/LLRC err */
#define HIP_FLAG_LOOPBACK	0x08    /* set internal loopback */
		int	timeo;		/* how long to time out */
	} params;

	/* struct init is unchanged from HIPPI for Challenge except for
	 * for the addresses necessary for peer to peer messaging. The
	 * driver will download different b2h_buf values for SRC and
	 * DST LINCs. The same values are downloaded for d2b and c2b,
	 * but only the SRC LINC will use the d2b and only the DST LINC
	 * will use the c2b.
	 */

	struct {			/* initialization parameters */
	    /* Board to host buffer: */
	    __uint32_t  b2h_buf_hi;	/* high-word of address */
	    __uint32_t  b2h_buf;	/* low-word  of address */
	    __uint32_t  b2h_len;	/* Length */
	    __uint32_t  iflags;		/* initialization flags (see above) */

	    /* Data to board (src LINC) buffer: */
	    __uint32_t  d2b_buf_hi;
	    __uint32_t  d2b_buf;
	    __uint32_t  d2b_len;	/* D2B_LEN (# of d2b's in ring) */
	    __uint32_t  host_nbpp_mlen;	/* Put (NBPP|MLEN) here */
#define HIP_INIT_MLEN_MASK   0xfff

	    /* Control to board (dst LINC) buffer: */
	    __uint32_t  c2b_buf_hi;
	    __uint32_t  c2b_buf;
	    __uint32_t	c2b_len;

	    /* Peer LINC's PCI cfg BaseAddrReg 0 */
	    __uint32_t	peer_pcibase;

	    /* These addresses are only used with the PEER_TO_PEER_DMA_WAR. */
	    /* Addresses of the message areas in the host. */
	    __uint32_t	src_msg_area_hi;
	    __uint32_t	src_msg_area_lo;	
	    __uint32_t	dst_msg_area_hi;		
	    __uint32_t	dst_msg_area_lo;

	    /* PPCI-bus address of location in host memory for 4640
	     * to write its stop-polling message. This is a pointer
	     * to a hipfw_sleep_t.
	     */
	    __uint64_t	b2h_sleep;	

	} init;
    } arg;

    /* host-read/board-written part of the communications area
     * see firm/linc/include/errors.h for definitions
     */
    volatile __uint32_t sign;		/* board signature */

    volatile __uint32_t pad1;

    volatile __uint32_t vers;		/* board/eprom version */
    volatile __uint32_t pad2;

    volatile __uint32_t inum;		/* ++ on board-to-host interrupt */
    volatile __uint32_t pad3;

    volatile __uint32_t cmd_ack;	/* ID of last completed command */
    volatile __uint32_t pad4;

    volatile union {
	__uint32_t    cmd_res[16];	/* minimize future version problems */
    } res;
} hip_hc_t;

/* 
 * Board-to-host DMA. Same struct is used for both dst2drv and 
 * src2drv rings.
 */
typedef struct hip_b2h {
	u_char	b2h_sn;		/* sequence number */
	u_char	b2h_op;		/* operation,stack */
	union {
		u_short	b2hu_s;
		struct {
			u_char	b2hi_pages;
			u_char	b2hi_words;
		} b2h_in;
		struct {
			u_char	b2hod_status;
			u_char	b2hod_n;
		} b2h_odone;
#ifdef HIPPI_BP
		struct {
			u_short	portid;
		} b2h_bp_portint;
#endif
	} b2hu;
	__uint32_t	b2h_l;
} hip_b2h_t;

#define b2h_s		b2hu.b2hu_s
#define b2h_pages	b2hu.b2h_in.b2hi_pages
#define b2h_words	b2hu.b2h_in.b2hi_words
#define b2h_ndone	b2hu.b2h_odone.b2hod_n
#define b2h_ostatus	b2hu.b2h_odone.b2hod_status

/* This is how the board stuffs info into a HIPPI-LE b2h */
#define B2H_LE_LEN(bp)		((int)( ((bp)->b2h_l >> 14) & 0xfffc))
#define B2H_LE_CKSUM(bp)	((__uint32_t)( (bp)->b2h_l & 0xffff))

/* status for ODONE msgs from src LINC */
#define B2H_OSTAT_GOOD	0
#define B2H_OSTAT_SEQ	1	/* HIPPI sequence error */
#define	B2H_OSTAT_DSIC	2	/* DSIC lost */
#define B2H_OSTAT_TIMEO	3	/* connection timed-out */
#define B2H_OSTAT_CONNLS 4	/* connection lost */
#define B2H_OSTAT_REJ	5	/* connection rejected */
#define B2H_OSTAT_SHUT	6	/* interface shut down before sent */
#define B2H_OSTAT_SPAR	7	/* source parity error (not good) */

/* These appear is short portion of b2h on a receive */
#define B2H_ISTAT_I	0x8000	/* I-field tacked on the front of header */
#define B2H_ISTAT_MORE	0x4000	/* we haven't found the end of packet yet */

/* These are in bits [30:0] of long portion of b2h on a receive if [31] = 1
 */
#define B2H_IERR_PARITY		0x00001	/* Destination parity error */
#define B2H_IERR_LLRC		0x00002	/* Destination LLRC error */
#define B2H_IERR_SEQ            0x00004 /* Destination sequence error */
#define B2H_IERR_SYNC           0x00008 /* Destination sync error */
#define B2H_IERR_ILBURST	0x00010	/* Destination illegal burst error */
#define B2H_IERR_SDIC           0x00020 /* Destination SDIC lost error */

#define B2H_IERR_FRAMING	0x00040	/* framing error during packet */
#define B2H_IERR_FLAG_ERR	0x00080	/* flag sync lost during packet */
#define B2H_IERR_LINKLOST_ERR	0x00100	/* lost linkready during pkt */
#define B2H_IERR_NO_PKT_RCV	0x00200	/* no pkt signal in connection */
#define B2H_IERR_READY		0x00400 /* data received when no readys sent  */
#define B2H_IERR_NO_BURST_RCV	0x00800	/* no burst in packet */
#define B2H_IERR_STATE		0x01000	/* illegal hippi serial state transitions 
					 * see hippi_stats_t */

#define HIP_B2H_OPMASK	0xF0
#define HIP_B2H_STMASK	0x0F		/* "stack" id-- */
#define HIP_B2H_NOP	(0<<4)
#define HIP_B2H_SLEEP	(1<<4)		/* board has run out of work */
#define HIP_B2H_ODONE	(2<<4)		/* output DMA commands finished */
#define HIP_B2H_IN	(3<<4)		/* input data is available */
#define HIP_B2H_IN_DONE	(4<<4)		/* input DMA is done */
#define HIP_B2H_BP_PORTINT	(5<<4)	/* BP interrupt for a port */

/*
 * Control-to-board msg format. For dst-side LINC only.
 */
typedef struct hip_c2b {
	u_short		c2b_param;	/* usually length */
	u_char		c2b_op;
	u_char		pad1;
	__uint32_t	pad2;
	__uint64_t	c2b_addr;
} hip_c2b_t;

#define HIP_C2B_OPMASK		0xF0
#define HIP_C2B_STMASK		0x0F
#define HIP_C2B_EMPTY   	(0*16)
#define HIP_C2B_SML		(1*16)		/* add (hdr) mbuf to pool */
#define HIP_C2B_BIG		(2*16)		/* add (data) page to pool */
#define	HIP_C2B_WRAP    	(3*16)		/* back to start of buffer */
#define HIP_C2B_READ		(4*16)		/* post large read to ULP */



/* Per device variables for HIPPI-FP interface.
 */

#define HIPPIFP_MAX_WRITESIZE	(16*1024*1024)	/* 2 Megabytes */
#define HIPPIFP_MAX_READSIZE	(16*1024*1024)	/* 2 Megabyte */

/* Number of pages of c2b's to post largest possible read() */
#define C2B_RDLISTPGS	(((HIPPIFP_MAX_READSIZE/NBPP+3) * \
			sizeof(hip_c2b_t) + NBPP-1)/ NBPP)




/*
 * Data to board msg format:
 */
typedef union hip_d2b {
	/* "Chunk" format: */
	struct hip_d2b_sg {
		u_short		len;		/* length of chunk */
		u_short		pad1;
		__uint32_t	pad2;
		__uint64_t	addr;
	} sg;

	/* "Head" format: */
	struct hip_d2b_hd {
		u_short	chunks;		/* number of chunks */
		u_char flags;		/* start-of-request flag */
		u_char stk;		/* ulp stack */
		u_short	fburst;		/* size of first burst */
		u_short sumoff;		/* merge checksum to this offset */
	} hd;
} hip_d2b_t;

/* flags in header */
#define HIP_D2B_RDY	0x80		/* in first hd.start of DMA string */
#define HIP_D2B_BAD	0xc0		/* start of un-ready DMA string */
#define HIP_D2B_IFLD	0x20		/* I-field is 1st word in 1st chunk */
#define HIP_D2B_NEOC	0x10		/* don't drop connection at end */
#define HIP_D2B_NEOP	0x08		/* don't drop packet line at end */
#define HIP_D2B_NACK	0x04		/* don't acknowledge transmit */
#define HIP_D2B_BEGPC	0x02		/* tell RR to begin PermConn Mode */


/* mailbox to write when a D2B queue should be read by the firmware */
#define HIP_SRC_D2B_RDY_MBOX		0x1
#define HIP_DST_D2B_RDY_MBOX		0x1

/* HIPPI DMA "stack" numbers.
 */
#define HIP_STACK_LE		0	/* ifnet TCP/IP interface */
#define HIP_STACK_IPI3		1	/* IPI-3 command set driver */
#define HIP_STACK_RAW		2	/* HIPPI-PH interface */
#define HIP_STACK_ST		3	/* HIPPI-ST ULP device interface */
				        /* a.k.a schedule transfers */
				        /* a.k.a hippi-bypass */
#define HIP_STACK_FP		4	/* HIPPI-FP ULP device interface */
#define HIP_N_STACKS		16


/* Maximum number of ulp's open simultaneously
 */
#define	HIPPIFP_MAX_OPEN_ULPS	8


#endif /* __HIPPI_FIRM_H */

