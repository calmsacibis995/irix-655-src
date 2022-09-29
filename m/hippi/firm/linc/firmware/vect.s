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
 * vect.s
 *
 * $Revision: 1.7 $
 *
 */

#include <sys/asm.h>
#include <sys/regdef.h>

#include "r4650.h"
#include "hippi_sw.h"


#define trap	teq zero,zero

	.extern	exception_entry0
	.extern cacheerr_exception0
	.extern nmi_exception0
	.extern interrupt_entry0

	.text
	.set noreorder

	.globl	__vect
	.ent	__vect,0

__vect:
					/* offset 0x000 */
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap

					/* offset 0x080 */
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap

	j	cacheerr_exception0	/* offset 0x100 */
	 nop
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap

	la	k0,exception_entry0	/* offset 0x180 */
	jr	k0
	 nop
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap

	la	k0,nmi_exception0	/* offset 0x1c0 -- "s/w vector" */
	jr	k0			/* called from lincprom for nmi */
	 nop
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap

	la	k0,interrupt_entry0	/* offset 0x200 */
	jr	k0
	 nop
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap
	trap

	la	k0,_start		/* offset 0x280 */
	jr	k0
	 nop

	.end	__vect

