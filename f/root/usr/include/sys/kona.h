/************************************************************************** 
 *									  *
 * 		 Copyright (C) 1994, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
	kona.h	$Revision: 1.71 $	$Date: 1998/12/10 22:28:00 $

	Interface definitions needed by both the kernel and user-level code.
*/

#ifndef _KONA_H_
#define _KONA_H_

/*
	BEGIN needed by gfx.h -- the following includes are needed by,
	but not included by gfx.h.  They should be removed from here
	when gfx.h and rrm.h are fixed.
*/
#if defined(_LANGUAGE_C) || defined(__cplusplus)
/* Needed by gfx.h	*/
#include <sys/types.h>
#ifdef	_KERNEL

/* Needed by rrm.h which is included in gfx.h	*/
#include <sys/sema.h>
#endif	/* _KERNEL	*/
/*	END of needed by gfx.h	*/

#include <sys/gfx.h>
#include <sys/types.h>
#endif

#include <sys/kona_if.h>

#ifdef IP19
#define KONA_BOARDNAME		"KONA"		/* Infinite Reality - R4400	*/
#endif

#ifdef IP21
#define KONA_BOARDNAME		"KONAT"		/* Infinite Reality - TFP	*/
#endif

#ifdef IP25
#define KONA_BOARDNAME		"KONAS"		/* Infinite Reality - SHIVA	*/
#endif

#ifdef IP27
#define KONA_BOARDNAME		"KONAL"		/* Infinite Reality - SN0	*/
#endif

/*
	Pipe death reasons.  One of these reasons will be passed to
	kona_die for each unique reason that it is called.  Kona_die
	stores the reason code in bdata->dead for later retrieval by
	kpm.  The value 0 (KD_WORKING) is special in that it
	indicates to the kernel that the pipe has been initialized
	and is in a working state.
*/
#define	KD_WORKING			 0
#define	KD_NEVER_INITIALIZED		 1	/* initial state set in kona_alloc_bdata	*/
/* the real death reasons begin here	*/
#define	KD_CONTEXT_DEACT_TIMEOUT	 2
#define	KD_CONTEXT_VERIFY_FAIL		 3
#define	KD_IDMA_TIMEOUT			 4
#define	KD_SELECTFEED_TIMEOUT		 5
#define	KD_HIWATER_TIMEOUT		 6
#define	KD_GFIFO_CLOGGED_NO_TBUS_IO	 7
#define	KD_GFIFO_CLOGGED_WITH_TBUS_IO	 8
#define	KD_SETTING_TIMEOUT_FAILED	 9
#define	KD_GFIFO_OVERFLOWED		10
#define	KD_ODMA_OVERFLOWED		11
#define	KD_XG_ERROR			12
#define	KD_PIO_OVERFLOWED		13
#define	KD_IDMA_OVERFLOWED		14
#define	KD_MEMORY_SWAPIN_TIMEOUT	15
#define	KD_MEMORY_SWAPOUT_TIMEOUT	16
#define KD_MOPUP_TIMEOUT		17
#define KD_CHECKPIPE_TIMEOUT		18
#define KD_DEBUG_REQUEST		19
#define	KD_CONTEXT_ACT_TIMEOUT	 	20
#define KD_MAPRAM_VERIFY_FAILED		21
#define KD_PARITY_ERROR			22
#define KD_GFIFO_ABOVE_HI_WATER		23
#define KD_GFX_CREDITS_STUCK_BELOW_BIAS	24

/*
	Offsets of the user-accessable portions of the KONA graphics region.
	All these offsets are relative to the attach address.

	There are three primary sub-regions used for communicating with
	the pipe that are mapped by the kernel directly into a thread's
	address space:

		The Graphics FIFO.
		The HW Registers.
		The return data (rdata) memory area.

	These addresses should be independant of CPU board type.
*/

/* graphics FIFO for commands and data.				*/
#define	KONA_MAP_PIPE_OFFSET			0
#define KONA_MAP_PIPE_OFFSET_END		0x3fff
#define	KONA_MAP_FIFO_SUBREGION_SIZE		(KONA_MAP_PIPE_OFFSET_END - KONA_MAP_PIPE_OFFSET + 1)

/* HIP registers -- diags read/write, read-only otherwise.	*/
#define	KONA_MAP_REGS_OFFSET			0x4000
#define	KONA_MAP_REGS_OFFSET_END		0x7fff
#define	KONA_MAP_REGS_SUBREGION_SIZE		(KONA_MAP_REGS_OFFSET_END - KONA_MAP_REGS_OFFSET + 1)
#define	KONA_MAP_HIP1_REGS_SUBREGION_SIZE	KONA_MAP_REGS_SUBREGION_SIZE

/* rdata -- memory for storing data returned from the pipe.	*/
#define	KONA_MAP_RDATA_OFFSET			0x8000
#define	KONA_MAP_RDATA_OFFSET_END		0xbfff
#define	KONA_MAP_RDATA_SUBREGION_SIZE		(KONA_MAP_RDATA_OFFSET_END - KONA_MAP_RDATA_OFFSET + 1)

/* F chip registers -- for diags only	*/
#define	KONA_MAP_F_REGS_OFFSET			0xc000
#define	KONA_MAP_F_REGS_OFFSET_END		0xffff
#define	KONA_MAP_F_REGS_SUBREGION_SIZE		(KONA_MAP_F_REGS_OFFSET_END - KONA_MAP_F_REGS_OFFSET + 1)

/* XG chip registers -- for diags only	*/
#define	KONA_MAP_XG_REGS_OFFSET			0xc000
#define	KONA_MAP_XG_REGS_OFFSET_END		(KONA_MAP_XG_REGS_OFFSET+0x1fffff)
#define	KONA_MAP_XG_REGS_SUBREGION_SIZE		(KONA_MAP_XG_REGS_OFFSET_END - KONA_MAP_XG_REGS_OFFSET + 1)

#ifdef HIP1
#define KONA_MAP_IO_REGS_OFFSET			KONA_MAP_F_REGS_OFFSET
#define KONA_MAP_IO_REGS_OFFSET_END		KONA_MAP_F_REGS_OFFSET_END
#define KONA_MAP_IO_REGS_SUBREGION_SIZE		KONA_MAP_F_REGS_SUBREGION_SIZE
#endif

#ifdef HIP2
#define KONA_MAP_IO_REGS_OFFSET			KONA_MAP_XG_REGS_OFFSET
#define KONA_MAP_IO_REGS_OFFSET_END		KONA_MAP_XG_REGS_OFFSET_END
#define KONA_MAP_IO_REGS_SUBREGION_SIZE		KONA_MAP_XG_REGS_SUBREGION_SIZE
#endif

/* Size for the total region.  Note dependency of order of subregions.	*/
#define	KONA_MAP_SIZE		(KONA_MAP_RDATA_OFFSET_END - KONA_MAP_PIPE_OFFSET)
#define	KONA_DIAG_MAP_SIZE	(KONA_MAP_IO_REGS_OFFSET_END - KONA_MAP_PIPE_OFFSET)


/* private ioctl numbers */
#define KONA_IOCTL_BASE			10000
#define KONA_DMAREAD			(KONA_IOCTL_BASE + 0)
#define KONA_DMAWRITE			(KONA_IOCTL_BASE + 1)
#define KONA_START_SELECTFEED		(KONA_IOCTL_BASE + 2)
#define KONA_FINISH_SELECTFEED		(KONA_IOCTL_BASE + 3)
#define KONA_SELECT_CURSOR		(KONA_IOCTL_BASE + 5)
#define KONA_SETCONFIGINFO		(KONA_IOCTL_BASE + 6)
#define KONA_DMAPOOL_OLD		(KONA_IOCTL_BASE + 7)
#define KONA_DEBUG_IOCTL		(KONA_IOCTL_BASE + 8)
#define KONA_SET_CHANNEL_RECT		(KONA_IOCTL_BASE + 9)
#define KONA_MEM_ALLOC			(KONA_IOCTL_BASE + 10)
#define KONA_MEM_FREE			(KONA_IOCTL_BASE + 11)
#define KONA_GET_XY_CHANNEL		(KONA_IOCTL_BASE + 12)
#define KONA_MEM_REALLOC		(KONA_IOCTL_BASE + 14)
#define KONA_SHARE			(KONA_IOCTL_BASE + 15)
#define KONA_HEALTH			(KONA_IOCTL_BASE + 16)
#define KONA_CHECK_PIPE			(KONA_IOCTL_BASE + 17)
#define KONA_DMAPOOL			(KONA_IOCTL_BASE + 18)
#define KONA_GET_NCLOPS_TOPOLOGY	(KONA_IOCTL_BASE + 19)
#define KONA_NCLOPS_CONFIGURE		(KONA_IOCTL_BASE + 20)
#define KONA_NCLOPS_QUERY_CONFIG	(KONA_IOCTL_BASE + 21)
#define KONA_NCLOPS_BIND		(KONA_IOCTL_BASE + 22)
#define KONA_NCLOPS_UNBIND		(KONA_IOCTL_BASE + 23)
#define KONA_NCLOPS_DESTROY_CONFIG	(KONA_IOCTL_BASE + 24)

/* regular ioctl's shouldn't go higher than 99 */

#define KONA_DIAG_IOCTL_BASE		(KONA_IOCTL_BASE + 100)		/* actual ioctl's defined in kona_diag.h	*/


/*
	rdata area
*/

#if defined(_LANGUAGE_C) || defined(__cplusplus)
typedef struct _kona_rdata {
	int			valid;				/* return area is valid				*/
	int			count;
	int			data[(KONA_MAP_RDATA_SUBREGION_SIZE/sizeof(int))-4];	/* XXX -4 4 other words?*/
	int			instruments_overflow;	/* use to detect overflow			*/
	int			pad;				/* make sure this is double word aligned	*/
} kona_rdata;
#endif

#if defined(_LANGUAGE_C) || defined(__cplusplus)


/*
	KONA Ioctl args
*/

/*
	The kona_dmavec structure is a dma "vector" that describes
	a chunk of memory that we will dma into.  We of course need
	to know the base address of the memory and the length of
	total dma.  To properly handle strided dma (which kona supports)
	we also need to know the length of each line and the padding
	or "stride" to add at the end of line to get back to the beginning
	of the next line.  All values are in bytes.

	The kona_dmaread_args and kona_dmawrite_args structures make
	use of this building block to describe their respective dma's.
*/
typedef struct kona_dmavec {
	caddr_t base;		/* user virtual address of data block				*/
	uint	len;		/* total length of dma in bytes					*/
	uint	llen;		/* length of each line in bytes (or == len if irrelevant)	*/
	uint	stride; 	/* amount to add in bytes at the end of each line		*/
} kona_dmavec_t, kona_dmavec;

#if _MIPS_SIM == _ABI64 && defined(_KERNEL)
typedef struct abi32_kona_dmavec {
	app32_ptr_t	base;	/* user virtual address of data block				*/
	uint		len;	/* length of data block in bytes				*/
	uint		llen;	/* length of each line in bytes (or == len if irrelevant)	*/
	uint		stride;	/* amount to add in bytes at the end of each line		*/
} abi32_kona_dmavec_t, abi32_kona_dmavec;
#endif


typedef struct kona_dmaread_args {
	kona_dmavec	dmavec;		/* dma vector describing how to do the dma		*/
	int		hip_data[32];	/* buffer for HIP cmds/data				*/
	int		hip_cmdcnt;	/* how many commands in buffer				*/
} kona_dmaread_args_t, kona_dmaread_args;

#if _MIPS_SIM == _ABI64 && defined(_KERNEL)
typedef struct abi32_kona_dmaread_args {
	abi32_kona_dmavec	dmavec;		/* dma vector describing how to do the dma	*/
	int			hip_data[32];	/* buffer for HIP cmds/data			*/
	int			hip_cmdcnt;	/* how many commands in buffer			*/
} abi32_kona_dmaread_args_t, abi32_kona_dmaread_args;
#endif

typedef struct kona_dmawrite_args {
	struct kona_dmavec	*tptr;	/* table of dmavec's, one dma happens for each		*/
	uint			tlen;	/* how many entries in the table			*/
	int			hip_data[32];	/* buffer for HIP cmds/data			*/
	int			hip_cmdcnt;	/* how many commands in buffer			*/
} kona_dmawrite_args_t, kona_dmawrite_args;

#if _MIPS_SIM == _ABI64 && defined(_KERNEL)
typedef struct abi32_kona_dmawrite_args {
	app32_ptr_t	tptr;	/* (struct kona_dmavec *)					*/
	uint		tlen;
	int		hip_data[32];	/* buffer for HIP cmds/data			*/
	int		hip_cmdcnt;	/* how many commands in buffer			*/
} abi32_kona_dmawrite_args_t, abi32_kona_dmawrite_args;
#endif

/*
	kona_start_selectfeed_args is really just a
	dmavec structure (defined above) plus one more integer
*/
typedef struct kona_start_selectfeed_args {
	struct kona_dmavec dmavec;
        int snapshot;
} kona_start_selectfeed_args_t, kona_start_selectfeed_args;


#if _MIPS_SIM == _ABI64 && defined(_KERNEL)
typedef struct abi32_kona_start_selectfeed_args {
	struct abi32_kona_dmavec dmavec;
	int snapshot;
} abi32_kona_start_selectfeed_args_t, abi32_kona_start_selectfeed_args;
#endif


typedef struct kona_finish_selectfeed_args {
	int wordcount;		/* number of words transferred	*/
	int overflow;		/* buffer overflow flag		*/
} kona_finish_selectfeed_args_t, kona_finish_selectfeed_args;

/* 
 * KONA_SELECT_CURSOR ioctl structures 
 */
typedef struct kona_select_cursor_args {
	int		mode;		/* on/off/xhair/glyph		*/
#define KONA_CURSOR_OFF		0	/* this turns cursor off	*/
#define KONA_CURSOR_XHAIR	1	/* this enables a cursor xhair	*/
#define KONA_CURSOR_GLYPH	2	/* this enables a cursor glyph	*/
	/*
	 * the below is used only when mode is KONA_CURSOR_GLYPH
	 */
	int		addr;		/* cursor sddress		*/
	int		hotx;		/* x component of hot spot	*/
	int		hoty;		/* y component of hot spot	*/
} kona_select_cursor_args_t, kona_select_cursor_args;


typedef struct kona_dmapool_args {
	void	 *addr;		/* user buffer virtual address	*/
	unsigned len;		/* length of buffer		*/
	unsigned type;		/* type of dma pool		*/
	unsigned share;		/* is this dma pool shared?	*/
} kona_dmapool_args_t, kona_dmapool_args;

#if _MIPS_SIM == _ABI64 && defined(_KERNEL)
typedef struct abi32_kona_dmapool_args {
	app32_ptr_t	addr;	/* user buffer virtual address	*/
	unsigned	len;	/* length of buffer		*/
	unsigned	type;	/* type of dma pool		*/
	unsigned	share;	/* is this dma pool shared?	*/
} abi32_kona_dmapool_args_t, abi32_kona_dmapool_args;
#endif


/*
 * Structure returned by ioctl GETBOARDINFO when executed on a KONA.
 */
typedef struct kona_info {
	struct gfx_info 	gfx_info; 	/* device independent information */
	struct kona_config	config_info;	/* Kona config info (from kona_if.h) */
} kona_info_t;

/*
	Structure used by the kona debugging ioctl.
	See kona_debug.[ch] for details.

	XXX The fixed-size strings below are a hack.
	Fix them to take real char *'s.  The only
	trouble is that it's a pain to copyin all
	those pieces.
*/
typedef struct kona_debug_ioctl_args {
	char 		routine[128];	/* Name of the routine		*/
	char 		action[32];	/* action to be performed	*/
	unsigned	arg;		/* numeric argument for action. */
} kona_debug_ioctl_args_t;

/* 
 * KONA_SET_CHANNEL_RECT ioctl structures 
 */
 
#define KONA_SET_CHANNEL_RECT_INVALID		0	/* channel doesn't get a cursor	*/
#define KONA_SET_CHANNEL_RECT_VALID		1	/* channel gets xmap cursor	*/
#define KONA_SET_CHANNEL_RECT_VALID_DAC_CURSOR	2	/* channel uses DAC cursor	*/

#define KONA_SET_CHANNEL_RECT_MIN_PRIORITY	-100
#define KONA_SET_CHANNEL_RECT_MAX_PRIORITY	 100

typedef struct kona_set_channel_rect_args {
	char		channel_num;
	char		valid;		/* is this channel active? 2 == use DAC cursor	*/
	signed char	priority;	/* higher priority channels get the cursor	*/
	short		x1, y1, x2, y2;	/* channel footprint				*/
	/* the following are only needed for DAC cursor mode				*/
	signed char	dac_hotx;	/* cursor hotspot				*/
	signed char	dac_hoty;
	short		dac_xout;	/* video timing output resolution		*/
	short		dac_yout;
} kona_set_channel_rect_args_t, kona_set_channel_rect_args;

/* KONA_GET_XY_CHANNEL ioctl structures */
typedef struct kona_get_xy_channel_args {
	short		x, y;		/* input:   x,y position			*/
	char		channel_num;	/* output:  channel number			*/
	short		x1, y1, x2, y2;	/* output:  channel footprint			*/
} kona_get_xy_channel_args_t;

/* KONA_MEM_ALLOC and KONA_MEM_FREE ioctl structures	*/
typedef struct kona_mem_args {
	unsigned char	type;		/* arm ge tex					*/
	unsigned char	key;		/* for multiple allocations per type		*/
	unsigned short	flag;		/* flag bits					*/
#define KONA_MEM_FLAG_GE_UNIQUE	0x0001	/*	data is unique to multiple units	*/
	unsigned int	size;		/* size in bytes				*/
	unsigned int	align;		/* alignment restriction in bytes		*/
} kona_mem_args_t;

/* KONA_SHARE ioctl structures */
typedef struct kona_share_args {
	long long share_id;		/* unique opaque sharing id			*/
} kona_share_args_t;

/* KONA_HEALTH ioctl structures */
typedef struct kona_health_args {
	int death_reason;		/* from KD_* list above (pipe death reasons)		*/
	int saved_reg_hh_intr_info;	/* state of hh_intr_info before kona_die() changed it	*/
	int saved_reg_hh_tbus_status;	/* state of hh_tbus_status before kona_die() changed it	*/
} kona_health_args_t;


#define KONA_NCLOPS_MAX_PIPES 32

typedef struct KonaHyperpipeNetworkStruct {
    unsigned int index;
    unsigned int networkId;
} KonaHyperpipeNetwork;

typedef struct kona_nclops_topology {
    int num_pipes;
    KonaHyperpipeNetwork nclops_topology[KONA_NCLOPS_MAX_PIPES];
} kona_nclops_topology_t;

typedef struct kona_nclops_pipe_config {
    int flags;
    int channel;
    int time_slice;
    int num;
}  kona_nclops_pipe_config_t;

typedef struct {
    int num_pipes;
    int nw_id;
    int hpId;
    kona_nclops_pipe_config_t pipe[KONA_NCLOPS_MAX_PIPES];
} kona_nclops_config_args_t;

typedef struct {
    int hpId;
} kona_nclops_bind_args_t;

#endif /* defined(_LANGUAGE_C) || defined(__cplusplus) */
#endif /* _KONA_H_ */
