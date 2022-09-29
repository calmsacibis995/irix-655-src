/*
 * Copyright 1995,1996 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
 * the contents of this file may not be disclosed to third parties, copied or
 * duplicated in any form, in whole or in part, without the prior written
 * permission of Silicon Graphics, Inc.
 *
 * RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software clause at DFARS 252.227-7013, and/or in similar or
 * successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
 * rights reserved under the Copyright Laws of the United States.
 */

/*
 * $Revision: 1.20 $	$Date: 1997/06/03 07:42:39 $
 *
 * $Log: rrunner.h,v $
 * Revision 1.20  1997/06/03 07:42:39  jimp
 * support for xmit_retry counter in "hipcntl status"
 *
 * Revision 1.19  1997/03/17  15:29:25  jimp
 * v1.60 - add copyright statement
 *
 * Revision 1.18  1997/03/10  05:31:39  jimp
 * v1.40 Beta 3
 *
 * Revision 1.17  1997/03/03  15:19:55  jimp
 * v1.30
 *
 * Revision 1.16  1997/02/19  06:51:58  jimp
 * Beta1.0 - v1.20 firmware
 *
 * Revision 1.15  1997/02/10  06:13:38  jimp
 * support for mbox change of dest rr accept/reject
 *
 * Revision 1.13  1997/01/31  16:04:50  jimp
 * added define in recv_state_reg for HALF_DUPLEX
 *
 * Revision 1.12  1997/01/19  23:35:40  jimp
 * support for unified rrunner.h file
 *
 * Revision 1.9  1996/12/17  23:56:42  jimp
 * new HIPPI-SERIAL status flags, dump of 4640 cached state, misc bug fixes, support for SDQHEAD ioctl
 *
 * Revision 1.8  1996/10/24  17:43:56  jimp
 * Added some comments
 *
 * Revision 1.5  1996/09/22  18:34:07  jimp
 * fixes misc bugs - sent gigabytes of data - unknown corruption issue -might be rr
 *
 * Revision 1.4  1996/09/16  07:20:14  jimp
 * implements send/rcv, deadman timers, and LED's
 *
 * Revision 1.1  1996/06/04  17:54:52  irene
 * Added LINC_PCI_MEM_SIZE define.
 *
 * Revision 1.0  1996/05/14  19:02:45  irene
 * No Message Supplied
 *
 * Revision 1.0  1996/05/08  23:39:59  irene
 * No Message Supplied
 *
 *
 */
#ifndef _ROAD_RUNNER_H
#define _ROAD_RUNNER_H
/*
 * This file defines characteristices of Essesntial Communications 
 * Inc's RoadRunner chip: address offsets of various registers,
 * interpretations of the bits in these registers, etc, as defined
 * in Esseential's "Roadrunner PCI/HIPPI Controller" specification.
 *
 *  Access from the host is constrained to PCI Config (256 bytes) 
 *  and Memory (4KB) Address Spaces (i.e. no support for I/O Address 
 *  Space).
 *
 *  PCI Config Space
 *  ----------------
 *  A 256 byte window is provided into this space. The first 64 bytes
 *  are standard, as defined by the PCI spec. The use of the rest of
 *  the space is not defined in the Roadrunner spec.
 *
 *  Note: These 64 bytes are also accessible through the first 64
 *  bytes of PCI Memory space, see below. And Config space registers
 *  which are defined as being read-only by the PCI spec are read-write
 *  when accessed through the shared PCI Memory window.
 *
 *  PCI Memory Space
 *  ----------------
 *  A 4KB window mappable from the host. This window is broken
 *  down as follows:
 *  
 *  000 - 03F (64B)	PCI Configuration registers (R/W access to all)
 *  040 - 07F (64B)	RR's "General Control" registers
 *  080 - 0BF (64B)	RR's "Host DMA Control" registers
 *  0C0 - 0FF (64B)	RR's "Local Memory Configuration" registers
 *  100 - 13F (64B)	RR's Host FIFO access (mostly for diag use)
 *  140 - 17F (64B)	RR's "Host DMA Assist" registers
 *  180 - 1FF (128B)	The 32 internal CPU registers (r0 through r31)
 *  200 - 3FF (512B)	The "General Communications" Area. The 1st
 *			256 bytes consist of 32 "mailboxes", which,
 *		        when written by the host, causes a (maskable)
 *			mailbox event to be triggered to the RR CPU.
 *			Writing the 1st mailbox also causes a clearing
 *			of the PCI interrupt. This gen. comm. area is
 *			actually the 1st 512 bytes of the SRAM.
 *  400 - 7FF (1KB)	reserved
 *  800 - FFF (2KB)	a moveable window into the RR's SRAM. This
 *			window is moved by writing the base of the
 *			desired target range into the "Window Base
 *			Address" (one of the "General Control" 
 *			registers) above.
 */

#define LINC_PCI_MEM_SIZE   0x08000000	/* 128 MB */

/* Roadrunner PCI Definitions */

#define RR_PCISTAT_MED_TIME	01
#define RR_BIST_POWER_UP  	1
#define RR_BIST_BASIC_MEM 	2
#define RR_BIST_MEM_64K   	3

#define RR_MAX_SRAM	0x1FFFFF    /* 2MB */

/* This struct describes the 4KB PCI memory space */
#define RR_PCI_MEM_SIZE		4096
typedef struct {
    /* -------- PCI Config Registers (0 - 0x3F) -------------------- */
    uint_t cfg_regs[16];

    /* -------- General Control Registers (0x40 - 0x7F) --------------- */

    uint_t misc_host_ctrl_reg;	    /* offset 0x40 */
#define INVAL_OPCODE	    0x0800  /* R/O, guess I screwed up */
#define CPU_PARITY_ERR	    0x0400  /* R/O, p. err reading SRAM, halted */
#define	RR_HALT_INSTR	    0x0200  /* R/O, halt instr exec-ed */
#define	RR_HALTED	    0x0100  /* R/O, rr halted */
#define	RR_SINGLESTEP	    0x0020  /* R/W, write this to single step */
#define RR_HALT		    0x0010  /* R/W, write this to halt the cpu */
#define RR_HARDRESET	    0x0008  /* R/W, force h/w reset, equivalent
				       to PCI reset signal */
#define ENABLE_SWAP	    0x0004  /* R/W, enable endian swap */
#define CLEAR_PCI_INTR	    0x0002  /* W/O, clear PCI IntA. */
#define INTR_STATE	    0x0001  /* R/O, state of IntA pin */

    uint_t misc_local_ctrl_reg;	    /* offset 0x44 */
#define RR_EN_DCACHE	    0x2000 /* R/W, enable data cache */
#define RR_EN_EPROMWR	    0x1000 /* R/W, enable writes to EPROM */
#define RR_FORCE_DMA_PERR   0x0800 /* R/W, force parity error on read DMA */
#define RR_EN_PAR_CHK       0x0400 /* R/W, enable parity checking when
				      reading from SRAM */
#define RR_ADDL_DESC	    0x0200 /* R/W, double the number of descriptors,
				      to max of 256 receive, 128 xmit */
#define RR_ADDL_SRAM	    0x0100 /* R/W, double SRAM, from 1MB to 2MB */
#define RR_DMA_BURST_MASK   0x00E0 /* R/W, encoded bits force termination of
				    * PCI ops at any of the foll. boundaries:
				    * disable/4/16/32/64/128/256/1KB */
#define RR_FAST_EPROM	    0x0008 /* R/W, EEPROM region to use fast 
				    * 1-cycle access */
#define RR_SET_INTR	    0x0004 /* W/O, write this bit to set PCI IntA */
#define RR_CLR_INTR	    0x0002 /* W/O, write this bit to clear PCI IntA */
#define RR_SRAM_ACCESS	    (RR_EN_PAR_CHK|RR_ADDL_SRAM)
				   /* Turn off these bits to get to EPROM */

    uint_t prog_counter_reg;	   /* offset 0x48 */

    uint_t breakpoint_reg;	   /* offset 0x4C */
#define DISABLE_BP  1

    uint_t res2;	    /* offset 0x50, reserved for timer bits 62-32 */
    uint_t timer_reg;	    /* offset 0x54, timer, incr's every 0.97 usec */

    uint_t timerref_reg;    /* offset 0x58, "timer reference", timer 
			     * event is generated when TimerReg's value
			     * matches TimerRefReg's. */

    uint_t pci_state_reg;    /* offset 0x5C */

    uint_t event_reg;	    /* offset 0x60, Event Register */
#define RREVT_TIMER		0x40000000
#define RREVT_XMT_ATTN		0x10000000
#define RREVT_RCV_ATTN		0x04000000
#define RREVT_MBOX		0x00800000
#define RREVT_DMAREADERR	0x00080000
#define RREVT_DMAWRITEERR	0x00040000
#define RREVT_DMAREADDONE	0x00020000
#define RREVT_DMAWRITEDONE	0x00010000
#define RREVT_ASST_RDHIDONE	0x00004000
#define RREVT_ASST_WRHIDONE	0x00002000
#define RREVT_ASST_RDLODONE	0x00001000
#define RREVT_ASST_WRLODONE	0x00000800
#define RREVT_RCV_STARTED	0x00000040
#define RREVT_RCV_COMPLETE	0x00000020
#define RREVT_XMT_COMPLETE	0x00000004

    uint_t mbox_event_reg;    /* offset 0x64 */

    /* Window base and data provides access to NIC SRAM memory. 
     * Set win_base_reg to point to the base of memory are you wish
     * to access, then go in through the window pointer (last 2KB
     * of this 4KB shared PCI Memory address space). If you only wish
     * to read/write the 1st word in the window, you can also get to
     * it by reading/writing win_data_reg.
     */
    uint_t win_base_reg;    /* offset 0x68 */
    uint_t win_data_reg;    /* offset 0x6C */
#define RR_MEMWIN_SIZE	512	/* size in words */

    /* RoadRunner HIPPI Receive State Register controls and 
     * monitors the HIPPI receive interface in the NIC. Look at err
     * bits when a HIPPI receive Error Event occurs. 
     */
    uint_t recv_state_reg;  /* offset 0x70 */
#define ENABLENEWCONN		(1<<0)
#define RESETRECVINTERF		(1<<1)
#define RR_RCV_HALF_DUPLEX	(1<<4)  
#define RR_RCV_LNK_READY	(1<<16) /* link ready is rec'd from glinks */
#define RR_RCV_REQUEST		(1<<17) /* request is being received */
#define RR_RCV_CONN		(1<<18) /* connect is asserted */
#define RR_RCV_PKT		(1<<19) /* packet is being received */
#define RR_RCV_BURST		(1<<20) /* burst is being received */

#define RR_RCV_FLAG_SYNC	(1<<22) /* alternating flag is synched */
#define RR_RCV_GLINK_ERR	(1<<24) /* glink frame or no data error */
#define RR_RCV_FLAG_ERR		(1<<25) /* glink alt flag lost synch */


    /* Bits 7-5 (0xE0) contain encoded values which say must have
     * this much receive buffer space to accept a connection, even
     * if ENABLENEWCONN bit is set:
     * 	0	- accept all packets
     * 	0x20	- 1K
     * 	0x40	- 2K
     * 	0x60	- 4K
     * 	0x80	- 8K
     * 	0xa0	- 16K
     * 	0xc0	- 32K
     * 	0xe0	- 64K
     */

    /* RoadRunner HIPPI Transmit State Register controls and monitors the 
     * HIPPI transmit interface in the NIC.
     */
    uint_t xmit_state_reg;  /* offset 0x74 */
#define ENABLEXMIT	    0x000001
#define PERMCONN	    0x000002 /* keep conns active nothing to send */
#define RR_XMIT_BURST	    0x100000 /* burst is asserted */
#define RR_XMIT_PKT	    0x080000 /* packet is asserted */
#define RR_XMIT_CONN        0x040000 /* connection being received */
#define RR_XMIT_REQUEST     0x020000 /* request is asserted */

    uint_t hippi_ovrhd_reg; /* offset 0x78 */
#define RR_OVRHD_OH8_SYNC   0x1	     /* receive synched to OH8 */

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

    /* -------- DMA FIFO Access (0x100 - 0x13F) -------- */
    uint_t dma_fifo_access[16];

    /* -------- DMA Assist Registers (0x140 - 0x17F) ---------*/
    uint_t dma_assist_regs[16];
    
    /* -------- RR GPRs (0x180 - 0x1FF) -------- */
    uint_t rr_gpr[32];
    
    /* -------- General Communications Area (0x200-0x3FF) ------------ */
    
    union {
	uint_t i[128];
	
	struct {
	    union {
		/* mailbox 31 is glink status/rr alive */
		struct {
		    uint_t	int_clr;
		    uint_t	timeo;
		    uint_t	PH_on; /* 0 = FP headers, 1 = PH mode (no FP header) */
		    uint_t 	rsvd[26];
		    uint_t	retry_count;
		    uint_t	panic_code;
		    uint_t	status;
#define RR_STATUS_DEAD         0
#define RR_STATUS_INIT_SUCCESS 1
#define RR_STATUS_GLINK_DOWN   2
#define RR_STATUS_ACCEPT_CONN  3	
#define RR_STATUS_REJECT_CONN  4
		} s; /* source */
		struct {
		    uint_t	int_clr;
		    uint_t	timeo;
		    uint_t	PH_on; /* 0 = FP headers, 1 = PH mode (no FP header) */
		    uint_t 	rsvd[27];
		    uint_t	panic_code;
		    uint_t	status;
		} d; /* destination */
		uint_t		mailbox[32];
	    } mb;
	    /* -------- Base Addresses of Queues (0x290-0x2a4) ------------ */
	    
	    uint_t 		l2rr;/* wrap done using "wrap next" bit */
	    uint_t		rr2l;
	    uint_t		rr2l_end;
	    
	    uint_t		b_data_buff;	  /* ignored by source rr */
	    uint_t		b_data_buff_end; /* ignored by source rr */
	    uint_t		ring_consumers;  /* ignored by source rr */
	    uint_t		misc[26];
	    union	{
		uint_t		i[32];	/* reserved for future cmd data */
	    } cmd;
	    union {
		uint_t		i[32];	/* reserved for future cmd ack */
	    } ack_data;
	} cmd;
    } rr_gca;
    
    /* --- Don't touch this! It will send the system into deep freeze -- */
    uint_t  rr_reserved[256];

    /* Last 2K bytes is window into SRAM, starting at location
     * indicated by contents of win_base_reg */
    uint_t rr_sram_window[512];

} rr_pci_mem_t;

/* RR DMA Assist descriptor */
typedef struct {
    uint_t  da_hostaddr;
    uint_t  da_localaddr;
    uint_t  da_len;
    uint_t  da_state;
    uint_t  resvd;
    uint_t  chksum_res;
    uint_t  fw1;
    uint_t  fw2;
} rr_dadesc_t;



typedef struct {
  __uint32_t	op_len;
#define		SRRD_MB	(1<<31)	/* more bit */
#define		SRRD_DD	(1<<30)	/* Dummy Descriptor */
#define		SRRD_WR	(1<<29)	/* must be 0 */
#define		SRRD_VB	(1<<28)	/* Validity Bit. Bit alternates on and off */

#define		SRRD_SI	(1<<27)	/* 1 => Same I-field as previous descriptor */
#define		SRRD_PC	(1<<26)	/* Permanent Connection */
#define		SRRD_WN	(1<<25)	/* Wrap Next */
#define		SRRD_CC	(1<<24)	/* Continue Connection */

#define 	SRRD_LEN_MASK 0x1ffff

  __uint32_t	*addr;
  __uint32_t	ifield;		/* ifield - ALWAYS valid */
  __uint32_t	pad;
}  src_l2rr_t;


typedef __uint32_t src_rr2l_t;

/* source roadrunner status opcodes */
#define SRRS_VB			(1<<31) /* Validity Bit. Bit alternates on/off */
#define SRRS_OP_MASK		0x7f000000
#define SRRS_OP_SHFT		24
#define SRRS_OP_XMIT_OK		(0x0<<SRRS_OP_SHFT) /*transmit succesful  */
#define SRRS_OP_DESC_FLUSHED 	(0x1<<SRRS_OP_SHFT) /*subseq descs to here have been flushed */
#define SRRS_OP_CONN_TIMEO 	(0x3<<SRRS_OP_SHFT) /* connection timed out */

#define SRRS_OP_DST_DISCON 	(0x4<<SRRS_OP_SHFT) /* destination disconnected */
#define SRRS_OP_CONN_REJ	(0x5<<SRRS_OP_SHFT) /* connection rejected */
#define SRRS_OP_SRC_PERR	(0x7<<SRRS_OP_SHFT) /* local parity error forced
						       bad par. onto HIPPI */

/* addr of descriptor -- all prior descr have been processed */
#define SRRS_ADDR_MASK		0x00ffffff


typedef struct {
    __uint32_t    desc_ring_consumer;
    __uint32_t    data_ring_consumer;
} dst_l2rr_t;


typedef struct {
  __uint32_t		*addr;
  int			len;
  __uint32_t		flag;
#define DRRD_VB		        (1<<31)
#define DRRD_IFP		(1<<30)
#define DRRD_EOP		(1<<29)

#define DRRD_PAUSE_NO_DESC	(1<<21)
#define DRRD_PAUSE_NO_BUFF	(1<<20)
#define DRRD_NO_PKT_RCV		(1<<19)
#define DRRD_NO_BURST_RCV	(1<<18)

#define DRRD_LAST_WORD_ODD	(1<<17)
#define DRRD_FBURST_SHORT	(1<<16)

#define DRRD_READY_ERR		(1<<10)
#define DRRD_LINKRDY_ERR	(1<<9)
#define DRRD_FLAG_ERR		(1<<8)
#define DRRD_FRAMING_ERR	(1<<7)
#define DRRD_BURST_SIZE_ERR_SHIFT 5
#define DRRD_BURST_SIZE_MASK	(3<<DRRD_BURST_SIZE_ERR_SHIFT)
#define DRRD_ERR_ST_SHIFT	2
#define DRRD_ERR_ST_MASK	(0x7<<DRRD_ERR_ST_SHIFT)
#define DRRD_LLRC_ERR		(1<<1)
#define DRRD_PAR_ERR		1

/* flags which cause a flush to occur */
#define DRRD_ERR_MASK (DRRD_NO_PKT_RCV |	\
                       DRRD_NO_BURST_RCV |	\
		       DRRD_READY_ERR | 	\
		       DRRD_LINKRDY_ERR |	\
		       DRRD_FLAG_ERR |		\
		       DRRD_FRAMING_ERR |	\
		       DRRD_BURST_SIZE_MASK |	\
		       DRRD_LLRC_ERR |		\
		       DRRD_PAR_ERR |		\
		       DRRD_ERR_ST_MASK)

/* flags which are for informational purposes */
#define DRRD_INFO_MASK (DRRD_PAUSE_NO_DESC |    \
                        DRRD_PAUSE_NO_BUFF)

} dst_rr2l_t;

#endif /* _ROAD_RUNNER_H */
