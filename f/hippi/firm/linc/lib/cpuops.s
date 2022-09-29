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
 * cpuops.s
 *
 * $Revision: 1.7 $
 */

#include <sys/asm.h>
#include <sys/regdef.h>

#include "r4650.h"
#include "lincutil.h"
#include <sys/PCI/linc.h>

	.text

	.set noreorder


/* void breakpoint();
 */
LEAF(breakpoint)
	break	2
	jr	ra
	nop
END(breakpoint)

	
/* void cache_off(void);
*/
LEAF(cache_off)
	li	t0, 0x22222222
	mtc0	t0, C0_CALG

END(cache_off)	
	

/* void nuke_icache(void);
 *
 * Invalidate entire I-cache using index operations.
 */
LEAF(nuke_icache)
	la	t0,K0BASE		/* use K0BASE addresses for index ops */
	li	t1,ICACHE_INDEXES

1:
	cache	CACH_PI | C_IINV, 0(t0)	/* invalidate block0 */
	ori	t2,t0,CACHEB
	cache	CACH_PI | C_IINV, 0(t2) /* invalidate block1 */

	addi	t1,t1,-1
	bgtz	t1,1b
	 addi	t0,t0,ICACHE_LINESIZE

	jr	ra
	 nop
END(nuke_icache)


/* void nuke_dcache(void);
 *
 * Invalidate (no write-back of dirty lines!) entire D-cache using tag
 * store operations.
 */
LEAF(nuke_dcache)
	mtc0	zero,C0_TAGLO
	la	t0,K0BASE
	li	t1,DCACHE_INDEXES
1:
	cache	CACH_PD | C_IST, 0(t0)	/* store tag in block0 */
	addi	t1,t1,-1
	bgtz	t1,1b
	 addi	t0,t0,DCACHE_LINESIZE
	
	li	t0,0xffff0000
	mtc0	t0,C0_TAGLO
	la	t0,K0BASE+CACHEB
	li	t1,DCACHE_INDEXES
1:
	cache	CACH_PD | C_IST, 0(t0)	/* store tag in block1 */
	addi	t1,t1,-1
	bgtz	t1,1b
	 addi	t0,t0,DCACHE_LINESIZE

	jr	ra
	 nop
END(nuke_dcache)


/* void flush_dcache(void);
 *
 * Flush (writing back dirty lines) the entire D-cache using index operations.
 */
LEAF(flush_dcache)
	la	t0,K0BASE		/* use K0BASE addresses for index ops */
	li	t1,DCACHE_INDEXES

1:
	cache	CACH_PD | C_IWBINV, 0(t0)	/* wb invalidate block0 */
	ori	t2,t0,CACHEB
	cache	CACH_PD | C_IWBINV, 0(t2)	/* wb invalidate block1 */

	addi	t1,t1,-1
	bgtz	t1,1b
	 addi	t0,t0,DCACHE_LINESIZE

	jr	ra
	 nop
END(flush_dcache)

/* void invalidate_icache( void *addr, int len );
 *
 * Invalidate an address range from the I-cache.
 */
LEAF(invalidate_icache)

	andi	t0,a0,ICACHE_LINEMASK
	subu	a0,a0,t0
	add	a1,a1,t0

1:

	cache	CACH_PI | C_HINV, 0(a0)

	addi	a1,a1,-ICACHE_LINESIZE
	bgtz	a1,1b
	 addiu	a0,a0,ICACHE_LINESIZE

	jr	ra
	 nop
END(invalidate_icache)


/* void wbinval_dcache( void *addr, int len );
 *
 * Write-back/invalidate an address range from D-cache.
 */
LEAF(wbinval_dcache)

	andi	t0,a0,DCACHE_LINEMASK
	subu	a0,a0,t0
	add	a1,a1,t0

1:
	cache	CACH_PD | C_HWBINV, 0(a0)

	addi	a1,a1,-DCACHE_LINESIZE
	bgtz	a1,1b
	 addiu	a0,a0,DCACHE_LINESIZE

	jr	ra
	 nop
END(wbinval_dcache)


/* void wbinval_dcache1( void *addr );
 *
 * Write-back/invalidate a single cache line at addr.
 */
LEAF(wbinval_dcache1)
	jr	ra
	 cache	CACH_PD | C_HWBINV, 0(a0)
END(wbinval_dcache1)


/* void inval_dcache( void *addr, int len );
 *
 * Invalidate an address range in D-cache.
 */
LEAF(inval_dcache)
	andi	t0,a0,DCACHE_LINEMASK
	subu	a0,a0,t0
	add	a1,a1,t0

1:
	cache	CACH_PD | C_HINV, 0(a0)

	addi	a1,a1,-DCACHE_LINESIZE
	bgtz	a1,1b
	 addiu	a0,a0,DCACHE_LINESIZE

	jr	ra
	 nop
END(inval_dcache)


/* void inval_dcache1( void *addr );
 *
 * Invalidate a single cache line at addr.
 */
LEAF(inval_dcache1)
	jr	ra
	 cache	CACH_PD | C_HINV, 0(a0)
END(inval_dcache1)


/* uint32_t getsr(void);
 */
LEAF(getsr)
	jr	ra
	 mfc0	v0,C0_SR
END(getsr)

/* void setsr(uint32_t s);
 */
LEAF(setsr)
	jr	ra
	 mtc0	a0,C0_SR
END(setsr)

/* uint32_t get_r4k_count(void);
 */
LEAF(get_r4k_count)
	jr	ra
	 mfc0	v0,C0_COUNT
END(get_r4k_count)

/* void set_r4k_compare( uint32_t compare );
 */
LEAF(set_r4k_compare)
	jr	ra
	 mtc0	a0,C0_COMPARE
END(set_r4k_compare)

/* int splhi(void);
 */
LEAF(splhi)
	mfc0	t0,C0_SR
	andi	v0,t0,(SR_IMASK|SR_IE)		# return these bits to caller
	xor	t0,t0,v0			# turn these bits off in SR
	mtc0	t0,C0_SR

	nop					# no-ops to avoid cp0 hazards
	jr	ra
	 nop
END(splhi)

/* int splx(int s);
 */
LEAF(splx)
	mfc0	t0,C0_SR
	andi	v0,t0,(SR_IMASK|SR_IE)		# return these bits to caller
	xor	t0,t0,v0			# turn them off
	or	t0,t0,a0			# add in the bits in s
	mtc0	t0,C0_SR

	nop					# no-ops to avoid cp0 hazards
	jr	ra
	 nop
END(splx)

/* uint64_t rd64(uint64_t *);
 *
 * Do two 32-bit reads uninterrupted.
 */
LEAF(rd64)
	mfc0	t0,C0_SR
	li	t1,~SR_IE
	and	t1,t0,t1
	mtc0	t1,C0_SR	/* turn off interrupts. */
	nop
	nop
	nop
	nop
	lw	v0,0(a0)
	lw	v1,4(a0)
	jr	ra
	 mtc0	t0,C0_SR
END(rd64)

LEAF(__ll_lshift)
	slti	t0,a3,32
	beq	t0,zero,__ll_lshift_32
	 li	t0,32
	sllv	v0,a0,a3
	sllv	v1,a1,a3
	sub	t0,t0,a3
	srlv	t0,a1,t0
	jr	ra
	 or	v0,v0,t0
__ll_lshift_32:
	sub	t0,a3,t0
	sllv	v0,a1,t0
	jr	ra
	 li	v1,0
END(__ll_lshift)

LEAF(__ull_rshift)
	slti	t0,a3,32
	beq	t0,zero,__ull_rshift_32
	 li	t0,32
	srlv	v0,a0,a3
	srlv	v1,a1,a3
	sub	t0,t0,a3
	sllv	t0,a0,t0
	jr	ra
	 or	v1,v1,t0
__ull_rshift_32:
	sub	t0,a3,t0
	srlv	v1,a0,t0
	jr	ra
	 li	v0,0
END(__ull_rshift)

/* void rgather(void *faddr, void *taddr);
*/
LEAF(rgather)
	li	t0, 0x00ffffff	/*mask out hi bits and make k1seg */
	and	a0, a0, t0
	li	t0, LINC_SDRAM_WG2_ADDR | 0xa0000000
	or	a0, a0, t0

	lw	t0, 0(a0)
	lw	t1, 4(a0)
	sw	t0, 0(a1)
	sw	t1, 4(a1)

	jr	ra
	 nop
END(rgather)
	
/* void wgather(uint *faddr, uint *taddr, int len);
	size is in bytes
*/
LEAF(wgather)

	li	t0, 0x00ffffff	/* mask out hi bits and make k1seg */
	and	a1, a1, t0
	li	t0, 0xa0000000
	or	a1, a1, t0
	beq	a2, 8, 2f	/* branch on what size write gather */
	 nop
	beq	a2, 16, 4f
	 nop	
	j	8f
	 nop

2:
	li	t0, LINC_SDRAM_WG2_ADDR
	or	a1, a1, t0
	lw	t0, 0(a0)
	lw	t1, 4(a0)
	sw	t0, 0(a1)
	sw	t1, 4(a1)
	j	end
	 nop
4:
	li	t0, LINC_SDRAM_WG4_ADDR
	or	a1, a1, t0
	lw	t0, 0(a0)
	lw	t1, 4(a0)
	sw	t0, 0(a1)
	sw	t1, 4(a1)
	lw	t0, 8(a0)
	lw	t1, 12(a0)
	sw	t0, 8(a1)
	sw	t1, 12(a1)
	j	end
	 nop

8:
	li	t0, LINC_SDRAM_WG8_ADDR
	or	a1, a1, t0
	lw	t0, 0(a0)
	lw	t1, 4(a0)
	sw	t0, 0(a1)
	sw	t1, 4(a1)
	lw	t0, 8(a0)
	lw	t1, 12(a0)
	sw	t0, 8(a1)
	sw	t1, 12(a1)

	lw	t0, 16(a0)
	lw	t1, 20(a0)
	sw	t0, 16(a1)
	sw	t1, 20(a1)
	lw	t0, 24(a0)
	lw	t1, 28(a0)
	sw	t0, 24(a1)
	sw	t1, 28(a1)
	
end:
#ifdef RINGBUS_WAR
	li	t0, LINC_SCSR | 0xa0000000
	lw	t0, 0(t0)
#endif
	jr	ra
	 nop
END(wgather)
	
	.set	reorder

