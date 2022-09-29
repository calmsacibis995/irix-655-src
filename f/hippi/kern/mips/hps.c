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

/*
 * hippi.c
 *
 * Lego HIPPI-Serial
 *
 * This driver provides the user-level API to access HIPPI-FP and HIPPI-PH
 * layers.  It also provides hooks for the TCP/IP HIPPI driver, if_hip,
 * and the HIPPI Bypass driver hippibp.
 */
#ident  "hippi.c: $Revision: 1.65 $" $Date: 1998/06/16 21:04:16 $

#include "sys/types.h"
#include "sys/cpu.h"
#include "sys/systm.h"
#include "sys/cmn_err.h"
#include "sys/errno.h"
#include "sys/uio.h"
#include "sys/iobus.h"
#include "sys/ioctl.h"
#include "sys/file.h"
#include "sys/cred.h"
#include "sys/poll.h"
#include "sys/immu.h"
#include "ksys/as.h"
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
#include "sys/nic.h"
#include "sys/PCI/PCI_defs.h"
#include "sys/PCI/pciio.h"
#include "sys/PCI/linc.h"
#include "sys/PCI/bridge.h"
#include "sys/xtalk/xwidget.h"
#include "sys/atomic_ops.h"
#include "fwvers.h"
#include "lincprom.h"
#include "sys/hip_errors.h"
#include "sys/hippi.h"
#include "sys/hippi_firm.h"
#include "sys/hps_priv.h"
#include "sys/hps_ext.h"
#include "sys/if_hip_ext.h"
#include "sys/serialio.h"
#include "sys/idbgentry.h"


#ifdef HPS_DEBUG
int	hps_debug_lvl = 0;	/* Controls printing of debug info */
#define dprintf(lvl, x)	if (hps_debug_lvl>=lvl) printf x
#else
#define dprintf(lvl, x)
#endif	/* HPS_DEBUG */

int	hps_devflag = D_MP;
static  int hps_nextunit = 0;

#define _LINC_ADDRSPC_SIZE  0x08000000	    /* 128 MB. */
#define _LINC_SDRAM_SIZE    0x400000	    /*   4 MB */
#define _LINC_PROM_SIZE     0x200000	    /*   2 MB address space */
#define _LINC_RESET	    (LINC_LCSR_COLD_RST)
/* XXX - not sure if I can't just write a 0 to clear, but don't want to
 *	 lose these reset values. */
#define _LINC_CLR_RESET	    (LINC_LCSR_IGNORE_ERRORS | LINC_LCSR_BOOTING)

/*
 * XXX TO DO:
 * 1. Put some real code in hps_error_handler().
 * 2. Should check for HPS_SRC_WAIT and HPS_DST_WAIT return values
 *    and bail out gracefully if need be.
 * 3. Performance tweaks after Jim gets his fw up:
 *      Check Bridge device reg - RRB configs
 *	Set GBR bit?
 *	specify VCHANs in pciio_dmatrans_addr() calls?
 */

/* =====================================================================
 *			FUNCTION  PROTOTYPES
 */

extern void	hps_init(void);
extern int	hps_attach(vertex_hdl_t vhdl);

extern int	hps_open(dev_t *devp, int oflag, int otyp, cred_t *crp);
extern int	hps_close(dev_t dev, int oflag, int otyp, cred_t *crp);

extern int	hps_ioctl(dev_t dev, int cmd, void *arg,
			    int mode, cred_t *crp, int *rvalp);

extern int	hps_read(dev_t dev, uio_t *uiop, cred_t *crp);
extern int	hps_write(dev_t dev, uio_t *uiop, cred_t *crp);

extern int	hps_poll(dev_t dev, short events, int anyyet,
			   short *reventsp, struct pollhead **phpp);

static error_handler_f	hps_error_handler;

void	hps_src_intr( intr_arg_t );
void	hps_dst_intr( intr_arg_t );

void    hps_send_dummy_desc( hippi_vars_t *, u_char );
void	hps_fp_odone( hippi_vars_t *, volatile struct hip_d2b_hd *, int );
void	hps_fp_input( hippi_vars_t *, volatile hip_b2h_t * );

/* ==================================================================
 * hps_ioc_* are routines invoked by ioctls.
 */
int	hps_ioc_mkhwg( hippi_vars_t * );
int	hps_ioc_setonoff( hps_soft_t *, hippi_vars_t *, void *arg );
int	hps_ioc_getstats( hippi_vars_t *, void *arg );
int	hps_ioc_bindulp( hps_soft_t *, hippi_vars_t *, void *arg, int mode);
int	hps_ioc_setmac( hippi_vars_t *, void *arg );

static int hps_ioc_flash( hippi_vars_t *, int cmd, void * arg );
static void hps_readversions ( hippi_vars_t * );

/* =====================================================================
 * 	Private idbg routines
 */
#ifdef HPS_DEBUG
static void hippivars_idbg (hippi_vars_t *);
static void hippisoft_idbg (hps_soft_t *);
static void hippidev_idbg  (vertex_hdl_t);
#endif
/* =====================================================================
 *		Device-Related Constants and Structures
 */

/* LINC's Dev/Vendor ID, also used by cards other than ours */
#define	HPS_VENDOR_ID_NUM	0x10A9
#define	HPS_DEVICE_ID_NUM	0x0002

/* Number of pages of c2b's to post largest possible read() */
#define C2B_RDLISTPGS	(((HIPPIFP_MAX_READSIZE/NBPP+3) * \
				sizeof(hip_c2b_t) + NBPP-1)/ NBPP)
/* From fwvers.h */
hippi_linc_fwvers_t hippi_srcvers = {1, HIP_VERSION_MAJOR, HIP_VERSION_MINOR};
hippi_linc_fwvers_t hippi_dstvers = {0, HIP_VERSION_MAJOR, HIP_VERSION_MINOR};


/* =====================================================================
 * 	Private idbg routines
 */

#ifdef HPS_DEBUG
static void
fpulp_idbg (struct hippi_fp_ulps * p)
{
    qprintf ("opens=%d, ulpFlags=0x%x, ulpId=%d\n",
	     p->opens, p->ulpFlags, p->ulpId);
    qprintf ("Read: fpHdr_avail=%d, D2_avail=%d, offset=%d, count=%d\n",
	     p->rd_fpHdr_avail, p->rd_D2_avail, p->rd_offset, p->rd_count);
    qprintf ("rd_sv at %x, rd_semacnt=%d, rd_dmadn at %x\n",
	     &p->rd_sv, p->rd_semacnt, &p->rd_dmadn);
    qprintf ("pollhead = %x, fpd1head = %x, c2b_rdlist = %x\n",
	     p->rd_pollhdp, p->rd_fpd1head,  p->rd_c2b_rdlist);
}

static void
hippivars_idbg (hippi_vars_t *hp)
{
    int i;

    if (hp == NULL) {
	qprintf ("Invalid argument. USAGE: hippivars(hippi_vars_t*)\n");
	return;
    }
    
    qprintf (" -------------- I/O infrastructure stuff ------------\n");
    qprintf ("Vertices: dst=0x%x, src=0x%x, base dev=0x%x, BP ctl=0x%x\n",
	     hp->dst_vhdl, hp->src_vhdl, hp->dev_vhdl, hp->bp_vhdl);
    qprintf ("piomap=0x%x, src_intr=0%x, dst_intr=0x%x\n",
	     hp->piomap, hp->src_intr, hp->dst_intr);

    qprintf (" ------------- On board addresses -------------------\n");
    qprintf ("PCI Config spaces at: src 0x%x, dst 0x%x\n",
	     hp->src_cfg, hp->dst_cfg);
    qprintf ("src LINC at: lincregs 0x%x, SDRAM 0x%x, EEPROM 0x%x\n",
	     hp->src_lincregs, hp->src_bufmem, hp->src_eeprom);
    qprintf ("             HC area 0x%x, stat_area 0x%x\n",
	     hp->src_hc, hp->src_stat_area);
    qprintf ("dst LINC at: lincregs 0x%x, SDRAM 0x%x, EEPROM 0x%x\n",
	     hp->dst_lincregs, hp->dst_bufmem, hp->dst_eeprom);
    qprintf ("             HC area 0x%x, stat_area 0x%x\n",
	     hp->dst_hc, hp->dst_stat_area);

    qprintf (" --------------- Unit management stuff ---------------\n");
    qprintf ("unit=%d, hi_state=0x%x\n",
	     hp->unit, hp->hi_state);
    qprintf ("on-board src fw version = 0x%x, dst vers = %x\n",
	     hp->hi_srcvers, hp->hi_dstvers);
    qprintf ("hw flags = 0x%x, src timeout=0x%x, dst timeout=0x%x\n",
	     hp->hi_hwflags, hp->hi_stimeo, hp->hi_dtimeo);

    qprintf ("-------------- source side communications ----------------\n");
    qprintf ("s2h ring at 0x%x, current ptr = %x, end = %x\n",
	     hp->hi_s2h, hp->hi_s2hp, hp->hi_s2h_last);
    qprintf ("src sleep desc at %x, s2h seq # = %d\n",
	      hp->hi_ssleep, hp->hi_s2h_sn);
    qprintf ("d2b ring at %x, prod = %x, cons = %x, last = %x\n",
	     hp->hi_d2b, hp->hi_d2b_prod, hp->hi_d2b_cons, hp->hi_d2b_last);
    qprintf ("src cmd id = %d, cmd_line = %d\n",
	     hp->hi_src_cmd_id, hp->hi_src_cmd_line);

    qprintf ("-------------- dest side communications ----------------\n");
    qprintf ("d2h ring at 0x%x, curr ptr at %x, end at %x\n",
	     hp->hi_d2h, hp->hi_d2hp, hp->hi_d2h_last);
    qprintf ("dst sleep desc %x, intr id = %d\n",
	     hp->hi_dsleep, hp->hi_d2h_sn);
    qprintf ("c2b ring at 0x%x, current c2b ptr at %x\n",
	     hp->hi_c2b, hp->hi_c2bp);
    qprintf ("dst cmd id = %d, cmd_line = %d\n",
	     hp->hi_dst_cmd_id, hp->hi_dst_cmd_line);

    qprintf ("--------------- Locks and semaphores ------------------\n");
    qprintf ("devsema={%x,%x}, sv_mutex @ %x\n", 
	     hp->devsema.s_un.s_lock, hp->devsema.s_queue, &hp->sv_mutex);
    qprintf ("shc_slock=%x, src_mutex @ %x\n", hp->shc_slock, &hp->src_mutex);
    qprintf ("dhc_slock=%x, dst_mutex=%x\n", hp->dhc_slock, &hp->dst_mutex);
    qprintf ("rawoutq_sema={%x,%x}, src_sema={%x,%x}\n", 
	     hp->rawoutq_sema.s_un.s_lock, hp->rawoutq_sema.s_queue,
	     hp->src_sema.s_un.s_lock, hp->src_sema.s_queue);
    for (i = 0; i < HIPPIFP_MAX_WRITES; i++) {
      	qprintf ("rawoutq_sleep[i]=%x, error=%d\n", 
		 i, hp->rawoutq_sleep[i], hp->rawoutq_error[i]);
    }
    qprintf ("rawoutq_in=%d, rawoutq_out=%d, PHmode=%d\n",
	     hp->rawoutq_in, hp->rawoutq_out, hp->PHmode);

    qprintf (" -------------- ULP management -----------------------\n");
    for (i = 0; i < HIPPIFP_MAX_CLONES; i += 4) {
	qprintf ("clone_info[%d] = %x %x %x %x\n",
		 i, hp->clone_info[i], hp->clone_info[i+1], 
		 hp->clone_info[i+2], hp->clone_info[i+3]);
    }
    for (i = 0; i < HIPPI_ULP_MAX+1; i += 8) {
	  qprintf ("ulpFromId[%d] = %d %d %d %d ",
		   i, hp->ulpFromId[i], hp->ulpFromId[i+1],
		   hp->ulpFromId[i+2], hp->ulpFromId[i+3]);
	  qprintf ("%d %d %d %d\n",
		   hp->ulpFromId[i+4], hp->ulpFromId[i+5],
		   hp->ulpFromId[i+6], hp->ulpFromId[i+7]);
    }
    for (i = 0; i < HIPPIFP_MAX_OPEN_ULPS+1; i++) {
	qprintf ("---- hippi_fp_ulp  ulp[%d]:\n", i);
	fpulp_idbg (&hp->ulp[i]);
    }

    qprintf (" ------------ Debug statistics ----------------------\n");
    qprintf ("S2H: calls=%d, work=%d, pokes=%d, busy=%d\n",
	     hp->stat_s2h_calls, hp->stat_s2h_work, 
	     hp->stat_s2h_pokes, hp->stat_s2h_busy);
    qprintf ("D2H: calls=%d, work=%d, pokes=%d, busy=%d\n",
	     hp->stat_d2h_calls, hp->stat_d2h_work,
	     hp->stat_d2h_pokes, hp->stat_d2h_busy);
}

static void 
hippisoft_idbg (hps_soft_t *soft)
{
    if (soft == NULL) {
	qprintf ("Invalid argument. USAGE: hippisoft(hps_soft_t*)\n");
	return;
    }
    qprintf ("isclone=%d, cloneid=%d, vhdl=0x%x, hippi_devp=0x%x\n",
	     soft->isclone, soft->cloneid, soft->vhdl, soft->hippi_devp);
    qprintf ("ulpId=%d, ulpIndex=%d, mode=%d, cloneFlags=0x%x\n",
	     soft->ulpId, soft->ulpIndex, soft->mode, soft->cloneFlags);
    qprintf ("Write: pktResid=%d, Ifield=0x%x, fburst=%d\n",
	     soft->wr_pktOutResid, soft->wr_Ifield, soft->wr_fburst);
    qprintf ("FP header: ulp=%d, flags=0x%x, D1/D2 offset=0x%x, D2size=%d\n",
	     soft->wr_fpHdr.hfp_ulp_id, soft->wr_fpHdr.hfp_flags,
	     soft->wr_fpHdr.hfp_d1d2off, soft->wr_fpHdr.hfp_d2size);
    qprintf ("Errors: src=%d, dst=%d\n", soft->src_error, soft->dst_errors);
}

static void 
hippidev_idbg (vertex_hdl_t v)
{
    hps_soft_t * soft;

    if ((v == NULL) || ((soft = hps_soft_get(v)) == NULL)) {
	qprintf ("Invalid argument. USAGE: hippidev(vertex_hdl_t)\n");
	return;
    }
    hippisoft_idbg (soft);
}
#endif


/* =====================================================================
 *			Driver Initialization
 */

/* 
 *	hps_init: called once during system startup.
 *	Register our interest in _any_ LINC found in the system.
 */
void
hps_init(void)
{
    dprintf(5, ("hps_init()\n"));
    pciio_driver_register(HPS_VENDOR_ID_NUM, HPS_DEVICE_ID_NUM, "hps_", 0);
}

/*
 *	hps_attach: called by the pciio infrastructure
 *	once for each vertex representing a LINC.
 *
 *	In large configurations, it is possible for a
 *	large number of CPUs to enter this routine all at
 *	nearly the same time, for different specific
 *	instances of the device. Attempting to give your
 *	devices sequence numbers based on the order they
 *	are found in the system is not only futile but
 *	may be dangerous as the order may differ from
 *	run to run.
 */

/* Parses a hw graph pathname string and returns the module number,
 * or -1 if not found.
 * e.g. given the name "/hw/module/1/slot/io3/xwidget/pci/0"
 * 	will return 1.
 */
static	int
get_module(char * vname)
{
    char *cp;

    if ((cp = strstr (vname, "module/")) == NULL)
      	return -1;

    cp += 7;
    if ((*cp < '0') || (*cp > '9'))
      	return -1;

    return (atoi (cp));
}

/* Parses a hw graph pathname string and returns the I/O slot 
 * number, or -1 if not found.
 * e.g. given the name "/hw/module/1/slot/io3/xwidget/pci/0"
 * 	will return 3.
 */
static int
get_ioslot(char *vname)
{
    char *cp;

    if ((cp = strstr (vname, "slot/io")) == NULL)
      	return -1;

    cp += 7;
    if ((*cp < '0') || (*cp > '9'))
      	return -1;

    return (atoi (cp));
}

int
hps_attach(vertex_hdl_t vhdl)
{
    int            lockunit;
    hps_soft_t     *soft;
    hippi_vars_t   *hippi_devp;
    pciio_info_t    info;
    pciio_slot_t    slot;
    vertex_hdl_t    regdev;
    vertex_hdl_t    srcvhdl;
    vertex_hdl_t    xconnvhdl;
    graph_error_t   rc;
    char            vname[256];
    __uint32_t	    sbar0, dbar0;
#ifdef NO_NIC_PARTNUM_YET
    char	   *nic_string;
#endif
    int		    module, ioslot;
#ifndef HUB1_INTR_WAR
    device_desc_t   dev_desc;
#endif
    volatile uint32_t 	*himrp;

    vertex_to_name(vhdl, vname, sizeof(vname));
    dprintf(5, ("hps_attach(%s)\n", vname));

    /* Get the BRIDGE NIC info from the pci bridge. */
    rc = hwgraph_traverse(vhdl, "../..", &xconnvhdl);
    if (rc != GRAPH_SUCCESS) {
	cmn_err( CE_ALERT,
		"hps_attach(): %v: Could not find hwgraph vertex for PCI Bridge connect point.\n", vhdl);
	return -1;
    }

#ifdef NO_NIC_PARTNUM_YET
    /* For now, assume any LINC card is HIPPI. */
    rc = hwgraph_info_get_LBL (xconnvhdl, INFO_LBL_NIC,
				   (arbitrary_info_t *) &nic_string);
    if (rc == GRAPH_SUCCESS)
	dprintf (1, ("hps_attach(): NIC info string: %s\n", nic_string));
    else
	cmn_err(CE_ALERT,
		"hps_attach():%v: No NIC info available!\n", xconnvhdl);
#else
    if (!nic_vertex_info_match(xconnvhdl, "Part:030-0968-")) {
	dprintf (1, ("hps_attach(): LINC, but not HIPPI-Serial!\n"));
	return -1;
    }
#endif

    /*
     * Find slot number. We will do all our initialization when
     * called for slot 0 (dst LINC). If slot 1 (src LINC) pass for now. 
     */
    info = pciio_info_get(vhdl);
    ASSERT(info != NULL);
    slot = pciio_info_slot_get(info);
    dprintf(5, ("hps_attach (slot = %d)\n", slot));
    if (slot != 0)
	return 0;

    module = get_module(vname);
    ioslot = get_ioslot(vname);

    dprintf(5, ("hps_attach (module = %d, I/O slot = %d)\n",
		 module, ioslot));

    /*
     * Reach up & around and claim the vertex representing the
     * src LINC in slot 1. 
     */
    rc = hwgraph_traverse(vhdl, "../1", &srcvhdl);
    if (rc != GRAPH_SUCCESS) {
	cmn_err( CE_ALERT,
		"hps_attach(): %v: Could not find hwgraph vertex for src LINC on HIPPI-Serial card.\n", vhdl );
	return -1;
    }

    /*
     * Set up our per-device information for this vertex. 
     */
    hippi_devp = kmem_zalloc(sizeof (hippi_vars_t), KM_SLEEP);
    ASSERT(hippi_devp != NULL);
    soft = kmem_zalloc(sizeof (hps_soft_t), KM_SLEEP);
    ASSERT(soft != NULL);
    soft->hippi_devp = hippi_devp;
    hippi_devp->dst_vhdl = vhdl;	/* Destination LINC, slot 0 */
    hippi_devp->src_vhdl = srcvhdl;	/* Source LINC in slot 1 */
    dprintf(5,("hps_attach: slot0 vtx=%x, slot1 vtx=%x\n", vhdl, srcvhdl));

    hippi_devp->hi_ssleep = kmem_zalloc( 2*sizeof (hipfw_sleep_t),
					 KM_SLEEP | KM_CACHEALIGN);
    hippi_devp->hi_dsleep = hippi_devp->hi_ssleep + 1;
    hippi_devp->hi_ssleep->flags = HIPFW_FLAG_SLEEP;
    hippi_devp->hi_dsleep->flags = HIPFW_FLAG_SLEEP;

    /*
     * Find our PCI CONFIG registers. 
     */

    hippi_devp->dst_cfg = (volatile pci_cfg_hdr_t *) pciio_piotrans_addr
      					(vhdl, 0 /* dev_desc unused */ ,
	 				PCIIO_SPACE_CFG,
	 				0, sizeof(pci_cfg_hdr_t),
	 				0 /* flags */ );
    ASSERT(hippi_devp->dst_cfg != NULL);

    hippi_devp->src_cfg = (volatile pci_cfg_hdr_t *) pciio_piotrans_addr
					(srcvhdl, 0 /* dev_desc unused */ ,
	 				PCIIO_SPACE_CFG,
	 				0, sizeof(pci_cfg_hdr_t),
	 				0 /* flags */ );
    ASSERT(hippi_devp->src_cfg != NULL);

    dprintf(2,("hps_attach(): src cfg space at kvaddr %x, dst at kvaddr %x.\n",
	    hippi_devp->src_cfg, hippi_devp->dst_cfg));

    sbar0 = hippi_devp->src_cfg->bar0;
    dprintf(2, ("hps_attach(): src LINC cmd/stat = 0x%08x\n",
		hippi_devp->src_cfg->stat_cmd));
    dprintf(2, ("hps_attach(): src LINC BAR[0] = 0x%08x\n", sbar0));
    dprintf(2, ("hps_attach(): src LINC BIST/hdr/lat/cache = 0x%08x\n",
		hippi_devp->src_cfg->bhlc.i));
    /* We need to turn on the MEM enable and DMA master enable bits */
    hippi_devp->src_cfg->stat_cmd |= 6;

    dbar0 = hippi_devp->dst_cfg->bar0;
    dprintf(2, ("hps_attach(): dst LINC cmd/stat = 0x%08x\n",
		hippi_devp->dst_cfg->stat_cmd));
    dprintf(2, ("hps_attach(): dst LINC BAR[0] = 0x%08x\n", dbar0));
    dprintf(2, ("hps_attach(): dst LINC BIST/hdr/lat/cache = 0x%08x\n",
		hippi_devp->dst_cfg->bhlc.i));
    /* We need to turn on the MEM enable and DMA master enable bits */
    hippi_devp->dst_cfg->stat_cmd |= 6;

    /* To get to LINC regs we have to go through mapped big windows: */
    hippi_devp->piomap = pciio_piomap_alloc(vhdl, 0, PCIIO_SPACE_MEM32,
				      dbar0, 2 * _LINC_ADDRSPC_SIZE,
				      2 * _LINC_ADDRSPC_SIZE, 0);
    ASSERT (hippi_devp->piomap != NULL);

    hippi_devp->dst_lincregs = (volatile __uint32_t *) pciio_piomap_addr(
				hippi_devp->piomap,
				dbar0 + LINC_MISC_REGS_ADDR,
				LINC_PROM_ADDR - LINC_MISC_REGS_ADDR);
    dprintf(1,("hps_attach(): dst LINC regs at kvaddr %x.\n", 
		hippi_devp->dst_lincregs));
    ASSERT (hippi_devp->dst_lincregs != NULL);

#ifdef USE_MAILBOX
    /* map dest mailbox register */
    hippi_devp->dst_mbox = (volatile __uint64_t *) pciio_piomap_addr(
				hippi_devp->piomap,
				dbar0 + LINC_MAILBOX_ADDR,
				LINC_MAILBOX_PGSIZE * LINC_NUM_MAILBOXES);

    dprintf(1,("hps_attach(): dst LINC mailbox at kvaddr %x.\n",
	       hippi_devp->dst_mbox));
#endif

    hippi_devp->dst_eeprom = pciio_piomap_addr( hippi_devp->piomap,
					    dbar0 + LINC_PROM_ADDR,
					    _LINC_PROM_SIZE);
    dprintf(1,("hps_attach(): dst EEPROM at kvaddr %x.\n", 
		hippi_devp->dst_eeprom));
    ASSERT (hippi_devp->dst_eeprom != NULL);
    /*
     * LINC SDRAMs are 4MB each, and src fw has the pio-write data
     * structures spread all over that, and swindow will only map 2MB
     * per device, so we need to go through a big window.
     */
    hippi_devp->dst_bufmem = (volatile void *) pciio_piomap_addr (
					    hippi_devp->piomap,
					    dbar0, _LINC_SDRAM_SIZE);
    dprintf(1,("hps_attach(): dst SDRAM at kvaddr %x.\n", hippi_devp->dst_bufmem));
    ASSERT(hippi_devp->dst_bufmem != NULL);

#if ((DST_HC_OFFSET & 7) != 0)
#error "DST_HC_OFFSET is NOT double-word aligned!"
#endif
    hippi_devp->dst_hc = (volatile struct hip_hc *)
	((char *) hippi_devp->dst_bufmem + DST_HC_OFFSET);
    hippi_devp->dst_stat_area = (volatile hippi_stats_t *) (hippi_devp->dst_hc + 1);

    hippi_devp->src_lincregs = (volatile __uint32_t *) pciio_piomap_addr(
				hippi_devp->piomap,
				sbar0 + LINC_MISC_REGS_ADDR,
				LINC_PROM_ADDR - LINC_MISC_REGS_ADDR);

    dprintf(1,("hps_attach(): src LINC regs at kvaddr %x.\n", 
		hippi_devp->src_lincregs));
    ASSERT (hippi_devp->src_lincregs != NULL);

#ifdef USE_MAILBOX
    /* map src mailbox register. */
    hippi_devp->src_mbox = (volatile __uint64_t *) pciio_piomap_addr(
				hippi_devp->piomap,
				sbar0 + LINC_MAILBOX_ADDR,
				LINC_MAILBOX_PGSIZE * LINC_NUM_MAILBOXES);

    dprintf(1,("hps_attach(): src LINC mailbox at kvaddr %x.\n",
	       hippi_devp->src_mbox));
#endif

    hippi_devp->src_eeprom = pciio_piomap_addr( hippi_devp->piomap,
					    sbar0 + LINC_PROM_ADDR,
					    _LINC_PROM_SIZE);
    dprintf(1,("hps_attach(): src EEPROM at kvaddr %x.\n", 
		hippi_devp->src_eeprom));
    ASSERT (hippi_devp->src_eeprom != NULL);

    hippi_devp->src_bufmem = (volatile void *) pciio_piomap_addr (
					    hippi_devp->piomap,
					    sbar0, _LINC_SDRAM_SIZE);
    dprintf(5,("hps_attach(): src SDRAM at kvaddr %x.\n", hippi_devp->src_bufmem));
    ASSERT(hippi_devp->src_bufmem != NULL);

#if ((SRC_HC_OFFSET & 7) != 0)
#error "SRC_HC_OFFSET is NOT double-word aligned!"
#endif

    hippi_devp->src_hc = (volatile struct hip_hc *)
	((char *) (hippi_devp->src_bufmem) + SRC_HC_OFFSET);
    hippi_devp->src_stat_area = (volatile hippi_stats_t *) (hippi_devp->src_hc + 1);

    hps_readversions (hippi_devp);

    if (!hippi_devp->hi_srcvers.parts.is_src) {
	cmn_err (CE_ALERT,
"hps_attach(): %v: source-side controller EEPROM does not hold source firmware!\n",
		 hippi_devp->src_vhdl);
    }

    if (hippi_devp->hi_dstvers.parts.is_src) {
	cmn_err (CE_ALERT,
"hps_attach(): %v: destination-side controller EEPROM does not hold destination firmware!\n",
		 hippi_devp->dst_vhdl);
    }

    if ( (hippi_devp->hi_srcvers.parts.major != 
	  hippi_devp->hi_dstvers.parts.major) ||
	 (hippi_devp->hi_srcvers.parts.minor != 
	  hippi_devp->hi_dstvers.parts.minor) ) {
	cmn_err (CE_ALERT,
"hps_attach(): %v: source side firmware version (%d.%d) does not match destination firmware version (%d.%d)\n",
		 hippi_devp->dst_vhdl,
		 hippi_devp->hi_srcvers.parts.major,
		 hippi_devp->hi_srcvers.parts.minor,
		 hippi_devp->hi_dstvers.parts.major,
		 hippi_devp->hi_dstvers.parts.minor);
    }

#ifndef HUB1_INTR_WAR
    /* Mask off all interrupts - host-ints will be unmasked when we bring up
       the board. */
    /* This should not be needed - masks supposed to be on at reset,
       according to spec, but they don't appear to be in practice. */
    himrp = hippi_devp->src_lincregs +
	   ((LINC_HOST_INTERRUPT_MASK-LINC_MISC_REGS_ADDR)/sizeof(uint32_t));
    *himrp = 0xffffffff;
    dprintf(5, ("hps_attach(): src LINC interrupts masked (0x%x).\n", *himrp));

    himrp = hippi_devp->dst_lincregs +
	   ((LINC_HOST_INTERRUPT_MASK-LINC_MISC_REGS_ADDR)/sizeof(uint32_t));
    *himrp = 0xffffffff;
    dprintf(5, ("hps_attach(): dst LINC interrupts masked (0x%x).\n",*himrp));

    /*
     * Set up our interrupt. Both LINCs interrupt on INTA. 
     */
    /* Far as I can tell, getting a dev_desc only serves the purpose
     * of associating a name with the interrupt?
     */
    dev_desc = device_desc_dup (vhdl_to_dev(vhdl));
    device_desc_intr_name_set (dev_desc, "hps_ dst");
    hippi_devp->dst_intr = pciio_intr_alloc(vhdl, /* device getting intr */
				      dev_desc, PCIIO_INTR_LINE_A,
				      vhdl /* owner vertex */ );
    device_desc_free(dev_desc);
    if (hippi_devp->dst_intr) {
	dprintf (5, ("hps_attach(): dst intr successfully allocated.\n"));
	if (pciio_intr_connect (hippi_devp->dst_intr, hps_dst_intr,
			        (intr_arg_t) hippi_devp, (void *) 0) < 0)
	    cmn_err (CE_ALERT, "hps_attach: %v: pciio_intr_connect() failed.\n", vhdl );
	else
	    dprintf (5, ("hps_attach(): dst intr successfully connected.\n"));
    }
    else
        cmn_err (CE_ALERT, "hps_attach: %v: pciio_intr_alloc() for dst LINC failed.\n", vhdl );


    dev_desc = device_desc_dup (vhdl_to_dev(srcvhdl));
    device_desc_intr_name_set (dev_desc, "hps_ src");
    hippi_devp->src_intr = pciio_intr_alloc(srcvhdl, /* device getting intr */
				      dev_desc, PCIIO_INTR_LINE_A,
				      srcvhdl /* owner vertex */ );
    device_desc_free(dev_desc);
    if (hippi_devp->src_intr) {
	dprintf (5, ("hps_attach(): src intr successfully allocated.\n"));
    	if (pciio_intr_connect(hippi_devp->src_intr, hps_src_intr,
			   (intr_arg_t) hippi_devp, (void *) 0) < 0)
	    cmn_err (CE_ALERT,
		     "hps_attach: %v: pciio_intr_alloc() failed.\n", srcvhdl);
	else
	    dprintf (5, ("hps_attach(): src intr successfully connected.\n"));
    }
    else
        cmn_err (CE_ALERT, "hps_attach: %v: pciio_intr_alloc() for src LINC failed.\n", srcvhdl );
#endif /*  HUB1_INTR_WAR */

    pciio_error_register(vhdl, hps_error_handler, hippi_devp);
    pciio_error_register(srcvhdl, hps_error_handler, hippi_devp);

    lockunit = atomicAddInt(&hps_nextunit, 1)	- 1;
    hippi_devp->unit = -1;

    /* Reset the Bridge RRB and device X registers. 
     *
     * XXX   Once we get 
     *	     the programmed NICs, we could _try_ to  use a nic callback
     *	     routine to set 
     *	     our config requirements in pci hints structure so that pcibr
     *	     will do the right thing for us.
     *
     *       Unfortunately, we have LINC 0 and LINC 1 appearing as
     *	     devices 0 and 1 respectively where IDSEL and INT
     *	     are concerned. But for DMA, LINC 0 uses the req/grant
     *	     pairs for devices 0 and 1, and LINC 1 uses the req/grant
     * 	     pairs of devices 2 and 3. I can't even _begin_ to explain
     *	     this to the PCI infrastructure...
     */

    {
	bridge_t	*bregs;

	bregs = (bridge_t * ) xtalk_piotrans_addr (xconnvhdl, NULL, 0,
						    sizeof(bridge_t), 0);

	dprintf(1, ("hps_attach(): bridge addr = 0x%x\n", bregs));

	/* 
	 * pcibr doesn't know about "devices" 2 and 3, so we have
	 * to set the deviceX registers for those.
	 * Turn on Coherency bit. Turn OFF dis_page_chk bit.
	 * Enable write gather only for device 3 (LINC1 DMA1)
	 */
	bregs->b_device[0].reg = (bregs->b_device[0].reg & BRIDGE_DEV_OFF_MASK)
				 | BRIDGE_DEV_ERR_LOCK_EN
				 | BRIDGE_DEV_VIRTUAL_EN
				 | BRIDGE_DEV_COH
				 | BRIDGE_DEV_DEV_IO_MEM;

	bregs->b_device[1].reg = (bregs->b_device[1].reg & BRIDGE_DEV_OFF_MASK)
				 | BRIDGE_DEV_ERR_LOCK_EN
				 | BRIDGE_DEV_VIRTUAL_EN
				 | BRIDGE_DEV_COH
				 | BRIDGE_DEV_DEV_IO_MEM;

	bregs->b_device[2].reg = (bregs->b_device[2].reg & BRIDGE_DEV_OFF_MASK)
				 | BRIDGE_DEV_ERR_LOCK_EN
				 | BRIDGE_DEV_VIRTUAL_EN
				 | BRIDGE_DEV_COH
				 | BRIDGE_DEV_DEV_IO_MEM;

	bregs->b_device[3].reg = (bregs->b_device[3].reg & BRIDGE_DEV_OFF_MASK)
				 | BRIDGE_DEV_ERR_LOCK_EN
				 | BRIDGE_DEV_VIRTUAL_EN
				 | BRIDGE_DEV_COH
				 | BRIDGE_DEV_DEV_IO_MEM
				 | BRIDGE_DEV_DIR_WRGA_EN;

	dprintf (5, ("hps_attach(): breg->b_device[0:3] = %x, %x, %x, %x\n",
		     bregs->b_device[0].reg, bregs->b_device[1].reg,
		     bregs->b_device[2].reg, bregs->b_device[3].reg));


	/* 	 2 RRBs each to "dev 0" (LINC0 DMA0) and "dev 1" (LINC0 DMA1)
	 *	 and 6 each for "dev 2" (LINC1 DMA0) and "dev 3" 
	 * 	 (LINC1 DMA1). LINC1 is the source side and does more reads.
	 *       All the buffers have been allocated to virtual channel 0
	 *       as fw does not use 2 V.C.s
	 */
	bregs->b_even_resp = 0x88999999;
	bregs->b_odd_resp  = 0x88999999;
	dprintf (5, ("hps_attach(): bridge even/odd resp buffers = %x/%x\n",
		     bregs->b_even_resp, bregs->b_odd_resp));

	/* Associate LINC0 DMA0 ("device 0") with INT-0
	 *           LINC1 DMA1 ("device 3") with INT-1
	 * for flush-on-interrupts. 
	 * This must be done AFTER the pciio_intr_* calls above
	 * to over-write those settings.
	 */
	bregs->b_int_device = 0x18;
	dprintf (5, ("hps_attach(): bridge int dev reg = %x\n",
		     bregs->b_int_device) );


	/* Set the PCI_RETRY_CNT = 0 to defeat retry timeout if Bridge Rev
	 * is less than Rev D.
	 */
	if (bregs->b_widget.w_id>>28 < 4) {
	    dprintf(5, ("hps_attach(): Bridge Rev = 0x%x, setting PCI_RETRY_CNT = 0\n", 
			bregs->b_widget.w_id>>28));
	    bregs->b_bus_timeout &= ~0x3ff;
		    
	}
    }

    /* initialize these locks only once */
    initsema(&hippi_devp->devsema, 1);/* Upper level sleep lock */
    init_mutex (&hippi_devp->sv_mutex, MUTEX_DEFAULT, "hps_svmutex", lockunit);

    init_spinlock(&hippi_devp->dhc_slock, "hps_dhcsl", lockunit);
    init_mutex (&hippi_devp->dst_mutex, MUTEX_DEFAULT, "hps_dmutex", lockunit);

    init_spinlock(&hippi_devp->shc_slock, "hps_shcsl", lockunit);
    init_mutex (&hippi_devp->src_mutex, MUTEX_DEFAULT, "hps_srcsl", lockunit);

    /*
     * Set up board/host communications areas in host memory. 
     */

    /* Source-LINC-to-host ring: */
    hippi_devp->hi_s2h = (volatile hip_b2h_t *)
	kvpalloc((HIP_S2H_LEN * sizeof(hip_b2h_t) + NBPP - 1) /
		 NBPP, VM_DIRECT | VM_STALE, 0);

    /* Dest-LINC-to-host ring: */
    hippi_devp->hi_d2h = (volatile hip_b2h_t *)
	kvpalloc((HIP_D2H_LEN * sizeof(hip_b2h_t) + NBPP - 1) /
		 NBPP, VM_DIRECT | VM_STALE, 0);

    /* Data to board (for src LINC): */
    hippi_devp->hi_d2b = (volatile hip_d2b_t *)
	kvpalloc((HIP_D2B_LEN * sizeof(hip_d2b_t) + NBPP - 1) /
		 NBPP, VM_DIRECT | VM_STALE, 0);

    /* Control to board (for dst LINC): */
    hippi_devp->hi_c2b = (volatile hip_c2b_t *)
	kvpalloc((HIP_C2B_LEN * sizeof(hip_c2b_t) + NBPP - 1) /
		 NBPP, VM_DIRECT | VM_STALE, 0);

#ifdef PEER_TO_PEER_DMA_WAR
    /* If we aren't doing peer to peer in the board we need to allocate
       the messaging space in the host memory. */
    
    /* Message area for src */
    hippi_devp->hi_src_msg_area = (volatile __uint64_t *)
	kvpalloc(sizeof(__uint64_t), VM_DIRECT | VM_STALE, 0);

    *hippi_devp->hi_src_msg_area = 0;

    dprintf(5,("kvpalloc addr for src is 0x%llx\n",
	       hippi_devp->hi_src_msg_area));
    
    /* Message area for dst */
    hippi_devp->hi_dst_msg_area = (volatile __uint64_t *)
	kvpalloc(sizeof(__uint64_t), VM_DIRECT | VM_STALE, 0);

    *hippi_devp->hi_dst_msg_area = 0;

    dprintf(5,("kvpalloc addr for dst is 0x%llx\n",
	       hippi_devp->hi_dst_msg_area));
#else
    /* If we aren't using them initialize the fields to 0. */
    hippi_devp->hi_src_msg_area = 0;
    hippi_devp->hi_dst_msg_area = 0;
#endif

    /* Create device to open for regular hippi */
    regdev = GRAPH_VERTEX_NONE;
    rc = hwgraph_char_device_add(vhdl, "hippi", "hps_", &regdev);
    if (rc != GRAPH_SUCCESS) {
	cmn_err(CE_ALERT,
		"hps_attach(): %v: Could not add device vertex for hps_ driver: ret_code=%d, new_dev=%x\n", vhdl, rc, regdev);
	return -1;
    }
    ASSERT(regdev != GRAPH_VERTEX_NONE);
    hwgraph_fastinfo_set(regdev, (arbitrary_info_t) soft);
    hippi_devp->dev_vhdl = regdev;


    dprintf(5, ("hps_attach(): hippi_devp=%x, vertices  src=%x, dst=%x, dev=%x.\n",
		hippi_devp, srcvhdl, vhdl, regdev));
    dprintf(1, ("hps_attach(unit %d): src hc area = %x\n", 
		 hippi_devp->unit, hippi_devp->src_hc));
    dprintf(1, ("hps_attach(unit %d): dst hc area = %x\n", 
		 hippi_devp->unit, hippi_devp->dst_hc));
    dprintf(1, ("hps_attach(unit %d): src-to-host area = %x\n", 
		 hippi_devp->unit, hippi_devp->hi_s2h));
    dprintf(1, ("hps_attach(unit %d): dst-to-host area = %x\n", 
		 hippi_devp->unit, hippi_devp->hi_d2h));
    dprintf(1, ("hps_attach(unit %d): data-to-src area = %x\n", 
		 hippi_devp->unit, hippi_devp->hi_d2b));
    dprintf(1, ("hps_attach(unit %d): cntl-to-dst area = %x\n", 
		 hippi_devp->unit, hippi_devp->hi_c2b));

    dprintf (2, ("hps_attach(%d): Dst LINC LCSR = %08x\n", hippi_devp->unit,
		*(hippi_devp->dst_lincregs + (LINC_LCSR-LINC_MISC_REGS_ADDR)/sizeof(uint32_t))));
    dprintf (2, ("hps_attach(%d): Src LINC LCSR = %08x\n", hippi_devp->unit, 
		*(hippi_devp->dst_lincregs + (LINC_LCSR-LINC_MISC_REGS_ADDR)/sizeof(uint32_t))));
      
    /* Add to inventory */
    hwgraph_inventory_add (regdev, INV_NETWORK, INV_NET_HIPPI, INV_HIPPIS_XTK,
			   hippi_devp->unit,
			   ((module << 16) | (ioslot & 0xffff)));


#ifdef HPS_DEBUG
    idbg_addfunc("hippidev",  hippidev_idbg);
    idbg_addfunc("hippivars", hippivars_idbg);
    idbg_addfunc("hippisoft", hippisoft_idbg);
#endif
    return 0;			/* attach successsful */
}


/* =====================================================================
 *		INTERRUPT ENTRY POINTS
 */


/* XXX: what about other (error) bits in the HISR ? */
/*
 * hps_src_intr()
 * 	Interrupt handler for source side (LINC 1).
 *
 * Locking notes:
 *    Since all interrupts from a device are only directed to a
 *    single CPU, and since further interrupts are blocked till
 *    this handler completes, we don't need any locking for vars
 *    that are manipulated only in this routine and nowhere else
 *    i.e. the s2h variables (hi_s2hp and hi_s2h_sn) or the
 *    hi_d2b_cons. If this driver is ever changed so that these
 *    variables are manipulated elsewhere, the locking issue has
 *    to be revisited.
 */
void
hps_src_intr (intr_arg_t arg)
{
    int  i, s;
    volatile struct hip_b2h *b2hp;
#ifdef HPS_DEBUG
    int	    work;
#endif
    hippi_vars_t * hippi_devp;
    volatile uint32_t 	*hisrp;

    hippi_devp = (hippi_vars_t *) arg;

    /* Turn off the interrupt */
    hisrp = hippi_devp->src_lincregs +
	   ((LINC_HOST_INTERRUPT_STATUS-LINC_MISC_REGS_ADDR)/sizeof(uint32_t));

    *hisrp = LINC_HISR_TO_HOST_INT_MASK;

#ifdef HPS_DEBUG
    work = 0;
    hippi_devp->stat_s2h_calls++;
#endif

    /* First see if controller has stopped polling work queue and
     * there's  work pending.
     */
    if ((hippi_devp->hi_ssleep->flags & HIPFW_FLAG_SLEEP) &&
	(hippi_devp->hi_ssleep->index != 
	 		(hippi_devp->hi_d2b_prod - hippi_devp->hi_d2b))) {
	s = HPS_SRCLOCK_HC;
	if (HPS_SRCWAIT > 0) {
	    hippi_devp->hi_ssleep->flags &= ~HIPFW_FLAG_SLEEP;
	    HPS_SRCOP (HCMD_WAKEUP);
#ifdef HPS_DEBUG
	    hippi_devp->stat_s2h_pokes++;
#endif
	}
	HPS_SRCUNLOCK_HC(s);
    }

    b2hp = hippi_devp->hi_s2hp;
    for (;;) {
	if ( b2hp->b2h_sn != hippi_devp->hi_s2h_sn ) { /* no more work */
	    hippi_devp->hi_s2hp = b2hp;
#ifdef HPS_DEBUG
	    hippi_devp->stat_s2h_work += work;
#endif
	    return;
	} /* if no more work */
#ifdef HPS_DEBUG
	work++;
#endif
	switch ( b2hp->b2h_op & HIP_B2H_OPMASK ) {

	case HIP_B2H_ODONE:
	    /* Xmt complete, so free up buffers */
	    i = b2hp->b2h_ndone;

	    while ( i-- > 0 ) {
		volatile struct hip_d2b_hd *hd;

		do {
		    hd = &hippi_devp->hi_d2b_cons->hd;

		    ASSERT( hd->flags & HIP_D2B_RDY );

		    hippi_devp->hi_d2b_cons += hd->chunks+1;
		    if ( hippi_devp->hi_d2b_cons > hippi_devp->hi_d2b_last )
			hippi_devp->hi_d2b_cons -= HIP_D2B_LEN;
		} while ( hd->flags & HIP_D2B_NACK );

		ASSERT( (b2hp->b2h_op&HIP_B2H_STMASK) == hd->stk );

		if ( hd->stk == HIP_STACK_LE )
		    ifhip_le_odone( hippi_devp, hd, b2hp->b2h_ostatus );
		else
		    hps_fp_odone( hippi_devp, hd, b2hp->b2h_ostatus );
	    }	/* while ( i-- > 0 ) */
	    break;

	default:
	    cmn_err( CE_WARN, "hps%d: unknown op/stk from src LINC: %d\n",
		     hippi_devp->unit, b2hp->b2h_op );
	    break;

	}   /* 	switch ( b2hp->b2h_op & HIP_B2H_OPMASK ) */

	hippi_devp->hi_s2h_sn++;
	b2hp++;
	if ( b2hp > hippi_devp->hi_s2h_last )
	    b2hp = & hippi_devp->hi_s2h[0];

    } /* for (;;) */
}

/* XXX Check locking needs for this - this routine is single threaded,
 *     so no lock needed for hi_d2bp, hi_d2b_sn. How about fp_input
 *     and le_input?
 */
/* Interrupt handler for dst side. */
void
hps_dst_intr (intr_arg_t arg)
{
    int	    s;

    hippi_vars_t * hippi_devp;
    volatile uint32_t 	*hisrp;
    volatile struct hip_b2h *b2hp;
#ifdef HPS_DEBUG
    int	    work;
#endif

    hippi_devp = (hippi_vars_t *) arg;

    /* Turn off the interrupt */
    hisrp = hippi_devp->dst_lincregs +
	   ((LINC_HOST_INTERRUPT_STATUS-LINC_MISC_REGS_ADDR)/sizeof(uint32_t));

    *hisrp = LINC_HISR_TO_HOST_INT_MASK;

#ifdef HPS_DEBUG
    work = 0;
    hippi_devp->stat_d2h_calls++;
#endif

    /* First see if controller has stopped polling work queue and
     * there's work pending. */
    if ((hippi_devp->hi_dsleep->flags & HIPFW_FLAG_SLEEP) &&
	(hippi_devp->hi_dsleep->index != 
	 		(hippi_devp->hi_c2bp - hippi_devp->hi_c2b))) {
	s = HPS_DSTLOCK_HC;
	if (HPS_DSTWAIT > 0) {
	    hippi_devp->hi_dsleep->flags &= ~HIPFW_FLAG_SLEEP;
	    HPS_DSTOP (HCMD_WAKEUP);
#ifdef HPS_DEBUG
	    hippi_devp->stat_d2h_pokes++;
#endif
	}
	HPS_DSTUNLOCK_HC(s);
    }

    b2hp = hippi_devp->hi_d2hp;
    for (;;) {
	if ( b2hp->b2h_sn != hippi_devp->hi_d2h_sn ) { /* no more work */
	    ifhip_fillin(hippi_devp);
	    hippi_devp->hi_d2hp = b2hp;
#ifdef HPS_DEBUG
	    hippi_devp->stat_d2h_work += work;
#endif
	    return;
	} /* if no more work */
#ifdef HPS_DEBUG
	work++;
#endif
	switch ( b2hp->b2h_op & HIP_B2H_OPMASK ) {

	case HIP_B2H_IN_DONE:
	    if ( (b2hp->b2h_op&HIP_B2H_STMASK) == HIP_STACK_LE ) {
		/* board wakes up if it gives us HIPPI-LE input */
		ifhip_le_input( hippi_devp, b2hp );
	    }
	    else
		hps_fp_input( hippi_devp, b2hp );
	    break;

	case HIP_B2H_IN:
	    hps_fp_input( hippi_devp, b2hp );
	    break;

#ifdef HIPPI_BP
	case HIP_B2H_BP_PORTINT:
	    hippibp_portint(hippi_devp->bp_vhdl, b2hp->b2h_s, b2hp->b2h_l);
	    break;
#endif /* HIPPI_BP */

	default:
	    cmn_err( CE_WARN, "hps%d: unknown op/stk from dst LINC: %d\n",
		     hippi_devp->unit, b2hp->b2h_op );
	    break;

	}   /* 	switch ( b2hp->b2h_op & HIP_B2H_OPMASK ) */

	hippi_devp->hi_d2h_sn++;
	b2hp++;
	if ( b2hp > hippi_devp->hi_d2h_last )
	    b2hp = & hippi_devp->hi_d2h[0];

    } /* for (;;) */

}


/* ==================================================================
 *	INTERFACE ROUTINES to be called by hippibp driver.
 */
/*
 * Return some privileged friendinfo stuff to a trusted co-driver.
 * The magic token "cookie" is actually a pointer to our private
 * data structure.
 *
 * Args are:
 *  bpdev - device vertex of driver wanting this info
 *  fp	  - address of friendinfo struct the values are to be stuffed in
 *  cookie- magic token we gave to bypass driver when we called its
 *	    attach routine.
 *
 *  Return value is fp if all went well, NULL if cookie didn't check out.
 */
hps_friend_info_t *
hps_get_friendinfo (vertex_hdl_t bpdev,
		    hps_friend_info_t *fp,
		    void * cookie)
{
    hippi_vars_t *hippi_devp;

    hippi_devp = (hippi_vars_t *) cookie;
    if ((hippi_devp == NULL) || (hippi_devp->bp_vhdl != bpdev))
	return NULL;

    fp->unit	  = hippi_devp->unit;

    fp->shc_slock = &hippi_devp->shc_slock;
    fp->dhc_slock = &hippi_devp->dhc_slock;

    fp->shc_area  = hippi_devp->src_hc;
    fp->dhc_area  = hippi_devp->dst_hc;

    fp->sbufmem	  = hippi_devp->src_bufmem;
    fp->dbufmem	  = hippi_devp->dst_bufmem;

    fp->scnctpt   = hippi_devp->src_vhdl;
    fp->dcnctpt   = hippi_devp->dst_vhdl;

    return fp;
}

/* Next 4 routines must be called with appropriate hc lock held.
 * The model is:
 *  - grab src hc lock
 *  - if (hps_srcwait() < 0) release lock, take recovery action else
 *  - stuff hc area params
 *  - hps_srchwop()
 *  - release src hc lock
 */

/* ARGSUSED */
int
hps_srcwait(int usecs, void * cookie)
{
    return hps_wait_usec ((hippi_vars_t *) cookie, usecs, 1);
}

void
hps_srchwop(int op, void * cookie)
{
    hippi_vars_t * hippi_devp = (hippi_vars_t *) cookie;

    *(__uint64_t *) &hippi_devp->src_hc->cmd = 
                     (((__uint64_t)op << 32) | (++hippi_devp->hi_src_cmd_id));
#if 0
    hippi_devp->src_hc->cmd = op;
    hippi_devp->src_hc->cmd_id = ++hippi_devp->hi_src_cmd_id;
#endif
}

/* Must be called with dst hc lock held. */
int
hps_dstwait(int usecs, void *cookie)
{
    return hps_wait_usec ((hippi_vars_t *) cookie, usecs, 0);
}

void
hps_dsthwop(int op, void * cookie)
{
    hippi_vars_t * hippi_devp = (hippi_vars_t *) cookie;

    *(__uint64_t *) &hippi_devp->dst_hc->cmd = 
                     (((__uint64_t)op << 32) | (++hippi_devp->hi_dst_cmd_id));
#if 0
    hippi_devp->dst_hc->cmd = op;
    hippi_devp->dst_hc->cmd_id = ++hippi_devp->hi_dst_cmd_id;
#endif
}


/* ===================================================================
 *			HIPPI hardware control
 */

/* There are a set of synchronous commands sent down to the board.
 * They do things like initialization, assigning/deassing ULPs, and
 * "waking" the board to notify it that there is work to be done in the
 * work queues (d2b for transmit, c2b for receive and receive buffer
 * management).  The commands are done by writing arguments
 * directly into the board's memory and incrementing the cmd_id.
 * The work queues, on the other hand, are kept in host memory and
 * are DMA'ed by the board
 */

/* Wait up to <wait> usecs for previous command to complete.  This is
 * usually called from within the hippi_wait() macro.  The previous command
 * must complete before you muck with the arguments for the next command or
 * if you need to synchronize before doing something else.
 *
 * Must be called with the appropriate src/dst hc lock held.
 */
int
hps_wait_usec( hippi_vars_t *hippi_devp, int wait, int is_src )
{
    __uint32_t	cmd_ack;
    __uint32_t	match_ack;
    volatile struct hip_hc *hc; 

    if (is_src) {
	hc = hippi_devp->src_hc;
	match_ack = hippi_devp->hi_src_cmd_id;
    }
    else {
	hc = hippi_devp->dst_hc;
	match_ack = hippi_devp->hi_dst_cmd_id;
    }

    while ((cmd_ack = hc->cmd_ack) != match_ack) {
	if (wait < 0) {
	    cmn_err( CE_WARN,
		     "hippi%d: %s 4650 asleep at cmd id=%d: op=%d. Last ack=%d.",
		     hippi_devp->unit, is_src ? "src" : "dst",
		     match_ack, hc->cmd, cmd_ack);
	    cmn_err( CE_CONT,
		    "hippi src error code = %08x, dst error code = %08x\n",
		     hippi_devp->src_hc->sign,
		     hippi_devp->dst_hc->sign);
	    break;
	}
	DELAY(OP_CHECK);
	wait -= OP_CHECK;
    }
    return(wait);
}

/* 
 * Assuming the board has been reset, bring it into operational
 * state. This is called when root does a HIPPI_SETONOFF ioctl.
 * Synchronization with other user ops is through hippi_devp->devsema.
 */
static int
hippi_bd_bringup( hippi_vars_t	* hippi_devp)
{
    int             i;
    paddr_t	    paddr;
    iopaddr_t	    dma_addr;
    volatile __uint32_t * lcsr;
    volatile uint32_t 	*himrp;
    volatile uint32_t 	*hisrp;

    /* Initialize all the global semaphores */
    initnsema(&hippi_devp->rawoutq_sema, HIPPIFP_MAX_WRITES, "hps_rawout");
    initnsema(&hippi_devp->src_sema, 1, "hps_src");

    for (i = 0; i < HIPPIFP_MAX_WRITES; i++)
	initnsema(&hippi_devp->rawoutq_sleep[i], 0, "rawoutq_sleep");

    hippi_devp->rawoutq_in = 0;
    hippi_devp->rawoutq_out = 0;
    for (i = 0; i < HIPPIFP_MAX_WRITES; i++)
	hippi_devp->rawoutq_error[i] = 0;
    hippi_devp->PHmode = 0;

    /* Initialize clone and ulp tables */
    bzero(hippi_devp->clone_info, sizeof(hippi_devp->clone_info));
    bzero(hippi_devp->ulp, sizeof(hippi_devp->ulp));

    /* Initialize ULP ID table */
    for (i = 0; i <= HIPPI_ULP_MAX; i++)
	hippi_devp->ulpFromId[i] = 255;

    /*
     * Zero out communications areas 
     */
    bzero((void *) hippi_devp->hi_d2b, HIP_D2B_LEN * sizeof(hip_d2b_t));
    bzero((void *) hippi_devp->hi_c2b, HIP_C2B_LEN * sizeof(hip_c2b_t));
    bzero((void *) hippi_devp->hi_s2h, HIP_S2H_LEN * sizeof(hip_b2h_t));
    bzero((void *) hippi_devp->hi_d2h, HIP_D2H_LEN * sizeof(hip_b2h_t));

    /*
     * Initialize  "data-to-board" ring.
     */
    hippi_devp->hi_d2b_cons = &hippi_devp->hi_d2b[0];
    hippi_devp->hi_d2b_prod = &hippi_devp->hi_d2b[0];
    hippi_devp->hi_d2b_last = &hippi_devp->hi_d2b[HIP_D2B_LEN - 1];
    hippi_devp->hi_d2b_prod->hd.flags = HIP_D2B_BAD;

    /*
     * Initialize src-to-host interrupt ring.
     */
    hippi_devp->hi_s2h_sn = 1;
    for (i = 0; i < HIP_S2H_LEN; i++)
	hippi_devp->hi_s2h[i].b2h_sn = (u_char) (i & 0xFF);
    hippi_devp->hi_s2hp = &hippi_devp->hi_s2h[0];
    hippi_devp->hi_s2h_last = &hippi_devp->hi_s2h[HIP_S2H_LEN - 1];
#if (HIP_S2H_LEN % 256) == 0
#error "HIP_S2H_LEN can't be a multiple of 256"
#endif

    /*
     * Initialize dst-to-host interrupt ring.
     */
    hippi_devp->hi_d2h_sn = 1;
    for (i = 0; i < HIP_D2H_LEN; i++)
	hippi_devp->hi_d2h[i].b2h_sn = (u_char) (i & 0xFF);
    hippi_devp->hi_d2hp = &hippi_devp->hi_d2h[0];
    hippi_devp->hi_d2h_last = &hippi_devp->hi_d2h[HIP_D2H_LEN - 1];
#if (HIP_D2H_LEN % 256) == 0
#error "HIP_D2H_LEN can't be a multiple of 256"
#endif

    /*
     * Initialize host-to-board control ring 
     */
    hippi_devp->hi_c2bp = &hippi_devp->hi_c2b[0];
    hippi_devp->hi_c2bp->c2b_op = HIP_C2B_EMPTY;

    hippi_devp->hi_src_cmd_id = 1;
    hippi_devp->hi_dst_cmd_id = 1;
    hippi_devp->hi_hwflags = (hippi_devp->hi_hwflags & HIP_FLAG_LOOPBACK) |
	(hippi_ndisc_perr ?
	 (HIP_FLAG_ACCEPT | HIP_FLAG_NODISC) : HIP_FLAG_ACCEPT) ;
    hippi_devp->hi_stimeo = HIPPI_DEFAULT_STIMEO;
    hippi_devp->hi_dtimeo = HIPPI_DEFAULT_DTIMEO;

    /*
     * Make sure both controllers are up and running:
     */
    hippi_devp->hi_bringup_tries++;

    lcsr = hippi_devp->src_lincregs + 
	   (LINC_LCSR-LINC_MISC_REGS_ADDR)/sizeof(uint32_t);

    if (*lcsr & LINC_LCSR_BOOTING) {
        dprintf (1, ("hps%d: src LCSR (%x) still in BOOTING mode.\n", 
		 hippi_devp->unit, *lcsr));
	if (hippi_devp->hi_bringup_tries > HPS_QUIET_INIT_TRIES) {
	    hippi_devp->hi_bringup_tries = 0;
	    cmn_err(CE_WARN,
                "hps%d: HIPPI-Serial source controller stuck in boot mode.\n",
                hippi_devp->unit);
	}
	return EBUSY;
    }
    dprintf (1, ("hps%d: src LCSR (%x) out of BOOTING mode.\n", 
		 hippi_devp->unit, *lcsr));

    lcsr = hippi_devp->dst_lincregs + 
	   (LINC_LCSR-LINC_MISC_REGS_ADDR)/sizeof(uint32_t);
    if (*lcsr & LINC_LCSR_BOOTING) {
        dprintf (1, ("hps%d: dst LCSR (%x) still in BOOTING mode.\n", 
		 hippi_devp->unit, *lcsr));
        if (hippi_devp->hi_bringup_tries > HPS_QUIET_INIT_TRIES) {
            hippi_devp->hi_bringup_tries = 0;
            cmn_err(CE_WARN,
                "hps%d: HIPPI-Serial destination controller stuck in boot mode.\n",
                hippi_devp->unit);
        }
	return EBUSY;
    }
    dprintf (1, ("hps%d: dst LCSR (%x) out of BOOTING mode.\n", 
		 hippi_devp->unit, *lcsr));

    if (hippi_devp->src_hc->sign != HIP_SIGN) {
	dprintf (1, ("hps%d: bad src error code = %08x.\n", 
	     hippi_devp->unit, hippi_devp->src_hc->sign));
        if (hippi_devp->hi_bringup_tries > HPS_QUIET_INIT_TRIES) {
            hippi_devp->hi_bringup_tries = 0;
	    cmn_err(CE_WARN,
	       "hps%d: bad error code (%08x) from HIPPI-Serial source side!",
	        hippi_devp->unit, hippi_devp->src_hc->sign);
	    cmn_err(CE_CONT,
	       "(hps%d:  HIPPI-Serial dest controller error code = %08x)\n",
	        hippi_devp->unit, hippi_devp->dst_hc->sign);
	}
	return EBUSY;
    }
    dprintf (1, ("hps%d: src error code = %08x.\n", 
		 hippi_devp->unit, hippi_devp->src_hc->sign));

    if (hippi_devp->dst_hc->sign != HIP_SIGN) {
	dprintf (1, ("hps%d: bad dst error code = %08x.\n",
	    hippi_devp->unit, hippi_devp->dst_hc->sign));
        if (hippi_devp->hi_bringup_tries > HPS_QUIET_INIT_TRIES) {
            hippi_devp->hi_bringup_tries = 0;
	    cmn_err(CE_WARN,
		"hps%d: bad error code (%08x) from HIPPI-Serial destination side!\n",
		hippi_devp->unit, hippi_devp->dst_hc->sign);
	}
	return EBUSY;
    }
    dprintf (1, ("hps%d: dst error code = %08x.\n",
		 hippi_devp->unit, hippi_devp->dst_hc->sign));

    hippi_devp->hi_bringup_tries = 0;	/* whew, we're up */

    /*
     * Next, initialize the card with all the communications areas and
     * parameters. 
     */
    /* XXX - or in prefetch and vchan bits? */


#ifdef PEER_TO_PEER_DMA_WAR
    /* if we're not using peer to peer messaging we need buffers in host mem */
    
    /* src and dst must know both addrs since they read one and
       write the other */
    paddr = kvtophys((void *) hippi_devp->hi_src_msg_area);
    dma_addr = pciio_dmatrans_addr
			(hippi_devp->src_vhdl, 0, paddr,
			 sizeof(__uint64_t),
			  PCIIO_DMA_DATA | PCIIO_DMA_A64);

    *(__uint64_t *) &hippi_devp->src_hc->arg.init.src_msg_area_hi = dma_addr;
    *(__uint64_t *) &hippi_devp->dst_hc->arg.init.src_msg_area_hi = dma_addr;

    dprintf(5,("pciio_dmatrans_addr for src is 0x%llx\n",dma_addr));

    paddr = kvtophys((void *) hippi_devp->hi_dst_msg_area);
    dma_addr = pciio_dmatrans_addr
			(hippi_devp->dst_vhdl, 0, paddr,
			 sizeof(__uint64_t),
			  PCIIO_DMA_DATA | PCIIO_DMA_A64);

    *(__uint64_t *) &hippi_devp->src_hc->arg.init.dst_msg_area_hi = dma_addr;
    *(__uint64_t *) &hippi_devp->dst_hc->arg.init.dst_msg_area_hi = dma_addr;

    dprintf(5,("pciio_dmatrans_addr for dst is 0x%llx\n",dma_addr));
#else
    /* If we're not using them, initialize the values to 0. */
    *(__uint64_t *) &hippi_devp->src_hc->arg.init.src_msg_area_hi = 0;
    *(__uint64_t *) &hippi_devp->dst_hc->arg.init.src_msg_area_hi = 0;
    *(__uint64_t *) &hippi_devp->src_hc->arg.init.dst_msg_area_hi = 0;
    *(__uint64_t *) &hippi_devp->dst_hc->arg.init.dst_msg_area_hi = 0;
#endif

#define HINIT	hippi_devp->src_hc->arg.init
    /* Source side reads our d2b ring and writes to our s2h ring. */
    paddr = kvtophys((void *) hippi_devp->hi_s2h);
    /* Request BARRIER attr bit on this s2h address */
    dma_addr = pciio_dmatrans_addr
			(hippi_devp->src_vhdl, 0, paddr,
			 (HIP_S2H_LEN * sizeof(hip_b2h_t)),
			  PCIIO_DMA_CMD | PCIIO_DMA_A64);
    ASSERT (dma_addr != NULL);
    *(__uint64_t *) &HINIT.b2h_buf_hi = dma_addr;	/* hi and lo */
    *(__uint64_t *) &HINIT.b2h_len = (__uint64_t)HIP_S2H_LEN<<32 |
				     hippi_devp->hi_hwflags; /* len & iflags */

    /* For the d2b ring We want no BARRIER */
    paddr = kvtophys((void *) hippi_devp->hi_d2b);
    dma_addr = pciio_dmatrans_addr
			(hippi_devp->src_vhdl, 0, paddr,
			 (HIP_D2B_LEN * sizeof(hip_d2b_t)),
			  PCIIO_DMA_DATA | PCIIO_DMA_A64);
    ASSERT (dma_addr != NULL);

    *(__uint64_t *) &HINIT.d2b_buf_hi = dma_addr;	/* hi and lo */
    *(__uint64_t *) &HINIT.d2b_len = (__uint64_t)HIP_D2B_LEN<<32 | NBPP | MLEN;

    /* Save the widget ID for future shortcuts. pciio_dmatrans_addr
     * takes way too long for use on every read and write request.
     * We just OR this with the curproc_vtop()ed address. The fw
     * will twiddle the other bits(PFTCH, PRECISE and VCHAN) as needed.
     */
    hippi_devp->dma_addr = (dma_addr & PCI64_ATTR_TARG_MASK);

    /* Src does not use c2b */
    *(__uint64_t *) &HINIT.c2b_buf_hi = 0;
    *(__uint64_t *) &HINIT.c2b_len = hippi_devp->dst_cfg->bar0;/* len&pcibase */

    paddr = kvtophys((void *) hippi_devp->hi_ssleep);
    dma_addr = pciio_dmatrans_addr
			(hippi_devp->src_vhdl, 0, paddr,
			 (sizeof(hipfw_sleep_t)),
			  PCIIO_DMA_CMD | PCIIO_DMA_A64);
    HINIT.b2h_sleep = dma_addr;

#undef HINIT
    HPS_SRCOP (HCMD_INIT);

    /* Enable interrupts */
    /* XXX - I don't really know what to do with LINC errors - 
             expect firmware to deal with them? May need to unmask
	     more error bits if need arises...
     */
    himrp = hippi_devp->src_lincregs +
	   ((LINC_HOST_INTERRUPT_MASK-LINC_MISC_REGS_ADDR)/sizeof(uint32_t));
    *himrp = ~LINC_HISR_TO_HOST_INT_MASK;

    hisrp = hippi_devp->src_lincregs +
	   ((LINC_HOST_INTERRUPT_STATUS-LINC_MISC_REGS_ADDR)/sizeof(uint32_t));
    *hisrp = LINC_HISR_TO_HOST_INT_MASK;

    /* Now for the dst side: */
#define HINIT	hippi_devp->dst_hc->arg.init
    /* Dest side reads our c2b ring and writes to our d2h ring. */
    paddr = kvtophys((void *) hippi_devp->hi_d2h);
    /* Dst to host ring needs BARRIER op: */
    dma_addr = pciio_dmatrans_addr
			(hippi_devp->dst_vhdl, 0, paddr,
			 (HIP_D2H_LEN * sizeof(hip_b2h_t)),
			  PCIIO_DMA_CMD | PCIIO_DMA_A64);
    ASSERT (dma_addr != NULL);
    *(__uint64_t *) &HINIT.b2h_buf_hi = dma_addr;       /* hi and lo */
    *(__uint64_t *) &HINIT.b2h_len = (__uint64_t)HIP_D2H_LEN<<32 |
                                     hippi_devp->hi_hwflags; /* len & iflags */
    /* control to board: no BARRIER, with PRECISE */
    paddr = kvtophys((void *) hippi_devp->hi_c2b);
    dma_addr = pciio_dmatrans_addr
			(hippi_devp->dst_vhdl, 0, paddr,
			 (HIP_C2B_LEN * sizeof(hip_c2b_t)),
			  PCIIO_DMA_DATA | PCIIO_DMA_A64);
    ASSERT (dma_addr != NULL);

    *(__uint64_t *) &HINIT.c2b_buf_hi = dma_addr;

    *(__uint64_t *) &HINIT.c2b_len = (__uint64_t)HIP_C2B_LEN<<32 |
				     hippi_devp->src_cfg->bar0;/* len&pcibase */

    /* Dest does not use d2b */
    *(__uint64_t *) &HINIT.d2b_buf_hi = 0;
    *(__uint64_t *) &HINIT.d2b_len = (NBPP | MLEN); 	/* host_nbpp_mlen */

    paddr = kvtophys((void *) hippi_devp->hi_dsleep);
    dma_addr = pciio_dmatrans_addr
			(hippi_devp->dst_vhdl, 0, paddr,
			 (sizeof(hipfw_sleep_t)),
			  PCIIO_DMA_CMD | PCIIO_DMA_A64);
    HINIT.b2h_sleep = dma_addr;

    HPS_DSTOP (HCMD_INIT);
#undef HINIT

    /* Enable interrupts */
    /* XXX - I don't really know what to do with LINC errors - 
             expect firmware to deal with them? May need to unmask
	     more error bits if need arises...
     */
    himrp = hippi_devp->dst_lincregs +
	   ((LINC_HOST_INTERRUPT_MASK-LINC_MISC_REGS_ADDR)/sizeof(uint32_t));
    *himrp = ~LINC_HISR_TO_HOST_INT_MASK;	/* mask enabled */

    hisrp = hippi_devp->dst_lincregs +
	   ((LINC_HOST_INTERRUPT_STATUS-LINC_MISC_REGS_ADDR)/sizeof(uint32_t));
    *hisrp = LINC_HISR_TO_HOST_INT_MASK;	/* intrs cleared to trigger */

    /* Set this last of all since this is what all other routines
     * use to determine if the board is up, and this routine is not
     * called with any locks held.
     */
    hippi_devp->hi_state = HI_ST_UP;
    hippibp_bd_up(hippi_devp->bp_vhdl);

    return 0;
}

/* This brings down the HIPPI board and deallocates all the 
 * resources. It even hits the HIPPI board with a reset.
 *
 * This is called (through a HIPPI_SETONOFF ioctl) with 
 * hippi_devp->devsema held. 
 */
static void
hippi_bd_shutdown( hippi_vars_t *hippi_devp )
{
	int	i, j;
	struct hippi_fp_ulps *ulpp;
	volatile    __uint32_t * slcsr;
	volatile    __uint32_t * dlcsr;
	volatile uint32_t 	*himrp;

	hippi_devp->hi_state = 0;

	slcsr = hippi_devp->src_lincregs + 
		(LINC_LCSR-LINC_MISC_REGS_ADDR)/sizeof(uint32_t);
	dlcsr = hippi_devp->dst_lincregs + 
		(LINC_LCSR-LINC_MISC_REGS_ADDR)/sizeof(uint32_t);

	/* Reset the HIPPI board. Hit both LINCs with cold reset. */
	*slcsr = _LINC_RESET;
	*dlcsr = _LINC_RESET;


	/* Tell if_hip() that we're closing shop. */
	ifhip_shutdown( hippi_devp );

	/* Tell bypass we are shutting down */
	hippibp_bd_down(hippi_devp->bp_vhdl);

	/* Free up anybody waiting on source semaphores. */
	mutex_lock (&hippi_devp->sv_mutex, PZERO);
#if 0
/* XXX - this code was in the old driver. But there, the users
 * 	 waking up on these semas did not do vsemas if the state 
 *       was down. In this driver we do, so should not need these
 *       extras here.
 */
	while ( cvsema( & hippi_devp->rawoutq_sema ) )
		;
	for (j=0; j<50; j++)
		vsema( & hippi_devp->rawoutq_sema );
	while ( cvsema( & hippi_devp->src_sema ) )
		;
	for (j=0; j<50; j++)
		vsema( & hippi_devp->src_sema );
#endif
	for (i=0; i<HIPPIFP_MAX_WRITES; i++) {
	    vsema ( &hippi_devp->rawoutq_sleep[i] );
	}

	for (i=0; i<HIPPIFP_MAX_OPEN_ULPS+1; i++) {
	    ulpp = & hippi_devp->ulp[i];
	    if ( ulpp->opens > 0 ) {

		/* Free up anybody waiting on destination semaphores */
		ulpp->rd_semacnt = 1;
		sv_broadcast ( & ulpp->rd_sv );
		while ( cvsema( & ulpp->rd_dmadn ) )
			/* empty */;
                for (j=0; j<50; j++)
                        vsema( & ulpp->rd_dmadn );
		if ( ulpp->ulpFlags & ULPFLAG_R_POLL ) {
			pollwakeup( ulpp->rd_pollhdp,  POLLERR );
			ulpp->ulpFlags &= ~ULPFLAG_R_POLL;
		}
	   }
	}
	mutex_unlock( &hippi_devp->sv_mutex );

	/* Check if there are any open bound fds. If not, also
	 * clean up the write semaphores. */
	for (i=0; i < HIPPIFP_MAX_CLONES; i++)
	    if ( hippi_devp->clone_info[i] &&
		hippi_devp->clone_info[i]->ulpIndex != ULPIND_UNBOUND)
		break;

	if ( i == HIPPIFP_MAX_CLONES ) {

	    freesema( & hippi_devp->rawoutq_sema );
	    freesema( & hippi_devp->src_sema );

	    for (i=0; i<HIPPIFP_MAX_WRITES; i++)
		freesema( & hippi_devp->rawoutq_sleep[i] );
	}

	/* Take the 4640s and RRs out of reset. */
	*slcsr = _LINC_CLR_RESET;
	*dlcsr = _LINC_CLR_RESET;

	/* Mask off interrupts */
	himrp = hippi_devp->src_lincregs +
	   ((LINC_HOST_INTERRUPT_MASK-LINC_MISC_REGS_ADDR)/sizeof(uint32_t));
	*himrp = 0xffffffff;

	himrp = hippi_devp->dst_lincregs +
	   ((LINC_HOST_INTERRUPT_MASK-LINC_MISC_REGS_ADDR)/sizeof(uint32_t));
	*himrp = 0xffffffff;
	hippi_devp->hi_hwflags = 0;

}

/* ====================================================================
 *             DRIVER-to-FIRMWARE queues handler.
 */

/* 
 * hps_send_c2b()
 *
 * Put a control message to dst side firmware into the queue.
 * Put a stopper (invalid opcode) in the entry after this, then
 * fill in this entry, making sure that the opcode goes in last.
 *
 * If fw has stopped polling this queue, send it a HCMD_WAKEUP msg to
 * say that there's work pending.
 */
/* ARGSUSED */
void
hps_send_c2b(hippi_vars_t * hippi_devp, __uint32_t ops,
	     int param, iopaddr_t addr)
{
    volatile hip_c2b_t *c2bp;
    volatile hip_c2b_t *c2bp2;

    HPS_DSTLOCK;
    c2bp = hippi_devp->hi_c2bp;
    c2bp2 = c2bp;

    if ( ++c2bp2 >= & hippi_devp->hi_c2b[ HIP_C2B_LEN-1 ] ) {
	c2bp2->c2b_op = HIP_C2B_WRAP;
	c2bp2 = & hippi_devp->hi_c2b[0];
    }
    c2bp2->c2b_op = HIP_C2B_EMPTY;
    
    /* It's critical that the opcode, "ops", be put in LAST. */
    c2bp->c2b_addr = (__uint64_t) addr;
    * (volatile __uint32_t *) &c2bp->c2b_param = (param<<16)|(ops<<8);

    hippi_devp->hi_c2bp = c2bp2;
    HPS_DSTUNLOCK;

    if (hippi_devp->hi_dsleep->flags & HIPFW_FLAG_SLEEP)
	hps_wake_dst (hippi_devp);

#ifdef USE_MAILBOX
    /* tell firmware we've got some descriptors for it */
    hps_dst_d2b_rdy(hippi_devp);
#endif
}




/* =====================================================================
 *		DRIVER Entry point routines
 */

/* ARGSUSED */
int
hps_open(dev_t *devp, int oflag, int otyp, cred_t *crp)
{
	vertex_hdl_t	vhdl = dev_to_vhdl(*devp);
	hps_soft_t *	soft = hps_soft_get(vhdl);
	hippi_vars_t	*hippi_devp = soft->hippi_devp;
	vertex_hdl_t	newvhdl;
	hps_soft_t *	newsoft;
	graph_error_t	rc;
	int		cloneIndex;

	if (soft->isclone)
		return EBUSY;
	dprintf (5, ("hps_open: %v\n", vhdl));

	psema( & hippi_devp->devsema, PZERO );
	cloneIndex=0;
	while ( cloneIndex < HIPPIFP_MAX_CLONES &&
		hippi_devp->clone_info[cloneIndex])
			cloneIndex++;
	if ( cloneIndex >= HIPPIFP_MAX_CLONES ) {
	    dprintf(5,("hps_open(%d): too many clone fds\n",
		       hippi_devp->unit));
	    vsema( & hippi_devp->devsema );
	    return ENXIO;
	}

	/* Create new vertex for this open. */
	rc = hwgraph_vertex_create(&newvhdl);
	if (rc != GRAPH_SUCCESS) {
		vsema( & hippi_devp->devsema );
		return ENOMEM;
	}
	ASSERT (newvhdl != NULL);

	/* Associate cloned vertex with this driver, without
	 * having it show up in hwgraph */
	hwgraph_char_device_add(newvhdl, NULL, "hps_", NULL);

	newsoft = kmem_zalloc (sizeof (hps_soft_t), KM_SLEEP);
	newsoft->isclone = 1;
	newsoft->cloneid = cloneIndex;
	newsoft->hippi_devp = hippi_devp;
	newsoft->vhdl = newvhdl;
	newsoft->ulpIndex = ULPIND_UNBOUND;
	newsoft->mode = oflag & (FREAD|FWRITE);
	hps_soft_set(newvhdl, newsoft);

	hippi_devp->clone_info[cloneIndex] = newsoft;
	vsema( & hippi_devp->devsema );

	*devp = vhdl_to_dev(newvhdl);
	dprintf(5,("hps_open(%d): dev = %x, flag = 0x%x\n", 
		    hippi_devp->unit, newvhdl, oflag));
	return 0;
}

/* ARGSUSED */
int
hps_close(dev_t dev, int oflag, int otyp, cred_t *crp)
{
    int		    i, ulpIndex, s;
    vertex_hdl_t    vhdl = dev_to_vhdl(dev);
    hps_soft_t	    *soft = hps_soft_get(vhdl);
    hippi_vars_t    *hippi_devp = soft->hippi_devp;
    struct hippi_fp_ulps	*ulpp;

    ASSERT (soft->isclone);
    dprintf(5,("hps_close(%d): dev=%x\n", hippi_devp->unit, dev));

    psema( & hippi_devp->devsema, PZERO );

    ulpIndex = soft->ulpIndex;

    if ( ulpIndex != ULPIND_UNBOUND ) {
	if ( (hippi_devp->hi_state & HI_ST_UP) ) {

	    if ( (soft->cloneFlags & CLONEFLAG_W_NBOP) ||
		 (soft->cloneFlags & CLONEFLAG_W_NBOC) ) {
		hps_send_dummy_desc (hippi_devp, HIP_D2B_RDY | HIP_D2B_NACK);
	    }
	    if ( soft->cloneFlags & CLONEFLAG_W_HOLDING )
		vsema( &hippi_devp->src_sema );

	    soft->cloneFlags = 0;
	    soft->wr_pktOutResid = 0;

	    if ( soft->mode & FREAD ) {

		ASSERT( ulpIndex <= HIPPIFP_MAX_OPEN_ULPS );
		ulpp = & hippi_devp->ulp[ ulpIndex ];

		ASSERT( ulpp->opens > 0 );
		if ( --ulpp->opens == 0 ) {
		    s = HPS_DSTLOCK_HC;
		    HPS_DSTWAIT;
		    if ( ulpp->ulpId == HIPPI_ULP_PH ) {
			*(__uint64_t *) &hippi_devp->dst_hc->arg.cmd_data[0] =
				(__uint64_t)HIP_STACK_RAW<<32;
			hippi_devp->PHmode = 0;
		    }
		    else {
			*(__uint64_t *) &hippi_devp->dst_hc->arg.cmd_data[0] =
				(__uint64_t)((ulpp->ulpId<<16)|(ulpIndex+HIP_STACK_FP)) << 32;
			hippi_devp->ulpFromId[ ulpp->ulpId ] = 255;
		    }
		    HPS_DSTOP (HCMD_DSGN_ULP);
		    HPS_DSTWAIT;
		    HPS_DSTUNLOCK_HC (s);

		    sv_destroy( &ulpp->rd_sv );
		    freesema( &ulpp->rd_dmadn );

		    phfree( ulpp->rd_pollhdp );

		    if ( ulpp->rd_fpd1head )
			kmem_free( ulpp->rd_fpd1head, HIPPIFP_HEADBUFSIZE );
		    kvpfree( (void *)ulpp->rd_c2b_rdlist, C2B_RDLISTPGS );

		} /* if ( --ulpp->opens == 0 ) */

	    } /* if ( soft->mode & FREAD ) */

	} /* if (hippi_devp->hi_state & HI_ST_UP) */
	else {
	    /********************************************************
	     * If board is shut down, we are just cleaning up here. *
	     * Free up poll-head, semaphores, etc., on last close.  *
	     *******************************************************/

	    if ( soft->cloneFlags & CLONEFLAG_W_HOLDING )
		vsema( &hippi_devp->src_sema );

	    if ( soft->mode & FREAD ) {

		ASSERT( ulpIndex <= HIPPIFP_MAX_OPEN_ULPS );
		ulpp = & hippi_devp->ulp[ ulpIndex ];

		ASSERT( ulpp->opens > 0 );
		if ( --ulpp->opens == 0 ) {

		    sv_destroy( &ulpp->rd_sv );
		    freesema( &ulpp->rd_dmadn );

		    phfree( ulpp->rd_pollhdp );

		    if ( ulpp->rd_fpd1head )
			kmem_free( ulpp->rd_fpd1head, HIPPIFP_HEADBUFSIZE );

		    kvpfree( (void *)ulpp->rd_c2b_rdlist, C2B_RDLISTPGS );

		}
	    }

	    hippi_devp->clone_info[soft->cloneid] = NULL;

	    /* Check for last close of a bound fd, if so, free
	     * the write semas - note it's not enough to just
	     * check the hippi_devp->ulp[i].opens since a user
	     * could have opened for write-only and wouldn't show
	     * up in the ulp table.
	     */
	    for (i=0; i < HIPPIFP_MAX_CLONES; i++)
		if ( hippi_devp->clone_info[i] &&
		     hippi_devp->clone_info[i]->ulpIndex != ULPIND_UNBOUND)
		    break;

	    if ( i == HIPPIFP_MAX_CLONES ) {

		/* Last close of a bound fd -- clean everything else up */
		freesema( & hippi_devp->rawoutq_sema );
		freesema( & hippi_devp->src_sema );

		for (i=0; i<HIPPIFP_MAX_WRITES; i++)
		    freesema( & hippi_devp->rawoutq_sleep[i] );
	    }
	} /* ! (hippi_devp->hi_state & HI_ST_UP) */
    } /* ulpIndex != ULPIND_UNBOUND */

    hippi_devp->clone_info[soft->cloneid] = NULL;
    vsema( & hippi_devp->devsema );

    kmem_free (soft, sizeof (hps_soft_t));
    hwgraph_vertex_destroy(vhdl);

    return 0;
}

/* =====================================================================
 *		CONTROL ENTRY POINT
 */

/*
 */
/* ARGSUSED */
int
hps_ioctl(dev_t dev, int cmd, void *arg,
	    int mode, cred_t *crp, int *rvalp)
{
    int		    error = 0;
    vertex_hdl_t    vhdl = dev_to_vhdl(dev);
    hps_soft_t	    *soft = hps_soft_get(vhdl);
    hippi_vars_t    *hippi_devp = soft->hippi_devp;


    ASSERT (soft->isclone);
    dprintf(5,("hps_ioctl(%d): dev=%x cmd=%x\n", hippi_devp->unit, dev, cmd));

    psema( &hippi_devp->devsema, PZERO );
	
    ASSERT( (mode & (FREAD|FWRITE)) == soft->mode );

    /* ioctls starting at 64 are administrative and don't need
     * the card up and running.
     */
    if ( (hippi_devp->hi_state & HI_ST_UP) == 0 && (cmd&255)<64 && (cmd != SIOC_MKHWG)) {
	vsema( &hippi_devp->devsema );
	return ENODEV;
    }

    switch (cmd) {

    case SIOC_MKHWG:
	/* This call is from ioconfig, to tell us that our
	 * unit number has been assigned.
	 */
	error = hps_ioc_mkhwg(hippi_devp);
	break;

    case HIPPI_SETONOFF:
	error = hps_ioc_setonoff(soft, hippi_devp, arg);
	break;

    case HIPPI_PGM_FLASH:
    case HIPPI_ERASE_FLASH:
    case HIPPI_GET_FLASH:
	error = hps_ioc_flash (hippi_devp, cmd, arg);
	break;

    case HIPIOC_GET_STATS:
	error = hps_ioc_getstats (hippi_devp, arg);
	break;

    case HIPIOC_BIND_ULP:
	error = hps_ioc_bindulp (soft, hippi_devp, arg, mode);
	break;

    case HIPPI_GET_MACADDR:
	if ( copyout (hippi_devp->mac_addr, arg, 6) < 0 )
	    error = EFAULT;
	break;

    case HIPPI_SET_MACADDR:
	error = hps_ioc_setmac(hippi_devp, arg);
	break;

    case HIPPI_SET_LOOPBACK:
	if (!_CAP_ABLE(CAP_DEVICE_MGT))
	    error = EPERM;
	else if (hippi_devp->hi_state & HI_ST_UP)
	    error = EINVAL;
	else
	    hippi_devp->hi_hwflags = HIP_FLAG_LOOPBACK;
	break;

    case HIPIOCW_I:
	if ( soft->ulpIndex == ULPIND_UNBOUND ) {
	    error = EINVAL;
	    break;
	}
	soft->wr_Ifield = (hippi_i_t) (long)arg;
	break;

    case HIPIOCW_D1_SIZE:
	if ((soft->ulpIndex == ULPIND_UNBOUND) ||
	    (soft->ulpId == HIPPI_ULP_PH)) {
	    error = EINVAL;
	    break;
	}
	if ((long)arg<0 || (long)arg>HIPPI_MAX_D1AREASIZE || ((long)arg & 7)) {
	    error = EINVAL;
	    break;
	}

	if ( (long)arg > 0 )
	    soft->wr_fpHdr.hfp_flags |= HFP_FLAGS_P;
	else
	    soft->wr_fpHdr.hfp_flags &= ~HFP_FLAGS_P;
	soft->wr_fpHdr.hfp_d1d2off = (u_short)(long)arg;
	break;

    case HIPIOCW_START_PKT:
	if ((soft->ulpIndex == ULPIND_UNBOUND) ||
	    (soft->cloneFlags & CLONEFLAG_W_NBOP)) { /* unfinished pkt */
	    error = EINVAL;
	    break;
	}
	soft->wr_pktOutResid = (__uint32_t) (long)arg;
	break;

    case HIPIOCW_CONNECT:
	if ((soft->ulpIndex == ULPIND_UNBOUND) ||
	    (soft->cloneFlags & CLONEFLAG_W_NBOC) ||
	    (soft->cloneFlags & CLONEFLAG_W_PERMCONN)) { /* already! */
	    error = EINVAL;
	    break;
	}
	soft->cloneFlags |= CLONEFLAG_W_PERMCONN;
	soft->wr_Ifield = (__uint32_t)(long)arg;
	break;

    case HIPIOCW_SHBURST:
	if ((soft->ulpIndex == ULPIND_UNBOUND) ||
	    ((long)arg < 0 || (long)arg > 1024 || ((long)arg&3))) {
	    error = EINVAL;
	    break;
	}
	if ( soft->ulpId != HIPPI_ULP_PH ) {
	    /* If HIPPI-FP, enforce that FP/D1 are exactly
	     * first burst.
	     */
	    if ((long)arg > 0 && (long)arg != 8+soft->wr_fpHdr.hfp_d1d2off) {
		error = EINVAL;
		break;
	    }
	    /* Set B-bit? */
	    if ( arg )
		soft->wr_fpHdr.hfp_flags |= HFP_FLAGS_B;
	    else
		soft->wr_fpHdr.hfp_flags &= ~HFP_FLAGS_B;
	}

	/* Allow using 1024 to set B-bit without really shortening
	 * first burst. A value of zero indicates standard size
	 * (1K) first burst.
	 */
	if ( (long) arg == 1024 )
	    soft->wr_fburst = 0;
	else
	    soft->wr_fburst = (u_short)(long) arg;
	break;

    case HIPIOCW_END_PKT:
	if ( soft->ulpIndex == ULPIND_UNBOUND ) {
	    error = EINVAL;
	    break;
	}

	/* null operation if packet is already done */
	if ( soft->wr_pktOutResid == 0 )
	    break;

	/* XXX This is a temporary workaround to disable END_PKT while
	   in permenent connection modw for the 3.1 release. */
	if (soft->cloneFlags & CLONEFLAG_W_PERMCONN) {
	    error = EINVAL;
	    break;
	}

	soft->wr_pktOutResid = 0;
	soft->cloneFlags &= ~CLONEFLAG_W_NBOP;

	/* This should NOT be sent unless we've actually got the src_sema,
	 * i.e. we've actually started sending a packet. */
	if (soft->cloneFlags & CLONEFLAG_W_HOLDING)
	    hps_send_dummy_desc (hippi_devp,
				 ( soft->cloneFlags & CLONEFLAG_W_PERMCONN ) ?
				 HIP_D2B_RDY | HIP_D2B_NEOC | HIP_D2B_NACK :
				 HIP_D2B_RDY | HIP_D2B_NACK);

	/* Release lock if we aren't holding a connection */
	if ( ! (soft->cloneFlags & CLONEFLAG_W_PERMCONN) ) {
	    if ( (soft->cloneFlags & CLONEFLAG_W_HOLDING) )
		vsema( &hippi_devp->src_sema );
	    soft->cloneFlags &= ~(CLONEFLAG_W_HOLDING|CLONEFLAG_W_NBOC);
	}
	break;

    case HIPIOCW_DISCONN:
	if ((soft->ulpIndex == ULPIND_UNBOUND) ||
	    !(soft->cloneFlags & CLONEFLAG_W_PERMCONN)) {
	    error = EINVAL;
	    break;
	}
	soft->wr_pktOutResid = 0;

	/* Tell controller to drop Packet and Connection only if we own
	 * the src_sema. */
	if ( (soft->cloneFlags & CLONEFLAG_W_HOLDING) ) {
	    hps_send_dummy_desc (hippi_devp, HIP_D2B_RDY | HIP_D2B_NACK);
	    vsema( & hippi_devp->src_sema );
	    soft->cloneFlags &= ~CLONEFLAG_W_HOLDING;
	}
	soft->cloneFlags &=
		  ~(CLONEFLAG_W_PERMCONN|CLONEFLAG_W_PC_ON|CLONEFLAG_W_NBOP|CLONEFLAG_W_NBOC);
	break;

    case HIPIOCW_ERR:
	if ( soft->ulpIndex == ULPIND_UNBOUND )
	    error = EINVAL;
	else
	    *rvalp = soft->src_error;
	break;

    case HIPIOCR_ERRS:
	if ( soft->ulpIndex == ULPIND_UNBOUND )
	    error = EINVAL;
	else
	    *rvalp = soft->dst_errors;
	break;

    case HIPIOCR_PKT_OFFSET: {
	struct hippi_fp_ulps    *ulpp;

	if ( soft->ulpIndex <= HIPPIFP_MAX_OPEN_ULPS )
	    ulpp = & hippi_devp->ulp[ soft->ulpIndex ];
	else
	    ulpp = 0;

	if ( ! ulpp )
	    error = EINVAL;
	else
	    *rvalp = ulpp->rd_offset;
	break;

    }
	
    case HIPIOC_ACCEPT_FLAG:
	/* Can only switch accept state off/on if card is up. */
	if ((hippi_devp->hi_state & HI_ST_UP) == 0) {
	    error = EINVAL;
	    break;
	}
	if ( arg )
	    error = hps_setparams ( hippi_devp, 0, /* to dst */
				    hippi_devp->hi_hwflags | HIP_FLAG_ACCEPT);
	else
	    error = hps_setparams ( hippi_devp, 0, /* to dst */
				   hippi_devp->hi_hwflags & ~HIP_FLAG_ACCEPT );
	break;

    case HIPIOC_STIMEO:
        if ((long)arg <= 0)
	    error = EINVAL;
	else {
	    hippi_devp->hi_stimeo = (int)(long)arg;
	    error = hps_setparams ( hippi_devp, 1 /* to src */,
				    hippi_devp->hi_hwflags );
	}
	break;

#ifdef HIPIOC_DTIMEO
    case HIPIOC_DTIMEO:
	hippi_devp->hi_dtimeo = (long)arg;
	error = hps_setparams ( hippi_devp, 0 /* to dst */,
			        hippi_devp->hi_hwflags );
	break;
#endif

    case HIPPI_GET_SRCVERS:
	*rvalp = (int) hippi_devp->hi_srcvers.whole;
	break;

    case HIPPI_GET_DSTVERS:
	*rvalp = (int) hippi_devp->hi_dstvers.whole;
	break;

    case HIPPI_GET_DRVRVERS:
	/* Version of the firmware compiled into this driver */
	*rvalp = (int) hippi_dstvers.whole;
	break;

    default:
	error = EINVAL;
	break;

    }	/* switch (cmd) */

    vsema( &hippi_devp->devsema );
    return error;
}


/* =====================================================================
 *		DATA TRANSFER ENTRY POINTS
 *
 */
/* ARGSUSED */
int
hps_read(dev_t dev, uio_t *uiop, cred_t *crp)
{
    volatile hip_c2b_t *c2b_rdp;
    struct hippi_fp_ulps *ulpp;
    caddr_t v_addr;
    int	cookie;
    int	error=0, ulpIndex;
    int	len, blen, rdlist_len;
    pfn_t pfn[ 1+HIPPIFP_MAX_READSIZE/NBPP ], *pfnp;
    vertex_hdl_t    vhdl = dev_to_vhdl(dev);
    hps_soft_t	    *soft = hps_soft_get(vhdl);
    hippi_vars_t    *hippi_devp = soft->hippi_devp;

    /* Read must be single, 64-bit aligned, contiguous. */
    if ( (uiop->uio_iovcnt != 1) ||
	 ((long) (uiop->uio_iov->iov_base) & 7) ||
	 ((uiop->uio_iov->iov_len) & 7) ||
	 (uiop->uio_iov->iov_len > HIPPIFP_MAX_READSIZE) ||
	 (uiop->uio_iov->iov_len < 8) )
	return EINVAL;

    v_addr = uiop->uio_iov->iov_base;
    len = uiop->uio_iov->iov_len;

    /* Card must (still) be up */
    if ( ! (hippi_devp->hi_state & HI_ST_UP) )
	return ENODEV;
	
    /* Make sure clone device is bound. */
    ulpIndex = soft->ulpIndex;
    if ( ulpIndex > HIPPIFP_MAX_OPEN_ULPS )
	return ENXIO;	/* unbound */
	
    ulpp = & hippi_devp->ulp[ ulpIndex ];

#ifdef HPS_DEBUG
    if ( ulpp->ulpId == HIPPI_ULP_PH ) {
	ASSERT( hippi_devp->PHmode );
    }
    else {
	ASSERT( ! hippi_devp->PHmode );
    }
#endif

    if(uiop->uio_segflg == UIO_SYSSPACE) {	
	dki_dcache_inval(v_addr,len);
    }
    else if(uiop->uio_pbuf != NULL) /* strategy has allready pinned them */
        {
        }
    else { /* if(uiop->uio_segflg == UIO_USERSPACE) */
	if ( 0 != (error=fast_userdma( v_addr, len, B_READ, &cookie ) ) )
	    return error;
    }

    /*
     * Wait until there is data available to read.
     */
    mutex_lock (&hippi_devp->sv_mutex, PZERO);
    if (! (hippi_devp->hi_state & HI_ST_UP) )
        mutex_unlock ( & hippi_devp->sv_mutex );
    else {
	if (ulpp->rd_semacnt > 0) {
	    ulpp->rd_semacnt--;
	    mutex_unlock ( & hippi_devp->sv_mutex );
	}
	else if (sv_wait_sig(&ulpp->rd_sv,0,&hippi_devp->sv_mutex,0) != 0) {
	    if(uiop->uio_segflg == UIO_USERSPACE && (uiop->uio_pbuf == NULL)) {
		fast_undma( v_addr, len, B_READ, &cookie );
	    }
	    return EINTR;
	}
    }
    if ( ! (hippi_devp->hi_state & HI_ST_UP) ) {
	if(uiop->uio_segflg == UIO_USERSPACE && (uiop->uio_pbuf == NULL)) {
	    fast_undma( v_addr, len, B_READ, &cookie );
	}
	return ENODEV;
    }
    if ( ulpp->rd_D2_avail < 0 ) {
	soft->dst_errors = (ulpp->rd_D2_avail & 0x7FFFFFFF);

	if(uiop->uio_segflg == UIO_USERSPACE && (uiop->uio_pbuf == NULL)) {
	    fast_undma( v_addr, len, B_READ, &cookie );
	}
	return EIO;
    }
   
    /*
     * Just return header if appropriate.
     */
    if ( ulpp->rd_fpHdr_avail > 0 ) {
	/* We bcopy this because the header is already in memory. */
	if(uiop->uio_segflg == UIO_USERSPACE && (uiop->uio_pbuf == NULL)) {
	    fast_undma( v_addr, len, B_READ, &cookie );
	}
	soft->dst_errors = 0;

	/* HIPPI-PH doesn't send headers up */
	ASSERT( ulpp->ulpId != HIPPI_ULP_PH );

	/* Double-check ULP-ID */
	ASSERT( * ( (u_char *) ulpp->rd_fpd1head ) == ulpp->ulpId );

	/* You're in trouble if you use a read buffer too small for
	 * an FP/D1 header.  Header gets discarded.
	 */
	if (uiop->uio_iov->iov_len < ulpp->rd_fpHdr_avail) {
	    error = EINVAL;
	    goto wake_ret;
	}

	if ( uiomove( (caddr_t) ulpp->rd_fpd1head,
		       ulpp->rd_fpHdr_avail, UIO_READ, uiop ) < 0 ) {
	    error = EFAULT;
	    goto wake_ret;
	}

#if HPS_DEBUG
	/* This makes sure we're getting a new header each time */
	bzero( ulpp->rd_fpd1head, HIPPIFP_HEADBUFSIZE );
#endif
	if ( ulpp->rd_D2_avail > 0 ) {
	    ulpp->rd_offset = ulpp->rd_fpHdr_avail;
	    ulpp->rd_fpHdr_avail = 0;
	    goto wake_ret;
	}
	/* No associated D2 data... */
	ulpp->rd_offset = 0;
	ulpp->rd_fpHdr_avail = 0;

	/* return the header buffer for the next packet. */
	hps_send_c2b (hippi_devp,
		      HIP_C2B_SML | (ulpIndex+HIP_STACK_FP),
		      HIPPIFP_HEADBUFSIZE,
		      hippi_devp->dma_addr |
		      (iopaddr_t)kvtophys((void *)ulpp->rd_fpd1head));
	return 0;

    } /* if ( ulpp->rd_fpHdr_avail > 0 )  */


    ASSERT( ulpp->rd_D2_avail > 0 );
    ASSERT( len > 0 );
		
    ulpp->rd_D2_avail = 0;
    if(uiop->uio_segflg == UIO_SYSSPACE) {
	int m_len = len;
	caddr_t m_v_addr = v_addr;
	pfnp = pfn;
	while(m_len > 0) {
	    blen = min( NBPP - ((u_long)m_v_addr & POFFMASK ), len );
	    *pfnp = (int)(kvtophys(m_v_addr)>> PNUMSHFT); /* make page number */
	    m_v_addr += blen;
	    pfnp ++;
	    m_len -= blen;
	}
    }
    else {    /* Do mapping all in one call...it's faster */
      if(uiop->uio_pbuf != NULL) {
	  if ( !vtopv( v_addr, len, pfn, sizeof(int),0,0 ) )
	      panic( "hps_read: vtopv failed!" );
      }
      else {
	if ( ! curproc_vtop( v_addr, len, (int*)pfn, sizeof(int) ) )
	  panic( "hps_read: vtop failed!" );
      }
    }
    c2b_rdp = ulpp->rd_c2b_rdlist;
    rdlist_len = 0;
    pfnp = pfn;

    while ( len > 0 ) {
	blen = min( NBPP - ((u_long)v_addr & POFFMASK ), len );
	c2b_rdp->c2b_addr = hippi_devp->dma_addr |
	      			((__uint64_t)*pfnp<<PNUMSHFT) |
				((__uint64_t)v_addr&POFFMASK);
	c2b_rdp->c2b_param = blen;
	c2b_rdp->c2b_op = HIP_C2B_BIG;
	v_addr += blen;
	len -= blen;
	c2b_rdp++;
	rdlist_len++;
	pfnp++;
    }

    c2b_rdp->c2b_op = HIP_C2B_EMPTY;
    ASSERT( len == 0 );
    ASSERT( rdlist_len<C2B_RDLISTPGS*NBPP/sizeof(hip_c2b_t) );

    hps_send_c2b( hippi_devp, HIP_C2B_READ |
		     		  ( ulpp->ulpId == HIPPI_ULP_PH ?
				   HIP_STACK_RAW : (ulpIndex+HIP_STACK_FP) ),
		 rdlist_len*sizeof(hip_c2b_t),
		 hippi_devp->dma_addr |
		     (iopaddr_t)kvtophys( (void *)ulpp->rd_c2b_rdlist ) );

    /* wait for input DMA done.  */
    psema( & ulpp->rd_dmadn, PZERO );

    if ( ! (hippi_devp->hi_state & HI_ST_UP) ) {
	if(uiop->uio_segflg == UIO_USERSPACE && (uiop->uio_pbuf == NULL)) {
	    fast_undma(uiop->uio_iov->iov_base,uiop->uio_iov->iov_len, B_READ, &cookie );
	}
	return EIO;
    }
    
    if ( ulpp->rd_count < 0 ) {  		/* error occurred */
	ulpp->rd_offset = 0;
	soft->dst_errors = (ulpp->rd_count & 0x7FFFFFFF);

	/* Put the buffer back for next header.
	 */
	hps_send_c2b( hippi_devp,
		     HIP_C2B_SML | (ulpIndex+HIP_STACK_FP),
		     HIPPIFP_HEADBUFSIZE,
		     hippi_devp->dma_addr |
		     (iopaddr_t)kvtophys((void *)ulpp->rd_fpd1head));
	if(uiop->uio_segflg == UIO_USERSPACE && (uiop->uio_pbuf == NULL)) {
	    fast_undma(uiop->uio_iov->iov_base,uiop->uio_iov->iov_len, B_READ, &cookie );
	}
	return EIO;
    }
    else {  				/* no error */
	soft->dst_errors = 0;
	uiop->uio_resid = uiop->uio_iov->iov_len - ulpp->rd_count;

	if ( ulpp->ulpFlags & ULPFLAG_R_MORE ) {
	    /* More D2 area to read */

	    ulpp->ulpFlags &= ~ULPFLAG_R_MORE;
	    if ( ulpp->rd_offset + ulpp->rd_count < ulpp->rd_offset )
		ulpp->rd_offset = 0x7fffffff;
	    else
	        ulpp->rd_offset += ulpp->rd_count;
				
	    ulpp->rd_D2_avail = 1;

	    mutex_lock (&hippi_devp->sv_mutex, PZERO );
	    if ( (hippi_devp->hi_state & HI_ST_UP) != 0 ) {
		if (ulpp->ulpFlags&ULPFLAG_R_POLL) {
		    pollwakeup( ulpp->rd_pollhdp, POLLIN|POLLRDNORM );
		    ulpp->ulpFlags &= ~ULPFLAG_R_POLL;
		}
		if (sv_signal (&ulpp->rd_sv) == 0) {
		    /* no one waiting yet */
		    ulpp->rd_semacnt++;
		}
	    }		
	    mutex_unlock (&hippi_devp->sv_mutex);
	}
	else {
	    ulpp->rd_offset = 0;

	    /* Put the buffer back for next header. */
	    hps_send_c2b( hippi_devp,
			     HIP_C2B_SML | (ulpIndex+HIP_STACK_FP),
			     HIPPIFP_HEADBUFSIZE,
			     hippi_devp->dma_addr |
			     (iopaddr_t) kvtophys((void *)ulpp->rd_fpd1head));
	}
    }
    if(uiop->uio_segflg == UIO_USERSPACE && (uiop->uio_pbuf == NULL)) {
	fast_undma(uiop->uio_iov->iov_base,uiop->uio_iov->iov_len,B_READ, &cookie );
    }

    return 0;

wake_ret:
    /* wake up someone to read the rest of this stuff. */
    mutex_lock (&hippi_devp->sv_mutex, PZERO);
    if (ulpp->ulpFlags&ULPFLAG_R_POLL) {
	pollwakeup( ulpp->rd_pollhdp, POLLIN|POLLRDNORM );
	ulpp->ulpFlags &= ~ULPFLAG_R_POLL;
    }
    if (sv_signal (&ulpp->rd_sv) == 0) /* no one waiting yet */
        ulpp->rd_semacnt++;
    mutex_unlock (&hippi_devp->sv_mutex);
    return error;
}

/* ARGSUSED */
int
hps_write(dev_t dev, uio_t *uiop, cred_t *crp)
{
    volatile union hip_d2b *d2bp;
    pfn_t *ppfn;
    caddr_t	v_addr;
    u_int 	*fphdr, *fphdr_buf;
    int	i, error, len, blen, fphdr_len, d2b_flags=0;
    int	clone_flags, dma_chunks, fburst;
    int	cookie;
    pfn_t pfn[ HIPPIFP_MAX_WRITESIZE/NBPP+1 ];

    vertex_hdl_t    vhdl = dev_to_vhdl(dev);
    hps_soft_t	    *soft = hps_soft_get(vhdl);
    hippi_vars_t    *hippi_devp = soft->hippi_devp;

    /* Write must be single, aligned, contiguous.
     */
    if ( (uiop->uio_iovcnt != 1) ||
	 ((long) (uiop->uio_iov->iov_base) & 7) ||
	 ((uiop->uio_iov->iov_len) & 7) ||
	 (uiop->uio_iov->iov_len > HIPPIFP_MAX_WRITESIZE) ||
	 (uiop->uio_iov->iov_len < 8) )
	return EINVAL;

    /* Card must be (still) up */
    if ( ! (hippi_devp->hi_state & HI_ST_UP) )
	return ENODEV;

    /* Check to make sure clone device is bound.
     */
    if ( soft->ulpIndex >= ULPIND_UNBOUND )
	return ENXIO;

    /* Grab one of four write semaphores */
    if ( psema( & hippi_devp->rawoutq_sema, PWAIT | PCATCH ) )
	return EINTR;

    /* Card must be (still) up */
    if ( ! (hippi_devp->hi_state & HI_ST_UP) ) {
	vsema( & hippi_devp->rawoutq_sema );
	return ENODEV;
    }

    /* Grab HIPPI source semaphore if we don't already own it. */
    if ( ! (soft->cloneFlags & CLONEFLAG_W_HOLDING) ) {
	if ( psema( & hippi_devp->src_sema, PWAIT | PCATCH ) ) {
	    vsema( & hippi_devp->rawoutq_sema );
	    return EINTR;
	}
	/* Card must be (still) up */
	if ( ! (hippi_devp->hi_state & HI_ST_UP) ) {
	    vsema( & hippi_devp->rawoutq_sema );
	    vsema( & hippi_devp->src_sema );
	    return ENODEV;
	}
    }

    v_addr = uiop->uio_iov->iov_base;
    len = uiop->uio_iov->iov_len;

    if(uiop->uio_segflg == UIO_SYSSPACE) {
	dki_dcache_wb(v_addr,len);
    }
    else if(uiop->uio_pbuf != NULL) /* strategy has allready pinned them */
        {
        }
    else { /* if(uiop->uio_segflg == UIO_USERSPACE) */
	if ( 0 != (error=fast_userdma( v_addr, len, B_WRITE, &cookie )) ) {
	    vsema( & hippi_devp->rawoutq_sema );
	    if ( ! (soft->cloneFlags & CLONEFLAG_W_HOLDING) )
		vsema(  & hippi_devp->src_sema );
	    return error;
	}
    }

    uiop->uio_resid = 0; /* assume we'll transfer it all */
    clone_flags = soft->cloneFlags;

    /* Figure out FP header that goes onto this
     * packet.
     */
    fphdr = 0;
    fphdr_len = 0;
    fphdr_buf = 0;
    /* Header needed for begin of each pkt for FP, and beg of conn for PH */
    if (!(clone_flags & CLONEFLAG_W_NBOP) &&
	(soft->ulpId != HIPPI_ULP_PH || !(clone_flags & CLONEFLAG_W_NBOC))){

	int	d1size = 0;
	__uint32_t	*p;

	if ( ! ( clone_flags & CLONEFLAG_W_NBOC ) )
	    fphdr_len += 4; /* sizeof( I-field ) */

	if ( soft->ulpId != HIPPI_ULP_PH ) {
	    d1size = (soft->wr_fpHdr.hfp_d1d2off & HFP_D1SZ_MASK);
	    fphdr_len += sizeof(hippi_fp_t);
	}

	ASSERT( fphdr_len > 0 );

	/* Get a fphdr_buf and make it end on a long-word boundary. */
	fphdr_buf = kmem_zalloc( fphdr_len+4, KM_SLEEP );
	fphdr = fphdr_buf;
	if ( (( (long)fphdr + fphdr_len) & 4) != 0 )
	    fphdr++;

	p=fphdr;
	if ( ! ( clone_flags & CLONEFLAG_W_NBOC ) )
	    *p++ = soft->wr_Ifield;
	   
	if ( soft->ulpId != HIPPI_ULP_PH ) {
	    bcopy( &soft->wr_fpHdr, p, sizeof(hippi_fp_t));
	    if ( soft->wr_pktOutResid == 0 ) { /* single-pkt write */
		if ( len >= d1size )
		    ((hippi_fp_t *)p )->hfp_d2size = len - d1size;
		else
		    ((hippi_fp_t *)p )->hfp_d2size = 0;
	    }
	    else {
		if ( soft->wr_pktOutResid >= d1size )
		    ( (hippi_fp_t *) p )->hfp_d2size =
				soft->wr_pktOutResid - d1size;
		else
		    ( (hippi_fp_t *) p )->hfp_d2size = 0;
	    }
	}
    }

    if (soft->wr_pktOutResid &&
	(soft->wr_pktOutResid != HIPPI_D2SIZE_INFINITY)) {
	if ( len > soft->wr_pktOutResid ) {
	    uiop->uio_resid = len - soft->wr_pktOutResid;
	    len = soft->wr_pktOutResid;
	    soft->wr_pktOutResid = 0;
	}
	else {
	  soft->wr_pktOutResid -= len;
	}
    }

    /* Handle short-first-burst */
    fburst = soft->wr_fburst;
    blen = len + ((soft->ulpId==HIPPI_ULP_PH) ? 0 : sizeof(hippi_fp_t));
    if ( fburst && 
	 ( (clone_flags & CLONEFLAG_W_NBOP) || fburst > blen ||
	   (fburst == blen && soft->wr_pktOutResid == 0) ) )
	fburst = 0;

    /* Manipulate all the flags.
     */
    d2b_flags = ( (clone_flags & (CLONEFLAG_W_NBOC|CLONEFLAG_W_NBOP)) ?
			0 : HIP_D2B_IFLD ) |
		( (soft->wr_pktOutResid > 0) ?
					(HIP_D2B_NEOC|HIP_D2B_NEOP) : 0 ) ;
    if (clone_flags & CLONEFLAG_W_PERMCONN) {
        if (clone_flags & CLONEFLAG_W_PC_ON)
	    d2b_flags |= HIP_D2B_NEOC;
	else {
	    d2b_flags |= (HIP_D2B_NEOC | HIP_D2B_BEGPC);
	    clone_flags |= CLONEFLAG_W_PC_ON;
	}
    }

    if ( soft->wr_pktOutResid > 0 )
	clone_flags |= (CLONEFLAG_W_NBOP|CLONEFLAG_W_NBOC);
    else {
	clone_flags &= ~(CLONEFLAG_W_NBOP|CLONEFLAG_W_NBOC);
	if ( (clone_flags & CLONEFLAG_W_PERMCONN) )
	  clone_flags |= CLONEFLAG_W_NBOC;
    }
    soft->cloneFlags = clone_flags;

    /* Start loading packet onto HW Q.
     */
    d2bp = D2B_NXT( hippi_devp->hi_d2b_prod );
    dma_chunks = 0;

    if ( fphdr ) {
	/* Put I field, FP header, D1 header onto HW Q */
	/* zero the 48 bits of pad after the len for better rrdbg */
	*(__uint64_t *) &d2bp->sg.len = (__uint64_t)fphdr_len << 48;
	d2bp->sg.addr = hippi_devp->dma_addr | (iopaddr_t)kvtophys(fphdr);
	d2bp = D2B_NXT( d2bp );
	dma_chunks++;
    }

    if((uiop->uio_segflg == UIO_SYSSPACE)) { 
	int m_len = len;
	caddr_t m_v_addr = v_addr;
	ppfn = pfn;
	while(m_len > 0) {
	    blen = min( NBPP - ((u_long)m_v_addr & POFFMASK ), len );
	    *ppfn = (pfn_t)(kvtophys(m_v_addr)>> PNUMSHFT); /* make page number */
	    m_v_addr += blen;
	    ppfn ++;
	    m_len -= blen;
	}
    }
    else {
	/* Do mapping all in one call...it's faster */
        if(uiop->uio_pbuf != NULL) {
	    if ( ! vtopv( v_addr, len, pfn, sizeof(int),0,0 ) )
	      panic( "hps_read: vtopv failed!" );
	}
	else {
	    if ( ! curproc_vtop( v_addr, len, (int*)pfn, sizeof(int) ) )
	      panic( "hps_write: vtop2 failed!" );
	}
    }

    ppfn=pfn;
    while ( len > 0 ) {
	blen = min( NBPP - ((u_long)v_addr & POFFMASK ), len );
	/* zero the 48 bits of pad after the len for better rrdbg */
	*(__uint64_t *) &d2bp->sg.len = (__uint64_t) blen<< 48;
	d2bp->sg.addr = hippi_devp->dma_addr |
			((u_long) *(ppfn++)<<PNUMSHFT)|
			((u_long) v_addr&POFFMASK);
	if (blen > HPS_PFTCH_THRESHOLD)
	    d2bp->sg.addr |= PCI64_ATTR_PREF;
	v_addr += blen;
	d2bp = D2B_NXT(d2bp);
	dma_chunks++;
	len -= blen;
    }

    d2bp->hd.flags = HIP_D2B_BAD;

    hippi_devp->hi_d2b_prod->hd.chunks = dma_chunks;
    hippi_devp->hi_d2b_prod->hd.stk = HIP_STACK_FP;
    hippi_devp->hi_d2b_prod->hd.fburst = fburst;
    hippi_devp->hi_d2b_prod->hd.sumoff = 0xffff;
    hippi_devp->hi_d2b_prod->hd.flags = HIP_D2B_RDY | d2b_flags;
    hippi_devp->hi_d2b_prod = d2bp;

    if (hippi_devp->hi_ssleep->flags & HIPFW_FLAG_SLEEP)
	hps_wake_src ( hippi_devp );

#ifdef USE_MAILBOX
	/* tell firmware we've got some descriptors for it */
	hps_src_d2b_rdy(hippi_devp);
#endif

    /* Pick semaphore we are to sleep on BEFORE releasing src sema */
    i = hippi_devp->rawoutq_in;
    if ( ++hippi_devp->rawoutq_in >= HIPPIFP_MAX_WRITES )
	hippi_devp->rawoutq_in=0;

/********
    if ( soft->ulpId != HIPPI_ULP_PH &&
	(clone_flags & (CLONEFLAG_W_NBOC|CLONEFLAG_W_NBOP)) )
********/
    if (clone_flags & (CLONEFLAG_W_NBOC|CLONEFLAG_W_NBOP))
	soft->cloneFlags |= CLONEFLAG_W_HOLDING;
    else {
	soft->cloneFlags &= ~CLONEFLAG_W_HOLDING;
	vsema( &hippi_devp->src_sema );
    }

    /* wait for ODONE.  Don't think I need to check hi_state for UP
     * since hps_close does a vsema on rawoutq_sleep.
     */
    psema (&hippi_devp->rawoutq_sleep[i], PZERO );

    if ( ! (hippi_devp->hi_state & HI_ST_UP) )
	soft->src_error = B2H_OSTAT_SHUT;
    else
        soft->src_error = hippi_devp->rawoutq_error[ i ];

    /* If an error occurs and we're doing more than a single
     * write packet/connection, we need to clean up here...
     */

    if ( soft->src_error &&
	( soft->cloneFlags & (CLONEFLAG_W_NBOC|CLONEFLAG_W_NBOP|
			      CLONEFLAG_W_HOLDING) )  ) {

	if (hippi_devp->hi_state & HI_ST_UP)
	    hps_send_dummy_desc (hippi_devp, HIP_D2B_RDY | HIP_D2B_NACK);

	if ( soft->cloneFlags & CLONEFLAG_W_HOLDING )
	    vsema( &hippi_devp->src_sema );

	soft->cloneFlags &= ~(CLONEFLAG_W_HOLDING |
			      CLONEFLAG_W_NBOC | CLONEFLAG_W_NBOP |
			      CLONEFLAG_W_PERMCONN | CLONEFLAG_W_PC_ON );
	soft->wr_pktOutResid = 0;
    }
    vsema( & hippi_devp->rawoutq_sema );

    if ( fphdr_buf )
	kmem_free( fphdr_buf, fphdr_len+4 );
	
    if(uiop->uio_segflg == UIO_USERSPACE && (uiop->uio_pbuf == NULL)) {
	fast_undma( uiop->uio_iov->iov_base, uiop->uio_iov->iov_len, B_WRITE,
		   &cookie );
    }
	
    return soft->src_error ? EIO : 0;
}


/* =====================================================================
 *		POLL ENTRY POINT
 */

/* ARGSUSED */
int
hps_poll(dev_t dev, short events, int anyyet,
	   short *reventsp, struct pollhead **phpp)
{
    int	ulpIndex;
    vertex_hdl_t    vhdl = dev_to_vhdl(dev);
    hps_soft_t	    *soft = hps_soft_get(vhdl);
    hippi_vars_t    *hippi_devp = soft->hippi_devp;
    struct hippi_fp_ulps *ulpp;

    dprintf(5, ("hps_poll (dev=0x%x, events=0x%x) called.\n", dev, events ));

    ulpIndex = soft->ulpIndex;
    if ( ulpIndex > HIPPIFP_MAX_OPEN_ULPS )
	return ENXIO;	/* unbound! */

    ulpp = & hippi_devp->ulp[ ulpIndex ];

    ASSERT( ulpp->opens > 0 );

    if ( events & (POLLIN | POLLRDNORM) ) {
	mutex_lock (&hippi_devp->sv_mutex, PZERO);
	if ( ulpp->rd_D2_avail == 0 && ulpp->rd_fpHdr_avail == 0 ) {
	    events &= ~(POLLIN | POLLRDNORM);
	    ulpp->ulpFlags |= ULPFLAG_R_POLL;
	    if (!anyyet)
		*phpp = ulpp->rd_pollhdp;
	}
	mutex_unlock (&hippi_devp->sv_mutex);
    }

    /* XXX: we could do the same for writes, see if
     * write semaphore is > 0 !!!
     */

    *reventsp = events;
    return 0;
}

/* =====================================================================
 *		Routines invoked by ioctls.
 */

/*
 * hps_mkhwg()
 * Invoked by SIOC_MKHWG ioctl from ioconfig.
 *
 * All our (and bypass) device
 * nodes are attached on the LINC0 connectpoint. But we create
 * an alias /hw/hippi/<unit> to point to that connectpt so it
 * looks like all the device nodes for that card are in
 * /hw/hippi/<unit>/{hippi, hippibp, hippibp<job>}
 *
 * We also call hippibp_attach() and ifhip_attach() at this
 * point and no earlier because both those routines want to
 * know the controller unit number.
 */
int
hps_ioc_mkhwg(hippi_vars_t * hippi_devp) 
{
    int		    unit;
    vertex_hdl_t    bpdev;
    vertex_hdl_t    newvhdl;
    char	    buf[32];

    if (hippi_devp->unit != -1) {
	/* already initialized */
	dprintf (1, ("hps_ioctl (SIOC_MKHWG): %v: unit number already assigned!\n", hippi_devp->dev_vhdl));
	return EIO;
    }
    if ((unit = device_controller_num_get(hippi_devp->dev_vhdl)) < 0) {
	cmn_err(CE_ALERT,
		"hps_ioctl(SIOC_MKHWG): %v: No controller number.\n",
		hippi_devp->dev_vhdl);
	return EIO;
    }
    hippi_devp->unit = unit;
    dprintf (5, ("hps_ioctl (SIOC_MKHWG): %v, unit number %d assigned!\n",
		 hippi_devp->dev_vhdl, unit));

    /* Create alias for this device in /hw/hippi. First, get 
     * /hw/hippi vertex, creating one if it doesn't yet exist.
     */
    if (hwgraph_traverse (GRAPH_VERTEX_NONE,
			  "hippi", &newvhdl) != GRAPH_SUCCESS) {
	dprintf (5, ("Creating /hw/hippi.\n"));
	if (hwgraph_path_add (GRAPH_VERTEX_NONE,
			      "hippi", &newvhdl) != GRAPH_SUCCESS) {
	    cmn_err(CE_ALERT,
		    "hps_ioctl(SIOC_MKHWG): %v: Couldn't create /hw/hippi.\n",
		    hippi_devp->dev_vhdl);
	    return ENOMEM;
	}
    }

    dprintf (5, ("Creating hwgraph alias /hw/hippi/%d.\n", unit));
    sprintf (buf, "%d", unit);
    hwgraph_edge_add (newvhdl, hippi_devp->dst_vhdl, buf);

    bpdev = GRAPH_VERTEX_NONE;
    if (hwgraph_path_add (hippi_devp->dst_vhdl,
			  "bypass", &bpdev) != GRAPH_SUCCESS) {
	cmn_err(CE_ALERT,
		"hps_ioctl(SIOC_MKHWG): %v: Couldn't create bypass vertex.\n",
		hippi_devp->dev_vhdl);
	return ENOMEM;
    }

    hippi_devp->bp_vhdl = GRAPH_VERTEX_NONE;

    /* This call could be a stub if there is no bypass installed. */
    hippibp_attach(bpdev, &hippi_devp->bp_vhdl, (void *)hippi_devp);

    ifhip_attach(hippi_devp);
    return 0;
}

/*
 * hps_ioc_setonoff()
 * invoked by HIPPI_SETONOFF ioctl.
 */
int
hps_ioc_setonoff(hps_soft_t * soft, hippi_vars_t * hippi_devp, void * arg)
{
    int	    i, up_err;

    if (!_CAP_ABLE(CAP_DEVICE_MGT)) {
	return EPERM;
    }

    if ( soft->ulpIndex != ULPIND_UNBOUND ) {
	return EBUSY;
    }

    if ( arg && (hippi_devp->hi_state & HI_ST_UP) == 0 ) {
	/* bringup requested & board not already up */

	/* Make sure this is only open file descriptor */
	i = 0;
	while ( (i < HIPPIFP_MAX_CLONES ) &&
	    	( hippi_devp->clone_info[i] == NULL ||
	          hippi_devp->clone_info[i] == soft))
	    i++;

	if ( i < HIPPIFP_MAX_CLONES ) {
	    return EBUSY;
	}

	up_err = hippi_bd_bringup( hippi_devp );
	  
	/* hippi_bd_bringup() clears all the clone_info[]s
	 * so put this one back
	 */
	hippi_devp->clone_info[soft->cloneid] = soft;
	if (up_err)
	    return up_err;
	if (hippi_devp->hi_state != HI_ST_UP)
	    return ENODEV;

    } else if ( ! arg && (hippi_devp->hi_state & HI_ST_UP) != 0 ) {

	/* shutdown requested & board is in up state. */
	hippi_bd_shutdown( hippi_devp );

    } else if ( ! arg ) {

	/* Shutdown request in down state: do another reset
	 * board. Hit both LINCs with cold reset, then release
	 * reset on both.
	 */
	volatile    __uint32_t * lcsr;

	lcsr = hippi_devp->src_lincregs + 
		(LINC_LCSR-LINC_MISC_REGS_ADDR)/sizeof(uint32_t);
	*lcsr = _LINC_RESET;
	*lcsr = _LINC_CLR_RESET;

	lcsr = hippi_devp->dst_lincregs + 
		(LINC_LCSR-LINC_MISC_REGS_ADDR)/sizeof(uint32_t);
	*lcsr = _LINC_RESET;
	*lcsr = _LINC_CLR_RESET;

	hippi_devp->hi_bringup_tries = 0;	/* prepare to count up tries */

    } else {
	/* bringup requested and board is already up */
	return EINVAL;
    }

    return 0;
}

/* 
 * hps_ioc_getstats()
 * Invoked by HIPIOC_GET_STATS ioctl.
 */
hps_ioc_getstats ( hippi_vars_t *hippi_devp,
		  void * arg)
{
    int	i, s, error = 0;
    /* hippi_stats_t is 33 words, so alloc an extra 8 bytes */
    __uint64_t *temp;
    int size;

    /* round length up to a 64 it boundary */
    size = (sizeof(hippi_stats_t) & ~(8-1)) + 8;

    temp = (__uint64_t*)kmem_alloc(size, KM_SLEEP);

    /* ------ Get Statistics from source-side 4640 ----------- */
    s = HPS_SRCLOCK_HC;
    if (HPS_SRCWAIT < 0) {
	HPS_SRCUNLOCK_HC(s);
	error = EBUSY;
	goto ret_err;
    }
    HPS_SRCOP( HCMD_STATUS );
    if (HPS_SRCWAIT < 0) {
	HPS_SRCUNLOCK_HC(s);
	error = EBUSY;
	goto ret_err;
    }
    for (i=0; i < size/sizeof(__uint64_t); i++)
	temp[i] = ((__uint64_t *)hippi_devp->src_stat_area)[i];
    HPS_SRCUNLOCK_HC(s);

    /* ------ Get Statistics from dest-side 4640 ----------- */
    s = HPS_DSTLOCK_HC;
    if (HPS_DSTWAIT <= 0) {
	HPS_DSTUNLOCK_HC(s);
	error = EBUSY;
	goto ret_err;
    }
    HPS_DSTOP( HCMD_STATUS );
    if (HPS_DSTWAIT <= 0) {
	HPS_DSTUNLOCK_HC(s);
	error = EBUSY;
	goto ret_err;
    }
    for (i=0; i < size/sizeof(__uint64_t); i++)
	temp[i] |= ((__uint64_t *)hippi_devp->dst_stat_area)[i];
    HPS_DSTUNLOCK_HC(s);

    if ( copyout( (caddr_t)temp, (caddr_t)arg, sizeof(hippi_stats_t)) < 0 )
      error = EFAULT;

ret_err:
    kmem_free( temp, size);
    return error;
}

/*
 * hps_ioc_bindulp()
 * Invoked by HIPIOC_BIND_ULP ioctl.
 */
hps_ioc_bindulp( hps_soft_t * soft,
		 hippi_vars_t * hippi_devp,
		 void * arg,
		 int mode)
{
    int s, ulpIndex, ulp_id;
    struct hippi_fp_ulps *ulpp;

    ulp_id = (int)(long) arg;
    dprintf (5, ("hps_ioc_bindulp: unit=%d, ulp_id=0x%x, mode=0x%x\n",
		 hippi_devp->unit, ulp_id, mode));

    if ( (ulpIndex = soft->ulpIndex) != ULPIND_UNBOUND ) {
	return EINVAL;
    }

    if ( ulp_id > HIPPI_ULP_MAX || ulp_id < HIPPI_ULP_PH ) {
	return EINVAL;
    }

    /* Don't allow HIPPI-LE bind */
    if ( ulp_id == HIPPI_ULP_LE )
	return EINVAL;

    /* Don't allow bind to HIPPI-PH if networking is UP */
    if ( ulp_id == HIPPI_ULP_PH && (hippi_devp->hi_hwflags&HIP_FLAG_IF_UP))
	return EBUSY;

    soft->ulpId = ulp_id;

    if ( ulp_id != HIPPI_ULP_PH ) {
	/* Init default FP header */
	bzero( & soft->wr_fpHdr, sizeof(hippi_fp_t) );
	soft->wr_fpHdr.hfp_ulp_id = ulp_id;
    }

    /* Init default I-field */
    soft->wr_Ifield = HIPPI_DEFAULT_I;

    soft->wr_pktOutResid = 0;
    soft->wr_fburst = 0;

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
	    /* First process to bind to this ULP, so
	     * allocate an active read ULP structure for this ULP
	     */
	    if ( ulp_id == HIPPI_ULP_PH ) {
		/* Make sure nobody is open for FP before granting PH */
		for (ulpIndex=0; ulpIndex<HIPPIFP_MAX_OPEN_ULPS; ulpIndex++)
		    if ( hippi_devp->ulp[ulpIndex].opens > 0 )
			return EBUSY;
		hippi_devp->PHmode = 1;
		ulpIndex = HIPPIFP_MAX_OPEN_ULPS;
	    }
	    else {
		/* User wants FP, make sure no one is open for PH */
		if ( hippi_devp->PHmode )
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
	    ulpp->ulpFlags = 0;
	    ulpp->ulpId = ulp_id;

	    ulpp->rd_fpHdr_avail = 0;
	    ulpp->rd_D2_avail = 0;
	    ulpp->rd_offset = 0;
	    ulpp->rd_count = 0;

	    sv_init( &ulpp->rd_sv, SV_FIFO, NULL );
	    initsema( &ulpp->rd_dmadn, 0 );
	    ulpp->rd_semacnt = 0;

	    ulpp->rd_c2b_rdlist = (hip_c2b_t *)
			kvpalloc( C2B_RDLISTPGS, VM_DIRECT|VM_STALE,0 );

	    /* XXX: does this need to be done on a per-clone
	     * basis?  I can't right now.
	     */
	    ulpp->rd_pollhdp = phalloc( KM_SLEEP );

	    /* First read on the ULP, tell hardware about
	     * ULP --> stk association.
	     * This message goes to the destination side only.
	     */
	    s = HPS_DSTLOCK_HC;
	    HPS_DSTWAIT;
	    if ( ulp_id == HIPPI_ULP_PH ) {
		*(uint64_t *) &hippi_devp->dst_hc->arg.cmd_data[0] =
						(uint64_t)HIP_STACK_RAW<<32;
		HPS_DSTOP(HCMD_ASGN_ULP);
		HPS_DSTUNLOCK_HC(s);
	    }
	    else {
		*(uint64_t *) &hippi_devp->dst_hc->arg.cmd_data[0] =
			(uint64_t) ((ulp_id<<16)|(ulpIndex+HIP_STACK_FP)) << 32;
		HPS_DSTOP(HCMD_ASGN_ULP);
		HPS_DSTUNLOCK_HC(s);

		ulpp->rd_fpd1head = (hippi_fp_t *)
				kmem_alloc( HIPPIFP_HEADBUFSIZE,
					KM_SLEEP | KM_CACHEALIGN );

		hps_send_c2b( hippi_devp, HIP_C2B_SML|(ulpIndex+HIP_STACK_FP),
			      HIPPIFP_HEADBUFSIZE, 
			     hippi_devp->dma_addr |
			     (iopaddr_t) kvtophys((void *)ulpp->rd_fpd1head));
	    }
	}
	else {
	    /* Not first bind, just increment open count. */
	    ulpp = & hippi_devp->ulp[ulpIndex];

	    /* Attach another clone to this open ULP */
	    ASSERT( ulpp->opens > 0 );
	    ulpp->opens++;
	}
	soft->ulpIndex = ulpIndex;

    }
    else /* not (mode & FREAD) */
	soft->ulpIndex = ULPIND_NOREADERS;

    return 0;
}

int
hps_setparams( hippi_vars_t *hippi_devp, int to_src, int newflags )
{
    int s;

    if (!_CAP_ABLE(CAP_DEVICE_MGT)) {
	return EPERM;
    }

    if (to_src) {
	s = HPS_SRCLOCK_HC;
	hippi_devp->hi_hwflags = newflags;
	if (HPS_SRCWAIT < 0) {
	    HPS_SRCUNLOCK_HC(s);
	    return EBUSY;
	}
	hippi_devp->hi_hwflags = newflags;

	* (__uint64_t *) &hippi_devp->src_hc->arg.params =
	  	((__uint64_t) newflags << 32) | hippi_devp->hi_stimeo;

	HPS_SRCOP(HCMD_PARAMS);
	HPS_SRCUNLOCK_HC(s);
    }
    else {
	s = HPS_DSTLOCK_HC;
	hippi_devp->hi_hwflags = newflags;
	if (HPS_DSTWAIT < 0) {
	    HPS_DSTUNLOCK_HC(s);
	    return EBUSY;
	}
	hippi_devp->hi_hwflags = newflags;

	* (__uint64_t *) &hippi_devp->dst_hc->arg.params =
	  	((__uint64_t) newflags << 32) | hippi_devp->hi_dtimeo;

	HPS_DSTOP(HCMD_PARAMS);
	HPS_DSTUNLOCK_HC(s);
    }
    return 0;
}

/*
 * Send a dummy descriptor to src 4640 asking to drop packet
 * and/or connection.
 */
/*
 * hi_d2b_prod is protected by the src_sema, so caller must own src_sema.
 */
void
hps_send_dummy_desc (hippi_vars_t *hippi_devp, u_char flags)
{
	volatile union hip_d2b *d2bp;

	d2bp = hippi_devp->hi_d2b_prod;

	d2bp->hd.chunks = 0;
	d2bp->hd.stk = HIP_STACK_FP;
	d2bp = D2B_NXT( d2bp );
	d2bp->hd.flags = HIP_D2B_BAD;
	hippi_devp->hi_d2b_prod->hd.fburst = 0;
	hippi_devp->hi_d2b_prod->hd.flags = flags;
	hippi_devp->hi_d2b_prod = d2bp;

	if (hippi_devp->hi_ssleep->flags & HIPFW_FLAG_SLEEP)
	     hps_wake_src(hippi_devp);

#ifdef USE_MAILBOX
	/* tell firmware we've got some descriptors for it */
	hps_src_d2b_rdy(hippi_devp);
#endif
}

/* Send a wakeup msg to the src 4640. */
void
hps_wake_src (hippi_vars_t * hippi_devp) 
{
    int	s;

    s = HPS_SRCLOCK_HC;
    if (hippi_devp->hi_ssleep->flags & HIPFW_FLAG_SLEEP) {
	if (HPS_SRCWAIT > 0) {
	    hippi_devp->hi_ssleep->flags &= ~HIPFW_FLAG_SLEEP;
	    HPS_SRCOP (HCMD_WAKEUP);
#ifdef HPS_DEBUG
	    hippi_devp->stat_s2h_pokes++;
#endif
	}
    }
    /* else someone else beat us to it. */
    HPS_SRCUNLOCK_HC(s);
}

/* Send a wakeup msg to the dst 4640. */
void
hps_wake_dst (hippi_vars_t * hippi_devp) 
{
    int	s;

    s = HPS_DSTLOCK_HC;
    if (hippi_devp->hi_dsleep->flags & HIPFW_FLAG_SLEEP) {
	if (HPS_DSTWAIT > 0) {
	    hippi_devp->hi_dsleep->flags &= ~HIPFW_FLAG_SLEEP;
	    HPS_DSTOP (HCMD_WAKEUP);
#ifdef HPS_DEBUG
	    hippi_devp->stat_d2h_pokes++;
#endif
	}
    } /* else someone else beat us to it. */
    HPS_DSTUNLOCK_HC(s);
}


/*
 * hps_fp_odone called out of hps_src_intr(), with no locks held.
 * Wake up the process blocked on this write.
 */
/* ARGSUSED */
void
hps_fp_odone( hippi_vars_t *hippi_devp, 
	      volatile struct hip_d2b_hd *hd,
	      int status )
{
    dprintf(5, ("hps_fp_odone called, hd=%x status=%d\n", (long)hd, status ));
    ASSERT( hd->chunks > 0 );

    hippi_devp->rawoutq_error[ hippi_devp->rawoutq_out ] = status;
    vsema( & hippi_devp->rawoutq_sleep[ hippi_devp->rawoutq_out ] );

    if ( ++hippi_devp->rawoutq_out >= HIPPIFP_MAX_WRITES )
	hippi_devp->rawoutq_out = 0;
}


void
hps_fp_input( hippi_vars_t *hippi_devp, volatile struct hip_b2h *b2hp )
{
    struct hippi_fp_ulps *ulpp;

    dprintf(5, ("hps_fp_input: b2hp=%x (%x %x)\n",
		(long)b2hp, *((int *)b2hp), *((int *)b2hp+1) ));

    if ( (b2hp->b2h_op & HIP_B2H_STMASK) == HIP_STACK_RAW )
	ulpp = & hippi_devp->ulp[ HIPPIFP_MAX_OPEN_ULPS ];
    else
	ulpp = & hippi_devp->ulp[(b2hp->b2h_op & HIP_B2H_STMASK)-
				 HIP_STACK_FP ];

    if ( ulpp->opens == 0 )		/* XXX */
	return;

    if ( (b2hp->b2h_op & HIP_B2H_OPMASK) == HIP_B2H_IN ) {
	ulpp->rd_fpHdr_avail = b2hp->b2h_s;
	ulpp->rd_D2_avail = b2hp->b2h_l;

#if HPS_CALLBACK /* call back processing */
    /*
        If the client has an input procedure, call it now
        to let 'em know that something has arrived.
    */
    if (ulpp->rd_input_proc != NULL)
        (*ulpp->rd_input_proc)(ulpp->rd_input_arg,NULL,NULL,NULL);
 
#endif /* call back processing */

	mutex_lock (&hippi_devp->sv_mutex, PZERO);
	if ( ulpp->ulpFlags & ULPFLAG_R_POLL ) {
	    pollwakeup( ulpp->rd_pollhdp,  POLLIN|POLLRDNORM );
	    ulpp->ulpFlags &= ~ULPFLAG_R_POLL;
	}
	if (sv_signal (&ulpp->rd_sv) == 0) {
	    /* no one waiting yet */
	    ulpp->rd_semacnt++;
	}
	mutex_unlock (&hippi_devp->sv_mutex);
    }
    else { /* HIP_B2H_IN_DONE */

	ulpp->rd_count = b2hp->b2h_l;
	if ( (int)b2hp->b2h_l >= 0 && (b2hp->b2h_s & B2H_ISTAT_MORE) ) {
	    ulpp->ulpFlags |= ULPFLAG_R_MORE;
	}

	/* DMA complete.  Wake up process. */
	vsema( &ulpp->rd_dmadn );
    }
}

/* =====================================================================
 *		ERROR HANDLING ENTRY POINTS
 */
static int
hps_error_handler(void *einfo,
		    int error_code,
		    ioerror_mode_t mode,
		    ioerror_t *ioerror)
{
	hippi_vars_t   *hippi_devp = (hippi_vars_t *)einfo;
	vertex_hdl_t	vhdl = hippi_devp->dev_vhdl;
	char	       *ecname;
	iopaddr_t erraddr = IOERROR_GETVALUE(ioerror,busaddr);

	switch (error_code) {

	case PIO_READ_ERROR:
		ecname = "PIO Read Error";
		break;

	case PIO_WRITE_ERROR:
		ecname = "PIO Write Error";
		break;

	case DMA_READ_ERROR:
		ecname = "DMA Read Error";
		break;

	case DMA_WRITE_ERROR:
		ecname = "DMA Write Error";
		break;

	default:
		ecname = "Unknown Error Type";
		break;
	}

	cmn_err(CE_ALERT,
		"hps %v: %s (code=%d,mode=%d) at PCI address 0x%X\n",
		vhdl, ecname, error_code, mode, erraddr);
	/*
	 * XXX - TBD: Clean up and reset card.
	 */

	return IOERROR_HANDLED;
}

/************************************************************************
 *			FLASH EEPROM routines				*
 ************************************************************************/

#define HPS_PROM_SECTOR_SIZE 	0x4000	/* 16K */
#define HPS_PROM_NSECTORS	8
#define HPS_PROM_SIZE		0x20000	/* 128K */

/* Command byte values for Am29F010 Flash Memory */
#define FLASH_CYCLE_1		0xaa
#define FLASH_CYCLE_2		0x55
#define FLASH_RESET		0xf0
#define FLASH_PROGRAM		0xa0
#define FLASH_ERASE		0x80
#define FLASH_ERASECHIP		0x10
#define FLASH_ERASESECTOR	0x30

#define FLASH_DQ_POLLING	0x80
#define FLASH_DQ_TOGGLE		0x40
#define FLASH_DQ_TIMEOUT	0x20
#define FLASH_DQ_SERSTMR	0x08

/* hps_readversions()
 * Read version numbers from EEPROM.
 * This routine is called with the LINC held in reset
 */
static void
hps_readversions (hippi_vars_t * hp)
{
	volatile __uint32_t * lregs;
	volatile u_char *fvers;
	u_char *dvers;
	int i;
	volatile lincprom_fhdr_t *fhdrp;
	u_int magic;

	/* Get byte-bus in sane mode. */
	/* Clear all the error bits: */
	lregs = hp->src_lincregs;
	lregs[(LINC_BYTEBUS_CONTROL-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)] =
		LINC_BBCSR_EN_ERR | LINC_BBCSR_RST_ERR |	/* RW1C */
		LINC_BBCSR_PROM_SZ_ERR | LINC_BBCSR_PAR_ERR |	/* RW1C */
		LINC_BBCSR_WR_TO |				/* RW1C */
		LINC_BBCSR_BBUS_EN | LINC_BBCSR_PULS_WID_W(0x0f) |
		LINC_BBCSR_A_TO_CS_W(0) | LINC_BBCSR_CS_TO_EN_W(0) |
		LINC_BBCSR_EN_WID_W(0x04) | LINC_BBCSR_CS_TO_A_W(0);

	/* Set the values we want: */
	lregs[(LINC_BYTEBUS_CONTROL-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)] =
		LINC_BBCSR_BBUS_EN | LINC_BBCSR_PULS_WID_W(0x0f) |
		LINC_BBCSR_A_TO_CS_W(0) | LINC_BBCSR_CS_TO_EN_W(0) |
		LINC_BBCSR_EN_WID_W(0x04) | LINC_BBCSR_CS_TO_A_W(0);


	lregs = hp->dst_lincregs;
	lregs[(LINC_BYTEBUS_CONTROL-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)] =
		LINC_BBCSR_EN_ERR | LINC_BBCSR_RST_ERR |	/* RW1C */
		LINC_BBCSR_PROM_SZ_ERR | LINC_BBCSR_PAR_ERR |	/* RW1C */
		LINC_BBCSR_WR_TO |				/* RW1C */
		LINC_BBCSR_BBUS_EN | LINC_BBCSR_PULS_WID_W(0x0f) |
		LINC_BBCSR_A_TO_CS_W(0) | LINC_BBCSR_CS_TO_EN_W(0) |
		LINC_BBCSR_EN_WID_W(0x04) | LINC_BBCSR_CS_TO_A_W(0);

	/* Set the values we want: */
	lregs[(LINC_BYTEBUS_CONTROL-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)] =
		LINC_BBCSR_BBUS_EN | LINC_BBCSR_PULS_WID_W(0x0f) |
		LINC_BBCSR_A_TO_CS_W(0) | LINC_BBCSR_CS_TO_EN_W(0) |
		LINC_BBCSR_EN_WID_W(0x04) | LINC_BBCSR_CS_TO_A_W(0);

	/* Fheader is located at start of sector 2 in EEPROM. Each
	   EEPROM sector is 0x4000 (16K) */
	fhdrp=(volatile lincprom_fhdr_t *)((u_char *)hp->src_eeprom + 0x8000);

	/* First check the magic number to make sure the fhdr has been
	   programmed. */
	dvers = (u_char *) &magic;
	fvers = (volatile u_char *) &fhdrp->magic;
	for ( i = 0; i < sizeof(magic) ; i++)
	  *dvers++ = *fvers++;
	if (magic != LINCPROM_FHDR_MAGIC) {
	    cmn_err(CE_ALERT,
		    "hps_readversions(): %v: Invalid Fheader in src EEPROM.\n",
		    hp->src_vhdl);
	    dprintf (5, ("hps_readversions(): %x: magic = %x\n",
			 fhdrp, magic));
	    hp->hi_srcvers.whole = 0;
	}
	else {
	    dvers = (u_char *) &hp->hi_srcvers;
	    fvers = (volatile u_char *) &fhdrp->firmware_vers;
	    for ( i = 0; i < sizeof(hippi_linc_fwvers_t) ; i++)
	      *dvers++ = *fvers++;
	    dprintf (5, ("hps_readversions(): %v: src fw version = %x\n",
			 hp->src_vhdl, hp->hi_srcvers.whole));

	    /* Also read the MAC address, which is in sector 1 of the
	     * src-side LINC. */
	    dvers = (u_char *) &hp->mac_addr[0];
	    fvers = (u_char *)hp->src_eeprom + 0x4000;
	    for ( i = 0; i < 6 ; i++)
	      *dvers++ = *fvers++;
	    dprintf (5, ("hps_readversions(): %v: MAC addr = %x:%x:%x:%x:%x:%x\n",
			 hp->src_vhdl, hp->mac_addr[0], hp->mac_addr[1],
			 hp->mac_addr[2], hp->mac_addr[3],
			 hp->mac_addr[4], hp->mac_addr[5]));
	    /* SGI mac addresses begin with 08:00:69: */
	    if ((hp->mac_addr[0] != 8) || (hp->mac_addr[1] != 0) ||
		(hp->mac_addr[2] != 0x69)) {
		cmn_err(CE_ALERT,
"hps: %v: This card does not have a valid MAC address programmed.\n",
		    hp->src_vhdl);
	    }
	}

	/* Now repeat the whole thing, to the other side. */
	fhdrp=(volatile lincprom_fhdr_t *)((u_char *)hp->dst_eeprom + 0x8000);

	/* First check the magic number to make sure the fhdr has been
	   programmed. */
	dvers = (u_char *) &magic;
	fvers = (volatile u_char *) &fhdrp->magic;
	for ( i = 0; i < sizeof(hippi_linc_fwvers_t) ; i++)
	  *dvers++ = *fvers++;
	if (magic != LINCPROM_FHDR_MAGIC) {
	    cmn_err(CE_ALERT,
		    "hps_readversions(): %v: Invalid Fheader in dst EEPROM.\n",
		    hp->dst_vhdl);
	    dprintf (5, ("hps_readversions(): %x: magic = %x\n",
			 fhdrp, magic));
	    hp->hi_dstvers.whole = 0;
	}
	else {
	    dvers = (u_char *) &hp->hi_dstvers;
	    fvers = (volatile u_char *) &fhdrp->firmware_vers;
	    for ( i = 0; i < sizeof(hippi_linc_fwvers_t) ; i++)
	      *dvers++ = *fvers++;
	    dprintf (5, ("hps_readversions(): %v: dst fw version = %x\n",
			 hp->dst_vhdl, hp->hi_dstvers.whole));
	}

}

/* Called with devsema held. Caller checks to make sure that 
 * board is in down state first.
 */
static int
hps_ioc_flash( hippi_vars_t *hippi_devp, int cmd, void *arg )
{
	int		error = 0, i, attempts, doneflag;
	int		s;
	int		is_src = 0;
	u_long		sectormask;
	volatile u_char	*flash;
	volatile __uint32_t * lregs;

	hip_flash_req_t  flasharg;
	u_char		dq, *image;

	if (!_CAP_ABLE(CAP_DEVICE_MGT)) {
	    	return EPERM;
	}

	/* Don't allow if card is up */
	if ( (hippi_devp->hi_state & HI_ST_UP) != 0 ) {
	    	return EINVAL;
	}

	if (cmd == HIPPI_ERASE_FLASH) {
		sectormask = (u_long) arg;
		if (sectormask & 0xff000000)
			is_src = 1;
		sectormask &= 0xffffff;
		dprintf (1, ("hps_flash_request: ERASE sectormask = %x.\n",
			      sectormask));
		if ( (sectormask & ((u_int)-1<<HPS_PROM_NSECTORS)) != 0)
			return EINVAL;
		cmn_err( CE_NOTE, "hippi%d: %v: erasing flash EEPROM.\n",
			 hippi_devp->unit, 
			 is_src ? hippi_devp->src_vhdl : hippi_devp->dst_vhdl);
	} else {    /* program or get */
		if ( copyin( arg, &flasharg, sizeof(flasharg) ) < 0 )
			return EFAULT;
		if ( flasharg.offset >= HPS_PROM_SIZE ||
		     flasharg.offset + flasharg.len > HPS_PROM_SIZE )
			return EINVAL;
		is_src = flasharg.is_src; 
		image = kmem_alloc( flasharg.len, KM_SLEEP );
		ASSERT( image != 0 );
		if ((cmd == HIPPI_PGM_FLASH) &&
		    (copyin ((caddr_t)flasharg.data, image, flasharg.len)<0)){
			kmem_free( image, flasharg.len );
			return EFAULT;
		}
		dprintf (1, 
("hps_flash_request: cmd=0x%x, is_src=%d, offset=0x%x, len=0x%x, data=%x\n",
			cmd, is_src, flasharg.offset, flasharg.len,
			flasharg.data));

		if (cmd == HIPPI_PGM_FLASH)
		    cmn_err(CE_NOTE, "hippi%d: %v: programming flash EEPROM.\n",
			    hippi_devp->unit,
			    is_src ? hippi_devp->src_vhdl : hippi_devp->dst_vhdl);
	}

	if (is_src) {
		flash = (volatile u_char *) hippi_devp->src_eeprom;
		lregs = hippi_devp->src_lincregs;
	} else {
		flash = (volatile u_char *) hippi_devp->dst_eeprom;
		lregs = hippi_devp->dst_lincregs;
	}

	if (cmd != HIPPI_GET_FLASH) {
		/* Hold the board in reset mode - BOTH SIDES! */
		hippi_devp->src_lincregs[(LINC_LCSR-LINC_MISC_REGS_ADDR)/
					 sizeof(uint32_t)] = _LINC_RESET;
		hippi_devp->dst_lincregs[(LINC_LCSR-LINC_MISC_REGS_ADDR)/
					 sizeof(uint32_t)] = _LINC_RESET;
	}

	/* Get byte-bus in sane mode. */
	/* Clear all the error bits: */
	lregs[(LINC_BYTEBUS_CONTROL-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)] =
		LINC_BBCSR_EN_ERR | LINC_BBCSR_RST_ERR |	/* RW1C */
		LINC_BBCSR_PROM_SZ_ERR | LINC_BBCSR_PAR_ERR |	/* RW1C */
		LINC_BBCSR_WR_TO |				/* RW1C */
		LINC_BBCSR_BBUS_EN | LINC_BBCSR_PULS_WID_W(0x0f) |
		LINC_BBCSR_A_TO_CS_W(0) | LINC_BBCSR_CS_TO_EN_W(0) |
		LINC_BBCSR_EN_WID_W(0x04) | LINC_BBCSR_CS_TO_A_W(0);

	/* Set the values we want: */
	lregs[(LINC_BYTEBUS_CONTROL-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)] =
		LINC_BBCSR_BBUS_EN | LINC_BBCSR_PULS_WID_W(0x0f) |
		LINC_BBCSR_A_TO_CS_W(0) | LINC_BBCSR_CS_TO_EN_W(0) |
		LINC_BBCSR_EN_WID_W(0x04) | LINC_BBCSR_CS_TO_A_W(0);
	dprintf (1, ("Before flash operation, BBCSR = %x\n",
		    lregs[(LINC_BYTEBUS_CONTROL-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)]));

	switch ( cmd ) {
	case HIPPI_ERASE_FLASH:
		s = splhi();

		/* Send EEPROM a RESET command. */
		flash[ 0x5555 ] = FLASH_CYCLE_1;
		flash[ 0x2aaa ] = FLASH_CYCLE_2;
		flash[ 0x5555 ] = FLASH_RESET;

		/* Send EEPROM an ERASESECTOR command. */
		flash[ 0x5555 ] = FLASH_CYCLE_1;
		flash[ 0x2aaa ] = FLASH_CYCLE_2;
		flash[ 0x5555 ] = FLASH_ERASE;
		flash[ 0x5555 ] = FLASH_CYCLE_1;
		flash[ 0x2aaa ] = FLASH_CYCLE_2;
		for ( i = 0; i < HPS_PROM_NSECTORS; i++)
		   if ( (sectormask & (1<<i)) != 0 ) {
			flash[ i * HPS_PROM_SECTOR_SIZE ] = FLASH_ERASESECTOR;
			dq = flash[ i * HPS_PROM_SECTOR_SIZE ];
			if ( 0 != (dq & FLASH_DQ_SERSTMR) ) {
				dprintf( 0, ( "hps_flash_request: "
					"sector erase timer expired!\n" ));
				error = EIO;
				break;
			}
		   }
		splx( s );
		if ( error )
			break;

		attempts=0;
		do {
		    DELAYBUS( 1000 );

		    /* Go through each sector being erased and see
		     * if any are not done or have timed-out.
		     */
		    doneflag = 1;
		    for (i=0; i<HPS_PROM_NSECTORS; i++)
			if ( (sectormask & (1<<i)) != 0 ) {

			    dq = flash[ i*HPS_PROM_SECTOR_SIZE ];

			    if ( (dq & FLASH_DQ_POLLING) == 0 ) {

				/* Has it timed out? */
				if ( (dq & FLASH_DQ_TIMEOUT) != 0 ) {
					doneflag = 1;
					dprintf( 0, ("hps_flash_request: "
					   "timed out erasing sector %d\n",
					   i ));
					break;
				}
				else
					/* This sector is not done. */
					doneflag = 0;

			    }
			}

		} while ( ++attempts < 10000 && ! doneflag );

		if ( attempts >= 10000 ||
		     (dq & (FLASH_DQ_POLLING|FLASH_DQ_TIMEOUT)) ==
			FLASH_DQ_TIMEOUT ) {
			dprintf( 0, ("hps_flash_request: "
				"trouble erasing\n" ));
			error = EIO;
		}
/* XXX - if we just erased flash, surely we don't want to take the
	 board out of reset? */
		break;

	case HIPPI_PGM_FLASH:
		/* Send EEPROM another RESET command. */
		flash[ 0x5555 ] = FLASH_CYCLE_1;
		flash[ 0x2aaa ] = FLASH_CYCLE_2;
		flash[ 0x5555 ] = FLASH_RESET;

		/* Program the FLASH */
		for (i=0; !error && i<flasharg.len; i++) {

			s = splhi();

			/* Send EEPROM the PROGRAM command. */
			flash[ 0x5555 ] = FLASH_CYCLE_1;
			flash[ 0x2aaa ] = FLASH_CYCLE_2;
			flash[ 0x5555 ] = FLASH_PROGRAM;
			flash[ flasharg.offset + i ] = image[ i ];

			DELAYBUS( 14 );
			/* Wait for programming to complete. */
			attempts=0;
			do {
				DELAYBUS( 10 );
				dq = flash[ flasharg.offset + i ];
			} while ( ++attempts<10000 &&
				  ((dq ^ image[i]) & FLASH_DQ_POLLING) != 0 &&
				  (dq & FLASH_DQ_TIMEOUT) == 0 );

			dq = flash[ flasharg.offset + i ];
			if ( attempts >= 10000 || dq != image[i] )
				error = EIO;
				
			splx( s );
		}

		kmem_free( image, flasharg.len );

		/* Re-read firmware version, now that we've
		   got new fw loaded. */
		hps_readversions (hippi_devp);
#if 0
		if (is_src)
	    	    hp->hi_srcvers = ??? WHERE IN EEPROM ??? 
		else
	    	    hp->hi_dstvers = ??? WHERE IN EEPROM ??? 
#endif

		/* If no errors encountered, we can take board out of reset. */
		if ((!error) && (flasharg.clear_reset)) {
			hippi_devp->src_lincregs[
			(LINC_LCSR-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)] = 
				_LINC_CLR_RESET;
			hippi_devp->dst_lincregs[
			(LINC_LCSR-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)] =
				_LINC_CLR_RESET;
		}
		break;

	case HIPPI_GET_FLASH:
		for (i=0; i<flasharg.len; i++)
			image[ i ] = flash[ flasharg.offset + i ];
		
		if ( copyout( image, (caddr_t)flasharg.data, flasharg.len )<0 )
			error = EFAULT;

		kmem_free( image, flasharg.len );
		break;

#ifdef HPS_DEBUG
	default:
		cmn_err( CE_PANIC, "hps_flash_req cmd=0x%x???\n", cmd );
#endif
	}
	dprintf (1, ("After flash operation, BBCSR = %x\n",
	lregs[(LINC_BYTEBUS_CONTROL-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)]));

	return error;
}

/* 
 * A HIPPI_SET_MACADDR ioctl invokes this routine. The MAC address is
 * the only thing in sector 1. (LINCPROM code is in sector 0, and operating
 * firmware is in sector 2.) We check to the state of the board (must be
 * down) and the MAC address for validity. Then erase sector 1 and write
 * the mac address into it.
 */
int
hps_ioc_setmac (hippi_vars_t * hippi_devp, void * arg)
{
	volatile u_char	*flash;
	volatile __uint32_t * lregs;
	volatile uint32_t 	*himrp;
	u_char	mac_addr[6];
	int	attempts, s, i;
	int	error = 0;
	u_char  dq;

	if ( copyin( arg, mac_addr, sizeof(mac_addr) ) < 0 )
		return EFAULT;

	if (!_CAP_ABLE(CAP_DEVICE_MGT)) {
	    	return EPERM;
	}

	/* Don't allow if card is up */
	if ( (hippi_devp->hi_state & HI_ST_UP) != 0 ) {
	    	return EINVAL;
	}

	/* Check for valid SGI prefix 08:00:69 */
	if ((mac_addr[0] != 8) || (mac_addr[1] != 0) || (mac_addr[2] != 0x69))
		return EINVAL;

	flash = (volatile u_char *) hippi_devp->src_eeprom;
	lregs = hippi_devp->src_lincregs;

	/* Committed now. */
	s = splhi();

	/* Hold source-side LINC in reset */
	lregs[(LINC_LCSR-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)] = _LINC_RESET;

	/* Get byte-bus in sane mode. */
	/* Clear all the error bits: */
	lregs[(LINC_BYTEBUS_CONTROL-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)] =
		LINC_BBCSR_EN_ERR | LINC_BBCSR_RST_ERR |	/* RW1C */
		LINC_BBCSR_PROM_SZ_ERR | LINC_BBCSR_PAR_ERR |	/* RW1C */
		LINC_BBCSR_WR_TO |				/* RW1C */
		LINC_BBCSR_BBUS_EN | LINC_BBCSR_PULS_WID_W(0x0f) |
		LINC_BBCSR_A_TO_CS_W(0) | LINC_BBCSR_CS_TO_EN_W(0) |
		LINC_BBCSR_EN_WID_W(0x04) | LINC_BBCSR_CS_TO_A_W(0);

	/* Set the values we want: */
	lregs[(LINC_BYTEBUS_CONTROL-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)] =
		LINC_BBCSR_BBUS_EN | LINC_BBCSR_PULS_WID_W(0x0f) |
		LINC_BBCSR_A_TO_CS_W(0) | LINC_BBCSR_CS_TO_EN_W(0) |
		LINC_BBCSR_EN_WID_W(0x04) | LINC_BBCSR_CS_TO_A_W(0);

	/* Send EEPROM a RESET command. */
	flash[ 0x5555 ] = FLASH_CYCLE_1;
	flash[ 0x2aaa ] = FLASH_CYCLE_2;
	flash[ 0x5555 ] = FLASH_RESET;

	/* Send EEPROM an ERASESECTOR command. */
	flash[ 0x5555 ] = FLASH_CYCLE_1;
	flash[ 0x2aaa ] = FLASH_CYCLE_2;
	flash[ 0x5555 ] = FLASH_ERASE;
	flash[ 0x5555 ] = FLASH_CYCLE_1;
	flash[ 0x2aaa ] = FLASH_CYCLE_2;

	/* Erase sector 1 */
	flash[ HPS_PROM_SECTOR_SIZE ] = FLASH_ERASESECTOR;
	dq = flash[ HPS_PROM_SECTOR_SIZE ];
	if ( 0 != (dq & FLASH_DQ_SERSTMR) ) {
		cmn_err ( CE_ALERT, ( "hps_ioc_setmac: "
				"EEPROM sector erase timer expired!\n" ));
		error = EIO;
		goto ret_err;
	}

	/* wait for erase to complete. */
	attempts=0;
	do {
		DELAYBUS( 1000 );
		dq = flash[ HPS_PROM_SECTOR_SIZE ];

		if ( (dq & FLASH_DQ_POLLING) != 0 ) 
			break;

		/* Has it timed out? */
		if ( (dq & FLASH_DQ_TIMEOUT) != 0 ) {
			cmn_err ( CE_ALERT, "hps_ioc_setmac: "
				   "timed out erasing sector 1\n") ;
			error = EIO;
			goto ret_err;
		}
	} while ( ++attempts < 10000 );

	if ( attempts >= 10000 ) {
		cmn_err( CE_ALERT,
			"hps_ioc_setmac: trouble erasing sector 1\n");
		error = EIO;
		goto ret_err;
	}

	/* Erase went OK, so now write the MAC address. */
	/* Send EEPROM another RESET command. */
	flash[ 0x5555 ] = FLASH_CYCLE_1;
	flash[ 0x2aaa ] = FLASH_CYCLE_2;
	flash[ 0x5555 ] = FLASH_RESET;

	/* Program the FLASH */
	for (i=0; !error && i< 6; i++) {
		/* Send EEPROM the PROGRAM command. */
		flash[ 0x5555 ] = FLASH_CYCLE_1;
		flash[ 0x2aaa ] = FLASH_CYCLE_2;
		flash[ 0x5555 ] = FLASH_PROGRAM;
		flash[ HPS_PROM_SECTOR_SIZE + i ] = mac_addr[ i ];

		DELAYBUS( 14 );
		/* Wait for programming to complete. */
		attempts=0;
		do {
			DELAYBUS( 10 );
			dq = flash[ HPS_PROM_SECTOR_SIZE + i ];
		} while ( ++attempts<10000 &&
			  ((dq ^ mac_addr[i]) & FLASH_DQ_POLLING) != 0 &&
			  (dq & FLASH_DQ_TIMEOUT) == 0 );

		dq = flash[ HPS_PROM_SECTOR_SIZE + i ];
		if ( attempts >= 10000 || dq != mac_addr[i] )
			error = EIO;
	}


ret_err:
	/* Take LINC out of reset */
	lregs[(LINC_LCSR-LINC_MISC_REGS_ADDR)/sizeof(uint32_t)] = _LINC_CLR_RESET;

	/* Mask off interrupts */
	himrp = hippi_devp->src_lincregs +
	   ((LINC_HOST_INTERRUPT_MASK-LINC_MISC_REGS_ADDR)/sizeof(uint32_t));
	*himrp = 0xffffffff;

	himrp = hippi_devp->dst_lincregs +
	   ((LINC_HOST_INTERRUPT_MASK-LINC_MISC_REGS_ADDR)/sizeof(uint32_t));
	*himrp = 0xffffffff;

	splx( s );
	if (error)
		return error;

	/* Finally, read back the programmed MAC address. */
	for (i = 0; i < 6; i++)
		hippi_devp->mac_addr[i] = flash[ HPS_PROM_SECTOR_SIZE + i ];

	/* Hit board with another reset */

	return 0;
}

#if HPS_CALLBACK /* register call back */
int
hippiregister_callback(dev_t dev, void(*proc)(void *,caddr_t,int,int),
	caddr_t arg)
{
	vertex_hdl_t    vhdl = dev_to_vhdl(dev);
	hps_soft_t      *soft = hps_soft_get(vhdl);
	hippi_vars_t    *hippi_devp = soft->hippi_devp;
	struct hippi_fp_ulps *ulpp;
 
	ASSERT (soft->isclone);
 
	psema( &hippi_devp->devsema, PZERO );
	if ( soft->ulpIndex > HIPPIFP_MAX_OPEN_ULPS )
		return ENXIO; /* unbound */

	ulpp = & hippi_devp->ulp[ soft->ulpIndex ];

	ulpp->rd_input_proc = proc; /* once the mad driver registers it */
	ulpp->rd_input_arg = arg;		/* should always be there */
	vsema( &hippi_devp->devsema );

	return 0;
}
#endif /* register call back */

int
hippiread(dev_t dev, uio_t *uiop, cred_t *crp)
{
	return hps_read( dev, uiop, crp);
}

int
hippiwrite(dev_t dev, uio_t *uiop, cred_t *crp)
{
	return hps_write( dev, uiop, crp);
}

int
hippiopen(dev_t *devp, int oflag, int otyp, cred_t *crp)
{
	return hps_open( devp, oflag, otyp, crp);
}

int
hippiclose(dev_t dev, int oflag, int otyp, cred_t *crp)
{
	return hps_close(dev, oflag, otyp,  crp);
}


int
hippiioctl(dev_t dev, int cmd, void *arg,
            int mode, cred_t *crp, int *rvalp)
{
        return hps_ioctl( dev, cmd, arg, mode, crp, rvalp);
}

