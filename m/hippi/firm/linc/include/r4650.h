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

#ifndef _R4650_H_
#define _R4650_H_

/*
 * r4650.h
 *
 *	A kludgey header file that takes R4000 defines in sys/sbd.h
 *	and tweaks them for the IDT R4650.
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

#ident "$Revision: 1.6 $"

#ifndef R4000
#define R4000 1
#endif
#include <sys/sbd.h>

#define CPU_HZ		133333333


/* R4650 cache parameters. */
#define DCACHE_LINESIZE		32
#define ICACHE_LINESIZE		32
#define DCACHE_LINEMASK		(DCACHE_LINESIZE-1)
#define ICACHE_LINEMASK		(ICACHE_LINESIZE-1)

#define DCACHE_INDEXES		128	/* 2-way associative */
#define ICACHE_INDEXES		128	/* 2-way associative */

#define ICACHE_SIZE		(ICACHE_LINESIZE*ICACHE_INDEXES*2)
#define DCACHE_SIZE		(DCACHE_LINESIZE*DCACHE_INDEXES*2)

/* SR register changes. */
#undef SR_TS
#undef SR_RP
#undef SR_KX
#undef SR_SX
#undef SR_KSU_MSK
#undef SR_KSU_USR
#undef SR_KSU_KS

#define SR_DL		0x01000000	/* data cache lock set A */
#define SR_IL		0x00800000	/* instr cache lock set A */
#define SR_UM		0x00000010	/* user mode bit. */

/* Extra CAUSE register bits. */
#define CAUSE_DW	0x02000000	/* Watch exception caused by DWatch */
#define CAUSE_IW	0x01000000	/* Watch exception caused by IWatch */
#define CAUSE_IV	0x00800000	/* New int vector at 0x200 */

/* Exception code changes. */
#undef EXC_MOD
#undef EXC_RMISS
#undef EXC_WMISS
#undef EXC_VCEI
#undef EXC_VCED

#define EXC_IBOUND	EXC_CODE(2)
#define EXC_DBOUND	EXC_CODE(3)

/* Extra TAGLO bits */
#define PFIFOBIT	0x04

/* Cache index operations use this bit to select set B within a cache
 * block.
 */
#define CACHEA	0x0000
#define CACHEB	0x1000

/* undefine the R4000 C0 registers that the R4650 doesn't have. */

#undef C0_INX
#undef C0_RAND
#undef C0_TLBLO
#undef C0_CTXT
#undef C0_TLBHI
#undef C0_TLBLO_0
#undef C0_TLBLO_1
#undef C0_PGMASK
#undef C0_TLBWIRED
#undef C0_LLADDR
#undef C0_WATCHLO
#undef C0_WATCHHI
#undef C0_TAGHI

#ifdef _LANGUAGE_ASSEMBLY

#define C0_IBASE	$0
#define C0_IBOUND	$1
#define C0_DBASE	$2
#define C0_DBOUND	$3
#define C0_CALG		$17	/* cache attributes for memory regions */
#define C0_IWATCH	$18
#define C0_DWATCH	$19

#else

#define C0_IBASE	0
#define C0_IBOUND	1
#define C0_DBASE	2
#define C0_DBOUND	3
#define C0_CALG		17	/* cache attributes for memory regions */
#define C0_IWATCH	18
#define C0_DWATCH	19

#endif /* _LANGUAGE_ASSEMBLY */

#endif /* _R4650_H_ */
