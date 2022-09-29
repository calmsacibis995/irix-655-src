/**************************************************************************
 *									  *
 * 		 Copyright (C) 1994 Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
#ifndef __HIPPIBP_H
#define __HIPPIBP_H

/* Parameters for HIPIOC's for Hippi ByPass, used by MPI libraries.
 * These ioctl calls may only be issued on a Hippi ByPass device.
 */

/* Bypass Descriptor
 *
 * Structure to send or receive a message.
 * Written by user directly to firmware memory to source a packet onto
 * the hippi network. Specifies either a "normal" or "fixed" offset opcode,
 * the page index the data is on, the offset and length of the message 
 * on that page, and the destination buffer to be delivered to (hostx:port).
 *
 * A receive descriptor uses the same definition, but the hostx and port fields
 * are invalid.
 *
 * 16K_NEW allows the user to transport a user word within the destination
 * descriptor. It assumes a 16 KB length and zero offset, so the first word "fw"
 * at the source and destination becomes a header payload.
 *
 * All reads and writes to the firmware must be 32 bit, thus the "i" sub-field.
*/

typedef union {
        struct  {
        __uint32_t d_bufx : 16; /* 2^16 = 64K bufs => 1G @ 16K per page */
        __uint32_t d_off  : 16; /* 2^16 = 64K words => 256 Kbytes max */

        __uint32_t s_bufx : 16; /* 2^16 = 64K bufs => 1G @ 16K per page */
        __uint32_t s_off  : 16; /* 2^16 = 64K words => 256 Kbytes max */

        __uint32_t get_job : 5; /* used in G bit; pass thru on Everest */
        __uint32_t slot   : 5;  /* for multiple outstanding long msgs */
        __uint32_t hostx  : 6;  /* fill in dst_host at src, recv src host at dest */
        __uint32_t port   : 16; /* fill in dest port at src, recv src port at dest */

        __uint32_t G      : 1;  /* get bit; 1 if doing a get (unimplemented) */
        __uint32_t X      : 1;  /* "use physical addr" bit; (unimplemented) */
        __uint32_t S      : 1;  /* single packet mode (vs. multi-pkt) */
        __uint32_t F      : 1;  /* first subblock in a block if 1 */
        __uint32_t I      : 1;  /* interrupt dest if 1 */
        __uint32_t D 	  : 1;  /* descriptors sent up bit */
        __uint32_t Z      : 2;  /* MBZ for current implementn. on Everest */
        __uint32_t len    : 24; /* length for single pkt; blocknum for multi pkt */
} d;
    __uint32_t  i[4];
    __uint64_t  l[2];
} hippi_bp_desc;



/* setHostx
 *
 * Each job may setup its' own set of values into the host table for
 * that job.
 */
struct hip_set_hostx {		/* set ifields */
	__uint64_t	addr;		/* user address of ifield values */
	__uint32_t	nbr_ifields;	/* number of ifield values */
};

/* getHostx
 *
 * Any user may issue this to obtain an ifield array of 32-bit values from
 * a predefined address in controller memory.  If max_ifields is not large
 * enough to contain the entire list, then only max_ifields items will be
 * returned.
 */
struct hip_get_hostx {		/* get ifields */
	__uint64_t	addr;		/* user address for ifield values */
	__uint64_t	nbr_ifields_ptr;	/* returned (32 bit)count */
	__uint32_t	max_ifields;	/* max values to return */
};

/* setBPconfig
 *
 * May only be issued by root.  Used to setup which ULP will be used by
 * the Hippi Firmware for ByPass packets.
 *
 * getBPconfig
 * Used by any user to determine system's BP configuration
 */
struct hip_bp_config {
	u_char		ulp;		/* ulp reserved for ByPass */
	__uint32_t	max_jobs;	/* max number of jobs (per cntrler) */
	__uint32_t	max_portids;	/* max portid entries (per cntrler) */
	__uint32_t	max_dfm_pgs;	/* max dest freemap pgs (per job) */
	__uint32_t	max_sfm_pgs;	/* max src freemap pgs (per job) */
	__uint32_t	max_ddq_pgs;	/* max pgs in dest desc que (perjob) */
};

/* setupJob
 *
 * Initialize authentication number for job and reserve the max number of
 * port entries required for this job.
 */
struct hip_set_job {
	__uint32_t	auth[3];	/* authentication number */
	__uint32_t	flags	  : 16;	/* option flags */
#define HIPPIBP_JOB_ENABLE_MAILBOX 0x8000

	__uint32_t	max_ports : 16;	/* max port entries for job */
	__uint32_t	ddq_size;	/* size of dest desc que (pgs) */
};

/* enablePort
 *
 * Allocate a portid from those reserved for this job.  Driver will pass portid
 * along with jobid to the controller firmware.  Allocated portid returned to
 * the caller. The pgidx is converted into a physical address and
 * passed to the firmware (must be from the ???)
 */
struct hip_enable_port {
	__uint32_t	pgidx;		/* index in portid page list */
	__uint64_t	portid_ptr;	/* points to 32-bit return value */
};

/* enableIntr
 *
 * Enable interrupts on specified portid.  When one occurs, send the specified
 * signal to the process which invoked this ioctl.
 */
struct hip_enable_intr {
	__uint32_t	enable;		/* 0=disable, 1=enable */
	__uint32_t	portid;		/* portid */
	__uint32_t	signal_no;	/* signal number */
};

/* get_sdesq_head
 *
 * Return current head of source descriptor queue.
 */


/* enableJob
 *
 * Driver passes job number and authentication number to firmware to start
 * job.
 *
 */
struct hip_enable_job {
	__uint32_t	ack_host;	/* hostx for this host */
	__uint32_t	ack_port;	/* port to send ACK packets to */
};

/* getFWaddr
 *
 * Get firmware address (to be used by kernel processes only)
 * To be invoked by DVM services to
 * obtain various addresses within the Hippi controller.
 * ALLOWED TO "root" USER ONLY.
 * 
 * addrType
 *		1	sfreemap base
 *		2	dfreemap base
 *		3	sdesq base
 *		4	dfreelist base
 * Returns
 * 	0 (good status)
 *     -1 (error status)
 *
 *	FWaddr		uncached address in controller of structure base
 *	FWaddrSize	size in bytes of structure
 */
struct hip_get_fwaddr {
	__uint32_t	addrType;	/* selector for FW address type */
	__uint64_t	FWaddrPtr;	/* PTR to 64-bit returned addr */
	__uint64_t	FWaddrSize;	/* ptr to 64-bit returned size*/
};

/* setup_bpio
 * Setup specified range of user address for Hippi ByPass I/O
 * Driver performs I/O lockdown and virtual to physical address translation
 * for the firmware and places the pages into the specified freemap for
 * use by the user process.
 *
 * Returns a 32-bit handle to be used by the user for teardown when
 * I/O is complete.
 */

struct hip_io_setup {
	__uint64_t	uaddr;		/* user virtual address */
	__uint64_t	ulen;		/* length of user region (bytes)*/
	__uint32_t	uflags;		/* B_READ, B_WRITE flag bits */
#define	BPIO_SFM_SEL		0
#define BPIO_DFM_SEL    	1
#define BPIO_SFM_AND_DFM_SEL    2
	__uint32_t	mapselect;	/* as defined above */
	__uint32_t	start_index;	/* start index in map */
	__uint64_t	handle_ptr;	/* ptr to place for returned handle */
};

struct hip_io_teardown {
	__uint32_t	handle;
};

#define HIPPIBP_MAX_IOSETUP	64

/* Device offsets for mmap requests
 *
 * The model here is that each of the regions which user code (actually MPI
 * library) wishes to map is represented as a different "device offset"
 * into the Hippi BP device.  Some of these regions are only one page in
 * length, but we've spaced them equally so appropriate offset bits can be
 * viewed as a "region select".  Each region has more than enough room to
 * grow, since each is 128 MB, and there is sufficient room to add regions
 * and still stay within a 32-bit offset.
 */

#define HIPPIBP_SFREEMAP_OFF	0
#define HIPPIBP_DFREEMAP_OFF	0x08000000
#define HIPPIBP_SDESQ_OFF	0x10000000
#define HIPPIBP_DFLIST_OFF	0x18000000
#define HIPPIBP_PORTIDMAP_OFF	0x20000000
#define HIPPIBP_MBOX_OFF	0x28000000

/* Per device variables for HIPPI-BP (ByPass) interface
 * The following defines can't be modified without corresponding changes
 * in the Hippi Firmware.
 */
/* Factors affecting size Limitations:
 *     HIPPIBP_MAX_JOBS - in xio version of card, one mbox per job
 *                        and 32 mboxes on LINC.
 */
#define	HIPPIBP_MAX_JOBS	16
#define	HIPPIBP_MAX_HOSTX	128
#define HIPPIBP_MAX_PORTIDS	HIPPIBP_MAX_JOBS*128

/* special port number, when selected the driver allocates the port number */
#define	HIPPIBP_PRIVATE_PORTNUM	-1


/* Following defines are for support of multiple destination freelists and
 * sub-divides one page of the destination freelist with a fixed subdivision
 * which is agreed upon by both the MPI library intrinsics and the Hippi
 * firmware.
 */

#define	HIPPIBP_MAX_SLOTS		16
#define	HIPPIBP_SHORT_MSG_DFL_SIZE	8192
#define	HIPPIBP_DFL_SIZE		16384


/* Bypass protocol information, returned by HIPIOC_BP_PROT_INFO. The
 * first 32-bit value is guaranteed to be a "protocol info format",
 * which determines the format of the rest of the struct.
 */
typedef struct hippibp_prot_info {
	__uint32_t	format;		/* Format # of BP_PROT_INFO */
	__uint32_t	bp_major_vers;	/* Bypass protocol version major # */
	__uint32_t	bp_minor_vers;	/* Bypass protocol version minor # */
	__uint32_t	spare[13];	/* reserved for future expansion */
} hippibp_prot_info_t;

#define HIPPIBP_PROTINFO_FORMAT_CURR	0	/* Current protinfo format */

#endif /* __HIPPIBP_H */
