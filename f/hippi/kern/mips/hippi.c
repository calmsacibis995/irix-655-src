/*
 * hippi.c
 *
 * Challenge/Onyx HIPPI.
 *
 * This driver provides the user-level API to access HIPPI-FP and HIPPI-PH
 * layers.  It also provides hooks for the TCP/IP HIPPI driver, if_hip.
 *
 * Copyright 1994 Silicon Graphics, Inc.  All rights reserved.
 *
 */

#ident "$Revision: 1.40 $"

#include "sys/param.h"
#include "sys/conf.h"
#include "sys/debug.h"
#include "ksys/ddmap.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/sysmacros.h"
#include "sys/ioctl.h"
#include "sys/file.h"
#include "sys/errno.h"
#include "sys/cred.h"
#include "sys/uio.h"
#include "sys/immu.h"
#include "sys/cpu.h"
#include "sys/sbd.h"
#include "sys/timers.h"
#include "sys/kmem.h"
#include "sys/sema.h"
#include "sys/poll.h"
#include "sys/ddi.h"
#include "sys/cmn_err.h"
#include "sys/invent.h"
#include "sys/mbuf.h"
#include "sys/kabi.h"

#include "sys/EVEREST/everest.h"
#include "sys/EVEREST/evintr.h"
#include "sys/EVEREST/io4.h"
#include "sys/EVEREST/vmecc.h"

#include "sys/hippi.h"
#include "sys/hippibp.h"
#include "sys/hippidev.h"
#ifdef HIPPI_BP
#include "sys/idbgentry.h"
#define	HIPPIBP_UP(x)	((x)->hippibp_state & HIPPIBP_ST_CONFIGED)
#endif /* HIPPI_BP */

#include "ehip.vers"	/* get version # of firmware (ehip_vers) */

int	hippidevflag = D_MP;

extern int hippi_ndisc_perr;	/* from master.d/hippi */


/*********************************************************************
 * HIPPI character device interface routines
 *********************************************************************/

#define LENTOD2BS(l)		(4+(l)/NBPP)	/* d2b's needed for len l */

hippi_vars_t hippi_device[ EV_MAX_HIPADAPS ];


void	hippi_fp_odone( hippi_vars_t *, volatile struct hip_d2b_hd *, int );
void	hippi_fp_input( hippi_vars_t *, volatile struct hip_b2h * );
int	hippi_fp_bind( hippi_vars_t *, u_int, int, int );
void	hippi_intr( eframe_t *, void * );
void	hippifreemap_release(struct hippi_freemap *, int);

static int hippieraseflash( hippi_vars_t * );
static int hippiwriteflash( hippi_vars_t *, u_char *, u_int, int );
static u_int hippireadversion( hippi_vars_t * );

/* Prototypes for HIPPI-LE layer hooks */

void	ifhip_attach( hippi_vars_t * );
void	ifhip_le_odone( hippi_vars_t *, volatile struct hip_d2b_hd *, int );
int	ifhip_fillin( hippi_vars_t * );
void	ifhip_le_input( hippi_vars_t *, volatile struct hip_b2h * );
void	ifhip_shutdown( hippi_vars_t * );


/* Find next d2b in the ring */
#define D2B_NXT(p)	((p)+1>hippi_devp->hi_d2b_lim ? \
				&hippi_devp->hi_d2b[0] : (p)+1 )

/* Number of pages of c2b's to post largest possible read() */
#define C2B_RDLISTPGS	(((HIPPIFP_MAX_READSIZE/NBPP+3) * \
				sizeof(union hip_c2b) + NBPP-1)/ NBPP)

/* Set up in ml/EVEREST/io4.c */
extern ioadap_t io4hip_adaps[];
extern int	io4hip_cnt;

#ifdef HIPPI_DEBUG
int	hippi_debug = 0;
#endif /* DEBUG */

/************************************************************************
 *
 * HIPPI hardware control
 *
 ************************************************************************/

/* DEBUG */
int hippi_n_intrs = 0;	/* XXX */
int hippi_n_wakes = 0;	/* XXX */


/* There are a set of synchronous commands sent down to the board.
 * They do things like initialization, assigning/deassing ULPs, and
 * "waking" the board to notify it that there is work to be done in the
 * work queues (d2b for transmit, c2b for receive and receive buffer
 * management).  The commands are done by writing arguments
 * directly into the board's memory and incrementing the cmd_id.
 * The work queues, on the other hand, are kept in host memory and
 * are DMA'ed by the board
 */

/* Send (synchronous) control command down to the board */
#define hiphw_op(hi,c)	((hi)->hi_hc->cmd = (c),		\
			 (hi)->hi_cmd_line = __LINE__,		\
			 (hi)->hi_hc->cmd_id = ++(hi)->hi_cmd_id)




/* Wait up to <wait> usecs for previous command to complete.  This is
 * usually called from within the hippi_wait() macro.  The previous command
 * must complete before you muck with the arguments for the next command or
 * if you need to synchronize before doing something else.
 */
void
hippi_wait_usec(hippi_vars_t *hippi_devp, int wait, char *file, int lineno )
{
	u_int	cmd_ack;

	while ((cmd_ack=hippi_devp->hi_hc->cmd_ack) != hippi_devp->hi_cmd_id) {
		if (wait < 0) {
			cmn_err( CE_WARN,
				"hippi%d: board asleep at %s:%d with 0x%x not 0x%x after 0x%x at %d\n",
				hippi_devp->unit, file, lineno, cmd_ack,
				hippi_devp->hi_cmd_id, hippi_devp->hi_hc->cmd,
				hippi_devp->hi_cmd_line);
				break;
		}
		DELAY(OP_CHECK);
		wait -= OP_CHECK;
	}
}


/* Assuming the board has been reset, bring it into operational
 * state.
 */
static void
hippi_bd_bringup( hippi_vars_t *hippi_devp )
{
	int 	i;
	int	wait;

	/* Initialize all the global semaphores */
	initsema( & hippi_devp->rawoutq_sema,HIPPIFP_MAX_WRITES);
	initsema( & hippi_devp->src_sema, 1 );
	for (i=0; i<HIPPIFP_MAX_WRITES; i++)
		initsema( & hippi_devp->rawoutq_sleep[i], 0 );
	
	hippi_devp->rawoutq_in = 0;
	hippi_devp->rawoutq_out = 0;
	for (i=0; i<HIPPIFP_MAX_WRITES; i++)
		hippi_devp->rawoutq_error[i] = 0;
	hippi_devp->dstLock = 0;

	/* Initialize clone and ulp tables */
	bzero( hippi_devp->clone, sizeof(hippi_devp->clone) );
	for (i=0; i<HIPPIFP_MAX_CLONES; i++)
		hippi_devp->clone[i].ulpIndex = ULPIND_UNUSED;
	bzero( hippi_devp->ulp, sizeof(hippi_devp->ulp) );

	/* Initialize ULP ID table */
	for (i=0; i<=HIPPI_ULP_MAX; i++)
		hippi_devp->ulpFromId[i] = 255;
		
	/* Zero out communications areas
	 */
	bzero( (void *)hippi_devp->hi_d2b, HIP_D2B_LEN*sizeof(union hip_d2b));
	bzero( (void *)hippi_devp->hi_c2b, HIP_C2B_LEN*sizeof(union hip_c2b));
	bzero( (void *)hippi_devp->hi_b2h, HIP_B2H_LEN*sizeof(struct hip_b2h));
	
	/* Initialize DMA data to board structure.
	 */
	hippi_devp->hi_d2bp_hd = &hippi_devp->hi_d2b[0];
	hippi_devp->hi_d2bp_tl = &hippi_devp->hi_d2b[0];
	hippi_devp->hi_d2b_lim = &hippi_devp->hi_d2b[HIP_D2B_LEN-1];
	hippi_devp->hi_d2bp_hd->hd.flags = HIP_D2B_BAD;

	/* Initialize Board-to-host interrupt ring.
	 */
	hippi_devp->hi_b2h_sn = 1;
	hippi_devp->hi_b2h_active = 0;
	for (i=0; i<HIP_B2H_LEN; i++)
		hippi_devp->hi_b2h[i].b2h_sn = (u_char)(i&0xFF);
	hippi_devp->hi_b2hp = & hippi_devp->hi_b2h[0];
	hippi_devp->hi_b2h_lim = & hippi_devp->hi_b2h[HIP_B2H_LEN-1];

#if (HIP_B2H_LEN % 256) == 0
#error "HIP_B2H_LEN can't be a multiple of 256"
#endif

	/* Initialize Host-to-board control ring
	 */
	hippi_devp->hi_c2bp = & hippi_devp->hi_c2b[0];
	hippi_devp->hi_c2bp->c2b_op = HIP_C2B_EMPTY;

	hippi_devp->hi_state = (HI_ST_SLEEP|HI_ST_UP);
	hippi_devp->hi_cmd_id = 1;
	hippi_devp->hi_hwflags = hippi_ndisc_perr ?
			(HIP_FLAG_ACCEPT|HIP_FLAG_NODISC) : HIP_FLAG_ACCEPT;
	hippi_devp->hi_stimeo = HIPPI_DEFAULT_STIMEO;
	hippi_devp->hi_dtimeo = HIPPI_DEFAULT_DTIMEO;

	/* Wait for firmware to be ready to accept init command
	 */

	/* point PIO region 1 to 0x0480000 on the board and AM=0x0F */
	EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1), 0x00003C09 );
	wait = 1500000;	/* XXX 1.5 seconds! */
	while ( wait > 0 && hippi_devp->hi_hc->sign != HIP_SIGN ) {
		DELAY(OP_CHECK);
		wait -= OP_CHECK;
	}
	if ( wait < 0 ) {
		cmn_err( CE_WARN,
			"hippi%d: no board signature!\n", hippi_devp->unit );
		return;
	}
	/* point PIO region 1 to 0x0400000 on the board and AM=0x0F */
	EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1), 0x00003C08 );

	/* Next, initialize the card with all the communications
	 * areas and parameters.  The in-host data structures are
	 * always in k0seg so we don't bother with address bits [39:32].
	 */
#define HINIT	hippi_devp->hi_hc->arg.init

	HINIT.b2h_buf_hi =	0;
	HINIT.b2h_buf =	(__uint32_t)kvtophys((void *)hippi_devp->hi_b2hp );
	HINIT.b2h_len =	 	HIP_B2H_LEN;
	HINIT.iflags =   	hippi_devp->hi_hwflags;
	HINIT.d2b_buf_hi =	0;
	HINIT.d2b_buf =	(__uint32_t)kvtophys((void *) hippi_devp->hi_d2bp_hd );
	HINIT.d2b_len =		HIP_D2B_LEN;
	HINIT.host_nbpp_mlen =	(NBPP | MLEN);
	HINIT.c2b_buf_hi =	0;
	HINIT.c2b_buf = (__uint32_t)kvtophys((void *)hippi_devp->hi_c2b );
	HINIT.stat_buf_hi =	0;	/* XXX: obsolete */
	HINIT.stat_buf =	0;	/* XXX: obsolete */

#undef HINIT
	/* Write into the cmd_ack to initialize controller memory parity.
	 * This lets us poll the cmd_ack for completion of the HCMD_INIT
	 * wihtout getting a Bus Error due to bad controller memory parity.
	 */
	hippi_devp->hi_hc->cmd_ack = 0;

	hiphw_op(hippi_devp,HCMD_INIT);

	/* First INIT needs an interrupt */
	*hippi_devp->hi_bdctl = HIP_BDCTL_29K_INT;
}


/* This brings down the HIPPI board and deallocates all the resources.
 * It even hits the HIPPI board with a reset.
 */
static void
hippi_bd_shutdown( hippi_vars_t *hippi_devp )
{
	int	i, j;
	struct hippi_fp_ulps *ulpp;

	mutex_lock( & hippi_devp->devslock, PZERO );

	hippi_devp->hi_state = 0;
#ifdef HIPPI_BP
	/* Force new SET_BPCFG message to be issued */
	hippi_devp->hippibp_state &= ~(HIPPIBP_ST_CONFIGED|HIPPIBP_ST_OPENED);
#endif	

	/* Reset the HIPPI board */
	*hippi_devp->hi_bdctl = HIP_BDCTL_RESET_29K;
	*hippi_devp->hi_bdctl = 0;

	/* Tell if_hip() that we're closing shop. */
	ifhip_shutdown( hippi_devp );

	/* Free up anybody waiting on source semaphores.
	 * V it a few more times to avoid race where another
	 * processor may be about to P a semaphore. XXX gag.
	 */
	while ( cvsema( & hippi_devp->rawoutq_sema ) )
		;
	for (j=0; j<50; j++)
		vsema( & hippi_devp->rawoutq_sema );

	while ( cvsema( & hippi_devp->src_sema ) )
		;
	for (j=0; j<50; j++)
		vsema( & hippi_devp->src_sema );

	for (i=0; i<HIPPIFP_MAX_WRITES; i++) {
		while ( cvsema( & hippi_devp->rawoutq_sleep[i] ) )
			;
		for (j=0; j<50; j++)
			vsema( & hippi_devp->rawoutq_sleep[i] );
	}
	
	for (i=0; i<HIPPIFP_MAX_OPEN_ULPS+1; i++) {
	   ulpp = & hippi_devp->ulp[i];
	   if ( ulpp->opens > 0 ) {

		/* Free up anybody waiting on destination semaphores */
		while ( cvsema( & ulpp->rd_sema ) )
			;
		for (j=0; j<50; j++)
			vsema( & ulpp->rd_sema );
		while ( cvsema( & ulpp->rd_dmadn ) )
			;
		for (j=0; j<50; j++)
			vsema( & ulpp->rd_dmadn );
		if ( ulpp->ulpFlags & ULPFLAG_R_POLL ) {
			pollwakeup( ulpp->rd_pollhdp,  POLLERR );
			ulpp->ulpFlags &= ~ULPFLAG_R_POLL;
		}
	   }
	}

	mutex_unlock( & hippi_devp->devslock );
}


/* There is only one hippiinit() for all boards installed.
 * It is up to lower level routines in ml/EVEREST/io4.c and
 * ml/EVEREST/hipinit.c to find the HIPPI boards and put their
 * locations into the io4hip_adaps[] array.
 *
 * This routine doesn't bring the board up to full operational
 * state.  hipcntl(1m) gets a chance to check the firmware revision
 * and reprogram the board's EEPROM (and reset it again) before
 * hippi_bd_bringup() is called.
 */
void
hippiinit()
{
	int	unit, j;
	hippi_vars_t *hippi_devp;
	ioadap_t *io4hipp;
	graph_error_t rc;
	vertex_hdl_t io4vhdl, hipvhdl, hippivhdl;
	char loc_str[16];

#ifdef HIPPI_DEBUG
	printf("hippiinit: running HIPPI_DEBUG hippi driver.\n");
#endif

	for (unit=0; unit < io4hip_cnt; unit++) {
		hippi_devp = & hippi_device[unit];
		io4hipp = & io4hip_adaps[unit];

		hippi_devp->unit = unit;

		/* initialize clone table */
		for (j=0; j<HIPPIFP_MAX_CLONES; j++)
			hippi_devp->clone[j].ulpIndex = ULPIND_UNUSED;
		
		/* initialize these locks only once */
		init_mutex( & hippi_devp->devmutex, MUTEX_DEFAULT,
			"hipmut", unit );
		init_mutex( & hippi_devp->devslock, MUTEX_DEFAULT,
			"hipsmu", unit );
		
		/*
		 * Set up F/VMECC and get board addresses mapped in.
		 */
		hippi_devp->hi_swin = io4hipp->swin;
		hippi_devp->hi_bdctl = (__uint32_t *)(io4hipp->swin + 0x4000);

		io4hipp->lwin = (__psunsigned_t) iospace_alloc( HIPPI_LW_SIZE,
			LWIN_PFN( SWIN_REGION(io4hipp->swin), io4hipp->padap));
		
		/* Program F:
		 *
		 * bit 0: order read resp
		 * bit 1: allow cmd overlap
		 * bit 2: dma_rd_4lines
		 * bits 5..3: 4 cache lines fetch ahead
		 */
		
		EV_SET_REG( io4hipp->swin + FCHIP_MODE, 0x27 );
		
		/* Program VMECC:
		 * CONFIG:
		 *	bit 0 - set to turn VMECC_RESET *off*
		 *	bit 1 - quicker PIOs (?)
		 *	bit 5 - VMECC slave responds to privileged AM patterns
		 *	bit 6 - disable VME slave coalescing
		 *	bit 7 - FCI command overlap enable
		 *	bit 11 - enable A64 slave operations
		 */

		/* XXX: flop around the reset pin to be sure the
		 * arbitrator is synced to the VMECC.
		 */
		EV_SET_REG( io4hipp->swin + VMECC_CONFIG, 0x00000001 );
		EV_SET_REG( io4hipp->swin + VMECC_CONFIG, 0x00000000 );
		EV_SET_REG( io4hipp->swin + VMECC_CONFIG, 0x000008E3 );

		EV_GET_REG( io4hipp->swin + VMECC_ERRCAUSECLR );

		EV_SET_REG( io4hipp->swin + VMECC_PIOREG(0), 0x00000000 );

		/* Clear 29K reset */
		*hippi_devp->hi_bdctl = 0;
		*hippi_devp->hi_bdctl = HIP_BDCTL_RESET_29K;
		*hippi_devp->hi_bdctl = 0;

		/* These point into big window space. */
		hippi_devp->hi_hc = (volatile struct hip_hc *)
			( io4hipp->lwin + VMECC_PIOREG_MAPADDR(1) );
		hippi_devp->hi_stat_area = (volatile struct hippi_stats *)
			& hippi_devp->hi_hc[1] ;

		/* point PIO region 1 to 0x0400000 on the board and AM=0x0F */
		EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1), 0x00003C08);
		
#ifdef HIPPI_DEBUG
		printf("hippiinit: unit = %d  hi_bdctl = %x\n", unit,
			(long) hippi_devp->hi_bdctl );
#endif

		/* Set up board/host communications areas in memory.
		 */
		hippi_devp->hi_b2h = (volatile struct hip_b2h *)
			kvpalloc( (HIP_B2H_LEN*sizeof(struct hip_b2h)+NBPP-1)/
					NBPP, VM_DIRECT|VM_STALE,0);
		hippi_devp->hi_d2b = (volatile union hip_d2b *)
			kvpalloc( (HIP_D2B_LEN*sizeof(union hip_d2b)+NBPP-1)/
					NBPP, VM_DIRECT|VM_STALE,0);
		hippi_devp->hi_c2b = (volatile union hip_c2b *)
			kvpalloc( (HIP_C2B_LEN*sizeof(union hip_c2b)+NBPP-1)/
					NBPP, VM_DIRECT|VM_STALE,0);
		
		/* Set up interrupts */
		evintr_connect( (evreg_t *)(io4hipp->swin + VMECC_VECTORAUX0),
			EVINTR_LEVEL_HIPPI(unit&3), SPLDEV,
			EVINTR_ANY, hippi_intr, (void *) ((long)unit&3) );

		EV_SET_REG( io4hipp->swin + VMECC_INT_ENABLESET,
			1L<<VMECC_INUMAUX0 );

#ifdef HIPPI_DEBUG
		/* DEBUG */
		printf( "hippi_init%d:  comm area = %x\n",
			unit, (long) hippi_devp->hi_hc );
		printf( "hippi_init%d:  d2b area = %x\n",
			unit, (long) hippi_devp->hi_d2b );
		printf( "hippi_init%d:  b2h area = %x\n",
			unit, (long) hippi_devp->hi_b2h );
		printf( "hippi_init%d:  c2b area = %x\n",
			unit, (long) hippi_devp->hi_c2b );
#endif

#ifdef HIPPI_DEBUG
		hippi_devp->hi_firmvers = 0;
#else
		hippi_devp->hi_firmvers = hippireadversion( hippi_devp );
		if ( hippi_devp->hi_firmvers == 0 )
			hippi_devp->hi_firmvers = -1;
#endif

		/* Add to inventory */
		add_to_inventory(INV_NETWORK, INV_NET_HIPPI, INV_HIO_HIPPI,
		    unit, io4hipp->slot<<28 | io4hipp->padap<<24 |
		    hippi_devp->hi_firmvers );

		rc = io4_hwgraph_lookup(io4hipp->slot, &io4vhdl);
		if (rc == GRAPH_SUCCESS) {
			sprintf(loc_str, "%s/%d", "hippi", unit);
			rc = hwgraph_char_device_add(io4vhdl, loc_str,
				"hippi", &hipvhdl);
		}

		/*
		 * Create alias for this device in /hw/hippi. First, get 
		 * /hw/hippi vertex, creating one if it doesn't yet exist.
		 */
		if (hwgraph_traverse (GRAPH_VERTEX_NONE, "hippi", &hippivhdl)
			!= GRAPH_SUCCESS) {
			if (hwgraph_path_add (GRAPH_VERTEX_NONE,
				"hippi", &hippivhdl) != GRAPH_SUCCESS) {
				cmn_err(CE_ALERT,
				    "hipinit: %d: Couldn't create /hw/hippi.",
					unit);
				continue;
			}
		}

		sprintf(loc_str, "%d", unit);
		hwgraph_edge_add(hippivhdl, hipvhdl, loc_str);

		ifhip_attach( hippi_devp );
	}
}




/* Process b2h interrupt ring.  This is the work queue with stuff
 * the board wants us to do.  Interrupts must be off.  This routine
 * is sometimes called "from above" (i.e. an application thread)
 * as well as from the interrupt stack.
 */
int
hippi_b2h( hippi_vars_t *hippi_devp )
{
	int 	i;
	int	pokebd, work = 0;
	volatile struct hip_b2h *b2hp;
	static	hippi_b2h_calls = 0;	/* XXX */
	static	hippi_b2h_work = 0;	/* XXX */

	/* Only enter this routine once */
	if ( hippi_devp->hi_b2h_active )
		return work;
	hippi_devp->hi_b2h_active = 1;

	pokebd = 0;
	b2hp = hippi_devp->hi_b2hp;
	for (;;) {
		if ( b2hp->b2h_sn != hippi_devp->hi_b2h_sn ) {
			/* no more work */

			if ( ifhip_fillin( hippi_devp ) > 0 ) {
				/* Fix race in board going to sleep as host
				 * is giving it buffers.
				 */
				pokebd = 1;
				continue;
			}

			if (pokebd && (hippi_devp->hi_state & HI_ST_SLEEP) ) {
				hippi_wait( hippi_devp );
				hippi_devp->hi_state &= ~HI_ST_SLEEP;
				hiphw_op(hippi_devp,HCMD_WAKEUP);
				hippi_n_wakes++;
			}
			hippi_devp->hi_b2h_active = 0;
			hippi_devp->hi_b2hp = b2hp;
			hippi_b2h_calls++;	/* XXX */
			hippi_b2h_work += work;	/* XXX */
			return work;
		}
		work++;

		switch ( b2hp->b2h_op & HIP_B2H_OPMASK ) {

		case HIP_B2H_NOP:
#ifdef HIPPI_DEBUG
			printf("hippi_b2h:  NOP!?\n");
#endif
			break;

		case HIP_B2H_SLEEP:
			hippi_devp->hi_state |= HI_ST_SLEEP;
#ifdef HIPPI_DEBUG
			if ( hippi_debug )
o				printf("hippi_b2h:  board went to sleep!!!\n");
#endif
			break;

		case HIP_B2H_ODONE:
			/* Free up buffers
			 */
			i = b2hp->b2h_ndone;
			while ( i-- > 0 ) {
			   volatile struct hip_d2b_hd *hd;

			   do {
				hd = &hippi_devp->hi_d2bp_tl->hd;

				ASSERT( hd->flags & HIP_D2B_RDY );

				hippi_devp->hi_d2bp_tl += hd->chunks+1;
				if ( hippi_devp->hi_d2bp_tl >
						hippi_devp->hi_d2b_lim )
					hippi_devp->hi_d2bp_tl -= HIP_D2B_LEN;
			   } while ( hd->flags & HIP_D2B_NACK );

			   ASSERT( (b2hp->b2h_op&HIP_B2H_STMASK) == hd->stk );

			   if ( hd->stk == HIP_STACK_LE )
				ifhip_le_odone( hippi_devp, hd,
						b2hp->b2h_ostatus );
			   else
				hippi_fp_odone( hippi_devp, hd,
						b2hp->b2h_ostatus );
			}
			break;

		case HIP_B2H_IN_DONE:
			if ( (b2hp->b2h_op&HIP_B2H_STMASK) == HIP_STACK_LE ) {

				/* board wakes up if it gives us HIPPI-LE input
				 */
				hippi_devp->hi_state &= ~HI_ST_SLEEP;
				ifhip_le_input( hippi_devp, b2hp );
			}
			else
				hippi_fp_input( hippi_devp, b2hp );
			break;

		case HIP_B2H_IN:
			hippi_fp_input( hippi_devp, b2hp );
			break;

#ifdef HIPPI_BP
		case HIP_B2H_BP_PORTINT:
			{
			struct hippi_port *port;
			int signo;
			
			port = &hippi_devp->hippibp_portids[
				b2hp->b2hu.b2h_bp_portint.portid];
			signo = port->signo;

			if (port->pid)
				pid_signal(port->pid, signo, 0, 0);

			hippi_devp->hi_hc->arg.bp_portint_ack.portid =
				b2hp->b2hu.b2h_bp_portint.portid;
			hippi_devp->hi_hc->arg.bp_portint_ack.cookie =
				b2hp->b2h_l;
			hippi_wait( hippi_devp );
			hiphw_op(hippi_devp, HCMD_BP_PORTINT_ACK);
			break;
			}
#endif /* HIPPI_BP */
		default:
			/* XXX: should probably panic! */
			printf("hippi_b2h: unknown op/s: %d\n", b2hp->b2h_op );
			break;
		}

		hippi_devp->hi_b2h_sn++;
		b2hp++;
		if ( b2hp > hippi_devp->hi_b2h_lim )
			b2hp = & hippi_devp->hi_b2h[0];
	}
	/*NOTREACHED*/
}


/* Interrupt entry point.  Go find b2h work from card.
 *
 * In configurations with more than 4 HIPPI cards, we share
 * interrupt vectors amongst multiple boards.  So, if there's
 * more than 4 boards, check the b2h work queues for both
 * boards that have that interrupt vector.
 */

/*ARGSUSED*/
void
hippi_intr( eframe_t *ef, void *arg )
{
	int		unit;
	hippi_vars_t	*hippi_devp;

	hippi_n_intrs++;

	unit = (int) (long) arg;

	if ( io4hip_cnt <= 4 ) {

		/* This is the normal interrupt case with 4 boards or
		 * less.  "Same as it ever was..."
		 */

		hippi_devp = & hippi_device[unit];

		if ( ! (hippi_devp->hi_state & HI_ST_UP) )
			return;

		/* Clear board-to-host interrupt.  Reading the board-control
		 * register after zeroing the host-interrupt bit serves two
		 * purposes:  it makes sure the PIO that zeros the register
		 * has made it down to the board before we start b2h work and
		 * it also eliminates a problem when both the board
		 * and the host write the register at the same instant.
		 *
		 * This doesn't need to be protected by a spin-lock.
		 */
		*hippi_devp->hi_bdctl = 0;
		while ( HIP_BDCTL_HOST_INT & *hippi_devp->hi_bdctl )
			*hippi_devp->hi_bdctl = 0;

		mutex_lock( & hippi_devp->devslock, PZERO );

		(void)hippi_b2h( hippi_devp );

		mutex_unlock( & hippi_devp->devslock );
	}
	else {
		/* This is the abnormal interrupt case when we have more
		 * than 4 boards.  Each board shares its interrupt vector
		 * with another.
		 */
		hippi_vars_t	*hippi_devp2;

		hippi_devp = & hippi_device[unit];
		hippi_devp2 = & hippi_device[unit+4];

		if ( ! (hippi_devp->hi_state & HI_ST_UP) )
			hippi_devp = 0;
		if ( !(hippi_devp2->hi_state&HI_ST_UP)|| unit+4 >= io4hip_cnt )
			hippi_devp2 = 0;
		
		/* Clear board-to-host interrupt.  Reading the board-control
		 * register after zeroing the host-interrupt bit serves two
		 * purposes:  it makes sure the PIO that zeros the register
		 * has made it down to the board before we start b2h work and
		 * it also eliminates a problem when both the board
		 * and the host write the register at the same instant.
		 *
		 * These don't need to be protected by spin-locks.
		 */
		if ( hippi_devp ) {
			*hippi_devp->hi_bdctl = 0;
			while ( HIP_BDCTL_HOST_INT & *hippi_devp->hi_bdctl )
				*hippi_devp->hi_bdctl = 0;
		}
		if ( hippi_devp2 ) {
			*hippi_devp2->hi_bdctl = 0;
			while ( HIP_BDCTL_HOST_INT & *hippi_devp2->hi_bdctl )
				*hippi_devp2->hi_bdctl = 0;
		}

		if ( hippi_devp ) {
			mutex_lock( & hippi_devp->devslock, PZERO );
			(void) hippi_b2h( hippi_devp );
			mutex_unlock( & hippi_devp->devslock );
		}
		if ( hippi_devp2 ) {
			mutex_lock( & hippi_devp2->devslock, PZERO );
			(void) hippi_b2h( hippi_devp2 );
			mutex_unlock( & hippi_devp2->devslock );
		}
	}
}


/* Called to send a WAKEUP command to the board.  It checks to
 * see if the board is asleep before it bothers.
 */
void
hippi_wakeup( hippi_vars_t *hippi_devp )
{
	mutex_lock( & hippi_devp->devslock, PZERO );

	if ( 0 != (hippi_devp->hi_state & HI_ST_UP) ) {

	    (void)hippi_b2h( hippi_devp );

	    if ( 0 != (hippi_devp->hi_state & HI_ST_SLEEP) ) {
		hippi_wait( hippi_devp );
		hiphw_op( hippi_devp, HCMD_WAKEUP );
		hippi_n_wakes++;
		hippi_devp->hi_state &= ~HI_ST_SLEEP;
	    }
	}

	mutex_unlock( & hippi_devp->devslock );
}

/* Same as hippi_wakeup() but assumes caller has already grabbed the
 * device mutex.  You MUST have that lock to call this.
 */
void
hippi_wakeup_nolock( hippi_vars_t *hippi_devp )
{
	if ( 0 != (hippi_devp->hi_state & HI_ST_UP) ) {

	    (void)hippi_b2h( hippi_devp );

	    if ( 0 != (hippi_devp->hi_state & HI_ST_SLEEP) ) {
		hippi_wait( hippi_devp );
		hiphw_op( hippi_devp, HCMD_WAKEUP );
		hippi_n_wakes++;
		hippi_devp->hi_state &= ~HI_ST_SLEEP;
	    }
	}
}


/* Queue up a C2B.  Assumed interrupts are off. */
void
hippi_send_c2b( hippi_vars_t *hippi_devp, int ops, int param, void *addr )
{
	volatile union hip_c2b *c2bp = hippi_devp->hi_c2bp;
	volatile union hip_c2b *c2bp2 = c2bp;

	if ( ++c2bp2 >= & hippi_devp->hi_c2b[ HIP_C2B_LEN-1 ] ) {
		c2bp2->c2bl[0] = HIP_C2B_WRAP<<8;
		c2bp2 = & hippi_devp->hi_c2b[0];
	}
	c2bp2->c2bl[0] = HIP_C2B_EMPTY<<8;

#if _MIPS_SIM == _ABI64
	c2bp->c2bll = ((long)param<<48)|((long)ops<<40)|( (u_long) addr );
#else
	c2bp->c2b_addr = (u_int) addr;

	/* It's critical that the opcode, "ops", be put in LAST! */

	c2bp->c2bl[0] = (param<<16)|(ops<<8);	/* addrhi = 0 */
#endif

	hippi_devp->hi_c2bp = c2bp2;
}


/* Send new flags and parameters to card.  Interrupts should be OFF. */
void
hippi_hwflags( hippi_vars_t *hippi_devp, int newflags )
{
	mutex_lock( & hippi_devp->devslock, PZERO );

	hippi_wait( hippi_devp );

	hippi_devp->hi_hwflags = newflags;

	hippi_devp->hi_hc->arg.params.flags = hippi_devp->hi_hwflags;
	hippi_devp->hi_hc->arg.params.stimeo = hippi_devp->hi_stimeo;
	hippi_devp->hi_hc->arg.params.dtimeo = hippi_devp->hi_dtimeo;

	hiphw_op( hippi_devp, HCMD_PARAMS );

	mutex_unlock( & hippi_devp->devslock );

	return;
}



#ifdef HIPPI_BP

#ifdef HIPPI_BP_DEBUG

int	hippibp_debug = 0;	/* Controls printing of debug info */
#define dprintf(lvl, x)	if (hippibp_debug>=lvl) printf x

#else /* !HIPPI_BP_DEBUG */

#define dprintf(lvl, x)

#endif


int
hippibp_wait_usec(hippi_vars_t *hippi_devp, int wait)
{
	u_int	cmd_ack;

	while ((cmd_ack=hippi_devp->hi_hc->cmd_ack) != hippi_devp->hi_cmd_id) {
		if (wait < 0) {
			cmn_err( CE_WARN,
				"hippi%d: board asleep with 0x%x not 0x%x after 0x%x at %d\n",
				hippi_devp->unit, cmd_ack,
				hippi_devp->hi_cmd_id, hippi_devp->hi_hc->cmd,
				hippi_devp->hi_cmd_line);
				break;
		}
		DELAY(OP_CHECK);
		wait -= OP_CHECK;
	}
	return(wait);
}

/* Following fields point to a single page in the system which can be
 * used to "fill-in" unused entries in the various Hippi ByPass page maps.
 * This allows the Hippi Firmware to transfer to any "page index" in the
 * list without checking for a valid entry (i.e. all entries are "valid"
 * for transfer purposes).
 */

void	*hippibp_garbage_addr;
int	hippibp_garbage_pfn;


static void
hippibp_idbg_freemap(struct hippi_freemap *fmap)
{
	int i;

	qprintf("ID 0x%x  pfns 0x%x vaddr 0x%x pgcnt 0x%x\n",
		fmap->ID, fmap->pfns, fmap->vaddr, fmap->pgcnt);

	for (i=0; i<fmap->pgcnt; i++) {
		qprintf("slot %d  pfn 0x%x\n", i, fmap->pfns[i]);
	}
}

static void
hippibp_idbg(int argpc)
{
	int unit, job, maxunit=io4hip_cnt;
	hippi_vars_t *hippi_devp;
	struct hippi_bp_job *bpjob;

	qprintf("hippibp_idbg: called with argpc 0x%x\n", argpc);

	for (unit=0; unit<maxunit; unit++) {
		hippi_devp = &hippi_device[unit];
		qprintf("hippi unit %d  ulp 0x%x maxjobs %d maxportidp %d\n",
			hippi_devp->bp_ulp, hippi_devp->bp_maxjobs,
			hippi_devp->bp_maxportids);
		qprintf("maxdfmpgs %d maxsfmpgs %d maxddqpgs %d\n",
			hippi_devp->bp_maxdfmpgs, hippi_devp->bp_maxsfmpgs,
			hippi_devp->bp_maxddqpgs);
		for (job=0; job < hippi_devp->bp_maxjobs; job++) {
			bpjob = &hippi_devp->bp_jobs[job];
			qprintf("hippi unit %d job %d\n", unit, job);
			qprintf("jobFlags 0x%x portmax %d portused %d\n",
				bpjob->jobFlags, bpjob->portmax,bpjob->portused);
			qprintf("portidPagemap\n");
			hippibp_idbg_freemap( &bpjob->portidPagemap );
			qprintf("sourceFreemap\n");
			hippibp_idbg_freemap( &bpjob->Sfreemap );
			qprintf("destinationFreemap\n");
			hippibp_idbg_freemap( &bpjob->Dfreemap );
		}
	}
}

int
hippibp_config_driver( hippi_vars_t *hippi_devp)
{
	int job, i, pgs;
	struct hippi_bp_job *bpjob;
	volatile __uint32_t *FWbase;

	if (hippibp_garbage_pfn == 0) {
		if ((hippibp_garbage_addr = kvpalloc(1,0,0)) == NULL)
			return ENOMEM;
		hippibp_garbage_pfn = kvatopfn(hippibp_garbage_addr);

		dprintf(1,("hippibp_config_driver: allocate gpage 0x%x pfn 0x%x\n",
			hippibp_garbage_addr, hippibp_garbage_pfn));
	}

	for (job = 0;
	     job < hippi_devp->cached_bp_fw_conf.num_jobs; job++) {

		pgs = hippi_devp->cached_bp_fw_conf.dfm_size/sizeof(int);
		FWbase = hippi_devp->hi_bp_dfreemap + job * pgs;

		for (i=0; i < pgs; i++)
			FWbase[i] = hippibp_garbage_pfn;

		pgs = hippi_devp->cached_bp_fw_conf.sfm_size/sizeof(int);
		FWbase = hippi_devp->hi_bp_sfreemap + job * pgs;

		for (i=0; i < pgs; i++)
			FWbase[i] = hippibp_garbage_pfn;
		/*
		 * Software (driver) re-initialization
		 */

		if (job >= HIPPIBP_MAX_JOBS)
			continue;

		bpjob = &hippi_devp->bp_jobs[job];
		bpjob->portused = 0;
		bpjob->hippibp_DFlistPage =
			hippi_devp->hi_bp_dfreelist +
				job * hippi_devp->cached_bp_fw_conf.dfl_size;
	}

	/*
	 * Software (driver) re-initialization
	 */

	if (hippi_devp->hippibp_portids == NULL) {
		hippi_devp->hippibp_portids =
			kmem_alloc(sizeof(struct hippi_port)*
				HIPPIBP_MAX_PORTIDS, KM_SLEEP);
	}

	for (i=0; i<HIPPIBP_MAX_PORTIDS; i++)
		hippi_devp->hippibp_portids[i].jobid = (char)-1;

	hippi_devp->hippibp_portfree =
		hippi_devp->hippibp_portmax =
			hippi_devp->bp_maxportids;
	return 0;
}

/* hippibpinit
 * Read firmware configuration information and caches it in the device
 * table.  Setup bp_jobs structure if not already allocated.
 *
 * NOTE: May be called more than once (i.e. whenever firmware reloaded).
 * Intended to be called from HIPIOC_SET_BPCFG.
 */
int
hippibpinit( hippi_vars_t *hippi_devp )
{
	
	volatile __uint32_t *FWinfo;
	__uint32_t *cachedInfo;
	int i;
	char *FWbaseaddr;

	if (!(hippi_devp->hi_state & HI_ST_UP))
		return( ENODEV );

	/* Need to wait for preceeding HCMD_INIT to finish (may have just
	 * reset the board), since FWinfo not setup until that command
	 * completes.
	 * NOTE: Board may take a long time and we may timeout. Return
	 *	 EBUSY to "hipcntl" so it can goto sleep and retry later.
	 */
	mutex_lock( &hippi_devp->devslock, PZERO );
	if (hippibp_wait_usec(hippi_devp, DELAY_OP) < 0) {
		mutex_unlock( &hippi_devp->devslock );
		return EBUSY;
	}
	mutex_unlock( &hippi_devp->devslock );

	hippi_devp->bp_fw_conf =(volatile struct hip_bp_fw_config *)
	        ((char *)hippi_devp->hi_stat_area + sizeof(struct hippi_stats)
		+ C2B_D2B_D2BREAD_SIZE);

	/* Copy all configuration info from firmware into controller device
	 * table for cached access (only field which is dynamic is the
	 * dma_status).
	 */
	FWinfo = (volatile __uint32_t *) hippi_devp->bp_fw_conf;
	cachedInfo = (__uint32_t *)&hippi_devp->cached_bp_fw_conf;
	for (i=0; i< (sizeof(struct hip_bp_fw_config)/sizeof(__uint32_t));i++)
		cachedInfo[i] = FWinfo[i];

	FWbaseaddr = (char *)hippi_devp->hi_hc;

	hippi_devp->hi_bp_ifield = (volatile __uint32_t *)
		(FWbaseaddr + hippi_devp->cached_bp_fw_conf.hostx_base);

	hippi_devp->hi_bp_stats = (volatile __uint32_t *)
		(FWbaseaddr + hippi_devp->cached_bp_fw_conf.bpstat_base);

	hippi_devp->hi_bp_sdhead = (volatile char *)
		(FWbaseaddr + hippi_devp->cached_bp_fw_conf.bpjob_base);

	hippi_devp->hi_bp_sdqueue = (volatile char *)
		(FWbaseaddr + hippi_devp->cached_bp_fw_conf.sdq_base);

	hippi_devp->hi_bp_dfreelist =  (volatile char *)
		(FWbaseaddr + hippi_devp->cached_bp_fw_conf.dfl_base);

	hippi_devp->hi_bp_sfreemap = (volatile __uint32_t *)
		(FWbaseaddr + hippi_devp->cached_bp_fw_conf.sfm_base);

	hippi_devp->hi_bp_dfreemap = (volatile __uint32_t *)
		(FWbaseaddr + hippi_devp->cached_bp_fw_conf.dfm_base);

	dprintf(1, ("hippiinit: sfreemap 0x%x ifield 0x%x\n",
		hippi_devp->hi_bp_sfreemap, hippi_devp->hi_bp_ifield));
	dprintf(1, ("hippiinit: dfreemap 0x%x dfreelist 0x%x\n",
		hippi_devp->hi_bp_dfreemap, hippi_devp->hi_bp_dfreelist));
	dprintf(1, ("hippiinit: sdqueue 0x%x sdhead 0x%x\n",
		hippi_devp->hi_bp_sdqueue, hippi_devp->hi_bp_sdhead));

	dprintf(1, ("hippibpinit: num_jobs %d num_ports %d\n",
		hippi_devp->cached_bp_fw_conf.num_jobs,
		hippi_devp->cached_bp_fw_conf.num_ports));
	dprintf(1, ("hippibpinit: hostx base 0x%x size 0x%x\n",
		hippi_devp->cached_bp_fw_conf.hostx_base,
		hippi_devp->cached_bp_fw_conf.hostx_size));
	dprintf(1, ("hippibpinit: dfl base 0x%x size 0x%x\n",
		hippi_devp->cached_bp_fw_conf.dfl_base,
		hippi_devp->cached_bp_fw_conf.dfl_size));
	dprintf(1, ("hippibpinit: sfm base 0x%x size 0x%x\n",
		hippi_devp->cached_bp_fw_conf.sfm_base,
		hippi_devp->cached_bp_fw_conf.sfm_size));
	dprintf(1, ("hippibpinit: dfm base 0x%x size 0x%x\n",
		hippi_devp->cached_bp_fw_conf.dfm_base,
		hippi_devp->cached_bp_fw_conf.dfm_size));
	dprintf(1, ("hippibpinit: bpstat base 0x%x size 0x%x\n",
		hippi_devp->cached_bp_fw_conf.bpstat_base,
		hippi_devp->cached_bp_fw_conf.bpstat_size));
	dprintf(1, ("hippibpinit: sdq base 0x%x size 0x%x\n",
		hippi_devp->cached_bp_fw_conf.sdq_base,
		hippi_devp->cached_bp_fw_conf.sdq_size));
	dprintf(1, ("hippibpinit: bpjob base 0x%x size 0x%x\n",
		hippi_devp->cached_bp_fw_conf.bpjob_base,
		hippi_devp->cached_bp_fw_conf.bpjob_size));


	if (hippi_devp->bp_jobs == 0) {
		idbg_addfunc("hippibp", hippibp_idbg);
		hippi_devp->bp_jobs =
			kmem_zalloc(HIPPIBP_MAX_JOBS*
				sizeof(struct hippi_bp_job), KM_NOSLEEP);
	}

	return(0);
}

void hippibp_freemap_pfn_alloc(struct hippi_freemap *freemap, int maxpgs)
{
	int i;

	/* First time job is ever opened, allocate page to contain
	 * list of physical page numbers for freemap.
	 *
	 * NOTE: We use MAXEVERpgs which gives the largest limits that
	 * any HIPIOC_SET_BPCFG command could ever set, since we only
	 * perform this allocation once per system boot (we could fix
	 * this to deallocate before new CFG and then use maxpgs).
	 */

	if (freemap->pfns == (int *)0) {

		freemap->pfns =
			kmem_alloc( maxpgs*sizeof(int), KM_SLEEP );

		/* Initialize freemap physical page list to point
		 * to garbage page.
		 */

		for (i=0; i<maxpgs; i++)
			freemap->pfns[i] = hippibp_garbage_pfn;

		dprintf(1,("hippibp_freemap_pfn_alloc: pfn array 0x%x : 0x%x\n",
		 	freemap->pfns, maxpgs*sizeof(int)));
	}
}

/************************************************************************
 *
 * HIPPI By Pass character device interface extensions
 *
 *
 ************************************************************************/

int
hippibpopen( dev_t *devp, int flag )
{
	int 	job, error = 0;
	u_int	emaj = getemajor( *devp );
	u_int	m = geteminor( *devp );
	u_int	unit = UNIT_FROM_MINOR( m );
	hippi_vars_t *hippi_devp;

	/* Check for valid unit number */

	if ( unit >= io4hip_cnt || ISCLONE_FROM_MINOR(m) ) {
		return ENODEV;
	}

	hippi_devp = & hippi_device[unit];

	mutex_lock( &hippi_devp->devmutex, PZERO );

	/* Verify that job number is valid for this system and that
	 * the bp_jobs structure has been allocated by the sysadmin
	 * via a SET_BPCFG ioctl call.
	 */
	job = JOBID_FROM_MINOR( m );
	if ((job >= HIPPIBP_MAX_JOBS) ||
	    (job >= hippi_devp->cached_bp_fw_conf.num_jobs) ||
	    ((hippi_devp->hippibp_state & HIPPIBP_ST_CONFIGED) == 0)) {
		error = ENXIO;
		goto dontopen;
	}

	if (hippi_devp->bp_jobs[job].jobFlags & JOBFLAG_EXCL) {
		error = EBUSY;
		goto dontopen;
	}

	if ((flag & FEXCL) && hippi_devp->bp_jobs[job].jobFlags) {
		error = EBUSY;
		goto dontopen;
	}

	hippi_devp->hippibp_state |= HIPPIBP_ST_OPENED;
	hippi_devp->bp_jobs[job].jobFlags = JOBFLAG_OPEN;
	if (flag & FEXCL)
		hippi_devp->bp_jobs[job].jobFlags |= JOBFLAG_EXCL;

	*devp = makedevice(emaj,MINOR_OF_JOB(m,job));

	dprintf(1,( "hippiopen(BP %d:%d): flag = 0x%x\n",
		unit, job, flag ));

dontopen:
	mutex_unlock( &hippi_devp->devmutex );
	return ( error );
}


int
hippimap(dev_t dev, vhandl_t *vt, off_t off, int len, int prot)
{
	u_int	m = geteminor(dev);
	u_int	unit = UNIT_FROM_MINOR( m );
	hippi_vars_t *hippi_devp = &hippi_device[unit];
	caddr_t kvaddr;
	int status=0, job, npgs=0, vmflags=0;
	volatile __uint32_t *FWmbase;
	struct hippi_freemap *freemap = NULL;
	int olen;

	if (!ISBP_FROM_MINOR( m ))
		return EINVAL;

	job = JOBID_FROM_MINOR( m );

	dprintf(1,("hippimap(%d:%d) uaddr 0x%x len 0x%x \n", unit, job,
		   v_getaddr(vt), len));
	dprintf(2,("hippimap: vhandl 0x%x off 0x%x len 0x%x prot 0x%x\n",
	       vt, off, len,  prot));
	dprintf(2,("hippimap: handle 0x%x addr 0x%x len 0x%x preg 0x%x\n",
	       v_gethandle(vt), v_getaddr(vt), v_getlen(vt)));


	mutex_lock( &hippi_devp->devmutex, PZERO );
	if (!HIPPIBP_UP(hippi_devp))
		goto return_sema;

	olen = len;

	if ((off == HIPPIBP_SFREEMAP_OFF) || (off == HIPPIBP_DFREEMAP_OFF) ||
	    (off == HIPPIBP_PORTIDMAP_OFF)   ) {

		/* Src or Dest Freemap */

		int maxpgs, MAXEVERpgs;

		if (off == HIPPIBP_SFREEMAP_OFF) {
			freemap = &hippi_devp->bp_jobs[job].Sfreemap;
			maxpgs = hippi_devp->bp_maxsfmpgs;
			FWmbase = hippi_devp->hi_bp_sfreemap +
					(job * hippi_devp->cached_bp_fw_conf.sfm_size/sizeof(int));
			MAXEVERpgs = HIPPIBP_MAX_SMAP_PGS;
		} else if (off == HIPPIBP_DFREEMAP_OFF) {
			freemap = &hippi_devp->bp_jobs[job].Dfreemap;
			maxpgs = hippi_devp->bp_maxdfmpgs;
			FWmbase = hippi_devp->hi_bp_dfreemap +
					(job * hippi_devp->cached_bp_fw_conf.dfm_size/sizeof(int));
			MAXEVERpgs = HIPPIBP_MAX_DMAP_PGS;
		} else {
			freemap = &hippi_devp->bp_jobs[job].portidPagemap;
			maxpgs = hippi_devp->bp_jobs[job].portmax*
					hippi_devp->bp_jobs[job].ddqpgs;

			MAXEVERpgs = HIPPIBP_MAX_PORTIDS*MAX(2,hippi_devp->bp_jobs[job].ddqpgs);
			/* This map doesn't get copied to controller */
			FWmbase = 0;

			/* If ddqpgs > 1, the the destination descriptor
			 * queue entries are more than one page.  We allocate
			 * the entires portidPagemap contiguously because
			 * it's much easier than performing portmax allocations
			 * of ddqpgs each and then saving the individual
			 * "chunk" address ... though we may have trouble
			 * obtaining the desired number of pages.
			 */
			if (hippi_devp->bp_jobs[job].ddqpgs > 1) {
				if (btoc(len) %hippi_devp->bp_jobs[job].ddqpgs) {
					status = EINVAL;
					goto return_sema;
				}
				vmflags = VM_PHYSCONTIG;
			}
		}
		dprintf(1,("Hippi FW freemap base 0x%x\n", FWmbase));

		if (btoc(len) > maxpgs) {
			status = EINVAL;
			goto return_sema;
		}

		/* First time job is ever opened, allocate page to contain
		 * list of physical page numbers for freemap.
		 *
		 * NOTE: We use MAXEVERpgs which gives the largest limits that
		 * any HIPIOC_SET_BPCFG command could ever set, since we only
		 * perform this allocation once per system boot (we could fix
		 * this to deallocate before new CFG and then use maxpgs).
		 */

		if (freemap->pfns == (int *)0)
			hippibp_freemap_pfn_alloc( freemap, MAXEVERpgs );

		if (freemap->vaddr == NULL) {
			ASSERT(freemap->vaddr == NULL);
			ASSERT(freemap->ID == NULL);

			/* Allocate a chunk of system virtual memory
			 * for freemap. Don't sleep performing this large
			 * allocation since we're holding the hippi device
			 * lock and we may be short on memory due to other
			 * hippi users who need to obtain the lock in order
			 * to release their memory (especially a problem
			 * for kernel virtual space required).
			 */
			kvaddr = (caddr_t)kvpalloc( btoc(len),
						vmflags|VM_NOSLEEP, 0);
			if (kvaddr == NULL) {
				status = ENOMEM;
				goto return_sema;
			}
			freemap->vaddr = kvaddr;
			freemap->pgcnt = btoc(len);

			/* Build list of physical page numbers
			   for Hippi Controller */

			while (len > 0) {
				freemap->pfns[npgs] = kvatopfn(kvaddr);
				if (FWmbase)
					FWmbase[npgs] = freemap->pfns[npgs];
				npgs++;
				len -= NBPP;
				kvaddr += NBPP;
			}
		}

		if (btoc(olen) != freemap->pgcnt) {
			status = EINVAL;
			goto return_sema;
		}

		/* Map this system memory into user virtual address space */

		status = v_mapphys(vt, freemap->vaddr, ctob(freemap->pgcnt));

		if (status) {
			cmn_err(CE_WARN, "hippimap: v_mapphys status 0x%x\n",
				status);
			goto return_sema;
		}

	} else if (off == HIPPIBP_SDESQ_OFF) {	/* SrcDesq */
		dprintf(1,("hippimap(%d:%d) SDesq uaddr 0x%x\n", unit, job,
			v_getaddr(vt)));

		if (len != hippi_devp->cached_bp_fw_conf.sdq_size) {
			status = EINVAL;
			goto return_sema;
		}

		if (hippi_devp->bp_jobs[job].hippibp_SDesqPage == NULL) {
				hippi_devp->bp_jobs[job].hippibp_SDesqPage =
					hippi_devp->hi_bp_sdqueue +
						job * hippi_devp->cached_bp_fw_conf.sdq_size;
		}

		if (status = v_mapphys(vt,
			(void *)hippi_devp->bp_jobs[job].hippibp_SDesqPage, btoc(len)))
				goto return_sema;

	} else if (off == HIPPIBP_DFLIST_OFF) {	/* DesFreelist */

		dprintf(1,("hippimap(%d:%d) DFlist uaddr 0x%x\n", unit, job,
			v_getaddr(vt)));

		if (len != hippi_devp->cached_bp_fw_conf.dfl_size) {
			status =  EINVAL;
			goto return_sema;
		}

		if (status = v_mapphys(vt,
			(void *)hippi_devp->bp_jobs[job].hippibp_DFlistPage,btoc(len)))
				goto return_sema;;

	} else {
		cmn_err(CE_WARN, "hippimap: UNKNOWN offset 0x%x\n", off);
		status =  EINVAL;
		goto return_sema;
	}
	
#if HIPPI_BP_DEBUG && DEBUG
	if (hippibp_debug >= 2)
		idbg_pregpde( v_getpreg(vt) );	/* print pregion map */
#endif /* HIPPI_BP_DEBUG */

return_sema:
	mutex_unlock( &hippi_devp->devmutex );
	return(status);
}

void
hippifreemap_release ( struct hippi_freemap *freemap, int recoverpages )
{
	if (freemap->ID)
		cmn_err(CE_WARN,"hippifreemap_release2: ID still set!\n");

	if (recoverpages && freemap->vaddr) {
		kvpfree(freemap->vaddr, freemap->pgcnt);
	} else if (!recoverpages)
		cmn_err(CE_WARN,"hippifreemap_release: LOSE %d pps at 0x%x\n",
			freemap->pgcnt, freemap->vaddr);

	freemap->vaddr = 0;
	freemap->pgcnt = 0;
}

int
hippiunmap(dev_t dev, vhandl_t *vt)
{
	u_int	m = geteminor(dev);
	u_int	unit = UNIT_FROM_MINOR( m );
	hippi_vars_t *hippi_devp = &hippi_device[unit];
	__psunsigned_t	ID;
	int	job;

	if (!ISBP_FROM_MINOR( m ))
		return EINVAL;

	job = JOBID_FROM_MINOR( m );
	ID = v_gethandle(vt);

	dprintf(2,("hippiunmap: dev 0x%x vhandl 0x%x\n", dev, vt));
	dprintf(2,("hippiunmap: handle 0x%x addr 0x%x len 0x%x \n",
		ID, v_getaddr(vt), v_getlen(vt)));


	if (ID == hippi_devp->bp_jobs[job].Sfreemap.ID) {

		dprintf(1,("hippiunmap(%d:%d): Sfreemap uaddr 0x%x\n",
			unit, job, v_getaddr(vt)));

		hippi_devp->bp_jobs[job].Sfreemap.ID = 0;

	} else if (ID == hippi_devp->bp_jobs[job].Dfreemap.ID) {

		dprintf(1,("hippiunmap(%d:%d): Dfreemap uaddr 0x%x\n",
			unit, job, v_getaddr(vt)));

		hippi_devp->bp_jobs[job].Dfreemap.ID = 0;

	} else if (ID == hippi_devp->bp_jobs[job].portidPagemap.ID) {

		dprintf(1,("hippiunmap(%d:%d): portidPagemap uaddr 0x%x\n",
			unit, job, v_getaddr(vt)));

		hippi_devp->bp_jobs[job].portidPagemap.ID = 0;

	} else if (ID == hippi_devp->bp_jobs[job].SDesqID) {

		dprintf(1,("hippiunmap(%d:%d): SDesq\n", unit, job));

		hippi_devp->bp_jobs[job].SDesqID = 0;

	} else if (ID == hippi_devp->bp_jobs[job].DFlistID) {

		dprintf(1,("hippiunmap(%d:%d): DFlist\n", unit, job));

		hippi_devp->bp_jobs[job].DFlistID = 0;

	}

	return 0; /* success */
}

/* This routine waits until the we're sure that the HIPPI firmware is not
 * actively performing DMA to the page we're trying to unpin.
 */

void
hippibp_teardown_wait(volatile __uint32_t *dma_status,
		      struct hippi_io *hippimap,
		      int	job)
{
	volatile __uint32_t	status;
	__uint32_t		retval;
	int	pgidx;

	while (1) {
		status = *dma_status;

		dprintf(1,("hippibp_teardown_wait(%d): dma_status 0x%x\n",
			   job, status));
		if (!((status >> HIPPIBP_DMA_ACTIVE_SHIFT) &
		    HIPPIBP_DMA_ACTIVE_MASK))
			break;	/* no DMA currently active */

		if (((status >> HIPPIBP_DMA_JOB_SHIFT) &
		    HIPPIBP_DMA_JOB_MASK) != job)
			break;	/* active DMA for other job */

		if (hippimap->mapselect == HIPPIBP_DFM_SEL) {
			if (((status >> HIPPIBP_DMA_CLIENT_SHIFT) &
				HIPPIBP_DMA_CLIENT_MASK) !=
					HIPPIBP_DMA_CLIENT_DFM)
				break;	/* active DMA for other list */
		} else if (hippimap->mapselect == HIPPIBP_SFM_SEL) {
			if (((status >> HIPPIBP_DMA_CLIENT_SHIFT) &
				HIPPIBP_DMA_CLIENT_MASK) !=
					HIPPIBP_DMA_CLIENT_SFM)
				break;	/* active DMA for other list */
		} else if (hippimap->mapselect == HIPPIBP_DFM_AND_SFM_SEL) {
			retval = (status >> HIPPIBP_DMA_CLIENT_SHIFT) &
					HIPPIBP_DMA_CLIENT_MASK;
			if(retval != HIPPIBP_DMA_CLIENT_SFM &&
				retval != HIPPIBP_DMA_CLIENT_DFM)
				break;	/* active DMA for other list */
		} else
			cmn_err(CE_WARN,"hippibp_teardown: unknown map select\n");

		pgidx = (status >> HIPPIBP_DMA_PGX_SHIFT) & HIPPIBP_DMA_PGX_MASK;
		if ((status >> HIPPIBP_DMA_2PG_SHIFT) & HIPPIBP_DMA_2PG_MASK) {
			if ((pgidx < (hippimap->startindex-1)) ||
			    (pgidx > hippimap->endindex))
				break;	/* active DMA for other page in list */
		} else {
			if ((pgidx < hippimap->startindex) ||
			    (pgidx > hippimap->endindex))
				break;	/* active DMA for other page in list */
		}
		  
	}

}

void
hippibp_iomap_teardown( u_int unit, int job, int idx)
{
	hippi_vars_t *hippi_devp = &hippi_device[unit];
	struct hippi_bp_job *bpjob;
	volatile __uint32_t *FWbase;
	int j;

	bpjob = &hippi_devp->bp_jobs[job];

	/* below, the "&" instead of "==", together with if followed by
	 * if (w/o an else) allows SFM_AND_DFM_SEL to work (both code
	 * paths executed)
	*/
	if (bpjob->hipiomap[idx].mapselect & HIPPIBP_DFM_SEL) {	/* dfreemap */
		FWbase = hippi_devp->hi_bp_dfreemap +
			job*(hippi_devp->cached_bp_fw_conf.dfm_size/sizeof(int));
		for (j=bpjob->hipiomap[idx].startindex;
		     j<=bpjob->hipiomap[idx].endindex; j++) {
			bpjob->Dfreemap.pfns[j] =
				FWbase[j] = hippibp_garbage_pfn;
		}
	}

	if (bpjob->hipiomap[idx].mapselect & HIPPIBP_SFM_SEL) { /* sfreemap */
		FWbase = hippi_devp->hi_bp_sfreemap +
			job*(hippi_devp->cached_bp_fw_conf.sfm_size/sizeof(int));
		for (j=bpjob->hipiomap[idx].startindex;
		     j<=bpjob->hipiomap[idx].endindex; j++) {
			bpjob->Sfreemap.pfns[j] =
				FWbase[j] = hippibp_garbage_pfn;
		}
	}

	hippibp_teardown_wait( &hippi_devp->bp_fw_conf->dma_status,
			      &bpjob->hipiomap[idx],
			      job);

	fast_undma( (void *)bpjob->hipiomap[idx].uaddr,
		bpjob->hipiomap[idx].ulen,
		bpjob->hipiomap[idx].uflags | B_READ,
		&bpjob->hipiomap[idx].dmacookie );

	/* now reclaim entry for future use */

	bpjob->hipiomap_cnt--;
	bpjob->hipiomap[idx].mapselect = 0;

	dprintf(1,("hippi_teardown(%d:%d)(%d %d %d): vaddr 0x%x len 0x%x \n",
	      unit,job,idx+1, bpjob->hipiomap[idx].startindex,
	      bpjob->hipiomap[idx].endindex,
	      bpjob->hipiomap[idx].uaddr, bpjob->hipiomap[idx].ulen));
}

void
hippibp_exit_callback_complete( hippi_bp_job_t *bpjob)
{
	ulong_t curpid = 0;
	int i;

	drv_getparm(PPID, &curpid); /* get pid of current process */
	dprintf(1,("hippibp_exit_callback_complete: bpjob 0x%x curpid 0x%x max 0x%x\n",
		bpjob, curpid, bpjob->hippibp_maxproc));
	for (i=0; i<bpjob->hippibp_maxproc; i++)
		if (bpjob->callback_list[i] == curpid)
			break;

	if (i == bpjob->hippibp_maxproc) {
		cmn_err(CE_WARN,"hippibp_exit_callback_complete: UNKNOWN proc\n");
	} else {
		bpjob->callback_list[i] = 0;
	}
}

static void
hippibp_exit(int arg)
{
	int unit, job, i, maxunit=io4hip_cnt;
	hippi_vars_t *hippi_devp;
	hippi_bp_job_t *bpjob;
	ulong_t curpid = 0;

	drv_getparm(PPID, &curpid); /* get pid of current process */
	
	unit = (arg >>16);
	job = arg & 0x0000ffff;

	dprintf(1,("hippibp_exit(%d:%d): curpid %d\n",
		unit, job, curpid));

	if (unit >= maxunit)
		cmn_err(CE_PANIC, "hippibp_exit: bad unit number %d\n", unit);
	hippi_devp = &hippi_device[unit];

	if (job >= hippi_devp->bp_maxjobs)
		cmn_err(CE_PANIC, "hippibp_exit: bad job number %d\n", job);
	bpjob = &hippi_devp->bp_jobs[job];

	/* scan for any ports which has interrupt signals sent to
	 * the exiting process.
	 */

	for (i=0; i<hippi_devp->hippibp_portmax; i++)
		if (hippi_devp->hippibp_portids[i].pid == curpid) {

			dprintf(1,("hippibp_exit: curpid 0x%x port %d\n",
				curpid, i));
			hippi_devp->hippibp_portids[i].pid = 0;
			hippi_devp->hippibp_portids[i].signo = 0;
		}

	if (bpjob->hipiomap_cnt) {
		mutex_lock( &hippi_devp->devmutex, PZERO );
		for (i=0; i<HIPPIBP_MAX_IOSETUP; i++) {
			if ((bpjob->hipiomap[i].mapselect) &&
			    (bpjob->hipiomap[i].pid == curpid)) {
				hippibp_iomap_teardown( unit, job, i );
				dprintf(1,("hippi_exit(%d:%d)(%d %d %d): vaddr 0x%x len 0x%x \n",
					unit,job,i+1, bpjob->hipiomap[i].startindex,
					bpjob->hipiomap[i].endindex,
					bpjob->hipiomap[i].uaddr,
					bpjob->hipiomap[i].ulen));
			}
		}
		mutex_unlock( &hippi_devp->devmutex );
	}
	hippibp_exit_callback_complete( bpjob );
}

/* This procedure keeps track of which procs already have pending exit
 * callbacks so we don't activate duplicate callbacks.
 * We allow at most HIPPIBP_MAX_PROCS to have pending callbacks on this
 * job.  Used to deactivate interrupts on ports whose interrupts signals
 * were enabled, as well as to allow us to tear-down pending pinned
 * pages from HIPIOC_SETUP_BPIO calls.
 * Uses following data in the job structure:
 * 	bpjob->callback_list[HIPPIBP_MAX_PROCS];
 * 	bpjob->hippibp_maxproc;
 */
int
hippibp_add_exit_callback( hippi_bp_job_t *bpjob, int unit, int job )
{
	int status = 0, hole=HIPPIBP_MAX_PROCS, i;
	ulong_t curpid = 0;

	drv_getparm(PPID, &curpid); /* get pid of current process */

	/* First we check if we already have an exit_callback pending */

	dprintf(1,("hippibp_add_exit_callback(%d:%d): curpid %d\n",
		unit, job, curpid));

	for (i=0; i<bpjob->hippibp_maxproc; i++) {
		if (bpjob->callback_list[i] == curpid)
			return 0;
		if ((hole == HIPPIBP_MAX_PROCS) &&
			(bpjob->callback_list[i] == 0))
			hole = i;
	}

	/* If not, make sure we have room in the exit_callback table */

	if (hole == HIPPIBP_MAX_PROCS)
		hole = bpjob->hippibp_maxproc;

	if (hole >= HIPPIBP_MAX_PROCS) {
		cmn_err(CE_WARN,"hippibp_add_exit_callback: too many procs!\n");
		return (ENOMEM);
	}

	if ((unit >= 16) || (job >= 16384))
		cmn_err(CE_PANIC,"hippibp_add_exit_callback: unit %d job %d\n",
			unit, job);

	status = add_exit_callback(curpid, 0, (void(*)(void *))hippibp_exit,
		(void *)((__psunsigned_t)(unit << 16) | job));

	if (status)
		cmn_err(CE_WARN,"hippibp: add_exit err %d\n", status);
	else {
		bpjob->callback_list[hole] = curpid;
		bpjob->hippibp_maxproc = MAX( hole+1, bpjob->hippibp_maxproc);
		dprintf(1,("hippibp_add_exit_callback: hole %d maxproc %d\n",
			hole, bpjob->hippibp_maxproc));
	}

	return(status);
}


/*ARGSUSED*/
int
hippibpioctl(dev_t dev, int cmd, long arg, int mode, cred_t *cred_p, int *rvalp)
{
	int	error = 0;
	u_int	m = geteminor(dev);
	u_int	unit = UNIT_FROM_MINOR( m );
	hippi_vars_t *hippi_devp = &hippi_device[unit];
	struct hippi_bp_job *bpjob;
	int job;


	mutex_lock( &hippi_devp->devmutex, PZERO );
	if (!HIPPIBP_UP(hippi_devp)) {
		mutex_unlock( &hippi_devp->devmutex );
		return( ENODEV );
	}

	job = JOBID_FROM_MINOR( m );
	bpjob = &hippi_devp->bp_jobs[job];

	dprintf(2,("hippiioctl(BP %d:%d): cmd 0x%x\n", unit, job,cmd));

	switch (cmd) {

	case HIPIOC_SET_HOSTX: {
		struct hip_set_hostx sifields;
		int i;
		volatile __uint32_t *FWbase;

		if (copyin((caddr_t)arg, (caddr_t)&sifields,
			sizeof(struct hip_set_hostx)) < 0) {
			error = EFAULT;
			break;
		}

		dprintf(1,("HIPIOC_SIFIELDS: addr 0x%x nbr 0x%x\n",
			sifields.addr, sifields.nbr_ifields));
			       
		if (sifields.nbr_ifields >
		    (hippi_devp->cached_bp_fw_conf.hostx_size/sizeof(int))) {
			error =  EINVAL;
			break;
		}

		if ((bpjob->ifields == 0) &&
			((bpjob->ifields = (__uint32_t *)
				kmem_zalloc(hippi_devp->cached_bp_fw_conf.hostx_size,
					    KM_NOSLEEP)) == NULL)) {
				error = ENOMEM;
				break;
		}

		if (copyin((void *)sifields.addr, bpjob->ifields,
			sifields.nbr_ifields*sizeof(int)) < 0)
		{
			error =  EFAULT;
			break;
		}

		bpjob->ifieldcnt = sifields.nbr_ifields;
		FWbase = (__uint32_t *)((char *)hippi_devp->hi_bp_ifield +
			job*hippi_devp->cached_bp_fw_conf.hostx_size);

		dprintf(1,("hippiioctl: ifield array 0x%x, FW 0x%x\n",
			bpjob->ifields, FWbase));

		for (i=0; i < sifields.nbr_ifields; i++)
			FWbase[i] = bpjob->ifields[i];
		break;
		}

	case HIPIOC_GET_HOSTX: {
		struct hip_get_hostx lifields;
		int ifieldcnt;

		if (copyin((caddr_t)arg, (caddr_t)&lifields,
			sizeof(struct hip_get_hostx)) < 0) {
			error = EFAULT;
			break;
		}
		dprintf(1,("HIPIOC_LIFIELDS: addr 0x%x 0x%x max 0x%x\n",
			lifields.addr, lifields.nbr_ifields_ptr,
			lifields.max_ifields ));
			       
		/* Don't return more than was asked for */

		ifieldcnt = bpjob->ifieldcnt;
		if (ifieldcnt > lifields.max_ifields)
			ifieldcnt = lifields.max_ifields;

		if (copyout(bpjob->ifields,
			(void *)lifields.addr,
			ifieldcnt*sizeof(int)) < 0) {
			error = EFAULT;
			break;
		}

		if (copyout(&ifieldcnt,
			(void*)lifields.nbr_ifields_ptr,
			sizeof(int)) < 0) {
			error =  EFAULT;
		}

		break;
		}

	case HIPIOC_SET_JOB: {
		struct hip_set_job setjob;

		if (copyin((caddr_t)arg, (caddr_t)&setjob,
			sizeof(struct hip_set_job)) < 0) {
			error = EFAULT;
			break;
		}

		/* Invalid request if user asks for too many portids OR if
		 * the descriptor size is not a multiple of 8 bytes.
		 */
		{
		int missingFWports;

		/* Firmware might not support as many ports as driver */

		missingFWports = hippi_devp->bp_maxportids -
			hippi_devp->cached_bp_fw_conf.num_ports;
		if (missingFWports < 0)
			missingFWports = 0;

		if (setjob.max_ports >
		    (hippi_devp->hippibp_portfree - missingFWports)) {
			error = EINVAL;
			break;
		      }
		}
		/* reserve ports from controller's pool, then
		 * place count of reserved ports into job structure.
		 */
		hippi_devp->hippibp_portfree -= setjob.max_ports;
		bpjob->portmax = setjob.max_ports;

		bpjob->authno[0] = setjob.auth[0];
		bpjob->authno[1] = setjob.auth[1];
		bpjob->authno[2] = setjob.auth[2];
		bpjob->ddqpgs = MIN(btoc(setjob.ddq_size),
				    hippi_devp->bp_maxddqpgs);
		dprintf(1,("setjob(%d:%d): maxport %d auth 0x%x 0x%x 0x%x\n",
			unit, job, bpjob->portmax, bpjob->authno[0],
			bpjob->authno[1], bpjob->authno[2]));
		dprintf(1,("setjob(%d:%d): ddqpgs 0x%x\n",
			unit, job, bpjob->ddqpgs));
		break;
		}

	case HIPIOC_ENABLE_PORT: {
		struct hip_enable_port enable_port;
		__psunsigned_t physaddr;
		int i;

		if (copyin((caddr_t)arg, (caddr_t)&enable_port,
			sizeof(struct hip_enable_port)) < 0) {
			error =  EFAULT;
			break;
		}
		dprintf(2,("getport(%d:%d): pgidx 0x%x retptr 0x%x\n",
		      unit,job,enable_port.pgidx,enable_port.portid_ptr));

		if (bpjob->portused >= bpjob->portmax) {
			error =  EINVAL;
			break;
		}

		if (copyin((void *)enable_port.portid_ptr, &i,
				sizeof(int)) < 0) {
			error = EFAULT;
			break;
		}

		if (i != HIPPIBP_PRIVATE_PORTNUM) {
			if (i < 0 || i > hippi_devp->hippibp_portmax) {
				error = EINVAL;
				break;
			}
			if (hippi_devp->hippibp_portids[i].jobid !=
			    (char) -1) {
				error = EBUSY;
				break;
			}
			hippi_devp->hippibp_portids[i].jobid = job;
			hippi_devp->hippibp_portids[i].pid = 0;
			hippi_devp->hippibp_portids[i].signo = 0;
			bpjob->portused++;
		} else {
			for (i=0; i<hippi_devp->hippibp_portmax; i++) {
				if (hippi_devp->hippibp_portids[i].jobid !=
				    (char) -1)
					continue;
				hippi_devp->hippibp_portids[i].jobid = job;
				hippi_devp->hippibp_portids[i].pid = 0;
				hippi_devp->hippibp_portids[i].signo = 0;
				bpjob->portused++;
				break;
			}
		}

		if (i >= hippi_devp->hippibp_portmax) {
			cmn_err(CE_WARN,"ERROR- out of ports\n");
			error = EINVAL;
		} else if (copyout(&i, (void*)enable_port.portid_ptr,
				   sizeof(int))<0)
			error =  EFAULT;
		else if ((enable_port.pgidx > bpjob->portidPagemap.pgcnt) ||
			 (enable_port.pgidx % bpjob->ddqpgs) ||
			 (bpjob->portidPagemap.vaddr == NULL))
			error = EINVAL;

		if (error) {
			hippi_devp->hippibp_portids[i].jobid = (char)-1;
			break;
		}

		physaddr = (__psunsigned_t)ctob(
				bpjob->portidPagemap.pfns[enable_port.pgidx]);

		dprintf(1,("getport(%d:%d): port %d pgid %d phys 0x%x\n",
			   unit,job,i, enable_port.pgidx, physaddr));
		mutex_lock( &hippi_devp->devslock, PZERO );
		hippi_wait( hippi_devp );

		/* OPCODE must be written as word, so kludge it
		hippi_devp->hi_hc->arg.bp_port.ux.s.opcode =
			HIP_BP_PORT_PGX;
		*/
		hippi_devp->hi_hc->arg.bp_port.ux.i = HIP_BP_PORT_NOPGX << 28;
		hippi_devp->hi_hc->arg.bp_port.job    = job;
		hippi_devp->hi_hc->arg.bp_port.port    = i;
		hippi_devp->hi_hc->arg.bp_port.ddq_hi = physaddr>>32;
		hippi_devp->hi_hc->arg.bp_port.ddq_lo = (int)physaddr;
		hippi_devp->hi_hc->arg.bp_port.ddq_size = bpjob->ddqpgs * NBPP;

		hiphw_op( hippi_devp, HCMD_BP_PORT);
		mutex_unlock( &hippi_devp->devslock );

		break;
		}

	case HIPIOC_ENABLE_JOB: {
		struct hip_enable_job enablejob;

		if (copyin((caddr_t)arg, (caddr_t)&enablejob,
			sizeof(struct hip_enable_job)) < 0) {
			error = EFAULT;
			break;
		}
		dprintf(1,("enableJob(%d:%d): enabled\n",unit,job));
		mutex_lock( &hippi_devp->devslock, PZERO );
		hippi_wait( hippi_devp );

		hippi_devp->hi_hc->arg.bp_job.enable = 1;
		hippi_devp->hi_hc->arg.bp_job.job = job;

		hippi_devp->hi_hc->arg.bp_job.fm_entry_size = NBPP;
		hippi_devp->hi_hc->arg.bp_job.ack_host =
			bpjob->ack_host = enablejob.ack_host;
		hippi_devp->hi_hc->arg.bp_job.ack_port =
			bpjob->ack_port = enablejob.ack_port;
		hippi_devp->hi_hc->arg.bp_job.auth[0] = bpjob->authno[0];
		hippi_devp->hi_hc->arg.bp_job.auth[1] = bpjob->authno[1];
		hippi_devp->hi_hc->arg.bp_job.auth[2] = bpjob->authno[2];


		hiphw_op( hippi_devp, HCMD_BP_JOB);
		mutex_unlock( &hippi_devp->devslock  );

		break;
		}
	case HIPIOC_GET_BPCFG: {
		struct hip_bp_config bp_cfg;

	  	bp_cfg.ulp = hippi_devp->bp_ulp;
		bp_cfg.max_jobs = hippi_devp->bp_maxjobs;
		bp_cfg.max_portids = hippi_devp->bp_maxportids;
		bp_cfg.max_dfm_pgs = hippi_devp->bp_maxdfmpgs;
		bp_cfg.max_sfm_pgs = hippi_devp->bp_maxsfmpgs;
		bp_cfg.max_ddq_pgs = hippi_devp->bp_maxddqpgs;

		if (copyout((caddr_t)&bp_cfg, (caddr_t)arg,
			sizeof(struct hip_bp_config)) < 0) {
			error =  EFAULT;
			break;
		}

		break;
		}

	case HIPIOC_ENABLE_INTR: {
		struct hip_enable_intr hip_intr;
		ulong_t curpid = 0;

		drv_getparm(PPID, &curpid); /* get pid of current process */

		if (copyin((caddr_t)arg, (caddr_t)&hip_intr,
			sizeof(struct hip_enable_intr)) < 0) {
			error =  EFAULT;
			break;
		}

		if ((hip_intr.portid > HIPPIBP_MAX_PORTIDS) ||
		    (hippi_devp->hippibp_portids[hip_intr.portid].jobid != job)){
			error = EINVAL;
			break;
		}
		if (hip_intr.enable) {
			hippi_devp->hippibp_portids[hip_intr.portid].signo =
				hip_intr.signal_no;
			hippi_devp->hippibp_portids[hip_intr.portid].pid =
				curpid;
		} else if (hippi_devp->hippibp_portids[hip_intr.portid].pid ==
				curpid)  {
			hippi_devp->hippibp_portids[hip_intr.portid].signo = 0;
			hippi_devp->hippibp_portids[hip_intr.portid].pid = 0;
		} else
			error = EINVAL;

		if (!error) {
			error = hippibp_add_exit_callback(bpjob, unit, job);

			if (error)
				cmn_err(CE_WARN,"hippibp: add_exit err %d\n",
					error);
		}

		dprintf(1,("enable_intr(%d:%d): portid 0x%x signo %d curpid %d\n",
		      unit,job,hip_intr.portid, hip_intr.signal_no,
		      curpid));
		break;
		}
	case HIPIOC_GET_SDESQHEAD: {
		int	retval;

		retval = *(volatile __uint32_t*)(hippi_devp->hi_bp_sdhead +
			job*hippi_devp->cached_bp_fw_conf.bpjob_size);

		dprintf(1,("get_sdhead(%d:%d): return val 0x%x\n",
		      unit,job,retval));

		if (copyout(&retval, (caddr_t)arg, sizeof(int)) <0)
			error = EFAULT;
		break;
		}

	case HIPIOC_GET_FWADDR:
		{
		struct hip_get_fwaddr hip_fwaddr;
		caddr_t baseaddr, FWsize;

		if (!_CAP_ABLE(CAP_DEVICE_MGT)) {
			error = EPERM;
			break;
		}
		if (copyin((caddr_t)arg, (caddr_t)&hip_fwaddr,
			sizeof(struct hip_get_fwaddr)) < 0) {
			error =  EFAULT;
			break;
		}

		error = 0;

		switch (hip_fwaddr.addrType) {
		case 1:
			baseaddr = (caddr_t)(hippi_devp->hi_bp_sfreemap
				+ job*(hippi_devp->cached_bp_fw_conf.sfm_size/sizeof(int)));
			FWsize = (caddr_t)((__psunsigned_t)hippi_devp->cached_bp_fw_conf.sfm_size);
			break;
		case 2:
			baseaddr = (caddr_t)(hippi_devp->hi_bp_dfreemap
				+ job*(hippi_devp->cached_bp_fw_conf.dfm_size/sizeof(int)));
			FWsize = (caddr_t)((__psunsigned_t)hippi_devp->cached_bp_fw_conf.dfm_size);
			break;
		case 3:
			baseaddr = (caddr_t)(hippi_devp->hi_bp_sdqueue
				+ job*hippi_devp->cached_bp_fw_conf.sdq_size);
			FWsize = (caddr_t)((__psunsigned_t)hippi_devp->cached_bp_fw_conf.sdq_size);
			break;
		case 4:
			baseaddr = (caddr_t)(hippi_devp->hi_bp_dfreelist
				+ job * hippi_devp->cached_bp_fw_conf.dfl_size);
			FWsize = (caddr_t)((__psunsigned_t)hippi_devp->cached_bp_fw_conf.dfl_size);
			break;
		default:
			error = EINVAL;
		}
		if ((error == 0) && (
			(copyout(&baseaddr, (caddr_t)hip_fwaddr.FWaddrPtr,
				sizeof(hip_fwaddr.FWaddrPtr)) <0) ||
			(copyout(&FWsize, (caddr_t)hip_fwaddr.FWaddrSize,
				sizeof(hip_fwaddr.FWaddrSize)) <0) ))
				error = EFAULT;
		break;
		}
	case HIPIOC_TEARDOWN_BPIO: {
		struct hip_io_teardown hip_teardown;
		int i;
		ulong_t curpid = 0;

		drv_getparm(PPID, &curpid); /* get pid of current process */

		if ((bpjob->jobFlags & JOBFLAG_EXCL) == 0) {
			error =  EPERM;
			break;
		}

		if (copyin((caddr_t)arg, (caddr_t)&hip_teardown,
			sizeof(struct hip_io_teardown)) < 0) {
			error =  EFAULT;
			break;
		}
		i = hip_teardown.handle - 1;
		if ((i < 0) || (bpjob->hipiomap == 0) ||
		    (bpjob->hipiomap[i].mapselect==0) ||
		    (curpid != bpjob->hipiomap[i].pid)) {
			error = EINVAL;
			break;
		}
		hippibp_iomap_teardown( unit, job, i );

		dprintf(1,("hippi_teardown(%d:%d)(%d %d %d): vaddr 0x%x len 0x%x \n",
		      unit,job,i+1, bpjob->hipiomap[i].startindex,
		      bpjob->hipiomap[i].endindex,
		      bpjob->hipiomap[i].uaddr, bpjob->hipiomap[i].ulen));
		break;
		}
	case HIPIOC_SETUP_BPIO: {
		struct hip_io_setup hip_setup;
		int i, j, k, cookie;
		volatile __uint32_t *FWbase;
		int	sfm_selected = 0, dfm_selected = 0;
		ulong_t curpid = 0;

		drv_getparm(PPID, &curpid); /* get pid of current process */

		if ((bpjob->jobFlags & JOBFLAG_EXCL) == 0) {
			error =  EPERM;
			break;
		}

		if (copyin((caddr_t)arg, (caddr_t)&hip_setup,
			sizeof(struct hip_io_setup)) < 0) {
			error =  EFAULT;
			break;
		}
		dprintf(1,("hippi_setup_bpio(%d:%d) uaddr 0x%x len 0x%x \n",
		      unit,job,hip_setup.uaddr,hip_setup.ulen));

		if (bpjob->hipiomap == 0) {
			bpjob->hipiomap =
				kmem_zalloc(HIPPIBP_MAX_IOSETUP *
					sizeof(struct hippi_io), KM_NOSLEEP);
			bpjob->hipio_temppfn =
				kmem_zalloc(MAX(HIPPIBP_MAX_SMAP_PGS,
						HIPPIBP_MAX_DMAP_PGS) *
					sizeof(int), KM_NOSLEEP);
		}
		for (i=0; i<HIPPIBP_MAX_IOSETUP; i++) {
			if (bpjob->hipiomap[i].mapselect == 0)
				break;
		}
		if (i >= HIPPIBP_MAX_IOSETUP) {
			error =  ENOSPC;
			break;
		}
		dprintf(1,("hippi_setup_bpio(%d:%d) fast_userdma called \n",
		      unit,job));

		bpjob->hipiomap[i].startindex = hip_setup.start_index;
		bpjob->hipiomap[i].endindex = hip_setup.start_index +
			btoc(hip_setup.uaddr+hip_setup.ulen-1) -
				btoct(hip_setup.uaddr) - 1;
		bpjob->hipiomap[i].uaddr = hip_setup.uaddr;
		bpjob->hipiomap[i].ulen = hip_setup.ulen;
		bpjob->hipiomap[i].uflags = hip_setup.uflags;
		bpjob->hipiomap[i].pid = curpid;

		if (hip_setup.mapselect == BPIO_DFM_SEL) { /* dfreemap */
			if ((bpjob->hipiomap[i].endindex >=
			     hippi_devp->bp_maxdfmpgs) ||
			    (bpjob->hipiomap[i].startindex <
			     bpjob->Dfreemap.pgcnt)){
				dprintf(1,("hippi_setup_bpio(%d:%d) DFM EINVAL\n", unit,job));
				error = EINVAL;
				break;
			}  
			dfm_selected = 1;
		} else if (hip_setup.mapselect == BPIO_SFM_SEL) { /* sfreemap */
			if ((bpjob->hipiomap[i].endindex >=
			     hippi_devp->bp_maxsfmpgs) ||
			    (bpjob->hipiomap[i].startindex <
			     bpjob->Sfreemap.pgcnt)) {
				dprintf(1,("hippi_setup_bpio(%d:%d) SFM EINVAL\n", unit,job));
				error = EINVAL;
				break;
			}
			sfm_selected = 1;
		} else if (hip_setup.mapselect == BPIO_SFM_AND_DFM_SEL) { 
						/* SFM and DFM */
			if ((bpjob->hipiomap[i].endindex >=
			     hippi_devp->bp_maxsfmpgs) ||
			    (bpjob->hipiomap[i].endindex >=
			     hippi_devp->bp_maxdfmpgs) ||
			    (bpjob->hipiomap[i].startindex <
			     bpjob->Sfreemap.pgcnt)    ||
			    (bpjob->hipiomap[i].startindex <
			     bpjob->Dfreemap.pgcnt)) {
				dprintf(1,("hippi_setup_bpio(%d:%d) SFM and DFM EINVAL\n", 
					unit,job));
				error = EINVAL;
				break;
			}
			dfm_selected = 1;
			sfm_selected = 1;
		} else {  /* unknown mapselect */
			dprintf(1,("hippi_setup_bpio(%d:%d) unknown mapselect %d \n", 
				unit, job, hip_setup.mapselect));
			error = EINVAL;
			break;
		}

		/* If uaddr is NOT a KUSEG, then fast_userdma does not perform
		 * a setup and does not initialize the cookie!  Then we get
		 * into trouble if we do a fast_undma().
		 */
		if (!IS_KUSEG(hip_setup.uaddr)) {
			error = EINVAL;
			break;
		}
		/* NOTE: We "or" in the B_READ flag in order to work-around
		 * the problem of user pages which may be COW.  If we setup
		 * the current page, then the user COW-s it, our virtual
		 * map no longer contains the same page we setup.  The
		 * unsetup wil unlock the wrong page and leave the other
		 * page locked.  Setting B_READ causes fast_userdma to
		 * pre-COW the page by "subyte" on each page.
		 */
		 
		if (fast_userdma( (void *)hip_setup.uaddr, hip_setup.ulen,
				  hip_setup.uflags | B_READ,
				  &cookie )) {
			error = EFAULT;
			break;
		}
		dprintf(1,("hippi_setup_bpio(%d:%d) fast_userdma OK (0x%x)\n",
		      unit,job,cookie));
		bpjob->hipiomap[i].dmacookie = cookie;

		if (!curproc_vtop((void *)hip_setup.uaddr, hip_setup.ulen,
			&bpjob->hipio_temppfn[0], sizeof(int)))  {

			fast_undma( (void *)bpjob->hipiomap[i].uaddr,
				bpjob->hipiomap[i].ulen,
				bpjob->hipiomap[i].uflags|B_READ,
				&bpjob->hipiomap[i].dmacookie );

			dprintf(1,("hippi_setup_bpio(%d:%d) vtop() failed\n",
			      unit,job));
			error = EFAULT;
			break;
		}

		dprintf(1,("hippi_setup_bpio(%d:%d) vtop() OK\n", unit,job));

		if (dfm_selected) { /* dfreemap */
			bpjob->hipiomap[i].mapselect |= HIPPIBP_DFM_SEL;
			FWbase = hippi_devp->hi_bp_dfreemap + job *
				(hippi_devp->cached_bp_fw_conf.dfm_size /
					sizeof(int));

			if (bpjob->Dfreemap.pfns == 0)
				hippibp_freemap_pfn_alloc( &bpjob->Dfreemap,
					HIPPIBP_MAX_DMAP_PGS );

			for (j=bpjob->hipiomap[i].startindex,k=0;
			     j<=bpjob->hipiomap[i].endindex; j++,k++) {
				bpjob->Dfreemap.pfns[j] =
				  bpjob->hipio_temppfn[k];
				FWbase[j] = 
				  bpjob->hipio_temppfn[k];
				dprintf(1,("hippi_iosetup(%d:%d) DFM 0x%x pfn 0x%x\n",
				      unit,job, j, bpjob->hipio_temppfn[k]));
			}
		} 

		if (sfm_selected) { /* sfreemap */
			bpjob->hipiomap[i].mapselect |= HIPPIBP_SFM_SEL;
			FWbase = hippi_devp->hi_bp_sfreemap + job *
				(hippi_devp->cached_bp_fw_conf.sfm_size /
					sizeof(int));

			if (bpjob->Sfreemap.pfns == 0)
				hippibp_freemap_pfn_alloc( &bpjob->Sfreemap,
					HIPPIBP_MAX_SMAP_PGS );

			for (j=bpjob->hipiomap[i].startindex,k=0;
			     j<=bpjob->hipiomap[i].endindex; j++,k++) {
				bpjob->Sfreemap.pfns[j] =
				  bpjob->hipio_temppfn[k];
				FWbase[j] = 
				  bpjob->hipio_temppfn[k];
				dprintf(1,("hippi_iosetup(%d:%d) SFM 0x%x pfn 0x%x\n",
				      unit,job, j, bpjob->hipio_temppfn[k]));
			}
		} 

		error = hippibp_add_exit_callback(bpjob, unit, job);

		if (error)
			cmn_err(CE_WARN,"hippibp: add_exit err %d\n", error);

		bpjob->hipiomap_cnt++;
		i++;	/* don't allow a zero handle, so offset by 1 */
		if (copyout(&i, (caddr_t)hip_setup.handle_ptr, sizeof(int)) <0)
			error = EFAULT;

		dprintf(1,("hippi_iosetup(%d:%d)(%d %d %d): vaddr 0x%x len 0x%x \n",
		      unit,job,i, bpjob->hipiomap[i-1].startindex,
		      bpjob->hipiomap[i-1].endindex,
		      hip_setup.uaddr, hip_setup.ulen));
		break;
		}
	case HIPIOC_BP_PROT_INFO: {
		hippibp_prot_info_t bppi;

		bppi.format	   = HIPPIBP_PROTINFO_FORMAT_CURR;
		bppi.bp_major_vers = HIPPIBP_PROTOCOL_MAJOR;
		bppi.bp_minor_vers = HIPPIBP_PROTOCOL_MINOR;

		if (copyout((caddr_t)&bppi, (caddr_t)arg,
			    sizeof(hippibp_prot_info_t)) < 0) {
			error =  EFAULT;
			break;
		}

		break;
		}

	default:
		printf("hippiioctl: UNKNOWN not implemented\n");
		error = EINVAL;
		break;
	}

	mutex_unlock( &hippi_devp->devmutex );
	return error;
}


int
hippibpclose( dev_t dev )
{
	u_int	m = geteminor(dev);
	u_int	unit = UNIT_FROM_MINOR( m );
	hippi_vars_t *hippi_devp;
	int i, job, recoverpgs=1;
	struct hippi_bp_job *bpjob;
	__uint32_t	*dma_status, status;

	hippi_devp = & hippi_device[unit];
	job = JOBID_FROM_MINOR(m);
	bpjob = &hippi_devp->bp_jobs[job];

	/* First we issue a "disable job" command to the FW.
	 * This should cause the FW to stop transmitting and receiving
	 * for this job, except for a (single) DMA "in-progress"
	 */

	mutex_lock( &hippi_devp->devmutex, PZERO );
	if (HIPPIBP_UP(hippi_devp)) {
		volatile __uint32_t *FWbase;
		int j;

		mutex_lock( &hippi_devp->devslock, PZERO );

		hippi_wait( hippi_devp );
		hippi_devp->hi_hc->arg.bp_job.enable = 0;
		hippi_devp->hi_hc->arg.bp_job.job    = job;
		hippi_devp->hi_hc->arg.bp_job.fm_entry_size = NBPP;
		hippi_devp->hi_hc->arg.bp_job.ack_host = 0;
		hippi_devp->hi_hc->arg.bp_job.ack_port = 0;
		hippi_devp->hi_hc->arg.bp_job.auth[0] = 0;
		hippi_devp->hi_hc->arg.bp_job.auth[1] = 0;
		hippi_devp->hi_hc->arg.bp_job.auth[2] = 0;
		hiphw_op( hippi_devp, HCMD_BP_JOB);
		mutex_unlock( &hippi_devp->devslock  );
		dprintf(1,("hippiclose(BP %d:%d) job disable sent to FW\n",unit,job));

		/* Make sure all FW accessible pages point to the
		 * "garbage page".  First we handle the pages setup by
		 * calls to hippimap().
		 */

		FWbase = hippi_devp->hi_bp_dfreemap + job *
			(hippi_devp->cached_bp_fw_conf.dfm_size/sizeof(int));

		for (i=0; i < bpjob->Dfreemap.pgcnt; i++)
			FWbase[i] = bpjob->Dfreemap.pfns[i] =
				hippibp_garbage_pfn;

		FWbase = hippi_devp->hi_bp_sfreemap + job *
			(hippi_devp->cached_bp_fw_conf.sfm_size/sizeof(int));

		for (i=0; i < bpjob->Sfreemap.pgcnt; i++)
			FWbase[i] = bpjob->Sfreemap.pfns[i] =
				hippibp_garbage_pfn;

		/* Now we handle cleanup of map entries which were setup
		 * by HIPIOC_SETUP_BPIO and which were not torn down by
		 * calls to HIPIOC_TEARDOWN_BPIO.
		 */

		if (bpjob->hipiomap_cnt)
		  for (i=0; i<HIPPIBP_MAX_IOSETUP; i++)  {
		    if (bpjob->hipiomap[i].mapselect & HIPPIBP_DFM_SEL) {

			/* dfreemap */

			FWbase = hippi_devp->hi_bp_dfreemap + job *
				(hippi_devp->cached_bp_fw_conf.dfm_size/sizeof(int));

			for (j=bpjob->hipiomap[i].startindex;
				j<=bpjob->hipiomap[i].endindex; j++)

				bpjob->Dfreemap.pfns[j] =
					FWbase[j] = hippibp_garbage_pfn;
			
		    }

		    if (bpjob->hipiomap[i].mapselect & HIPPIBP_SFM_SEL) {
	
			/* sfreemap */
	
			FWbase = hippi_devp->hi_bp_sfreemap + job *
				(hippi_devp->cached_bp_fw_conf.sfm_size/sizeof(int));

			for (j=bpjob->hipiomap[i].startindex;
				j<=bpjob->hipiomap[i].endindex; j++)

				bpjob->Sfreemap.pfns[j] =
					FWbase[j] = hippibp_garbage_pfn;
		    }  
		  }  /* for each of HIPPIBP_MAX_IOSETUP entries */
	}  else 
		goto recover_iomaps;

	for (i=0; i<hippi_devp->hippibp_portmax; i++) {
		if (hippi_devp->hippibp_portids[i].jobid == job) {

			/* Tell Hippi FW that this port is inactive */
		  
			mutex_lock( &hippi_devp->devslock, PZERO );
			hippi_wait( hippi_devp );

			/* OPCODE must be written as word, so kludge it
			   hippi_devp->hi_hc->arg.bp_port.ux.s.opcode =
				HIP_BP_PORT_DISABLE;
			*/
			hippi_devp->hi_hc->arg.bp_port.ux.i =
				HIP_BP_PORT_DISABLE << 28;
			hippi_devp->hi_hc->arg.bp_port.job    = job;
			hippi_devp->hi_hc->arg.bp_port.port    = i;
			hippi_devp->hi_hc->arg.bp_port.ddq_hi = 0;
			hippi_devp->hi_hc->arg.bp_port.ddq_lo = 0;

			hippi_devp->hi_hc->res.cmd_res[0] = 0xdeadbeef;

			hiphw_op( hippi_devp, HCMD_BP_PORT);
			hippi_wait( hippi_devp );

			/* Check the return code to see if port is free
			 * or if the FW has a DMA "in-progress".
			 */
			
			dprintf(1,("hippiclose(%d:%d): disable port %d\n",
				unit,job,i));

			hippi_devp->hippibp_portids[i].jobid = (char)-1;
			hippi_devp->bp_jobs[job].portused--;

			mutex_unlock( &hippi_devp->devslock  );

		}
	}

	/* wait for all DMA to finish for this job */

	dma_status = (__uint32_t *)&hippi_devp->bp_fw_conf->dma_status;
	while (1) {
		status = *dma_status;
		if (!((status >> HIPPIBP_DMA_ACTIVE_SHIFT) &
			HIPPIBP_DMA_ACTIVE_MASK))
			break;	/* no DMA currently active */

		if (((status >> HIPPIBP_DMA_JOB_SHIFT) &
			HIPPIBP_DMA_JOB_MASK) != job)
			break;	/* active DMA for other job */

		dprintf(1,("hippiclose(%d:%d): DMA ACTIVE (0x%x)\n",
				unit, job, status));
	}

	if (bpjob->portused) {
		cmn_err(CE_WARN,"hippiclose: USING PORTIDS\n");
		recoverpgs=0;
	} else {
		hippi_devp->hippibp_portfree +=
			bpjob->portmax;
		bpjob->portmax = 0;
	}

recover_iomaps:

	mutex_lock( &hippi_devp->devslock, PZERO );
	hippi_wait( hippi_devp );
	mutex_unlock( &hippi_devp->devslock  );

	/* Job has been disabled, all of the freemaps contain garbage page
	 * pointers, and there is no active DMA for this job.
	 *			 OR
	 * controller has been reset, so no DMA is possible.
	 *
	 * It is now safe to recover all of the pages used by the job.
	 */

	if (bpjob->hipiomap_cnt) {

		cmn_err(CE_WARN,"hippiclose: setup_bpio still inuse\n");

		for (i=0; i<HIPPIBP_MAX_IOSETUP; i++)
		  if (bpjob->hipiomap[i].mapselect) {

		    fast_undma( (void *)bpjob->hipiomap[i].uaddr,
			       bpjob->hipiomap[i].ulen,
			       bpjob->hipiomap[i].uflags|B_READ,
			       &bpjob->hipiomap[i].dmacookie );
		    
		    bpjob->hipiomap[i].mapselect = 0;
		    bpjob->hipiomap_cnt--;
		  }

		if (bpjob->hipiomap_cnt)
			cmn_err(CE_WARN,"hippibpclose: cant find I/O setup region\n");
		cmn_err(CE_WARN,"hippiclose: hipiomap_cnt %d\n",bpjob->hipiomap_cnt);
	}

	if (bpjob->Sfreemap.ID || bpjob->Dfreemap.ID ||
	    bpjob->portidPagemap.ID || bpjob->SDesqID || bpjob->DFlistID) {
	
		recoverpgs=0;

		cmn_err(CE_WARN, "hippiclose: ENTRY STILL ACTIVE\n");
		dprintf(1,("Sfreemap.ID 0x%x Dfreemap.ID 0x%x\n",
			   bpjob->Sfreemap.ID, bpjob->Dfreemap.ID));
		dprintf(1,("portidPagemap.ID 0x%x SDesqID 0x%x\n",
			   bpjob->portidPagemap.ID, bpjob->SDesqID));
		dprintf(1,("DFlistID 0x%x\n", bpjob->DFlistID));
	} else
		hippi_devp->bp_jobs[JOBID_FROM_MINOR(m)].jobFlags = 0;

	for (i=0; i<bpjob->hippibp_maxproc; i++)
		if (bpjob->callback_list[i] != 0)
			cmn_err(CE_WARN,"hippibp_close: missing callback for 0x%x\n",
				bpjob->callback_list[i]);

	bpjob->hippibp_maxproc = 0;

	hippifreemap_release( &bpjob->Sfreemap, recoverpgs );

	hippifreemap_release( &bpjob->Dfreemap, recoverpgs );

	hippifreemap_release(&bpjob->portidPagemap, recoverpgs);

	mutex_unlock( &hippi_devp->devmutex );
	dprintf(1,("hippiclose: completed!\n"));
	return 0;
}
#endif /* HIPPI_BP */


/************************************************************************
 *
 * HIPPI character device interface
 *
 *
 ************************************************************************/


int
hippiopen( dev_t *devp, int flag )
{
	int 	cloneIndex, error = 0;
	u_int	emaj = getemajor( *devp );
	u_int	m = geteminor( *devp );
	u_int	unit = UNIT_FROM_MINOR( m );
	hippi_vars_t *hippi_devp;

	/* Check for valid unit number */

#ifdef HIPPI_BP
	if (ISBP_FROM_MINOR(m))
		return( hippibpopen( devp, flag ));
#endif /* HIPPI_BP */

	if ( unit >= io4hip_cnt || ISCLONE_FROM_MINOR(m) )
		return ENODEV;

	hippi_devp = & hippi_device[unit];

	mutex_lock( & hippi_devp->devmutex, PZERO );

	cloneIndex=0;
	while ( cloneIndex < HIPPIFP_MAX_CLONES &&
		hippi_devp->clone[cloneIndex].ulpIndex!=ULPIND_UNUSED)
			cloneIndex++;
	if ( cloneIndex >= HIPPIFP_MAX_CLONES ) {
		error = ENXIO;
		goto dontopen;
	}
	hippi_devp->clone[cloneIndex].ulpIndex = ULPIND_UNBOUND;
	hippi_devp->clone[cloneIndex].mode = flag & (FREAD|FWRITE);
	hippi_devp->clone[cloneIndex].cloneFlags = 0;
	hippi_devp->clone[cloneIndex].src_error = 0;
	hippi_devp->clone[cloneIndex].dst_errors = 0;
	*devp = makedevice(emaj,MINOR_OF_CLONE(m,cloneIndex));
#ifdef HIPPI_DEBUG
	if ( hippi_debug )
		printf( "hippiopen: *devp = 0x%x flag = 0x%x\n",
			(int) *devp, flag );
#endif
	if ( ISPREBOUND_FROM_MINOR( m ) ) {

	   /* Bind this automatically */

	   if ( ISFP_FROM_MINOR( m ) )
		error = hippi_fp_bind( hippi_devp,
			MINOR_OF_CLONE(m,cloneIndex),
			PREBOUND_ULP_FROM_MINOR(m), flag );

	   else
		error = hippi_fp_bind( hippi_devp,
			MINOR_OF_CLONE(m,cloneIndex),
			HIPPI_ULP_PH, flag );
	}

dontopen:
	mutex_unlock( & hippi_devp->devmutex );
	return error;
}



int
hippiclose( dev_t dev )
{
	int	i, ulpIndex;
	u_int	m = geteminor(dev);
	u_int	unit = UNIT_FROM_MINOR( m );
	hippi_vars_t *hippi_devp;
	struct hippi_fp_ulps *ulpp;
	struct hippi_fp_clones *clonep;

	ASSERT( unit < EV_MAX_HIPADAPS );

#ifdef HIPPI_DEBUG
	if ( hippi_debug )
		printf( "hippiclose: dev = 0x%x\n", (int) dev );
#endif

#ifdef HIPPI_BP
	if (ISBP_FROM_MINOR( m ))
		return( hippibpclose( dev ) );
#endif /* HIPPI_BP */

	hippi_devp = & hippi_device[unit];

	mutex_lock( & hippi_devp->devmutex, PZERO );

	ASSERT( ISCLONE_FROM_MINOR( m ) );

	/* ULP */

	clonep = &hippi_devp->clone[ CLONEID_FROM_MINOR(m) ];
	ulpIndex = clonep->ulpIndex;

	ASSERT( ulpIndex != ULPIND_UNUSED );

	if ( ulpIndex != ULPIND_UNBOUND ) {
	    if ( (hippi_devp->hi_state & HI_ST_UP) ) {

		if ( clonep->cloneFlags & CLONEFLAG_W_HOLDING )
			vsema( &hippi_devp->src_sema );

		if ( (clonep->cloneFlags & CLONEFLAG_W_NBOP) ||
				(clonep->cloneFlags & CLONEFLAG_W_NBOC) ) {
			volatile union hip_d2b *d2bp;

			/* XXX: disconnect source/drop packet! */
			d2bp = hippi_devp->hi_d2bp_hd;
			d2bp->hd.chunks = 0;
			d2bp->hd.stk = HIP_STACK_FP;
			d2bp = D2B_NXT( d2bp );
			d2bp->hd.flags = HIP_D2B_BAD;
			hippi_devp->hi_d2bp_hd->hd.fburst = 0;
			hippi_devp->hi_d2bp_hd->hd.flags =
				HIP_D2B_RDY | HIP_D2B_NACK;
			hippi_devp->hi_d2bp_hd = d2bp;
			hippi_wakeup( hippi_devp );
		}
		clonep->cloneFlags = 0;
		clonep->wr_pktOutResid = 0;


		if ( clonep->mode & FREAD ) {

		   ASSERT( ulpIndex <= HIPPIFP_MAX_OPEN_ULPS );

		   ulpp = & hippi_devp->ulp[ ulpIndex ];

		   ASSERT( ulpp->opens > 0 );

		   if ( --ulpp->opens == 0 ) {

			mutex_lock( & hippi_devp->devslock, PZERO );

			hippi_wait( hippi_devp );
			if ( ulpp->ulpId == HIPPI_ULP_PH ) {
			   hippi_devp->hi_hc->arg.cmd_data[0] = HIP_STACK_RAW;
			   hippi_devp->dstLock = 0;
			}
			else {
			   hippi_devp->hi_hc->arg.cmd_data[0] =
				   (ulpp->ulpId<<16)|(ulpIndex+HIP_STACK_FP);
			   hippi_devp->ulpFromId[ ulpp->ulpId ] = 255;
			}

			hiphw_op(hippi_devp,HCMD_DSGN_ULP);
			hippi_wait( hippi_devp );

			mutex_unlock( & hippi_devp->devslock );

			freesema( &ulpp->rd_sema );
			freesema( &ulpp->rd_dmadn );
			phfree( ulpp->rd_pollhdp );
			if ( ulpp->rd_fpd1head )
			   kmem_free( ulpp->rd_fpd1head, HIPPIFP_HEADBUFSIZE );
			ulpp->rd_fpd1head = 0;
			kvpfree( (void *)ulpp->rd_c2b_rdlist, C2B_RDLISTPGS );
			if ( io4ia_war ) {
				kvpfree( (void *)ulpp->io4ia_war_page0, 1 );
				kvpfree( (void *)ulpp->io4ia_war_page1, 1 );
			}

		   }

		}
	    } /* (hippi_devp->hi_state & HI_ST_UP) */
	    else {

		/********************************************************
		 * If board is shut down, we are just cleaning up here. *
		 * Free up poll-head, semaphores, etc., on last close.  *
		 *******************************************************/

		if ( clonep->mode & FREAD ) {

		   ASSERT( ulpIndex <= HIPPIFP_MAX_OPEN_ULPS );

		   ulpp = & hippi_devp->ulp[ ulpIndex ];

		   ASSERT( ulpp->opens > 0 );

		   if ( --ulpp->opens == 0 ) {
			freesema( &ulpp->rd_sema );
			freesema( &ulpp->rd_dmadn );
			phfree( ulpp->rd_pollhdp );
			if ( ulpp->rd_fpd1head )
			   kmem_free( ulpp->rd_fpd1head, HIPPIFP_HEADBUFSIZE );
			ulpp->rd_fpd1head = 0;
			kvpfree( (void *)ulpp->rd_c2b_rdlist, C2B_RDLISTPGS );
			if ( io4ia_war ) {
				kvpfree( (void *)ulpp->io4ia_war_page0, 1 );
				kvpfree( (void *)ulpp->io4ia_war_page1, 1 );
			}
		   }

		}

		clonep->ulpIndex = ULPIND_UNUSED;
		for (i=0; i<HIPPIFP_MAX_CLONES; i++)
			if ( hippi_devp->clone[i].ulpIndex != ULPIND_UNUSED )
				break;
		if ( i >= HIPPIFP_MAX_CLONES ) {

			/* Last close-- clean everything else up */

			freesema( & hippi_devp->rawoutq_sema );
			freesema( & hippi_devp->src_sema );
			for (i=0; i<HIPPIFP_MAX_WRITES; i++)
				freesema( & hippi_devp->rawoutq_sleep[i] );
		}

	    } /* ! (hippi_devp->hi_state & HI_ST_UP) */

	} /* ulpIndex != ULPIND_UNBOUND */
   
	clonep->ulpIndex = ULPIND_UNUSED;
	mutex_unlock( & hippi_devp->devmutex );

	return 0;
}



int
hippiread( dev_t dev, uio_t *uiop )
{
	volatile union hip_c2b *c2b_rdp;
	hippi_vars_t *hippi_devp;
	struct hippi_fp_ulps *ulpp;
	caddr_t v_addr;
	int	cookie;
	int	error, ulpIndex;
	int	len, blen, rdlist_len;
	u_int	m = geteminor( dev );
	u_int	unit = UNIT_FROM_MINOR( m );
	int	pfn[ 1+HIPPIFP_MAX_READSIZE/NBPP ], *pfnp;

#ifdef HIPPI_BP
	if (ISBP_FROM_MINOR( m ))
		return EINVAL;
#endif /* HIPPI_BP */
	hippi_devp = &hippi_device[unit];

	/* Read must be single, 64-bit aligned, contiguous.
	 */
	if ( uiop->uio_iovcnt != 1 ||
	     ((long) (uiop->uio_iov->iov_base) & 7) ||
	     ( (uiop->uio_iov->iov_len) & 7) ||
	     uiop->uio_iov->iov_len > HIPPIFP_MAX_READSIZE ||
	     uiop->uio_iov->iov_len < 8 )
		return EINVAL;

	v_addr = uiop->uio_iov->iov_base;
	len = uiop->uio_iov->iov_len;

	/* Card must (still) be up */
	if ( ! (hippi_devp->hi_state & HI_ST_UP) )
		return ENODEV;
	
	/* Make sure clone device is bound.
	 */
	ASSERT( ISCLONE_FROM_MINOR(m) );
	ulpIndex = hippi_devp->clone[ CLONEID_FROM_MINOR(m) ].ulpIndex;
	if ( ulpIndex > HIPPIFP_MAX_OPEN_ULPS )
		return ENXIO;	/* unbound */
	
	ulpp = & hippi_devp->ulp[ ulpIndex ];

#ifdef DEBUG
	if ( ulpp->ulpId == HIPPI_ULP_PH ) {
		ASSERT( hippi_devp->dstLock );
	}
	else {
		ASSERT( ! hippi_devp->dstLock );
	}
#endif

	if ( 0 != (error=fast_userdma( v_addr, len, B_READ, &cookie ) ) )
		return error;

	/* Wait until there is data available to read.
	 */
	if ( psema( & ulpp->rd_sema, PWAIT | PCATCH ) ) {
		fast_undma( v_addr, len, B_READ, &cookie );
		return EINTR;
	}
	if ( ! (hippi_devp->hi_state & HI_ST_UP) ) {
		fast_undma( v_addr, len, B_READ, &cookie );
		return ENODEV;
	}
	if ( ulpp->rd_D2_avail < 0 ) {
		hippi_devp->clone[ CLONEID_FROM_MINOR(m) ].dst_errors =
			(ulpp->rd_D2_avail & 0xFF);
		fast_undma( v_addr, len, B_READ, &cookie );
		return EIO;
	}
	
	/* Just return header if appropriate.
	 */
	if ( ulpp->rd_fpHdr_avail > 0 ) {

		/* We bcopy this because the header is already in memory. */
		undma( v_addr, len, B_READ );

		hippi_devp->clone[ CLONEID_FROM_MINOR(m) ].dst_errors = 0;

		/* HIPPI-PH doesn't send headers up */
		ASSERT( ulpp->ulpId != HIPPI_ULP_PH );

		/* You're in trouble if you use a read buffer too small for
		 * an FP/D1 header.  Header gets discarded.
		 */
		if ( uiop->uio_iov->iov_len < ulpp->rd_fpHdr_avail ) {
			vsema( & ulpp->rd_sema );
			return EINVAL;
		}
		else if ( uiomove( (caddr_t) ulpp->rd_fpd1head,
			         ulpp->rd_fpHdr_avail, UIO_READ, uiop ) < 0 ) {
			vsema( & ulpp->rd_sema );
			return EFAULT;
		}

#ifdef DEBUG
		/* Double-check ULP-ID */
		ASSERT( * ( (u_char *) ulpp->rd_fpd1head ) == ulpp->ulpId );

		/* This makes sure we're getting a new header each time */
		bzero( ulpp->rd_fpd1head, HIPPIFP_HEADBUFSIZE );
#endif
		
		if ( ulpp->rd_D2_avail > 0 ) {
			ulpp->rd_offset = ulpp->rd_fpHdr_avail;
			ulpp->rd_fpHdr_avail = 0;

			/* It's this thread's responsibility to wake up
			 * someone to read the body of the packet.
			 */
			vsema( & ulpp->rd_sema );
		}
		else {
			/* D1 area with no D2 data... */

			ulpp->rd_offset = 0;
			ulpp->rd_fpHdr_avail = 0;

			/* Put the buffer back for next header.
			 */
			mutex_lock( & hippi_devp->devslock, PZERO );

			hippi_send_c2b( hippi_devp,
				HIP_C2B_SML | (ulpIndex+HIP_STACK_FP),
				HIPPIFP_HEADBUFSIZE,
				(void *)kvtophys((void *)ulpp->rd_fpd1head));

			hippi_wakeup_nolock( hippi_devp );

			mutex_unlock( & hippi_devp->devslock );
		}
	}
	else {
		ASSERT( ulpp->rd_D2_avail > 0 );

		ASSERT( len > 0 );
		
		ulpp->rd_D2_avail = 0;

		/* Do mapping all in one call...it's faster */
		if ( ! curproc_vtop( v_addr, len, pfn, sizeof(int) ) )
			panic( "hippiread: vtop failed!" );

		c2b_rdp = ulpp->rd_c2b_rdlist;
		rdlist_len = 0;
		pfnp = pfn;

		if ( io4ia_war && ((u_long)v_addr & (NBCL-1)) != 0 ) {
			blen = min( NBPP - ((u_long)v_addr & POFFMASK ), len );
#if _MIPS_SIM == _ABI64
			c2b_rdp->c2bll = (u_long)blen<<48 |
				(u_long)HIP_C2B_BIG<<40 |
				(u_long)kvtophys(ulpp->io4ia_war_page0);
#else
			c2b_rdp->c2b_addrhi = 0;
			c2b_rdp->c2b_addr =
				(u_long)kvtophys(ulpp->io4ia_war_page0);
			c2b_rdp->c2b_param = blen;
#endif
			v_addr += blen;
			len -= blen;
			c2b_rdp++;
			rdlist_len++;
			pfnp++;
		}

		while ( len > 0 ) {

			blen = min( NBPP - ((u_long)v_addr & POFFMASK ), len );

			ASSERT(!io4ia_war || ((u_long)v_addr & (NBCL-1)) == 0);
			if ( io4ia_war && (blen&(NBCL-1)) != 0 ) {
				ASSERT( blen==len );
				break;
			}

#if _MIPS_SIM == _ABI64
			c2b_rdp->c2bll = (u_long)blen<<48 |
				(u_long)HIP_C2B_BIG<<40 |
			 	((u_long)*pfnp<<PNUMSHFT) |
				((u_long)v_addr&POFFMASK);
#else
			c2b_rdp->c2b_addrhi = 0;
			c2b_rdp->c2b_addr =
			 (*pfnp<<PNUMSHFT)|((u_long)v_addr&POFFMASK);
			c2b_rdp->c2b_param = blen;
			c2b_rdp->c2b_op = HIP_C2B_BIG;
#endif

			v_addr += blen;
			len -= blen;
			c2b_rdp++;
			rdlist_len++;
			pfnp++;
		}

		if ( len > 0 ) {
			ASSERT( io4ia_war );
			ASSERT( ((u_long)v_addr & (NBCL-1)) == 0 );
#if _MIPS_SIM == _ABI64
			c2b_rdp->c2bll = (u_long)len<<48 |
				(u_long)HIP_C2B_BIG<<40 |
				(u_long)kvtophys(ulpp->io4ia_war_page1);
#else
			c2b_rdp->c2b_addrhi = 0;
			c2b_rdp->c2b_addr =
				(u_long)kvtophys(ulpp->io4ia_war_page1);
			c2b_rdp->c2b_param = len;
			c2b_rdp->c2b_op = HIP_C2B_BIG;
#endif

			v_addr += len;
			len = 0;
			c2b_rdp++;
			rdlist_len++;
		}

		c2b_rdp->c2b_op = HIP_C2B_EMPTY;
		ASSERT( len == 0 );
		ASSERT( rdlist_len<C2B_RDLISTPGS*NBPP/sizeof(union hip_c2b) );

		mutex_lock( & hippi_devp->devslock, PZERO );
		hippi_send_c2b( hippi_devp, HIP_C2B_READ |
			( ulpp->ulpId == HIPPI_ULP_PH ?
				HIP_STACK_RAW : (ulpIndex+HIP_STACK_FP) ),
			rdlist_len*sizeof(union hip_c2b),
			(void *)kvtophys( (void *)ulpp->rd_c2b_rdlist ) );

		hippi_wakeup_nolock( hippi_devp );

		mutex_unlock( & hippi_devp->devslock );

		/* wait for input DMA done.  */
		psema( & ulpp->rd_dmadn, PZERO );

		if ( ulpp->rd_count < 0 ) {
			ulpp->rd_offset = 0;
			hippi_devp->clone[ CLONEID_FROM_MINOR(m) ].dst_errors =
				(ulpp->rd_count & 0xFF);

			/* In an error case, we have no idea where the DMA
			 * stopped so we have to flush the IA cache with
			 * io4_flush_cache().
			 */
			if ( io4ia_war )
				io4_flush_cache( hippi_devp->hi_swin );

			/* Put the buffer back for next header.
			 */
			mutex_lock( & hippi_devp->devslock, PZERO );

			hippi_send_c2b( hippi_devp,
				HIP_C2B_SML | (ulpIndex+HIP_STACK_FP),
				HIPPIFP_HEADBUFSIZE,
				(void *)kvtophys((void *)ulpp->rd_fpd1head));

			hippi_wakeup_nolock( hippi_devp );

			mutex_unlock( & hippi_devp->devslock );
			undma(uiop->uio_iov->iov_base,uiop->uio_iov->iov_len,
				B_READ );
			return EIO;
		}
		else {
			hippi_devp->clone[CLONEID_FROM_MINOR(m)].dst_errors=0;

			uiop->uio_resid=uiop->uio_iov->iov_len-ulpp->rd_count;

			if ( io4ia_war ) {
			    /* If read was bigger than threshold we have to
			     * flush the IA totally.
			     */
			    if ( ulpp->rd_count > (HIPPI_DST_THRESH-1)*1024 )
				io4_flush_cache( hippi_devp->hi_swin );
			    /* If read was incomplete, touch the last byte. */
			    else if ( uiop->uio_resid &&
	     		        	(((u_long)uiop->uio_iov->iov_base+
					   ulpp->rd_count )& (NBCL-1)) != 0 ) {
				*( (volatile u_char *)
					((u_long)uiop->uio_iov->iov_base+
				 	 ulpp->rd_count-1) );
			    }
			}

			if ( ulpp->ulpFlags & ULPFLAG_R_MORE ) {

				/* More D2 area to read */

				ulpp->ulpFlags &= ~ULPFLAG_R_MORE;
				if ( ulpp->rd_offset + ulpp->rd_count <
						ulpp->rd_offset )
					ulpp->rd_offset = 0x7fffffff; /* XXX */
				else
					ulpp->rd_offset += ulpp->rd_count;
				
				ulpp->rd_D2_avail = 1;

				if ( (hippi_devp->hi_state & HI_ST_UP) != 0 ) {
					if (ulpp->ulpFlags&ULPFLAG_R_POLL) {
						pollwakeup( ulpp->rd_pollhdp,
							POLLIN|POLLRDNORM );
						ulpp->ulpFlags &=
							~ULPFLAG_R_POLL;
					}
					vsema( & ulpp->rd_sema );
				}
			}
			else {
				ulpp->rd_offset = 0;

				/* Put the buffer back for next header.
				 */
				mutex_lock( & hippi_devp->devslock, PZERO );

				hippi_send_c2b( hippi_devp,
					HIP_C2B_SML | (ulpIndex+HIP_STACK_FP),
					HIPPIFP_HEADBUFSIZE,
					(void *)
					kvtophys((void *)ulpp->rd_fpd1head));

				hippi_wakeup_nolock( hippi_devp );

				mutex_unlock( & hippi_devp->devslock );
			}
		}

		undma(uiop->uio_iov->iov_base,uiop->uio_iov->iov_len,B_READ );
	}

	if ( io4ia_war ) {
		v_addr = uiop->uio_iov->iov_base;
		len = uiop->uio_iov->iov_len;
		if ( ((u_long)v_addr&(NBCL-1)) != 0 ) {
			blen = min( NBPP - ((u_long)v_addr&POFFMASK), len );
			if ( copyout( ulpp->io4ia_war_page0,v_addr,blen ) < 0 )
				return EFAULT;
			v_addr += blen;
			len -= blen;
		}
		ASSERT( ((u_long)v_addr&(NBCL-1)) == 0 || len == 0 );
		if ( (len&(NBCL-1)) != 0 ) {
			blen = min( ((u_long)v_addr+len)&POFFMASK, len );
			if ( copyout( ulpp->io4ia_war_page1,
				      v_addr+len-blen, blen ) < 0 )
				return EFAULT;
		}
	}

	return 0;
}





int
hippiwrite( dev_t dev, uio_t *uiop )
{
	volatile union hip_d2b *d2bp;
	int *ppfn;
	struct hippi_fp_clones *clonep;
	hippi_vars_t *hippi_devp;
	caddr_t	v_addr;
	int	m = geteminor( dev );
	int	unit = UNIT_FROM_MINOR( m );
	u_int 	*fphdr, *fphdr_buf;
	int	i, error, len, blen, fphdr_len, d2b_flags=0;
	int	clone_flags, dma_chunks, fburst;
	int	cookie;
	int	pfn[ HIPPIFP_MAX_WRITESIZE/NBPP+1 ];

#ifdef HIPPI_BP
	if (ISBP_FROM_MINOR( m ))
		return EINVAL;
#endif /* HIPPI_BP */
	hippi_devp = &hippi_device[unit];

	/* Write must be single, aligned, contiguous.
	 */
	if ( uiop->uio_iovcnt != 1 ||
	     ((long) (uiop->uio_iov->iov_base) & 7) ||
	     ( (uiop->uio_iov->iov_len) & 7) ||
	     uiop->uio_iov->iov_len > HIPPIFP_MAX_WRITESIZE ||
	     uiop->uio_iov->iov_len < 8 )
		return EINVAL;	/* XXX */

	/* Card must be (still) up */
	if ( ! (hippi_devp->hi_state & HI_ST_UP) )
		return ENODEV;

	/* Check to make sure clone device is bound.
	 */
	clonep = & hippi_devp->clone[CLONEID_FROM_MINOR(m)];
	if ( clonep->ulpIndex >= ULPIND_UNBOUND )
		return ENXIO;
	
	/* Grab one of four write semaphores */
	if ( psema( & hippi_devp->rawoutq_sema, PWAIT | PCATCH ) )
		return EINTR;
	/* Card must be (still) up-- don't worry about rawoutq_sema */
	if ( ! (hippi_devp->hi_state & HI_ST_UP) )
		return ENODEV;

	/* Grab HIPPI source semaphore if we don't own it. */
	if ( ! (clonep->cloneFlags & CLONEFLAG_W_HOLDING) ) {
		if ( psema( & hippi_devp->src_sema, PWAIT | PCATCH ) ) {
			vsema( & hippi_devp->rawoutq_sema );
			return EINTR;
		}
		/* Card must be (still) up */
		if ( ! (hippi_devp->hi_state & HI_ST_UP) )
			return ENODEV;
	}

	clonep = & hippi_devp->clone[ CLONEID_FROM_MINOR(m) ];

	v_addr = uiop->uio_iov->iov_base;
	len = uiop->uio_iov->iov_len;

	if ( 0 != (error=fast_userdma( v_addr, len, B_WRITE, &cookie )) ) {
		vsema( & hippi_devp->rawoutq_sema );
		if ( ! (clonep->cloneFlags & CLONEFLAG_W_HOLDING) )
			vsema(  & hippi_devp->src_sema );
		return error;
	}

	uiop->uio_resid = 0; /* assume we'll transfer it all */

	clone_flags = clonep->cloneFlags;

	/* Figure out FP header that goes onto this
	 * packet.
	 */
	fphdr = 0;
	fphdr_len = 0;
	fphdr_buf = 0;
	if ( ! (clone_flags & CLONEFLAG_W_NBOP) &&
	     ( clonep->ulpId != HIPPI_ULP_PH ||
	       !( clone_flags & CLONEFLAG_W_NBOC) ) ) {

	   int	d1size = 0;
	   __uint32_t	*p;

	   if ( ! ( clone_flags & CLONEFLAG_W_NBOC ) )
		fphdr_len += 4; /* sizeof( I-field ) */
	   if ( clonep->ulpId != HIPPI_ULP_PH ) {

		d1size = (clonep->wr_fpHdr.hfp_d1d2off & HFP_D1SZ_MASK);
		fphdr_len += sizeof(hippi_fp_t);
	   }
	
	   ASSERT( fphdr_len > 0 );

	   fphdr_buf = kmem_zalloc( fphdr_len+4, KM_SLEEP );
	   fphdr = fphdr_buf;
	   if ( (( (long)fphdr + fphdr_len ) & 4) != 0 ) /* XXX: gag */
		fphdr++;
	
	   p=fphdr;
	   if ( ! ( clone_flags & CLONEFLAG_W_NBOC ) )
		*p++ = clonep->wr_Ifield;
	   if ( clonep->ulpId != HIPPI_ULP_PH ) {
		bcopy( &clonep->wr_fpHdr, p, sizeof(hippi_fp_t));
		if ( clonep->wr_pktOutResid == 0 ) {
			if ( len >= d1size )
				((hippi_fp_t *)p )->hfp_d2size = len - d1size;
			else
				((hippi_fp_t *)p )->hfp_d2size = 0; /* XXX */
		}
		else {
			if ( clonep->wr_pktOutResid >= d1size )
				( (hippi_fp_t *) p )->hfp_d2size =
					clonep->wr_pktOutResid - d1size;
			else
				( (hippi_fp_t *) p )->hfp_d2size = 0; /* XXX */
		}
	   }
	}

	if ( clonep->wr_pktOutResid && clonep->wr_pktOutResid !=
			HIPPI_D2SIZE_INFINITY ) {

		if ( len > clonep->wr_pktOutResid ) {
			uiop->uio_resid = len - clonep->wr_pktOutResid;
			len = clonep->wr_pktOutResid;
			clonep->wr_pktOutResid = 0;
		}
		else
			clonep->wr_pktOutResid -= len;

	}

	/* Handle short-first-burst */
	fburst = clonep->wr_fburst;
	blen = len + ((clonep->ulpId==HIPPI_ULP_PH) ? 0 : sizeof(hippi_fp_t));
	if ( fburst && ( (clone_flags & CLONEFLAG_W_NBOP) || fburst > blen ||
	     (fburst == blen && clonep->wr_pktOutResid == 0) ) )
		fburst = 0;

	/* Manipulate all the flags.
	 */
	d2b_flags = ( (clone_flags & (CLONEFLAG_W_NBOC|CLONEFLAG_W_NBOP)) ?
			0 : HIP_D2B_IFLD ) |
		( (clone_flags & CLONEFLAG_W_PERMCONN) ? HIP_D2B_NEOC : 0 ) |
		( (clonep->wr_pktOutResid > 0) ?
					(HIP_D2B_NEOC|HIP_D2B_NEOP) : 0 ) ;
	if ( clonep->wr_pktOutResid > 0 )
		clone_flags |= (CLONEFLAG_W_NBOP|CLONEFLAG_W_NBOC);
	else {
		clone_flags &= ~(CLONEFLAG_W_NBOP|CLONEFLAG_W_NBOC);
		if ( (clone_flags & CLONEFLAG_W_PERMCONN) )
			clone_flags |= CLONEFLAG_W_NBOC;
	}
	clonep->cloneFlags = clone_flags;


	/* Start loading packet onto HW Q.
	 */
	d2bp = D2B_NXT( hippi_devp->hi_d2bp_hd );
	dma_chunks = 0;

	if ( fphdr ) {
		/* Put I field, FP header, D1 header onto HW Q */
#if _MIPS_SIM == _ABI64
		d2bp->ll = (u_long)fphdr_len<<48 | ((u_long) kvtophys(fphdr));
#else
		d2bp->l[0] = fphdr_len<<16;
		d2bp->sg.addr = (u_long)kvtophys( fphdr );
#endif
		d2bp = D2B_NXT( d2bp );
		dma_chunks++;
	}

	/* Do mapping all in one call...it's faster */
	if ( ! curproc_vtop( v_addr, len, pfn, sizeof(int) ) )
		panic( "hippiwrite: vtop2 failed!" );

	ppfn=pfn;
	while ( len > 0 ) {
		blen = min( NBPP - ((u_long)v_addr & POFFMASK ), len );

#if _MIPS_SIM == _ABI64
		d2bp->ll = (u_long)blen<<48 | ((u_long) *(ppfn++)<<PNUMSHFT) |
			((u_long)v_addr&POFFMASK);
#else
		d2bp->l[0] = blen<<16;
		d2bp->sg.addr = ((u_long) *(ppfn++)<<PNUMSHFT)|
				((u_long) v_addr&POFFMASK);
#endif

		v_addr += blen;
		d2bp = D2B_NXT(d2bp);
		dma_chunks++;
		len -= blen;
	}

	d2bp->hd.flags = HIP_D2B_BAD;

	hippi_devp->hi_d2bp_hd->hd.chunks = dma_chunks;
	hippi_devp->hi_d2bp_hd->hd.stk = HIP_STACK_FP;
	hippi_devp->hi_d2bp_hd->hd.fburst = fburst;
	hippi_devp->hi_d2bp_hd->hd.flags = HIP_D2B_RDY | d2b_flags;
	hippi_devp->hi_d2bp_hd = d2bp;

	hippi_wakeup( hippi_devp );

	/* Pick semaphore we are to sleep on BEFORE releasing src sema */
	i = hippi_devp->rawoutq_in;
	if ( ++hippi_devp->rawoutq_in >= HIPPIFP_MAX_WRITES )
		hippi_devp->rawoutq_in=0;

	if ( clonep->ulpId != HIPPI_ULP_PH &&
	     (clone_flags & (CLONEFLAG_W_NBOC|CLONEFLAG_W_NBOP)) )
		clonep->cloneFlags |= CLONEFLAG_W_HOLDING;
	else {
		clonep->cloneFlags &= ~CLONEFLAG_W_HOLDING;
		vsema( &hippi_devp->src_sema );
	}

	/* wait for ODONE. */
	psema( & hippi_devp->rawoutq_sleep[ i ], PZERO );
	
	clonep->src_error = hippi_devp->rawoutq_error[ i ];

	if ( ! (hippi_devp->hi_state & HI_ST_UP) )
		clonep->src_error = B2H_OSTAT_SHUT;
	else {
		/* If an error occurs and we're doing more than a single
		 * write packet/connection, we need to clean up here...
		 */
		if ( clonep->src_error &&
		     ( clonep->cloneFlags & (CLONEFLAG_W_NBOC|CLONEFLAG_W_NBOP|
						CLONEFLAG_W_HOLDING) )  ) {

			/* XXX: disconnect source/drop packet! */
			d2bp = hippi_devp->hi_d2bp_hd;
			d2bp->hd.chunks = 0;
			d2bp->hd.stk = HIP_STACK_FP;
			d2bp = D2B_NXT( d2bp );
			d2bp->hd.flags = HIP_D2B_BAD;
			hippi_devp->hi_d2bp_hd->hd.fburst = 0;
			hippi_devp->hi_d2bp_hd->hd.flags = 
				HIP_D2B_RDY | HIP_D2B_NACK;
			hippi_devp->hi_d2bp_hd = d2bp;
			hippi_wakeup( hippi_devp );
			
			if ( clonep->cloneFlags & CLONEFLAG_W_HOLDING )
				vsema( &hippi_devp->src_sema );

			clonep->cloneFlags &= ~(CLONEFLAG_W_HOLDING |
				CLONEFLAG_W_NBOC | CLONEFLAG_W_NBOP |
				CLONEFLAG_W_PERMCONN );
			clonep->wr_pktOutResid = 0;

		}

		vsema( & hippi_devp->rawoutq_sema );
	}

	if ( fphdr_buf )
		kmem_free( fphdr_buf, fphdr_len+4 );
	
	fast_undma( uiop->uio_iov->iov_base, uiop->uio_iov->iov_len, B_WRITE,
		&cookie );
	
	return clonep->src_error ? EIO : 0;
}



/*ARGSUSED*/
int
hippiioctl(dev_t dev, int cmd, long arg, int mode, cred_t *cred_p, int *rvalp)
{
	int	ulpIndex, ulp_id, error = 0;
	u_int	m = geteminor(dev);
	u_int	unit = UNIT_FROM_MINOR( m );
	hippi_vars_t *hippi_devp = &hippi_device[unit];
	struct hippi_fp_ulps *ulpp;
	struct hippi_fp_clones *clonep;

#ifdef HIPPI_DEBUG
	if ( hippi_debug )
		printf( "hippiioctl(minor=%d,cmd=%x,arg=%x) called.\n",
			m, cmd, arg );
#endif

#ifdef HIPPI_BP
	if (ISBP_FROM_MINOR( m ))
		return( hippibpioctl( dev, cmd, arg, mode, cred_p, rvalp) );
#endif /* HIPPI_BP */

	ulpp = 0;

	mutex_lock( & hippi_devp->devmutex, PZERO );

	ASSERT( ISCLONE_FROM_MINOR(m) );

	clonep = & hippi_devp->clone[ CLONEID_FROM_MINOR(m) ];
	ulp_id = clonep->ulpId;
	ulpIndex = clonep->ulpIndex;
	if ( ulpIndex <= HIPPIFP_MAX_OPEN_ULPS )
		ulpp = & hippi_devp->ulp[ ulpIndex ];
	else
		ulpp = 0;
	
	ASSERT( (mode & (FREAD|FWRITE)) == clonep->mode );

	/* ioctl's starting at 64 are administrative and don't need
	 * the card up and running.
	 */
	if ( (hippi_devp->hi_state & HI_ST_UP) == 0 && (cmd&255)<64 ) {
		mutex_unlock( & hippi_devp->devmutex );
		return ENODEV;
	}


	switch (cmd) {

	case HIPIOC_BIND_ULP:
		/* XXX: allow re-binding??? */
		if ( ulpIndex != ULPIND_UNBOUND ) {
			error = EINVAL;
			break;
		}

		ulp_id = arg;
		if ( ulp_id > HIPPI_ULP_MAX && ulp_id != HIPPI_ULP_PH ) {
			error = EINVAL;
			break;
		}

		error = hippi_fp_bind( hippi_devp, m, ulp_id, clonep->mode );

		break;

	case HIPIOCW_I:
		if ( ulpIndex == ULPIND_UNBOUND ) {
			error = EINVAL;
			break;
		}
		clonep->wr_Ifield = (hippi_i_t) arg;
		break;

	case HIPIOCW_D1_SIZE:
		if ( ulpIndex == ULPIND_UNBOUND ) {
			error = EINVAL;
			break;
		}
		/* Must be FP device.
		 */
		if ( ulp_id == HIPPI_ULP_PH ) {
			error = EINVAL;
			break;
		}

		if ( arg < 0 || arg > HIPPI_MAX_D1AREASIZE || (arg & 7) ) {
			error = EINVAL;
			break;
		}

		if ( arg > 0 )
			clonep->wr_fpHdr.hfp_flags |= HFP_FLAGS_P;
		else
			clonep->wr_fpHdr.hfp_flags &= ~HFP_FLAGS_P;
		clonep->wr_fpHdr.hfp_d1d2off = arg;
		break;
	
	case HIPIOCW_START_PKT:
		if ( ulpIndex == ULPIND_UNBOUND ) {
			error = EINVAL;
			break;
		}
		/* Haven't finished last packet
		 */
		if ( clonep->cloneFlags & CLONEFLAG_W_NBOP ) {
			error = EINVAL;
			break;
		}

		clonep->wr_pktOutResid = arg;

		break;
	
	case HIPIOCW_CONNECT:
		if ( ulpIndex == ULPIND_UNBOUND ) {
			error = EINVAL;
			break;
		}
		/* Already connected! */
		if ( clonep->cloneFlags & CLONEFLAG_W_PERMCONN ) {
			error = EINVAL;
			break;
		}

		clonep->cloneFlags |= CLONEFLAG_W_PERMCONN;
		clonep->wr_Ifield = arg;

		break;
	
	case HIPIOCW_SHBURST:
		if ( ulpIndex == ULPIND_UNBOUND ||
		     arg < 0 || arg > 1024 || (arg&3) ) {
			error = EINVAL;
			break;
		}

		if ( ulp_id != HIPPI_ULP_PH ) {

			/* If HIPPI-FP, enforce that FP/D1 are exactly
			 * first burst.
			 */
			if (arg > 0 && arg != 8+clonep->wr_fpHdr.hfp_d1d2off) {
				error = EINVAL;
				break;
			}

			/* Set B-bit? */
			if ( arg )
				clonep->wr_fpHdr.hfp_flags |= HFP_FLAGS_B;
			else
				clonep->wr_fpHdr.hfp_flags &= ~HFP_FLAGS_B;

		}

		/* Allow using 1024 to set B-bit without really shortening
		 * first burst.
		 */
		if ( arg == 1024 )
			clonep->wr_fburst = 0;
		else
			clonep->wr_fburst = arg;

		break;
	
	case HIPIOCW_END_PKT:
		if ( ulpIndex == ULPIND_UNBOUND ) {
			error = EINVAL;
			break;
		}

		/* XXX: null operation if packet is already done */
		if ( clonep->wr_pktOutResid == 0 )
			break;

		clonep->wr_pktOutResid = 0;
		clonep->cloneFlags &= ~CLONEFLAG_W_NBOP;

		/* XXX: tell card to drop pkt */
		{
			volatile union hip_d2b *d2bp;

			/* XXX: drop packet! */
			d2bp = hippi_devp->hi_d2bp_hd;
			d2bp->hd.chunks = 0;
			d2bp->hd.stk = HIP_STACK_FP;
			d2bp = D2B_NXT( d2bp );
			d2bp->hd.flags = HIP_D2B_BAD;
			hippi_devp->hi_d2bp_hd->hd.fburst = 0;
		        hippi_devp->hi_d2bp_hd->hd.flags =
			  ( clonep->cloneFlags & CLONEFLAG_W_PERMCONN ) ?
				HIP_D2B_RDY | HIP_D2B_NEOC | HIP_D2B_NACK :
				HIP_D2B_RDY | HIP_D2B_NACK;
			hippi_devp->hi_d2bp_hd = d2bp;
			hippi_wakeup( hippi_devp );
		}

		/* Release lock if we aren't holding a connection */
		if ( ! (clonep->cloneFlags & CLONEFLAG_W_PERMCONN) ) {
			if ( (clonep->cloneFlags & CLONEFLAG_W_HOLDING) )
				vsema( &hippi_devp->src_sema );
			clonep->cloneFlags &=
				~(CLONEFLAG_W_HOLDING|CLONEFLAG_W_NBOC);

		}
		break;

	case HIPIOCW_DISCONN:
		if ( ulpIndex == ULPIND_UNBOUND ) {
			error = EINVAL;
			break;
		}
		if ( !( clonep->cloneFlags & CLONEFLAG_W_PERMCONN) ) {
			error = EINVAL;
			break;
		}

		clonep->wr_pktOutResid = 0;
		clonep->cloneFlags &=
		  ~(CLONEFLAG_W_PERMCONN|CLONEFLAG_W_NBOP|CLONEFLAG_W_NBOC);
		
		/* XXX: tell card to disconnect (and drop pkt)! */
		{
			volatile union hip_d2b *d2bp;

			/* XXX: disconnect source/drop packet! */
			d2bp = hippi_devp->hi_d2bp_hd;
			d2bp->hd.chunks = 0;
			d2bp->hd.stk = HIP_STACK_FP;
			d2bp = D2B_NXT( d2bp );
			d2bp->hd.flags = HIP_D2B_BAD;
			hippi_devp->hi_d2bp_hd->hd.fburst = 0;
			hippi_devp->hi_d2bp_hd->hd.flags = 
				HIP_D2B_RDY | HIP_D2B_NACK;
			hippi_devp->hi_d2bp_hd = d2bp;
			hippi_wakeup( hippi_devp );
		}

		if ( (clonep->cloneFlags & CLONEFLAG_W_HOLDING) ) {

			clonep->cloneFlags &= ~CLONEFLAG_W_HOLDING;
			vsema( & hippi_devp->src_sema );

		}
		break;
	
	case HIPIOCW_ERR:
		if ( ulpIndex == ULPIND_UNBOUND )
			error = EINVAL;
		else
			*rvalp = clonep->src_error;
		break;
	
	case HIPIOCR_PKT_OFFSET:
		if ( ! ulpp )
			error = EINVAL;
		else
			*rvalp = ulpp->rd_offset;
		break;
	
	case HIPIOCR_ERRS:
		if ( ulpIndex == ULPIND_UNBOUND )
			error = EINVAL;
		else
			*rvalp = clonep->dst_errors;
		break;
	
	case HIPIOC_ACCEPT_FLAG:
		if ( arg )
			hippi_hwflags( hippi_devp,
				hippi_devp->hi_hwflags | HIP_FLAG_ACCEPT);
		else
			hippi_hwflags( hippi_devp,
				hippi_devp->hi_hwflags & ~HIP_FLAG_ACCEPT );
		break;

	case HIPIOC_GET_STATS: {
		int	i;
		__uint32_t *temp = kmem_alloc(sizeof(hippi_stats_t), KM_SLEEP);

		mutex_lock( & hippi_devp->devslock, PZERO );

		hippi_wait( hippi_devp );
		hiphw_op( hippi_devp, HCMD_STATUS );
		hippi_wait( hippi_devp );

		/* XXX: copyout might not use 32-bit operations! */

		for (i=0; i < sizeof(hippi_stats_t)/sizeof(__uint32_t); i++)
			temp[i] = ((__uint32_t *)hippi_devp->hi_stat_area)[i];
		
		mutex_unlock( & hippi_devp->devslock );

		if ( copyout( (caddr_t)temp, (caddr_t)arg,
			sizeof(hippi_stats_t)) < 0 )
				error = EFAULT;
		
		kmem_free( temp, sizeof(hippi_stats_t) );
	}
		break;
	
#ifdef HIPPI_BP
	case HIPIOC_GET_BPSTATS:
		{
		__uint32_t hippibp_stats[sizeof(hippibp_stats_t)/4];
		int i;

		if ((hippi_devp->hi_bp_stats == 0) ||
		    !(hippi_devp->hi_state & HI_ST_UP)) {
			error = ENODEV;
			break;
		}

		mutex_lock( &hippi_devp->devslock, PZERO );

		hippi_wait( hippi_devp );
		hiphw_op( hippi_devp, HCMD_STATUS );
		hippi_wait( hippi_devp );

		/* XXX: copyout might not use 32-bit operations! */

		/* Copy from controller using word access */

		for (i=0; i<(sizeof(hippibp_stats_t)/4); i++)
			hippibp_stats[i] = hippi_devp->hi_bp_stats[i];

		mutex_unlock( &hippi_devp->devslock  );

		if (copyout((caddr_t)hippibp_stats, (caddr_t)arg,
			    sizeof(hippibp_stats_t)))
			error = EFAULT;
		break;
		}

	case HIPIOC_SET_BPCFG: {
		struct hip_bp_config bp_cfg;

		if (copyin((caddr_t)arg, (caddr_t)&bp_cfg,
			sizeof(struct hip_bp_config)) < 0) {
			error =  EFAULT;
			break;
		}
		if (hippi_devp->hippibp_state & HIPPIBP_ST_OPENED) {
			error = EINVAL;
			break;
		}

		/* Allocate ByPass control structure if it isn't already allocated.
		 * Make sure controller is operational before performing allocation
		 * since it dynamically sets up the initialization info.
		 */

		if (!(hippi_devp->hippibp_state & HIPPIBP_ST_CONFIGED) &&
			(error = hippibpinit( hippi_devp)))
				break;

	  	hippi_devp->bp_ulp = bp_cfg.ulp;
		hippi_devp->bp_maxjobs = 
			MIN(bp_cfg.max_jobs, HIPPIBP_MAX_JOBS);
		hippi_devp->bp_maxportids = 
			MIN(bp_cfg.max_portids, HIPPIBP_MAX_PORTIDS);
		hippi_devp->bp_maxdfmpgs =
			MIN(bp_cfg.max_dfm_pgs, HIPPIBP_MAX_DMAP_PGS);
		hippi_devp->bp_maxsfmpgs =
			MIN(bp_cfg.max_sfm_pgs, HIPPIBP_MAX_SMAP_PGS);
		hippi_devp->bp_maxddqpgs = bp_cfg.max_ddq_pgs;

		/* need to enforce the firmware limits too */
		hippi_devp->bp_maxjobs = 
			MIN(hippi_devp->bp_maxjobs,
			    hippi_devp->cached_bp_fw_conf.num_jobs);
		hippi_devp->bp_maxportids = 
			MIN(hippi_devp->bp_maxportids,
			    hippi_devp->cached_bp_fw_conf.num_ports);
		hippi_devp->bp_maxdfmpgs =
			MIN(hippi_devp->bp_maxdfmpgs,
			    (hippi_devp->cached_bp_fw_conf.dfm_size)/sizeof(int));
		hippi_devp->bp_maxsfmpgs =
			MIN(hippi_devp->bp_maxsfmpgs,
			    (hippi_devp->cached_bp_fw_conf.sfm_size)/sizeof(int));

		dprintf(1,("set_bpcfg(%d): ulp 0x%x jobs %d portids %d\n",
		      unit,bp_cfg.ulp, bp_cfg.max_jobs,bp_cfg.max_portids));

		dprintf(1,("set_bpcfg(%d): dfmpgs %d sfmpgs %d ddqpgs %d\n",
		      unit,bp_cfg.max_dfm_pgs, bp_cfg.max_sfm_pgs,
			   bp_cfg.max_ddq_pgs));

		dprintf(1,("set_bpcfg(%d): CONFIG maxjobs %d maxportids %d\n",
		      unit, hippi_devp->bp_maxjobs,hippi_devp->bp_maxportids));

		dprintf(1,("set_bpcfg(%d): CONFIG maxdfmpgs %d maxsfmpgs %d\n",
		      unit,hippi_devp->bp_maxdfmpgs,hippi_devp->bp_maxsfmpgs));

		mutex_lock( &hippi_devp->devslock, PZERO );
		hippi_wait( hippi_devp );
			
		hippi_devp->hi_hc->arg.bp_conf.ulp = hippi_devp->bp_ulp;
		hiphw_op( hippi_devp, HCMD_BP_CONF);
			
		mutex_unlock( &hippi_devp->devslock  );

		if (error = hippibp_config_driver( hippi_devp ))
			break;

		hippi_devp->hippibp_state |= HIPPIBP_ST_CONFIGED;
			
		break;
		}

	case HIPIOC_GET_BPCFG: {
		struct hip_bp_config bp_cfg;

		if (!hippi_devp->bp_jobs)
			bzero( &bp_cfg, sizeof(struct hip_bp_config));
		else {
			bp_cfg.ulp = hippi_devp->bp_ulp;
			bp_cfg.max_jobs = hippi_devp->bp_maxjobs;
			bp_cfg.max_portids = hippi_devp->bp_maxportids;
			bp_cfg.max_dfm_pgs = hippi_devp->bp_maxdfmpgs;
			bp_cfg.max_sfm_pgs = hippi_devp->bp_maxsfmpgs;
			bp_cfg.max_ddq_pgs = hippi_devp->bp_maxddqpgs;
		}
		if (copyout((caddr_t)&bp_cfg, (caddr_t)arg,
			sizeof(struct hip_bp_config)) < 0) {
			error =  EFAULT;
			break;
		}

		break;
		}
#endif /* HIPPI_BP */

	case HIPIOC_STIMEO:
		hippi_devp->hi_stimeo = 1+(arg/250);
		hippi_hwflags( hippi_devp, hippi_devp->hi_hwflags );
		break;

#ifdef HIPIOC_DTIMEO
	case HIPIOC_DTIMEO:
		hippi_devp->hi_dtimeo = 1+(arg/250);
		hippi_hwflags( hippi_devp, hippi_devp->hi_hwflags );
		break;
#endif

	case HIPPI_GET_FIRMVERS:

		/* Return the version of the firmware
		 * installed in the HIPPI adapter.
		 */

		*rvalp = hippi_devp->hi_firmvers;
		break;
	
	case HIPPI_GET_DRVRVERS:

		/* Return the version of the firmware the
		 * driver expects.
		 */

		*rvalp = ehip_vers;
		break;

	case HIPPI_SETONOFF:
		if (!_CAP_ABLE(CAP_DEVICE_MGT)) {
			error = EPERM;
			break;
		}

		if ( ulpIndex != ULPIND_UNBOUND ) {
			error = EBUSY;
			break;
		}
		
		if ( arg && (hippi_devp->hi_state & HI_ST_UP) == 0 ) {
			int	i;

			/* Make sure this is only open file descriptor */
			i=0;
			while ( hippi_devp->clone[i].ulpIndex==ULPIND_UNUSED ||
				clonep == &hippi_devp->clone[i] )
				i++;
			if ( i<HIPPIFP_MAX_CLONES ) {
				error = EBUSY;
				break;
			}

			hippi_bd_bringup( hippi_devp );

			/* XXX: hippi_bd_bringup() resets all the
			 * ulpIndex's so put this one back
			 */

			clonep->ulpIndex = ULPIND_UNBOUND;
		}
		else if ( ! arg && (hippi_devp->hi_state & HI_ST_UP) != 0 )
			hippi_bd_shutdown( hippi_devp );
		else if ( ! arg ) {

			/* Already down?  Do another reset */

			*hippi_devp->hi_bdctl = HIP_BDCTL_RESET_29K;
			*hippi_devp->hi_bdctl = 0;
		}
		else /* board is already up */
			error = EINVAL;
		break;

	case HIPPI_PGM_FLASH: {
		u_char	*firm;
		struct hip_dwn hdwn;
		inventory_t *hipinvent;

		if (!_CAP_ABLE(CAP_DEVICE_MGT)) {
			error = EPERM;
			break;
		}

		/* Don't allow if card is up */
		if ( (hippi_devp->hi_state & HI_ST_UP) != 0 ) {
			error = EINVAL;
			break;
		}

		cmn_err( CE_NOTE, "hippi%d: reprogramming flash EEPROM.\n",
			hippi_devp->unit );

		if ( copyin( (caddr_t)arg, &hdwn, sizeof(hdwn) ) < 0 ) {
			mutex_unlock( & hippi_devp->devmutex );
			return EFAULT;
		}
		if ( hdwn.len > 0x1000000 ) {
			mutex_unlock( & hippi_devp->devmutex );
			return EINVAL;
		}

		firm = kmem_alloc( hdwn.len, KM_SLEEP );
		if ( ! firm ) {
			mutex_unlock( & hippi_devp->devmutex );
			return ENOMEM;
		}
		
		if ( copyin( (caddr_t)arg+sizeof(hdwn), firm, hdwn.len ) < 0) {
			kmem_free( firm, hdwn.len );
			mutex_unlock( & hippi_devp->devmutex );
			return EFAULT;
		}

#ifndef HIPPI_DEBUG
		*hippi_devp->hi_bdctl = HIP_BDCTL_RESET_29K;
#endif
		if ( hippieraseflash( hippi_devp ) < 0 ) {
			kmem_free( firm, hdwn.len );
			mutex_unlock( & hippi_devp->devmutex );
			return EIO;
		}

		if ( hippiwriteflash( hippi_devp, firm, hdwn.addr, hdwn.len )
		     < 0 ) {
			kmem_free( firm, hdwn.len );
			mutex_unlock( & hippi_devp->devmutex );
			return EIO;
		}

#ifdef HIPPI_DEBUG
		*hippi_devp->hi_bdctl = HIP_BDCTL_RESET_29K;
		*hippi_devp->hi_bdctl = 0;
		printf( "made EEPROM change...Restart 29K emulator.\n" );
		debug(0);
#else
		*hippi_devp->hi_bdctl = 0;
		hippi_devp->hi_firmvers = hdwn.vers;

		hipinvent = find_inventory(0, INV_NETWORK, INV_NET_HIPPI,
			INV_HIO_HIPPI, unit, -1 );

		/* update inventory */

		replace_in_inventory( hipinvent,
			INV_NETWORK, INV_NET_HIPPI, INV_HIO_HIPPI, unit,
			io4hip_adaps[unit].slot<<28 |
				io4hip_adaps[unit].padap<<24 |
					hippi_devp->hi_firmvers );

#endif /* HIPPI_DEBUG */

		kmem_free( firm, hdwn.len );

		mutex_unlock( & hippi_devp->devmutex );
		return error;
	}

	default:
		error = EINVAL;
		break;
	}

	mutex_unlock( & hippi_devp->devmutex );
	return error;
}


int
hippi_fp_bind( hippi_vars_t *hippi_devp, u_int m, int ulp_id, int mode )
{
	int	ulpIndex;
	struct	hippi_fp_ulps *ulpp;
	struct	hippi_fp_clones *clonep;

#ifdef HIPPI_DEBUG
	if ( hippi_debug )
		printf(
		"hippi_fp_bind: unit=%d m=0x%x ulp_id=0x%x mode = 0x%x\n",
			hippi_devp->unit, m, ulp_id, mode );
#endif /* DEBUG */
	
	ASSERT( ISCLONE_FROM_MINOR( m ) );

	if ( ! (hippi_devp->hi_state&HI_ST_UP) )
		return ENODEV;

	/* Don't allow HIPPI-LE bind */
	if ( ulp_id == HIPPI_ULP_LE )
		return EINVAL;
	
	/* Don't allow bind to HIPPI-PH if networking is UP */
	if ( ulp_id == HIPPI_ULP_PH && (hippi_devp->hi_hwflags&HIP_FLAG_IF_UP))
		return EBUSY;


	clonep = & hippi_devp->clone[ CLONEID_FROM_MINOR(m) ];
	clonep->ulpId = ulp_id;

	if ( ulp_id != HIPPI_ULP_PH ) {
		/* Init default FP header */
		bzero( & clonep->wr_fpHdr, sizeof(hippi_fp_t) );
		clonep->wr_fpHdr.hfp_ulp_id = ulp_id;
	}

	/* Init default I-field */
	clonep->wr_Ifield = HIPPI_DEFAULT_I;

	clonep->wr_pktOutResid = 0;
	clonep->wr_fburst = 0;

	if ( mode & FREAD ) {

	   if ( ulp_id == HIPPI_ULP_PH ) {
		if ( hippi_devp->ulp[HIPPIFP_MAX_OPEN_ULPS].opens > 0 )
			ulpIndex = HIPPIFP_MAX_OPEN_ULPS;
		else
			ulpIndex = 255;
	   }
	   else
		ulpIndex = hippi_devp->ulpFromId[ulp_id];


	   if ( ulpIndex >= HIPPIFP_MAX_OPEN_ULPS+1 ) {

		/* Allocate an active read ULP structure for this ULP
		 */
		if ( ulp_id == HIPPI_ULP_PH ) {
			ulpIndex = HIPPIFP_MAX_OPEN_ULPS;
			for (ulpIndex=0; ulpIndex<HIPPIFP_MAX_OPEN_ULPS; ulpIndex++)
				if ( hippi_devp->ulp[ulpIndex].opens > 0 )
					return EBUSY;
			hippi_devp->dstLock = 1;
		}
		else {
			if ( hippi_devp->dstLock )
				return EBUSY;

			for (ulpIndex=0; ulpIndex<HIPPIFP_MAX_OPEN_ULPS; ulpIndex++)
				if ( hippi_devp->ulp[ulpIndex].opens == 0 )
					break;

			/* Too many open ULP's? */
			if ( ulpIndex >= HIPPIFP_MAX_OPEN_ULPS )
				return EBUSY;	/* XXX: bad error value */

			hippi_devp->ulpFromId[ulp_id] = ulpIndex;
		}

		
		ulpp = & hippi_devp->ulp[ulpIndex];

		ulpp->opens = 1;
		ulpp->ulpId = ulp_id;
		ulpp->rd_fpHdr_avail = 0;
		ulpp->rd_D2_avail = 0;
		ulpp->rd_offset = 0;
		ulpp->ulpFlags = 0;
		ulpp->rd_count = 0;
		initsema( &ulpp->rd_sema, 0 );
		initsema( &ulpp->rd_dmadn, 0 );
		ulpp->rd_c2b_rdlist = (union hip_c2b *)
			kvpalloc( C2B_RDLISTPGS, VM_DIRECT|VM_STALE,0 );
		/* XXX: does this need to be done on a per-clone
		 * basis?  I can't right now.
		 */
		ulpp->rd_pollhdp = phalloc( KM_SLEEP );

		if ( io4ia_war ) {
			ulpp->io4ia_war_page0 =
				kvpalloc( 1, VM_DIRECT|VM_STALE,0 );
			ulpp->io4ia_war_page1 =
				kvpalloc( 1, VM_DIRECT|VM_STALE,0 );
		}

		/* First read on the ULP, tell hardware about
		 * ULP --> stk association
		 */
		mutex_lock( & hippi_devp->devslock, PZERO );

		hippi_wait( hippi_devp );
		if ( ulp_id == HIPPI_ULP_PH ) {
			hippi_devp->hi_hc->arg.cmd_data[0] = HIP_STACK_RAW;
			hiphw_op(hippi_devp,HCMD_ASGN_ULP);
			mutex_unlock( & hippi_devp->devslock );
		}
		else {
			hippi_devp->hi_hc->arg.cmd_data[0] =
				(ulp_id<<16)|(ulpIndex+HIP_STACK_FP);
			hiphw_op(hippi_devp,HCMD_ASGN_ULP);

			ulpp->rd_fpd1head = (hippi_fp_t *)
				kmem_alloc( HIPPIFP_HEADBUFSIZE,
					KM_SLEEP | KM_CACHEALIGN );

			hippi_send_c2b( hippi_devp, HIP_C2B_SML |
				(ulpIndex+HIP_STACK_FP),
				HIPPIFP_HEADBUFSIZE, (void *)kvtophys(
					(void *)ulpp->rd_fpd1head) );
			
			hippi_wakeup_nolock( hippi_devp );

			mutex_unlock( & hippi_devp->devslock );
		}
	   }
	   else {

		ulpp = & hippi_devp->ulp[ulpIndex];

		/* Attach another clone to this open ULP */
		ASSERT( ulpp->opens > 0 );
		ulpp->opens++;

	   }

	   clonep->ulpIndex = ulpIndex;

	}
	else
	   clonep->ulpIndex = ULPIND_NOREADERS;

	return 0;
}


hippipoll(dev_t dev, short events, int anyyet, short *reventsp,
	  struct pollhead **phpp )
{
	int	ulpIndex;
	u_int	m = geteminor(dev);
	u_int	unit = UNIT_FROM_MINOR( m );
	hippi_vars_t *hippi_devp = &hippi_device[unit];
	struct hippi_fp_ulps *ulpp;

#ifdef HIPPI_DEBUG
	if ( hippi_debug )
		printf( "hippipoll(minor=%d,events=%d) called.\n", m, events );
#endif
	
	ulpIndex = hippi_devp->clone[ CLONEID_FROM_MINOR(m) ].ulpIndex;
	if ( ulpIndex > HIPPIFP_MAX_OPEN_ULPS )
		return ENXIO;	/* unbound! */
	ulpp = & hippi_devp->ulp[ ulpIndex ];

	ASSERT( ulpp->opens > 0 );

	if ( events & (POLLIN | POLLRDNORM) ) {
		mutex_lock( & hippi_devp->devslock, PZERO );
		if ( ulpp->rd_D2_avail == 0 && ulpp->rd_fpHdr_avail == 0 ) {
			events &= ~(POLLIN | POLLRDNORM);
			ulpp->ulpFlags |= ULPFLAG_R_POLL;
			if (!anyyet)
				*phpp = ulpp->rd_pollhdp;
		}
		mutex_unlock( & hippi_devp->devslock );
	}

	/* XXX: we could do the same for writes, see if
	 * write semaphore is > 0 !!!
	 */

	*reventsp = events;
	return 0;
}


/*
 * hippi_fp_odone: must be called with interrupts OFF!
 */
/*ARGSUSED*/
void
hippi_fp_odone( hippi_vars_t *hippi_devp, volatile struct hip_d2b_hd *hd,
	int status )
{

#ifdef HIPPI_DEBUG
	if ( hippi_debug ) {
		printf("hippi_fp_odone called, hd=%x status=%d\n",
			(long)hd, status );
	}
#endif

	ASSERT( hd->chunks > 0 );

	hippi_devp->rawoutq_error[ hippi_devp->rawoutq_out ] = status;
	vsema( & hippi_devp->rawoutq_sleep[ hippi_devp->rawoutq_out ] );

	if ( ++hippi_devp->rawoutq_out >= HIPPIFP_MAX_WRITES )
		hippi_devp->rawoutq_out = 0;
}


void
hippi_fp_input( hippi_vars_t *hippi_devp, volatile struct hip_b2h *b2hp )
{
	struct hippi_fp_ulps *ulpp;

#ifdef HIPPI_DEBUG
	if ( hippi_debug )
		printf( "hippi_fp_input: b2hp=%x (%x %x)\n",
			(long)b2hp, *((int *)b2hp), *((int *)b2hp+1) );
#endif
	
	if ( (b2hp->b2h_op & HIP_B2H_STMASK) == HIP_STACK_RAW )
		ulpp = & hippi_devp->ulp[ HIPPIFP_MAX_OPEN_ULPS ];
	else
		ulpp = & hippi_devp->ulp[
			(b2hp->b2h_op & HIP_B2H_STMASK) - HIP_STACK_FP ];

	if ( ulpp->opens == 0 )		/* XXX */
		return;

	if ( (b2hp->b2h_op & HIP_B2H_OPMASK) == HIP_B2H_IN ) {
		
		if ( ulpp->ulpFlags & ULPFLAG_R_POLL ) {
			pollwakeup( ulpp->rd_pollhdp,  POLLIN|POLLRDNORM );
			ulpp->ulpFlags &= ~ULPFLAG_R_POLL;
		}

		ulpp->rd_fpHdr_avail = b2hp->b2h_s;
		ulpp->rd_D2_avail = b2hp->b2h_l;
		vsema( &ulpp->rd_sema );

	}
	else { /* HIP_B2H_IN_DONE */

		ulpp->rd_count = b2hp->b2h_l;
		if ( (int)b2hp->b2h_l >= 0 && (b2hp->b2h_s & B2H_ISTAT_MORE) )
			ulpp->ulpFlags |= ULPFLAG_R_MORE;

		/* DMA complete.  Wake up process. */
		vsema( &ulpp->rd_dmadn );
	}
}




/***************************
 *  Flash Memory Routines  *
 ***************************/

#define FLASH_SIZE	0x00020000
#define ERASE_PLSCNT	3000
#define PGM_PLSCNT	25


/* Erase the Flash EEPROM.  This can take a while. */

static int
hippieraseflash( hippi_vars_t *hippi_devp )
{
	int	i,plscnt;
	volatile __uint32_t	*pio1, *b;
	evreg_t	old_pioreg1;

	old_pioreg1 = EV_GET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1) );

	pio1 = (volatile __uint32_t *) (io4hip_adaps[hippi_devp->unit].lwin +
			VMECC_PIOREG_MAPADDR(1) );

	/* First, program all bytes to 00H */

	for ( i=0; i<FLASH_SIZE; i++) {
		EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1),
				(i & 2) ? 0x3C03 : 0x3C02 );
		b = & pio1[ (i>>2) + ((i&1)<<20) ];

		plscnt = 0;
		do {
			*b = 0x40000000;	/* Set-up Program CMD*/
			*b = 0;			/* Write */
						/* wait 10 usecs */
			EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOTIMER, 32 );

			*b = 0xC0000000;	/* Program Verify CMD*/
						/* wait 6 usecs */
			EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOTIMER, 18 );

		} while ( ((*b)&0xFF000000) != 0 && ++plscnt < PGM_PLSCNT );

		if ( plscnt >= PGM_PLSCNT ) {
			pio1[ 0 ] = 0;
			pio1[ 0 ] = 0;
			EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1),
				old_pioreg1 );

			cmn_err( CE_WARN,
				"hippi%d: erase FAILED while zeroing flash\n",
				hippi_devp->unit );
			return -1;
		}
	}
	
	i=0;
	plscnt=0;
	do {
		/* Erase */
		EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1), 0x3C02 );
		pio1[ 0 ] = 0x20000000;
		pio1[ 0 ] = 0x20000000;
		drv_usecwait( 10000 );		/* wait at least 10ms */
		plscnt++;

		/* Erase Verify */
		while ( i<FLASH_SIZE ) {
			EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1),
					(i & 2) ? 0x3C03 : 0x3C02 );
			b = & pio1[ (i>>2) + ((i & 1)<<20) ];

			*b = 0xA0000000;		/* Erase Verify CMD */
							/* wait 6 usecs */
			EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOTIMER, 18 );
			if ( ((*b)>>24) != 0xFF )
				break;
			i++;
		}

	} while ( i < FLASH_SIZE && plscnt < ERASE_PLSCNT );

	pio1[ 0 ] = 0;
	pio1[ 0 ] = 0;
	EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1), old_pioreg1 );

	if ( plscnt >= ERASE_PLSCNT ) {
		cmn_err( CE_WARN,
			"hippi%d: EEPROM erase FAILED!\n", hippi_devp->unit );
		return -1;
	}
	else
		return 0;
}

/* Write the image into flash EEPROM.  Assumes flash is blank. */
static int
hippiwriteflash(hippi_vars_t *hippi_devp, u_char *image, u_int addr, int len)
{
	int	i, plscnt;
	volatile __uint32_t	*pio1, *b;
	__uint32_t	val;
	evreg_t	old_pioreg1;

	old_pioreg1 = EV_GET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1) );

	pio1 = (volatile __uint32_t *) ( io4hip_adaps[hippi_devp->unit].lwin +
			VMECC_PIOREG_MAPADDR(1) );

	for (i=addr; i<len+addr; i++) {
		EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1),
				(i & 2) ? 0x3C03 : 0x3C02 );
		b = & pio1[ (i>>2) + ((i&1)<<20) ];

		val = (__uint32_t)image[i]<<24;

		plscnt = 0;
		do {
			*b = 0x40000000;	/* Set-up Program CMD*/
			*b = val;		/* Write */
						/* wait 10 usecs */
			EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOTIMER, 32 );

			*b = 0xC0000000;	/* Program Verify CMD*/
						/* wait 6 usecs */
			EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOTIMER, 18 );

		} while ( ((*b)&0xFF000000) != val && ++plscnt < PGM_PLSCNT );

		if ( plscnt >= PGM_PLSCNT ) {
			cmn_err( CE_WARN, "hippi%d: flash write failed!\n",
				hippi_devp->unit );
			pio1[ 0 ] = 0;
			pio1[ 0 ] = 0;
			EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1),
				old_pioreg1 );
			return -1;
		}
	}

	pio1[ 0 ] = 0;
	pio1[ 0 ] = 0;
	EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1), old_pioreg1 );

	return 0;
}

/* Read the version number of the firmware.  */
static u_int
hippireadversion( hippi_vars_t *hippi_devp )
{
	volatile __uint32_t	*pio1;
	__uint32_t	v0, v1;
	evreg_t	old_pioreg1;

	old_pioreg1 = EV_GET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1) );

	pio1 = (volatile __uint32_t *) ( io4hip_adaps[hippi_devp->unit].lwin +
			VMECC_PIOREG_MAPADDR(1) );

	EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1), 0x3C02 );

	v1 = *(pio1+HIPPI_VERS_OFFS/sizeof(__uint32_t))&0xFF000000;
	v0 = *(pio1+(HIPPI_VERS_OFFS+0x00400000)/sizeof(__uint32_t))&0xFF000000;
	v1 |= ( v0 >> 8 );

	EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1), 0x3C03 );

	v0 = *(pio1+HIPPI_VERS_OFFS/sizeof(__uint32_t))&0xFF000000;
	v1 |= ( v0 >> 16 );
	v0 = *(pio1+(HIPPI_VERS_OFFS+0x00400000)/sizeof(__uint32_t))&0xFF000000;
	v1 |= ( v0 >> 24 );

	EV_SET_REG( hippi_devp->hi_swin + VMECC_PIOREG(1), old_pioreg1 );

	return v1;
}

