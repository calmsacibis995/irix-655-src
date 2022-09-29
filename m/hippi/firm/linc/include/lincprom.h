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

#ifndef __LINCPROM_H_
#define __LINCPROM_H_

/*
 * lincprom.h
 *
 * Header file for lincprom, a generic boot-up PROM for LINC devices.
 *
 * Copyright 1996, Silicon Graphics, Inc.
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

#ident "$Revision: 1.10 $"
#ifndef _KERNEL
#include "eframe.h"
#endif

#include "../../../kern/sys/hip_errors.h"

/*
 * The flash PROM used on LINC devices (AMD Am29F010) is a 128K
 * PROM divided into 8 16K (individually erasable) sectors.  The
 * first sector will be mapped into the boot-mode (SR.BEV=1)
 * exception vector space (0xbfc00000) after power-on/reset.
 * That sector will contain boot-up code that can then copy
 * operation firmware from other sectors into SDRAM or SSRAM,
 * download code from the serial port, or execute code copied
 * into SDRAM/SSRAM from the host.
 *
 * The LINC boot-up PROM will go through the following steps:
 *
 * 1) Initialize CPU, LINC, etc.
 * 2) Initialize SDRAM refresh and test memory.
 * 3) Test SSRAM if it exists.
 * 4) Check sector #2 (phys addr 0x0fc08000) for magic #.  Copy
 *    firmware to SDRAM or SSRAM and execute if it is present.
 * 5) Go into download mode which can allow either download of
 *    code to memory over serial port (via S-records) or can accept
 *    execute commands from host (assuming host copied executable
 *    code to memory).
 */


#define SSRAM_SIZE		0x00020000	/* 128K */

#define LPROM_STACK		0x8001fff0

/* Buffer memory initialization parameters.
 */
#define BUFMEM_CTL_VAL (LINC_BMC_REFRESH_W(0x410) | \
			LINC_BMC_REF_EN | \
			LINC_BMC_REFR_TO_ACT_W(5) | \
			LINC_BMC_PR_TO_ACT_W(2) | \
			LINC_BMC_PAR_CHK_EN | \
			LINC_BMC_ACT_TO_PR_W(3) | \
			LINC_BMC_RAS_TO_CAS_W(2) | \
			LINC_BMC_CAS_LATENCY_W(2) )
				
#define BUFMEM_OPMODE  (LINC_BMO_MODE_LAT_W(2) | LINC_BMO_MODE_BURST_W(3) )

/* Things that cause NMIs. */
#define LINC_CISR_NMI_SOURCES	(LINC_CISR_SYSAD_DPAR| \
	LINC_CISR_BUFMEM_RD_ERR|LINC_CISR_REG_SIZE|LINC_CISR_WR_TO_RO| \
	LINC_CISR_CPCI_RD_ERR|LINC_CISR_BBUS_RTO| \
	LINC_CISR_IDMA_BUSY_ERR|LINC_CISR_NMI_BUTTON|LINC_CISR_SET_NMI| \
	LINC_CISR_BBUS_PARERR)


/* lincprom firmware header parameters.
 */
#define LINCPROM_FHDR_MAGIC	0x6c696e63	/* "linc" */

#define LINCPROM_PERMINFO	0x1fc04000	/* persistent store */

#define LINCPROM_FHDR		0x1fc08000
#define LINCPROM_FHDR_TEXT	0x1fc08020

#define LINCPROM_DEFAULT_DL	0x80000000

#ifdef _LANGUAGE_ASSEMBLY
#define SETLEDS(s)	li t0,((~(s))&LINC_LED_LED_MASK) ;\
			sw t0,PHYS_TO_K1(LINC_LED)
#else
#define SETLEDS(s)	LINC_WRITEREG(LINC_LED, (~(s))&LINC_LED_LED_MASK)
#define GETLEDS()	LINC_READREG(LINC_LED)
#endif /* _LANGUAGE_ASSEMBLY */

#ifdef _LANGUAGE_C

/* If the magic number appears at 0x0fc08000 (physical addr), then
 * interpret the first eight words as this.  Copy the firmware
 * starting at 0x0fc08020 to the address at start_addr and execute
 * starting with address "entry."
 */
typedef struct {
	u_int	magic;			/* magic num (LINCPROM_FHDR_MAGIC) */
	u_int	start_addr;		/* where to copy firmware */
	u_int	size;			/* how many bytes to copy */
	u_int	entry;			/* entry point */
	u_int	cksum;			/* checksum */
	u_int	lincprom_vers;		/* included from fwvers.h */
	u_int	firmware_vers;		/* included from fwvers.h */
	u_int	resvd;
} lincprom_fhdr_t;

#endif /* _LANGUAGE_C */

#define LINCPROM_EXEC_MBOX	8 /* mailbox to write an execution address if
				   * lincprom has no built in firmware. */

/* Upon NMI or Soft Reset, we'll dump the contents of all the CPU
 * and LINC registers into a structure located in SDRAM.  These
 * definitions show the layout of the LINC registers dumped.
 */
#define	LINCDUMP_CISR	0
#define	LINCDUMP_LCSR	1
#define	LINCDUMP_DCSR0	2
#define	LINCDUMP_DCSR1	3
#define	LINCDUMP_SCEA	4
#define	LINCDUMP_ICSR	5
#define	LINCDUMP_IHA	6
#define	LINCDUMP_ILA	7
#define	LINCDUMP_CERR	8
#define	LINCDUMP_CEA	9
#define	LINCDUMP_BME	10
#define	LINCDUMP_BMEA	11
#define	LINCDUMP_BBCSR	12
#define	LINCDUMP_PERR	13
#define	LINCDUMP_PEAH	14
#define	LINCDUMP_PEAL	15
#define LINCDUMP_REGS	16

#ifdef _LANGUAGE_C
#ifndef _KERNEL
typedef struct {
	reg_t		regs[ MAX_REGS ];		/* CPU registers */
	uint32_t	lincregs[ LINCDUMP_REGS ];	/* LINC registers */
} lincprom_nmidump_t;
#endif
#endif /* _LANGUAGE_C */
#endif /* __LINCPROM_H_ */
