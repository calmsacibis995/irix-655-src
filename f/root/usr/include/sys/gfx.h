#ifndef __SYS_GFX_H__
#define __SYS_GFX_H__

/**************************************************************************
 *		 Copyright (C) 1990-1996 Silicon Graphics, Inc.		  *
 *  These coded instructions, statements, and computer programs	 contain  *
 *  unpublished	 proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may	not be disclosed  *
 *  to	third  parties	or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 **************************************************************************/

/*
 *   $Revision: 1.160 $
 */
#include <sys/rrm.h>

#ifdef _KERNEL
#include <sys/kabi.h>
#include <ksys/ddmap.h>
#include <sys/cpumask.h>
#include <sys/sema.h>

#ifdef GFX_PIPEEND
#include <sys/dpipe.h>
#endif

#define HOLD_NONFATALSIG(x)             hold_nonfatalsig((x))
#define RELEASE_NONFATALSIG(x)          release_nonfatalsig((x))
extern struct gfxinfo *gfxinfo;
#define GFXINFO		gfxinfo[cpuid()]
#define GFX_WAITC_ON()	gfx_waitc_on();
#define GFX_WAITC_OFF()	gfx_waitc_off();
#define GFX_WAITF_ON()	gfx_waitf_on();
#define GFX_WAITF_OFF()	gfx_waitf_off();
#endif /* _KERNEL */

/*
 *   I_STR ioctl commands for gfx STREAMS
 *
 *   All Gfx ioctls share the same cmd token space to
 *   avoid unnecessary indirections.
 *
 *   There are three types of ioctl.
 *     - GFX device independant gfx operations (defined here)
 *     - RRM rendering resource manager commands (defined in rrm.h)
 *     - Graphics board specific cmds (defined in a board-specific header e.g. gr1.h)
 */

#ifndef NULL
#define	NULL	0L
#endif
/*
 *   GFX commands
 */

#define GFX_BASE		100
#define GFX_GETNUM_BOARDS	(GFX_BASE+1)
#define GFX_GETBOARDINFO	(GFX_BASE+2)
#define GFX_ATTACH_BOARD	(GFX_BASE+3)
#define GFX_DETACH_BOARD	(GFX_BASE+4)
#define GFX_IS_MANAGED		(GFX_BASE+5)

	/* Have to be the board owner to use these */
#define GFX_INITIALIZE		(GFX_BASE+6)
#define GFX_START		(GFX_BASE+7)
#define GFX_DOWNLOAD		(GFX_BASE+8)
#define GFX_BLANKSCREEN		(GFX_BASE+9)
#define GFX_MAPALL		(GFX_BASE+10)
#define GFX_LABEL		(GFX_BASE+11)
#define GFX_REATTACH_BOARD	(GFX_BASE+12)
#define GFX_SET_DIAG_FLAG	(GFX_BASE+13)

        /* Start of board private commands */
#define GFX_PRIVATE_BASE        10000

/*
 *	gfx ioctl arguments
 */

struct gfx_getboardinfo_args {
	unsigned int board; /* board number (contiguous from zero) */
	void *buf;		/* buffer for reply */
	unsigned int len;	/* max length, length of reply (bytes) */
};

#if _MIPS_SIM == _ABI64 && defined(_KERNEL)
struct irix5_gfx_getboardinfo_args {
	unsigned int board;	/* board number (contiguous from zero) */
	app32_ptr_t  buf;	/* buffer for reply */
	unsigned int len;	/* max length, length of reply (bytes) */
};
#endif


struct gfx_attach_board_args {
	unsigned int board;
	void        *vaddr;     /* this is a user space address */
};


#if _MIPS_SIM == _ABI64 && defined(_KERNEL)
struct irix5_gfx_attach_board_args {
	unsigned int board;
	app32_ptr_t  vaddr;     /* this is a user space address */
};
#endif

struct gfx_attach_dm_table_args {
	void *addr;
	int  len;
};


#if _MIPS_SIM == _ABI64 && defined(_KERNEL)
struct irix5_gfx_attach_dm_table_args {
	app32_ptr_t addr;
	int  len;
};
#endif

struct gfx_download_args {		/* GFX_DOWNLOAD */
	int len;			/* length in bytes of ucode buffer */
	void *addr;			/* address of ucode buffer */
	int magic;			/* magic number in ucode file */
	int unit;			/* unit number to load ucode into */
	int off;			/* offset in memory space where ucode goes */
	int width;			/* width in bytes of ucode word */
	int entry;			/* entry point (if applicable) */
	int run;			/* run flag (should the engine be started?) */		
};


#if _MIPS_SIM == _ABI64 && defined(_KERNEL)
struct irix5_gfx_download_args {	/* GFX_DOWNLOAD */
	int len;			/* length in bytes of ucode buffer */
	app32_ptr_t addr;		/* address of ucode buffer */
	int magic;			/* magic number in ucode file */
	int unit;			/* unit number to load ucode into */
	int off;			/* offset in memory space where ucode goes */
	int width;			/* width in bytes of ucode word */
	int entry;			/* entry point (if applicable) */
	int run;			/* run flag (should the engine be started?) */		
};
#endif

#define GFX_DOWNLOAD_LIMIT 4096


struct gfx_private_args {
	unsigned int cmd;
	char data[1];
};

/*
 *   M_PROTO message types for gfx STREAM
 */
#define GFX_PROTO_ACK		1
#define GFX_PROTO_NACK		-1
#define GFX_PROTO_BASE		100
#define GFX_POSCURSOR		(GFX_PROTO_BASE+2)

/*
 *   PROTO message args
 */

struct gfx_poscursor {
	int cmd;			/* GFX_POSCURSOR */
	int x ;
	int y ;
};

union gfx_proto	{
	int cmd;			/* may be unknown */
	struct gfx_poscursor poscursor;
};


/*
 *   Graphics board description
 */

#define GFX_INFO_NAME_SIZE 	16
#define GFX_INFO_LABEL_SIZE 	16

struct gfx_info {
	char name[GFX_INFO_NAME_SIZE]; 	/* unique name for this graphics */
	char label[GFX_INFO_LABEL_SIZE];/* manager's name for this board */
    	unsigned short int xpmax;	/* screen x size in pixels */
	unsigned short int ypmax;	/* screen y size in pixels */
	unsigned int length;		/* size of a complete info struct */
	                                /* for this board */
};

#ifdef _KERNEL

struct gfx_data;
struct gfx_gfx;
struct rrm_rnode;
struct RRM_ValidateClip;

typedef struct gfx_list {
	int		refcnt;
	int		readers;	/* num of readers reading */
	sv_t		wait;		/* waiting readers or writers */
	lock_t		lock;
	struct gfx_gfx *gfxp;
} gfx_list_t;

typedef struct gfx_group {
        /* unique thread group identifier: */
        thread_group_handle_t	thdgroup;
	int			refcnt;
	int			iglruncnt;	/* IrisGL: #group members running */
	lock_t			lock;		/* lock for above fields */
	struct gfx_group *next;
} gfx_group_t;

typedef struct gthread {
    thread_handle_t	thd;	/* unique thread identifier */
    unsigned int	flags;	/* gfx flags for this thread */

    /* list of open gfx connections for this thread */
    /* (shared for an irisgl sproc share group) */
    gfx_list_t	*gfxlist;

    /* shared data for group of gfx threads */
    /* such as an irisgl sproc share group or */
    /* an OpenGL pthread application */
    gfx_group_t	*group;

    struct gthread *next;
} gthread_t;

/*
 *   Values for gthread.flags
 */
#define GTHREAD_GFXKP		0x00000001 /* thread was kpreempted while holding gfxsema */
#define GTHREAD_IGL_SGRAPH      0x00000002 /* member of IrisGL thread group just given graphics */
#define GTHREAD_EXITTED		0x00000004 /* thread has gone through gfx_exit() */
#define GTHREAD_SUSPENDING	0x00000008 /* thread is going through rrmSuspend() */


typedef struct gfxtablelock {
	int readers;
	lock_t lock;
} gfxtablelock_t;

void gfxtable_readerlock(gfxtablelock_t *);
void gfxtable_readerunlock(gfxtablelock_t *);
int gfxtable_writerlock(gfxtablelock_t *);
void gfxtable_writerunlock(gfxtablelock_t *,int);

/* XXXmacko this "hash function" seems good for now */
#define GFX_GROUP_TABLE_INDEX(grp)	(int)(((__psunsigned_t)grp & (GFX_GROUP_TABLE_SIZE-1)) ?\
					      ((__psunsigned_t)grp & (GFX_GROUP_TABLE_SIZE-1)) :\
					      (((__psunsigned_t)grp>>12) & (GFX_GROUP_TABLE_SIZE-1)))
#define GFX_GROUP_TABLE_SIZE		16	/* XXXmacko constant for now */
extern gfx_group_t **gfx_group_table;
extern gfxtablelock_t gfx_group_table_lock;
extern int *gfx_group_table_stats;

/* XXXmacko this "hash function" seems good for now */
#define GTHREAD_TABLE_INDEX(thd)	(int)(((((((((__psunsigned_t)thd >> 8) & 0xff) * 29) \
						 ^ (((__psunsigned_t)thd >> 16) & 0xff)) * 29) \
						 ^ (((__psunsigned_t)thd >> 24) & 0xff)) \
						) & (GTHREAD_TABLE_SIZE-1))
#define GTHREAD_TABLE_SIZE		16	/* XXXmacko constant for now */
extern gthread_t **gthread_table;
extern gfxtablelock_t gthread_table_lock;
extern int *gthread_table_stats;

extern gthread_t * gthread_find(thread_handle_t thd);
extern struct gfx_gfx * gthread_find_gfxp(thread_handle_t thd);
extern int gthread_scan(int ((*gfunc)(gthread_t *, void *, int)), void *garg);

/*
 * Device independent interface to device dependent graphics board functions.
 */

struct gfx_fncs {
			 /* functions for the Graphics Driver */
	int (*gf_Info)(struct gfx_data *, void *, unsigned int, int *);    
	int (*gf_Attach)(struct gfx_gfx *, caddr_t);
	int (*gf_Detach)(struct gfx_gfx *);
	int (*gf_Initialize)(struct gfx_gfx *);
	int (*gf_Download)(struct gfx_gfx *, struct gfx_download_args *);
	int (*gf_Start)(struct gfx_gfx *);
	int (*gf_PositionCursor)(struct gfx_data *, int, int);
	int (*gf_Exit)(struct gfx_gfx *); /* XXXnate just added this */

			/* functions for the Rendering Resource Manager */
	int (*gf_CreateDDRN)(struct gfx_data *, struct rrm_rnode *);
	int (*gf_DestroyDDRN)(struct gfx_data *, struct gfx_gfx *, struct rrm_rnode *);
	int (*gf_ValidateClip)(struct gfx_gfx *, struct rrm_rnode *,
		struct rrm_rnode *, struct RRM_ValidateClip *);
	int (*gf_SetNullClip)(struct RRM_ValidateClip *);
	int (*gf_MapGfx)(struct gfx_gfx *, __psunsigned_t, int);
	int (*gf_UnMapGfx)(struct gfx_gfx *);
	int (*gf_InvalTLB)(struct gfx_gfx *);
	int (*gf_PcxSwap)(struct gfx_data *, struct rrm_rnode *,
		struct rrm_rnode *, struct rrm_rnode *);
	int (*gf_PcxSwitch)(struct gfx_data *, struct rrm_rnode *,
		struct rrm_rnode *);
	int (*gf_SchedSwapBuf)(struct gfx_data *, struct rrm_rnode *,int,int);
	int (*gf_UnSchedSwapBuf)(struct gfx_gfx *, struct rrm_rnode *, int);
	int (*gf_SchedRetraceEvent)(struct gfx_data *, struct rrm_rnode *);
	int (*gf_SetDisplayMode)(struct gfx_gfx *, int, unsigned int);
	int (*gf_SchedMGRSwapBuf)(struct gfx_gfx *, int, int, int, int);
	int (*gf_Suspend)(struct gfx_data*, struct gfx_gfx *, int);
	int (*gf_Resume)(struct gfx_data*, struct gfx_gfx *, int);
	int (*gf_ReleaseGfxSema)(struct gfx_gfx*);

	/*
	 * routine to call graphics board driver specific functions.
	 */
	int (*gf_Private)(struct gfx_gfx*, struct rrm_rnode *,
		unsigned int, void *, int *);

	/*
	 * Functions to install and uninstall
	 * link to Frame Scheduler
	 */
	int (*gf_FrsInstall)(struct gfx_data*, void* intrgroup);
	int (*gf_FrsUninstall)(struct gfx_data*);

#ifdef GFX_PIPEEND
        /* Datapipe library gfx pipe-end ioctl handler */
        int (*gf_DpIoctl)(struct gfx_gfx *,unsigned int,void *,int *);
#endif

};

#define GET_GFXSEMA(gfxp)					\
	{							\
	psema(&gfxp->gx_bdata->gfxsema,PZERO|PRECALC);		\
	ASSERT(gfxp->gx_bdata->gfxlock == 0);			\
	gfxp->gx_bdata->gfxsema_owner = gfxp;			\
	GFX_LOG(GET_GFXSEMA, gfxp->gx_bdata);			\
	GFXLOGV(gfxp);						\
	while (gfxp->gx_bdata->gfxbackedup) { DELAY(2); }	\
	}
#define RELEASE_GFXSEMA(gfxp)					\
	{							\
	int s = disableintr();					\
	GFX_LOG(REL_GFXSEMA,gfxp->gx_bdata);			\
	GFXLOGV(gfxp->gx_bdata->gfxsema_owner);			\
	if (gfxp->gx_fncs->gf_ReleaseGfxSema != NULL)		\
	    gfxp->gx_fncs->gf_ReleaseGfxSema(gfxp);		\
	gfxp->gx_bdata->gfxsema_owner = NULL;			\
	ASSERT(gfxp->gx_bdata->gfxlock == 0);			\
	vsema(&gfxp->gx_bdata->gfxsema);			\
	enableintr(s);						\
	}
/*
 *  In order to correctly support IrisGL sprocs in the face of
 *  kernel preemption, gfxlock changes need to be atomic.
 *  If we don't support native IrisGL (i.e. only IGLOO), there's
 *  no need to make all gfxlock operations atomic.
 */
#if !defined(IP27) && !defined(IP30) && !defined(IP32)
#define SUPPORT_NATIVE_IRISGL
#endif
#ifdef SUPPORT_NATIVE_IRISGL
#define GFXLOCK_ADD(lock,val)	atomicAddInt(lock,val)
#else
#define GFXLOCK_ADD(lock,val)	((*lock) += val)
#endif
#define INCR_GFXLOCK(gfxp)					\
	{							\
	GFXLOCK_ADD(&gfxp->gx_bdata->gfxlock,1);		\
	GFX_LOG(GFXLOCK_INCR,gfxp->gx_bdata);			\
	GFXLOGV(gfxp);						\
	GFXLOGV(gfxp->gx_bdata->gfxlock);			\
	ASSERT(gfxp->gx_bdata->gfxsema_owner == gfxp);		\
	ASSERT(gfxp->gx_bdata->gfxlock > 0);			\
	}
#define DECR_GFXLOCK(gfxp)					\
	{							\
	ASSERT(gfxp->gx_bdata->gfxlock > 0);			\
	ASSERT(gfxp->gx_bdata->gfxsema_owner == gfxp);		\
	GFX_LOG(GFXLOCK_DECR,gfxp->gx_bdata);			\
	GFXLOGV(gfxp);						\
	GFXLOGV(gfxp->gx_bdata->gfxlock-1);			\
	GFXLOCK_ADD(&gfxp->gx_bdata->gfxlock,-1);		\
	}


struct gfx_board { 
	struct gfx_info *gb_info ;
	struct gfx_gfx *gb_manager;
	struct gfx_gfx *gb_owner;
	int gb_number;	   /* same as index in GfxBoards */
	struct queue *gb_rq_ptr;
	struct queue *gb_wq_ptr;
	/* gunk for talking to shimq  */
	short int gb_devminor ;		/* Identifies the shmiq */
	short int gb_index ;		/* Identifies the device */
	struct gfx_fncs *gb_fncs ;
	struct gfx_data *gb_data ;
};


        



/*
 *   gfx driver structure
 *   one of these per open board, per thread
 */

struct gfx_gfx {

	/*  generic stuff  */
	struct gfx_board *gx_board; /* pointer to board */
	thread_handle_t gx_thd;		/* handle for thread that created this gfxp */
	struct gfx_gfx *gx_next;	/* ptr to next open gfx */

	/*  board driver stuff	*/
	struct gfx_fncs *gx_fncs;
	struct gfx_data *gx_bdata;	/* ptr to driver private data */
					/* there are many of these pointers */
					/* but only one copy of the data */
	ddv_handle_t gx_ddv;		/* handle to gfx region */
	unsigned int gx_flags;		/* flags needed by char side only */
	unsigned int gx_sflags;		/* flags needed by both sides */
	cpumask_t gx_tlbmask;		/* for late TLB flushing on MP */

	/*  rrm stuff  */
	void *gx_openrns;		/* RN's this gfx has open */
	void *gx_boundrn;		/* RN this gfx is currently bound to */

	/* share group fields */
	sema_t gx_sema;			/* semaphore for multiple sg procs */

	gfx_group_t *gx_group;		/* gfx_group that opened this gfxp (if applicable) */
};


/*
 *   Values for gfx_gfx.gx_flags
 */
#define GFX_MAPPED	0x00000001	/* the gfx region is mapped */
#define GFX_INUSE	0x00000002     	/* we are (or could be) talking to the pipe, and should */
                                        /* call gf_Suspend/gf_Resume during kernel preemptions. */
#define GFX_DIAG	0x00000004	/* the thread has identified itself as a diagnostic */
#define GFX_OPENGL	0x00000008	/* the thread is an OpenGL thread */
#define GFX_UNMAPPING	0x00000010     	/* thread is being unmapped */
#define GFX_DIAGMAPALL	0x00000020     	/* gfx mapped via special GFX_MAPALL ioctl */
/*
 *   Values for gfx_gfx.gx_sflags
 */
#define GFXS_STREAM	0x00000001	/* referenced by streams side of driver */
#define GFXS_CHAR	0x00000002	/* referenced by character side of driver */
/*
 *   Macros for gfx_gfx.gx_tlbmask
 */
#define CLEAR_TLBMASK(gfxp)		CPUMASK_CLRALL(gfxp->gx_tlbmask)
#define SET_TLBMASK(gfxp,cpu)		CPUMASK_SETB(gfxp->gx_tlbmask,cpu)
#define ASSIGN_TLBMASK(gfxp,cpu)	{CLEAR_TLBMASK(gfxp);SET_TLBMASK(gfxp,cpu);}
#define TEST_TLBMASK(gfxp,cpu)		CPUMASK_TSTB(gfxp->gx_tlbmask,cpu)

struct swapgroup;
struct gfx_data {
		/* for the Rendering Resource Manager */
	struct gfx_gfx *manager;	/* pointer to manager, if managed */
	void *currentrn;		/* Current rendering node */
	int numrns;			/* number of RN's open on this board */
	int numpcx;			/* number of pipe contexts in hardware*/
	struct rrm_rnode **loadedpcxs;	/* table of pipe contexts loaded
					   in hardware */
	struct rrm_rnode *newestrn;	/* most recently used rnp
					   (pcxid allocation) */
	struct rrm_rnode *oldestrn;	/* least recently used rnp
					   (pcxid allocation) */
	lock_t lock;			/* per board locking mechanism */
	sema_t		gfxsema;	/* The gfx semaphore is held during all
					   times the thread has the pipe
					   mapped.  It is also necessary to
					   hold it during modification of
					   rnp->validmask, even if the thread
					   doesn't have the pipe.  Unless the
					   gfxlock is non-zero, the thread will
					   give up gfxsema during times when it
					   is _not_ running (see rrmSuspend()
					   and rrmResume()).
					 */
	struct gfx_gfx	*gfxsema_owner; /* When the semaphore is grabbed, this
					   is set to the current gfxp.  It is
					   used to avoid re-grabbing gfxsema.
					 */
	int		gfxlock;	/* The lock is used so that even when a
					   thread sleeps it will hold gfxsema.
					   This is necessary in ValidateMyPcx,
					   and in some machine dependent code.
					   XXX warning: all gfxlock operations
					   must now be atomic operations in
					   order for IrisGL sprocs to work.
					   (Once IrisGL sproc support is gone
					   they can be normal operations again)
					 */
	int		gfxbusylock;	/* when a thread switches out, if the
					   device dependent graphics driver decides
					   that its not an opportune time to context
					   switch the graphics (based on user activity)
					   then we can force the state of gfxlock to
					   hold off a graphics context switch.
					*/
	int nummodes;			/* number of mode registers in
					   hardware*/
	unsigned int *loadedmodes;	/* table of mode registers loaded
					   in hardware */
	volatile int gfxbackedup;	/* flag set by fifo interrupt handler when
					 * fifo is near full and we shouldn't drop 
					 * anymore tokens 
					 */
	int gfxbackedup_notlocked;	/* flag set by fifo hiwater handler when
					 * we catch that fifo is near full as we
					 * are suspending a thread... and so we
					 * haven't locked gfxsema.
					 */
	int (*gf_ClearBackupFnc)(struct gfx_data *);
					/* When gfx is backed up, it is sometimes
					 * possible to clear the backup by 
					 * using a board-dependent function. 
					 * This function is called from 
					 * gfx_disp if gfx is backed up and
					 * the function is defined.
					 */
	int gfxresched;
	unsigned int vsync_cnt;		/* vsync counter for IrisGL; writeable
					 * by apps */
	unsigned int vsync_cnt_ogl;	/* vsync counter for OpenGL; not
					 * writeable by apps */
	struct rrm_rnode *vsync_sleep_list;
					/* The list of vsync sleepers; they are
					 * inserted by the RRM when a thread
					 * calls the vsync sleep ioctl, and
					 * deleted by the vertical retrace
					 * interrupt handler via an rrm routine.
					 */
	int   frs_vsync;                 /* Flag to manage linkage to Frame
					  * Scheduler. True if the vsync
					  * interrupt has to be forwarded to
					  * some other cpus.
					  */
	void* frs_intrgroup;             /* Interrupt group descriptor to be used
					  * to forward the vsync interrupt to
					  * cpus managed by the Frame Scheduler.
					  */
	int wait_rrm;			  /* wait list implemented in rrm */
	int wait_time;			  /* retrace counter for wait_list */
	struct rrm_rnode *wait_list;	  /* rrm's wait list (for swapinterval) */

	unsigned int swapready_line_cnt;  /* number of swapready lines available */
	struct swapgroup **swapready_line;/* groups of panes driving each swapready line */

	unsigned int cushion_depth;	  /* cushion depth supported by this hardware */
};




/*
 *   args to GfxKiller() routine called by gthread_scan()
 */

struct GfxKiller_args {		/* arg struct given to gthread_scan() */
	int	signal;		/* signal to send */
	struct gfx_data	*bdata;	/* pointer to board data; identifies a board */
	gthread_t	*gthd;	/* your gthread if you shouldn't be killed */
				/* (e.g. board manager exits), NULL otherwise */
};


/*
 * Multiple reader, single writer lock routines for list rooted at pp->p_gfx.
 */
struct gfx_rrm_message;
extern int  GfxKiller(gthread_t *, void *, int);
extern int  GfxListWriterLock(gfx_list_t *);
extern void GfxListWriterUnlock(gfx_list_t *, int);
extern void GfxListReaderLock(gfx_list_t *);
extern void GfxListReaderUnlock(gfx_list_t *);
extern int  GfxMessage( struct gfx_rrm_message *);
extern void GfxIntrMessage( struct gfx_rrm_message *);

/*
 * GfxRegisterBoard() is called by drivers that wish to register
 * a newly configured board.  For drivers that run on platforms
 * supporting parallel and dynamic device configuration, the 
 * driver must bracket accesses of GfxNumberOfBoards and calls to
 * GfxRegisterBoard() with invocations of LOCK_GFXBOARDINIT() and
 * UNLOCK_GFXBOARDINIT().  Note that GfxBoardLimit is determined
 * at the beginning of time and never changes whereas
 * GfxNumberOfBoards is incremented with each call to
 * GfxRegisterBoard().
 */
extern sema_t GfxBoardInitSema;
#define LOCK_GFXBOARDINIT()   psema(&GfxBoardInitSema, PZERO)
#define UNLOCK_GFXBOARDINIT() vsema(&GfxBoardInitSema)
extern int  GfxBoardLimit;
extern int  GfxNumberOfBoards;
extern int  GfxRegisterBoard(struct gfx_fncs *, struct gfx_data *, struct gfx_info *);

extern int gfx_fault(vhandl_t *vt, void *_gfxp, caddr_t vaddr, int rw);

#ifdef DEBUG
/*
 *  Functions available for debug printf's:
 *  (where PA is a parenthesized argument list for dprintf())
 *
 *      DPRINT(PA)
 *              - calls: "dprint PA"
 *              - Examples:
 *                DPRINT(("gfx rules!\n"));
 *                DPRINT(("gfx is #%d!\n",1));
 *      DPRINTIF(C,PA)
 *              - calls: "dprint PA" iff condition C evaluates to a non-zero value.
 *              - Examples:
 *                DPRINTIF(gfx_rules,("gfx rules!\n"));
 *                DPRINTIF(gfx_rules,("gfx is #%d!\n",1));
 *
 *  Note: the 'extra' parentheses may seem annoying at first, but
 *  do have several advantages:  1) when NOT compiled DEBUG, these
 *  macros expand to nothing, so there's no need to worry about
 *  expressions with side-effects, etc.  2) they allow you to pass
 *  any number of arguments to dprintf().
 */
extern void dprintf(char *format, ... );

#define DPRINT(PRINTF_ARGS)	dprintf PRINTF_ARGS
#define DPRINTIF(CONDITION,PRINTF_ARGS)	\
	if (CONDITION) dprintf PRINTF_ARGS

#else

#define DPRINT(P)		/*nothing*/
#define DPRINTIF(C,P)		/*nothing*/

#endif

/*
 *  GFX_IDBG
 *
 *  If you define GFX_IDBG (or DEBUG) when compiling the code in
 *  gfx/kern/graphics several gfx-specific kernel debugger functions
 *  will be available.
 *
 *  Functions:
 *
 */
#if defined(GFX_IDBG) || defined(DEBUG)
extern int  idbg_addfunc(char *name, void (*func)());
extern void gdbg_gfxplist(__psint_t);
extern void gdbg_gfxowner(long);
extern void gdbg_gfxproc(long);
extern void gdbg_gfxuthread(long);
extern void gdbg_gthread_table(long);
extern void gdbg_gfxgroup_table(long);
extern void gdbg_gthread(long);
extern void gdbg_gfxlist(long);
extern void gdbg_gfxgroup(long);
extern void gdbg_gfx(long);
extern void gdbg_rrn(long);
extern void gdbg_pane(long);
extern void gdbg_bdata(long);
#endif

#if defined(DEBUG) || defined(GFX_EVENTLOG)
/*
 *  This is a utility function you can call to make sure all
 *  CPUs have dropped into the debugger.  This prevents cases
 *  where one cpu drops into the debugger and the rest continue
 *  on and overflow the gfx event logging buffers with events
 *  that happened long after the point in time we care about.
 */
extern void stop_all_cpus(char *str);  
#endif

/*
 *  GFX_EVENTLOG
 *
 *  If you define GFX_EVENTLOG when compiling the code in gfx/kern
 *  (and below), you pick up a definition for the GFXLOG* macros and
 *  a debugger command, gfxlog [num].  These two in combination can
 *  give you a reverse chronological event log of all "interesting"
 *  graphics related events that have occurred (up to a maximum of
 *  GEL_MAXLOG per cpu).
 *
 *  The GFXLOG*() macros are used to record timestamped events in a
 *  per-cpu circular buffer.  The log records a string/token or some
 *  piece of data along with line number and file name information.
 *  
 *      GFXLOGS(S) - records a string, S
 *      GFXLOGT(T) - records a stringized-token, #T
 *      GFXLOGV(V) - records a value
 *
 *  The data recorded is usually just some number/token/string which
 *  you can then correspond back to some place in the code.  For
 *  example, the Venice macros that put tokens in the graphics pipe
 *  have been modified to also log the event with GFX_LOG() (an older
 *  macro from the first implementation of this event logging
 *  mechanism; aka HARDCORE_DEBUG).  It is possible to pass any number
 *  or string to the logger, you just have to make sense of it.
 *
 *  Feel free to add as much logging/instrumentation as is
 *  necessary.  It doesn't get compiled if GFX_EVENTLOG is off,
 *  so it doesn't really affect anything.
 *
 *  Other useful macros are:
 *      GEL_STOPLOGGING    - to stop logging
 *      GEL_STARTLOGGING   - to start logging
 *
 *  Generally, the way to use this is to drop into the debugger when
 *  some event occurs that is bad (pipe crash, bad CP command, kernel
 *  panic, etc).  Then do something like 'gfxlog 100' to see the last
 *  100 gfx events that the kernel has seen.  Then using other
 *  debugger commands like "plist" and "gfx" you can convert most of
 *  the remaining hex values into more symbolic values.  This results
 *  in a nicely annotated traceback of exactly what the kernel did
 *  and to whom.  That's incredibly handy for debugging complicated
 *  MP related issues.
 *
 *  NOTE WELL: These macros are useful, but how you use them influences
 *             how much info they'll give you.  If you have a tough MP
 *             related bug,  then when the event that you want to
 *             trigger on occurs,  you'll want to be sure to call the
 *             GEL_STOPLOGGING macro and then (possibly) call
 *             stop_all_cpus().  If you don't do this, then what can
 *             happen is that the other cpu's will continue on, spinning
 *             on various events and in a very short time wrap around in
 *             their buffers.  Then when you ask for a traceback, you'll
 *             see lots of junk from the other cpu's, but not what you're
 *             interested in.  This can be very misleading, so generally
 *             when you reach the point you want to freeze the machine, do
 *             a GEL_STOPLOGGING followed by a call to stop_all_cpus().
 *
 */

#ifdef GFX_EVENTLOG

typedef struct gfx_event_log
{
  unsigned long long time;
  char *file;
  unsigned int line;
  unsigned int type;
  union {
      char *s;
      unsigned long v;
      } v;
} gfx_event_log;

#define GEL_NUM		0
#define GEL_STRING	1

#ifndef GEL_MAXLOG
#define GEL_MAXLOG	1024
#endif

extern int GEL_log;
extern int *GEL_cnt;
extern gfx_event_log **GEL_hist;
extern lock_t GEL_lock;

extern int GEL_init(void);
extern void GEL_restart(long p1);
extern void sort_gfx_event_logs(long p1);
extern void dump_gfx_event_log_without_source_refs(long p1);
extern void dump_gfx_event_log_with_source_refs(long p1);
extern void simple_gfx_dump(long p1);

#define GEL_STARTLOGGING	GEL_log=(GEL_cnt ? 1 : 0)
#define GEL_RESTARTLOGGING	GEL_restart()
#define GEL_STOPLOGGING		GEL_log=0
/* for compatibility with old HARDCORE_DEBUG stuff */
#define GFX_STOP_LOGGING	GEL_STOPLOGGING

#if defined(IP27)
#define PRE_GET_TIME
/* This generates fewer warnings than GET_LOCAL_RTC on IP27. */
/* It's basically what GET_LOCAL_RTC becomes (w/out a constant condition) */
#define GET_TIME  ((clkreg_t)*(HUBREG_CAST(RAW_NODE_SWIN_BASE(0,HUB_REGISTER_WIDGET)+(PI_RT_COUNT))))
#elif defined(IP19) || defined(IP21) || defined(IP25)
#define PRE_GET_TIME
#define GET_TIME  GET_LOCAL_RTC
#elif defined(IP20) || defined(IP22) || defined(IP26) || defined(IP28) || defined(IP30) || defined(IP32)
extern unsigned int update_ust_i(void);
extern void get_ust_nano(unsigned long long *);
#define PRE_GET_TIME unsigned long long ust; update_ust_i(); get_ust_nano(&ust);
#define GET_TIME (ust >> 10) /* gives us units roughly in microseconds */
#else
#error <<BOMB - need GET_TIME definition for this platform>>
#endif			     


#define GFXLOGV(V)						\
	{							\
	    int s,cpu,event;					\
	    PRE_GET_TIME;					\
	    if (GEL_log) {					\
		s = mp_mutex_spinlock(&GEL_lock);		\
		cpu = cpuid();					\
		event = GEL_cnt[cpu];				\
	        GEL_cnt[cpu] = (GEL_cnt[cpu]+1)%GEL_MAXLOG;	\
	        GEL_hist[cpu][event].time = GET_TIME;		\
		mp_mutex_spinunlock(&GEL_lock,s);		\
	        GEL_hist[cpu][event].type = GEL_NUM;		\
	        GEL_hist[cpu][event].v.v = (unsigned long)V;	\
	        GEL_hist[cpu][event].line = __LINE__;		\
	        GEL_hist[cpu][event].file = __FILE__;		\
		}						\
	}

#define GFXLOGS(S)						\
	{							\
	    int s,cpu,event;					\
	    PRE_GET_TIME;					\
	    if (GEL_log) {					\
		s = mp_mutex_spinlock(&GEL_lock);		\
		cpu = cpuid();					\
		event = GEL_cnt[cpu];				\
	        GEL_cnt[cpu] = (GEL_cnt[cpu]+1)%GEL_MAXLOG;	\
	        GEL_hist[cpu][event].time = GET_TIME;		\
		mp_mutex_spinunlock(&GEL_lock,s);		\
	        GEL_hist[cpu][event].type = GEL_STRING;		\
	        GEL_hist[cpu][event].v.s = S;			\
	        GEL_hist[cpu][event].line = __LINE__;		\
	        GEL_hist[cpu][event].file = __FILE__;		\
		}						\
	}

#define GFXLOGT(T)	GFXLOGS(#T)

/* for compatibility with old HARDCORE_DEBUG stuff */
#define GFX_LOG(CMD,VAL)	{ GFXLOGS(#CMD); GFXLOGV(VAL); }

#else  /* GFX_EVENTLOG */

#define GFXLOGT(VAL)		/*nothing*/
#define GFXLOGS(VAL)		/*nothing*/
#define GFXLOGV(VAL)		/*nothing*/
#define GEL_STARTLOGGING	/*nothing*/
#define GEL_RESTARTLOGGING	/*nothing*/
#define GEL_STOPLOGGING		/*nothing*/
/* for compatibility with old HARDCORE_DEBUG stuff */
#define GFX_LOG(CMD,VAL)	/*nothing*/
/* for compatibility with old HARDCORE_DEBUG stuff */
#define GFX_STOP_LOGGING	/*nothing*/

#endif /* GFX_EVENTLOG */


#endif /* _KERNEL */

#endif /* __SYS_GFX_H__ */

