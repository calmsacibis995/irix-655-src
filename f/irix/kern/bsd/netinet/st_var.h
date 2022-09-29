/*
 *               Copyright (C) 1997 Silicon Graphics, Inc.                
 *                                                                        
 *  These coded instructions, statements, and computer programs  contain  
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  
 *  are protected by Federal copyright law.  They  may  not be disclosed  
 *  to  third  parties  or copied or duplicated in any form, in whole or  
 *  in part, without the prior written consent of Silicon Graphics, Inc.  
 *                                                                        
 *
 *  Filename: st_var.h
 *  Description: PCB and state variavles required for core ST functionality.
 */

#ifndef __ST_VAR_H__
#define __ST_VAR_H__


#include "sys/errno.h"
#include "sys/types.h"
#include "sys/sema.h"
#include "sys/uio.h"
#include "net/if.h"
#include "net/route.h"
#include "netinet/st_if.h"
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/in_pcb.h>

/* File Placement:
 *
 * st.h          Defines Scheduled Transfers Header
 * st_var.h      This file; defines code conventions, file locations,
 *               protocol control blocks
 * st_debug.h    Defines debugging routines
 *
 *
 * st_core.c     No longer exists -> See st_fsm.c
 * st_debug.c    Debugging routines:  std_*                   [Arch Dependent]
 * st_fsm.c      Contains "core" protocol routines:  stc_*    [Arch Independ.]
 * st_input.c    Input/Interrupt handler                      [Arch Dependent]
 * st_subr.c     Contains archecture dependent routines       [Arch Dependent]
 *               to support protocol initialization, memory
 *               allocation, etc.
 * st_fsm_arch.c Contains architecture dependent routines     [Arch Dependent]
 *               to support st_fsm.c:  stvc_*
 * st_usrreq.c   Describes user/socket interaction            [Arch Dependent]
 *               On other OS -- could be written as st_ulp.c
 *               or st_winsock.c, etc.
 *
 */



/* 
 * ST-Core Code conventions:
 *
 * All _KERNEL-specific routines should take socket *, as input.
 * This gives full access to all data structures, including
 * inpcb.  (This may change as time progresses, but this
 * is gives 'full _KERNEL access' during our initial 
 * implementation.)
 *
 * All generic / ST-core routines should only deal with stpcb.
 * Moreover, generic routines should avoid addressing, inpcb,
 * ifnet, if at all possible. 
 *
 * Oh yah, we need to change this.
 * ST-core routines should NEVER EVER reference mbufs.
 * Doh.
 *
 */


/* _KERNEL-Specific Macros, Data Elements, Function Defs */
#ifdef _KERNEL
extern zone_t *stpcb_zone;
extern struct inpcb stpcb_head;
extern	int	stp_mbuf_copy;

extern int st_output(struct mbuf *, struct socket *);

extern int st_findroute(struct socket *);

#endif	/* _KERNEL */




/* Rather than send down a fullblown IP header, we send down
 * the essentials required to do an ARP.  This of course will
 * change when we decide whether or not our protocol stack
 * is IP-based or MAC-based.  In the event of MAC-based, this
 * implies we need a method of determing the IP address (RARP)
 * to pass to the user during an accept().  This also implies
 * we have some reasonable knowledge of determining what IP
 * address we should use in the event of IP aliasing.
 *
 * In other words, I vote we have an ST/IP based stack ->
 * this also allows for future IP operation. 
 * 
 * In anycase this dinky structure is for STP/IF communication.
 */


typedef int 	st_tid_t;
typedef u_char 	st_state_t;

struct stpcb;

#define	STPCB_DEBUG


/* Virtual Connection Descriptor */
typedef struct st_vcd_s {

	u_short      vc_flags;
#define STP_VCD_FLAG_OOO 0x01                 /* Out Of Order capable */
#define STP_VCD_OUT_VC_ROT 0x02               /* o/p VCNUM set */
       
	u_int32_t    vc_lport;
	u_int32_t    vc_lkey;
	u_int16_t    vc_max_lslots;
	u_int32_t    vc_lbufsize;
	u_int32_t    vc_lmaxstu;
	u_int32_t    vc_lmaxblock;

	u_int32_t    vc_rport;
	u_int32_t    vc_rkey;
	u_int16_t    vc_max_rslots;
	u_int32_t    vc_rbufsize;
	u_int32_t    vc_rmaxstu;
	u_int32_t    vc_rmaxblock;

	u_int32_t    vc_ethertype;

	u_int16_t    vc_rslots;		/* remote slots */
	u_int16_t    vc_lslots;		/* local slots, shac supports far more than this */
	u_int16_t    vc_vslots;		/* visible slots, value sent during connection setup */
	/* jgregor hack - clean this up after 0.5 */
	u_int16_t    vc_true_max_lslots;/* real, honest-to-god size of the local DDQ! */
	u_int16_t    vc_true_max_rslots;/* real, honest-to-god size of the remote DDQ! */
	u_int16_t    vc_lsync;		/* local sync num */
	u_int8_t     vc_out_vcnum;	/* VC num to use for data */

	u_int8_t     vc_credits[4];	/* credits on each VC */
	sv_t         slot_sync;		/* for slots on VC */
	uchar_t      asked_for_slots;	/* sent out for slots? */
} st_vcd_t;



typedef	struct st_buf {
	u_char	     bufx_flags;
#	define	BUF_NONE	0x0
#	define	BUF_ADDR	0x01
#	define	BUF_BUFX	0x02
#	define	BUF_MAPPED	0x04


	uint		bufx_cookie;	
	uint		payload_len;	
	uint		num_bufx;	
	caddr_t		test_bufaddr;	
	struct mbuf	*temp_mbuf;	
	uio_t 		*uio;	
	union	{
		caddr_t		bufaddr;
		struct	{
			u_int32_t	bufx;
			u_int32_t	offset;
		} bufx_t;
	} st_addr_bufx;
} st_buf_t;


#define	BNUM_TAB_SIZE	1024

typedef struct st_tx_s {
	st_state_t   tx_state;
			
	u_int32_t    tx_flags;
#define	TX_RTS_SENDSTATE	0x01

	u_int32_t    tx_len_of_send;
	u_int32_t    tx_tlen;		
	st_buf_t     tx_buf;		
	uio_t        *tx_uio;		
	u_int16_t    tx_local_bufsize;	/* log units */
	u_int16_t    tx_remote_bufsize;	/* log units */
	u_int16_t    tx_blocksize;	/* for asynch xfer */
	u_int16_t    tx_ctsreq;		
	u_int32_t    tx_foffset;
	u_int8_t     tx_spray_width;
	
	u_int8_t     tx_ACKed;
	u_int32_t    tx_acked_Bnum;
	u_int32_t    tx_max_Bnum_sent;

	u_int32_t    tx_iid;		/* I_Id of xfer */
	u_int32_t    tx_rid;		/* R_Id of peer */
#ifdef	STPCB_DEBUG
	char	     data_CTS_tab[BNUM_TAB_SIZE]; /* received CTS */
	u_int32_t    tx_max_Bnum_required;
	u_int32_t    tx_num_cts_seen;
#endif	/* STPCB_DEBUG */
} st_tx_t;

/* maximum number of Mx's per connection? */
#define	 ST_RX_MX_ENTRIES		16

typedef struct st_rx_s {
  	st_state_t   rx_state;

	u_int32_t    rx_flags;
#define	RX_RTS_SENDSTATE	0x01

	u_int32_t    rx_tlen;		/* tlen from RTS */
	u_int32_t    rx_cts_len;	/* length that hasn't had CTS's sent */
	u_int32_t    rx_data_len;       /* length of data left to receive */
	st_buf_t     rx_buf;
	u_int16_t    rx_local_bufsize;	/* log units */
	u_int16_t    rx_remote_bufsize;	/* log units */
	u_int32_t    rx_blocksize;	/* for asynch xfer */
	u_int8_t     rx_spray_width;

  	u_int32_t    rx_iid;		/* I_Id of peer */
  	u_int32_t    rx_rid;		/* R_Id of xfer */

  	uint32_t     rx_first_offset;	/* first CTS's offset */

	st_mx_t	     rx_mx_template;    /* template for mx */
	u_int32_t    rx_num_mx;		/* number of valid mx entries */
  	u_int16_t    rx_mx[ST_RX_MX_ENTRIES]; /* R_Mx of xfer */
	u_int16_t    rx_mx_tmp;

	u_int32_t    cur_bnum;		/* b_num for next CTS */
	u_int32_t    last_B_num;	/* last data b_num recvd */
	struct mbuf  *hdr_for_RSR;
	char	     bnum_tab[BNUM_TAB_SIZE]; /* received bnums */
#ifdef	STPCB_DEBUG
	char	     CTS_tab[BNUM_TAB_SIZE]; /* sent CTSs */
#endif	/* STPCB_DEBUG */
} st_rx_t;


typedef struct st_kx_s {
  	sthdr_rts_t saved_rts;
  	st_state_t   kx_state;		/* just for debug */
} st_kx_t;


#define	TX	4
#define	RX	2

#define	MAX_TX_ENTRIES	1
#define	MAX_RX_ENTRIES	1
#define	MAX_KX_ENTRIES	1


#define	NUM_VC_TIMERS		1
#define	NUM_TX_TIMERS		MAX_TX_ENTRIES
#define	NUM_RX_TIMERS		MAX_RX_ENTRIES
#define	NUM_SLOT_TIMERS		1
#define STPT_NTIMERS    	NUM_VC_TIMERS + NUM_TX_TIMERS \
				+ NUM_RX_TIMERS + NUM_SLOT_TIMERS

#define STPT_VC_TIMER    	0
#define STPT_SLOT_TIMER    	NUM_VC_TIMERS + NUM_TX_TIMERS \
				+ NUM_RX_TIMERS

/* timeout vales */
#define	VC_TIMEOUT_VAL		50
#define	TX_TIMEOUT_VAL		40
#define	RX_TIMEOUT_VAL		40
#define	SLOTS_TIMEOUT_VAL	40

/* how deep do you want to pipeline today? */
#define  ST_NUM_CTS_OUTSTANDING		64


/* ST Protocol Control Block */
struct stpcb {

	st_state_t           s_vc_state;  /* state of this connection */
	st_vcd_t             s_vcd;       /* virtual connection desc. */
	u_short              s_flags;
#define STP_SF_BYPASS		0x0001
#define STP_SF_USERKEY		0x0002
#define STP_SF_USERSLOTS	0x0004
#define STP_SF_USERVISSLOTS	0x0008

	u_short              blocksize;

	st_tx_t		tx[MAX_TX_ENTRIES];
	st_rx_t		rx[MAX_RX_ENTRIES];
	st_kx_t		kx[MAX_KX_ENTRIES];
	
	uint32_t	num_kid_allocated;

	short           s_timer[STPT_NTIMERS]; 


	/* We only have one timeout and one maxretry field. */

	u_int32_t            s_optimeout;  /* one timer for VCD */
	u_int32_t            s_numretries;

	/* ST-Core Modules should NEVER reference s_inpcb */
	struct socket       *s_so;        /* back pointer to socket */
	struct inpcb        *s_inp;       /* back pointer to internet pcb */
	struct ifnet        *s_ifp;       /* ifnet interface */
	struct sockaddr      s_dst;       /* dst address     */
        struct st_ifnet_s   *s_stifp;     /* st ifnet structure */

	/* 
	 * There will be common routines in the case of non-supporting 
	 * drivers, but in the instance of SuperHippi or other special 
	 * devices, there will be device-dependent func's. 
	 */
	
#	define	INVALID_PORT	-1
	int (*s_portalloc)();
	int (*s_portfree)();

#	define	INVALID_BUFX	-1
#	define	BAD_OFFSET	-1
	int (*s_bufalloc)();
	int (*s_buffree)();

#	define	INVALID_TID		-1	/* not a valid TID */
#	define	FREE_TID		0	/* unused tid-space */
#	define	INVALID_R_Mx		-1	

	st_tid_t (*s_iidalloc)(struct stpcb *);	
	int 	(*s_iidfree)(struct stpcb *, st_tid_t);	

	st_tid_t (*s_ridalloc)(struct stpcb *);	
	int 	(*s_ridfree)(struct stpcb *, st_tid_t);		
	
	st_tid_t (*s_kidalloc)(struct stpcb *);
	int 	(*s_kidfree)(struct stpcb *, st_tid_t, sthdr_t *);
	int 	(*s_keyalloc)(struct stpcb *, ushort);
	u_int16_t 	(*s_R_Mx_alloc)(struct stpcb *);
	void 	(*s_R_Mx_free)(struct stpcb *, u_int16_t);

#ifdef	STPCB_DEBUG
	/* DEBUG structures; these are for internal kern-ST stack
	** debugging only, and can be removed from MR code */
	u_int16_t	last_rx_mx_setup;
	u_int16_t	last_rx_mx_torndown;
	u_int32_t	last_RSR_R_id_sent;
	u_int32_t	last_RSR_I_id_sent;
	u_int32_t	last_RSR_R_id_recvd;
	u_int32_t	last_RSR_I_id_recvd;
#endif	/* STPCB_DEBUG */
	
};


/* Local operations which affect the DATA or VC FSMs; these are not
 * in st.h because they aren't a part of hte ST specs.
 * however, we shall keep the opcode space for these distinct from the 
 * the ST opcodes
*/
#define	WRITE_SYSCALL		0xa0
#define	READ_SYSCALL		0xa1
#define	TX_TIMEOUT		0xa2
#define	RX_TIMEOUT		0xa3


#define	ST_STATE_SLOTS		1	/* num slots left? */
#define	ST_STATE_XFER		2	/* xfer complete */
#define	ST_STATE_BLOCK		3	/* particular block reach ok? */


/* the invalid state in both the VC and DATA FSMs */
#define	STP_INVALID_STATE	-1

/* Virtual Connection States per the ST Specification 
 *
 * NOTE: Disregard 'virtual' -- see ST specification.  VC does
 *       not imply ATM VC's or the VC's on HIPPI-6400 link-level.
 *
 */
#define STP_VCS_CLOSED        0
#define STP_VCS_LISTEN        1
#define STP_VCS_RCSENT        2
#define STP_VCS_CONNECTED     3
#define STP_VCS_RDSENT        4
#define STP_VCS_DASENT        5
#define STP_VCS_DISCONNECTED  STP_VCS_CLOSED


/* Initiator's states */
/* ready to send an RTS */
#define STP_READY_FOR_RTS	STP_VCS_CONNECTED
/* sent an RTS, pining the pages now */ 
#define STP_SEND_RTS_PINNING	100
/* sent an RTS, page-pinning is complete */ 
#define STP_RTS_PINNED		110
/* page-pinning is not complete yet, but got one or more CTS */ 
#define STP_CTS_PINNING		120
/* page-pinning done, got CTS, now shovelling DATA mesgs */
#define STP_DATA_SEND		130
/* end the xfer: either naturally, or something went wrong */
#define	STP_INIT_END_TRANSFER 	140
/* wait for ACK that of "end-xfer" from responder */
#define	STP_WAIT_ACK		150

/* stop-gap: until RA is introduced into the state-diags */
#define	STP_RA_RECEIVED		160




/* Responder's states */
/* ready to receive an RTS */
#define STP_READY_FOR_RTS	STP_VCS_CONNECTED
/* received an RTS, but recv-buffer not posted yet */
#define STP_RTS_RECEIVED	200
/* pinning the posted buffer, and not received RTS yet */
#define STP_BUFF_PINNING	210
/* done with pinning the posted recv. buffer; RTS not recvd yet */
#define STP_BUFF_RECEIVED	220
/* got an RTS, and recv-buffer; but pinning recv buffer not 
 * complete yet */
#define	STP_RECV_RTS_PINNING	230
/* bufs posted and pinned; have sent out one or more CTS, and
 * ready to receive DATA */
#define STP_DATA_RECV		240
/* end the xfer: either naturally, or something went wrong */
#define	STP_RESP_END_TRANSFER	250

/* stop-gap: until RA is introduced into the state-diags */
#define	STP_RA_SENT		260




/* 
 * Virtual Connection State Machine Opcodes 
 *
 * The VC opcode space overlaps with the
 * ST opcode space.
 *
 * 0x01 thru 0x1F are reserved for ST.
 * 
 */
#define ST_VOP_ULISTEN       (0x06 << ST_OPCODE_SHIFT)
#define ST_VOP_UUNLISTEN     (0x07 << ST_OPCODE_SHIFT)
#define ST_VOP_UCONNECT      (0x08 << ST_OPCODE_SHIFT)
#define ST_VOP_UDISCONNECT   (0x09 << ST_OPCODE_SHIFT)


#define	INVAL_KEY	0


/* temporatry for GSN-ST bringup */
/* blocksz should be 24: 16M is the largest pgsz in IRIX,
** and SHAC doesn't allow (in theory) sending more than 16M */ 
#define	BLOCK_SIZE			24
#define	ST_MAX_STU_SZ			BLOCK_SIZE
#define	ST_LOG_BUFSZ			14
#define	VC0_FIFO_CREDITS		255
#define	VC1_FIFO_CREDITS		255
#define	VC2_FIFO_CREDITS		255
#define	VC3_FIFO_CREDITS		255
#define VC3_UPPER_LIMIT			(128 << 10)
#define DEFAULT_DATA_VC_NUM		3
#define	ST_DDQ_NUM_SLOTS 		255
#define	ST_DEFAULT_NUM_SLOTS 		255
/* #define	ST_MIN_ALLOWED_SLOTS 	ST_NUM_CTS_OUTSTANDING */
#define	ST_MIN_ALLOWED_SLOTS 		5


/* functions declarations -- as used by many of the ST modules */

#ifdef _KERNEL

/* Todo:
 *   stvc_attach  takes socket *     -> should be stpcb *
 *   stvc_attach  takes socket *     -> should be stpcb *
 *   stvc_bind    takes mbuf *       -> should be void  *
 *   stvc_connect takes mbuf *       -> should be void  *
 *   stvc_newconn takes inaddrpair * -> should be void  *
 */

extern int	log2(uint);
extern int	st_usrreq(struct socket *, int, struct mbuf *,
				struct mbuf *, struct mbuf *);
extern int      stvc_attach(struct socket *);
extern int      stvc_detach(struct socket *);

extern int      stvc_bind(struct stpcb *, struct mbuf *);
extern int      stvc_listen(struct stpcb *sp);
extern int      stvc_connect(struct stpcb *sp, struct mbuf *);
extern void     stvc_isdisconnected(struct stpcb *);
extern void     stvc_isdisconnecting(struct stpcb *);
extern void     stvc_isconnected(struct stpcb *);
extern void     stvc_isconnecting(struct stpcb *);

extern struct stpcb *stvc_newconn(struct stpcb *, struct inaddrpair *);
extern void 	stpcb_init(struct stpcb *sp);

extern int      stvc_input(struct stpcb *, ushort, struct mbuf *);
extern int      stvc_output(struct stpcb *, ushort, short);
extern int      st_data_ctl_output(struct stpcb *, ushort, short);

extern int	st_start_write(struct stpcb *, int);
extern int      stdata_input(struct stpcb *, ushort, struct mbuf *);
extern int	st_send_data(struct stpcb *, sthdr_t *, int); 
extern int	stc_hdr_template(struct stpcb *, ushort, sthdr_t *,
								short);
extern int	st_rs_template(struct stpcb *, ushort, sthdr_t *, 
							      uchar_t);
extern void	st_data_init_tx_state(st_tx_t *, st_state_t);
extern void	st_data_init_rx_state(st_rx_t *, st_state_t);
extern int	stc_vc_fsm(struct stpcb **, ushort, sthdr_t *,
						struct inaddrpair *);
extern int	st_data_fsm(struct stpcb **, ushort, sthdr_t *, int tid);
extern int	st_tx_timer_expiry(struct stpcb *, uint);
extern int	st_rx_timer_expiry(struct stpcb *, uint);
extern int	st_slots_timer_expiry(struct stpcb *);

extern int	st_ask_for_slots(struct stpcb *);
extern int	st_sync_up(struct stpcb *, sthdr_rs1_t *);
extern int	wait_for_slots(struct stpcb *);

extern int	st_process_RSR(struct stpcb *, sthdr_t *);
extern int	st_respond_to_RTS(struct stpcb *, sthdr_t *, int);
extern int	st_send_WAIT(struct stpcb *, sthdr_t *, int);
extern int	st_process_DATA(struct stpcb *, sthdr_t *, int, 
							struct mbuf *);
extern int	st_ask_for_ack(struct stpcb *, st_tid_t);
extern int	st_ack_last_recv(struct stpcb *, st_tid_t, uint32_t);

extern int	st_ask_for_block_info(struct stpcb *, 
						uint32_t, st_tid_t);
extern int	st_ack_a_block(struct stpcb *, st_tid_t, sthdr_rs3_t *);

extern int	st_sosend(register struct socket *, struct mbuf *,
				register struct uio *, int, 
							struct mbuf *);
extern int	st_soreceive(register struct socket *, struct mbuf **,
				register struct uio *, int *, 
							struct mbuf **);
extern int	st_sodisconnect(register struct socket *, int);

extern int	st_alloc_kern_buf(caddr_t *, uint);
extern void	st_free_kern_buf(caddr_t *, uint);

extern void 	iidfree_all(struct stpcb *);
extern void 	ridfree_all(struct stpcb *);
extern void 	kidfree_all(struct stpcb *);
extern int 	rx_is_valid(struct stpcb *, st_tid_t);
extern int 	get_max_set_bnum(st_rx_t *);

extern int	st_retire_write(struct stpcb *, sthdr_t *sth, int);

extern void	st_set_timer(struct stpcb *, uint, uint);
extern void	st_cancel_timer(struct stpcb *, uint);
extern void	st_cancel_timers(struct stpcb *);
extern struct stpcb *	st_timers(struct stpcb *, __psint_t);
extern int	st_setup_buf(struct stpcb *, st_buf_t *, 
					uio_t *, u_char, st_tid_t);
extern int	st_teardown_buf(struct stpcb *, st_buf_t *, u_char,
							st_tid_t);
extern int      st_bypass_setopt(int op, struct socket *so, int level,
					int optname, struct mbuf **mp);
extern int      st_bypass_getopt(int op, struct socket *so, int level,
					int optname, struct mbuf **mp);

#endif /* _KERNEL */

#define STP_SIOC_GETSTIFNET     _IOWR('Z', 128, caddr_t)

#endif 	/* __ST_VAR_H__ */
