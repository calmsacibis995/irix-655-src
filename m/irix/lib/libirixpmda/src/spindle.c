/*
 * --- Disk spindle statistics --
 * This is mostly ripped out of sardc.
 * Mark Goodwin, markgw@sgi.com Mon Nov 29 13:27:21 EST 1993
 *
 */

#ident "$Id: spindle.c,v 1.44 1998/09/07 09:20:01 tes Exp $"

#if defined(IRIX6_2) || defined(IRIX6_3)
#define _KMEMUSER
#endif

#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/var.h>
#include <sys/sema.h>
#include <sys/iobuf.h>
#include <sys/stat.h>
#include <sys/elog.h>
#include <sys/sbd.h>
#include <sys/immu.h>
#include <sys/sysmp.h>
#include <sys/sysinfo.h>
#include <sys/file.h>
#include <sys/flock.h>


#if defined(IRIX6_2) || defined(IRIX6_3)
/* /dev/kmem method */
#include <sys/kmem.h>
#include <sys/buf.h>
#include <sys/dvh.h>
#include <sys/dkio.h>
#include <sys/scsi.h>
#include <sys/dksc.h>
#include <sys/usraid.h>
#include <sys/dmamap.h>
#else
/* hardware graph method */
#include <sys/iograph.h>
#include <sys/attributes.h>
#include <invent.h>
#include <ftw.h>
#endif /* defined(IRIX6_2) || defined(IRIX6_3) */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <invent.h>
#include <syslog.h>
#include <errno.h>
#include <dslib.h>

#include "./spindle.h"
#if defined(IRIX6_2) || defined(IRIX6_3)
#include "./kmemread.h"
#endif
#include "pmapi.h"
#include "impl.h"
#include "./cluster.h"

int _pmSpindleCount = 0;
static spindle *s_stats = (spindle *)NULL;
static int initialized=0;
static int controller_initialized=0;

static int _pmControllerCount = 0;
static controller *c_stats = NULL;
static int *controller_disk_include = NULL;

#if BEFORE_IRIX6_5

#define SN_USE_BYTES 44 /* 36 + 8 */
typedef struct {	/* result from SCSI Inquiry command */
    char	fill0[36];
    char	sn[20];	/* serial number in first 8 bytes of vendor specific */
			/* rest is of no concern */
} inqres;

#else

typedef	struct
{
	unchar	pqt:3;	/* peripheral qual type */
	unchar	pdt:5;	/* peripheral device type */
	unchar	rmb:1,	/* removable media bit */
		dtq:7;	/* device type qualifier */
	unchar	iso:2,	/* ISO version */
		ecma:3,	/* ECMA version */
		ansi:3;	/* ANSI version */
	unchar	aenc:1,	/* async event notification supported */
		trmiop:1,	/* device supports 'terminate io process msg */
		res0:2,	/* reserved */
		respfmt:4;	/* SCSI 1, CCS, SCSI 2 inq data format */
	unchar	ailen;	/* additional inquiry length */	
	unchar	res1;	/* reserved */
	unchar	res2;	/* reserved */
	unchar	reladr:1,	/* supports relative addressing (linked cmds) */
		wide32:1,	/* supports 32 bit wide SCSI bus */
		wide16:1,	/* supports 16 bit wide SCSI bus */
		synch:1,	/* supports synch mode */
		link:1,	/* supports linked commands */
		res3:1,	/* reserved */
		cmdq:1,	/* supports cmd queuing */
		softre:1;	/* supports soft reset */
	unchar	vid[8];	/* vendor ID */
	unchar	pid[16];	/* product ID */
	unchar	prl[4];	/* product revision level*/
	unchar	vendsp[20];	/* vendor specific; typically firmware info */
	unchar	res4[40];	/* reserved for scsi 3, etc. */
	unchar	vendsp2[159];	/* more vendor specific (fill to 255 bytes) */
	/* more vendor specific information may follow */
} inqdata;

/* inquiry cmd that does vital product data as spec'ed in SCSI2 */
static int
vpinquiry12( struct dsreq *dsp, caddr_t data, long datalen, char vu,
  int page)
{
  dsp->ds_time = 1000 * 10; /* 10 seconds */
  fillg0cmd(dsp, (uchar_t *)CMDBUF(dsp), G0_INQU, 1, page, 0, B1(datalen),
        B1(vu<<6));
  filldsreq(dsp, (uchar_t *)data, datalen, DSRQ_READ|DSRQ_SENSE);
  return(doscsireq(getfd(dsp), dsp));
}

#endif

static int
get_scsi_sn(char *disk, char **res)
{
    char	*p;
    char	*q;
    int		lun = 0;
    int		sts = 0;
    static char	*sname = "/dev/scsi/scxxxdxxxlxx";
    dsreq_t	*dsp = NULL;
#if BEFORE_IRIX6_5
    static 	int inqbuf[sizeof(inqres)/sizeof(int)];
#else
#define PAGE_SERIAL	0x80
    int		i;
    int		serial = 0;
    static 	int vpinqbuf[sizeof(inqdata)/sizeof(int)];
    unsigned char *vpinq;
    int		vpinqbuf0[sizeof(inqdata)/sizeof(int)];
#endif

    *res = NULL;

    /*
     * translate
     *	dskxxxdyyy     -> /dev/scsi/scxxxdyyyl0
     *	dskxxxdyyylzzz -> /dev/scsi/scxxxdyyylzzZ
     */
    if (strncmp(disk, "dks", 3) != 0)
	goto finish;
    p = &sname[12];
    q = &disk[3];
    while (*q && *q != 'd')
	*p++ = *q++;
    if (*q != 'd')
	goto finish;
    *p++ = *q++;
    while (*q) {
	if (*q == 'l')
	    lun = 1;
	*p++ = *q++;
    }
    if (!lun) {
	*p++ = 'l';
	*p++ = '0';
    }
    *p = '\0';

    if (access(sname,  F_OK)) {
	__pmNotifyErr(LOG_WARNING, "get_scsi_sn: SCSI device \"%s\" does not exist ... need to run MAKEDEV?\n", sname);
	sts = -ENOENT;
	goto finish;
    }

    dsp = dsopen(sname, O_RDONLY);
    if (dsp == NULL) {
	sts = -oserror();
        if (getuid() == (uid_t)0) /* only display if root */
	    __pmNotifyErr(LOG_ERR, "get_scsi_sn: dsopen(\"%s\"): %s\n", 
	                 sname, strerror(-sts));
	goto finish;
    }

    /*
     * have SCSI handle, now do Inquiry ...
     *
     * We do it two different methods depending on IRIX version.
     * For >= 6.5 we do it the scsicontrol.c (eoe/cmd/scsicontrol) way,
     * which looks at the vital product data pages for Serial#.
     * For < 6.5 we do it the way we used to, using an Inquiry cmd.
     */

#if BEFORE_IRIX6_5
    fillg0cmd(dsp, (uchar_t *)CMDBUF(dsp), G0_INQU, 0, 0, 0, B1(sizeof(inqbuf)), B1(0));
    filldsreq(dsp, (uchar_t *)inqbuf, sizeof(inqbuf), DSRQ_READ|DSRQ_SENSE);
    dsp->ds_time = 1000 * 30; /* 30 seconds */
    if (doscsireq(getfd(dsp), dsp) != 0) {
	sts = -oserror();
	__pmNotifyErr(LOG_ERR, "get_scsi_sn: inquiry failed(\"%s\"): %s\n", 
		     sname, strerror(-sts));
	goto finish;
    }

    if (DATASENT(dsp) >= SN_USE_BYTES) {
        *res = ((inqres *)&inqbuf)->sn;
        (*res)[8] = '\0';
    }
    else {
        sts = PM_ERR_GENERIC;
	goto finish;
    }
#else
    if(vpinquiry12(dsp, (char *)vpinqbuf0, sizeof(vpinqbuf0), 0, 0) != 0) {
	sts = -oserror();
	__pmNotifyErr(LOG_ERR, "get_scsi_sn: vital data inquiry failed(\"%s\"): %s\n", 
		     sname, strerror(-sts));
	goto finish;
    }

    if(DATASENT(dsp) <4) {
	/* vital data inquiry OK, but says no pages supported (page 0) */
	sts = PM_ERR_GENERIC;
	goto finish;
    }

    vpinq = (unsigned char *)vpinqbuf0;
    for(i = vpinq[3]+3; i>3; i--) {
	    if(vpinq[i] == PAGE_SERIAL) {
		    serial = 1;
	    }
    }

    vpinq = (unsigned char *)vpinqbuf;

    if (serial) {
	if (vpinquiry12(dsp, (char *)vpinqbuf, sizeof(vpinqbuf), 0, PAGE_SERIAL) != 0) {
	    sts = -oserror();
	    __pmNotifyErr(LOG_ERR, "get_scsi_sn: failed to get serial# (\"%s\"): %s\n", 
			 sname, strerror(-sts));
	    goto finish;
	}

	if (DATASENT(dsp) > 3) {
	    int len = vpinq[3];
	    *res = (char*)vpinq+4;
	    (*res)[len] = '\0';
	}
	else {
	    sts = PM_ERR_GENERIC;
	    goto finish;
	}
    }
    else {
	sts = PM_ERR_GENERIC;
	goto finish;
    }
#endif


finish:
    if (dsp != NULL)
	dsclose(dsp);
    return sts;
}

static spindle *
new_spindle(void)
{

    static int ndevs_max = 0;

    _pmSpindleCount++;
    while (_pmSpindleCount >= ndevs_max) {
	ndevs_max += 4;
	s_stats = (spindle *)realloc(s_stats, ndevs_max * sizeof(spindle));
    }

     if (s_stats == NULL) {
	__pmNotifyErr(LOG_ERR, "spindle_stats_init: malloc or realloc failed: %s", pmErrStr(-oserror()));
	return NULL;
    }

    memset(&s_stats[_pmSpindleCount-1], 0, sizeof(spindle));
    s_stats[_pmSpindleCount-1].s_drivenumber = _pmSpindleCount-1; /* drive number */
    return(&s_stats[_pmSpindleCount-1]);
}

void
delete_last_drive(void)
{
    _pmSpindleCount--;
}

spindle *
spindle_stats(int *inclusion_map)
{
    spindle *s = s_stats;
    int	i;
    struct iotime iotim;

#if !defined(IRIX6_2) && !defined(IRIX6_3)
    int dkiotimesz = sizeof(iotim);
#endif

    if (!initialized) {
	__pmNotifyErr(LOG_WARNING, "spindle_stats: spindle_stats_init() has not been called");
	return NULL;
    }

    for (i=0; i < _pmSpindleCount; i++) {

#if defined(IRIX6_2) || defined(IRIX6_3)
	/* active non-excluded drives only */
	if (s[i].s_iotime_addr == (__psunsigned_t)0) {
#ifdef PCP_DEBUG
	    if (pmIrixDebug & DBG_IRIX_DISK)
	    	__pmNotifyErr(LOG_WARNING, 
			     "spindle_stats: s_iotime_addr is null for %d\n",
			     i);
#endif
	    continue;
	}
#endif
	
	if (inclusion_map != NULL && inclusion_map[i] == 0) {
	    /* do not collect stats for this drive */
#ifdef PCP_DEBUG
	    if (pmIrixDebug & DBG_IRIX_DISK)
	    	__pmNotifyErr(LOG_WARNING, 
			     "spindle_stats: inclusion_map is null for %d\n",
	    		     i);
#endif
	    continue;
	}

#if defined(IRIX6_2) || defined(IRIX6_3)
	/*
	 * kmem reader method
	 */
	if (kmemread(s[i].s_iotime_addr, &iotim, sizeof(iotim)) < 0) {
#ifdef PCP_DEBUG
	    if (pmIrixDebug & DBG_IRIX_DISK)
	    	__pmNotifyErr(LOG_ERR, 
			     "spindle_stats: kmemread failed for %d: %s\n",
	    		     i, strerror(oserror()));
#endif
	    continue;
	}
#else
	/*
	 * hardware graph method
	 */
	if (attr_get((char *)s[i].s_hwgname, INFO_LBL_DKIOTIME, (char *)&iotim, &dkiotimesz, 0) == -1) {
#ifdef PCP_DEBUG
	    if (pmIrixDebug & DBG_IRIX_DISK)
	    	__pmNotifyErr(LOG_ERR, 
			     "spindle_stats: attr_get failed for %s: %s\n",
	    		     s[i].s_hwgname, strerror(oserror()));
#endif
	    continue;
	}
#endif

	s[i].s_ops = iotim.io_cnt;
	s[i].s_wops = iotim.io_wops;
	s[i].s_bops = iotim.io_bcnt;
	s[i].s_wbops = iotim.io_wbcnt;
	s[i].s_act = iotim.io_act;
	s[i].s_resp = iotim.io_resp;
#if !BEFORE_IRIX6_5
	s[i].s_qlen = iotim.io_qc;
	s[i].s_sum_qlen = iotim.ios.io_misc;
#endif
    }

    return s;
}

#if !defined(IRIX6_2) && !defined(IRIX6_3)
/*ARGSUSED*/
static int
spindle_volume_filter(inventory_t *inventinfo, void *calldata)
{
    spindle	*drive;
    char	lun_ext[16];
    char	*sn;
    char	disk_rpath[MAXPATHLEN];
    char	disk_alias_name[MAXPATHLEN];
    int		sts = 0;

    if ((inventinfo->inv_class == INV_DISK && inventinfo->inv_type == INV_SCSIDRIVE) ||
	(inventinfo->inv_class == INV_SCSI && inventinfo->inv_type == INV_CDROM)) {

	drive = new_spindle();
	drive->s_inv_type = INV_SCSIDRIVE;

	if (inventinfo->inv_class == INV_DISK && inventinfo->inv_type == INV_SCSIDRIVE &&
	    inventinfo->inv_state) {
	    /* disk, not CDROM ... see hinv.c */
	    drive->s_lun = inventinfo->inv_state & 0xff;
	    if (drive->s_lun > 0)
		sprintf(lun_ext, "l%d", drive->s_lun);
	    else
	    	lun_ext[0] = (char)NULL;
	}
	else {
	    drive->s_lun = 0;
	    lun_ext[0] = (char)NULL;
	}

	drive->s_ctlr = inventinfo->inv_controller;
	drive->s_unit = inventinfo->inv_unit;

	sprintf(drive->s_dname, "dks%dd%d%s", drive->s_ctlr, drive->s_unit, lun_ext);
	drive->s_wname = (int)strlen(drive->s_dname);

	/* internal instance id */
	drive->s_id = (drive->s_ctlr << 16) | (drive->s_unit << 8) | drive->s_lun;

	drive->s_hwgname = NULL;
	sprintf(disk_alias_name, "/hw/"EDGE_LBL_RDISK"/%svol", drive->s_dname);
	if (realpath(disk_alias_name, disk_rpath) != NULL) {
	    drive->s_hwgname = strdup(disk_rpath);
	}
	else
	    drive->s_hwgname = strdup("unknown");

	if (inventinfo->inv_class == INV_SCSI && inventinfo->inv_type == INV_CDROM) {
	    /* CDROM, don't even bother trying! */
	    sn = NULL;
	    sts = 0;
	}
	else {
	    /* SCSI disk serial number? */
	    sts = get_scsi_sn(drive->s_dname, &sn);
	}

	/* Check that the drive really exists */
	if (sts == -ENODEV || sts == -ENOENT)
	    delete_last_drive();
	else {
	    if (sn != NULL)
		drive->s_sn = strdup(sn);
	    else
		drive->s_sn = strdup("unknown");

#ifdef PCP_DEBUG
	    if (pmIrixDebug & DBG_IRIX_DISK)
		__pmNotifyErr(LOG_DEBUG, 
		     "%16s ctl=%d unit=%d lun=%d id=0x%08x sn=%8.8s type=%d\n", 
		     drive->s_dname, drive->s_ctlr, drive->s_unit, drive->s_lun,
		     drive->s_id, drive->s_sn, drive->s_inv_type);
#endif
	} 
    }

    return 0;
}

int
spindle_stats_init(int reset)
{
    int err = 0;
    char *dir = "/hw/"EDGE_LBL_RDISK;
    int i;
    struct stat buf;

    if (initialized) {
	if (reset) {
	    /* rescan */
	    initialized = 0;
	    if (_pmSpindleCount > 0 && s_stats != NULL) {
		for (i=0; i < _pmSpindleCount; i++) {
		    if (s_stats[i].s_hwgname != NULL) {
			free(s_stats[i].s_hwgname);
			s_stats[i].s_hwgname = NULL;
		    }
		    if (s_stats[i].s_sn != NULL) {
			free(s_stats[i].s_sn);
			s_stats[i].s_sn = NULL;
		    }
		}
		_pmSpindleCount = 0;
	    }
		
	}
	else {
	    __pmNotifyErr(LOG_WARNING, "spindle_stats_init: called more than once!");
	    return _pmSpindleCount; /* not a fatal error */
	}
    }

    initialized++;
    _pmSpindleCount = 0;

    if ((err = stat(dir, &buf)) == -1) {
	/* no /hw/rdisk */
	goto FAIL;
    }

    /* scan the disk inventory and allocate a spindle entry for each volume */
    scaninvent((int (*)()) spindle_volume_filter, (void *)NULL);

    /* success */
    return _pmSpindleCount;

FAIL:
    __pmNotifyErr(LOG_WARNING, "spindle_stats_init: disk stats are not available: %s", pmErrStr(err));
    return err;
}

#else /* kmem reader version */

int
spindle_stats_init(int reset)
{
    int	j,w,c,d;
    char *sn;
    spindle *drive;
    int err;

    if (initialized) {
	if (reset) {
	    /* rescan */
	    initialized = 0;
	    if (_pmSpindleCount > 0 && s_stats != NULL) {
		for (j=0; j < _pmSpindleCount; j++) {
		    if (s_stats[j].s_sn != NULL) {
			free(s_stats[j].s_sn);
			s_stats[j].s_sn = NULL;
		    }
		}
	    }
	    _pmSpindleCount = 0;
	}
	else {
	    __pmNotifyErr(LOG_WARNING, "spindle_stats_init: called more than once!");
	    return _pmSpindleCount; /* not a fatal error */
	}
    }

    initialized++;

    /*
     * The size of the dksoftc array should be the product of the greater
     * of all the architectures' SCSI_MAXCTLR and SCSI_MAXTARG and DK_MAXLU.
     * Currently, DK_MAXLU is the same for all.
     */
    if (VALID_KMEM_ADDR(kernsymb[KS_DKIOTIME].n_value) &&
	VALID_KMEM_ADDR(kernsymb[KS_DKSCINFO].n_value)) {
	struct iotime	thisiot;
	struct dkscinfo	dkscinfo;
	__psunsigned_t	*dksciotime;
	uint		dksciotimesz;


	if (kmemread(kernsymb[KS_DKSCINFO].n_value, &dkscinfo, sizeof(dkscinfo)) == (int)sizeof(dkscinfo)) {
	    dksciotimesz = (int)sizeof(__psunsigned_t) * dkscinfo.maxctlr * dkscinfo.maxtarg * SCSI_MAXLUN;
	    dksciotime = (__psunsigned_t *)malloc(dksciotimesz);
	    if (dksciotime == NULL) {
		int sts = oserror();
		__pmNotifyErr(LOG_ERR, "spindle_stats_init: dksciotime malloc failed: %s", pmErrStr(-sts));
		return -sts;
	    }
	    if (kmemread(kernsymb[KS_DKIOTIME].n_value, dksciotime, dksciotimesz) != dksciotimesz) {
		__pmNotifyErr(LOG_ERR, "spindle_stats_init: dksciotime kmemread failed: %s", pmErrStr(-oserror()));
		free(dksciotime);
		return 0;
	    }

	    c = 0;
	    for ( j = 0; j < dkscinfo.maxctlr; j++ ) {
		for ( w = 0; w < dkscinfo.maxtarg; w++ ) {
		    for (d = 0; d < SCSI_MAXLUN; d++, c++) {
			if (dksciotime[c] != (__psunsigned_t)0 &&
			    kmemread((__psunsigned_t)dksciotime[c], &thisiot, sizeof(thisiot)) == (int)sizeof(thisiot)) {
			    drive = new_spindle();
			    drive->s_iotime_addr = (__psunsigned_t)dksciotime[c];
			    /* 8 is the magic # for Everest */
			    if (j < 8) {
				    if (d == 0)
					    sprintf(drive->s_dname, "dks%dd%d", j, w);
				    else
					    sprintf(drive->s_dname, "dks%dd%dl%d", j, w, d);
			    }
			    else {
				    if (d == 0)
					    sprintf(drive->s_dname, "dks%d%dd%d", j/8, j%8, w);
				    else
					    sprintf(drive->s_dname, "dks%d%dd%dl%d", j/8, j%8, w, d);
			    }
			    drive->s_ctlr = 10*(j/8) + (j%8);
			    drive->s_unit = w | (d << 4);
			    drive->s_inv_type = INV_SCSIDRIVE;
			    drive->s_wname = (int)strlen(drive->s_dname);
			    drive->s_id = (drive->s_inv_type << 16) |
				    (drive->s_ctlr << 8) | (drive->s_unit);
			    /* SCSI serial number? */
			    err = get_scsi_sn(drive->s_dname, &sn);

			    /* check that the drive really exists */
			    if (err == -ENODEV || err == -ENOENT)
				delete_last_drive();
			    else  {
				if (sn != NULL)
				    drive->s_sn = strdup(sn);
				else
				    drive->s_sn = strdup("unknown");

#ifdef PCP_DEBUG
				if (pmIrixDebug & DBG_IRIX_DISK)
				    __pmNotifyErr(LOG_DEBUG, 
						 "%16s ctl=%d unit=%d lun=%d id=0x%08x sn=%8.8s\n", 
						 drive->s_dname, drive->s_ctlr,
						 drive->s_unit, drive->s_lun, 
						 drive->s_id, drive->s_sn);
#endif
			    }
			}
		    }
		}
	    }
	    free(dksciotime);
	}
    }

    /*
     * The size of the usraid_softc array should be the product of the greater
     * of all the architectures' SCSI_MAXCTLR and SCSI_MAXTARG and USRAID_MAXLU.
     * Currently, USRAID_MAXLU is the same for all.
     */
    if (VALID_KMEM_ADDR(kernsymb[KS_USRAID_SOFTC].n_value)) {
	/* have RAID */
	/* see comment above for info on constants */
	struct usraid_softc	*usraid_softc[144 * 16 * USRAID_MAXLU];
	struct iotime		*usraid_iotime;
	struct usraid_info	 usraid_info;

	if (kmemread(kernsymb[KS_USRAID_SOFTC].n_value, usraid_softc, sizeof(usraid_softc))>0 &&
	    kmemread(kernsymb[KS_USRAID_INFO].n_value, &usraid_info, sizeof(usraid_info))>0) {

	    /* note that only lun 0 is supported for now */
	    usraid_iotime = (struct iotime *) kernsymb[KS_USRAID_IOTIME].n_value;
	    for (j = 0; j < usraid_info.maxctlr; j++) {
		for (w = 0; w < usraid_info.maxtarg; w++, usraid_iotime++) {
		    d = USRAID_MAXLU * (w + (usraid_info.maxtarg * j));
		    if (VALID_KMEM_ADDR((__psunsigned_t)usraid_softc[d])) {
			if ((drive = new_spindle()) == NULL) {
			    err = -ENOMEM;
			    goto FAIL;
			}
			drive->s_iotime_addr = (__psunsigned_t)usraid_iotime;
			/* 8 is the magic # for Everest */
			if (j < 8)
			    sprintf(drive->s_dname, "rad%dd%d", j, w);
			else
			    sprintf(drive->s_dname, "rad%d%dd%d", j/8, j%8, w);
			drive->s_wname = (int)strlen(drive->s_dname);
			drive->s_ctlr = 10*(j/8) + (j%8);
			drive->s_unit = w;
			drive->s_inv_type = INV_SCSIRAID;
			drive->s_id = (drive->s_inv_type << 16) |
			    (drive->s_ctlr << 8) | (drive->s_unit);
			/* SCSI serial number? */
			err = get_scsi_sn(drive->s_dname, &sn);

			/* check that the drive really exists */
			if (err == -ENODEV || err == -ENOENT)
			    delete_last_drive();
			else {
			    if (sn != NULL)
				drive->s_sn = strdup(sn);
			    else
				drive->s_sn = strdup("unknown");
#ifdef PCP_DEBUG
			    if (pmIrixDebug & DBG_IRIX_DISK)
				__pmNotifyErr(LOG_DEBUG, 
					     "%16s ctl=%d unit=%d lun=%d id=0x%08x sn=%8.8s\n", 
					     drive->s_dname, drive->s_ctlr, 
					     drive->s_unit, drive->s_lun, 
					     drive->s_id, drive->s_sn);
#endif
			}
		    }
		}
	    }
	}
    }

    /* success */
    return _pmSpindleCount;

FAIL:
    _pmSpindleCount = 0;
    initialized = 0;
    __pmNotifyErr(LOG_WARNING, "spindle_stats_init: disk stats are not available: %s", pmErrStr(err));
    return err;
}
#endif /* !defined(IRIX6_2) && !defined(IRIX6_3) */


static int
drive_controller(spindle *drive, int drivenumber)
{
    int sts;
    int i;
    controller *c;
    controller *newc;
    char *s;
    static int max_controllers = 0;

    c = c_stats;
    for (i=0; i < _pmControllerCount; i++) {
	if (c[i].c_ctlr == drive->s_ctlr && c[i].c_inv_type == drive->s_inv_type) {
	    c[i].c_ndrives++;
	    c[i].c_drives = (int *)realloc(c[i].c_drives, c[i].c_ndrives * sizeof(int));
	    if (c[i].c_drives == NULL)
		goto MALLOC_FAIL;
	    c[i].c_drives[c[i].c_ndrives-1] = drivenumber;
	    /* successfully DONE */
	    return 0;
	}
    }

    /* we haven't seen this controller before */
    _pmControllerCount++;
    while (_pmControllerCount >= max_controllers) {
	max_controllers += 4;
	c_stats = (controller *)realloc(c_stats, max_controllers * sizeof(controller));
	if (c_stats == NULL)
	    goto MALLOC_FAIL;
    }
    newc = &c_stats[_pmControllerCount-1];

    newc->c_id = (drive->s_inv_type << 16) | (drive->s_ctlr << 8);
    strcpy(newc->c_dname, drive->s_dname);
    if ((s = strrchr(newc->c_dname, 'd')) != NULL)
	*s = '\0';
    newc->c_wname = (int)strlen(newc->c_dname);
    newc->c_ctlr = drive->s_ctlr;
    newc->c_inv_type = drive->s_inv_type;
    newc->c_ndrives = 1;
    if ((newc->c_drives = (int *)malloc(sizeof(int))) == NULL)
	goto MALLOC_FAIL;
    newc->c_drives[0] = drivenumber;

    /* success */
    return 0;

MALLOC_FAIL:
    sts = oserror();
    __pmNotifyErr(LOG_ERR, "controller_stats_init: malloc failed: %s", pmErrStr(-sts));
    return -sts;
}

int
controller_stats_init(int reset)
{
    int i, e;

    if (!initialized) {
	__pmNotifyErr(LOG_ERR, "controller_stats_init: spindle_stats_init() has not been called");
	return 0;
    }

    if (controller_initialized) {
	if (reset) {
	    /* rescan */
	    if (controller_disk_include != NULL)
		free(controller_disk_include);
	    controller_disk_include = NULL;
	    _pmControllerCount = 0;
	}
	else {
	    __pmNotifyErr(LOG_WARNING, "controller_stats_init: disk controller stats already initialized");
	    return _pmControllerCount;
	}
    }

    for (i=0; i < _pmSpindleCount; i++) {
	if ((e = drive_controller(&s_stats[i], i)) < 0)
	    goto FAIL;
    }

    controller_disk_include = (int *)malloc(_pmSpindleCount * sizeof(int));
    if (controller_disk_include == NULL) {
	e = -oserror();
	goto FAIL;
    }


    /* success */
    controller_initialized = 1;
    return _pmControllerCount;

FAIL:
    _pmControllerCount = 0;
    __pmNotifyErr(LOG_WARNING, "controller_stats_init: disk controller stats are not available: %s", pmErrStr(e));
    return e;
}

controller *
controller_stats(int *inclusion_map)
{
    spindle	*sps;
    controller	*cntrls;
    int		i, j;
    int		d;

    if (!initialized) {
	__pmNotifyErr(LOG_WARNING, "controller_stats: spindle_stats_init() has not been called");
	return NULL;
    }

    if (!controller_initialized) {
	/* Controller stats may not have been initialised because there
	   are no disks */
	if (_pmSpindleCount > 0)
	    __pmNotifyErr(LOG_ERR, 
			  "controller_stats: controller_stats_init() has not been called");
	return NULL;
    }

    /*
     * NOTE: one controller_stats_init has succeeded
     * do not expect controller_stats ever to fail.
     */

    cntrls = c_stats;

    if (inclusion_map != NULL) {
	/* only collect stats for disks on ``interesting'' controllers */
	for (i=0; i < _pmControllerCount; i++) {
	    for (j=0; j < cntrls[i].c_ndrives; j++) {
		controller_disk_include[cntrls[i].c_drives[j]] = inclusion_map[i];
	    }
	}
	/* get associated spindle stats */
	sps = spindle_stats(controller_disk_include);
    }
    else {
	/* get all spindle stats */
	sps = spindle_stats(NULL);
    }

    for (i=0; i < _pmControllerCount; i++) {
	if (inclusion_map != NULL && inclusion_map[i] == 0)
	    continue;
	cntrls[i].c_ops = cntrls[i].c_wops = cntrls[i].c_bops = 
		cntrls[i].c_wbops = cntrls[i].c_act = cntrls[i].c_resp = 0;
#if !BEFORE_IRIX6_5
	cntrls[i].c_qlen = cntrls[i].c_sum_qlen = 0;
#endif
	for (j=0; j < cntrls[i].c_ndrives; j++) {
	    d = cntrls[i].c_drives[j];
	    cntrls[i].c_ops += sps[d].s_ops;
	    cntrls[i].c_wops += sps[d].s_wops;
	    cntrls[i].c_bops += sps[d].s_bops;
	    cntrls[i].c_wbops += sps[d].s_wbops;
	    cntrls[i].c_act += sps[d].s_act;
	    cntrls[i].c_resp += sps[d].s_resp;
#if !BEFORE_IRIX6_5
	    cntrls[i].c_qlen += sps[d].s_qlen;
	    cntrls[i].c_sum_qlen += sps[d].s_sum_qlen;
#endif
	}
    }

    return cntrls;
}
