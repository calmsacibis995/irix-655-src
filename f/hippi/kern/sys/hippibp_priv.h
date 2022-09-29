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
 * hippibp_priv.h
 *
 *	Header file for Bypass driver of HIPPI-Serial card.
 *	This is not for public consumption. See hippibp.h for the
 *	public interface.
 */

#ifndef __HIPPIBP_PRIV_H
#define __HIPPIBP_PRIV_H
#ident	"$Revision: 1.11 $    $Date: 1997/10/31 22:38:31 $"

#define HIPPIBP_PROTOCOL_MAJOR	3	/* Protocol version major # */
#define HIPPIBP_PROTOCOL_MINOR  2	/* Protocol version minor # */

#define HIPPIBP_PERM		0666	/* /hw/hippi/X/bypass/X file perm */

#define	HIPPIBP_UP(x)	((x)->hippibp_state & HIPPIBP_ST_CONFIGED)

/* Per device variables for HIPPI-BP (ByPass) interface
 */
#define	HIPPIBP_MAX_SMAP_PGS	4096
#define HIPPIBP_MAX_DMAP_PGS	4096
#define HIPPIBP_MAX_PORTID_PGS	HIPPIBP_MAX_PORTIDS
#define HIPPIBP_SDESQ_SIZE	16384		/* Total size of Src Desq */
#define HIPPIBP_DFREELIST_SIZE	16384		/* Total size of DFreelist */
#define	HIPPIBP_SDHEAD_SIZE	64		/* Total size of SrcDes head */

#define	HIPPIBP_MAX_PROCS	64	/* max number of processes in job */

typedef struct hippi_freemap {
		__psunsigned_t  ID;
		iopaddr_t       *pfns;
		caddr_t         vaddr;
		int             pgcnt;
} hippi_freemap_t;

typedef struct hippi_port {
		char		jobid;
		int		signo;	/* signal to send on intr */
		pid_t		pid;	/* process to send signal to on intr */
} hippi_port_t;

typedef struct hippi_io {
		int		dmacookie;
		__uint64_t	uaddr;
		__uint64_t	ulen;
		__uint32_t	uflags;
		__uint32_t	startindex;
		__uint32_t	endindex;
		pid_t		pid;	/* process which performed setup */
/* defines for mapselect */
#define	HIPPIBP_NO_SEL	0x0		/* no map (free entry) */
#define	HIPPIBP_SFM_SEL	0x01		/* source freemap */
#define	HIPPIBP_DFM_SEL	0x010		/* destination freemap */
#define	HIPPIBP_DFM_AND_SFM_SEL	0x011	/* dest and src freemap */
		char		mapselect;	/* 1 = sfreemp, 2 = dfreemap */
} hippi_io_t;

typedef struct hippi_bp_job {
		u_char		mode;		/* FREAD? FWRITE? */
		u_char		jobFlags;
		int		portmax;
		int		portused;
		__uint32_t	ddqpgs;

		struct hippi_freemap	Sfreemap;
		struct hippi_freemap	Dfreemap;
		struct hippi_freemap	portidPagemap;

		__psunsigned_t	SDesqID;
		__psunsigned_t	DFlistID;
		__psunsigned_t	MboxID;

		/* hippibp_ifields points to an array of ifield values to be
		 * used in the Hippi ByPass protocol.
		 */

		__uint32_t	*ifields;
		__uint32_t	ifieldcnt;

		/* misc fields */

		__uint32_t	authno[3];
		__uint32_t	ack_host;
		__uint32_t	ack_port;
		
		__uint32_t	hipiomap_cnt;
		struct hippi_io	*hipiomap;
#define JOBFLAG_OPEN		0x01	/* JOB is open */
#define JOBFLAG_EXCL		0x02	/* JOB is exclusive */


		/* Following two fields are so we can keep track of which
		 * proceses already have invoked add_exit_callback() for
		 * this job so we avoid duplicate callbacks.
		 */
		pid_t		callback_list[HIPPIBP_MAX_PROCS];
		int		hippibp_maxproc;

		volatile char	*hippibp_SDesqPage;
		volatile char	*hippibp_DFlistPage;
} hippi_bp_job_t;


typedef struct hippibp_vars {

	/* -------- Hw interface, shared with hps driver ---------- */
	/* These HCMD spinlocks are in hps driver's private
	 * structure. Must be held when issuing HCMD.
	 */
	int		unit;
	lock_t		*shc_slock; /* src HCMD spinlock */
	lock_t		*dhc_slock; /* dst HCMD spinlock */
	volatile struct hip_bp_hc *shc_area;   /* PIO address of src HC area. */
	volatile struct hip_bp_hc *dhc_area;   /* PIO address of dst HC area. */
	volatile void	*sbufmem;   /* PIO address of base of src SDRAM */
	volatile void	*dbufmem;   /* PIO address of base of dst SDRAM */
	void		*cookie;    /* magic token for hps cooperation */

	vertex_hdl_t	scnctpt;    /* src hw connect-pt for device vertices */
	vertex_hdl_t	dcnctpt;    /* dst hw connect-pt for device vertices */

	/* -- Own private data structures. hps has no knowledge of these: --*/
	sema_t		devsema;

	iopaddr_t	garbage_src_iopaddr;
	iopaddr_t	garbage_dst_iopaddr;

	/* Following pointers beginning src_ are to things on SRC 
	 * LINC's SDRAM. dst_ to stuff on DST LINC SDRAM. */
   	volatile iopaddr_t		*src_bp_sfreemap;
	volatile iopaddr_t		*dst_bp_dfreemap;
	volatile char			*dst_bp_dfreelist;
	volatile __uint32_t		*src_bp_ifield;
	volatile __uint64_t		*src_bp_stats;
	volatile __uint64_t		*dst_bp_stats;
	volatile char			*src_bp_sdqueue;
	volatile char			*src_bp_sdhead;

	/* Need pointer to address of bypass firmware config info.  Also
	 * we read a copy of the firmware config info so we don't need
	 * to re-read this info each time (except for dm_status, which is
	 * why we need the firmware pointer).
	 */
	volatile struct hip_bp_fw_config *src_bp_fw_conf;	/* SRC SDRAM */
	volatile struct hip_bp_fw_config *dst_bp_fw_conf;	/* DST SDRAM */

	struct hip_bp_fw_config		 cached_src_bp_fw_conf;
	struct hip_bp_fw_config		 cached_dst_bp_fw_conf;

	struct hippi_port	*hippibp_portids;
	int	hippibp_portmax;
	int	hippibp_portfree;

	/* Following defines state of HippiBP device */

#define	HIPPIBP_ST_CONFIGED	1	/* device has been configed */
#define	HIPPIBP_ST_OPENED	2	/* device has been BP opened */
#define	HIPPIBP_ST_UP		4	/* board is up */
	char	hippibp_state;

	/* Following fields may be setup from BP Config ioctl */

	char	bp_ulp;
	char	bp_maxjobs;		/* max jobs (per controller) */
	int	bp_maxportids;		/* max portids for controller */
	int	bp_maxdfmpgs;		/* max pgs in dest freemap (per job) */
	int	bp_maxsfmpgs;		/* max pgs in src freemap (per job) */
	int	bp_maxddqpgs;		/* max pgs in dest desc que (per job)*/

	struct hippi_bp_job *bp_jobs;
} hippibp_vars_t;

typedef struct hippibp_soft {
	char	vtype;
#define BPROOT	1	    /* basic root bypass vertex for cfg/stats */
#define	BPJOB	2	    /* job-specific vertex for bypass jobs */ 
	char	isclone;    /* for clone opens on BPROOT */
	char	jobid;	    /* only valid if vtype == BPJOB */

	hippibp_vars_t * hp;
} hippibp_soft_t;



#define hippibp_soft_set(v, s)	hwgraph_fastinfo_set(v, (arbitrary_info_t)s)
#define hippibp_soft_get(v)   (hippibp_soft_t *) hwgraph_fastinfo_get(v)
#define hippibp_vars_get(v)   ((hippibp_soft_t *) hwgraph_fastinfo_get(v))->hp

#ifdef LINC_BRINGUP
#define DELAY_OP	1000000    /* 1 sec wait */
#else
#define DELAY_OP	1000    /* 1 msec wait */
#endif
#define HIPPI_SRCWAIT(c)	hps_srcwait(DELAY_OP, c)
#define HIPPI_DSTWAIT(c)	hps_dstwait(DELAY_OP, c)
#define HIPPI_SRCHWOP(o, c)	hps_srchwop(o, c)
#define HIPPI_DSTHWOP(o, c)	hps_dsthwop(o, c)

#endif /* __HIPPIBP_PRIV_H */
