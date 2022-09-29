/*
	Copyright (C) 1995, Silicon Graphics, Inc.
*/

#ifndef _KONA_IF_H_
#define _KONA_IF_H_

/*
   This file contains tokens that the ucode (arm & ge) and X
   server send to the kernel for various forms of communication.

   NOTE: this file has to be included in lots of different code
         including Muse code and therefore cannot include wierd stuff
*/

/*
	These defines are the values that the GE[s] will feed
	to the kernel (through the gecomm area) as various events
	take place.  If the event is in response to a request
	that the kernel made at an earlier time, you should
	be sure to cancel the corresponding timeout.
*/
#define KONA_GE_INTR_PRINT		0
#define KONA_GE_SOFT_INTR_BASE		0x100				/* reserve lo vals for primitive ucode funcs */
#define KONA_GE_INTR_SELECTFEED		(KONA_GE_SOFT_INTR_BASE+0)
#define KONA_GE_INTR_DEACT		(KONA_GE_SOFT_INTR_BASE+1)
#define KONA_GE_INTR_MEM_SWAPOUT	(KONA_GE_SOFT_INTR_BASE+2)
#define KONA_GE_INTR_MOPUP		(KONA_GE_SOFT_INTR_BASE+3)
#define KONA_GE_INTR_MEM_SUSPEND	(KONA_GE_SOFT_INTR_BASE+4)
#define KONA_GE_INTR_CHECKPIPE		(KONA_GE_SOFT_INTR_BASE+5)


/*
	These defines are the values that the ARM will pass to
	the kernel (thru the HH_ARM_MBOX) as various events
	take place.  Additional data can be transferred thu the
	mbox, and responses to the ARM can be written to the mbox.
	Of course, the responses must be expected by ARM ucode.
	use the INTR_BIT_ARM_MASK of the HH_INTR_STATUS register
	to flow control the HH_ARM_MBOX register.  If the event
	is in response to a request that the kernel made at an
	earlier time, you should be sure to cancel the corresponding
	timeout.
*/
#define	KONA_ARM_INTR_PRINT		0
#define KONA_ARM_SOFT_INTR_BASE		0x100				/* reserve lo vals for primitive ucode funcs */
#define KONA_ARM_INTR_IDMA              (KONA_ARM_SOFT_INTR_BASE + 0)   /* idma complete interrupt */
#define KONA_ARM_INTR_MEM_SWAPIN	(KONA_ARM_SOFT_INTR_BASE + 2)
#define KONA_ARM_INTR_ACT               (KONA_ARM_SOFT_INTR_BASE + 3)

#if defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS) || defined(_LANGUAGE_COBOL)

#include "sys/nic_format.h"

typedef struct kona_config {
#define BDSET_KONA            0		/* IP19/25 */
#define BDSET_HILO            1		/* IP27    */
#define BDSET_HILO_LITE       2		/* IP27    */
#define BDSET_HILO_8C         3		/* IP27 with all imp_8c's */
#define BDSET_HILO_GE16       4		/* IP27 with GE16/RM9 */
	int 		bdset;          /* type of KONA board set */
        int             gecnt;          /* # GE11s */
        int             rmcnt;          /* # of RM boards */
        int             befram;         /* size of bef fifo ram */
        int             gpi;            /* GPI cable present */
        int             channels;       /* # of logical video channels (including option board's) */
        int             vidopt;         /* video option board types (DG_GET_BOARD_CONFIG_XXX_PRESENT flags) */
        int             colmap;         /* color map ram size */
        int             extgamma;       /* external gamma/colormap map ram size */
        struct {
                int           present;        /* rm is present */
                int           texram;         /* texture memory size */
        } rm[4];
	struct {
	    int		channel_count;      /* # of channels on dg board */
	    int		channel_rev;        /* rev level of channels on dg board */
#define KONA_DGOPT_NONE 0
#define KONA_DGOPT_PAB 1
#define KONA_DGOPT_VEG 2
#define KONA_DGOPT_DVO 3 /* this name is dead I think... */
#define KONA_DGOPT_DGO 3 /* should it be DGO... */
#define KONA_DGOPT_GVO 3 /* ...or GVO? */
#define KONA_DGOPT_22C 3
#define KONA_DGOPT_DVP 4
#define KONA_DGOPT_DPLEX 5 
	    int		opt_channel_type;   /* type of optional board channel */
	    int		opt_channel_rev;    /* rev level of channel on dg option board */
	} dg;
        struct {
                int     numges;         /* # GE11s that are being used */
                int     numrms;         /* # RM4s that are being used */
                int     occmask;        /* GED Occupancy Mask being used */
                int     narrow;         /* narrow (16 bit) or wide (32 bit) GE11 -> BEF interface */
#define KONA_MAX_GES    8
                int     geids[KONA_MAX_GES]; /* the logical to physical mapping of GE ids */
                int     scrwidth;
                int     scrheight;
#define KONA_PIXEL_DEPTH_SMALL          0x0     /* large pixel */
#define KONA_PIXEL_DEPTH_MEDIUM         0x1     /* jumbo pixel */
#define KONA_PIXEL_DEPTH_LARGE          0x2     /* colossal pixel */
/*
 * imp9 (hilo-light) does not support large and extra large pixels, and
 * does support extrasmall pixels which the macho imp8/8c think are too
 * wimpy for serious consideration.
 * Thus the imp9 reuses the extralarge id to indicate extra small.
 */
#define KONA_PIXEL_DEPTH_EXTRALARGE     0x3     /* extra-colossal pixel */
#define KONA_PIXEL_DEPTH_EXTRASMALL     0x3     /* teenie weenie ones */

		int     pixeldepth;     /* small, medium, large, extra large, or extra small */
		struct kona_mem_info {
			int maxkey;		/* maximum allowable key */
			int maxalign;		/* maximum alignment requirement */
			int multiplier;		/* multiplier for swapped out size */
			int shared;		/* flag indicates if shared or not */
			int ok2copy;		/* flag indicates if copydown supported */
			int pgsz;		/* page size in bytes */
			int pgavail;		/* total pages available */

#define KONA_MEM_TYPE_ARM	0
#define KONA_MEM_ARM_KEY_ATTR_STACK	0
#define KONA_MEM_ARM_KEY_MAX		KONA_MEM_ARM_KEY_ATTR_STACK

#define KONA_MEM_TYPE_SARM	1
#define KONA_MEM_SARM_KEY_DLIST	0
#define KONA_MEM_SARM_KEY_MAX		KONA_MEM_SARM_KEY_DLIST

#define KONA_MEM_TYPE_GE	2
#define KONA_MEM_GE_KEY_PIXMAP_I2I	0
#define KONA_MEM_GE_KEY_PIXMAP_I2C	1
#define KONA_MEM_GE_KEY_PIXMAP_C2C	2
#define KONA_MEM_GE_KEY_PIXMAP_S2S	3
#define KONA_MEM_GE_KEY_CTABLE_PRECONV	4
#define KONA_MEM_GE_KEY_CTABLE_POSTCONV	5
#define KONA_MEM_GE_KEY_CTABLE_POSTMAT	6
#define KONA_MEM_GE_KEY_CTABLE_TEXTURE	7
#define KONA_MEM_GE_KEY_HISTOGRAM	8
#define KONA_MEM_GE_KEY_UNIQUE_PER_GE	9
#define KONA_MEM_GE_KEY_CONV_DEACT	10
#define KONA_MEM_GE_KEY_MAX		KONA_MEM_GE_KEY_CONV_DEACT

#define KONA_MEM_TYPE_TEX	3
#define KONA_MEM_TEX_KEY_TEX		0
#define KONA_MEM_TEX_KEY_MAX		KONA_MEM_TEX_KEY_TEX

#define KONA_MEM_TYPE_MAX	KONA_MEM_TYPE_TEX

/* enumerations for multiple DMA pools (just 1 defined so far) */
#define KONA_DMAPOOL_TYPE_DLIST	0
#define KONA_DMAPOOL_TYPE_MAX	KONA_DMAPOOL_TYPE_DLIST

		} minfo[KONA_MEM_TYPE_MAX+1];
        } sw;
	struct {
		struct {
		    int		board;
		    int		ioadap;
		    int		hip;
#define KONA_ID_HIP1A(sysconf)		(((((sysconf).id.ge.hip) & 0x0000f000) == 0x00001000) && ((((sysconf).id.ge.hip) & 0xf0000000) == 0x00000000))
#define KONA_ID_HIP1B(sysconf)		(((((sysconf).id.ge.hip) & 0x0000f000) == 0x00001000) && ((((sysconf).id.ge.hip) & 0xf0000000) == 0x10000000))
#define KONA_ID_HIP2A(sysconf)		(((((sysconf).id.ge.hip) & 0x0000f000) == 0x00002000) && ((((sysconf).id.ge.hip) & 0xf0000000) == 0x00000000))
		    int		ged;
#define KONA_ID_GED1A(sysconf)		((((sysconf).id.ge.ged) & 0xf0000000) == 0)
#define KONA_ID_GED1B(sysconf)		((((sysconf).id.ge.ged) & 0xf0000000) == 0x10000000)
		    int		bef;
#define KONA_ID_BEF1A(sysconf)		((((sysconf).id.ge.bef) & 0xf0000000) == 0)
#define KONA_ID_BEF1B(sysconf)		((((sysconf).id.ge.bef) & 0xf0000000) == 0x10000000)
		} ge;
		struct {
		    int		board;
		    int		pg;
		    int		tg;
		    int		tf[4];
#define KONA_ID_TF1A(sysconf)		((((sysconf).id.rm[0].tf[0]) & 0xf0000000) == 0)
#define KONA_ID_TF1B(sysconf)		((((sysconf).id.rm[0].tf[0]) & 0xf0000000) == 0x10000000)
		    int		tm[8];
#define KONA_ID_TM1(sysconf,r,i)	((((sysconf).id.rm[r].tm[i]) & 0xf0000000) == 0x00000000)
#define KONA_ID_TM1A(sysconf,r,i)	((((sysconf).id.rm[r].tm[i]) & 0xf0000000) == 0x10000000)
#define KONA_ID_TM1B(sysconf,r,i)	((((sysconf).id.rm[r].tm[i]) & 0xf0000000) == 0x20000000)
		    int		imp[20];
#define KONA_ID_IMP8A(sysconf,r,i)	((((sysconf).id.rm[r].imp[i]) & 0xf0000000) == 0x0)
#define KONA_ID_IMP8C(sysconf,r,i)	((((sysconf).id.rm[r].imp[i]) & 0xf0000000) == 0x50000000)
#define KONA_ID_IMP9A(sysconf,r,i)	((((sysconf).id.rm[r].imp[i]) & 0x0000f000) == 0x00009000)
		} rm[4];
		struct {
		    int		board;
		    int		fm;
		    int		xmap[4];
#define KONA_ID_XMAP8A(sysconf,num)		((((sysconf).id.dg.xmap[(num)]) & 0xf0000000) == 0)
		    int		voc[8];
#define KONA_ID_VOC1A(sysconf)           ((((sysconf).id.dg.voc[0]) & 0xf0000000) == 0)
#define KONA_ID_VOC1B(sysconf)           ((((sysconf).id.dg.voc[0]) & 0xf0000000) == 0x10000000)
		    int		dac[8];
		} dg;
	} id;
        struct {
		nic_info_t ge;		/* Number-in-a-can info for GE board. */
		nic_info_t rm[4];	/* Number-in-a-can info for RM boards. */
		nic_info_t backplane;	/* Number-in-a-can info for KCAR backplane. */
		nic_info_t tm[4];	/* Number-in-a-can info for TM boards. */
		nic_info_t ktown;	/* Number-in-a-can info for KTOWN board. */
		nic_info_t dg;          /* Number-in-a-can info for DG board. */
		nic_info_t dgopt;       /* Number-in-a-can info for DG option. */
        } nic;
} kona_config;

#define LEATHERMAGIC_2RM	0x1
#define LEATHERMAGIC_TEXPRESENT	0x2
#define LEATHERMAGIC_FORCEDFIFO	0x4
#define LEATHERMAGIC_FORCESDRAM	0x8
#define LEATHERMAGIC_BEFNOSDRAM 0x10
#define LEATHERMAGIC_GEDRROBIN	0x20
#define LEATHERMAGIC_AUTORM	0x80
#define LEATHERMAGIC_NAKEDGL	0x100

#endif

#endif /* _KONA_IF_H_ */
