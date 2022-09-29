/*
 * hippidev.h
 *
 *	Challenge/Onyx HIPPI
 *
 */

#ifndef __HIPPIDEV_H
#define __HIPPIDEV_H


#define HIPPI_BP 1
#define HIPPI_BP_DEBUG 1


#ifdef HIPPI_BP
#define HIPPIBP_PROTOCOL_MAJOR	3	/* Protocol version major # */
#define HIPPIBP_PROTOCOL_MINOR  2	/* Protocol version minor # */
#endif   /* HIPPI_BP */

/************************************************************
 *							    *
 *        HIPPI Board/Host Interface description            *
 *							    *
 ************************************************************/


/* host-to-board commands
 *	values for hip_hc.cmd
 */
enum hip_cmd {
	HCMD_NOP=0,			/* 00	do nothing */
	HCMD_INIT,			/* 01	set parameters */
	HCMD_EXEC,			/* 02	execute downloaded code */
	HCMD_PARAMS,			/* 03	set operational flags/params */
	HCMD_WAKEUP,			/* 04	card has things to do */
	HCMD_ASGN_ULP,			/* 05	assign stack to ULP */
	HCMD_DSGN_ULP,			/* 06	assign stack to ULP */
	HCMD_STATUS,			/* 07	update status flags */
	HCMD_BP_JOB,		        /* 08   define a bypass job slot */
	HCMD_BP_PORT,		        /* 09   define a bypass process slot */
	HCMD_BP_CONF,                   /* 10   configure the bypass */
	HCMD_BP_PORTINT_ACK              /* 11   ack a port interrupt */
};


struct hip_hc {

    /* host-written/board-read portion of the communcations area
     */
    __uint32_t	cmd;                    /* one of the hip_cmd enum */
    __uint32_t	cmd_id;                 /* ID used by board to ack cmd */

    union {
	__uint32_t	cmd_data[16];	/* minimize future version problems */

	__uint32_t	    exec;	/* goto downloaded code */

	struct {
		__uint32_t  flags;	/* operational flags */
#define HIP_FLAG_ACCEPT		0x01	/* accepting connections */
#define	HIP_FLAG_IF_UP 		0x02	/* accept HIPPI-LE network traffic */
#define	HIP_FLAG_NODISC		0x04	/* no disconnect on parity/LLRC err */
		int	stimeo;		/* how long until source times out */
		int	dtimeo;		/* how long until dest times out */
	} params;

	struct {			/* initialization parameters */
	    __uint32_t  b2h_buf_hi;
	    __uint32_t  b2h_buf;	/* B2H buffer */
	    __uint32_t  b2h_len;	/* B2H_LEN */
	    __uint32_t  iflags;		/* initialization flags (see above) */
	    __uint32_t  d2b_buf_hi;
	    __uint32_t  d2b_buf;
	    __uint32_t  d2b_len;	/* D2B_LEN (# of d2b's in ring) */
	    __uint32_t  host_nbpp_mlen;	/* Put (NBPP|MLEN) here */
	    __uint32_t  c2b_buf_hi;
	    __uint32_t  c2b_buf;
	    __uint32_t  stat_buf_hi;
	    __uint32_t  stat_buf;	/* HCMD_FET_STATS */
	} init;

        struct {
	  __uint32_t  enable;	        /* enable/disable job  */
	  __uint32_t  job;		/* job slot number */
	  __uint32_t  fm_entry_size;	/* size of freemap entries (bytes) */
	  __uint32_t  auth[3];	        /* authentication number for job */
	  __uint32_t  ack_host;		/* host index for ACK field in pkt */
	  __uint32_t  ack_port;		/* port that should receive ACKs */
	} bp_job;

	struct {
	  union {
	    __uint32_t i;
	    struct {
	      __uint32_t opcode      :  4; /*  */
#define   HIP_BP_PORT_DISABLE       0
#define   HIP_BP_PORT_NOPGX         1
#define   HIP_BP_PORT_PGX           2
	      __uint32_t unused      : 12;
	      __uint32_t init_pgx    : 16;
	    }  s;
	  } ux;
	  __uint32_t  job;		/* job this port is attached to */
	  __uint32_t  port;		/* bypass destination buffer index */
	  __uint32_t  ddq_hi;	        /* physical address of base of dest desc queue */
	  __uint32_t  ddq_lo;
	  __uint32_t  ddq_size;	        /* size of a dest desc queue (bytes)*/

	} bp_port;
	struct {
	  __uint32_t  ulp;	        /* enable/disable job  */
	} bp_conf;
	struct {
	  __uint32_t	portid;		/* portid interrupt to ack */
	  __uint32_t	cookie;		/* interrupt cookie from FW */
	} bp_portint_ack;

    } arg;



    /* host-read/board-written part of the communications area
     */

    volatile __uint32_t sign;		/* board signature */
#define HIP_SIGN	0xBeadFace	/* HIPPI, beads, get it?!? */

    volatile __uint32_t vers;		/* board/eprom version */
    /*			((((yy-92)*13 + m)*32 +  d)*24 + hr)*60 + minute */
#define HIP_MIN_VERS	((((92-92)*13 + 7)*32 + 22)*24 + 0)*60 + 0
#define HIP_VERS_MASK	0x00ffffff
#define HIPPI_VERS_OFFS	0x30		/* where to find it in EEPROM */

    volatile __uint32_t inum;		/* ++ on board-to-host interrupt */

    volatile __uint32_t cmd_ack;	/* ID of last completed command */

    volatile union {
	__uint32_t    cmd_res[16];	/* minimize future version problems */
    } res;
};


/* board-to-host DMA
 */
typedef struct hip_b2h {
	u_char	b2h_sn;
	u_char	b2h_op;			/* operation,stack */
	union {
		u_short	b2hu_s;
		struct {
			u_char	b2hi_pages;
			u_char	b2hi_words;
		} b2h_in;
		struct {
			u_char	b2hod_status;
			u_char	b2hod_n;
		} b2h_odone;
		struct {
			u_short	portid;
		} b2h_bp_portint;
	} b2hu;
	__uint32_t	b2h_l;
} hip_b2h_t;

#define b2h_s		b2hu.b2hu_s
#define b2h_pages	b2hu.b2h_in.b2hi_pages
#define b2h_words	b2hu.b2h_in.b2hi_words
#define b2h_ndone	b2hu.b2h_odone.b2hod_n
#define b2h_ostatus	b2hu.b2h_odone.b2hod_status

/* This is how the board stuffs info into a HIPPI-LE b2h */
#define B2H_LE_LEN(bp)		((int)( ((bp)->b2h_l >> 14) & 0xfffc))
#define B2H_LE_CKSUM(bp)	((__uint32_t)( (bp)->b2h_l & 0xffff))

#define B2H_OSTAT_GOOD	0
#define B2H_OSTAT_SEQ	1	/* HIPPI sequence error */
#define	B2H_OSTAT_DSIC	2	/* DSIC lost */
#define B2H_OSTAT_TIMEO	3	/* connection timed-out */
#define B2H_OSTAT_CONNLS 4	/* connection lost */
#define B2H_OSTAT_REJ	5	/* connection rejected */
#define B2H_OSTAT_SHUT	6	/* interface shut down before sent */
#define B2H_OSTAT_SPAR	7	/* source parity error (not good) */

/* These appear is short portion of b2h on a receive */
#define B2H_ISTAT_I	0x8000	/* I-field tacked on the front of header */
#define B2H_ISTAT_MORE	0x4000	/* we haven't found the end of packet yet */

/* These are in bits [7:0] of long portion of b2h on a receive if [31:8]
 * are all ones. */
#define B2H_IERR_PARITY		0x01	/* Destination parity error */
#define B2H_IERR_LLRC		0x02	/* Destination LLRC error */
#define B2H_IERR_SEQ		0x04	/* Destination sequence error */
#define B2H_IERR_SYNC		0x08	/* Destination sync error */
#define B2H_IERR_ILBURST	0x10	/* Destination illegal burst error */
#define B2H_IERR_SDIC		0x20	/* Destination SDIC lost error */

#define HIP_B2H_OPMASK	0xF0
#define HIP_B2H_STMASK	0x0F		/* "stack" id-- */
#define HIP_B2H_NOP	(0<<4)
#define HIP_B2H_SLEEP	(1<<4)		/* board has run out of work */
#define HIP_B2H_ODONE	(2<<4)		/* output DMA commands finished */
#define HIP_B2H_IN	(3<<4)		/* input data is available */
#define HIP_B2H_IN_DONE	(4<<4)		/* input DMA is done */
#define HIP_B2H_BP_PORTINT	(5<<4)	/* BP interrupt for a port */


#if HIPPI_BP
#define HIP_B2H_LEN	2300		/* XXX: pulled out of air. */
#else
#define HIP_B2H_LEN	300		/* XXX: pulled out of air. */
#endif

#define	HIP_PAD_IN_DMA	24		/* input padding added by board */

#define NBCL		128		/* bytes in Everest cache line */
#define HIPPI_DST_THRESH 128		/* DST_OP_THRESH in firmware.  This
					 * is the number of bursts before
					 * data gets pushed up to the host
					 * (if PACKET line stays high).
					 */


/* host-control-to-board DMA
 */
typedef union hip_c2b {
	struct {
		u_short	c2bs_param;	/* usually length */
		u_char	c2bs_op;
		u_char	c2bs_addrhi;	/* bits 39-32 of phys address */
		__uint32_t	c2bs_addr;
	} c2bs;
	__uint32_t	c2bl[2];
	__uint64_t	c2bll;
} hip_c2b_t;
#define c2b_param	c2bs.c2bs_param
#define c2b_op		c2bs.c2bs_op
#define c2b_addrhi	c2bs.c2bs_addrhi
#define c2b_addr	c2bs.c2bs_addr

#define HIP_C2B_OPMASK		0xF0
#define HIP_C2B_STMASK		0x0F
#define HIP_C2B_EMPTY   	(0*16)
#define HIP_C2B_SML		(1*16)		/* add (hdr) mbuf to pool */
#define HIP_C2B_BIG		(2*16)		/* add (data) page to pool */
#define	HIP_C2B_WRAP    	(3*16)		/* back to start of buffer */
#define HIP_C2B_READ		(4*16)		/* post large read to ULP */

/* big enough to never be short of space
 *	max output queue + max input queue + max buffer pool
 *		+ sleep note + marker + max stack-assigns
 *		+ max raw input request
 */
#define HIP_C2B_LEN	512

/* host-data-to-board DMA
 */
typedef union hip_d2b {
	struct hip_d2b_sg {
		u_short	len;		/* length of chunk */
		u_char	flags;		/* not used in chunks */
		u_char	addrhi;		/* bits 39-32 of address */
		__uint32_t	addr;		/* host address */
	} sg;
	struct hip_d2b_hd {
		u_short	chunks;		/* number of chunks */
		u_char flags;		/* start-of-request flag */
		u_char stk;		/* ulp stack */
		u_short	fburst;		/* size of first burst */
		u_short sumoff;		/* merge checksum to this offset */
	} hd;
	__uint32_t	l[2];
	__uint64_t	ll;
} hip_d2b_t;

/* flags in header */
#define HIP_D2B_RDY	0x80		/* in first hd.start of DMA string */
#define HIP_D2B_BAD	0xc0		/* start of un-ready DMA string */
#define HIP_D2B_IFLD	0x20		/* I-field is 1st word in 1st chunk */
#define HIP_D2B_NEOC	0x10		/* don't drop connection at end */
#define HIP_D2B_NEOP	0x08		/* don't drop packet line at end */
#define HIP_D2B_NACK	0x04		/* don't acknowledge transmit */

/* HIPPI DMA "stack" numbers.
 */
#define HIP_STACK_LE		0	/* ifnet TCP/IP interface */
#define HIP_STACK_IPI3		1	/* IPI-3 command set driver */
#define HIP_STACK_RAW		2	/* HIPPI-PH interface */
#define HIP_STACK_FP		3	/* HIPPI-FP ULP device interface */
#define HIP_N_STACKS		16

#define HIP_D2B_LEN	2560		/* enough for all possible writes */

#define OP_CHECK        25              /* check command done this often */
#define	DELAY_OP	1000		/* most operations */



#define HIPPI_GET_FIRMVERS	('h'<<8|65)
#define HIPPI_GET_DRVRVERS	('h'<<8|66)
#define HIPPI_PGM_FLASH		('h'<<8|67)


struct hip_dwn {			/* parameter for HIPPI_PGM_FLASH */
	__uint32_t	addr;
	__uint32_t	len;
	__uint32_t	vers;
};




/********************************************************
 *							*
 *         Driver Internals				*
 *							*
 ********************************************************/

#ifdef _KERNEL

/*
 * Minor Numbers:
 *
 * non-clone (/dev/hippiN minors):
 *
 *	 1    1 1 1
 *       5    2 1 0 9 8 7      0
 *	+------+-+-+-+-+--------+
 *	| unit | |b|f|0|  ulp   |
 *	+------+-+-+-+----------+
 *		    ^ f: 0=hippi-ph, 1=hippi-fp (if autobind)
 *		  ^   b: 1=auto-bind 0=normal
 *
 * clone (internal only)
 *
 *	 1    1 1 1
 *       5    2 1 0 9 8 7      0
 *	+------+-+-+-+----------+
 *	| unit | |b|1|1|cloneid |
 *	+------+-+-+-+----------+
 */

#define UNIT_FROM_MINOR( m )		((m)>>12)
#define CLONEID_FROM_MINOR( m )		((m)&0x00FF)
#define ISBP_FROM_MINOR( m )		((m)&0x0800)
#define ISCLONE_FROM_MINOR( m )		((m)&0x0100)
#define ISPREBOUND_FROM_MINOR( m )	((m)&0x0400)
#define ISFP_FROM_MINOR( m )		((m)&0x0200)
#define PREBOUND_ULP_FROM_MINOR( m )	((m)&0x00FF)
#define MINOR_OF_CLONE( m, index )	( ((m)&0xF000) | 0x300 | index )
#define JOBID_FROM_MINOR( m )		((m)&0x00FF)
#define MINOR_OF_JOB( m, index )	( ((m)&0xF000) | 0x0800 | index )

#define ULPIND_UNUSED		255
#define ULPIND_UNBOUND		254
#define ULPIND_NOREADERS	253



/* How much of the VMECC Large window is mapped into IO space.
 * This should be conserved.
 */
#define HIPPI_LW_SIZE	0x01000000	/* 16 MB */


/* Per device variables for HIPPI-FP interface.
 */

#define	HIPPIFP_MAX_OPEN_ULPS	8
#define	HIPPIFP_MAX_CLONES	32

#define HIPPIFP_HEADBUFSIZE	( HIPPI_MAX_D1AREASIZE + sizeof(hippi_fp_t) + \
				  sizeof(hippi_i_t) )
#define HIPPIFP_MAX_WRITESIZE	(2*1024*1024)	/* 2 Megabytes */
#define HIPPIFP_MAX_READSIZE	(2*1024*1024)	/* 2 Megabyte */
#define HIPPIFP_MAX_WRITES	4		/* how many max write()'s on
						 * d2b.
						 */
#define HIPPI_DEFAULT_I		0x00000000

#ifdef HIPPI_BP
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
		int             *pfns;
		caddr_t         vaddr;
		int             pgcnt;
} hippi_freemap_t;

typedef struct hippi_port {
		char		jobid;
		int		signo;	/* signal to send on intr */
		pid_t		pid; /* process to send signal to on intr */
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
		int             *hipio_temppfn;
#define JOBFLAG_OPEN		0x01	/* JOB is open */
#define	JOBFLAG_EXCL		0x02	/* JOB is exclusive */


		/* Following two fields are so we can keep track of which
		 * proceses already have invoked add_exit_callback() for
		 * this job so we avoid duplicate callbacks.
		 */
		pid_t		callback_list[HIPPIBP_MAX_PROCS];
		int		hippibp_maxproc;
#ifdef HIPPI_BP_DEBUG
		/* These fields only used when hippibp_nobd is set.
		 * Allows us to perform additional testing without requiring
		 * a Hippi board.
		 */
		volatile char	*hippibp_SDesqPage;
		volatile char	*hippibp_DFlistPage;
#endif /* HIPPI_BP_DEBUG */

} hippi_bp_job_t;

 /* information from the Hippi Firmware.  This structure is located at a 
 * well-known address in the firware.
 */

struct hip_bp_fw_config {
	__uint32_t	num_jobs;	/* number of jobs supported by fw*/
	__uint32_t	num_ports;	/* number of ports supported by fw */
	__uint32_t	hostx_base;	/* base addr and size of hostx table */
	__uint32_t	hostx_size;
	__uint32_t	dfl_base;	/* base/size of destination freelist */
	__uint32_t	dfl_size;
	__uint32_t	sfm_base;	/* base/size of source freemap */
	__uint32_t	sfm_size;
	__uint32_t	dfm_base;	/* base/size of destination freemap */
	__uint32_t	dfm_size;
	__uint32_t	bpstat_base;	/* base/size of bypass status_base */
	__uint32_t	bpstat_size;
	__uint32_t	sdq_base;	/* base/size of source desc queue */
	__uint32_t	sdq_size;
	__uint32_t	bpjob_base;	/* base/size of fw job state */
	__uint32_t	bpjob_size;
	__uint32_t	dma_status;	/* page dma is currently touching*/

#define HIPPIBP_DMA_ACTIVE_SHIFT	31	/* 1 = dma is active */
#define HIPPIBP_DMA_ACTIVE_MASK		0x1

#define HIPPIBP_DMA_CLIENT_SHIFT	29	/* 1=port, 2=sfm, 3=dfm */
#define HIPPIBP_DMA_CLIENT_MASK		0x3
#define HIPPIBP_DMA_CLIENT_PORTMAP	1
#define HIPPIBP_DMA_CLIENT_SFM		2
#define HIPPIBP_DMA_CLIENT_DFM		3

#define	HIPPIBP_DMA_2PG_SHIFT		28	/* 1 => DMA spans next page */
#define	HIPPIBP_DMA_2PG_MASK		0x1
	
#define HIPPIBP_DMA_JOB_SHIFT		16	/* job that dma is for */
#define HIPPIBP_DMA_JOB_MASK		0xff
#define HIPPIBP_DMA_PGX_SHIFT		0	/* page index OR portid */
#define HIPPIBP_DMA_PGX_MASK		0xffff

        __uint32_t	reserved[11];	/* fill to 32 words */
};
#endif /* HIPPI_BP */

typedef struct hippi_vars {

	/******************* hardware interface variables *****************/
	int		unit;

	u_int		hi_state;	/* useful state bits */
#define HI_ST_SLEEP	0x0001		/* 1=C2B DMA is asleep */
#define HI_ST_UP	0x0002		/* board is actually up (not reset) */
	
	u_int		hi_firmvers;	/* firmware version */

	int		hi_hwflags;	/* current hardware operational flags*/

	u_long		hi_swin;	/* pointer to small window space */
	volatile __uint32_t	*hi_bdctl;	/* pointer to HIPPI board control reg*/
#define HIP_BDCTL_RESET_29K	0x04
#define HIP_BDCTL_29K_INT	0x02
#define HIP_BDCTL_HOST_INT	0x01

	volatile struct hip_hc		*hi_hc;
	volatile struct hippi_stats	*hi_stat_area;

#ifdef HIPPI_BP
/*
 * size of region in firmware memory to skip before starting bypass structures
 */
#define C2B_D2B_D2BREAD_SIZE       (256 + 32768 + 4096 + 16 + 384 + 100)

#define	HIPPIBP_STATS_SIZE	256

	volatile __uint32_t		*hi_bp_sfreemap;
	volatile __uint32_t		*hi_bp_dfreemap;
	volatile char			*hi_bp_dfreelist;
	volatile __uint32_t		*hi_bp_ifield;
	volatile __uint32_t		*hi_bp_stats;
	volatile char			*hi_bp_sdqueue;
	volatile char			*hi_bp_sdhead;

#endif /* HIPPI_BP */	

	volatile struct hip_b2h *hi_b2h;
	volatile union hip_d2b	*hi_d2b;
	volatile union hip_c2b	*hi_c2b;

	int		hi_cmd_id;	/* ID of last command give board */
	int		hi_cmd_line;

	volatile union hip_d2b 	*hi_d2bp_hd;	/* head pointer for d2b queue*/
	volatile union hip_d2b 	*hi_d2bp_tl;	/* tail pointer for d2b queue*/
	volatile union hip_d2b 	*hi_d2b_lim;	/* wrap here */

	volatile struct hip_b2h *hi_b2hp;	/* current pointer for b2h */
	volatile struct hip_b2h *hi_b2h_lim;	/* wrap here */
	u_char		hi_b2h_sn;		/* interrupt id number */
	u_char		hi_b2h_active;		/* flag to prevent rentry */

	volatile union hip_c2b	*hi_c2bp;	/* current pointer for c2b */

	int		hi_stimeo;		/* current source timeout */
	int		hi_dtimeo;		/* current dest timeout */
#define HIPPI_DEFAULT_STIMEO	20		/* roughly 5 secs */
#define HIPPI_DEFAULT_DTIMEO	20

	/* main mutex for opening/closing/ioctls */
	mutex_t	devmutex;

	/* main mutex/lock for interrupts, HCMD's, b2h's, etc. */
	mutex_t	devslock;

	/* Raw output queue for board */
	sema_t	rawoutq_sema;	/* limit outstanding writes */
	sema_t	src_sema;	/* lock for source and d2b/rawoutq */

	int	rawoutq_in, rawoutq_out;
	sema_t	rawoutq_sleep[HIPPIFP_MAX_WRITES];
	int	rawoutq_error[HIPPIFP_MAX_WRITES];


	/******************** character device interface variables *******/

	/* Map ULP IDs to open ULPs */
	u_char	ulpFromId[ HIPPI_ULP_MAX+1 ];

	/* Somebody receiving on HIPIP-PH? */
	int	dstLock;

	/* State info for HIPPI-FP clones. */
	struct hippi_fp_clones {

		int		ulpId;		/* ULP ID we are bound to */

		u_char		ulpIndex;	/* index into open ULP table */
		u_char		mode;		/* FREAD? FWRITE? */
		u_char		cloneFlags;
#define CLONEFLAG_W_HOLDING	0x01	/* We're holding the source hostage */
#define CLONEFLAG_W_PERMCONN	0x02	/* We've got a sustaining connection */
#define CLONEFLAG_W_NBOP	0x04	/* Next send won't be start of pkt */
#define CLONEFLAG_W_NBOC	0x08	/* Next send won't be start of conn */

		/* writes: */
		u_int		wr_pktOutResid;	/* bytes left in big packet */
		__uint32_t	wr_Ifield;
		hippi_fp_t	wr_fpHdr;
		u_short		wr_fburst;
		u_char		src_error;

		/* reads: */
		u_char		dst_errors;

	} clone[ HIPPIFP_MAX_CLONES ];

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
		sema_t		rd_sema;
		int		rd_count;
		sema_t		rd_dmadn;
		struct pollhead	*rd_pollhdp;
		hippi_fp_t 	*rd_fpd1head;
		union hip_c2b	*rd_c2b_rdlist;

		void		*io4ia_war_page0;
		void		*io4ia_war_page1;
	} ulp[ HIPPIFP_MAX_OPEN_ULPS+1 ]; /* last one is HIPPI-PH ulp */

#ifdef HIPPI_BP
	/* Need pointer to address of bypass firmware config info.  Also
	 * we read a copy of the firmware config info so we don't need
	 * to re-read this info each time (except for dm_status, which is
	 * why we need the firmware pointer).
	 */
	volatile struct hip_bp_fw_config *bp_fw_conf;
	struct hip_bp_fw_config cached_bp_fw_conf;

	struct hippi_port	*hippibp_portids;
	int	hippibp_portmax;
	int	hippibp_portfree;

	/* Following defines state of HippiBP device */

#define	HIPPIBP_ST_CONFIGED	1	/* device has been configed */
#define	HIPPIBP_ST_OPENED	2	/* device has been BP opened */
	char	hippibp_state;

	/* Following fields may be setup from BP Config ioctl */

	char	bp_ulp;
	char	bp_maxjobs;		/* max jobs (per controller) */
	int	bp_maxportids;		/* max portids for controller */
	int	bp_maxdfmpgs;		/* max pgs in dest freemap (per job) */
	int	bp_maxsfmpgs;		/* max pgs in src freemap (per job) */
	int	bp_maxddqpgs;		/* max pgs in dest desc que (per job)*/

	struct hippi_bp_job *bp_jobs;
#endif /* HIPPI_BP */
} hippi_vars_t;


/* These prototypes are for if_hip.c */

extern hippi_vars_t hippi_device[];

extern void hippi_wakeup( hippi_vars_t * );
extern void hippi_wakeup_nolock( hippi_vars_t * );
extern void hippi_send_c2b( hippi_vars_t *, int , int , void * );
extern int hippi_b2h( hippi_vars_t * );
extern void hippi_hwflags( hippi_vars_t *, int );

extern void hippi_wait_usec( hippi_vars_t *, int , char *, int );

#define hippi_wait(hi)  hippi_wait_usec( (hi), DELAY_OP, __FILE__, __LINE__ )


#endif /* _KERNEL */



#endif /* __HIPPIDEV_H */
