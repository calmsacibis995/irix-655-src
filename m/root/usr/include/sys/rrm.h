#ifndef __SYS_RRM_H__
#define __SYS_RRM_H__
/**************************************************************************
 *		 Copyright (C) 1990, Silicon Graphics, Inc.		  *
 *  These coded instructions, statements, and computer programs	 contain  *
 *  unpublished	 proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may	not be disclosed  *
 *  to	third  parties	or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 **************************************************************************/

/*
 *   $Revision: 1.80 $
 */
#include <sys/gfx.h>

/*
 * Device independent rendering resources that can be requested
 */
#define	RRM_RMASK_PCX		0x00000001	/* graphics context */
#define	RRM_RMASK_CLIP		0x00000002	/* clipping region */
#define	RRM_RMASK_BUFFER	0x00000004	/* buffer (double buffering) */
#define	RRM_RMASK_SWAPBUF	0x00000008	/* ability to swap buffers */

#define	RRM_RMASK_RETRACE	0x00000010	/* retrace event occurred  */
#define	RRM_RMASK_MESSAGE	0x00000020	/* messaged manager */
#define	RRM_RMASK_READ		0x00000040	/* readable region */
#define	RRM_RMASK_REFERENCE	0x00000080	/* pipe reference monitor */
#define	RRM_RMASK_ALL		(RRM_RMASK_PCX 	    | \
				 RRM_RMASK_CLIP     | \
				 RRM_RMASK_BUFFER   | \
				 RRM_RMASK_SWAPBUF  | \
				 RRM_RMASK_RETRACE  | \
				 RRM_RMASK_MESSAGE  | \
				 RRM_RMASK_READ     | \
				 RRM_RMASK_REFERENCE)

/*
 * rendering resource manager operations
 *
 * WARNING!  These RRM command numbers are used as an index into the
 * RRMfncs table so if they change, so must the RRMfncs table.
 */
#define RRM_BASE	      1000
#define RRM_OPENRN	      (RRM_BASE+0)  /* open rendering node */
#define RRM_CLOSERN	      (RRM_BASE+1)  /* close rendering node */
#define RRM_BINDPROCTORN      (RRM_BASE+2)  /* set current RN for proc */
#define RRM_BINDRNTOCLIP      (RRM_BASE+3)  /* bind RN to given clip region */
#define RRM_UNBINDRNFROMCLIP  (RRM_BASE+4)  /* unbind RN from clip region */
#define RRM_SWAPBUF	      (RRM_BASE+5)  /* swap buffers */
#define RRM_SETSWAPINTERVAL   (RRM_BASE+6)  /* swapinterval */
#define RRM_WAITFORRETRACE    (RRM_BASE+7)  /* wait for retrace */
#define RRM_SETDISPLAYMODE    (RRM_BASE+8)  /* display mode */
#define RRM_MESSAGE	      (RRM_BASE+9)  /* send message to board mgr */
#define RRM_INVALIDATERN      (RRM_BASE+10) /* invalidate resources in a RN */
#define RRM_VALIDATECLIP      (RRM_BASE+11) /* validate a RN's clip resource */
#define RRM_VALIDATESWAPBUF   (RRM_BASE+12) /* validate swap buffer ability */
#define RRM_SWAPGROUP         (RRM_BASE+13) /* join a swap group */
#define RRM_SWAPUNGROUP       (RRM_BASE+14) /* leave a swap group */
#define RRM_VALIDATEMESSAGE   (RRM_BASE+15) /* ack message */
#define RRM_GETDISPLAYMODES   (RRM_BASE+16) /* get display mode registers */
#define RRM_LOADDISPLAYMODE   (RRM_BASE+17) /* display mode */
#define RRM_CUSHIONBUFFER     (RRM_BASE+18) /* cushionbuffer on/off */
#define RRM_SWAPREADY         (RRM_BASE+19) /* group drive swapread on/off */
#define RRM_MGR_SWAPBUF	      (RRM_BASE+20) /* board manager swap buffer */
#define RRM_SETVSYNC          (RRM_BASE+21) /* set vsync counter */
#define RRM_GETVSYNC          (RRM_BASE+22) /* get vsync counter */
#define RRM_WAITVSYNC        (RRM_BASE+23)  /* wait on vsync counter */
#define RRM_BINDRNTOREADANDCLIP (RRM_BASE+24) /* bind RN to separate read/clip */
#define RRM_MAPCLIPTOSWPBUFID (RRM_BASE+25) /* takes clip ID, returns corresponding swapbuf ID (a.k.a. display ID) */
#define RRM_CMD_LIMIT	      (RRM_BASE+100)

/*
 * argument structures
 *
 * WARNING!  These structs MUST have rnid as their first argument.
 */

struct RRM_OpenRN {
	int		rnid;		/* rendering node ID */
	unsigned int	rmask;		/* resource mask: ones needed */
};

struct RRM_CloseRN {
	int		rnid;		/* rendering node ID */
};

struct RRM_BindProcToRN {
	int		rnid;		/* rendering node ID */
};

struct RRM_BindRNToClip {
	int		rnid;		/* rendering node ID */
	int		clipid;		/* opaque clip region ID */
	int		bindid;         /* return value */
};

struct RRM_UnbindRNFromClip {
	int		rnid;		/* rendering node ID */
	int		clipid;         /* opaque clip region ID */
};

struct RRM_SwapBuf {
	int		rnid;		/* rendering node ID */
	int		hwd_mode;	/* hardware dependent register value */
	int		hwi_mode;	/* hardware independent disp mode description (M_) */
};

struct RRM_CushionBuffer {
	int		xid;		/* Xwindow ID */
	int		flag;		/* on==1/off=0 */
};

struct RRM_SwapGroup {
	int		xid;		/* Xwindow ID */
	int		gxid;		/* Xwindow ID of group member */
};

struct RRM_SwapReady {
	int		xid;		/* Xwindow ID */
	unsigned int	line;		/* swapready line number */
	int		flag;		/* on==1/off=0 */
};

struct RRM_MGR_SwapBuf {
        int             rnid;           /* rendering node ID */
	int		swapbufid;	/* hardware DID */
	int		clipid;		/* opaque clip region ID */
	int		hwd_mode;	/* hardware dependent register value */
	int		hwi_mode;	/* hardware independent disp mode description (M_) */
};

struct RRM_SetSwapInterval {
	int		rnid;		/* rendering node ID */
	int		swapinterval;	/* number of frames between swaps */
};

struct RRM_WaitForRetrace {
	int		rnid;		/* rendering node ID */
};

struct RRM_SetDisplayMode {
	int		rnid;		/* rendering node ID */
	int		hwd_mode;	/* hardware dependent register value */
	int		hwi_mode;	/* hardware independent mode description (M_) */
};

struct RRM_LoadDisplayMode {
        int             rnid;           /* rendering node ID */
        int             clipid;         /* clip region ID */
	int		wid;		/* window ID that should be used */
	int		hwd_mode;	/* hardware dependent register value */
	int		hwi_mode;	/* hardware independent mode description (M_) */
};

struct RRM_GetDisplayModes {
	int		rnid;		/* rendering node ID */
	int		*buf;		/* buffer for mode descriptions (M_) */
	unsigned int	len;		/* length of buffer passed */
};

#if _MIPS_SIM == _ABI64 && defined(_KERNEL)
struct irix5_RRM_GetDisplayModes {
	int		rnid;		/* rendering node ID */
	app32_ptr_t     buf;		/* buffer for mode descriptions (M_) */
	unsigned int	len;		/* length of buffer passed */
};
#endif

struct RRM_InvalidateRN {
        int             rnid;           /* rendering node ID */
        int             clipid;         /* clip region ID */
	unsigned int	rmask;		/* resource mask: ones to invalidate */
	unsigned int	hwi_mode;	/* return value - mode window wants */
	unsigned int	flags;		/* input/output flags */
};
/* flags for RRM_InvalidateRN.flags */
#define	RRM_INVAL_WRONGBUF	0x1	/* return: DID left in wrong buffer */

struct RRM_PieceList {			/* window clipping piece list piece */
	int		x;		/* lower left x */
	int		y;		/* lower left y */
	int		xsize;		/* size in x */
	int		ysize;		/* size in y */
};

/* These arguments are interpreted by the dev. dependent graphics drivers.
   All arguments are not used by all drivers.
   We can probably simplify this interface. */
struct RRM_ValidateClip {
        int             rnid;           /* rendering node ID */
	int		clipid;		/* clip region ID */
	int		xorg;		/* window x origin */
	int		yorg;		/* window y origin */
	int		xsize;		/* window x size */
	int		ysize;		/* window y size */
	int		numpieces;	/* number pieces in clip region */
	struct RRM_PieceList *piecelist;/* window clipping piece list:
					   can be NULL (e.g. numpieces=1) */
	int		wid;		/* window ID that should be used */
	int		obscured;	/* window obscured =1, else =0 */
	int		widcheck;	/* WID check enabled =1, else =0 */
	int		changed;	/* window moved =1, shrunk =2, else =0*/
	unsigned int	hwi_mode;	/* current hwi mode for window */
	int		fboffset;	/* =0 for windows */
};

#if _MIPS_SIM == _ABI64 && defined(_KERNEL)
struct irix5_RRM_ValidateClip {
	int		rnid;		/* rendering node ID */
	int		clipid;		/* clip region ID */
	int		xorg;		/* window x origin */
	int		yorg;		/* window y origin */
	int		xsize;		/* window x size */
	int		ysize;		/* window y size */
	int		numpieces;	/* number pieces in clip region */
	app32_ptr_t     piecelist;      /* window clipping piece list:
					   can be NULL (e.g. numpieces=1) */
	int		wid;		/* window ID that should be used */
	int		obscured;	/* window obscured =1, else =0 */
	int		widcheck;	/* WID check enabled =1, else =0 */
	int		changed;	/* window moved =1, shrunk =2, else =0*/
	unsigned int	hwi_mode;	/* current hwi mode for window */
	int		fboffset;	/* =0 for windows */
};
#endif

struct RRM_ValidateSwapBuf {
        int             rnid;           /* rendering node ID */
	int		clipid;		/* clip region ID */
	int		swapbufid;	/* opaque swap buffers ID */
};

struct RRM_ValidateBuffer {
	int		rnid;		/* rendering node ID */
};

struct RRM_ValidateMessage {
	int		rnid;		/* rendering node ID */
};

struct RRM_SetVideoSync {
    	int		rnid;		/* rendering node ID */
	unsigned int	count;		/* new counter value */
};
  
struct RRM_GetVideoSync {
    	int		rnid;		/* rendering node ID */
	unsigned int	count;		/* returned counter value */
};
  
struct RRM_WaitVideoSync {
    	int		rnid;		/* rendering node ID */
	int		interval;	/* interval at wakeup */
	int		mod;		/* remainder at wakeup */
	unsigned int	count;		/* returned counter value */
};
  
struct RRM_BindRNToReadAndClip {
	int		rnid;		/* rendering node ID */
	int		clipid;		/* opaque clip region ID */
	int		readclipid;	/* opaque clip region ID */
	int		bindid;		/* return value */
	int		readbindid;	/* return value */
};

struct RRM_MapClipToSwapbufID {
        int             rnid;		/* rendering node ID */  
        int             clipid;		/* opaque clip region ID (WID) */
        int             swapbufid;	/* return value: swapbuf ID (DID) */
};

/*
 * Messege types being sent to board manager.
 */
#define	RRM_MSG_VALIDATECLIP	1	/* validate a given clip region */
#define	RRM_MSG_VALIDATESWAPBUF	2	/* validate ability to swap buffers */
#define RRM_MSG_SWAP_N_CLIP	3	/* atomic ValidateSwap and ValidateClip */
#define	RRM_MSG_BINDRNTOCLIP	4	/* bind given RN to given clip region */
#define	RRM_MSG_UNBINDRNFROMCLIP 5	/* unbind given RN from clip region */
#define RRM_MSG_MGR_SWAPDONE	9	/* board manager swap complete */
#define RRM_MSG_SWAP_INITIATED	10	/* OpenGL requested swap */
#define	RRM_MSG_SETDISPLAYMODE	11	/* set display mode */
#define	RRM_MSG_GENLOCK_NOTIFY	12	/* notify genlock acquired/lost */

	/*  these are generated by a GL program */
#define	RRM_MSG_FULLSCREEN	6	/* turn clipping off/on */
#define	RRM_MSG_ZBUFFERED	7	/* notify zbuffering */
#define	RRM_MSG_DONTFREEZE	8	/* dont freeze proc when win unmapped */

#define RRM_OGL_MSG_BIT			0x80 /* Open GL */
#define RRM_MSG_CMD_MASK		0x7f /* Open GL */

#define	RRM_MSG_OGL_BINDRNTOCLIP	(RRM_MSG_BINDRNTOCLIP | RRM_OGL_MSG_BIT)	 /* Open GL */
#define	RRM_MSG_OGL_UNBINDRNFROMCLIP	(RRM_MSG_UNBINDRNFROMCLIP | RRM_OGL_MSG_BIT) /* Open GL */


/* Increment this number, as more message types are defined */ 
#define RRM_MAX_MESSAGE_NUM	12	

#define GFX_MESSAGE_MAX_SIZE 10

struct RRM_Message_Header {
	int		rnid;		/* rendering node ID */
	unsigned int	rmask;		/* resource mask: ones to invalidate */
	unsigned char   type;		/* message type */
	int 		length;		/* message length */
};

struct RRM_Message_Fullscreen {
	struct RRM_Message_Header header;
	int clipid;
	int rnid;
	unsigned int flag;
};

struct RRM_Message_Dontfreeze {
	struct RRM_Message_Header header;
	int clipid;
	int rnid;
	unsigned int flag;
};

#ifdef _KERNEL

struct RRM_Message {
	struct RRM_Message_Header header;
	unsigned int data[GFX_MESSAGE_MAX_SIZE]; 
};
	
/*
 *   structure of an rrm message going upstream
 */

struct gfx_rrm_message {
	struct gfx_board *board;
	unsigned char type;
	int size;
	unsigned int arg[GFX_MESSAGE_MAX_SIZE];
};

/* group of panes for swapping */
struct swapgroup {
	unsigned	refcnt;		/* # members of this swap group								*/
	unsigned	swapcnt;	/* countdown of members who have yet to swap.  0 == all done; ready for next swap	*/
	unsigned	seqid;		/* latest seqid of the group (that of last member to have swapped)			*/
};

/*
 * PaneCache element
 */
struct pane {
    union {
	    int	p_paneid;
	    struct {
		    short incarn;
		    short paneid;
	    } paneid;
    } p_paneid;
    int    p_clipid;		/* opaque clip id - that's window to you buddy */
    int    p_serial;		/* serial number for validate clip */
    short  p_validmask;		/* resources which are valid */
    short  p_waitmask;		/* resources awaiting validation */
    short  p_rncnt;		/* reference count for number of bindies */
    short  p_displaybank;	/* currently displayed buffer */
    int    p_swapbufid;		/* opaque display id, for buffer swapping */
    int    p_swapinterval;	/* swap interval for this pane */
    uint   p_nextswaptime;	/* last swap time for this pane */
    uint   p_hwi_mode;		/* opaque HW indep. display mode */
    sema_t p_sema;		/* must have this to request validation */
    struct gfx_gfx *p_sema_owner;
    sv_t p_wait;		/* sleep on this to wait for validation */
    sv_t p_bwait;		/* sleep on this to wait for a buffer event */
    lock_t p_lock;		/* for MP weenies */
    struct gfx_data *p_bdata;	/* remember the board, so we can support multiple boards && get at the gfx semaphore */
    struct RRM_ValidateClip p_vclip;	/* current clip */
    struct swapgroup *p_swapgroup;	/* if not null, swapgroup to which this pane belongs */
    struct pane *p_next;	/* next pane in swapgroup */
    u_char  p_swapped;		/* flag to tell swapgroup whether or not this pane has swapped	*/
    u_char  p_cushion_depth;	/* depth of cushion supported by hardware */
    u_char  p_cushion_level;	/* current cushion depth for this pane */
};

/*
 * Device INDEPENDENT rendering node structure
 */
struct rrm_rnode {
	union {
		int	gr_rnid;
		struct {
			short incarn;
			short rnid;
		} rnid;
	} gr_rnid;
	unsigned short	gr_reqmask;	/* rendering resource request mask */
	unsigned short	gr_validmask;	/* rendering resource valid mask */
	void		*gr_pcx;	/* pipe context pointer */
	int		gr_pcxsize;	/* pipe context size (in bytes) */
	int		gr_clipid;	/* opaque clip region ID */
	int		gr_readclipid;	/* opaque clip region ID */
	int		gr_swapbufid;	/* opaque swap buffers ID */
	int		gr_swapinterval;/* swap interval for this RN */
	uint		gr_nextswaptime;/* last swap time for this RN */
	struct rrm_rnode *gr_vrwaitnext;/* next RN waiting for retrace */
	sv_t		gr_rnwait;	/* sleep on this RN */
	struct gfx_gfx	*gr_creategfxp;	/* gfx struct which opened this RN */
	struct gfx_gfx	*gr_gfxp;	/* gfx struct using this RN */
	struct gfx_data *gr_bdata;	/* ptr to gfx board data */
	short		gr_pcxid;	/* context ID on rendering node */
	char		gr_free;	/* RN is free if 1, allocated if 0 */
	char		gr_init;	/* 1 if done initial pcx validation */
	char		gr_inuse;	/* RN is bound to a proc if 1 */
	struct rrm_rnode *gr_older;	/* next older rnp (pcxid allocation) */
	struct rrm_rnode *gr_newer;	/* next newer rnp (pcxid allocation) */
	lock_t		gr_rnlock;	/* lock for this RN (MP) */
	void		*gr_private;	/* pointer to device dependent stuff */
	struct pane	*gr_pane;	/* pointer to pane cache entry */
	struct pane	*gr_readpane;	/* pointer to pane cache entry */
	int		gr_serial;	/* pane serial number */
	int		gr_readserial;	/* pane serial number */
	int		gr_vsync_int;	/* vsync interval */
	int		gr_vsync_mod;	/* vsync remainder */
	uint		gr_vsync_wakeup;/* counter value when wakeup happened */
	struct rrm_rnode *gr_vsync_next;/* next guy in the vsync list of sleepers */
	struct rrm_rnode *gr_wait_next; /* next guy in the wait list of sleepers */
};

/*
 * Element of a linked list of pointers to rendering nodes
 */
struct rrm_link {
	struct rrm_rnode	*gl_rnp;
	struct rrm_link		*gl_next;
};


/*
 * Function prototypes.
 */

struct gfx_data;
struct gr2_pixeldma_args;
extern int RRM_ValidFault(struct gfx_gfx *, void *);
extern int RRM_Dispatch(struct gfx_gfx *,int,void *, int *);
extern void RRM_Init(struct gfx_data *);
extern int RRM_Exit(struct gfx_gfx *);
extern int RRM_CheckValidFault(struct gfx_gfx *);
extern void RRM_init_panes(struct gfx_gfx *);
extern void rrmValidateBuffer(struct rrm_rnode *);
extern void rrmValidateRetrace(struct rrm_rnode *);
extern int rrmInvalidateRN(struct gfx_gfx *, struct rrm_rnode *,uint);
extern void rrmUnMapGfx(struct gfx_gfx *);
extern unsigned int rrmGetResourceMask(struct rrm_rnode *);
extern void rrmSetResourceMask(struct gfx_gfx *, struct rrm_rnode *,
			       register unsigned int);
extern int rrmWaitVideoSync(struct gfx_gfx *, struct rrm_rnode *, int interval,
			     int mod);
extern void rrmValidateVideoSync(struct gfx_data *);
extern void rrmValidateMGRSwapBuf(struct gfx_gfx *, unsigned int);
extern int rrmValidateMGRSwapBuf_NOSYNC(struct gfx_gfx *, unsigned int);
extern void rrmGenlockNotify(struct gfx_gfx *, unsigned int);
#endif /* _KERNEL */

#endif /* __SYS_RRM_H__ */
