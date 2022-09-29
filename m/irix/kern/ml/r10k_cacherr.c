/*
 * File: 	r10k_cacherr.c
 * Purpose: 	Process r10k cache error exception.
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

#if R10000

#ifdef R4000
#define ecc_handler r10k_ecc_handler
#define ecc_panic r10k_ecc_panic
#endif /* R4000 */

#include	<sys/types.h>
#include	<sys/sbd.h>
#include	<sys/cmn_err.h>
#include	<sys/reg.h>
#include	<sys/kmem.h>
#include 	<sys/debug.h>
#include	<sys/pda.h>
#include	<sys/systm.h>

#if	EVEREST

#include	<sys/EVEREST/everest.h>
#include	<sys/EVEREST/IP25.h>
#include	<sys/EVEREST/IP25addrs.h>
#include 	<sys/EVEREST/everror.h>
#include 	<sys/EVEREST/evconfig.h>

#elif IP28 || IP30 || IP32

#include 	<sys/cpu.h>

#elif 	SN0

#include        <sys/nodepda.h>
#include	<sys/SN/error.h>
#include 	"SN/error_private.h"
#include 	<sys/mapped_kernel.h>
#include 	<sys/SN/SN0/arch.h>

#endif

typedef	struct cef {
    eframe_t	cef_eframe;		/* eframe */
    eccframe_t	cef_eccf;		/* ecc frame */
    eframe_t	cef_panic;		/* eframe at time of panic */
#   define 	STACK_SIZE (ECCF_STACK_SIZE-(2 * sizeof(eframe_t)) - \
			    sizeof(eccframe_t))
    char	cef_stack[STACK_SIZE];
} cef_t;

#if EVEREST

cef_t **cacheErr_frames;

#elif IP28 || IP30 || IP32

cef_t *cacheErr_frames[MAXCPUS];

#endif

struct reg_values type_values[] = {
    0,	"Icache",
    1, 	"Dcache",
    2,	"Scache",
    3,	"Interface",
    0,	NULL
};

struct	reg_desc cache_desc[] = {	/* cache description */
    {CE_TYPE_MASK, 	-CE_TYPE_SHFT, "Type", NULL, type_values},
    {CE_EW, 		0, 	"EW", 	NULL, 	NULL},
    {CE_EE,		0,	"EE", 	NULL, 	NULL},
    {CE_D_WAY1,		0,	"D1",	NULL, 	NULL},
    {CE_D_WAY0,		0,	"D0",	NULL,	NULL},
    {CE_TA_WAY1,	0,	"TA1",	NULL,	NULL},	
    {CE_TA_WAY0,	0,	"TA[0]",NULL,	NULL},
    {CE_TS_WAY1,	0,	"TS[1]",NULL,	NULL},
    {CE_TS_WAY0,	0,	"TS[0]",NULL,	NULL},
    {CE_TM_WAY1,	0,	"TM[1]",NULL,	NULL},
    {CE_TM_WAY0,	0,	"TM[0]",NULL, 	NULL},
    {0,			0,	NULL,	NULL,	NULL}
};

struct	reg_desc interface_desc[] = {	/* interface description */
    {CE_TYPE_MASK, 	-CE_TYPE_SHFT, "Type", NULL, type_values},
    {CE_EW, 		0, 	"EW", 	NULL, 	NULL},
    {CE_EE,		0,	"EE", 	NULL, 	NULL},
    {CE_D_WAY1,		0,	"D1",	NULL, 	NULL},
    {CE_D_WAY0,		0,	"D0",	NULL,	NULL},
    {CE_SA,		0,	"SysAd",NULL,	NULL},
    {CE_SC,		0,	"SysCmd",NULL,	NULL},	
    {CE_SR,		0,	"SysRsp",NULL,	NULL},
    {0,			0,	NULL,	NULL,	NULL}
};
	
    

/*
 * Macros: XLINE_ADDR
 * Purpose: Compute A address to index the cache from the cache error
 *	    register, including the LSB (way) bit.
 * Parameters: cer - cache error register.
 * Notes: THese macros currently assume that the least significant bits of
 *	  the PIDX/SIDX fields in the CER register are 0 if the line sizes
 *	  are larger than the minimum.
 * 
 */
#define	SLINE_WAY0	(CE_D_WAY0|CE_TA_WAY0)
#define	SLINE_WAY1	(CE_D_WAY1|CE_TA_WAY1)
#define	SLINE_ADDR(cer)	(((cer) & CE_SINDX_MASK) + K0BASE)

#define	ILINE_WAY0	(CE_TA_WAY0|CE_TS_WAY0|CE_D_WAY0)
#define	ILINE_WAY1	(CE_TA_WAY1|CE_TS_WAY1|CE_D_WAY1)
#define	ILINE_ADDR(cer)	(((cer) & CE_PINDX_MASK) + K0BASE)

#define	DLINE_WAY0	(CE_TA_WAY0|CE_TS_WAY0|CE_TM_WAY0|CE_D_WAY0)
#define	DLINE_WAY1	(CE_TA_WAY1|CE_TS_WAY1|CE_TM_WAY1|CE_D_WAY1)
#define	DLINE_ADDR(cer)	(((cer) & CE_PINDX_MASK) + K0BASE)

extern void cacheOP(cacheop_t *);

#if IP25
static	void
ecc_clearSCCTag(__uint64_t cer, int way)
/*
 * Function: 	ecc_clearSCCTag
 * Purpose:	To avoid coherence problems caused by an SCC chip bug, we must
 *		make sure that cache line just invalidated is NOT mark valid
 * 		in the SCC duplicate tags.
 * Parameters:	cer - cache error register - assumes SIE or SCACHE error.
 *		way - 0 for way 0, 1 for way 1.
 * Returns:	nothing
 */
{
    __uint32_t	idx;

    idx = (cer & CE_SINDX_MASK) >> 7;
    *(__uint64_t *)(EV_BUSTAG_BASE+(idx<<4)|(way ? EV_BTRAM_WAY : 0)) = 0;
}

#endif

static	__uint64_t 
ecc_cacheOp(__uint32_t op, __uint64_t addr)
/*
 * Function: 	ecc_cacheOp
 * Purpose:	Perform the specified cache operation and return the
 *		value remaining in the taghi/taglo registers as a 64-bit
 *		value.
 * Parameters:	op    - cahe operation to perform, including cache selector.
 *		addr  - address, either virtual or physical with low bit
 *		        indicating way.
 * Returns:	64-bit cache tag (taghi/taglo registers concatenated).
 */
{
    cacheop_t	cop;

    cop.cop_address 	= addr;
    cop.cop_operation  	= op;
    
    cacheOP(&cop);

    return(((__uint64_t)cop.cop_taghi << 32) | (__uint64_t)cop.cop_taglo);
}

/*
 * Function	: ecc_cacheop_get
 * Parameters	: type -> what operation do we want to perform.
 *		  addr -> address to perform operation upon.
 *		  cop  -> pointer to cacheop structure.
 * Purpose	: to perform the necessary cache op.
 * Assumptions	: None.
 * Returns	: None (cop will be filled with relevant details)
 */

void
ecc_cacheop_get(__uint32_t type, __psunsigned_t addr, cacheop_t *cop)
{
	extern	void	cacheOP(cacheop_t *);

	cop->cop_operation = type;
	cop->cop_address   = addr;

	cacheOP(cop);
	return;
}


static	void
ecc_display(int flags, char *s, __uint32_t cer, 
	    __uint64_t errorEPC, __uint64_t tag[])
/*
 * Function: ecc_display
 * Purpose: 	To print information about a cache error given the cache error
 *		register.
 * Parameters:	flags - cmn_err flags to add in.
 *		s - pointer to a string indicating reason.
 *		cer - cache error register value.
 *		tag - cache tag read that decision was based on.
 * Returns: nothing
 */
{
    static char template[] = "Cache Error (%s) %R errorEPC=0x%x tag=0x%x paddr=0x%x %s\n";
    char *tag_valid = "(Tag Valid)";
    char *tag_invalid = "(Tag Invalid)";
    int invalid, way;

    switch(cer & CE_TYPE_MASK) {
    case CE_TYPE_I:
	for (way = 0; way <= 1; way++) {
	    if (tag[way] == 0)
		continue;

	    invalid = ((tag[way] & CTP_STATE_MASK) == 0);
	    if (tag[way])
		cmn_err(flags, template, s, (__uint64_t)cer, cache_desc,
			errorEPC, tag[way], PCACHE_ERROR_ADDR(cer, tag[way]),
			invalid ? tag_invalid : tag_valid);
	}
	break;
    case CE_TYPE_D:
	for (way = 0; way <= 1; way++) {
	    if (tag[way] == 0)
		continue;
	    invalid = (((tag[way] & 
			 CTP_STATE_MASK) >> CTP_STATE_SHFT) == CTP_STATE_I);

	    cmn_err(flags,template,s,(__uint64_t)cer,cache_desc,errorEPC,
		    tag[way], PCACHE_ERROR_ADDR(cer, tag[way]),
		    invalid ? tag_invalid : tag_valid);
	}
	break;

    case CE_TYPE_S:
	for (way = 0; way <= 1; way++) {
	    if (tag[way] == 0)
		continue;
	    invalid = (((tag[way] & 
			 CTS_STATE_MASK) >> CTS_STATE_SHFT) == CTS_STATE_I);

	    cmn_err(flags,template,s,(__uint64_t)cer,cache_desc,errorEPC,
		    tag[way], SCACHE_ERROR_ADDR(cer, tag[way]),
		    invalid ? tag_invalid : tag_valid);
	}
	break;

    case CE_TYPE_SIE:
	for (way = 0; way <= 1; way++) {
	    if (tag[way] == 0)
		continue;
#ifndef SN0
	    cmn_err(flags,template,s,(__uint64_t)cer,interface_desc,errorEPC,
		    tag[way], SCACHE_ERROR_ADDR(cer, tag[way]), tag_valid);
#else /* SN0 */
	{
	    paddr_t paddr;
	    int dimm;
	    int cnode;

	    paddr = SCACHE_ERROR_ADDR(cer, tag[way]);
	    dimm = (paddr & MEM_DIMM_MASK) >> MEM_DIMM_SHFT;
	    cnode = NASID_TO_COMPACT_NODEID(NASID_GET(paddr));
	    cmn_err(flags, 
		    "Interface Error. Suspect MEMORY BANK %d on %s\n",
		    dimm, (cnode == INVALID_CNODEID) ? "a remote partition" :
		    NODEPDA(cnode)->hwg_node_name);

	    if (error_page_discard(paddr, PERMANENT_ERROR,UNCORRECTABLE_ERROR))
		cmn_err(CE_NOTE, "Recovered from memory error by discarding the page");
	}
#endif
	}
	break;    
    }
}    

static void
ecc_cleanRegion(void *p, int l)
/*
 * Function: ecc_cleanRegion
 * Purpose:  To be sure a specified region is not in the cache.
 * Parameters:  p - address of base of region.
 *		l - length in bytes of region
 * Returns: nothing
 * Notes: Cleans multiples of line.
 */
{
    __uint64_t	pp = (__uint64_t)p;

    while (l > 0) {
	ecc_cacheOp(CACH_S+C_HWBINV, pp);
#if IP25
	/*
	 * Doing a SCACHE invalidate on IP25 can cause a problem in the 
	 * CC chip. It can cause a lose of coherence on the cache line 
	 * that is invalidated - if it gets pulled back and placed in 
	 * the other "way". It is not a problem here because the region
	 * we are cleaning should not be modified cached.
	 */
#endif
	pp += CACHE_SLINE_SIZE;
	l -= CACHE_SLINE_SIZE;
    }
}

void
ecc_init(void)
/*
 * Function: ecc_init
 * Purpose: Set up stacks etc for ecc handler for THIS cpu.
 * Parameters: 	none
 * Returns:	nothing
 * Notes: For EVEREST, we use the physical CPUID to index into the array.
 *	  It is easiest in locore.
 */
{
    /* REFERENCED */
    uint	physid;
    /* REFERENCED */
    __uint64_t	a;

#if EVEREST
    physid = EV_GET_LOCAL(EV_SPNUM) & EV_SPNUM_MASK;
#elif IP28 || IP32
    physid = 0;
#elif IP30
    physid = cpuid();
#endif

#if EVEREST
    if (!EVERROR_EXT->eex_ecc) {
	a = (__uint64_t)kmem_zalloc(sizeof(eccframe_t *) * EV_SPNUM_MASK, 
				   VM_CACHEALIGN | VM_DIRECT);
	if (!a) {
	    cmn_err(CE_PANIC,
		    "Unable to allocate memory for cache error handler");
	}
	ecc_cleanRegion((void *)a, sizeof(eccframe_t *) * EV_SPNUM_MASK);
	cacheErr_frames = (void *)K0_TO_K1(a);
	ecc_cleanRegion(&cacheErr_frames, sizeof(cacheErr_frames));
	EVERROR_EXT->eex_ecc = (cef_t **)K0_TO_K1(a);
    }

    a = (__uint64_t)kmem_zalloc(ECCF_STACK_SIZE, VM_CACHEALIGN | VM_DIRECT);
    if (!a) { 
	cmn_err(CE_PANIC,"Unable to allocate ECCF");
    }
    ecc_cleanRegion((void *)a, ECCF_STACK_SIZE);

    ((cef_t **)EVERROR_EXT->eex_ecc)[physid] = (cef_t *)K0_TO_K1(a);
#elif IP28 || IP32
    a = (__uint64_t)kmem_zalloc(ECCF_STACK_SIZE,VM_CACHEALIGN|VM_DIRECT);
    if (!a) {
	cmn_err(CE_PANIC,"Unable to allocate ECCF");
    }

    /* make sure low handler and real memory base are consistant */
    ecc_cleanRegion((void *)a, sizeof(struct cef));

    cacheErr_frames[physid] = (cef_t *)K0_TO_K1(a);
    ecc_cleanRegion(&cacheErr_frames, sizeof(cacheErr_frames));
#elif IP30
    a = (__uint64_t)kmem_zalloc(ECCF_STACK_SIZE, VM_CACHEALIGN|VM_DIRECT);
    if (!a) { 
	cmn_err(CE_PANIC,"Unable to allocate ECCF");
    }
    ecc_cleanRegion((void *)a, ECCF_STACK_SIZE);

    cacheErr_frames[physid] = (cef_t *)K0_TO_K1(a);
    *(volatile __psunsigned_t *)PHYS_TO_K1(CACHE_ERR_FRAMEPTR + (physid<<3)) =
			K0_TO_K1(a);
#elif SN0

#if defined (HUB_ERR_STS_WAR)
   if (WAR_ERR_STS_ENABLED) {
	    
	__uint64_t start_off, end_off;
	/*
	 * Allocate cache_err stack and store it in the CACHE_ERR_SP_PTR.
	 */
	a = (__uint64_t)kmem_zalloc(2 * ECCF_STACK_SIZE, VM_CACHEALIGN | VM_DIRECT);
	start_off = (__psunsigned_t)a & 0x3ffff;
	end_off = (__psunsigned_t)(a + ECCF_STACK_SIZE) & 0x3ffff;
	if (!a) { 
		return;
	}

	if ((start_off < 0x480)  || 
	    ((end_off > 0x400) && (end_off < ECCF_STACK_SIZE))) 
		a += ECCF_STACK_SIZE;
    } else
#endif /* HUB_ERR_STS_WAR */
    {
	/*
	 * Allocate cache_err stack and store it in the CACHE_ERR_SP_PTR.
	 */
	 a = (__uint64_t)kmem_zalloc(ECCF_STACK_SIZE, VM_CACHEALIGN | VM_DIRECT);
	if (!a) { 
	    cmn_err(CE_PANIC,"Unable to allocate ECCF");
	    return;
	}
    }

    ecc_cleanRegion((void *)a, ECCF_STACK_SIZE);
    *(__psunsigned_t *)(UALIAS_BASE + CACHE_ERR_SP_PTR) = 
				      K0_TO_K1(a + ECCF_STACK_SIZE - 0x10);
    *(__psunsigned_t *)(UALIAS_BASE + CACHE_ERR_IBASE_PTR) = 
	                              PHYS_TO_K1(TO_PHYS(MAPPED_KERN_RO_PHYSBASE(cnodeid())));
#endif
}

#if IP28 || IP32
/* use a constant for early ECC errors until malloc works */
void
early_ecc_init(char *addr)
{
	cacheErr_frames[0] = (cef_t *)addr;
}
#endif

#if defined (EVEREST) || defined (SN0)
/* ARGSUSED */
int
eccf_recoverable_threshold(eframe_t *ef, eccframe_t *eccf, uint cer)
{
	k_machreg_t lapse;
	k_machreg_t currtc = GET_LOCAL_RTC;
	
	ushort *cnt_p;

	switch(cer & CE_TYPE_MASK) {
	case CE_TYPE_I:
		cnt_p = &(eccf->ecct_recover_icache_count);
		break;
	case CE_TYPE_D:
		cnt_p = &(eccf->ecct_recover_dcache_count);
		break;
	case CE_TYPE_S:
		cnt_p = &(eccf->ecct_recover_scache_count);
		break;
	case CE_TYPE_SIE:
		cnt_p = &(eccf->ecct_recover_sie_count);
		break;
	default:
		return 1;
	}
	
	if (eccf->ecct_recover_rtc) {
		lapse = currtc - eccf->ecct_recover_rtc;
		if (lapse < CERR_RECOVER_TIME) {
			if (++(*cnt_p) > CERR_RECOVER_COUNT)
			    return 0;
			else
			    return 1;
		}
	}
	eccf->ecct_recover_rtc = currtc;
	*cnt_p = 0;
	
	return 1;
}
#endif /* defined (EVEREST) || defined (SN0) */


void
ecc_force_cache_line_invalid(__psunsigned_t addr, int way, int type)
{
	cacheop_t	cop;

	cop.cop_taglo = cop.cop_taghi = cop.cop_ecc = 0;

	cop.cop_address = addr | way;
	cop.cop_operation = type + C_IST;
	cacheOP(&cop);
	cop.cop_operation = type + C_ISD;
	cacheOP(&cop);
}    

/*ARGSUSED1*/
char *
ecc_handler(eframe_t *ef, eccframe_t *eccf)
/*
 * Function: 	ecc_handler
 * Purpose:	To process an r10k cache error exception.
 * Parameters:	Cache error control block for this CPU.
 */
{
    __uint64_t	cer;			/* u64 so not sign extended */
    __uint64_t	t;			/* tag */
    __uint64_t	addr;			/* address temp */
    __uint64_t	way;			/* way 0 or 1 */
    int		idx;
    char	*result = NULL;

#if defined (CACHE_DEBUG)
    extern int  r10k_halt_1st_cache_err;
    extern int  r10k_cache_debug;
    extern void qprintf(char *f, ...);
#endif /* CACHE_DEBUG */

#if defined (SN0)
    int		err_save = 0;

#if defined (CACHE_DEBUG)
#pragma weak icache_save
    extern void icache_save(int);
#pragma weak dcache_save
    extern void dcache_save(int);
#pragma weak scache_save
    extern void scache_save(int);
#endif /* CACHE_DEBUG */
#endif /* SN0 */

    int 	limit_exceeded = 0;

    cer = (__uint64_t)eccf->eccf_cache_err;

#if defined (CACHE_DEBUG)
#if defined (SN0)
    if (r10k_halt_1st_cache_err || r10k_cache_debug) {
#pragma	weak	print_cacherr
	  extern void print_cacherr(__uint32_t);

	if (print_cacherr) {
	  print_cacherr(cer);
	}
    }

    if (r10k_cache_debug) {
	  int cpu = cpuid();
	
	if (icache_save) {
	  icache_save(cpu);
	}
	if (dcache_save) {
	  dcache_save(cpu);
	}
	if (scache_save) {
	  scache_save(cpu);
	}

	  qprintf("saved cache state on CPU %d after ECC error\n",cpu);
    }
#else 
    if (r10k_halt_1st_cache_err)
          qprintf("Cache Error register 0x%x\n",cer);
#endif /* SN0 */

    if (r10k_halt_1st_cache_err)
           cmn_err(CE_PANIC,"Forced panic on first cache error\n");

#endif /* CACHE_DEBUG */

    /* Check if we can trace the current exception */

    if (ECCF_ADD(eccf->eccf_trace_idx,1) != eccf->eccf_putbuf_idx) {
	idx = eccf->eccf_trace_idx;

	/*
         * make sure that the index is always within array bounds.
	 * no reason why it should not be, but this was not initialized
	 * anywhere and the kernel would take an exception accessing some
	 * random index into the array. That problem has been fixed, but 
	 * this is an additional safegaurd.
         */
	if (idx > ECCF_TRACE_CNT)
	    idx = idx % ECCF_TRACE_CNT;

	eccf->eccf_trace[idx].ecct_cer = (__uint32_t)cer;
	eccf->eccf_trace[idx].ecct_errepc  = eccf->eccf_errorEPC;
	eccf->eccf_trace_idx = ECCF_ADD(eccf->eccf_trace_idx, 1);
#if defined (EVEREST) || defined (SN0)
	eccf->eccf_trace[idx].ecct_rtc = GET_LOCAL_RTC;
#endif /* EVEREST || SN0 */
    } else {
	idx = -1;			/* no tracing this time */
    }
#if defined (EVEREST) || defined (SN0)
    if (!ECCF_RECOVERABLE_THRESHOLD(ef, eccf, cer))
	limit_exceeded = 1;
#endif /* EVEREST || SN0 */

	eccf->eccf_tag[0] = eccf->eccf_tag[1] = 0;

	switch(cer & CE_TYPE_MASK) {
	case CE_TYPE_I:
#if defined (SN0)
	    err_save = 1;
#endif
	    eccf->eccf_icount++;
	    /*
	     * For Icache, we can just invalidate both ways, and continue 
	     * running.
	     */
	    addr = ILINE_ADDR(cer);
	    for (way = 0; way <= 1; way++) {
		    __uint32_t way_mask = (way) ? DLINE_WAY1 : DLINE_WAY0;
		    if ((cer & way_mask) == 0)
			continue;
		    t = ecc_cacheOp(CACH_PI + C_ILT, addr + way);
		    eccf->eccf_tag[way] = t;
	    }

	    (void)ecc_cacheOp(CACH_PI+C_IINV, addr); /* Invalidate and continue */
	    (void)ecc_cacheOp(CACH_PI+C_IINV, addr^1);/* Invalidate and continue */

	    ecc_force_cache_line_invalid(addr, 0, CACH_PI);
	    ecc_force_cache_line_invalid(addr, 1, CACH_PI);
	    if (limit_exceeded)
		result = "too many primary instruction cache error exceptions";
	    break;

    case CE_TYPE_D:
#if defined (SN0)
	err_save = 1;
#endif
	eccf->eccf_dcount++;
	addr = DLINE_ADDR(cer);

	if (!(cer & DLINE_WAY0) && !(cer & DLINE_WAY1)) {
	    if (cer & CE_EE) {
		/*
		 * Tag error on inconsistent block. * SHOULD NOT HIT THIS *
		 */
		result = "unrecoverable - dcache tag on inconsistent block";
		break;
	    }
	    result = "unrecoverable dcache error, neither way";
	    break;
	}

	for (way = 0; way <= 1; way++) {
	    __uint32_t way_mask = (way) ? DLINE_WAY1 : DLINE_WAY0;

	    if ((cer & way_mask) == 0)
		continue;

	    t = ecc_cacheOp(CACH_PD+C_ILT, addr + way);
	    eccf->eccf_tag[way] = t;

	    if (cer & CE_TM_MASK) {
		/*
		 * If a tag mod array error occured, we can not use the 
		 * STATEMOD to figure out if we can recover. In this case, 
		 * all we can do is invalidate the line if it is clean, 
		 * and panic if it is dirty.
		 */
		if (((t & CTP_STATE_MASK) >> CTP_STATE_SHFT) == CTP_STATE_DE) {
		    result = "unrecoverable - dcache tag mod";
		    break;	
		}
	    } 
	    if (cer & (CE_TA_MASK + CE_TS_MASK)) {
		/*
		 * Tag array/State array error are recoverable if line 
		 * is consistent with Scache.
		 */
		if (((t & CTP_STATEMOD_MASK) >> CTP_STATEMOD_SHFT) 
		                                       == CTP_STATEMOD_I) {
		    result = "unrecoverable - dcache tag";
		    break;	
		} 
	    } 
	    if (cer & CE_D_MASK) {
		if (((t & CTP_STATE_MASK) >> CTP_STATE_SHFT) == CTP_STATE_DE) {
		    /* 
		     * If dirty, panic - if not, invalidate and continue 
		     */
		    if (((t & CTP_STATEMOD_MASK) >> CTP_STATEMOD_SHFT) 
			                                == CTP_STATEMOD_I) {
			result = "unrecoverable - dcache data";
			break;
		    }
		} 
	    }

	    (void)ecc_cacheOp(CACH_PD+C_IINV, addr+way);
	}
	if (cer & CE_EE) {
	    /*
	     * Tag error on inconsistent block. * SHOULD NOT HIT THIS *
	     */
	    result = "unrecoverable - dcache tag on inconsistent block";
	    break;
	}
        if (limit_exceeded)
	   result = "too many primary data cache error exceptions";

	break;

    case CE_TYPE_S:
	eccf->eccf_scount++;
	addr = SLINE_ADDR(cer);

	if (!(cer & SLINE_WAY0) && !(cer & SLINE_WAY1)) {
#if defined (SN0)
		err_save = 1;
#endif
		result = "unrecoverable scache error, neither way";
		break;
	}

	for (way = 0; way <= 1; way++) {
	    __uint32_t way_mask = (way) ? SLINE_WAY1 : SLINE_WAY0;
	    
	    if ((cer & way_mask) == 0)
		continue;
	    
	    t = ecc_cacheOp(CACH_S + C_ILT, addr + way);
	    eccf->eccf_tag[way] = t;

	    if (((t & CTS_STATE_MASK) >> CTS_STATE_SHFT) == CTS_STATE_I) {
#if defined (CACHE_ERR_DEBUG)
		cmn_err(CE_WARN | CE_CPUID, "Scache error on invalid line");
		ecc_scache_line_dump(addr, way, printf);
#endif /* CACHE_ERR_DEBUG */

		continue;
	    }
#if defined (SN0)
	    err_save = 1;
#endif
	    if (cer & CE_TA_MASK) {
		result = "unrecoverable - scache tag";
		break;
	    } 
	    /*
	     * For now, turn this on only for SN0.
	     */

#if defined (SN0)
	{
	    int syn_check;
	    syn_check = ecc_scache_line_check_syndrome(addr, way);
	    if (syn_check > 1) {
#if defined (CACHE_ERR_DEBUG)
		cmn_err(CE_WARN | CE_CPUID, "Scache error: bad syndrome");
		ecc_scache_line_dump(addr, way, printf);
#endif /* CACHE_ERR_DEBUG */
	    }
	    else {
		result = "Transient scache error";
		break;
	    }
	}
#endif /* SN0 */
	    
	    if (((t & CTS_STATE_MASK) >> CTS_STATE_SHFT) == CTS_STATE_DE) {
		result = "unrecoverable scache data";
		break;
	    }
	    (void)ecc_cacheOp(CACH_S + C_IINV, addr + way);
#if IP25
	    ecc_clearSCCTag(addr, way);
#endif
	}
        if (limit_exceeded)
	   result = "too many secondary cache error exceptions";

	break;

    case CE_TYPE_SIE:
#if defined (SN0)
	err_save = 1;
#endif
	eccf->eccf_sicount++;
	addr = SLINE_ADDR(cer);

#if IP30
	if (cer & (CE_SA | CE_EE)) {
		static char	err_buf[160];
		heartreg_t	h_cause;
		heartreg_t	h_memerr_addr;
		heart_piu_t	*heart;
		extern char	*maddr_to_dimm(paddr_t);

		heart = HEART_PIU_K1PTR;
		h_cause = heart->h_cause;
		if (h_cause & HC_NCOR_MEM_ERR) {
			h_memerr_addr = heart->h_memerr_addr & HME_PHYS_ADDR;
			if ((h_memerr_addr & CE_SINDX_MASK) ==
			    (cer & CE_SINDX_MASK)) {
				sprintf(err_buf,
					"unrecoverable SIE caused by multi bit "
					"ECC error at physaddr 0x%x, DIMM %s",
					h_memerr_addr,
					maddr_to_dimm(h_memerr_addr));
				result = err_buf;
				break;
			}
		}
	}
#endif	/* IP30 */

	if (cer & (CE_SA | CE_SC | CE_SR)) {
		result = "unrecoverable SIE";
		break;
	}

	if (!(cer & CE_D_WAY0) && !(cer & CE_D_WAY1)) {
		result = "unrecoverable SIE, neither way";
		break;
	}

	for (way = 0; way <= 1; way++) {
	    __uint32_t way_mask = (way) ? CE_D_WAY1 : CE_D_WAY0;

	    if ((cer & way_mask) == 0)
		continue;

	    t = ecc_cacheOp(CACH_S + C_ILT, addr + way);
	    eccf->eccf_tag[way] = t;

	    if (((t & CTS_STATE_MASK) >> CTS_STATE_SHFT) == CTS_STATE_I) {
		continue;
	    }
	    
#if defined (SN0)
	    if (memerror_handle_uce(cer, t, ef, eccf)) {
		    (void)ecc_cacheOp(CACH_S + C_IINV, addr + way);
		    ecc_force_cache_line_invalid(addr, way, CACH_S);
		    continue;
	    }
#endif /* SN0 */	    

	    if (((t & CTS_STATE_MASK) >> CTS_STATE_SHFT) == CTS_STATE_DE) {
		result = "unrecoverable SIE - dirty scache";
		break;			/* for */
	    } 
	    (void)ecc_cacheOp(CACH_S + C_IINV, addr + way);
#if IP25
	    ecc_clearSCCTag(addr, way);
#endif
	    ecc_force_cache_line_invalid(addr, way, CACH_S);
	}

        if (limit_exceeded) {
#if defined (SN0) 
	{
	    cnodeid_t cnode = NASID_TO_COMPACT_NODEID(NASID_GET(addr));
	    cmn_err(CE_WARN, "Unrecoverable Interface error: Suspect memory address 0x%x At %s Bank %d", addr, (cnode == INVALID_CNODEID) ? 
		    "a remote partition" : NODEPDA(cnode)->hwg_node_name,
		    (addr & MEM_DIMM_MASK) >> MEM_DIMM_SHFT);
	}
#endif
	    result = "too many interface error exceptions.";
	}
	
	break;    

    default:
	result = "unrecoverable - unknown cache error";
	break;
    }

    if (idx >= 0) {
	eccf->eccf_trace[idx].ecct_tag[0]  = eccf->eccf_tag[0];
	eccf->eccf_trace[idx].ecct_tag[1]  = eccf->eccf_tag[1];
    }

    private.p_cacherr = 1;

    if (result) {
	/*
	 * If we are going to panic, and we are not already panicing 
	 * (which can occur if we get into symmon and then a symmon 
	 * command causes another cache error), we save the eframe
	 * and update the status.
	 */
	if (eccf->eccf_status != ECCF_STATUS_PANIC) {
	    eccf->eccf_status = ECCF_STATUS_PANIC;
	    ((cef_t *)ef)->cef_panic = *ef;
	}
    }
#if defined (SN0)
    if (err_save)
	    save_cache_error(cer, eccf->eccf_tag[0], eccf->eccf_tag[1]);
#endif

#if IP28 || IP30 || IP32
    /* Ensure we are consistant wrt to the cache in the face of
     * speculation that could touch our data.
     */
     for (idx = 0 ; idx < sizeof(struct cef); idx += CACHE_SLINE_SIZE) {
	ecc_cacheOp(CACH_S|C_HINV,PHYS_TO_K0((__psunsigned_t)eccf+idx));
     }
#endif
    return(result);
}



void
ecc_panic(char *s, eframe_t *ef, eccframe_t *e)
/*
 * Function: 	ecc_panic
 * Purpose:	Panic on a non-recoverable cache error.
 * Parameters:	s - panic string to print.
 *		ef - pointer to current eframe.
 *		s - pointer to current ecc frame.
 * Returns:	Does not return, PANICS
 */

{
#if defined (SN0)
    machine_error_dump(s);
#endif
    ecc_display(CE_PHYSID+CE_WARN, s, 
		e->eccf_cache_err,e->eccf_errorEPC, e->eccf_tag);
    cmn_err(CE_PHYSID+CE_PANIC, "Cache Error (%s) Eframe = 0x%x\n", s,
	    &((cef_t *)ef)->cef_panic);
}

void
ecc_log(void)
/*
 * Function: ecc_log
 * Purpose: To log any pending exceptions on THIS cpu only.
 */
{
#if EVEREST || IP28 || SN0 || IP32
    eccframe_t	*e;
    /* REFERENCED */
    uint	spnum;
    uint	i;

#if EVEREST
    spnum = EV_GET_LOCAL(EV_SPNUM);
#elif IP28 || IP32
    spnum = 0;
#endif

    /* 
     * Clear PDA cache error indication before checking, so that if a new
     * cache error arrives, we will call here again.
     */

    private.p_cacherr = 0;
#if !defined (SN0)    
    e = &cacheErr_frames[spnum]->cef_eccf;
#else
    e = (eccframe_t *)(UALIAS_BASE + CACHE_ERR_ECCFRAME);
#endif
    if (e->eccf_trace_idx == ECCF_ADD(e->eccf_putbuf_idx,ECCF_TRACE_CNT-1)){
	cmn_err(CE_WARN+CE_PHYSID, 
		"Some recoverable cache errors may not have been logged\n");
    }
    for (i = e->eccf_putbuf_idx; i != e->eccf_trace_idx; i = ECCF_ADD(i, 1)) {
	ecc_display(CE_WARN | CE_PHYSID | CE_TOOKACTIONS, "recoverable",
		    e->eccf_trace[i].ecct_cer, 
		    e->eccf_trace[i].ecct_errepc,
		    e->eccf_trace[i].ecct_tag);
    }
    e->eccf_putbuf_idx = i;		/* mark updated pointer */
#endif
}


#if EVEREST
void
ecc_pending(int slot, int slice)
/*
 * Function: 	ecc_pending
 * Purpose:	To display any recorded by un-displayed cache errors
 *		on the specified CPU.
 * Parameters:	slot - slot # of CPU
 *		slice - slice # of CPU
 * Returns: nothing
 */
{
    eccframe_t	*e;
    uint	i;

    e = &((cef_t **)EVERROR_EXT->eex_ecc)[(slot<<EV_SLOTNUM_SHFT)+
					  (slice<<EV_PROCNUM_SHFT)]->cef_eccf;

    if (e->eccf_trace_idx != e->eccf_trace_idx) {
	cmn_err(CE_WARN, "CPU %d: Pending cache errors\n", 
		EVCFG_CPUID(slot, slice));
	
	for (i = e->eccf_putbuf_idx;i!=e->eccf_trace_idx;i=ECCF_ADD(i, 1)) {
	    ecc_display(0, "recovered", e->eccf_trace[i].ecct_cer, 
			e->eccf_trace[i].ecct_errepc,
			e->eccf_trace[i].ecct_tag);
	}
    }
}
#endif /* EVEREST */

#if EVEREST || IP28 || IP30 || IP32 || SN
void
ecc_printPending(void (*printf)(char *, ...))
/*
 * Function: 	ecc_printPending
 * Purpose:	To dump pending cache errors
 * Parameters:	printf - pointer to printf style routine
 *			used to print out the results.
 * Notes: This routine called from symmon to dump the
 *	   cache error logs.
 */
{
    eccframe_t	*e;
    int		i, cpu;

#if EVEREST
    for (cpu = 0; cpu < EV_SPNUM_MASK; cpu++) {
	if (cacheErr_frames[cpu]) {
	    e = &cacheErr_frames[cpu]->cef_eccf;
	    printf("CPU [0x%x/0x%x]: Total Errors ic(%d) dc(%d) sc(%d) in(%d)\n",	
		   (cpu & EV_SLOTNUM_MASK) >> EV_SLOTNUM_SHFT, 
		   (cpu & EV_PROCNUM_MASK) >> EV_PROCNUM_SHFT,
#elif IP28 || IP30 || IP32
    for (cpu = 0; cpu < MAXCPUS; cpu++) {
	if (cacheErr_frames[cpu]) {
	    e = &cacheErr_frames[cpu]->cef_eccf;
	    printf("Total Errors [cpu %d] ic(%d) dc(%d) sc(%d) in(%d)\n",
		   cpu,
#elif SN
    for (cpu = 0; cpu < MAXCPUS; cpu++) {
	nasid_t nasid;
	paddr_t ualias_offset;

	if (pdaindr[cpu].pda != NULL) {
	    ualias_offset  = cputoslice(cpu) ? UALIAS_SIZE : 0;
	    nasid = cputonasid(cpu);
	    e = (eccframe_t *)
		TO_NODE_UNCAC(nasid, (ualias_offset + CACHE_ERR_ECCFRAME));
	    printf("Total Errors [cpu %d] ic(%d) dc(%d) sc(%d) in(%d)\n",
		   cpu,
#endif
		   e->eccf_icount, e->eccf_dcount, 
		   e->eccf_scount, e->eccf_sicount);

	    for (i = e->eccf_putbuf_idx;
		 i != e->eccf_trace_idx; i = ECCF_ADD(i, 1)) {
		printf("\tcer=0x%x errorEPC=0x%x tag [0]=0x%x [1]=0x%x\n",
		       (__uint64_t)e->eccf_trace[i].ecct_cer, 
		       (__uint64_t)e->eccf_trace[i].ecct_errepc, 
		       e->eccf_trace[i].ecct_tag[0], 
		       e->eccf_trace[i].ecct_tag[1]);
	    } 
	}
    }
}
#endif /* EVEREST || IP28 || IP30 || IP32 || SN*/

int
ecc_bitcount(__uint64_t wd)
{
    int numbits = 0;

    while (wd) {
	wd &= (wd - 1);	   /* This has the effect of resetting 
			    * the rightmost set bit */
	numbits++;
    }
    return numbits;
}


struct scdata_ecc_mask {
	__uint64_t d_emaskhi;
	__uint64_t d_emasklo;
};

struct scdata_ecc_mask scdata_eccm[SCDATA_ECC_BITS] = {
        { E9_8H, E9_8L },
	{ E9_7H, E9_7L },    
	{ E9_6H, E9_6L },
	{ E9_5H, E9_5L },
	{ E9_4H, E9_4L },
	{ E9_3H, E9_3L },
	{ E9_2H, E9_2L },
	{ E9_1H, E9_1L },
	{ E9_0H, E9_0L },
};

struct sctag_ecc_mask {
	__uint64_t t_emask;
};


struct sctag_ecc_mask sctag_eccm[SCTAG_ECC_BITS] = {
	E7_6W_ST,
	E7_5W_ST,
	E7_4W_ST,
	E7_3W_ST,
	E7_2W_ST,
	E7_1W_ST,
	E7_0W_ST,
};



char *cache_valid_str[] = {
	"invalid",
	"shared",
	"clean-ex",
	"dirty-ex"
};
#define BYTESPERQUADWD	(sizeof(int) * 4)
#define BYTESPERDBLWD	(sizeof(int) * 2)
#define BYTESPERWD	(sizeof(int) * 1)


    

/*
 * Function	: ecc_compute_sctag_ecc
 * Parameters	: tag -> the tag to compute ecc on.
 * Purpose	: to generate the ecc of the secondary cache tag word.
 * Assumptions	: None. 
 * Returns	: Returns the ecc check_bits.
 */

uint
ecc_compute_sctag_ecc(__uint64_t tag)
{
	uint	check_bits = 0;
	int	i;	
	
	for (i = 0; i < SCTAG_ECC_BITS; i++) {
		check_bits <<= 1;
		if (ecc_bitcount(tag & sctag_eccm[i].t_emask) & 1)
			check_bits |= 1;
	}
	return check_bits;
}


/*
 * Function	: ecc_compute_scdata_ecc
 * Parameters	: data_hi -> the upper 64 bits of the data.
 *		  data_lo -> the lower 64 bits of the data.
 * Purpose	: to generate the ecc of the secondary cache data word.
 * Assumptions	: None. 
 * Returns	: Returns the ecc check_bits of the sc data.
 */

uint
ecc_compute_scdata_ecc(__uint64_t data_hi, __uint64_t data_lo)
{
    uint	check_bits;
    int i;				 /* loop variables */
    __uint64_t ecc_word1, ecc_word2; /* ecc_words used for each pass */

    check_bits = 0;
    for (i = 0; i < SCDATA_ECC_BITS; i++) {
	check_bits <<= 1;
	ecc_word1 = data_hi & scdata_eccm[i].d_emaskhi;
	ecc_word2 = data_lo & scdata_eccm[i].d_emasklo;
	
	/*
	 * Get the xor of the 2 words. Since all we need to know 
	 * is if number of bits are odd or even, the xor works 
	 * out pretty well in giving us one word with all the info
	 */
	if ((ecc_bitcount(ecc_word1 ^ ecc_word2)) & 1)
	    check_bits |= 1;
    }
    if (ecc_bitcount(check_bits) & 1)
	check_bits |= SC_ECC_PARITY_MASK;
    return check_bits;
}


void
ecc_scache_line_data(__psunsigned_t k0addr, int way, t5_cache_line_t *sc)
{
    cacheop_t cop;	
    __uint64_t c_tag, c_data;
    int i, j, s;
    paddr_t paddr;
    int idx;
	
    s = splhi();

    idx = (k0addr & ((private.p_scachesize >> 1) - 1));

    ecc_cacheop_get(CACH_SD + C_ILT, k0addr | way, &cop);
    c_tag =(((__uint64_t)cop.cop_taghi << 32) | (__uint64_t)cop.cop_taglo);
    sc->c_tag = c_tag;
    paddr = ((c_tag & CTS_TAG_MASK) << 4) | idx;
    sc->c_addr = PHYS_TO_K0(paddr);

    for (i = 0; i < SCACHE_LINE_FRAGMENTS; i++) {
	for (j = 0; j < 2; j++) {
	    ecc_cacheop_get(CACH_SD + C_ILD, k0addr | way, &cop);
	    c_data =(((__uint64_t)cop.cop_taglo << 32)  |
		     (__uint64_t)cop.cop_taghi);
	    sc->c_data.sc_bits[i].sc_data[j] = c_data;
	    k0addr += BYTESPERDBLWD;
	}
	sc->c_data.sc_bits[i].sc_ecc = cop.cop_ecc;
    }
    splx(s);
    sc->c_idx = idx / CACHE_SLINE_SIZE;
    sc->c_type = CACH_SD;
}


void
ecc_scache_line_get(__psunsigned_t k0addr, int way, t5_cache_line_t *sc)
{
    int i;

    ecc_scache_line_data(k0addr, way, sc);

    sc->c_state = (sc->c_tag & CTS_STATE_MASK) >> CTS_STATE_SHFT;
    sc->c_way = way;

    for (i = 0; i < SCACHE_LINE_FRAGMENTS; i++) {
	sc->c_data.sc_bits[i].sc_cb = 
	    ecc_compute_scdata_ecc(sc->c_data.sc_bits[i].sc_data[0],
				   sc->c_data.sc_bits[i].sc_data[1]);
	sc->c_data.sc_bits[i].sc_syn =  ((sc->c_data.sc_bits[i].sc_ecc) ^
					 (sc->c_data.sc_bits[i].sc_cb));
    }
}


ecc_scache_bits_in_error(__uint32_t syn)
{
    int count;

    if (syn == 0) return 0;

    count  = ecc_bitcount(syn);

    if ((count == 1) || (count == 3) || (count == 5))
	return 1;

    return 2;
}

int
ecc_scache_line_check_syndrome(__psunsigned_t k0addr, int way)
{
    t5_cache_line_t sc;
    int i;
    int size = private.p_scachesize;
    __uint32_t syn;
    int error, max_error = 0;

    if (k0addr <= (size / (CACHE_SLINE_SIZE * SCACHE_WAYS)))
	k0addr = PHYS_TO_K0(k0addr * CACHE_SLINE_SIZE);
    else
	k0addr = PHYS_TO_K0(K0_TO_PHYS(k0addr)) & CACHE_SLINE_MASK;

    ecc_scache_line_get(k0addr, way, &sc);
    for (i = 0; i < SCACHE_LINE_FRAGMENTS; i++) 
	if (syn = sc.c_data.sc_bits[i].sc_syn) {
	    error = ecc_scache_bits_in_error(syn & 0x1ff);
	    if (error > max_error) max_error = error;
	}

    return max_error;
}


void
ecc_scache_line_show(t5_cache_line_t *sc, void (*prf)(char *, ...))
{
    int i;

    (*prf)("Addr %16x tag  %16x (Tag ecc %1x) Vindx %1x MRU %1x\n",
	   sc->c_addr, sc->c_tag, sc->c_tag & CTS_ECC_MASK,
	   (sc->c_tag & CTS_VIDX_MASK) >> CTS_VIDX_SHFT,
	   ((sc->c_tag & CTS_MRU) == 1));


    (*prf)("Index %8x  Way %1d State %8s\n",
	   sc->c_idx, sc->c_way, cache_valid_str[sc->c_state]);

	       
    for (i = 0; i < SCACHE_LINE_FRAGMENTS; i++) {
	(*prf)("Off %2x Data %16x Data %16x CB %3x ECC %3x Syn %3x BitErr %1x\n",
	       i * 16, 
	       sc->c_data.sc_bits[i].sc_data[0],
	       sc->c_data.sc_bits[i].sc_data[1],
	       sc->c_data.sc_bits[i].sc_cb,
	       sc->c_data.sc_bits[i].sc_ecc,
	       sc->c_data.sc_bits[i].sc_syn,
	       ecc_scache_bits_in_error(sc->c_data.sc_bits[i].sc_syn & 0x1ff));
    }
    (*prf)("\n");
}


void
ecc_picache_line_data(__psunsigned_t k0addr, int way, t5_cache_line_t *pic)
{
    cacheop_t cop;	
    __uint64_t c_tag, c_data;
    int i, s;
    paddr_t paddr;
    int idx;
	
    s = splhi();

    idx = (k0addr & ((R10K_MAXPCACHESIZE >> 1) - 1));

    ecc_cacheop_get(CACH_PI + C_ILT, k0addr | way, &cop);
    c_tag =(((__uint64_t)cop.cop_taghi << 32) | (__uint64_t)cop.cop_taglo);
    pic->c_tag = c_tag;
    paddr = ((c_tag & CTP_TAG_MASK) << 4) | idx;
    pic->c_addr = PHYS_TO_K0(paddr);

    for (i = 0; i < PICACHE_LINE_FRAGMENTS; i++) {
	ecc_cacheop_get(CACH_PI + C_ILD, k0addr | way, &cop);
	c_data =(((__uint64_t)cop.cop_taghi << 32)  |
		 (__uint64_t)cop.cop_taglo);
	pic->c_data.pic_bits[i].pic_data = c_data;
	pic->c_data.pic_bits[i].pic_ecc = cop.cop_ecc;
	k0addr += BYTESPERWD;
    }
    splx(s);
    pic->c_idx = idx / CACHE_ILINE_SIZE;
    pic->c_type = CACH_PI;
}


void
ecc_picache_line_get(__psunsigned_t k0addr, int way, t5_cache_line_t *pic)
{
    int i, invalid;

    ecc_picache_line_data(k0addr, way, pic);

    invalid = ((pic->c_tag & CTP_STATE_MASK) == 0);

    pic->c_state = !invalid;
    pic->c_way = way;

    for (i = 0; i < PICACHE_LINE_FRAGMENTS; i++) {
	pic->c_data.pic_bits[i].pic_cb = 
	    ecc_bitcount(pic->c_data.pic_bits[i].pic_data) & 1;
	pic->c_data.pic_bits[i].pic_syn = 
	    (pic->c_data.pic_bits[i].pic_ecc ^
	     pic->c_data.pic_bits[i].pic_cb);
    }
}


int
ecc_picache_line_check_syndrome(__psunsigned_t k0addr, int way)
{
    t5_cache_line_t pic;
    int i;

    if (k0addr < R10K_ICACHE_LINES)
	k0addr = PHYS_TO_K0(k0addr * CACHE_ILINE_SIZE);
    else
	k0addr = PHYS_TO_K0(K0_TO_PHYS(k0addr)) & CACHE_ILINE_MASK;

    ecc_picache_line_get(k0addr, way, &pic);
    for (i = 0; i < PICACHE_LINE_FRAGMENTS; i++) 
	if (pic.c_data.pic_bits[i].pic_syn) break;

    return (i == PICACHE_LINE_FRAGMENTS) ? 0 : 1;
}

void
ecc_picache_line_show(t5_cache_line_t *pic, void (*prf)(char *, ...))
{
    int i;

    (*prf)("Addr %16x tag  %16x tagparity %1x\n",
	   pic->c_addr, pic->c_tag, pic->c_tag & 1);

    (*prf)("Index %8x  Way %1d valid %1d\n",
	   pic->c_idx, pic->c_way, pic->c_state);
	       
    for (i = 0; i < PICACHE_LINE_FRAGMENTS; i++) {
	(*prf)("Off %2x Data %9x CB %1x ECC %1x Syn %1x\n",
	       i * 4,
	       pic->c_data.pic_bits[i].pic_data,
	       pic->c_data.pic_bits[i].pic_cb,
	       pic->c_data.pic_bits[i].pic_ecc,
	       pic->c_data.pic_bits[i].pic_syn);
    }
    (*prf)("\n");
}


void
ecc_pdcache_line_data(__psunsigned_t k0addr, int way, t5_cache_line_t *pdc)
{
    cacheop_t cop;	
    __uint64_t c_tag;
    int i, s;
    paddr_t paddr;
    int idx;
	
    s = splhi();

    idx = (k0addr & ((R10K_MAXPCACHESIZE >> 1) - 1));
	
    ecc_cacheop_get(CACH_PD + C_ILT, k0addr | way, &cop);
    c_tag =(((__uint64_t)cop.cop_taghi << 32) | (__uint64_t)cop.cop_taglo);
    pdc->c_tag = c_tag;
    paddr = ((c_tag & CTP_TAG_MASK) << 4) | idx;
    pdc->c_addr = PHYS_TO_K0(paddr);

    for (i = 0; i < PDCACHE_LINE_FRAGMENTS; i++) {
	ecc_cacheop_get(CACH_PD + C_ILD, k0addr | way, &cop);
	pdc->c_data.pdc_bits[i].pdc_data = cop.cop_taglo;
	pdc->c_data.pdc_bits[i].pdc_ecc = cop.cop_ecc;
	k0addr += BYTESPERWD;
    }
    splx(s);
    pdc->c_idx = idx / CACHE_DLINE_SIZE;
    pdc->c_type = CACH_PD;
}




void
ecc_pdcache_line_get(__psunsigned_t k0addr, int way, t5_cache_line_t *pdc)
{
    int i, j, count;
    __uint32_t data;

    ecc_pdcache_line_data(k0addr, way, pdc);

    pdc->c_state = (pdc->c_tag & CTP_STATE_MASK) >> CTP_STATE_SHFT;
    pdc->c_way = way;

    for (i = 0; i < PDCACHE_LINE_FRAGMENTS; i++) {
	data = pdc->c_data.pdc_bits[i].pdc_data;
	pdc->c_data.pdc_bits[i].pdc_cb = 0;
	for (j = 0; j < 4; j++) {
	    count = ecc_bitcount(data & (0xff << (j * 8)));
	    pdc->c_data.pdc_bits[i].pdc_cb |= (count & 1) << j;
	}
	pdc->c_data.pdc_bits[i].pdc_syn = 
	    (pdc->c_data.pdc_bits[i].pdc_ecc ^
	     pdc->c_data.pdc_bits[i].pdc_cb);
    }
}


int
ecc_pdcache_line_check_syndrome(__psunsigned_t k0addr, int way)
{
    t5_cache_line_t pdc;
    int i;

    if (k0addr < R10K_DCACHE_LINES)
	k0addr = PHYS_TO_K0(k0addr * CACHE_DLINE_SIZE);
    else
	k0addr = PHYS_TO_K0(K0_TO_PHYS(k0addr)) & CACHE_DLINE_MASK;

    ecc_pdcache_line_get(k0addr, way, &pdc);

    for (i = 0; i < PDCACHE_LINE_FRAGMENTS; i++) 
	if (pdc.c_data.pdc_bits[i].pdc_syn) break;

    return (i == PDCACHE_LINE_FRAGMENTS) ? 0 : 1;
}


void
ecc_pdcache_line_show(t5_cache_line_t *pdc, void (*prf)(char *, ...))
{
    int i;

    (*prf)("Addr %16x tag  %16x tagparity %1x statmod 0x%1x\n",
	   pdc->c_addr, pdc->c_tag,pdc->c_tag & 1,
	   (pdc->c_tag & CTP_STATEMOD_MASK) >> CTP_STATEMOD_SHFT);

    (*prf)("Index %8x  Way %1d State %7s\n",
	   pdc->c_idx, pdc->c_way, cache_valid_str[pdc->c_state]);

    for (i = 0; i < PDCACHE_LINE_FRAGMENTS; i++) {
	(*prf)("Off %2x Data %w328x CB %1x ECC %1x Syn %1x\n",
	       i * 4, 
	       pdc->c_data.pdc_bits[i].pdc_data,
	       pdc->c_data.pdc_bits[i].pdc_cb,
	       pdc->c_data.pdc_bits[i].pdc_ecc,
	       pdc->c_data.pdc_bits[i].pdc_syn);
    }
    (*prf)("\n");
}



void
ecc_picache_line_dump(__psunsigned_t k0addr, int way, void (*prf)(char *, ...))
{
    t5_cache_line_t pic;

    if (k0addr < R10K_ICACHE_LINES)
	k0addr = PHYS_TO_K0(k0addr * CACHE_ILINE_SIZE);
    else
	k0addr = PHYS_TO_K0(K0_TO_PHYS(k0addr)) & CACHE_ILINE_MASK;

    ecc_picache_line_get(k0addr, way, &pic);
    ecc_picache_line_show(&pic, prf);
}



void
ecc_pdcache_line_dump(__psunsigned_t k0addr, int way, void (*prf)(char *, ...))
{
    t5_cache_line_t pdc;

    if (k0addr < R10K_DCACHE_LINES)
	k0addr = PHYS_TO_K0(k0addr * CACHE_DLINE_SIZE);
    else
	k0addr = PHYS_TO_K0(K0_TO_PHYS(k0addr)) & CACHE_DLINE_MASK;

    ecc_pdcache_line_get(k0addr, way, &pdc);
    ecc_pdcache_line_show(&pdc, prf);
}


void
ecc_scache_line_dump(__psunsigned_t k0addr, int way, void (*prf)(char *, ...))
{
    t5_cache_line_t sc;
    int size = getcachesz(cpuid());

    if (k0addr <= (size / 256))
	k0addr = PHYS_TO_K0(k0addr * CACHE_SLINE_SIZE);
    else
	k0addr = PHYS_TO_K0(K0_TO_PHYS(k0addr)) & CACHE_SLINE_MASK;

    ecc_scache_line_get(k0addr, way, &sc);
    ecc_scache_line_show(&sc, prf);
}



void
ecc_scache_dump(void (*prf)(char *, ...), int check)
{	
    int line, size, way;

    size = private.p_scachesize;

    for (line = 0; 
	 line < (size / (CACHE_SLINE_SIZE * SCACHE_WAYS)); line++) {
	for (way = 0; way < SCACHE_WAYS; way++) 
	    if (!check || ecc_scache_line_check_syndrome(line, way))
		ecc_scache_line_dump(line, way, prf);
    }
}


void
ecc_picache_dump(void (*prf)(char *, ...), int check)
{	
    int line, way;
    
    for (line = 0; line < R10K_ICACHE_LINES; line++) {
	for (way = 0; way < PCACHE_WAYS; way++)
	    if (!check || ecc_picache_line_check_syndrome(line, way))
		ecc_picache_line_dump(line, way, prf);
    }
}


void
ecc_pdcache_dump(void (*prf)(char *, ...), int check)
{	
    int line, way;

    for (line = 0; line < R10K_DCACHE_LINES; line++) {
	for (way = 0; way < PCACHE_WAYS; way++)
	    if (!check || ecc_pdcache_line_check_syndrome(line, way))
		ecc_pdcache_line_dump(line, way, prf);
    }
}
    
#endif /* R10000 */
