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



#ifndef __SYS_HIPPI_H
#define __SYS_HIPPI_H

#ident "$Revision: 1.21 $"


#define HIPPI_ULP_LE	4	/* HIPPI-LE ISO 8802.2 Link Encapsulation */
#define HIPPI_ULP_IPIM	6	/* IPI-3 Master */
#define HIPPI_ULP_IPIS	7	/* IPI-3 Slave */
#define HIPPI_ULP_MAX	255

#define HIPPI_ULP_PH	-1	/* HIPPI-PH (no ULP) */

#define HIPPI_MAX_D1SETSIZE	1016	/* bytes */
#define HIPPI_MAX_D1AREASIZE	2048	/* bytes */


typedef struct hippi_fp {
	u_char	hfp_ulp_id;
	u_char	hfp_flags;
#define HFP_FLAGS_P	0x80
#define HFP_FLAGS_B	0x40
	u_short	hfp_d1d2off;
#define HFP_D1SZ_MASK	0x07F8
#define HFP_D1SZ_SHFT	3
#define HFP_D2OFF_MASK	0x0007
	__uint32_t	hfp_d2size;
} hippi_fp_t;

typedef __uint32_t hippi_i_t;	/* I-field type */


/*********************************************************
 *                                                       *
 *   HIPPI-FP and HIPPI-PH character device interface    *
 *                                                       *
 *********************************************************/

#define HIPIOC_BIND_ULP		('H'<<8|1)	/* bind /dev/hippi clone
						 * to a ULP.
						 */

/* writes:  */

#define HIPIOCW_I 		('H'<<8|4)	/* set I-field */

#define HIPIOCW_D1_SIZE		('H'<<8|5)	/* FP: set D1 AREA size */


#define HIPIOCW_START_PKT 	('H'<<8|7)	/* For large packets across
						 * multiple writes.  specify
						 * length or infinity.
						 */
#define HIPPI_D2SIZE_INFINITY	0xFFFFFFFF


#define HIPIOCW_SHBURST		('H'<<8|8)	/* First burst is this number
						 * of bytes.  Sets B bit */

#define HIPIOCW_END_PKT		('H'<<8|9)	/* End multiple write
						 * packet early or if
						 * length was "infinity."
						 */

#define HIPIOCW_CONNECT		('H'<<8|10)	/* For sustained connections.
						 * specify I-field.
						 */

#define HIPIOCW_DISCONN		('H'<<8|11)	/* Disconnect sustained
						 * connection.
						 */

#define HIPIOCW_ERR		('H'<<8|12)	/* HIPPI error type on last
						 * write on this fd.  Returns
						 * one of the following:
						 */

#define HIP_SRCERR_NONE		0
#define HIP_SRCERR_SEQ		1		/* HIPPI source sequence err */
#define HIP_SRCERR_DSIC		2		/* Source lost DSIC */
#define HIP_SRCERR_TIMEO	3		/* Source timed out xmit */
#define HIP_SRCERR_CONNLS	4		/* Source lost CONNECT signal*/
#define HIP_SRCERR_REJ		5		/* Connect req was rejected*/
#define HIP_SRCERR_SHUT		6		/* Interface shut down */
#define HIP_SRCERR_PAR		7               /* Source parity error */



/* reads:   */

#define HIPIOCR_PKT_OFFSET 	('H'<<8|16)	/* Next read is this offset
						 * in packet.
						 */
#define HIPIOCR_ERRS		('H'<<8|17)	/* Return bit vector of errors
						 * occurring on receive of
						 * HIPPI packet.
						 */

#define HIP_DSTERR_PARITY	0x01	/* Destination parity error */
#define HIP_DSTERR_LLRC		0x00002	/* Destination LLRC error */
#define HIP_DSTERR_SEQ		0x00004	/* Destination sequence error */
#define HIP_DSTERR_SYNC		0x00008	/* Destination sync error */
#define HIP_DSTERR_ILBURST	0x00010	/* Destination illegal burst error */
#define HIP_DSTERR_SDIC		0x00020	/* Destination SDIC lost error */
#define HIP_DSTERR_FRAMING	0x00040	/* framing error during packet */
#define HIP_DSTERR_FLAG_ERR	0x00080	/* flag sync lost during packet */
#define HIP_DSTERR_LINKLOST_ERR	0x00100	/* lost linkready during pkt */
#define HIP_DSTERR_NO_PKT_RCV	0x00200	/* no pkt signal in connection */
#define HIP_DSTERR_READY       	0x00400 /* data received when no readys sent  */
#define HIP_DSTERR_NO_BURST_RCV	0x00800	/* no burst in packet */
#define HIP_DSTERR_STATE	0x01000	/* illegal hippi serial state transitions 
					 * see hippi_stats_t */

/* device control: */
#define HIPIOC_GET_STATS	('h'<<8|32)	/* get statistics
						 * (hippi_stats_t) */
#define HIPIOC_ACCEPT_FLAG	('h'<<8|33)	/* for setting/resetting
						 * HIPPI accept flag */

#define HIPIOC_STIMEO		('h'<<8|34)	/* set source timeout (ms) */

#define	HIPPI_SETONOFF  	('h'<<8|64)	/* bring up or reset card */

/* Hippi Bypass ioctls - used by MPI libraries */
#define	HIPIOC_SET_HOSTX	('H'<<8|128)	/* set Ifields into FW (root)*/
#define	HIPIOC_GET_HOSTX	('H'<<8|129)	/* get Ifields from FW */
#define	HIPIOC_SET_JOB		('H'<<8|130)	/* setup job */
#define	HIPIOC_ENABLE_PORT	('H'<<8|131)	/* enable port */
#define	HIPIOC_ENABLE_JOB	('H'<<8|132)	/* enable BP job */
#define	HIPIOC_SET_BPCFG	('H'<<8|133)	/* set BP configuration (root)*/
#define	HIPIOC_GET_SDESQHEAD	('H'<<8|134)	/* get src desc queue head */
#define	HIPIOC_GET_BPCFG	('H'<<8|135)	/* get BP configuration */
#define	HIPIOC_ENABLE_INTR	('H'<<8|136)	/* enable interrupt on port */
#define	HIPIOC_GET_FWADDR	('H'<<8|137)	/* get Firmware Addresses (root) */
#define	HIPIOC_GET_BPSTATS	('H'<<8|138)	/* get ByPass stats (root) */
#define	HIPIOC_SETUP_BPIO	('H'<<8|139)	/* setup pages for BP I/O  */
#define	HIPIOC_TEARDOWN_BPIO	('H'<<8|140)	/* return BP I/O page group */
#define HIPIOC_BP_PROT_INFO	('H'<<8|141)	/* return BP protocol info */


typedef struct hippi_stats {
	u_int	hst_flags;              /* status flags */

#define HST_FORMAT_ID_SHIFT	28	/* start of 4 bit format ID field */
#define HST_FORMAT_ID_MASK	0xf0000000
#define HST_HIO			(0<<HST_FORMAT_ID_SHIFT)
#define HST_XIO			(1<<HST_FORMAT_ID_SHIFT)

  /* Used only by HIO hippi systems */
#define HST_FLAG_DSIC 		0x0001	/* SRC sees IC */
#define HST_FLAG_SDIC 		0x0002	/* DST sees IC */

  /* Used by  XIO serial hippi systems */
#define HST_FLAG_LOOPBACK	0x0004  /* internal loopback enabled */

  /* Used by XIO and HIO hippi systems */
#define HST_FLAG_DST_ACCEPT	0x0010	/* DST is accepting connections */
#define HST_FLAG_DST_PKTIN	0x0020	/* DST: PACKET input is high */
#define HST_FLAG_DST_REQIN	0x0040	/* DST: REQUEST input is high */

#define HST_FLAG_SRC_REQOUT	0x0100	/* SRC: REQUEST is asserted */
#define HST_FLAG_SRC_CONIN	0x0200	/* SRC: CONNECT input is high */

/* flags for XIO hippi serial (Origin Platforms) */
#define HST_FLAG_DST_LNK_RDY	0x00010000
#define HST_FLAG_DST_FLAG_SYNC	0x00020000 /* alternating flag is synched */
#define HST_FLAG_DST_OH8_SYNC	0x00040000
#define HST_FLAG_DST_SIG_DET	0x00080000 /* signal detect on destination */


	/* Source statistics */
	u_int	hst_s_conns; 		/* connections attempted */
	u_int	hst_s_packets;		/* total packets sent */

	/* Error statistics are different for Hippi Parallel (HIO) vs. Hippi Serial (XIO)
	 * In an effort to keep some binary compatibility, some fields 
	 * defined originally for Challenge platform hippi (HIO hippi)
	 * are re-used for Origin platform hippi serial (XIO hippi).
	 */
	union {
	    u_int	data[14];
	    struct {
		u_int	rejects;	/* connection attempts rejected */
		u_int	dm_seqerrs;	/* data sm sequence error */
		u_int	cd_seqerrs;	/* conn sm sequence error, dst */
		u_int	cs_seqerrs;	/* conn sm sequence error, src */
		u_int	dsic_lost;	
		u_int	timeo;		/* timed out connection attempts */
		u_int	connls;		/* connections dropped by other side */
		u_int	par_err;	/* source parity error */
		u_int	resvd[6];	/* reserved for future compatibility */
	    } hip_p;			/* HIO hippi board */
	    struct {			
		u_int	rejects;	/* connection attempts rejected */
		u_int	xmit_retry; 	/* successfully re-transmitted a packet */
		u_int	resvd0;		/* reserved for future compatibility */
		u_int	glink_resets;   /* number of times firmware reset glink */
		u_int	glink_err;	/* glink error count */
		u_int	timeo;		/* connection attempts timed out */
		u_int	connls;		/* connections dropped by other side */
		u_int	par_err;	/* source parity error */
		u_int	resvd1[4];	/* reserved for future compatibility */
		u_int	numbytes_hi;	/* number bytes sent */
		u_int	numbytes_lo;	/* number bytes sent */
	    } hip_s;			/* XIO hippi serial board */
					
					
	} sf;			        /* source format, system specific */
					
					/* Destination statistics */
	u_int	hst_d_conns;		/* total connections accepted */
	u_int	hst_d_packets;		/* total packets received */

	union {				
	    u_int	data[14];
	    struct {			
		u_int	badulps;	/* packets dropped due to unknown ULP */
		u_int	ledrop;		/* HIPPI-LE packets dropped */
		u_int	llrc;		/* conns dropped due to llrc error */
		u_int	par_err;	/* conns dropped due to parity error */
		u_int	seq_err;	/* conns dropped due to sequence err */
		u_int	sync;		/* sync errors */
		u_int	illbrst;	/* packets with illegal burst sizes */
		u_int	sdic_lost;	/* conns dropped due to sdic lost */
		u_int	nullconn;	/* connections with zero packets, 
					 * i.e. CONNECT, no PKT, deassert REQUEST */
		u_int	resvd[5];	/* reserved for future compatibility */
	    } hip_p;			
	    struct {			
		u_int	badulps;	/* packets dropped due to unknown ULP */
		u_int	ledrop;		/* HIPPI-LE packets dropped */
		u_int	llrc;		/* conns dropped due to llrc error */
		u_int	par_err;	/* conns dropped due to parity error */
		u_int	frame_state_err;/* framing error or state transition error
					 * state transition errors are:
					 * no request -> packet 
					 * no request -> burst 
					 * request -> burst	
					 * burst -> request	
					 * burst -> no request 
					 */
		u_int	flag_err;	/* flag sync lost during packet */
		u_int	illbrst;	/* packets with illegal burst sizes */
		u_int	pkt_lnklost_err; /* lost linkready during pkt */
		u_int	nullconn;	/* connections with zero packets */
		u_int	rdy_err;	/* data received when no readys sent */
		u_int	bad_pkt_st_err;	/* Packet got off to a bad start
					 * i.e.
					 * null packets - PKT, no BURST, deassert PKT
					 * insufficient start for FP
					 *    (BURST, less than 12 bytes data, deassert PKT)
					 */
		u_int	resvd;	
		u_int	numbytes_hi;	/* number bytes received */
		u_int	numbytes_lo;	/* number bytes received */
	    } hip_s;			
	} df;			/* destination format, system specific */

} hippi_stats_t;


/* Status structure returned with HIPIOC_GET_BPSTATS */

typedef struct hippibp_stats {
	/* BYPASS STATE */
	u_int   hst_bp_job_vec;		/* bypass job enable vector */
					/* job 0 is in bit 31, job 7 bit 24 */

	u_int   hst_bp_ulp;	        /* ulp used by bypass */

	/* SOURCE STATISTICS */
	u_int	hst_s_bp_descs;	        /* total bypass desc processed */
	u_int   hst_s_bp_packets;       /* total bypass packets sent */
	__uint64_t hst_s_bp_byte_count;	/* total bypass bytes sent */


	/* SOURCE ERRORS */
	/* descriptor errors */
	u_int   hst_s_bp_desc_hostx_err; /* hostx is out of bounds*/
	u_int	hst_s_bp_desc_bufx_err;	/* buffer index out of bounds */
	u_int	hst_s_bp_desc_opcode_err; /* invalid opcode */
	u_int	hst_s_bp_desc_addr_err;	/* packet length + offset
					   would cross a page boundary  */

	u_int	hst_s_bp_resvd[6];	/* future expansion */



	/* DESTINATION STATISTICS */
	u_int	hst_d_bp_descs;		/* total dest desc processed */
	u_int   hst_d_bp_packets;	/* total bypass packets received */
	__uint64_t hst_d_bp_byte_count;	/* total bypass bytes received */


	/* DESTINATION ERRORS */
	/* descriptor errors */	
        u_int	hst_d_bp_port_err;	/* port not enabled 
					   or port too large */
        u_int	hst_d_bp_job_err;  	/* job not enabled */
	u_int	hst_d_bp_no_pgs_err;  	/* no longer used */
        u_int	hst_d_bp_bufx_err;  	/* destination bufx not in bounds */
        u_int	hst_d_bp_auth_err;  	/* received authentication did
					   not match job authentication
					   for destination port */
        u_int	hst_d_bp_off_err;  	/* offset plus packet length
					   would cross a page boundary 
					   or not aligned properly*/
        u_int	hst_d_bp_opcode_err;  	/* received opcode was invalid */
        u_int	hst_d_bp_vers_err;  	/* received version number invalid */
        u_int	hst_d_bp_seq_err;  	/* sequence number for multi-pkt 
					   opcode was out of sequence */

        u_int	hst_d_bp_resvd[3];

} hippibp_stats_t;

#endif /* __SYS_HIPPI_H */



