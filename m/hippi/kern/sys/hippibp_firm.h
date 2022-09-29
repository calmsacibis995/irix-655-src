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
 * hippibp_firm.h
 *
 *	Header file for interface between HIPPI Bypass driver
 *	and the 4640 firmware.
 */

#ifndef __HIPPIBP_FIRM_H
#define __HIPPIBP_FIRM_H
#ident	"$Revision: 1.7 $    $Date: 1997/05/14 07:59:43 $"

/************************************************************
 *							    *
 *        HIPPI Board/Host Interface description            *
 *							    *
 ************************************************************/


/* 
 * Defines for arguments to BP commands issued through the hip_hc
 * cmd area. Args MUST fit within 16 (32-bit) words.
 */
typedef struct {
	__uint32_t  enable;	        /* enable/disable job  */
	__uint32_t  job;		/* job slot number */
	__uint32_t  fm_entry_size;	/* size of freemap entries (bytes) */
	__uint32_t  auth[3];	/* authentication number for job */
	__uint32_t  ack_host;	/* host index for ACK field in pkt */
	__uint32_t  ack_port;	/* port that should receive ACKs */
} bp_job_t;

typedef	struct {
	union {
		__uint32_t  i;
		struct {
		    __uint32_t opcode      :  4;
#define   HIP_BP_PORT_DISABLE       0
#define   HIP_BP_PORT_NOPGX         1
#define   HIP_BP_PORT_PGX           2
		    __uint32_t unused      : 12;
		    __uint32_t init_pgx    : 16;
		}  s;
	} ux;
	__uint32_t  job;      /* job this port is attached to */
	__uint32_t  port;     /* bypass destination buffer index */
	__uint32_t  ddq_hi;   /* physical address of base of dest desc queue */
	__uint32_t  ddq_lo;
	__uint32_t  ddq_size; /* size of a dest desc queue (bytes)*/
} bp_port_t;

typedef	struct {
	__uint32_t  ulp;	        /* enable/disable job  */
} bp_conf_t;

typedef struct {
	__uint32_t	portid;		/* portid interrupt to ack */
	__uint32_t	cookie;		/* interrupt cookie from FW */
} bp_portint_ack_t;

 /* information from the Hippi Firmware.  This structure is located at a 
 * well-known address in the firware.
 */

typedef struct hip_bp_fw_config {
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

	__uint32_t	mailbox_base;   /* base of mailboxes */
	__uint32_t	mailbox_size;   /* number of mailboxes supported */
        __uint32_t	reserved[13];	/* fill to 32 words */
} hip_bp_fw_config_t;


/* 
 * This struct hip_bp_hc is the same thing as struct hip_hc used
 * by the underlying hardware driver. 
 * 
 * It is redefined here for flexibility in making changes to the 
 * cmd arg union, and the response union. Additions may be freely
 * made to the respective unions as long as the sizes fit within
 * the place holders (stretchers?)
 *  	__uint32_t	cmd_data[16];
 * and
 *  	__uint32_t	cmd_res[16];
 *
 * !! DO NOT CHANGE ANY OF THE OTHER FIELDS!!
 */ 
/******************************************************************
 * WARNING: do not change hip_bp_hc without reading above comment.
 ******************************************************************/
typedef struct hip_bp_hc {

    /* host-written/board-read portion of the communications area
     */
    __uint32_t	cmd;
/* Opcodes for the hip_hc cmd.
 * Range 0-19 reserved for hps_
 *       20-39 reserved for hippibp
 */
#define	HCMD_BP_JOB		20	/* define a bypass job slot */
#define	HCMD_BP_PORT		21	/* define a bypass process slot */
#define	HCMD_BP_CONF		22      /* configure the bypass */
#define	HCMD_BP_PORTINT_ACK     23      /* ack a port interrupt */
#define	HCMD_BP_STATUS     	24      /* update bypass stats */
#define HCMD_BP_SDQHEAD		25	/* get bypass source descriptor
					   queue head ptr */

    __uint32_t	cmd_id;                 /* ID used by board to ack cmd */
    union {
	__uint32_t	cmd_data[20];	/* minimize future version problems */
	bp_job_t	    bp_job;
	bp_port_t	    bp_port;
	bp_conf_t	    bp_conf;
	bp_portint_ack_t    bp_portint_ack;
	__uint32_t	    sdqhead_jobnum; /* used by HCMD_SDQHEAD */
    } arg;

    /* host-read/board-written part of the communications area
     */
    volatile __uint32_t sign;		/* board signature */
    volatile __uint32_t pad1;
    volatile __uint32_t vers;		/* board/eprom version */
    volatile __uint32_t pad2;
    volatile __uint32_t inum;		/* ++ on board-to-host interrupt */
    volatile __uint32_t pad3;
    volatile __uint32_t cmd_ack;	/* ID of last completed command */
    volatile __uint32_t pad4;
    volatile union {
	__uint32_t    cmd_res[16];	/* minimize future version problems */
	__uint32_t    sdqhead;	        /* bypass src descriptor queue head */
    } res;
} hip_bp_hc_t;

#endif /* __HIPPIBP_FIRM_H */
