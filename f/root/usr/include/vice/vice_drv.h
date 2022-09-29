#ifndef __SYS_VICE_DRV_H__
#define __SYS_VICE_DRV_H__

/*
 * VICE kernel driver software interface defines
 *
 * $Revision: 1.25 $
 */
#if (_KERNEL)
#define	VICE_BASE	0xb7000000	/* k1seg address of vice chip in IP32 */

#define REGADDRW(a)     ((volatile unsigned int *)(vice_base + (a) + 4))
#define REGSW(a,v)      (*REGADDRW(a) = (v))
#define REGLW(a)        (*REGADDRW(a))

#define RAMADDR(a)      ((volatile unsigned int*)(vice_base + (a)))
#define RAMSW(a,v)      (*RAMADDR(a) = (v))
#define RAMLW(a)        (*RAMADDR(a))

#define RAMADDR_D(a)    ((volatile unsigned long long*)(vice_base + (a)))
#define RAMSD(a,v)      (*RAMADDR_D(a) = (v))
#define RAMLD(a)        (*RAMADDR_D(a))

/* For BSP */
#define REGADDRH(a)     ((volatile unsigned short*)(vice_base + (a) + 6))
#define REGSH(a,v)      (*REGADDRH(a) = (v))
#define REGLH(a)        (*REGADDRH(a))

/*
 * Macro to probe for Vice and add to inventory.  Macro-ized to share
 * the source code between ml/MOOSHEAD/IP32init.c and vice driver.
 *
 * NOTE: Vice is shipped with every IP32, but the below code works if the chip
 * is not in the system for some reason (as was the case for some early
 * bring-up configs).  This code is written around a Crime chip "feature"
 * that makes the cpu wait forever for a PIO read -- we can NOT just simply
 * read from a vice register and look for a valid ID or something.
 *
 * And now... the macro.  The following name of the given type is expected:
 *
 *	caddr_t vice_base;
 *
 */
#define	PROBE_AND_INVENTORY_VICE() \
{ \
	_crmreg_t intstat; \
\
	vice_base = (caddr_t) VICE_BASE; \
\
	/* Enable the Vice MSP "soft" interrupt */ \
	REGSW(VICE_INT_EN, VICE_INT_MSP_INTR); \
\
	/* Wait a bit */ \
	us_delay(10); \
\
	/* Force MSP soft interrupt, and wait a touch */ \
	REGSW(MSP_SW_INT, 0); \
	us_delay(10); \
\
	/* Ask Crime if it saw anything */ \
	intstat = READ_REG64(PHYS_TO_K1(CRM_INTSTAT), _crmreg_t); \
	if (!(intstat & CRM_INT_VICE)) { \
		vice_base = 0; /* Indicates "no vice found" */ \
	} else { \
\
		/* Yup... there's a Vice, clear interrupt condition */ \
		REGSW(VICE_INT_RESET, REGLW(VICE_INT)); \
\
		/* Make inventory entry */ \
		add_to_inventory(INV_COMPRESSION,INV_VICE,0,0,REGLW(VICE_ID)); \
	} \
}
#endif

#define	VICE_XFACE_SHIFT	16
#include "vice/vice_drv_rev.h"	/* VICE_DRV_XFACE and VICE_DRV_X */
#define	VICE_DRV_REV 	((VICE_DRV_XFACE << VICE_XFACE_SHIFT) | VICE_DRV_X)

#define VICE_MAXDEVS    64	/* num. simultaneous vice contexts */
#define VICE_DBGDEV     15      /* minor num of debugger special device */
#define VICE_FAKEDEV	0xff	/* for kernel's pseudo-atom */
#define	VICE_MAXSEQ	64


#define VICEIOC		('v'<<24|'x'<<16)
#define VICEIOCTL(x)	(VICEIOC|x)
#define VICEIOCTLN(x)	(x & 0xffff)

#define VICE_INTR	VICEIOCTL(2)	/* force an MSP interrupt */
#define VICE_PUT	VICEIOCTL(3)
#define VICE_SETTLB	VICEIOCTL(4)
#define	VICE_LOCK	VICEIOCTL(6)
#define	VICE_UNLOCK	VICEIOCTL(7)

#define	VICE_DMS_GETERRS VICEIOCTL(11)
#define	VICE_DMS_RID	VICEIOCTL(12)
#define	VICE_DMS_START	VICEIOCTL(13)
#define	VICE_DMS_SID	VICEIOCTL(15)
#define	VICE_DMS_OPOOL	VICEIOCTL(14)

#define	VICE_UPDATETLB	VICEIOCTL(16)

#define	VICE_IAMDBG	VICEIOCTL(17)
#define	VICE_WAKEUP	VICEIOCTL(18)
#define	VICE_LATESTINTR	VICEIOCTL(19)
#define	VICE_DBGSTART	VICEIOCTL(20)
#define VICE_STARTREQ	VICEIOCTL(21)

#define	VICE_TILE_ALLOC	VICEIOCTL(22)
#define	VICE_DMBUF_SETTLB VICEIOCTL(23)

#define	VICE_SEQUENCE	VICEIOCTL(30)

#define	VICE_GET_DRV_REV VICEIOCTL(31)
#define	VICE_PUT_VADDR	VICEIOCTL(32)

#define	VICE_DBG_NOTIFY	VICEIOCTL(0x101)

#define	VICE_CHIP_ID	VICEIOCTL(0x2000)

#define	VICE_TEST_0	VICEIOCTL(0xf000)
#define	VICE_TEST_1	VICEIOCTL(0xf001)
#define	VICE_TEST_2	VICEIOCTL(0xf002)
#define	VICE_TEST_3	VICEIOCTL(0xf003)

#define VICE_RETIRED_1  VICEIOCTL(1)
#define VICE_RETIRED_5  VICEIOCTL(5)
#define VICE_RETIRED_8  VICEIOCTL(8)
#define VICE_RETIRED_9  VICEIOCTL(9)
#define VICE_RETIRED_10 VICEIOCTL(10)
#define VICE_RETIRED_11 VICEIOCTL(11)
#define VICE_RETIRED_24 VICEIOCTL(24)
#define VICE_RETIRED_25 VICEIOCTL(25)

#define	VICE_PAGES	20

/*
 * Data structures and #defines for VICE_PUT
 */
#define	VICE_NARGS	16	/* Host can pass 16 4 byte args to MSP/BSP */
#define VICE_OVERLAY_HDR_SIZE 16	/* 16 bytes for header */

struct vice_request {
	int	vr_stat[4]; /* return: 1st 4 words of Data RAM */
	int	vr_flags;  /* always want flags... */
	int	vr_clocks; /* return: VICE clocks ticks used by app */

	int	vr_msp_offset;
	int	vr_msp_base_address;
	int	vr_msp_start_address;
	int	vr_msp_length;

	int 	vr_bsp_base_address;
	int 	vr_bsp_start_address;
	int 	vr_bsp_length;

	int	vr_bsp_table_length;

	int	vr_dram_base_address;
	int	vr_dram_length;
};
#define	vr_status vr_stat[0]
#define	vr_morestatus vr_stat[1]

/*
 * Bits indicate what of these exist in the memory buffer described by VICE_ETC
 */
#define	VICE_USE_MSP		0x0001
#define	VICE_USE_BSP		0x0002
#define	VICE_USE_BTBL		0x0004
#define	VICE_USE_ATOMARGS	0x0008
#define	VICE_RELOAD_NOTREQUIRED	0x0010	/* Leave 0 to force reload per atom */
#define	VICE_LOAD_ETC_DRAM	0x0020	/* hack to _not_ use atom args in dms */
#define	VICE_CLOCKS_STAMP	0x0040	/* vr_clocks stamped at viceintr */
#define	VICE_DIAG_NO_DMA_RESET	0x1000


#define	VICE_NBPP	4096

/*
 * Data structures and #defines for VICE_SETTLB
 */
#define VICE_OVERLAY_HEADER	512

#define	VICE_ETC		127	/* MSP, BSP and BSP Table here */
/* offset and size (in 4kb pages) of stuff in the "etc" entry: */
#define	VICE_ETC_NBPP		VICE_NBPP

#define	VICE_ETC_MSP		0
#define	VICE_ETC_MSP_PAGES	11
#define	VICE_ETC_MSP_BYTES	(VICE_ETC_MSP_PAGES * VICE_ETC_NBPP)

#define	VICE_ETC_DRAM		11
#define	VICE_ETC_DRAM_PAGES	2
#define	VICE_ETC_DRAM_BYTES	(VICE_ETC_DRAM_PAGES * VICE_ETC_NBPP)

#define	VICE_ETC_BSP		13
#define	VICE_ETC_BSP_PAGES	1
#define	VICE_ETC_BSP_BYTES	(VICE_ETC_BSP_PAGES * VICE_ETC_NBPP)

#define	VICE_ETC_BTBL		14
#define	VICE_ETC_BTBL_PAGES	2
#define	VICE_ETC_BTBL_BYTES	(VICE_ETC_BTBL_PAGES * VICE_ETC_NBPP)

#define	VICE_ETC_DRAMP(e) (int *)((char *)e + VICE_ETC_DRAM * VICE_ETC_NBPP)
#define	VICE_ETC_MSPP(e)  (int *)((char *)e + VICE_ETC_MSP * VICE_ETC_NBPP)
#define	VICE_ETC_BSPP(e)  (short *)((char *)e + VICE_ETC_BSP * VICE_ETC_NBPP)
#define	VICE_ETC_BTBLP(e) (int *)((char *)e + VICE_ETC_BTBL* VICE_ETC_NBPP)

#define	VICE_MAXTILES	128
#define	VICE_TILE_BASE	0x40000000

/*
 * Define mappings between VICE TLB entries and buffer id.
 */
typedef int tlbent_t;
typedef int tile_t;

/* Arg to VICE_SETTLB */
struct vice_tlbop {
	tlbent_t	vt_entry;
	tile_t  	vt_pmembufn;
	int		vt_nentries;
};

/* Arg to VICE_PUT_VADDR */
struct vice_vaddr_tlbop {
	struct vice_tlbop	vvt_vt;
	void			*vvt_uaddr;
	int			vvt_offset;	/* from etc_dram_base */
	int			vvt_nbytes;
};

struct vice_rt {
	struct vice_request	rt_req;
	struct vice_vaddr_tlbop	rt_tlbop;
};

/*
 * vice_put_vaddr() is quite strict about TLB mappings
 */
/* The input uaddr+len must be mapped in with in the first 49 entries */
#define	VICE_VPV_MAXIN	49
/* The output DMbuffer must be mapped started here: */
#define	VICE_VPV_DMBENT	(VICE_VPV_MAXIN+1) /* DMB_ENT in CRIME vice_pixel.c */

/*
 * Translate VICE DMA TLB entry number to VICE "virtual" address.
 * This is the brand of value to place in the DMA SMEM register(s).
 */
#define	VICE_VADDR(te)	(VICEMSP_SYS_BASE + (te) * TILE_SIZE)

/*
 * This describes the VICE "process" that is run for each DMS Event.
 * A VICE "process" is described by:
 *	1) MSP/BSP instructions, tables, and arguments (struct vice_request)
 *	2) TLB mappings to tiles in DMS Blocks (struct vice_dms_tmap[2])
 *	3) TLB mappings to other tiles (set using vice_settlb() )
 */

struct vice_dms_proc {
	struct vice_request 	vp_vr;
	int			vp_outgap;
	tile_t			vp_auxtile;
};

/* This assumes 16 words (64 bytes) of DMS event local data */
#define	VICE_DMS_INTR		10
#define	VICE_DMS_CLOCKS		11
#define	VICE_DMS_STATUS3	12
#define	VICE_DMS_STATUS2	13
#define	VICE_DMS_STATUS1	14
#define	VICE_DMS_STATUS0	15
#define	VICE_DMS_STATUS		VICE_DMS_STATUS0
#define	VICE_DMS_MSTATUS	VICE_DMS_STATUS1
#define	VICE_DMS_EMSTATUS	VICE_DMS_STATUS2

/*
 * Here's where input and output blocks are mapped when using DMS
 */
#define	VICE_DMS_IN		0
#define	VICE_DMS_OUT		64
#define	VICE_DMS_AUX		126

#define	VICE_DMS_TYPEJ		1
#define	VICE_DMS_THRU		2
#define	VICE_DMS_TYPEM		3

#define VICE_INTR_CLOSE         0xf000
#define VICE_INTR_TIMEDOUT      0xe000

#endif /* !  __SYS_VICE_DRV_H__ */
