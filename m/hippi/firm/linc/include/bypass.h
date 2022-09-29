/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1997, Silicon Graphics, Inc                *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/
/*---------- Bypass Structures and defines -------------------------------- */
/*---------------------------------
  * Bypass Job Structures ....
  * d = destination (if first letter) 
  *   = descriptor  (if second letter)
  * f = free
  * m = map
  *  q = queue
  *  j = job, d = port or destination
  *  ex: dfm = destination freemap
  *  ex: sdq = source descriptor queue
  *  ex: bp_d_job = bypass port job index -- the job index associated with a 
  *              particular destination buffer index (port).
---------------------------------*/

#ident "$Revision: 1.12 $"

#ifndef HIPPI_BYPASS
#define HIPPI_BYPASS


/* All long msg receive pkts must align to NBPCL boundary */
#define NBPCL 128

#define		BP_VERSION		00

#define	    BP_MAX_JOBS         30      /*max number of jobs */
#define	    BP_MAX_PORTS     	BP_MAX_JOBS*128 /*max bypass file descriptors */
#define	    BP_MAX_SLOTS	32 /* max slots for incoming packets per job */


#define	    BP_HOSTX_ENTRIES  	128    /* max entries in ifield index map */
#define	    BP_SDQ_ENTRIES     	16384/sizeof(hippi_bp_desc)  /*16K descriptor qeueue */
#define	    BP_SFM_ENTRIES   	4096   /* source free map size (each entry is a pfn)*/
#define	    BP_DFM_ENTRIES   	4096   /* destination freemap number of entries */

typedef union {
  struct {
    uint	G      : 1;	/* get bit; 1 if doing a get (unimplemented) */
    uint	X      : 1;	/* "use physical addr" bit; (unimplemented) */
    uint	S      : 1;	/* single packet mode (vs. multi-pkt) */
    uint	F      : 1;	/* first subblock in a block if 1 */
    uint	I      : 1;	/* interrupt dest if 1 */
    uint	D      : 1;	/* descriptors sent up bit */
    uint	Z      : 2;	/* MBZ for current implementn. on Everest */
#define BP_PKT_VERS 0
    uint	vers	:2;
#define BP_PKT_VERSION_SHIFT 22
    uint	s_hostx	:6;
    uint	s_port	:16;
#define BP_PKT_S_PORT_MASK 	0x0000ffff
#define BP_PKT_S_HOSTX_MASK 	0x003f0000
#define BP_PKT_S_HOSTX_SHIFT 	16
#define BP_PKT_VERSION_MASK 	0x00c00000
#define BP_PKT_OPCODE_MASK 	0xff000000
    
    uint	get_job	:5;
    uint	slot	:5;
    uint	d_hostx	:6;
    uint	d_port	:16;
#define BP_PKT_D_PORT_MASK 	0x0000ffff
#define BP_PKT_D_HOSTX_MASK 	0x003f0000
#define BP_PKT_SLOT_MASK 	0x07800000
#define BP_PKT_GET_JOB_MASK 	0xf8000000
    
    uint	pad	:8;
    uint	seqnum	:24;
#define BP_PKT_SEQ_NUM_MASK	0x00ffffff
    
    uint	s_bufx;
    uint	s_off;
    uint	d_bufx;
    uint    	d_off;
    uint	auth[3];
    uint	chksum;
    uint	wpad;
  } d ;
  uint 	i[12];
}   bp_pkt_hdr_t;

typedef struct {
  int expseqnum;
  int maxseqnum;
} bp_seq_num_t;

/******** Bypass Descriptor OPCODES ***********/

#define	    HIP_BP_DQ_INV       ~(0x0)

typedef struct {
  u_int 	addr_hi;
  u_int		addr_lo;
} freemap_t;
	
typedef struct {
    volatile hippi_bp_desc 	*sdq_head;
    volatile hippi_bp_desc	*sdq_end;
    uint	auth[3];
    uint	fm_entry_size;
    uint	ack_hostport;
#define		BP_ACK_PORT_MASK       0xffff
#define		BP_ACK_HOST_SHIFT      16
#define		BP_ACK_HOST_MASK       0xff

} bp_job_state_t;


typedef struct { 
    uint		st;
#define BP_PORT_INVAL 		0x0 /* port not initialized */
#define	BP_PORT_VAL 		0x1 /* valid port */
#define	BP_PORT_PGX 		0x2 /* valid buffer assigned to port(shouldn't happen) */
#define BP_PORT_BLK_INT 	0x3 /* block the interrupt to the host */
    uint		job;

    uint		d_data_size;
    uint		data_bufx;

    uint		dq_base_lo;
    uint		dq_base_hi;
    uint		dq_size;
    uint		dq_tail;

    uint		int_cnt;
} bp_port_state_t;


/* defines for accessing Bypass Descriptor info */
#define BP_DESC_OPCODE_MASK 0xff000000

enum dma_status_t {
    DMA_ENABLE_SRC_DATA,
    DMA_ENABLE_DST_DATA,
    DMA_ENABLE_DST_DESC,
    DMA_OFF
};


/*
void store_bp_dma_status(enum dma_status_t type, 
		    int num_bufx, 
		    int base_bufx, 
		    int job,
		    src_host_t *shost);

int sched_bp_rd(volatile bp_job_state_t *jobp, int job, 
                src_host_t *shost);

int sched_bp_wr(__uint32_t *pkt, dst_wire_t *dwire);
*/
#endif /* HIPPI_BYPASS */




