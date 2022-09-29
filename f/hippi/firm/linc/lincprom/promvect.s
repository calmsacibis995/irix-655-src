
/*
 * promvect.s
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
 *
 *
 */

#include <sys/asm.h>
#include <sys/regdef.h>

#include "r4650.h"
#include "sys/PCI/linc.h"

	.extern	reset
	.extern	bevexception
	.extern bevcacheerr

#define trap	teq zero,zero

	.text
	.set noreorder
	.set noat

	.globl	__start
	.ent	__start,0

__start:					/* offset 0x000 */
	j	reset
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

	trap					/* offset 0x080 */
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

	trap					/* offset 0x100 */
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

	trap					/* offset 0x180 */
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

						/* offset 0x200 */
#ifdef SABLE
	la	k0,bevexception
	jr	k0
	 nop
#else
	trap
	trap
	trap
	trap
#endif
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

	trap					/* offset 0x280 */
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

						/* offset 0x300 */
	j	bevcacheerr
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

						/* offset 0x380 */
	j	bevexception
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

	.set reorder
	.set at
	.end __start
