/**************************************************************************
 *									  *
 * 		 Copyright (C) 1996 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
/*
 * hippi_priv.h
 *
 *	Header file for xtalk HIPPI-Serial card driver.
 *	This file contains private driver structures.
 */

#ifndef __HIPPI_PRIV_H
#define __HIPPI_PRIV_H
#ident	"$Revision: 1.23 $    $Date: 1998/01/16 23:30:25 $"

/********************************************************
 *							*
 *         Driver Internals				*
 *							*
 ********************************************************/

#ifdef _KERNEL

#include "sys/PCI/linc.h"

#define ULPIND_UNUSED		255
#define ULPIND_UNBOUND		254
#define ULPIND_NOREADERS	253

/* Per device variables for HIPPI-FP interface.
 */

#define	HIPPIFP_MAX_CLONES	128  /* caution: don't exceed 256 without 
				changing hps_soft_t.cloneid from a uchar_t */

#define HIPPIFP_MAX_WRITES	4		/* how many max write()'s on
						 * d2b.
						 */

#define HIPPIFP_HEADBUFSIZE	( HIPPI_MAX_D1AREASIZE + sizeof(hippi_fp_t) + \
				  sizeof(hippi_i_t) )

#define HIPPI_DEFAULT_I		0x00000000

typedef struct hippi_vars {

	struct ifhip_vars	*ifhps_devp;

	/* hwgraph vertices: */
	vertex_hdl_t	dst_vhdl;	/* to vertex representing dst LINC */
	vertex_hdl_t	src_vhdl;	/* to vertex representing src LINC */
	vertex_hdl_t	dev_vhdl;	/* to regular device vertex */
	vertex_hdl_t	bp_vhdl;	/* to bypass device vertex */

	/* "piotrans"lated addresses: */
	volatile pci_cfg_hdr_t	*src_cfg;   /* to src LINC config space */
	volatile pci_cfg_hdr_t	*dst_cfg;   /* to dst LINC config space */
#ifdef USE_MAILBOX
	volatile __uint64_t	*src_mbox;  /* to src LINC mailbox space */
	volatile __uint64_t	*dst_mbox;  /* to dst LINC mailbox space */
#endif

	/* "piomap"ped spaces: */
	pciio_piomap_t		piomap;	    /* both LINCs share same big win */
	volatile __uint32_t	*src_lincregs;
	volatile __uint32_t	*dst_lincregs;
	volatile void		*src_bufmem;	/* to src LINC SDRAM */
	volatile void		*dst_bufmem;	/* to dst LINC SDRAM */
	volatile void		*src_eeprom;	/* to src LINC EEPROM */
	volatile void		*dst_eeprom;	/* to dst LINC EEPROM */

	/* interrupt structures: */
	pciio_intr_t		src_intr;
	pciio_intr_t		dst_intr;

	/* to various parts of SDRAMs on the card: */
	volatile struct hip_hc		*src_hc;
	volatile struct hip_hc		*dst_hc;
	volatile hippi_stats_t		*src_stat_area;
	volatile hippi_stats_t		*dst_stat_area;

	iopaddr_t		dma_addr;

	/******************* hardware interface variables *****************/
	int		unit;

	u_int		hi_state;	/* overall state bits */
#define HI_ST_UP	0x0002		/* board is actually up (not reset) */

	hippi_linc_fwvers_t hi_srcvers;	/* on-board firmware version */
	hippi_linc_fwvers_t hi_dstvers;	/* on-board firmware version */
	int		hi_hwflags;	/* current hardware operational flags*/
	int		hi_stimeo;		/* current source timeout */
	int		hi_dtimeo;		/* current dest timeout */
#define HIPPI_DEFAULT_STIMEO	20		/* roughly 5 secs */
#define HIPPI_DEFAULT_DTIMEO	20
	char		mac_addr[6];

	/* These four rings, in host memory, are our main
	 * communication channel with the card.
	 */
	volatile hip_b2h_t *hi_s2h;    /* src to host ring */
	volatile hip_b2h_t *hi_d2h;    /* dst to host ring */
	volatile hip_d2b_t *hi_d2b;    /* data to src ring */
	volatile hip_c2b_t *hi_c2b;    /* control to dst ring */

	int		hi_src_cmd_id;	/* ID of last command given to board */
	int		hi_src_cmd_line;

	int		hi_dst_cmd_id;	/* ID of last command given to board */
	int		hi_dst_cmd_line;

	hipfw_sleep_t 	   *hi_ssleep;
	hipfw_sleep_t 	   *hi_dsleep;

	/* "Data-to-board" ring management variables: */
	volatile hip_d2b_t	*hi_d2b_prod;	/* producer's next write */
	volatile hip_d2b_t 	*hi_d2b_cons;	/* consumer's next read */
	volatile hip_d2b_t 	*hi_d2b_last;	/* last elem in ring */

	/* "Control-to-board" ring management: */
	volatile hip_c2b_t *hi_c2bp;	/* current pointer for c2b */

	/* "Source-to-host" interrupt ring management variables: */
	volatile hip_b2h_t *hi_s2hp;	/* current pointer for s2h */
	volatile hip_b2h_t *hi_s2h_last;/* last elem in ring */
	u_char		hi_s2h_sn;	/* interrupt id number */

	/* "Destination-to-host" interrupt ring management variables: */
	volatile hip_b2h_t *hi_d2hp;	/* current pointer for d2h */
	volatile hip_b2h_t *hi_d2h_last;/* last elem in ring here */
	u_char		hi_d2h_sn;	/* interrupt id number */

        /* These two values are only used with the PEER_TO_PEER_DMA_WAR. */
	volatile __uint64_t *hi_src_msg_area;
	volatile __uint64_t *hi_dst_msg_area;

	/* These 6 locks are inited in hps_attach() and never freed. */
	sema_t	devsema;	/* main semaphore for opening/closing/ioctls */
	mutex_t	sv_mutex;	/* for all the sync variables and hi_state */

	/* LOCK order: the *hc_slocks are held for a very short time only.
	 * 		If you need both the _slock and the _mutex, grab
	 *		the mutex first, never the other way around.
	 */
	lock_t	dhc_slock; 	/* dest-side spinlock for all HCMD's */
	mutex_t	dst_mutex;	/* mutex lock for dst side queues: 
				   c2b, d2h ring, hi_in_sml/big[] 
				   and d2h_active */

	lock_t	shc_slock; 	/* src-side spinlock for all HCMD's */

/* XXX don't seem to need this: */
	mutex_t	src_mutex;	/* mutex lock for src-side rings and 
				 * s2h_active */

	/* Following locks are inited/freed in hps_bd_bringup()/shutdown(): */
	/* Raw output queue for board */
	sema_t	rawoutq_sema;	/* limit outstanding # of unacked writes */
	sema_t	src_sema;	/* lock for d2b_prod & rawoutq_in */

	/* Sync vars for waiting for "output-done"s */
	sema_t	rawoutq_sleep[HIPPIFP_MAX_WRITES];
	int	rawoutq_error[HIPPIFP_MAX_WRITES];
	int	rawoutq_in; 	/* next entry for user to wait on */
	int	rawoutq_out;	/* next entry acked & freed */

	/******************** character device interface variables *******/
	
	int	PHmode;	/* somebody opened for HIPIP-PH */

	/* Map ULP IDs to open ULPs */
	u_char	ulpFromId[ HIPPI_ULP_MAX+1 ];

	struct hps_soft *clone_info[ HIPPIFP_MAX_CLONES ];

	/* State info for HIPPI-FP ULPs open for reads. */
	struct hippi_fp_ulps {
		u_char	opens;		/* # of opens for read on this ulp */
		u_char	ulpFlags;
#define ULPFLAG_R_POLL		0x01	/* We are polling ULP for data */
#define ULPFLAG_R_MORE		0x02	/* Not done DMA'ing packet */

		int	ulpId;

		/* reads:  */
		int		rd_fpHdr_avail;
		int		rd_D2_avail;
		int		rd_offset;
		int		rd_count;

		/* These are protected by sv_slock, used in conjunction
		 * with hi_state */
		sv_t		rd_sv;
		int		rd_semacnt; /* for when data arrives before
					     * reader, and sv_signal returns 0
					     */
		sema_t		rd_dmadn;

		struct pollhead	*rd_pollhdp;
		hippi_fp_t 	*rd_fpd1head;
		hip_c2b_t	*rd_c2b_rdlist;
#if HPS_CALLBACK /* call back for maxstrat */
		 void(*rd_input_proc)(void *,caddr_t,int,int);
		 caddr_t rd_input_arg;
#endif
	} ulp[ HIPPIFP_MAX_OPEN_ULPS+1 ]; /* last one is HIPPI-PH ulp */

	int	hi_bringup_tries;	/* incremented on unsuccessful tries */

	/* These statistics are only maintained if compiled HPS_DEBUG */
	int     stat_s2h_calls;
	int	stat_s2h_work;
	int	stat_s2h_pokes;
	int	stat_s2h_busy;
	int     stat_d2h_calls;
	int	stat_d2h_work;
	int	stat_d2h_pokes;
	int	stat_d2h_busy;
} hippi_vars_t;

typedef struct hps_soft {
	char		isclone;
	uchar_t		cloneid;
	vertex_hdl_t	vhdl;
	hippi_vars_t	*hippi_devp;

	/* State info for HIPPI-FP clones. Only valid if isclone */
	int		ulpId;		/* ULP ID we are bound to */
	u_char		ulpIndex;	/* index into open ULP table */
	u_char		mode;		/* FREAD? FWRITE? */
	u_char		cloneFlags;
#define CLONEFLAG_W_HOLDING	0x01	/* We own the src_sema */
#define CLONEFLAG_W_PERMCONN	0x02	/* We've got a sustaining connection */
#define CLONEFLAG_W_NBOP	0x04	/* Next send won't be start of pkt */
#define CLONEFLAG_W_NBOC	0x08	/* Next send won't be start of conn */
#define CLONEFLAG_W_PC_ON	0x10	/* RR is already in perm conn mode. */
	/* writes: */
	u_int		wr_pktOutResid;	/* bytes left in big packet */
	__uint32_t	wr_Ifield;
	hippi_fp_t	wr_fpHdr;
	u_short		wr_fburst;
	u_char		src_error;

	/* reads: */
	u_int		dst_errors;
} hps_soft_t;

#define hps_soft_get(v)		(hps_soft_t *)hwgraph_fastinfo_get(v)
#define hps_soft_set(v, s)	hwgraph_fastinfo_set(v, (arbitrary_info_t)s)

/* These prototypes are for use by if_hip.c */
extern void hps_wake_src( hippi_vars_t * );
extern void hps_wake_dst( hippi_vars_t * );
extern void hps_send_c2b( hippi_vars_t *, __uint32_t , int , iopaddr_t );

extern int hps_setparams( hippi_vars_t *, int to_src, int newflags );
		/* moral equivalent of old hippi_hwflags() */

extern int hippi_ndisc_perr;  /* in master.d/hps */
extern int hps_wait_usec( hippi_vars_t *, int, int );

#ifdef USE_MAILBOX
/* These write the mailbox bit to notify fw that d2b's have been queued. */
#define hps_src_d2b_rdy(hdp) *(volatile __uint64_t*)((char*)hdp->src_mbox + \
			    (HIP_SRC_D2B_RDY_MBOX * LINC_MAILBOX_PGSIZE)) = 1;

#define hps_dst_d2b_rdy(hdp) *(volatile __uint64_t*)((char*)hdp->dst_mbox + \
			    (HIP_DST_D2B_RDY_MBOX * LINC_MAILBOX_PGSIZE)) = 1;
#endif

#ifdef LINC_BRINGUP
#define OP_CHECK        2500            /* check command done this often */
#define	DELAY_OP	1000000		/* most operations */
#else
#define OP_CHECK        25              /* check command done this often */
#define	DELAY_OP	1000		/* most operations */
#endif

#define HPS_SRCWAIT	hps_wait_usec(hippi_devp, DELAY_OP, 1)
#define HPS_DSTWAIT	hps_wait_usec(hippi_devp, DELAY_OP, 0)

#define HPS_SRCLOCK_HC 		mutex_spinlock (&hippi_devp->shc_slock)
#define HPS_SRCUNLOCK_HC(s)	mutex_spinunlock (&hippi_devp->shc_slock, (s))
#define HPS_SRCLOCK		mutex_lock (&hippi_devp->src_mutex, PZERO)
#define HPS_SRCUNLOCK		mutex_unlock (&hippi_devp->src_mutex)

#define HPS_DSTLOCK_HC 		mutex_spinlock (&hippi_devp->dhc_slock)
#define HPS_DSTUNLOCK_HC(s)	mutex_spinunlock (&hippi_devp->dhc_slock, (s))
#define HPS_DSTLOCK		mutex_lock (&hippi_devp->dst_mutex, PZERO)
#define HPS_DSTUNLOCK		mutex_unlock (&hippi_devp->dst_mutex)

#define HPS_SRCOP(c)    ( hippi_devp->hi_src_cmd_line = __LINE__,	\
			 *(__uint64_t *) &hippi_devp->src_hc->cmd = 	\
			 	(((__uint64_t) (c) << 32 ) |		\
				 ++(hippi_devp->hi_src_cmd_id)))
#define HPS_DSTOP(c)    ( hippi_devp->hi_dst_cmd_line = __LINE__,	\
			 *(__uint64_t *) &hippi_devp->dst_hc->cmd = 	\
			 	(((__uint64_t)(c) << 32 ) |		\
				 ++(hippi_devp->hi_dst_cmd_id)))

/* Find next d2b in the ring */
#define D2B_NXT(p)	( (p)+1 > hippi_devp->hi_d2b_last ? \
				&hippi_devp->hi_d2b[0] : (p)+1 )

#define HPS_PFTCH_THRESHOLD	128

/* Make d2b ring large enough that we don't have to check for wraps. */
#define HIP_D2B_LEN	((HIPPIFP_MAX_WRITES*(HIPPIFP_MAX_WRITESIZE/NBPP)) + \
			 (IFHIP_MAX_OUTQ * IFHIP_MAX_MBUF_CHAIN) + 100)

/* big enough to never be short of space
 *	max output queue + max input queue + max buffer pool
 *		+ sleep note + marker + max stack-assigns
 *		+ max raw input request
 */
/* XXX what does output queue have to do with this?
 * Shouldn't this be something like
 * #define HIP_C2B_LEN    HIP_MAX_BIG+HIP_MAX_SML+MAX_OPEN_ULPS+some_margin
 */
#define HIP_C2B_LEN	512

/* Lengths (in number of msgs) of src-to-host and dst-to-host rings */
#ifdef HIPPI_BP
#define HIP_D2H_LEN	2300
#else
#define HIP_D2H_LEN	HIP_C2B_LEN
#endif
#define HIP_S2H_LEN	200	/* XXX: shouldn't need to be
				(HIPPIFP_MAX_WRITES + IFHIP_MAX_OUTQ) */

#endif /* _KERNEL */

#endif /* __HIPPI_PRIV_H */
