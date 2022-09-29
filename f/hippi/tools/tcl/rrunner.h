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
 * $Revision: 1.4 $	$Date: 1996/09/21 05:36:33 $
 *
 * $Log: rrunner.h,v $
 * Revision 1.4  1996/09/21 05:36:33  irene
 * Update host control reg bits for Rev B.
 *
 * Revision 1.3  1996/08/23  04:12:18  irene
 * Added new PCI state reg.
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

/* PCI CONFIG SPACE - offsets in the 1st 64 bytes, defined by PCI spec */
#define	PCI_VENDOR_ID		0x0
#define PCI_STAT_CMD		0x4
#define PARITY_ERR		(1<<31)
#define MASTER_ENABLE		(1<<2)
#define	MEM_ACCESS_ENABLE	(1<<1)
#define CLEAR_STAT		0xFFFF0000  /* write 1 to clear */
#define	PCI_CONFIG_CLASS_REV	0x8
#define PCI_CONFIG_BASE_REG0	0x10
#define PCI_CONFIG_BASE_REG1	0x14
#define PCI_CHARACTERISTICS	0x3C

/* Structure definition for PCI config space */
typedef struct{
    uint_t	dev_vend_id;
    uint_t	stat_cmd;	    
    uint_t	cc3_rev;

    uchar_t	bist;
    uchar_t	hdr_type;
    uchar_t	lat_timer;
    uchar_t	cache_line_size;
    
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
} pci_cfg_hdr_t;

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

    /* RoadRunner HIPPI Receive State Register controls and 
     * monitors the HIPPI receive interface in the NIC. Look at err
     * bits when a HIPPI receive Error Event occurs. 
     */
    uint_t recv_state_reg;  /* offset 0x70 */
#define ENABLENEWCONN		0x01
#define RESETRECVINTERF		0x02
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
#define ENABLEXMIT	    0x01
#define PERMCONN	    0x02    /* keep conns active nothing to send */

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

    /* -------- DMA FIFO Access (0x100 - 0x13F) -------- */
    uint_t dma_fifo_access[16];

    /* -------- DMA Assist Registers (0x140 - 0x17F) ---------*/
    uint_t dma_assist_regs[16];

    /* -------- RR GPRs (0x180 - 0x1FF) -------- */
    uint_t rr_gpr[32];

    /* -------- General Communications Area (0x200-0x3FF) ------------ */
    uint_t rr_gca[128];

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

#endif /* _ROAD_RUNNER_H */
