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
 * hps_ext.h
 *
 *	Header file for xtalk HIPPI-Serial card driver and friends.
 *	This file contains data structures and prototypes which bypass
 *	driver needs.
 */
#ifndef __HPS_EXT_H
#define __HPS_EXT_H
#ident "$Revision: 1.10 $ $Date: 1997/08/28 17:08:11 $"

/* ====================================================================
 * hps prototypes are for use by Bypass driver
 */
/*
 * Stuff that the bp device will need to know about. To get this stuff
 * from the hps driver, need to call hps_get_friendinfo() with a cookie
 * that was handed down from hps in the hippibp_attach() call.
 */
typedef struct hps_friend_info {
    int		    unit;
    lock_t	    *shc_slock;	/* src-side spinlock for all HCMD's */
    lock_t	    *dhc_slock;	/* dst-side spinlock for all HCMD's */
    volatile void   *shc_area;	/* PIO address of src HC area. */
    volatile void   *dhc_area;	/* PIO address of dst HC area. */
    volatile void   *sbufmem;	/* PIO base address of src SDRAM */
    volatile void   *dbufmem;	/* PIO base address of dst SDRAM */
    vertex_hdl_t    dcnctpt;	/* LINC 0 connect point */
    vertex_hdl_t    scnctpt;	/* LINC 1 connect point */
} hps_friend_info_t;

extern hps_friend_info_t * hps_get_friendinfo (vertex_hdl_t bpdev,
						hps_friend_info_t *,
						void * cookie);
/* 
 * Returns struct stuffed with goodies to allow access to hardware.
 * Meant to be called out of hippibp_attach() routine.
 */

/*
 * This next set of routines is for use by bypass driver when it needs
 * to write HC cmds to src or dst LINC. The model is, (say writing to
 * src side):
 * 
 *	    s = mutex_spinlock (bp->shc_slock)
 *	    if (hps_srcwait (num_usecs, cookie) < 0) take recovery action else
 *	    write src side hc_cmd area params // NOT the opcode.
 *	    hps_srchwop (opcode, cookie)// Use THIS to write the opcode. 
 *					// Lets underlying driver sync up
 *					// private bookkeeping.
 *	    mutex_spinunlock (bp->shc_slock, s)
 */
extern int hps_srcwait(int usecs, void *cookie);
extern int hps_dstwait(int usecs, void *cookie);
extern void hps_srchwop(int op,	  void * cookie);
extern void hps_dsthwop(int op,	  void * cookie);

/* =====================================================================
 * bypass prototypes called by hps driver.
 * These should be stubs for a non-bypass installation.
 */
extern int	hippibp_attach (vertex_hdl_t bpvert, 
				vertex_hdl_t *bpctl,
				void *cookie);
		/* called out of hps_attach
		 * bpvert is the "bypass" vertex that all bp dev nodes
		 *        are to be attached to.
		 * bpctl  is address where the newly-created bp ctl
		 *        device is to be returned to the hps driver
		 *	  so that it can be passed as argument in the
		 *	  following calls.
		 * cookie is a token that entitles holder to get special
		 * 	  access to "friend" info from the hps driver.
		 * Return value is 0 for SUCCESS, non-zero otherwise.
		 */
extern void	hippibp_bd_down (vertex_hdl_t bpdev);
		/* to be called by hps when board is configed down or reset */

extern void	hippibp_bd_up (vertex_hdl_t bpdev);
		/* to be called by hps when board is configed up */

extern void	hippibp_portint (vertex_hdl_t bpdev, short b2hs, int b2hl);


/* ioctl codes we don't want to advertise in hippi.h */
#define HIPPI_GET_SRCVERS	('h'<<8|65)
#define HIPPI_GET_DSTVERS	('h'<<8|66)
#define HIPPI_ERASE_FLASH	('h'<<8|67)
#define HIPPI_PGM_FLASH		('h'<<8|68)
#define HIPPI_GET_FLASH		('h'<<8|69)
#define HIPPI_GET_DRVRVERS	('h'<<8|70)
#define HIPPI_GET_MACADDR	('h'<<8|71)
#define HIPPI_SET_MACADDR	('h'<<8|72)
#define HIPPI_SET_LOOPBACK	('h'<<8|73)

/* coordinate bringup tries with hipcntl */
#define HPS_QUIET_INIT_TRIES	10	      /* fail > tries --> diag msg */
#define HPS_INIT_SLEEP_INTVL	(CLK_TCK/2)   /* .5s sleep this between tries */

typedef struct hip_flash_request {	/* parameter for HIPPI_PGM_FLASH */
	__uint32_t	offset;		/* offset in flash */
	__uint32_t	len;		/* length of data */
	__uint64_t	data;		/* pointer to data in user space */
	int16_t		clear_reset; 	/* clear reset after operation */
	int16_t		is_src;		/* src or dst flash? */
} hip_flash_req_t;


#endif /* __HPS_EXT_H */
