/*
 * prommain.c
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

#ident "$Revision: 1.7 $"

#include <sys/types.h>

#include "r4650.h"

#include "sys/PCI/linc.h"
#include "lincprom.h"
#include "eframe.h"
#include "serial.h"
#include "lincutil.h"

#include "hippi_sw.h"

eframe_t iframe;

extern void bevdie( int, int, int );

#define FHDR()	( (lincprom_fhdr_t *)PHYS_TO_K1( LINCPROM_FHDR ) )

void
prom_fhdr(void)
{
	uint32_t	*src, *dst;
	int		bytes, i;
	uint32_t	cksum;
	lincprom_fhdr_t	*fhdr = FHDR();
	void (*entry)(void);

#if !defined(SIM) && !defined(SABLE)

	/* Calculate the checksum of the Fhdr. */
	cksum = 0;
	for (i=0; i<sizeof(lincprom_fhdr_t)/sizeof(uint32_t); i++) {
		uint32_t	d = ((uint32_t *)fhdr)[i];

		cksum += (d & 0xffff);
		cksum += (d >> 16);
		cksum = (cksum>>16) + (cksum&0xffff);
	}
	
	/* Copy firmware to memory (either SDRAM or SSRAM).
	 * Assume (destination) start address is K0SEG address and
	 * we'll use the D-cache to burst this data to SDRAM/SSRAM.
	 */
	bytes = (int) fhdr->size;
	src = (uint32_t *) PHYS_TO_K1( LINCPROM_FHDR_TEXT );
	dst = (uint32_t *) fhdr->start_addr;

	while ( bytes > 0 ) {

		*dst = *src;

		cksum += (*dst) & 0xffff;
		cksum += (*dst) >> 16;
		cksum = (cksum>>16) + (cksum&0xffff);

		src++;
		dst++;
		bytes -= sizeof(uint32_t);
	}

	/* Flush D-cache.
	 */
	flush_dcache();

	/* Invalidate I-cache.
	 */
	nuke_icache();

	/* Fold 1's complement checksum into 16 bits. */
	cksum = (cksum>>16) + (cksum&0xffff);
	cksum = (cksum>>16) + (cksum&0xffff);

	if ( cksum != 0xffff )
		bevdie( DIE_CKSUM, cksum, 0 );
#endif

	/* Enter firmware.  Firmware should reset BEV bit.
	 */
	entry = (void (*)(void)) fhdr->entry;
	(*entry)();
}

u_int last_compare;

void
prommain(void)
{
	void (*entry_addr)(void);
	hip_hc_t *hcmd = (hip_hc_t *)PHYS_TO_K1(HCMD_BASE);
	u_int i;

#ifdef SIM
	* (volatile u_char *) PHYS_TO_K1(LINC_SIMPASS) = 0;
#endif
	
	/* init sign register so driver can see progress */
	bzero(hcmd, sizeof(hip_hc_t));
	hcmd->sign = HIP_SIGN_INIT;

	/* Clear BOOTING bit */
	LINC_WRITEREG( LINC_LCSR, 0);

	/* If the magic number is present in the beginning of sector 1
	 * of the PROM, then copy the firmware to memory and execute
	 * it.
	 */
	if ( FHDR()->magic == LINCPROM_FHDR_MAGIC )
		prom_fhdr();
	
	/* Wait for a mailbox write to mailbox4 which tells us
	 * where to start executing (host downloaded) code.
	 */

	while ( !(LINC_READREG(LINC_MAILBOX_STATUS)&(1<<LINCPROM_EXEC_MBOX)))
	    blink_error_leds(HIP_SIGN_BOOT_DEATH, DIE_NOFIRM, 0);

	entry_addr = (void (*)(void))
		( * (uint64_t *) PHYS_TO_K1( LINC_MAILBOX_ADDR + 
				LINCPROM_EXEC_MBOX*LINC_MAILBOX_PGSIZE) );

	/* We got an execution address.  Turn off LEDs before starting.
	 */
	SETLEDS(0);

	/* Jump to execution address.
	 */
	(*entry_addr)();
}

