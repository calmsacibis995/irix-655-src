/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1996, Silicon Graphics, Inc                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/


#include "sys/types.h"
#include "sys/cpu.h"
#include "sys/systm.h"
#include "sys/cmn_err.h"
#include "sys/errno.h"
#include "sys/uio.h"
#include "sys/ioctl.h"
#include "sys/cred.h"
#include "sys/poll.h"
#include "sys/immu.h"
#include "ksys/ddmap.h"
#include "sys/invent.h"
#include "sys/debug.h"
#include "sys/sbd.h"
#include "sys/kmem.h"
#include "sys/edt.h"
#include "sys/dmamap.h"
#include "sys/hwgraph.h"
#include "sys/iobus.h"
#include "sys/iograph.h"
#include "sys/param.h"
#include "sys/pio.h"
#include "sys/sema.h"
#include "sys/ddi.h"
#include "sys/invent.h"
#include "sys/kabi.h"
#include "sys/mbuf.h"
#include "sys/PCI/PCI_defs.h"
#include "sys/PCI/pciio.h"
#include "sys/PCI/linc.h"
#include "sys/idbgentry.h"
#include "sys/hippi.h"
#include "sys/hps_ext.h"
#include "sys/hippibp_firm.h"
#include "sys/hippibp_priv.h"
#include "sys/hippibp.h"
#ifdef HIPPI_BP_DEBUG
#include "ksys/ddmap.h"
#define v_getpreg(VT) (VT)->v_preg
#endif
#define	HIPPIBP_UP(x)	((x)->hippibp_state & HIPPIBP_ST_CONFIGED)

/* 
 * hippibp.c
 * Driver for HIPPI bypass protocol. Somewhat dependent on underlying
 * HIPPI-Serial driver (hps). 
 */

/*
 * Notes on siamese-twin division.
 * (New owners, please read and remove.)
 *
 * 1. Tried to make a cleaner interface between the 2 so each has to
 *    know as little possible about each other. The underlying hw driver
 *    makes 4 hippibp_* calls:
 *	hippibp_attach()
 *	    called out of hps_attach(). Creates device nodes and inits
 *	    private data structures.
 *	hippibp_bd_down()
 *	    called by underlying driver when bringing board down.
 *	    Allows bp driver to do whatever cleanup it needs to before
 *	    yanking the cord.
 *	hippibp_bd_up()
 *	    called by underlying driver after board is brought up.
 *	hippibp_portint()
 *	    (Ugly, but couldn't avoid this one.) The hw driver will
 *	    manage the board-to-host queues. The anomaly is this single
 *	    BP msg type, so it will call hippibp_portint to process it.
 *
 *    These 4 routines will be stubs in the base hippi_s image shippied.
 *
 *    The hippibp driver should #include the base hippidev.h file, 
 *    which has prototypes for a few routines meant to be called by
 *    a trusted "friend" driver which has intimate knowledge of the
 *    underlying hardware. It is strongly recommended that the bp driver
 *    goes through these interface routines and does not make any
 *    assumptions about where the hw driver keeps things in its private
 *    data structures, as locations and names may change between releases.
 *
 * 2. I did not make any attempt to refine the locks as I did with
 *    the hw driver. Gave bp driver its own devsema for its own 
 *    state and structures, so wherever it used devsema in the
 *    old combined driver, it just uses devsema here.  (Tried to
 *    maintain the same names from the old combined driver wherever 
 *    possible.) However the HCMD area, which was also under devslock 
 *    protection in the old combined driver now requires new separate
 *    locks. There are 2 separate spinlocks to protect the source and 
 *    destination LINC HCMD areas. These shc_slock and dhc_slock are the
 *    only locks which the 2 drivers share. These locks are in the hps
 *    driver's own private data structure, and pointers to these locks
 *    are given to hippibp driver when it makes the hps_get_friendinfo()
 *    call.
 *
 * 3. In the old scheme, the user opened /dev/hippiN to do the
 *    HIPIOC_SET_BPCFG and HIPIOC_GET_BPSTATS ioctls, and /dev/hippibp<job>
 *    for actual bypass job operations. This same scheme is maintained, by
 *    having hps, during its own attach routine, create a "hippibp" device
 *    node, and calling the hippibp_attach() routine in this driver, which
 *    then creates additional job-specific device vertices.
 *    This allows the administrator to open the base hippibp device
 *    for config/stats and actual jobs to open the job-specific devices.
 *
 * 4. I kept everything as unchanged as possible from the old driver.
 *    Whenever I spotted anything that looked odd, I refrained from
 *    changing it but instead flagged it with a "XXX .... - ik." comment.
 *    Major changes I had to make to accomodate the new h/w were:
 *	a. hwgraph changes (new dev_t)
 *	b. split for twin-controller (distinct src and dst) architecture.
 *	c. pfns (ints) are given to firmware are now 64bit iopaddr_ts.
 *	   Should probably have changed the "pfn" field in struct freemap
 *	   to "iopaddr" but didn't, only changed its type. New owners may
 *	   want to go through and clean up all the "pfn" references as
 *	   they  are somewhat misleading.
 *
 * 5. Some bugs I noticed while porting the driver, and which I think 
 *    should be fixed:
 *    - The HIPPI_DSTWAIT/HIPPI_SRCWAIT calls should check for 
 *      return value and bail if <= 0.
 *    - The loops waiting for dma_status to clear are while(1) which
 *      is infinite if the fw has crashed with a DMA attention, leaving
 *      the flag active.
 */

#ifdef HIPPI_BP_DEBUG
int	hippibp_debug = 0;	/* Controls printing of debug info */
#define dprintf(lvl, x)	if (hippibp_debug>=lvl) printf x
#else
#define dprintf(lvl, x)
#endif	/* HIPPI_BP_DEBUG */

int	hippibpdevflag = D_MP;

/* Following fields point to a single page in the system which can be
 * used to "fill-in" unused entries in the various Hippi ByPass page maps.
 * This allows the Hippi Firmware to transfer to any "page index" in the
 * list without checking for a valid entry (i.e. all entries are "valid"
 * for transfer purposes).
 */

void	    *hippibp_garbage_addr;

/* =================================================================
 * Interface routines called by underlying hw driver
 */

/*
 * hippibp_attach()
 *
 * called out of hps_attach() with node of newly created "bypass"
 * vertex. Cookie is a token which allows hippibp to get access to
 * friend info from the hps driver. This routine creates a bunch
 * of device nodes: "ctl" control node, and individual number nodes
 * for jobs. The "ctl" device is returned to the called in *bpctl,
 * so that the hps driver can use it as argument to the bp routines
 * outlined in hippidev.h
 * 
 * Return value is 0 for SUCCESS, non-zero otherwise.
 */

int
hippibp_attach (vertex_hdl_t bpdir, vertex_hdl_t *bpctl, void * cookie)
{
    hippibp_soft_t  *bp, *newbp;
    hippibp_vars_t    *hp;
    hps_friend_info_t	finfo;
    int		    i;
    graph_error_t   rc;
    vertex_hdl_t    newdev;
    char	    name[32];

    rc = hwgraph_char_device_add(bpdir, "ctl", "hippibp", &newdev);
    if (rc != GRAPH_SUCCESS) {
	cmn_err(CE_ALERT,
		"hippibp_attach(): %v: Could not add device vertex for bypass control node: ret_code=%d, new_dev=%x\n", bpdir, rc, newdev);
	return -1;
    }
    ASSERT(newdev != GRAPH_VERTEX_NONE);
    *bpctl = newdev;

    if (hps_get_friendinfo (newdev, &finfo, cookie) == NULL)
	return -1;
    bp = kmem_zalloc (sizeof (hippibp_soft_t), KM_SLEEP);
    hp = kmem_zalloc (sizeof (hippibp_vars_t), KM_SLEEP);

    initnsema (&hp->devsema, 1, "hippibp_sema");

    hp->unit = finfo.unit;
    hp->shc_slock = finfo.shc_slock;
    hp->dhc_slock = finfo.dhc_slock;
    hp->shc_area = (volatile struct hip_bp_hc *)finfo.shc_area;
    hp->dhc_area = (volatile struct hip_bp_hc *)finfo.dhc_area;
    hp->sbufmem = finfo.sbufmem;
    hp->dbufmem = finfo.dbufmem;
    hp->cookie = cookie;
    hp->dcnctpt = finfo.dcnctpt;
    hp->scnctpt = finfo.scnctpt;

    bp->vtype = BPROOT;
    bp->hp = hp;

    hwgraph_fastinfo_set (newdev, (arbitrary_info_t) bp);

    /* XXX - should look into having the child nodes created upon 
     *	     a SET_BPCFG ioctl. And destroyed when configed down.
     */

    /*
     * Create child vertices named "0","1","2" as job-specific
     * device nodes.
     */
    for (i = 0; i < HIPPIBP_MAX_JOBS; i++) {
	sprintf (name, "%d", i);  
	dprintf(5,("hippibp_attach: adding device(%x,%s,\"hippibp\",%x)\n",
		     bpdir, name, &newdev));
	newdev = GRAPH_VERTEX_NONE;
	rc = hwgraph_char_device_add(bpdir, name, "hippibp", &newdev);
	dprintf (5, ("... return code = %d, newdev=%x\n", rc, newdev));
	if (rc != GRAPH_SUCCESS) {
	    cmn_err(CE_WARN,
	       "hippibp_attach: %v: Could not create device vertex for job %d, status=%d.\n",
			bpdir, i, rc);
	    continue;
	}
	ASSERT(newdev != GRAPH_VERTEX_NONE);

	hwgraph_chmod(newdev, HIPPIBP_PERM);

	newbp = kmem_zalloc (sizeof (hippibp_soft_t), KM_SLEEP);
	newbp->hp = bp->hp;
	newbp->vtype = BPJOB;
	newbp->jobid = i;
	hippibp_soft_set(newdev, newbp);
    }

    return 0;
}

/*
 * hippibp_bd_down()
 *
 * Called by underlying hw driver before config-ing board down or
 * resetting it for any reason.
 */
/* XXX: This routine was taken from the BP-specific code in the old
 *	combined driver's hippi_bd_shutdown() routine. Didn't see
 *	anything which looked like notification of bypass users.
 *	If there is any additional cleanup that should be done, please
 *	add it here. The underlying hw driver will wait for this routine
 *	to complete before proceding with rest of the shutdown. -- ik.
 */
void
hippibp_bd_down (vertex_hdl_t bpdev)
{
    hippibp_vars_t *hippi_devp;
    
    hippi_devp = hippibp_vars_get(bpdev);

    psema( & hippi_devp->devsema, PZERO );
    hippi_devp->hippibp_state &=~(HIPPIBP_ST_CONFIGED|HIPPIBP_ST_OPENED|
				  HIPPIBP_ST_UP);
    vsema( & hippi_devp->devsema );
}

/* Called by hps when board is configed up. Do whatever is needed to
 * set BP state vars for up state.
 */
void
hippibp_bd_up (vertex_hdl_t bpdev)
{
    hippibp_vars_t * hippi_devp = hippibp_vars_get(bpdev);

    psema( & hippi_devp->devsema, PZERO );
    hippi_devp->hippibp_state |= HIPPIBP_ST_UP;
    vsema( & hippi_devp->devsema );
}

void
hippibp_portint (vertex_hdl_t bpdev, short b2hs, int b2hl)
{
    hippibp_vars_t *hippi_devp;
    struct hippi_port *port;
    int signo, s;

/* XXX any locks needed for this to make sure that user hasn't gone away? */
    hippi_devp = hippibp_vars_get(bpdev);
    port = &hippi_devp->hippibp_portids[b2hs];
    signo = port->signo;

    if (port->pid)
	pid_signal(port->pid, signo, 0, 0);

    s = mutex_spinlock(hippi_devp->dhc_slock);
    HIPPI_DSTWAIT (hippi_devp->cookie);

#ifdef LINC1_MBOX_WAR
    /* LINC 0 mbox/odd-word bug workaround: must write this a single
     * long word. */
    * (__uint64_t *) &hippi_devp->dhc_area->arg.bp_portint_ack.portid =
      				((__uint64_t) b2hs << 32) | b2hl;
#else
    hippi_devp->dhc_area->arg.bp_portint_ack.portid = b2hs;
    hippi_devp->dhc_area->arg.bp_portint_ack.cookie = b2hl;
#endif

    HIPPI_DSTHWOP(HCMD_BP_PORTINT_ACK, hippi_devp->cookie);
    mutex_spinunlock(hippi_devp->dhc_slock, s);
}

/* ===================================================================
 *	private idbg routines
 */

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

/* Sorry fellas, no arrays any more. Can only dump for a given vertex. */
static void
hippibp_idbg(vertex_hdl_t vhdl)
{
    hippibp_soft_t	*bp;
    hippibp_vars_t	*hippi_devp;
    struct hippi_bp_job *bpjob;
    int	    job;

    qprintf("hippibp_idbg: called with vertex 0x%x\n", vhdl);
    if (vhdl == NULL)
	return;

    if ((bp = hippibp_soft_get (vhdl)) == NULL) {
	qprintf("hippibp_idbg: no fastinfo available.\n");
	return;
    }

    hippi_devp = bp->hp;

    qprintf("hippi unit %d  ulp 0x%x maxjobs %d maxportidp %d\n",
	    hippi_devp->unit, hippi_devp->bp_ulp,
	    hippi_devp->bp_maxjobs, hippi_devp->bp_maxportids);
    qprintf("maxdfmpgs %d maxsfmpgs %d maxddqpgs %d\n",
	    hippi_devp->bp_maxdfmpgs, hippi_devp->bp_maxsfmpgs,
	    hippi_devp->bp_maxddqpgs);
    for (job=0; job < hippi_devp->bp_maxjobs; job++) {
	bpjob = &hippi_devp->bp_jobs[job];
	qprintf("hippi unit %d job %d\n", hippi_devp->unit, job);
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

/* ====================================================================
 *		    DRIVER PRIVATE ROUTINES.
 */
/* 
 * _hippibpinit: NOT the os-called driver init routine.
 * 
 * This is called from HIPIOC_SET_BPCFG ioctl, under protection of
 * hippi_dev->devsema.May be called more than once (i.e. whenever
 * firmware reloaded).
 *
 * Read firmware configuration information and caches it in the device
 * table.  Setup bp_jobs structure if not already allocated.
 */
int
_hippibpinit( hippibp_vars_t *hippi_devp )
{
    volatile __uint32_t *FWinfo;
    __uint32_t *cachedInfo;
    int i, s;
    char *FWbaseaddr;

    /* Need to wait for preceeding HCMD_INIT to finish (may have just
     * reset the board), since FWinfo not setup until that command
     * completes.
     * NOTE: Board may take a long time and we may timeout. Return
     * EBUSY to "hipcntl" so it can goto sleep and retry later.
     */

    /* First, get src controller's configuration */
    s = mutex_spinlock( hippi_devp->shc_slock );
    if (HIPPI_SRCWAIT (hippi_devp->cookie) <= 0) {
	mutex_spinunlock( hippi_devp->shc_slock, s );
	return EBUSY;
    }
    hippi_devp->src_bp_fw_conf =(volatile struct hip_bp_fw_config *)
	        ((char *)&hippi_devp->shc_area[1]+sizeof(struct hippi_stats));

    /* Copy all configuration info from firmware into controller device
     * table for cached access (only field which is dynamic is the
     * dma_status).
     */
    FWinfo = (volatile __uint32_t *) hippi_devp->src_bp_fw_conf;
    cachedInfo = (__uint32_t *)&hippi_devp->cached_src_bp_fw_conf;
    for (i=0; i< (sizeof(struct hip_bp_fw_config)/sizeof(__uint32_t));i++)
	cachedInfo[i] = FWinfo[i];

    mutex_spinunlock( hippi_devp->shc_slock, s );

    /* All addresses supplied by fw are offsets relative to hc area. */
    FWbaseaddr = (char *)hippi_devp->shc_area;

    hippi_devp->src_bp_ifield = (volatile __uint32_t *)
	(FWbaseaddr + hippi_devp->cached_src_bp_fw_conf.hostx_base);

    hippi_devp->src_bp_stats = (volatile __uint64_t *)
	(FWbaseaddr + hippi_devp->cached_src_bp_fw_conf.bpstat_base);

    hippi_devp->src_bp_sdhead = (volatile char *)
	(FWbaseaddr + hippi_devp->cached_src_bp_fw_conf.bpjob_base);

    hippi_devp->src_bp_sdqueue = (volatile char *)
	(FWbaseaddr + hippi_devp->cached_src_bp_fw_conf.sdq_base);

    hippi_devp->src_bp_sfreemap = (volatile iopaddr_t *)
	(FWbaseaddr + hippi_devp->cached_src_bp_fw_conf.sfm_base);

    /* Repeat for dst controller's configuration */
    s = mutex_spinlock( hippi_devp->dhc_slock );
    if (HIPPI_DSTWAIT (hippi_devp->cookie) <= 0) {
	mutex_spinunlock( hippi_devp->dhc_slock, s );
	return EBUSY;
    }
    hippi_devp->dst_bp_fw_conf =(volatile struct hip_bp_fw_config *)
	        ((char *)&hippi_devp->dhc_area[1]+sizeof(struct hippi_stats));

    /* Copy all configuration info from firmware into controller device
     * table for cached access (only field which is dynamic is the
     * dma_status).
     */
    FWinfo = (volatile __uint32_t *) hippi_devp->dst_bp_fw_conf;
    cachedInfo = (__uint32_t *)&hippi_devp->cached_dst_bp_fw_conf;
    for (i=0; i< (sizeof(struct hip_bp_fw_config)/sizeof(__uint32_t));i++)
	cachedInfo[i] = FWinfo[i];

    mutex_spinunlock( hippi_devp->dhc_slock, s );

    /* All addresses supplied by fw are offsets relative to hc area. */
    FWbaseaddr = (char *)hippi_devp->dhc_area;

    /* XXX Jim says dfl_base is unused. */
    hippi_devp->dst_bp_dfreelist =  (volatile char *)
	(FWbaseaddr + hippi_devp->cached_dst_bp_fw_conf.dfl_base);

    hippi_devp->dst_bp_dfreemap = (volatile iopaddr_t *)
	(FWbaseaddr + hippi_devp->cached_dst_bp_fw_conf.dfm_base);

    hippi_devp->dst_bp_stats = (volatile __uint64_t *)
	(FWbaseaddr + hippi_devp->cached_dst_bp_fw_conf.bpstat_base);

    dprintf(1, ("hippibpinit: sfreemap 0x%x ifield 0x%x\n",
		hippi_devp->src_bp_sfreemap, hippi_devp->src_bp_ifield));
    dprintf(1, ("hippibpinit: dfreemap 0x%x dfreelist 0x%x\n",
		hippi_devp->dst_bp_dfreemap, hippi_devp->dst_bp_dfreelist));
    dprintf(1, ("hippibpinit: sdqueue 0x%x sdhead 0x%x\n",
		hippi_devp->src_bp_sdqueue, hippi_devp->src_bp_sdhead));

    dprintf(1, ("hippibpinit: num_jobs %d num_ports %d\n",
		hippi_devp->cached_src_bp_fw_conf.num_jobs,
		hippi_devp->cached_dst_bp_fw_conf.num_ports));
    dprintf(1, ("hippibpinit: hostx base 0x%x size 0x%x\n",
		hippi_devp->cached_src_bp_fw_conf.hostx_base,
		hippi_devp->cached_src_bp_fw_conf.hostx_size));
    dprintf(1, ("hippibpinit: dfl base 0x%x size 0x%x\n",
		hippi_devp->cached_dst_bp_fw_conf.dfl_base,
		hippi_devp->cached_dst_bp_fw_conf.dfl_size));
    dprintf(1, ("hippibpinit: sfm base 0x%x size 0x%x\n",
		hippi_devp->cached_src_bp_fw_conf.sfm_base,
		hippi_devp->cached_src_bp_fw_conf.sfm_size));
    dprintf(1, ("hippibpinit: dfm base 0x%x size 0x%x\n",
		hippi_devp->cached_dst_bp_fw_conf.dfm_base,
		hippi_devp->cached_dst_bp_fw_conf.dfm_size));
    dprintf(1, ("hippibpinit: src bpstat base 0x%x size 0x%x\n",
		hippi_devp->cached_src_bp_fw_conf.bpstat_base,
		hippi_devp->cached_src_bp_fw_conf.bpstat_size));
    dprintf(1, ("hippibpinit: dst bpstat base 0x%x size 0x%x\n",
		hippi_devp->cached_dst_bp_fw_conf.bpstat_base,
		hippi_devp->cached_dst_bp_fw_conf.bpstat_size));
    dprintf(1, ("hippibpinit: sdq base 0x%x size 0x%x\n",
		hippi_devp->cached_src_bp_fw_conf.sdq_base,
		hippi_devp->cached_src_bp_fw_conf.sdq_size));
    dprintf(1, ("hippibpinit: bpjob base 0x%x size 0x%x\n",
		hippi_devp->cached_src_bp_fw_conf.bpjob_base,
		hippi_devp->cached_src_bp_fw_conf.bpjob_size));

    if (hippi_devp->bp_jobs == 0) {
	idbg_addfunc("hippibp", hippibp_idbg);
	hippi_devp->bp_jobs =
		kmem_zalloc(HIPPIBP_MAX_JOBS*
			    sizeof(struct hippi_bp_job), KM_NOSLEEP);
    }

    return(0);
}

/*
 * hippibp_config_driver( hippibp_vars_t *hippi_devp)
 * Called out of SET_BPCFG ioctl after we've finished configuring the
 * board.
 */
int
hippibp_config_driver( hippibp_vars_t *hippi_devp)
{
    int job, i, pgs;
    struct hippi_bp_job *bpjob;
    volatile iopaddr_t *FWbase;

    if (hippibp_garbage_addr == 0) {
	if ((hippibp_garbage_addr = kvpalloc(1,0,0)) == NULL)
	    return ENOMEM;
	dprintf(1,("hippibp_config_driver: allocate gpage 0x%x\n",
		    hippibp_garbage_addr));

    }
    /* Get DMA translated address of garbage page relative to both
     * src and dst linc. These are probably the same.
     */
    if (hippi_devp->garbage_src_iopaddr == NULL) {
	hippi_devp->garbage_src_iopaddr =
	    pciio_dmatrans_addr(hippi_devp->scnctpt, 0, 
				kvtophys((caddr_t)hippibp_garbage_addr),
				NBPP,
				PCIIO_DMA_A64 | PCIIO_DMA_DATA);
	ASSERT(	hippi_devp->garbage_src_iopaddr != NULL);
	dprintf(1,("hippibp_config_driver(unit %d): src garbage page 0x%x\n",
		    hippi_devp->unit,
		    hippi_devp->garbage_src_iopaddr));
    }

    if (hippi_devp->garbage_dst_iopaddr == NULL) {
	hippi_devp->garbage_dst_iopaddr =
	    pciio_dmatrans_addr(hippi_devp->dcnctpt, 0, 
				kvtophys((caddr_t)hippibp_garbage_addr),
				NBPP,
				PCIIO_DMA_A64 | PCIIO_DMA_DATA);
	ASSERT(	hippi_devp->garbage_dst_iopaddr != NULL);
	dprintf(1,("hippibp_config_driver(unit %d): dst garbage page 0x%x\n",
		    hippi_devp->unit,
		    hippi_devp->garbage_dst_iopaddr));
    }

    for (job = 0; job < hippi_devp->cached_src_bp_fw_conf.num_jobs; job++) {

	/* Initialize dst freemap with garbage page iopaddr */
	pgs = hippi_devp->cached_dst_bp_fw_conf.dfm_size/sizeof(iopaddr_t);
	FWbase = hippi_devp->dst_bp_dfreemap + job * pgs;
	for (i=0; i < pgs; i++)
	    FWbase[i] = hippi_devp->garbage_dst_iopaddr;

	/* Initialize src freemap with garbage page iopaddr */
	pgs = hippi_devp->cached_src_bp_fw_conf.sfm_size/sizeof(iopaddr_t);
	FWbase = hippi_devp->src_bp_sfreemap + job * pgs;
	for (i=0; i < pgs; i++)
	    FWbase[i] = hippi_devp->garbage_src_iopaddr;

	/*
	 * Software (driver) re-initialization
	 */

	if (job >= HIPPIBP_MAX_JOBS)
	    continue;

	bpjob = &hippi_devp->bp_jobs[job];
	bpjob->portused = 0;
	bpjob->hippibp_DFlistPage =
			hippi_devp->dst_bp_dfreelist +
			job * hippi_devp->cached_dst_bp_fw_conf.dfl_size;
	
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
    hippi_devp->hippibp_portmax = hippi_devp->bp_maxportids;

    return 0;
}

/* First time job is ever opened, allocate page to contain
 * list of physical page numbers for freemap.
 *
 * NOTE: We use MAXEVERpgs which gives the largest limits that
 * any HIPIOC_SET_BPCFG command could ever set, since we only
 * perform this allocation once per system boot (we could fix
 * this to deallocate before new CFG and then use maxpgs).
 */

void hippibp_freemap_pfn_alloc(struct hippi_freemap *freemap,
			       int maxpgs,
			       iopaddr_t    garbage_iopaddr)
{
    int i;

    if (freemap->pfns == (iopaddr_t *)0) {
	freemap->pfns = kmem_alloc( maxpgs*sizeof(iopaddr_t), KM_SLEEP );

    /* Initialize freemap physical page list to point
     * to garbage page.
     */

    for (i=0; i<maxpgs; i++)
	freemap->pfns[i] = garbage_iopaddr;

    dprintf(1,("hippibp_freemap_pfn_alloc: pfn array 0x%x : 0x%x\n",
		freemap->pfns, maxpgs*sizeof(iopaddr_t)));
    }
}

void
hippifreemap_release ( struct hippi_freemap *freemap, int recoverpages )
{
	if (freemap->ID)
		cmn_err(CE_WARN,"hippifreemap_release2: ID still set!\n");

	if (recoverpages) {
		kvpfree(freemap->vaddr, freemap->pgcnt);
	} else
		cmn_err(CE_WARN,"hippifreemap_release: LOSE %d pps at 0x%x\n",
			freemap->pgcnt, freemap->vaddr);

	freemap->vaddr = 0;
	freemap->pgcnt = 0;
}

/* This routine waits until the we're sure that the HIPPI firmware is not
 * actively performing DMA to the page we're trying to unpin.
 * 
 * dma_status is address in SRC or DST LINC SDRAM, depending on whether
 * the map is src or dst freemap.
 */
void
hippibp_teardown_wait(volatile __uint32_t *dma_status,
		      struct hippi_io *hippimap,
		      int	job)
{
    volatile __uint32_t	status;
    __uint32_t		retval;
    int	pgidx;

/* XXX - this looks like a very dangerous potential infinite loop
 *       if the fw crashes with dma_status stuck active. -- ik.
 */
    while (1) {
	status = *dma_status;

	dprintf(1,("hippibp_teardown_wait(%d): dma_status 0x%x\n",
		   job, status));
	if (!((status >> HIPPIBP_DMA_ACTIVE_SHIFT) & HIPPIBP_DMA_ACTIVE_MASK))
	    break;	/* no DMA currently active */

	if (((status >> HIPPIBP_DMA_JOB_SHIFT) & HIPPIBP_DMA_JOB_MASK) != job)
	    break;	/* active DMA for other job */

	if (hippimap->mapselect == HIPPIBP_DFM_SEL) {
	    if (((status >> HIPPIBP_DMA_CLIENT_SHIFT) &
		HIPPIBP_DMA_CLIENT_MASK) != HIPPIBP_DMA_CLIENT_DFM)
		break;	/* active DMA for other list */
	} else if (hippimap->mapselect == HIPPIBP_SFM_SEL) {
	    if (((status >> HIPPIBP_DMA_CLIENT_SHIFT) &
		HIPPIBP_DMA_CLIENT_MASK) != HIPPIBP_DMA_CLIENT_SFM)
		break;	/* active DMA for other list */
	} else if (hippimap->mapselect == HIPPIBP_DFM_AND_SFM_SEL) {
		retval = (status >> HIPPIBP_DMA_CLIENT_SHIFT) &
				HIPPIBP_DMA_CLIENT_MASK;
		if(retval != HIPPIBP_DMA_CLIENT_SFM &&
			retval != HIPPIBP_DMA_CLIENT_DFM)
			break;  /* active DMA for other list */
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
hippibp_iomap_teardown( hippibp_soft_t *bp, int idx)
{
    hippibp_vars_t *hippi_devp = bp->hp;
    struct hippi_bp_job *bpjob;
    volatile iopaddr_t *FWbase;
    int j, job;

    job = bp->jobid;
    bpjob = &hippi_devp->bp_jobs[job];

    /* below, the "&" instead of "==", together with if followed by
     * if (w/o an else) allows SFM_AND_DFM_SEL to work (both code
     * paths executed)
     */
    if (bpjob->hipiomap[idx].mapselect & HIPPIBP_DFM_SEL) {	/* dfreemap */
	FWbase = hippi_devp->dst_bp_dfreemap + job *
		(hippi_devp->cached_dst_bp_fw_conf.dfm_size/sizeof(iopaddr_t));
	for (j = bpjob->hipiomap[idx].startindex;
	     j <= bpjob->hipiomap[idx].endindex; j++) {
	     bpjob->Dfreemap.pfns[j] =
	     FWbase[j] = hippi_devp->garbage_dst_iopaddr;
	}
	hippibp_teardown_wait( &hippi_devp->dst_bp_fw_conf->dma_status,
			       &bpjob->hipiomap[idx], job);
    }

    if (bpjob->hipiomap[idx].mapselect & HIPPIBP_SFM_SEL) { /* sfreemap */
	FWbase = hippi_devp->src_bp_sfreemap + job * 
		(hippi_devp->cached_src_bp_fw_conf.sfm_size/sizeof(iopaddr_t));
	for (j = bpjob->hipiomap[idx].startindex;
	     j <= bpjob->hipiomap[idx].endindex; j++) {
	    bpjob->Sfreemap.pfns[j] =
	    FWbase[j] = hippi_devp->garbage_src_iopaddr;
	}
	hippibp_teardown_wait( &hippi_devp->src_bp_fw_conf->dma_status,
			       &bpjob->hipiomap[idx], job);

    }

    fast_undma( (void *)bpjob->hipiomap[idx].uaddr,
		bpjob->hipiomap[idx].ulen,
		bpjob->hipiomap[idx].uflags | B_READ,
		&bpjob->hipiomap[idx].dmacookie );

    /* now reclaim entry for future use */

    bpjob->hipiomap_cnt--;
    bpjob->hipiomap[idx].mapselect = 0;

    dprintf(1,("hippi_teardown(%d:%d)(%d %d %d): vaddr 0x%x len 0x%x \n",
	      hippi_devp->unit,job,idx+1, bpjob->hipiomap[idx].startindex,
	      bpjob->hipiomap[idx].endindex,
	      bpjob->hipiomap[idx].uaddr, bpjob->hipiomap[idx].ulen));
}

void
hippibp_exit_callback_complete( hippi_bp_job_t *bpjob)
{
	ulong_t curpid = 0;
	int i;

	drv_getparm(PPID, &curpid); /* get pid of current process */

	dprintf(1,("hippibp_exit_callback_complete: bpjob 0x%x curpid %d max 0x%x\n",
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
hippibp_exit(hippibp_soft_t *bp)
{
	int job, i;
	hippibp_vars_t *hippi_devp;
	hippi_bp_job_t *bpjob;
	ulong_t curpid = 0;

	job = bp->jobid;
	hippi_devp = bp->hp;
	drv_getparm(PPID, &curpid); /* get pid of current process */

	dprintf(1,("hippibp_exit(%d:%d): curpid %d\n",
		hippi_devp->unit, job, curpid));

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
		psema( &hippi_devp->devsema, PZERO );
		for (i=0; i<HIPPIBP_MAX_IOSETUP; i++) {
			if ((bpjob->hipiomap[i].mapselect) &&
			    (bpjob->hipiomap[i].pid == curpid)) {
				hippibp_iomap_teardown( bp, i );
				dprintf(1,("hippi_exit(%d:%d)(%d %d %d): vaddr 0x%x len 0x%x \n",
					hippi_devp->unit,job,i+1,
					bpjob->hipiomap[i].startindex,
					bpjob->hipiomap[i].endindex,
					bpjob->hipiomap[i].uaddr,
					bpjob->hipiomap[i].ulen));
			}
		}
		vsema( &hippi_devp->devsema );
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
hippibp_add_exit_callback( hippi_bp_job_t *bpjob, int unit, int job,
			    hippibp_soft_t *bp )
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

	status = add_exit_callback(curpid, 0,
				   (void(*)(void *))hippibp_exit, 
				   (void *)bp);

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


/* ====================================================================
 *			DRIVER ENTRY POINTS
 */
/* ARGSUSED */
int
hippibpopen(dev_t *devp, int oflag, int otyp, cred_t *crp)
{
    vertex_hdl_t	vhdl = dev_to_vhdl(*devp);
    vertex_hdl_t	newvhdl;
    hippibp_soft_t	*bp, *newbp;
    hippibp_vars_t	*hippi_devp;
    graph_error_t	rc;
    int			job;
#ifdef HIPPI_BP_DEBUG
    char		buf[256];
#endif

    bp = hippibp_soft_get (vhdl);
    hippi_devp = bp->hp;

    dprintf (5, ("hippibpopen: %s\n", vertex_to_name(vhdl, buf, sizeof(buf))));

    switch (bp->vtype) {

    case BPROOT:
	if (bp->isclone)
	    return EBUSY;

	/* Create new vertex for this open. */
	rc = hwgraph_vertex_create(&newvhdl);
	if (rc != GRAPH_SUCCESS)
	    return ENOMEM;
	ASSERT (newvhdl != NULL);

	/* Associate cloned vertex with this driver, without
	 * having it show up in hwgraph */
	hwgraph_char_device_add(newvhdl, NULL, "hippibp", NULL);

	newbp = kmem_zalloc (sizeof (hippibp_soft_t), KM_SLEEP);
	newbp->hp = hippi_devp;
	newbp->vtype = BPROOT;
	newbp->isclone = 1;
	hippibp_soft_set(newvhdl, newbp);

	/* XXX - is there any reason to keep a pointer to newvhdl?
	 * this vertex is just used for cfg and stat. Nothing asynchronous.
	 */

	*devp = vhdl_to_dev(newvhdl);
	dprintf(1,("hippibpopen(%d): flag = 0x%x\n", hippi_devp->unit, oflag));
	return 0;

    case BPJOB:
	/* Only one open allowed per job vertex, so no new vertex created. */
	psema( & hippi_devp->devsema, PZERO );

	/* Verify that job number is valid for this system and that
	 * the bp_jobs structure has been allocated by the sysadmin
	 * via a SET_BPCFG ioctl call.
	 */
	job = bp->jobid;

	if ((job >= HIPPIBP_MAX_JOBS) ||
	    ((hippi_devp->hippibp_state & HIPPIBP_ST_CONFIGED) == 0) ||
	    (job >= hippi_devp->cached_src_bp_fw_conf.num_jobs) ||
	    (job >= hippi_devp->bp_maxjobs)) {
		vsema( & hippi_devp->devsema );
		return ENXIO;
	}

	if (hippi_devp->bp_jobs[job].jobFlags & JOBFLAG_EXCL) {
		vsema( & hippi_devp->devsema );
		return EBUSY;
	}

	if ((oflag & FEXCL) && hippi_devp->bp_jobs[job].jobFlags) {
		vsema( & hippi_devp->devsema );
		return EBUSY;
	}

	/* XXX: array folks,
	 *  - looks like this flag HIPPIBP_ST_OPENED flag never
	 *    gets turned off except at config-down. Shouldn't 
	 *    this be a counter of number of jobs open, decremented
	 *    at each job close? -- ik.
	 */
	hippi_devp->hippibp_state |= HIPPIBP_ST_OPENED;
	hippi_devp->bp_jobs[job].jobFlags = JOBFLAG_OPEN;
	if (oflag & FEXCL)
		hippi_devp->bp_jobs[job].jobFlags |= JOBFLAG_EXCL;

	vsema( & hippi_devp->devsema );

	dprintf(1,( "hippibpopen(%d:%d): flag = 0x%x\n",
		hippi_devp->unit, job, oflag ));
	return 0;

    default:
	/* No way. */
	cmn_err(CE_WARN, 
		"hippibpopen: %v: unknown vertex type (%d)\n",
		 vhdl, bp->vtype);
	return ENODEV;
    }
}

/* ARGSUSED */
int
hippibpclose(dev_t dev, int oflag, int otyp, cred_t *crp)
{
    vertex_hdl_t	vhdl = dev_to_vhdl(dev);
    hippibp_soft_t	*bp;
    hippibp_vars_t	*hippi_devp;
    int i, job, s, recoverpgs=1;
    struct hippi_bp_job *bpjob;
    __uint32_t	*dma_status, status;

    bp = hippibp_soft_get (vhdl);

    switch (bp->vtype) {

    case BPROOT:
	ASSERT (bp->isclone != 0);
	kmem_free (bp, sizeof (hippibp_soft_t));
	hwgraph_vertex_destroy(vhdl);
	return 0;

    case BPJOB:
	hippi_devp = bp->hp;
	job = bp->jobid;
	bpjob = &hippi_devp->bp_jobs[job];

	/* First we issue a "disable job" command to the FW. 
	 * (To both the src and dst controllers.)
	 * This should cause the FW to stop transmitting and receiving
	 * for this job, except for a (single) DMA "in-progress"
	 */
	psema( & hippi_devp->devsema, PZERO );
	hippi_devp->bp_jobs[job].jobFlags &= ~JOBFLAG_OPEN;

	if (HIPPIBP_UP(hippi_devp)) {
	    volatile iopaddr_t *FWbase;
	    int j;

	    s = mutex_spinlock( hippi_devp->shc_slock );
	    HIPPI_SRCWAIT (hippi_devp->cookie);
#ifdef LINC1_MBOX_WAR
	    /* LINC 1.0 mbox/odd-word bug says we can't do single
	       word accesses to odd words. */
	    * (__uint64_t *) &hippi_devp->shc_area->arg.bp_job.enable
	      		=  (__uint64_t) job;
	    * (__uint64_t *) &hippi_devp->shc_area->arg.bp_job.fm_entry_size
	      		= ((__uint64_t) NBPP) << 32;
	    * (__uint64_t *) &hippi_devp->shc_area->arg.bp_job.auth[1] = 0;
	    * (__uint64_t *) &hippi_devp->shc_area->arg.bp_job.ack_host = 0;
#else
	    hippi_devp->shc_area->arg.bp_job.enable = 0;
	    hippi_devp->shc_area->arg.bp_job.job    = job;
	    hippi_devp->shc_area->arg.bp_job.fm_entry_size = NBPP;
	    hippi_devp->shc_area->arg.bp_job.ack_host = 0;
	    hippi_devp->shc_area->arg.bp_job.ack_port = 0;
	    hippi_devp->shc_area->arg.bp_job.auth[0] = 0;
	    hippi_devp->shc_area->arg.bp_job.auth[1] = 0;
	    hippi_devp->shc_area->arg.bp_job.auth[2] = 0;
#endif

	    HIPPI_SRCHWOP (HCMD_BP_JOB, hippi_devp->cookie);
	    mutex_spinunlock( hippi_devp->shc_slock, s );

	    dprintf(1,("hippibpclose(BP %d:%d) job disable sent to src FW\n",
			hippi_devp->unit,job));

	    /* One more time, to the other side. */
	    s = mutex_spinlock( hippi_devp->dhc_slock );
	    HIPPI_DSTWAIT (hippi_devp->cookie);
#ifdef LINC1_MBOX_WAR
	    /* LINC 1.0 mbox/odd-word bug says we can't do single
	       word accesses to odd words. */
	    * (__uint64_t *) &hippi_devp->dhc_area->arg.bp_job.enable
	      		=  (__uint64_t) job;
	    * (__uint64_t *) &hippi_devp->dhc_area->arg.bp_job.fm_entry_size
	      		= ((__uint64_t) NBPP) << 32;
	    * (__uint64_t *) &hippi_devp->dhc_area->arg.bp_job.auth[1] = 0;
	    * (__uint64_t *) &hippi_devp->dhc_area->arg.bp_job.ack_host = 0;
#else
	    hippi_devp->dhc_area->arg.bp_job.enable = 0;
	    hippi_devp->dhc_area->arg.bp_job.job    = job;
	    hippi_devp->dhc_area->arg.bp_job.fm_entry_size = NBPP;
	    hippi_devp->dhc_area->arg.bp_job.ack_host = 0;
	    hippi_devp->dhc_area->arg.bp_job.ack_port = 0;
	    hippi_devp->dhc_area->arg.bp_job.auth[0] = 0;
	    hippi_devp->dhc_area->arg.bp_job.auth[1] = 0;
	    hippi_devp->dhc_area->arg.bp_job.auth[2] = 0;
#endif
	    HIPPI_DSTHWOP (HCMD_BP_JOB, hippi_devp->cookie);
	    mutex_spinunlock( hippi_devp->dhc_slock, s );

	    dprintf(1,("hippibpclose(BP %d:%d) job disable sent to dst FW\n",
			hippi_devp->unit,job));

	    /* Make sure all FW accessible pages point to the
	     * "garbage page".  First we handle the pages setup by
	     * calls to hippimap().
	     */
	    FWbase = hippi_devp->dst_bp_dfreemap + job *
		(hippi_devp->cached_dst_bp_fw_conf.dfm_size/sizeof(iopaddr_t));
	    for (i=0; i < bpjob->Dfreemap.pgcnt; i++)
		FWbase[i] = bpjob->Dfreemap.pfns[i] = 
			    hippi_devp->garbage_dst_iopaddr;

	    FWbase = hippi_devp->src_bp_sfreemap + job *
		(hippi_devp->cached_src_bp_fw_conf.sfm_size/sizeof(iopaddr_t));
	    for (i=0; i < bpjob->Sfreemap.pgcnt; i++)
		FWbase[i] = bpjob->Sfreemap.pfns[i] =
			    hippi_devp->garbage_src_iopaddr;

	    /* Now we handle cleanup of map entries which were setup
	     * by HIPIOC_SETUP_BPIO and which were not torn down by
	     * calls to HIPIOC_TEARDOWN_BPIO.
	     */

	    if (bpjob->hipiomap_cnt)
		for (i=0; i<HIPPIBP_MAX_IOSETUP; i++) {
		    if (bpjob->hipiomap[i].mapselect & HIPPIBP_DFM_SEL) {
			/* dfreemap */
			FWbase = hippi_devp->dst_bp_dfreemap + job *
				(hippi_devp->cached_dst_bp_fw_conf.dfm_size/sizeof(iopaddr_t));
			for (j=bpjob->hipiomap[i].startindex;
				j<=bpjob->hipiomap[i].endindex; j++)
			    bpjob->Dfreemap.pfns[j] = FWbase[j] = 
					hippi_devp->garbage_dst_iopaddr;
		    }

		    if (bpjob->hipiomap[i].mapselect & HIPPIBP_SFM_SEL){
			/* sfreemap */
			FWbase = hippi_devp->src_bp_sfreemap + job *
				(hippi_devp->cached_src_bp_fw_conf.sfm_size/sizeof(iopaddr_t));
			for (j=bpjob->hipiomap[i].startindex;
				j<=bpjob->hipiomap[i].endindex; j++)
			    bpjob->Sfreemap.pfns[j] = FWbase[j] = 
					hippi_devp->garbage_src_iopaddr;
		    }
		}    /* for each of HIPPIBP_MAX_IOSETUP entries */
	} else /* not if (HIPPIBP_UP(hippi_devp)) */
	    goto recover_iomaps;

	for (i=0; i<hippi_devp->hippibp_portmax; i++) {
	    if (hippi_devp->hippibp_portids[i].jobid == job) {

		/* Tell Hippi FW that this port is inactive.
		 * This goes to dest-side controller only. */

		s = mutex_spinlock( hippi_devp->dhc_slock );
		HIPPI_DSTWAIT (hippi_devp->cookie);
#ifdef LINC1_MBOX_WAR
		* (__uint64_t*) &hippi_devp->dhc_area->arg.bp_port.ux
		  	= (((__uint64_t)HIP_BP_PORT_DISABLE) << 60) | job;

		* (__uint64_t*) &hippi_devp->dhc_area->arg.bp_port.port
		  	= ((__uint64_t) i ) << 32;

		* (__uint64_t*) &hippi_devp->dhc_area->arg.bp_port.ddq_lo = 0;

		* (__uint64_t*) &hippi_devp->dhc_area->res.cmd_res[0] =
						(uint64_t)0xdeadbeef << 32;
#else
		/* OPCODE must be written as word, so kludge it
		   hippi_devp->hi_hc->arg.bp_port.ux.s.opcode =
			HIP_BP_PORT_DISABLE;
		*/
		hippi_devp->dhc_area->arg.bp_port.ux.i =
						    HIP_BP_PORT_DISABLE << 28;
		hippi_devp->dhc_area->arg.bp_port.job  = job;
		hippi_devp->dhc_area->arg.bp_port.port   = i;
		hippi_devp->dhc_area->arg.bp_port.ddq_hi = 0;
		hippi_devp->dhc_area->arg.bp_port.ddq_lo = 0;

		hippi_devp->dhc_area->res.cmd_res[0] = 0xdeadbeef;
#endif
		HIPPI_DSTHWOP (HCMD_BP_PORT, hippi_devp->cookie);
		mutex_spinunlock( hippi_devp->dhc_slock, s );

		/* Check the return code to see if port is free
		 * or if the FW has a DMA "in-progress".
		 */

		dprintf(1,("hippibpclose(%d:%d): disable port %d\n",

			hippi_devp->unit,job,i));

		hippi_devp->hippibp_portids[i].jobid = (char)-1;
		hippi_devp->bp_jobs[job].portused--;

	    }
	}

	/* wait for all DMA to finish for this job */
/*
 * XXX - These look like very dangerous potential infinite loops
 *       if the fw crashes with dma_status stuck active. -- ik.
 */
	/* First src side. */
	dma_status = (__uint32_t *)&hippi_devp->src_bp_fw_conf->dma_status;
	while (1) {
		status = *dma_status;
		if (!((status >> HIPPIBP_DMA_ACTIVE_SHIFT) &
			HIPPIBP_DMA_ACTIVE_MASK))
			break;	/* no DMA currently active */

		if (((status >> HIPPIBP_DMA_JOB_SHIFT) &
			HIPPIBP_DMA_JOB_MASK) != job)
			break;	/* active DMA for other job */

		dprintf(1,("hippibpclose(%d:%d): SRC DMA ACTIVE (0x%x)\n",
				hippi_devp->unit, job, status));
	}

	/* One more time, for the other side. */
	dma_status = (__uint32_t *)&hippi_devp->dst_bp_fw_conf->dma_status;
	while (1) {
		status = *dma_status;
		if (!((status >> HIPPIBP_DMA_ACTIVE_SHIFT) &
			HIPPIBP_DMA_ACTIVE_MASK))
			break;	/* no DMA currently active */

		if (((status >> HIPPIBP_DMA_JOB_SHIFT) &
			HIPPIBP_DMA_JOB_MASK) != job)
			break;	/* active DMA for other job */

		dprintf(1,("hippibpclose(%d:%d): DST DMA ACTIVE (0x%x)\n",
				hippi_devp->unit, job, status));
	}

	if (bpjob->portused) {
		cmn_err(CE_WARN,"hippibpclose: USING PORTIDS\n");
		recoverpgs=0;
	} else {
		hippi_devp->hippibp_portfree +=
			bpjob->portmax;
		bpjob->portmax = 0;
	}

recover_iomaps:

	s = mutex_spinlock( hippi_devp->shc_slock );
	HIPPI_SRCWAIT (hippi_devp->cookie);
	mutex_spinunlock( hippi_devp->shc_slock, s );

	s = mutex_spinlock( hippi_devp->dhc_slock );
	HIPPI_DSTWAIT (hippi_devp->cookie);
	mutex_spinunlock( hippi_devp->dhc_slock, s );

	/* Job has been disabled, all of the freemaps contain garbage page
	 * pointers, and there is no active DMA for this job.
	 *			 OR
	 * controller has been reset, so no DMA is possible.
	 *
	 * It is now safe to recover all of the pages used by the job.
	 */

	if (bpjob->hipiomap_cnt) {
		cmn_err(CE_WARN,"hippibpclose: setup_bpio still inuse\n");

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
		cmn_err(CE_WARN,"hippibpclose: hipiomap_cnt %d\n",bpjob->hipiomap_cnt);
	}

	if (bpjob->Sfreemap.ID || bpjob->Dfreemap.ID ||
	    bpjob->portidPagemap.ID || bpjob->SDesqID || bpjob->DFlistID) {
	
		recoverpgs=0;

		cmn_err(CE_WARN, "hippibpclose: ENTRY STILL ACTIVE\n");
		dprintf(1,("Sfreemap.ID 0x%x Dfreemap.ID 0x%x\n",
			   bpjob->Sfreemap.ID, bpjob->Dfreemap.ID));
		dprintf(1,("portidPagemap.ID 0x%x SDesqID 0x%x\n",
			   bpjob->portidPagemap.ID, bpjob->SDesqID));
		dprintf(1,("DFlistID 0x%x\n", bpjob->DFlistID));
	} else
		bpjob->jobFlags = 0;

	for (i=0; i<bpjob->hippibp_maxproc; i++)
		if (bpjob->callback_list[i] != 0)
			cmn_err(CE_WARN,"hippibp_close: missing callback for 0x%x\n",
				bpjob->callback_list[i]);

	bpjob->hippibp_maxproc = 0;

	hippifreemap_release( &bpjob->Sfreemap, recoverpgs );

	hippifreemap_release( &bpjob->Dfreemap, recoverpgs );

	hippifreemap_release(&bpjob->portidPagemap, recoverpgs);

	vsema( & hippi_devp->devsema );
	return 0;

    default:
	/* No way. */
	cmn_err(CE_WARN, 
		"hippibpclose: %v: unknown vertex type (%d)\n",
		vhdl, bp->vtype);
	return ENODEV;

    } /* switch (bp->vtype) */
}



/* ARGSUSED */
int
hippibpioctl(dev_t dev, int cmd, void *arg,
	    int mode, cred_t *crp, int *rvalp)
{
    int			error = 0;
    vertex_hdl_t	vhdl = dev_to_vhdl(dev);
    hippibp_soft_t	*bp = hippibp_soft_get (vhdl);
    hippibp_vars_t	*hippi_devp = bp->hp;
    int			s, unit;

    struct hippi_bp_job *bpjob;
    int job;

    unit = hippi_devp->unit;

    /* Backward compatibility issue. If !HIPPIBP_UP (i.e. not
     * yet SET_BPCFG-ed), old driver would not allow any ioctls
     * on the job-specific device nodes. Only allowed SET/GET_BPCFG
     * and GET_BPSTATS on the base hippi node while in that state.
     */
    if (bp->vtype == BPROOT) {
	if ((cmd != HIPIOC_GET_BPSTATS) && (cmd != HIPIOC_SET_BPCFG) &&
	    (cmd != HIPIOC_GET_BPCFG))
	    return EINVAL;
	dprintf (2, ("hippibpioctl(unit=%d, dev=%x,cmd=%x)\n",
		      unit, dev, cmd));
    }
    else {
	job = bp->jobid;
	bpjob = &hippi_devp->bp_jobs[job];
	dprintf (2, ("hippibpioctl(unit=%d, dev=%x, job=%d, cmd=%x)\n",
		      unit, dev, job, cmd));
    }

    psema (&hippi_devp->devsema, PZERO);
    if (!HIPPIBP_UP(hippi_devp) && (bp->vtype != BPROOT)) {
	vsema (&hippi_devp->devsema);
	return ENODEV;
    }    

    switch (cmd) {
    case HIPIOC_GET_BPSTATS: {
	__uint64_t hippibp_stats[sizeof(hippibp_stats_t)/8];
	int i;

	if ((hippi_devp->src_bp_stats == 0) ||
	    !(hippi_devp->hippibp_state & HIPPIBP_ST_UP)) {
		error = ENODEV;
		break;
	}

	/* First, get stats from src controller */
	s = mutex_spinlock( hippi_devp->shc_slock );
	HIPPI_SRCWAIT (hippi_devp->cookie);
	HIPPI_SRCHWOP (HCMD_BP_STATUS, hippi_devp->cookie);
	HIPPI_SRCWAIT (hippi_devp->cookie);
	mutex_spinunlock( hippi_devp->shc_slock, s );

	/* XXX: copyout might not use 32-bit operations! */
	/* Copy from controller using word access */
	for (i=0; i<(sizeof(hippibp_stats_t)/8); i++)
	    hippibp_stats[i] = hippi_devp->src_bp_stats[i];

	/* Now, overlay it with stats from dst controller */
	s = mutex_spinlock( hippi_devp->dhc_slock );
	HIPPI_DSTWAIT (hippi_devp->cookie);
	HIPPI_DSTHWOP (HCMD_BP_STATUS, hippi_devp->cookie);
	HIPPI_DSTWAIT (hippi_devp->cookie);
	mutex_spinunlock( hippi_devp->dhc_slock, s );

	/* Copy from controller using word access */
	for (i=0; i<(sizeof(hippibp_stats_t)/8); i++)
	    hippibp_stats[i] |= hippi_devp->dst_bp_stats[i];

	if (copyout ((caddr_t)hippibp_stats, (caddr_t)arg,
		    sizeof(hippibp_stats_t)))
	    error = EFAULT;
	break;

    } /* case HIPIOC_GET_BPSTATS */

    case HIPIOC_SET_BPCFG: {
	struct hip_bp_config bp_cfg;

	/* This ioctl must be issued on root BP device, not job
	   device nodes */
	if (bp->vtype != BPROOT) {
	    error = EINVAL;
	    break;
	}
	if (!(hippi_devp->hippibp_state & HIPPIBP_ST_UP)) {
	    error = ENODEV;
	    break;
	}
	if (hippi_devp->hippibp_state & HIPPIBP_ST_OPENED) {
	    error = EINVAL;
	    break;
	}
	if (copyin((caddr_t)arg, (caddr_t)&bp_cfg, 
		    sizeof(struct hip_bp_config)) < 0) {
	    error =  EFAULT;
	    break;
	}

	/* Allocate ByPass control structure if it isn't already allocated.
	 * Make sure controller is operational before performing allocation
	 * since it dynamically sets up the initialization info.
	 */

	if (!(hippi_devp->hippibp_state & HIPPIBP_ST_CONFIGED) &&
			(error = _hippibpinit( hippi_devp)))
	    break;

  	hippi_devp->bp_ulp = bp_cfg.ulp;
	hippi_devp->bp_maxjobs = MIN(bp_cfg.max_jobs, HIPPIBP_MAX_JOBS);
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
		    hippi_devp->cached_src_bp_fw_conf.num_jobs);
	hippi_devp->bp_maxportids = 
		MIN(hippi_devp->bp_maxportids,
		    hippi_devp->cached_dst_bp_fw_conf.num_ports);
	hippi_devp->bp_maxdfmpgs =
	    MIN(hippi_devp->bp_maxdfmpgs,
	    (hippi_devp->cached_dst_bp_fw_conf.dfm_size)/sizeof(iopaddr_t));
	hippi_devp->bp_maxsfmpgs =
	    MIN(hippi_devp->bp_maxsfmpgs,
	    (hippi_devp->cached_src_bp_fw_conf.sfm_size)/sizeof(iopaddr_t));

	dprintf(1,("set_bpcfg(%d): ulp 0x%x jobs %d portids %d\n",
		    unit,bp_cfg.ulp, bp_cfg.max_jobs,bp_cfg.max_portids));

	dprintf(1,("set_bpcfg(%d): dfmpgs %d sfmpgs %d ddqpgs %d\n",
		    unit,bp_cfg.max_dfm_pgs, bp_cfg.max_sfm_pgs,
			   bp_cfg.max_ddq_pgs));

	dprintf(1,("set_bpcfg(%d): CONFIG maxjobs %d maxportids %d\n",
		    unit,hippi_devp->bp_maxjobs,hippi_devp->bp_maxportids));

	dprintf(1,("set_bpcfg(%d): CONFIG maxdfmpgs %d maxsfmpgs %d\n",
		    unit,hippi_devp->bp_maxdfmpgs,hippi_devp->bp_maxsfmpgs));

	/* Configure src side with ULP */
	s = mutex_spinlock( hippi_devp->shc_slock );
	if (HIPPI_SRCWAIT (hippi_devp->cookie) <= 0) {
	    mutex_spinunlock( hippi_devp->shc_slock, s );
	    error = EBUSY;
	    break;
	}

	*(__uint64_t *) &hippi_devp->shc_area->arg.bp_conf.ulp =
					(__uint64_t)hippi_devp->bp_ulp << 32;
	HIPPI_SRCHWOP (HCMD_BP_CONF, hippi_devp->cookie);
	mutex_spinunlock( hippi_devp->shc_slock, s );

	/* Configure dst side with ULP */
	s = mutex_spinlock( hippi_devp->dhc_slock );
	if (HIPPI_DSTWAIT (hippi_devp->cookie) <= 0) {
	    mutex_spinunlock( hippi_devp->dhc_slock, s );
	    error = EBUSY;
	    break;
	}
	*(__uint64_t *) &hippi_devp->dhc_area->arg.bp_conf.ulp =
					(__uint64_t)hippi_devp->bp_ulp << 32;
	HIPPI_DSTHWOP (HCMD_BP_CONF, hippi_devp->cookie);
	mutex_spinunlock( hippi_devp->dhc_slock, s );

	if (error = hippibp_config_driver( hippi_devp ))
		break;

	hippi_devp->hippibp_state |= HIPPIBP_ST_CONFIGED;

	break;

    } /* case HIPIOC_SET_BPCFG */

    case HIPIOC_GET_BPCFG: {
	/* OK on either BPROOT or BPJOB device
	 * Old driver only allowed this on job-specific devices if the
	 * bpstate was CONFIGed. But allowed it generic hippi dev in
	 * any state. So we do the same.
	 */
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
    } /* case HIPIOC_GET_BPCFG */

    case HIPIOC_SET_JOB: {
	struct hip_set_job setjob;
	int missingFWports;

	if (copyin((caddr_t)arg, (caddr_t)&setjob,
	    sizeof(struct hip_set_job)) < 0) {
	    error = EFAULT;
	    break;
	}

	/* Invalid request if user asks for too many portids OR if
	 * the descriptor size is not a multiple of 8 bytes.
	 */

	/* XXX - is missingFWports really necessary?
	 * bp_maxportids already took into consideration 
	 * hippi_devp->cached_dst_bp_fw_conf.num_ports when it
	 * was set. -- ik.
	 */
	/* Firmware might not support as many ports as driver */
	missingFWports = hippi_devp->bp_maxportids -
			 hippi_devp->cached_dst_bp_fw_conf.num_ports;
	if (missingFWports < 0)
	    missingFWports = 0;

	if (setjob.max_ports >
		    (hippi_devp->hippibp_portfree - missingFWports)) {
	    error = EINVAL;
	    break;
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
	dprintf(1,("setjob(%d:%d): ddqpgs 0x%x\n", unit, job, bpjob->ddqpgs));
	break;
    } /* case HIPIOC_SET_JOB */

    case HIPIOC_ENABLE_JOB: {
	struct hip_enable_job enablejob;

	if (copyin((caddr_t)arg, (caddr_t)&enablejob,
	    sizeof(struct hip_enable_job)) < 0) {
	    error = EFAULT;
	    break;
	}
	dprintf(1,("enableJob(%d:%d): enabled\n",unit,job));

	/* First, download enable-job params to src side. */
	s = mutex_spinlock( hippi_devp->shc_slock );
	if (HIPPI_SRCWAIT (hippi_devp->cookie) <= 0) {
	    mutex_spinunlock( hippi_devp->shc_slock, s );
	    error = EBUSY;
	    break;
	}

#ifdef LINC1_MBOX_WAR
	bpjob->ack_host = enablejob.ack_host;
	bpjob->ack_port = enablejob.ack_port;
	*(__uint64_t*) &hippi_devp->shc_area->arg.bp_job.enable
		= (((__uint64_t) 1) << 32) | job;

	*(__uint64_t*) &hippi_devp->shc_area->arg.bp_job.fm_entry_size
	  	= (((__uint64_t) NBPP) << 32) | bpjob->authno[0];

	*(__uint64_t*) &hippi_devp->shc_area->arg.bp_job.auth[1]
	  	= (((__uint64_t) bpjob->authno[1]) << 32) | bpjob->authno[2];
	*(__uint64_t*) &hippi_devp->shc_area->arg.bp_job.ack_host
		= (((__uint64_t) enablejob.ack_host) << 32)|enablejob.ack_port;
#else
	hippi_devp->shc_area->arg.bp_job.enable = 1;
	hippi_devp->shc_area->arg.bp_job.job = job;

	hippi_devp->shc_area->arg.bp_job.fm_entry_size = NBPP;

	hippi_devp->shc_area->arg.bp_job.ack_host =
				  bpjob->ack_host = enablejob.ack_host;
	hippi_devp->shc_area->arg.bp_job.ack_port =
				  bpjob->ack_port = enablejob.ack_port;

	hippi_devp->shc_area->arg.bp_job.auth[0] = bpjob->authno[0];
	hippi_devp->shc_area->arg.bp_job.auth[1] = bpjob->authno[1];
	hippi_devp->shc_area->arg.bp_job.auth[2] = bpjob->authno[2];
#endif
	HIPPI_SRCHWOP (HCMD_BP_JOB, hippi_devp->cookie);
	mutex_spinunlock( hippi_devp->shc_slock, s );

	/* Repeat to dest side. */
	s = mutex_spinlock( hippi_devp->dhc_slock );
	if (HIPPI_DSTWAIT (hippi_devp->cookie) <= 0) {
	    mutex_spinunlock( hippi_devp->dhc_slock, s );
	    error = EBUSY;
	    break;
	}

#ifdef LINC1_MBOX_WAR
	*(__uint64_t*) &hippi_devp->dhc_area->arg.bp_job.enable
		= (((__uint64_t) 1) << 32) | job;

	*(__uint64_t*) &hippi_devp->dhc_area->arg.bp_job.fm_entry_size
	  	= (((__uint64_t) NBPP) << 32) | bpjob->authno[0];

	*(__uint64_t*) &hippi_devp->dhc_area->arg.bp_job.auth[1]
	  	= (((__uint64_t) bpjob->authno[1]) << 32) | bpjob->authno[2];
	*(__uint64_t*) &hippi_devp->dhc_area->arg.bp_job.ack_host
		= (((__uint64_t) enablejob.ack_host) << 32)|enablejob.ack_port;
#else
	hippi_devp->dhc_area->arg.bp_job.enable = 1;
	hippi_devp->dhc_area->arg.bp_job.job = job;

	hippi_devp->dhc_area->arg.bp_job.fm_entry_size = NBPP;

	hippi_devp->dhc_area->arg.bp_job.ack_host = enablejob.ack_host;
	hippi_devp->dhc_area->arg.bp_job.ack_port = enablejob.ack_port;

	hippi_devp->dhc_area->arg.bp_job.auth[0] = bpjob->authno[0];
	hippi_devp->dhc_area->arg.bp_job.auth[1] = bpjob->authno[1];
	hippi_devp->dhc_area->arg.bp_job.auth[2] = bpjob->authno[2];
#endif

	HIPPI_DSTHWOP (HCMD_BP_JOB, hippi_devp->cookie);
	mutex_spinunlock( hippi_devp->dhc_slock, s );

	break;
    } /* HIPIOC_ENABLE_JOB */

    case HIPIOC_ENABLE_PORT: {
	struct hip_enable_port enable_port;
	iopaddr_t physaddr;
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
	    if (hippi_devp->hippibp_portids[i].jobid != (char) -1) {
		error = EBUSY;
		break;
	    }
	    hippi_devp->hippibp_portids[i].jobid = job;
	    hippi_devp->hippibp_portids[i].pid = 0;
	    hippi_devp->hippibp_portids[i].signo = 0;
	    bpjob->portused++;
	} else {
	    for (i=0; i<hippi_devp->hippibp_portmax; i++) {
		if (hippi_devp->hippibp_portids[i].jobid != (char) -1)
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
	} else if (copyout(&i, (void*)enable_port.portid_ptr, sizeof(int))<0)
	    error =  EFAULT;
	else if ((enable_port.pgidx > bpjob->portidPagemap.pgcnt) ||
		     (enable_port.pgidx % bpjob->ddqpgs) ||
		     (bpjob->portidPagemap.vaddr == NULL))
	    error = EINVAL;

	if (error)
	    break;

	physaddr = bpjob->portidPagemap.pfns[enable_port.pgidx];

	dprintf(1,("getport(%d:%d): port %d pgid %d phys 0x%x\n",
			   unit,job,i, enable_port.pgidx, physaddr));

	/* ENABLE PORT goes to dst side only. */
	s = mutex_spinlock( hippi_devp->dhc_slock );
	if (HIPPI_DSTWAIT (hippi_devp->cookie) <= 0) {
	    mutex_spinunlock( hippi_devp->dhc_slock, s );
	    error = EBUSY;
	    break;
	}
#ifdef LINC1_MBOX_WAR
	* (__uint64_t*) &hippi_devp->dhc_area->arg.bp_port.ux
		  	= (((__uint64_t)HIP_BP_PORT_PGX) << 60) | job;

	* (__uint64_t*) &hippi_devp->dhc_area->arg.bp_port.port
		  	= (((__uint64_t) i ) << 32) | (physaddr>>32);

	* (__uint64_t*) &hippi_devp->dhc_area->arg.bp_port.ddq_lo
	  		= (((__uint64_t) (physaddr & 0xffffffff)) << 32) |
			    (bpjob->ddqpgs * NBPP);
#else
	/* OPCODE must be written as word, so kludge It
	hippi_devp->dhc_area->arg.bp_port.ux.s.opcode = HIP_BP_PORT_PGX;
	*/
	hippi_devp->dhc_area->arg.bp_port.ux.i = HIP_BP_PORT_NOPGX << 28;
	hippi_devp->dhc_area->arg.bp_port.job  = job;
	hippi_devp->dhc_area->arg.bp_port.port = i;
	hippi_devp->dhc_area->arg.bp_port.ddq_hi = physaddr>>32;
	hippi_devp->dhc_area->arg.bp_port.ddq_lo = (physaddr & 0xffffffff);
	hippi_devp->dhc_area->arg.bp_port.ddq_size = bpjob->ddqpgs * NBPP;
#endif

	HIPPI_DSTHWOP (HCMD_BP_PORT, hippi_devp->cookie);
	mutex_spinunlock( hippi_devp->dhc_slock, s );

	break;
    } /* HIPIOC_ENABLE_PORT */

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
	    (hippi_devp->cached_src_bp_fw_conf.hostx_size/sizeof(int))) {
	    error =  EINVAL;
	    break;
	}

	if ((bpjob->ifields == 0) &&
	    ((bpjob->ifields = (__uint32_t *)
		kmem_zalloc(hippi_devp->cached_src_bp_fw_conf.hostx_size,
			    KM_NOSLEEP)) == NULL)) {
	    error = ENOMEM;
	    break;
	}

	if (copyin((void *)sifields.addr, bpjob->ifields,
			sifields.nbr_ifields*sizeof(int)) < 0) {
	    error =  EFAULT;
	    break;
	}

	bpjob->ifieldcnt = sifields.nbr_ifields;
	FWbase = (__uint32_t *)((char *)hippi_devp->src_bp_ifield +
			    job*hippi_devp->cached_src_bp_fw_conf.hostx_size);

	dprintf(1,("hippibpioctl: ifield array 0x%x, FW 0x%x\n",
		    bpjob->ifields, FWbase));

	for (i=0; i < sifields.nbr_ifields; i++)
	    FWbase[i] = bpjob->ifields[i];

	break;
    } /* HIPIOC_SET_HOSTX */

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

	if (copyout(bpjob->ifields, (void *)lifields.addr, 
		    ifieldcnt*sizeof(int)) < 0) {
	    error = EFAULT;
	    break;
	}

	if (copyout(&ifieldcnt,	(void*)lifields.nbr_ifields_ptr, 
		    sizeof(int)) < 0) {
	    error =  EFAULT;
	}

	break;
    } /* HIPIOC_GET_HOSTX */

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
	    hippi_devp->hippibp_portids[hip_intr.portid].pid = curpid;
	} else if (hippi_devp->hippibp_portids[hip_intr.portid].pid ==
				curpid)  {
	    hippi_devp->hippibp_portids[hip_intr.portid].signo = 0;
	    hippi_devp->hippibp_portids[hip_intr.portid].pid = 0;
	} else
	    error = EINVAL;

	if (!error) {
	    error = hippibp_add_exit_callback(bpjob, unit, job, bp);
	    if (error)
		cmn_err(CE_WARN,"hippibp: add_exit err %d\n", error);
	}

	dprintf(1,("enable_intr(%d:%d): portid 0x%x signo %d curpid %d\n",
		      unit,job,hip_intr.portid, hip_intr.signal_no,
		      curpid));
	break;
    } /* HIPIOC_ENABLE_INTR */

    case HIPIOC_GET_SDESQHEAD: {
	__uint32_t    retval = 0;

	s = mutex_spinlock( hippi_devp->shc_slock );
	if (HIPPI_SRCWAIT (hippi_devp->cookie) > 0) {
	    *(uint64_t *) &hippi_devp->shc_area->arg.sdqhead_jobnum =
							(uint64_t) job << 32;
	    HIPPI_SRCHWOP (HCMD_BP_SDQHEAD, hippi_devp->cookie);
	    if (HIPPI_SRCWAIT (hippi_devp->cookie) > 0)
	        retval = 1;
	}
	mutex_spinunlock( hippi_devp->shc_slock, s );

	if (!retval) {
	    error = EBUSY;
	    break;
	}

	retval = hippi_devp->shc_area->res.sdqhead;
	dprintf(1,("get_sdhead(%d:%d): return val 0x%x\n",
		      unit,job,retval));

	if (copyout(&retval, (caddr_t)arg, sizeof(__uint32_t)) <0)
	    error = EFAULT;
	break;
    }

    case HIPIOC_GET_FWADDR: {
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
	    baseaddr = (caddr_t)(hippi_devp->src_bp_sfreemap
		+ job*(hippi_devp->cached_src_bp_fw_conf.sfm_size/sizeof(iopaddr_t)));
	    FWsize = (caddr_t)((__psunsigned_t)hippi_devp->cached_src_bp_fw_conf.sfm_size);
	    break;

	case 2:
	    baseaddr = (caddr_t)(hippi_devp->dst_bp_dfreemap
		+ job*(hippi_devp->cached_dst_bp_fw_conf.dfm_size/sizeof(iopaddr_t)));
	    FWsize = (caddr_t)((__psunsigned_t)hippi_devp->cached_dst_bp_fw_conf.dfm_size);
	    break;

	case 3:
	    baseaddr = (caddr_t)(hippi_devp->src_bp_sdqueue
			+ job*hippi_devp->cached_src_bp_fw_conf.sdq_size);
	    FWsize = (caddr_t)((__psunsigned_t)hippi_devp->cached_src_bp_fw_conf.sdq_size);
	    break;

	case 4:
	    baseaddr = (caddr_t)(hippi_devp->dst_bp_dfreelist
			+ job * hippi_devp->cached_dst_bp_fw_conf.dfl_size);
	    FWsize = (caddr_t)((__psunsigned_t)hippi_devp->cached_dst_bp_fw_conf.dfl_size);
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
    } /* HIPIOC_GET_FWADDR */

    case HIPIOC_SETUP_BPIO: {
	struct hip_io_setup hip_setup;
	int i, j, cookie;
#ifdef DEBUG
	int rc;
#endif
	volatile iopaddr_t *FWbase;
	int sfm_selected = 0, dfm_selected = 0;
	alenlist_t  uvlist;
	alenaddr_t  paddr;
	size_t	    plen;
	ulong_t curpid = 0;

	drv_getparm(PPID, &curpid); /* get pid of current process */

	if ((bpjob->jobFlags & JOBFLAG_EXCL) == 0) {
		error = EPERM;
		break;
	}

	if (copyin((caddr_t)arg, (caddr_t)&hip_setup,
	    sizeof(struct hip_io_setup)) < 0) {
	    error =  EFAULT;
	    break;
	}

	dprintf(1,("hippi_setup_bpio(%d:%d) uaddr 0x%x len 0x%x \n",
		    unit,job,hip_setup.uaddr,hip_setup.ulen));

	/* XXX - shouldn't this be KM_SLEEP? There's no checking for
	 *	 failure of the kmem_zalloc(). -- ik. */
	if (bpjob->hipiomap == 0) {
	    bpjob->hipiomap = kmem_zalloc(HIPPIBP_MAX_IOSETUP *
					sizeof(struct hippi_io), KM_NOSLEEP);
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
	    if ((bpjob->hipiomap[i].endindex >= hippi_devp->bp_maxdfmpgs) ||
		(bpjob->hipiomap[i].startindex < bpjob->Dfreemap.pgcnt)){
		dprintf(1,("hippi_setup_bpio(%d:%d) DFM EINVAL\n", unit,job));
		error = EINVAL;
		break;
	    }
	    dfm_selected = 1;
	} else if (hip_setup.mapselect == BPIO_SFM_SEL) { /* sfreemap */
	    if ((bpjob->hipiomap[i].endindex >= hippi_devp->bp_maxsfmpgs) ||
		(bpjob->hipiomap[i].startindex < bpjob->Sfreemap.pgcnt)) {
		    dprintf(1,("hippi_setup_bpio(%d:%d) SFM EINVAL\n", 
				unit,job));
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

	if (!(uvlist = uvaddr_to_alenlist (NULL, (caddr_t) hip_setup.uaddr, 
				 hip_setup.ulen, AL_NOCOMPACT))) {
	    fast_undma( (void *)bpjob->hipiomap[i].uaddr,
				bpjob->hipiomap[i].ulen,
				bpjob->hipiomap[i].uflags|B_READ,
				&bpjob->hipiomap[i].dmacookie );

	    dprintf(1,("hippi_setup_bpio(%d:%d) uvaddr_to_alenlist() failed\n",
			      unit,job));
	    error = EFAULT;
	    break;
	}

	dprintf(1,("hippi_setup_bpio(%d:%d) uvaddr_to_alenlist() OK, size=%d\n", 
		    unit,job, alenlist_size (uvlist)));
	ASSERT (alenlist_size (uvlist) == 1 +
				    (bpjob->hipiomap[i].endindex -
				     bpjob->hipiomap[i].startindex));

	if (dfm_selected) {	/* dfreemap */
	    if ( (alenlist_size (uvlist) != 1 + bpjob->hipiomap[i].endindex -
			bpjob->hipiomap[i].startindex) ||
	        !(uvlist=pciio_dmatrans_list (hippi_devp->dcnctpt, 0, uvlist,
			PCIIO_DMA_A64 | PCIIO_DMA_DATA)) ) {
		cmn_err(CE_WARN, "hippibp(HIPIOC_SETUP_BPIO): bad alenlist\n");
		error = EFAULT;
		alenlist_done (uvlist);
		break;
	    }

	    bpjob->hipiomap[i].mapselect |= HIPPIBP_DFM_SEL;
	    FWbase = hippi_devp->dst_bp_dfreemap + job *
		(hippi_devp->cached_dst_bp_fw_conf.dfm_size/sizeof(iopaddr_t));

	    if (bpjob->Dfreemap.pfns == 0)
		hippibp_freemap_pfn_alloc( &bpjob->Dfreemap,
					   HIPPIBP_MAX_DMAP_PGS,
					   hippi_devp->garbage_dst_iopaddr );

	    alenlist_cursor_init( uvlist, 0, NULL );

	    for ( j = bpjob->hipiomap[i].startindex;
		  j <= bpjob->hipiomap[i].endindex; ) {
#ifdef DEBUG
		rc = alenlist_get( uvlist, NULL, 0, &paddr, &plen, 0 );
#else
		alenlist_get( uvlist, NULL, 0, &paddr, &plen, 0 );
#endif
		ASSERT (rc == ALENLIST_SUCCESS);
		do {
			bpjob->Dfreemap.pfns[j] = (iopaddr_t) paddr;
			FWbase[j] = (iopaddr_t) paddr;
			dprintf(1,("hippi_iosetup(%d:%d) DFM 0x%x paddr 0x%x\n",
				      unit,job, j, paddr));
			plen -= NBPP;
			paddr += NBPP;
			j++;
		} while (plen > 0);
	    }
	}

	if (sfm_selected) {	/* sfreemap */
	    if ( (alenlist_size (uvlist) != 1 + bpjob->hipiomap[i].endindex -
			bpjob->hipiomap[i].startindex) ||
	        !(uvlist=pciio_dmatrans_list (hippi_devp->scnctpt, 0, uvlist,
			PCIIO_DMA_A64 | PCIIO_DMA_DATA)) ) {
		cmn_err(CE_WARN, "hippibp(HIPIOC_SETUP_BPIO): bad alenlist\n");
		error = EFAULT;
		alenlist_done (uvlist);
		break;
	    }

	    bpjob->hipiomap[i].mapselect |= HIPPIBP_SFM_SEL;
	    FWbase = hippi_devp->src_bp_sfreemap + job *
		(hippi_devp->cached_src_bp_fw_conf.sfm_size/sizeof(iopaddr_t));

	    if (bpjob->Sfreemap.pfns == 0)
		hippibp_freemap_pfn_alloc( &bpjob->Sfreemap,
					HIPPIBP_MAX_SMAP_PGS,
					hippi_devp->garbage_src_iopaddr );

	    alenlist_cursor_init( uvlist, 0, NULL );
	    for (j=bpjob->hipiomap[i].startindex;
			     j<=bpjob->hipiomap[i].endindex; ) {
#ifdef DEBUG
		rc = alenlist_get( uvlist, NULL, 0, &paddr, &plen, 0 );
#else
		alenlist_get( uvlist, NULL, 0, &paddr, &plen, 0 );
#endif
		ASSERT (rc == ALENLIST_SUCCESS);
		do {
			bpjob->Sfreemap.pfns[j] = (iopaddr_t) paddr;
			FWbase[j] = (iopaddr_t) paddr;
			dprintf(1,("hippi_iosetup(%d:%d) SFM 0x%x paddr 0x%x\n",
				      unit,job, j, paddr));
			plen -= NBPP;
			paddr += NBPP;
			j++;
		} while (plen > 0);
	    }
	}
	alenlist_done(uvlist);

	error = hippibp_add_exit_callback(bpjob, unit, job, bp);

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
    } /* HIPIOC_SETUP_BPIO */

    case HIPIOC_TEARDOWN_BPIO: {
	struct hip_io_teardown hip_teardown;
	int i;
	ulong_t curpid = 0;

	drv_getparm(PPID, &curpid); /* get pid of current process */

	if ((bpjob->jobFlags & JOBFLAG_EXCL) == 0) {
		error = EPERM;
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
	hippibp_iomap_teardown( bp, i );

	dprintf(1,("hippi_teardown(%d:%d)(%d %d %d): vaddr 0x%x len 0x%x \n",
		      unit,job,i+1, bpjob->hipiomap[i].startindex,
		      bpjob->hipiomap[i].endindex,
		      bpjob->hipiomap[i].uaddr, bpjob->hipiomap[i].ulen));
	break;
    } /* HIPIOC_TEARDOWN_BPIO */

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
    } /* HIPIOC_BP_PROT_INFO */

    default:
	dprintf(2, ("hippibpioctl: UNKNOWN cmd=%x not implemented\n",
		    cmd));
	error = EINVAL;
	break;

    } /* switch (cmd) */

    vsema( &hippi_devp->devsema );
    return error;
}

/*
 * Note: freemap entries which were ints (pfns) in the old combined
 *	 driver are now all pciio_dmatrans_addr()ed 64-bit iopaddr_t's.
 */
/* ARGSUSED */
int
hippibpmap(dev_t dev, vhandl_t *vt, off_t off, int len, int prot)
{
    vertex_hdl_t	vhdl = dev_to_vhdl(dev);
    hippibp_soft_t	*bp = hippibp_soft_get (vhdl);
    hippibp_vars_t	*hippi_devp = bp->hp;
    caddr_t		kvaddr;
    int status=0, job, npgs=0, vmflags=0;
    volatile iopaddr_t *FWmbase;
    struct hippi_freemap *freemap = NULL;
    int olen;

    if (bp->vtype != BPJOB)
	return EINVAL;

    job = bp->jobid;

    dprintf(1,("hippibpmap(%d:%d) uaddr 0x%x len 0x%x \n", 
	       hippi_devp->unit, job, v_getaddr(vt), len));
    dprintf(2,("hippibpmap: vhandl 0x%x off 0x%x len 0x%x prot 0x%x\n",
		vt, off, len,  prot));
    dprintf(2,("hippibpmap: handle 0x%x addr 0x%x len 0x%x\n",
		v_gethandle(vt), v_getaddr(vt), v_getlen(vt)));

    psema( & hippi_devp->devsema, PZERO );
    if (!HIPPIBP_UP(hippi_devp))
	goto return_sema;

    olen = len;

    if ((off == HIPPIBP_SFREEMAP_OFF) || (off == HIPPIBP_DFREEMAP_OFF) ||
	(off == HIPPIBP_PORTIDMAP_OFF)) {

	/* Src or Dest Freemap */
	int maxpgs, MAXEVERpgs;
	iopaddr_t   garbage_page;
	int	    mapflags;

	if (off == HIPPIBP_SFREEMAP_OFF) {
	    freemap = &hippi_devp->bp_jobs[job].Sfreemap;
	    maxpgs = hippi_devp->bp_maxsfmpgs;
	    FWmbase = hippi_devp->src_bp_sfreemap +
		(job * hippi_devp->cached_src_bp_fw_conf.sfm_size/sizeof(iopaddr_t));
	    MAXEVERpgs = HIPPIBP_MAX_SMAP_PGS;
	    garbage_page = hippi_devp->garbage_src_iopaddr;
	    mapflags = PCIIO_DMA_A64 | PCIIO_DMA_DATA;
	} else if (off == HIPPIBP_DFREEMAP_OFF) {
	    freemap = &hippi_devp->bp_jobs[job].Dfreemap;
	    maxpgs = hippi_devp->bp_maxdfmpgs;
	    FWmbase = hippi_devp->dst_bp_dfreemap +
		(job * hippi_devp->cached_dst_bp_fw_conf.dfm_size/sizeof(iopaddr_t));
	    MAXEVERpgs = HIPPIBP_MAX_DMAP_PGS;
	    garbage_page = hippi_devp->garbage_dst_iopaddr;
	    mapflags = PCIIO_DMA_A64 | PCIIO_DMA_DATA;
	} else {
	    freemap = &hippi_devp->bp_jobs[job].portidPagemap;
	    maxpgs = hippi_devp->bp_jobs[job].portmax *
		     hippi_devp->bp_jobs[job].ddqpgs;
	    MAXEVERpgs = HIPPIBP_MAX_PORTIDS *
			 MAX(2,hippi_devp->bp_jobs[job].ddqpgs);
	    garbage_page = hippi_devp->garbage_dst_iopaddr;

	    /* turn on BARRIER OP bit for this one. */
	    mapflags = PCIIO_DMA_A64 | PCIIO_DMA_CMD;

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
	if (freemap->pfns == (iopaddr_t *)0)
	    hippibp_freemap_pfn_alloc( freemap, MAXEVERpgs, garbage_page );

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
		if (off == HIPPIBP_SFREEMAP_OFF)
		    freemap->pfns[npgs] =
			pciio_dmatrans_addr(hippi_devp->scnctpt, 0,
				kvtophys((caddr_t)kvaddr), NBPP, mapflags);
		else
		    freemap->pfns[npgs] =
			pciio_dmatrans_addr(hippi_devp->dcnctpt, 0,
				kvtophys((caddr_t)kvaddr), NBPP, mapflags);
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
	dprintf(1,("hippibpmap(%d:%d) SDesq uaddr 0x%x\n", 
		   hippi_devp->unit, job, v_getaddr(vt)));

	if (len != hippi_devp->cached_src_bp_fw_conf.sdq_size) {
	    status = EINVAL;
	    goto return_sema;
	}

	if (hippi_devp->bp_jobs[job].hippibp_SDesqPage == NULL) {
	    hippi_devp->bp_jobs[job].hippibp_SDesqPage =
			hippi_devp->src_bp_sdqueue +
			job * hippi_devp->cached_src_bp_fw_conf.sdq_size;
	}

	if (status = v_mapphys(vt,
		(void *)hippi_devp->bp_jobs[job].hippibp_SDesqPage, btoc(len)))
	    goto return_sema;

    } else if (off == HIPPIBP_DFLIST_OFF) {	/* DesFreelist */

	dprintf(1,("hippibpmap(%d:%d) DFlist uaddr 0x%x\n",
		   hippi_devp->unit, job, v_getaddr(vt)));

	if (len != hippi_devp->cached_dst_bp_fw_conf.dfl_size) {
	    status =  EINVAL;
	    goto return_sema;
	}

	if (status = v_mapphys(vt,
		(void *)hippi_devp->bp_jobs[job].hippibp_DFlistPage,btoc(len)))
	    goto return_sema;;

    } else if (off == HIPPIBP_MBOX_OFF) {
	dprintf(1,("hippibpmap(%d:%d) Mailbox uaddr 0x%x\n",
		   hippi_devp->unit, job, v_getaddr(vt)));

	if (len != 8) {		/* mbox is 8 bytes */
	    status =  EINVAL;
	    goto return_sema;
	}

	/* LINC mboxes are spaced 64K apart in the mbox region */
	if (status = v_mapphys(vt,
			       (void *) ( (__uint64_t) hippi_devp->sbufmem 
			        + LINC_MAILBOX_ADDR + (job*0x10000) ),
			       btoc(len)))
	    goto return_sema;

    } else {
	cmn_err(CE_WARN, "hippibpmap: UNKNOWN offset 0x%x\n", off);
	status =  EINVAL;
	goto return_sema;
    }

#if HIPPI_BP_DEBUG && DEBUG && NOTYET
    if (hippibp_debug >= 2)
		idbg_pregpde( v_getpreg(vt) );	/* print pregion map */
#endif /* HIPPI_BP_DEBUG */

return_sema:
    vsema( & hippi_devp->devsema );
    return(status);
}

int
hippibpunmap(dev_t dev, vhandl_t *vt)
{
    vertex_hdl_t	vhdl = dev_to_vhdl(dev);
    hippibp_soft_t	*bp = hippibp_soft_get (vhdl);
    hippibp_vars_t	*hippi_devp = bp->hp;
     __psunsigned_t	ID;
    int	job;

    if (bp->vtype != BPJOB)
	return EINVAL;
    job = bp->jobid;

    ID = v_gethandle(vt);

    dprintf(2,("hippiunmap: dev 0x%x vhandl 0x%x\n", dev, vt));
    dprintf(2,("hippiunmap: handle 0x%x addr 0x%x len 0x%x\n",
		ID, v_getaddr(vt), v_getlen(vt)));

    if (ID == hippi_devp->bp_jobs[job].Sfreemap.ID) {

	dprintf(1,("hippiunmap(%d:%d): Sfreemap uaddr 0x%x\n",
		   hippi_devp->unit, job, v_getaddr(vt)));
	hippi_devp->bp_jobs[job].Sfreemap.ID = 0;

    } else if (ID == hippi_devp->bp_jobs[job].Dfreemap.ID) {

	dprintf(1,("hippiunmap(%d:%d): Dfreemap uaddr 0x%x\n",
		   hippi_devp->unit, job, v_getaddr(vt)));
	hippi_devp->bp_jobs[job].Dfreemap.ID = 0;

    } else if (ID == hippi_devp->bp_jobs[job].portidPagemap.ID) {

	dprintf(1,("hippiunmap(%d:%d): portidPagemap uaddr 0x%x\n",
		   hippi_devp->unit, job, v_getaddr(vt)));
	hippi_devp->bp_jobs[job].portidPagemap.ID = 0;

    } else if (ID == hippi_devp->bp_jobs[job].SDesqID) {

	dprintf(1,("hippiunmap(%d:%d): SDesq\n", hippi_devp->unit, job));
	hippi_devp->bp_jobs[job].SDesqID = 0;

    } else if (ID == hippi_devp->bp_jobs[job].DFlistID) {

	dprintf(1,("hippiunmap(%d:%d): DFlist\n", hippi_devp->unit, job));
	hippi_devp->bp_jobs[job].DFlistID = 0;

    } else if (ID == hippi_devp->bp_jobs[job].MboxID) {

	dprintf(1,("hippiunmap(%d:%d): Mbox\n", hippi_devp->unit, job));
	hippi_devp->bp_jobs[job].MboxID = 0;

    }

    return 0; /* success */
}

