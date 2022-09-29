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

#ifndef __LINCUTIL_H_
#define __LINCUTIL_H_

/*
 * lincutil.h
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

#ident "$Revision: 1.11 $"

/* Fatal LINC errors. */
#define LINC_CISR_ERRS_MASK	(LINC_CISR_SYSAD_DPAR| \
	LINC_CISR_BUFMEM_RD_ERR|LINC_CISR_REG_SIZE|LINC_CISR_WR_TO_RO| \
	LINC_CISR_CPCI_RD_ERR|LINC_CISR_BBUS_RTO| \
	LINC_CISR_IDMA_BUSY_ERR|LINC_CISR_BBUS_PARERR| \
	LINC_CISR_BBUS_ERR|LINC_CISR_BUFMEM_ERR| \
	LINC_CISR_CPCI_ERROR|LINC_CISR_PPCI_ERROR| \
	LINC_CISR_DMA1_ERR|LINC_CISR_DMA0_ERR)

#if defined(_LANGUAGE_C)

#include "r4650.h"

/* must mask off high 2 bits to allow signed arithmetic and
 * simple wrap calculation
 */
#define MICROSEC_TO_CLOCK_TICS (CPU_HZ/2/1000000)
#define ERROR_BLINK_TIMER 500000
	
#define timer_set(timer) (*timer = (int)(0x7fffffff & get_r4k_count()))

#define timer_expired(timer, comp) \
          (((0x80000000 + (int)(~0x80000000 & get_r4k_count())  \
	     - timer) & ~0x80000000) > (comp * MICROSEC_TO_CLOCK_TICS))


/* from util.c */
extern void	vasprintf( char *s, char *fmt, long *arg0p );
extern void	sprintf( char *s, char *fmt, ... );
extern void	delayncycles( int n );		/* delay n processor cycles */
extern int	strcmp( const char *s1, const char *s2 );
extern char	*strcpy( char *s1, const char *s2 );
extern void	bcopy( const void *src, void *dst, int length );
extern void	bzero( void *dst, int length );
extern void	wait_usec(int i);
extern void	blink_error_leds(int mode, int major, int minor);

extern void SerialSync(void);

/* DELAY(n) : delay n usecs. */
#define DELAY(n) delayncycles(((n)*(CPU_HZ/1000))/2000)


/* Low-level routines in cpuops.s
 */
extern void	breakpoint(void);
extern void	cache_off(void);
extern void	invalidate_icache( void *addr, int len );
extern void	nuke_icache(void);
extern void	nuke_dcache(void);
extern void	flush_dcache(void);
extern void	wbinval_dcache( void *addr, int len );
extern void	wbinval_dcache1( void *addr );
extern void	inval_dcache( void *addr, int len );
extern void	inval_dcache1( void *addr );
extern uint32_t getsr(void);
extern void	setsr(uint32_t s);
extern uint32_t get_r4k_count(void);
extern void	set_r4k_compare( uint32_t compare );

extern void	wgather(uint *faddr, uint *taddr, int len);
extern void	rgather(void *faddr, void *taddr);

extern uint32_t hi32( uint64_t x );
#define HI32(x) (*(uint32_t *)(&(x)))
#define LO32(x) ((uint32_t)(x))


/* Miscellaneous */
#define MIN(x,y)	( (x)>(y) ? (y) : (x) )
#define MAX(x,y)	( (x)<(y) ? (y) : (x) )

#endif /* _LANGUAGE_C */
#endif /* __LINCUTIL_H_ */
