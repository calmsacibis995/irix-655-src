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
 *  Filename: st_subr.c
 *  Description: sundry subroutines for use by the ST protocol.
 */


#include "sys/param.h"
#include "sys/debug.h"
#include "sys/systm.h"
#include "sys/mbuf.h"
#include "sys/socket.h"
#include "sys/socketvar.h"
#include "sys/protosw.h"

#include "sys/sysmacros.h"
#include "sys/errno.h"
#include "net/route.h"
#include "net/if.h"
#include "sys/types.h"
#include "sys/kmem.h"

#include "in.h"
#include "in_systm.h"
#include "ip.h"
#include "in_pcb.h"
#include "ip_var.h"
#include "ip_icmp.h"

#include "ksys/xthread.h"
#include "net/netisr.h"

#include "sys/tcpipstats.h"
#include "sys/cmn_err.h"

#include "st.h"
#include "st_var.h"
#include "st_macros.h"
#include "st_bufx.h"
#include "st_if.h"
#include "st_debug.h"

extern  network_input_t 	st_input;
extern 	int 	in_pcb_hashtablesize(void);


#define ST_MINHASHTABLESZ              64
#define ST_MAXHASHTABLESZ              8192
int st_hashtablesz;
zone_t *stpcb_zone;
struct inpcb stpcb_head; /* head of ST protocol control block list */

struct ifqueue stpintrq;


static lock_t	rid_tab_spinlock;
static lock_t	iid_tab_spinlock;
static lock_t	st_R_Mx_spinlock;
/* SHAC has only 12 bits for R-Mx */
#define	MAX_st_R_Mx	0x01000		

volatile char	st_R_Mx_tab[MAX_st_R_Mx];



int
log2(uint x)
{
	uint	tmp = x;
	int	log_x = -1;

	while(tmp)  {
		tmp >>= 1;
		log_x++;
	}
	return log_x;
}


static void
init_st_tid_tabs(void)
{
	spinlock_init(&rid_tab_spinlock, "st_rid_tab");
	spinlock_init(&iid_tab_spinlock, "st_iid_tab");
	spinlock_init(&st_R_Mx_spinlock, "st_R_Mx");
}


int
get_max_set_bnum(st_rx_t *rx)
{
	uchar_t	res;
	int	i, j;

	for(res = (uchar_t) 0xff, i = 0; i < BNUM_TAB_SIZE; i++)  {
		res = ((uchar_t) rx->bnum_tab[i]) & ((uchar_t) 0xff);
		dprintf(10, ("i: %d, tab-ent: 0x%x, res: 0x%x\n",
			i, (uchar_t) rx->bnum_tab[i], (uchar_t) res));
		if(0xff != (uchar_t) res)  {
			break;
		}
	}

	dprintf(10, ("i: %d, res: 0x%x at break\n",
		i, (uchar_t) res));

	if(BNUM_TAB_SIZE == i)  {
		return -1;
	}

	for(j = 0; j < 8; j++)  {
		if(!(res & (1 << j))) {
			break;
		}
	}

	if(! (i + j))  {
		return -1;
	}

	return i * 8 + j - 1;
}


static int
alloc_key(struct stpcb *sp, ushort opcode)
{
	if(opcode != ST_RCONNECT && opcode != ST_CANSWER) {
		dprintf(0, ("alloc_key: not connect or accept\n"));
		return -1;
	}

	while(sp->s_vcd.vc_lkey == 0) {
		/* if the key is invalid, both bypass and non-bypass
		** jobs will want a key; so the job-flags check is
		** not required */
		sp->s_vcd.vc_lkey = (u_int32_t) random();
	    	/* Temporary until rendevous spec is finalized */
	    	sp->s_vcd.vc_lkey = 0xdeadbeef;
	}

	return 0;
}



static u_int16_t
alloc_R_Mx(struct stpcb *sp)
{
	u_int16_t	rmx  = (u_int16_t) INVALID_R_Mx;
	int		cookie;
	/* SHAC gives us 12 bits of Mx, dammit! */
	static	ushort	mxcount;
	ushort		num_seen = 0;

	cookie = SPINLOCK(st_R_Mx_spinlock);
	do {
		rmx = mxcount;
		mxcount++;
		if(mxcount == MAX_st_R_Mx)  {
			mxcount = 0;
		}
		num_seen++;
	}  while(MX_IS_SET(st_R_Mx_tab, rmx) 
					&& num_seen < MAX_st_R_Mx);
	if(num_seen < MAX_st_R_Mx)  {
		dprintf(30, ("before set: rmx %u is 0x%x\n",
			rmx, st_R_Mx_tab[rmx]));
		MX_SET(st_R_Mx_tab, rmx);
		dprintf(30, ("after set: rmx %u is 0x%x\n",
			rmx, st_R_Mx_tab[rmx]));
		dprintf(20, ("set: rmx %u, Idx %u, shift %u, mask 0x%x\n",
			rmx, (rmx >> 3), (rmx & 0x7), 
			((0x80) >> (rmx & 0x7))));
		ASSERT(MX_IS_SET(st_R_Mx_tab, rmx));
		dprintf(20, ("is_set: rmx %u, Idx %u, shift %u, mask 0x%x\n",
			rmx, (rmx >> 3), (rmx & 0x7),
			((0x80) >> (rmx & 0x7))));
		
	}
	SPINUNLOCK(st_R_Mx_spinlock, cookie);
	
	if(num_seen >= MAX_st_R_Mx)  {
		rmx = (u_int16_t) INVALID_R_Mx;
		cmn_err(CE_PANIC, "Returning invalid RMX\n");
	}

	sp->last_rx_mx_setup = rmx;
	return rmx;
}


static void
dealloc_R_Mx(struct stpcb *sp, u_int16_t rmx)
{
	int		cookie;

	cookie = SPINLOCK(st_R_Mx_spinlock);
	dprintf(30, ("clear: rmx %u, Idx %u, shift %u, mask 0x%x\n",
		rmx, (rmx >> 3), (rmx & 0x7),
		(0x0ff & ~((0x80) >> (rmx & 0x7)))));
	ASSERT(MX_IS_SET(st_R_Mx_tab, rmx));
	MX_CLEAR(st_R_Mx_tab, rmx);
	ASSERT(! MX_IS_SET(st_R_Mx_tab, rmx));
	SPINUNLOCK(st_R_Mx_spinlock, cookie);
	sp->last_rx_mx_torndown = rmx;
}


int
get_new_tid(struct stpcb *sp, int hint, int max, char tx_or_rx)
{
	int		free = INVALID_TID, i;
	st_tx_t		*tx;
	st_rx_t		*rx;
	static u_int32_t	tx_iid = 1, rx_rid = 1;
	static const u_int32_t	ID_MASK = (u_int32_t) -1;

	/* 
	* linear right now; will optimize later if reqd
	* [must be CALLED with lock held]
	*/
	if(tx_or_rx == TX)  {
		tx = &(sp->tx[0]);
		for(i = hint; FREE_TID != tx[i].tx_iid && i < max; i++) {
			;
		}

		if(i < max)  {
			free = i;
			/* tx[i].tx_iid = lbolt; */
			do {
				/* CHECK: overflow in the following? */
				tx[i].tx_iid = tx_iid++ & ID_MASK;
			} while (tx[i].tx_iid == FREE_TID);
			dprintf(30, ("iid alloced is %u\n", 
				tx[i].tx_iid));
		}
	}
	else if(tx_or_rx == RX) {
		rx = &(sp->rx[0]);
		for(i = hint; FREE_TID != rx[i].rx_rid && i < max; i++) {
			;
		}

		if(i < max)  {
			free = i;
			/* rx[i].rx_rid = lbolt; */
			do {
				/* CHECK: overflow in the following? */
				rx[i].rx_rid = rx_rid++ & ID_MASK;
			} while (rx[i].rx_rid == FREE_TID);
			dprintf(30, ("rid alloced is %u\n", 
				rx[i].rx_rid));
		}
	}
	
	ASSERT(0 == free);
	return free;
}



int
find_tid_index(u_int32_t tid, struct stpcb *sp, int max, char tx_or_rx)
{
	int		rv = -1, i;
	st_tx_t		*tx;
	st_rx_t		*rx;

	if(tx_or_rx == TX)  {
		tx = &(sp->tx[0]);
		for(i = 0; tid != tx[i].tx_iid && i < max; i++)  {
			;
		}

		if(i < max) {
			rv = i;
		}	
	}
	else if(tx_or_rx == RX)  {
		rx = &(sp->rx[0]);
		for(i = 0; tid != rx[i].rx_rid && i < max; i++)  {
			;
		}

		if(i < max) {
			rv = i;
		}	
	}

	if(0 != rv) {
		dprintf(0, ("Bad tid 0x%x; returning %d from find_tid_index\n",
			tid, rv));
	}
	return rv;
}


u_int32_t
find_iid(int index, struct stpcb *sp, int max, char tx_or_rx)
{
	u_int32_t		iid = INVALID_TID;
	st_rx_t			*rx;
	st_tx_t			*tx;

	if(tx_or_rx == TX)  {
		tx = &(sp->tx[index]);
		iid = tx->tx_iid;
	}
	else if(tx_or_rx == RX)  {
		rx = &(sp->rx[index]);
		iid = rx->rx_iid;
	}
	return iid;
}


u_int32_t
find_rid(int index, struct stpcb *sp, int max, char tx_or_rx)
{
	u_int32_t		rid = INVALID_TID;
	st_rx_t			*rx;
	st_tx_t			*tx;

	if(tx_or_rx == TX)  {
		tx = &(sp->tx[index]);
		rid = tx->tx_rid;
	}
	else if(tx_or_rx == RX)  {
		rx = &(sp->rx[index]);
		rid = rx->rx_rid;
	}
	return rid;
}


static st_tid_t
alloc_zero_iid(struct stpcb *sp)
{
	st_tid_t	iid;
	st_tx_t	 	*tx;
	static st_tid_t hint;
	int		cookie;

	if(sp->s_vc_state != STP_VCS_CONNECTED)  {
		iid = -1;
		cmn_err(CE_WARN, 
		"allocating iid prematurely; VC still in state %s\n",
			st_decode_state(sp->s_vc_state));
	}
	else {
		cookie = SPINLOCK(iid_tab_spinlock);
		iid = get_new_tid(sp, hint, MAX_TX_ENTRIES, TX);
		SPINUNLOCK(iid_tab_spinlock, cookie);
		if(iid != INVALID_TID)  {
			hint = iid;
			tx = &(sp->tx[iid]);
			tx->tx_state = STP_VCS_CONNECTED;
			tx->tx_max_Bnum_sent = 0;
			tx->tx_num_cts_seen = 0;
			bzero(tx->data_CTS_tab, BNUM_TAB_SIZE);
		}
	}

	return iid;
}


static st_tid_t
alloc_zero_rid(struct stpcb *sp)
{
	st_tid_t	rid;
	st_rx_t		*rx;
	static st_tid_t hint;
	int		cookie;

	if(sp->s_vc_state != STP_VCS_CONNECTED)  {
		cmn_err(CE_WARN, 
		"allocating rid prematurely; VC still in state %s\n",
			st_decode_state(sp->s_vc_state));
		rid = -1;
	}
	else {
		cookie = SPINLOCK(rid_tab_spinlock);
		rid = get_new_tid(sp, hint, MAX_RX_ENTRIES, RX);
		SPINUNLOCK(rid_tab_spinlock, cookie);
		if(rid != INVALID_TID)  {
			hint = rid;
			rx = &(sp->rx[rid]);
			rx->rx_state = STP_VCS_CONNECTED;
			bzero(rx->bnum_tab, BNUM_TAB_SIZE);
			bzero(rx->CTS_tab, BNUM_TAB_SIZE);
		}
	}

	return rid;
}


static st_tid_t
alloc_zero_kid(struct stpcb *sp)
{
	st_tid_t	kid;
	st_kx_t		*kx;

	if(sp->s_vc_state != STP_VCS_CONNECTED)  {
		cmn_err(CE_WARN, 
		    	"k-id alloc on unconnected VC (state %s)\n",
				st_decode_state(sp->s_vc_state));
		kid = -1;
	}
	else {
		if(sp->num_kid_allocated != 0) {
			cmn_err(CE_WARN, "already a k-id allocated");
			kid = -1;
		}
		else {
			sp->num_kid_allocated++;
			kid = 0;
			kx = &(sp->kx[kid]);
			kx->kx_state = STP_VCS_CONNECTED;
		}
	}

	return kid;
}


static int
dealloc_zero_iid(struct stpcb *sp, st_tid_t iid)
{
	st_tx_t		*tx;
	int		cookie;


	tx = &(sp->tx[iid]);
	/* tx->tx_state = STP_INVALID_STATE; */
	/* 
	** should be in connected state: we don't have a TX buffer, 
	** but are still connected! 
	*/
	tx->tx_state = STP_READY_FOR_RTS;

	if(tx->tx_buf.bufx_flags != BUF_NONE)  {
		if(st_teardown_buf(sp, &tx->tx_buf,
				(u_char) ST_BUFX_ALLOW_SEND, iid)) {
			cmn_err(CE_PANIC, "st_teardown_buf failed\n");
		}
	}

	dprintf(30, ("deallocing iid %d\n", tx->tx_iid));
	cookie = SPINLOCK(iid_tab_spinlock);
	tx->tx_iid = FREE_TID;
	SPINUNLOCK(iid_tab_spinlock, cookie);

	return 0;
}


static int
dealloc_zero_rid(struct stpcb *sp, st_tid_t rid)
{
	st_rx_t		*rx;
	int		cookie;
	st_ifnet_t      *stifp = (st_ifnet_t *) sp->s_stifp;

	rx = &(sp->rx[rid]);
	/* rx->rx_state = STP_INVALID_STATE; */
	/*
	** should really be connected: we don't have a RX buffer,
	** but are still connected 
	*/ 
	rx->rx_state = STP_READY_FOR_RTS;


	if(rx->rx_buf.bufx_flags != BUF_NONE)  {
		if(st_teardown_buf(sp, &rx->rx_buf,
				(u_char) ST_BUFX_ALLOW_RECV, rid)) {
			cmn_err(CE_PANIC, "st_teardown_buf failed\n");
		}
	}
	
	while (rx->rx_num_mx > 0) {
		dprintf(30, ("dealloc_zero_rid: R_Mx_free on rmx %d\n",
			     rx->rx_mx));
		/* pre-decrement so can use as index into array */
		rx->rx_num_mx--;
		ASSERT_ALWAYS(0 == (*stifp->if_st_clear_mx)(
					 sp->s_ifp, rx->rx_mx[rx->rx_num_mx]));
		sp->s_R_Mx_free(sp, rx->rx_mx[rx->rx_num_mx]);
	}

	dprintf(30, ("deallocing rid %d\n", rx->rx_rid));
	cookie = SPINLOCK(rid_tab_spinlock);
	rx->rx_rid = FREE_TID;
	SPINUNLOCK(rid_tab_spinlock, cookie);

	return 0;
}


static int
dealloc_zero_kid(struct stpcb *sp, st_tid_t kid, sthdr_t *sth)
{
	st_kx_t		*kx;

	ASSERT_ALWAYS(1 == sp->num_kid_allocated);
	ASSERT_ALWAYS(0 == kid);	/* one outstanding for now */
	kx = &(sp->kx[kid]);

	bcopy(&kx->saved_rts, sth, sizeof(sthdr_t));

	bzero(kx, sizeof(st_kx_t));
	kx->kx_state = STP_INVALID_STATE;
	sp->num_kid_allocated--;

	return 0;
}


void
iidfree_all(struct stpcb *sp)
{
	ASSERT_ALWAYS(0 == sp->s_iidfree(sp, 0));
}


void
ridfree_all(struct stpcb *sp)
{
	ASSERT_ALWAYS(0 == sp->s_ridfree(sp, 0));
}

void
kidfree_all(struct stpcb *sp)
{
	sthdr_t		sth;

	ASSERT_ALWAYS(0 == sp->s_kidfree(sp, 0, &sth));
}


int
rx_is_valid(struct stpcb *sp, st_tid_t rid)
{
	st_rx_t		*rx = &(sp->rx[rid]);

	if(rx->rx_rid != FREE_TID
		&& rx->rx_rid != INVALID_TID 
		&& IS_RX_STATE(rx->rx_state)
	        && GOODMX(rx->rx_mx[0])) {
		return 1;
	}
	else {
		dprintf(20, ("rx_is_valid: State is %s, rid %u\n",
			st_decode_state(rx->rx_state), rx->rx_rid));
		return 0;
	}
}

int
wait_for_slots(struct stpcb *sp)
{
	int		retval = 0;
	struct  socket  *so = stpcbtoso(sp);

	ASSERT_ALWAYS(SOCKET_ISLOCKED(so));
	if(SLOTS_ENABLED(sp)) {
		if(! sp->s_vcd.asked_for_slots)  {
			st_set_timer(sp, STPT_SLOT_TIMER, SLOTS_TIMEOUT_VAL);
			dprintf(30, ("Set timer %d\n", STPT_SLOT_TIMER));
		}
		sp->s_vcd.asked_for_slots++;
		dprintf(10, ("Waiting for slots \n"));
		retval = sv_wait_sig(&(sp->s_vcd.slot_sync),
						PZERO, &so->so_sem, 0);
		if(-1 == retval) {
			dprintf(0, ("slot wait INTR!\n"));
		}
		dprintf(10, ("Done with slots wait; status %d\n", 
			retval));
		ASSERT_ALWAYS(! SOCKET_ISLOCKED(so));
		SOCKET_LOCK(so);
		st_cancel_timer(sp, STPT_SLOT_TIMER);
	}

	dprintf(30, ("Returning %d from wait_for_slots\n", retval));
	return retval;
}


int
slots_available(struct stpcb *sp)
{
	int		retval = 0, num_sleeping, num_woken;
	struct  socket  *so = stpcbtoso(sp);

	ASSERT_ALWAYS(SOCKET_ISLOCKED(so));
	if(SLOTS_ENABLED(sp)) {
		num_sleeping = sp->s_vcd.asked_for_slots;
		sp->s_vcd.asked_for_slots = 0;
		/* 
		num_woken = sv_signal(&(sp->s_vcd.slot_sync));
		*/
		num_woken = sv_broadcast(&(sp->s_vcd.slot_sync)); 
		ASSERT_ALWAYS(num_sleeping == num_woken);
		dprintf(10, ("Woken %d with slots, %d sleeping\n",
			num_woken, num_sleeping));
		retval = (num_woken == 0);
	}

	dprintf(30, ("Returning %d from slots_available\n", retval));
	return retval;
}


int
st_send_REJECT(struct stpcb *sp, sthdr_rts_t *rts, 
						ushort OpFlags, int tid)
{
	int		error = 0;
	struct  mbuf    *m0;
	sthdr_t		*sth;
	if_st_tx_desc_t	*ptxdesc;
	sthdr_ra_t	*ra;
	struct  socket  *so    = stpcbtoso(sp);

	STSTAT(stps_txrejects);
	ASSERT_ALWAYS(ST_RANSWER == (OpFlags & ST_OPCODE_MASK));
	
	/* bad hack, since we need to initialize the I-id of the
	*  RANSWER to the incoming rts, which will be lost if we
	*  directly call st_data_ctl_output()
	*/

	m0 = m_getclr(M_DONTWAIT, MT_HEADER);
	if (NULL == m0)  {
		return -1;
	}

	m0->m_len = MSIZE;
	m0->m_next = NULL;
	m0->m_off = MMINOFF;
	sth = (sthdr_t *)  (mtod(m0, caddr_t) + 
				sp->s_stifp->xmit_desc_sthdr_off);
	ptxdesc = (if_st_tx_desc_t *) (mtod(m0, caddr_t) +
				sp->s_stifp->xmit_desc_ifhdr_off);
	ptxdesc->flags = IF_ST_TX_CTL_MSG;
	ptxdesc->vc = 0;

	ra = &(sth->sth_ra);
	ra->I_id = rts->I_id;

	if(error = stc_hdr_template(sp, (OpFlags|ST_INTERRUPT), 
							sth, tid))  {
		dprintf(0, ("stc_hdr_template error in REJECT\n"));
		m_free(m0);
		return error;
	}

	error = st_output(m0, sp->s_so);

	if(error)  {
		cmn_err(CE_WARN, "Could not send REJECT \n");
	}

	return error;
}


int
st_alloc_kern_buf(caddr_t *vaddr, uint size)
{
	int	align = (1 << 20);	/* megabyte */

	dprintf(30, ("Allocating %u bytes (%d pages) in st_alloc_kern_buf\n",
		size, btoc(size)));

#if 0
	*vaddr = kvpalloc(btoc(size), VM_NOSLEEP | VM_CACHEALIGN, 0); 
#else
	*vaddr = kmem_contig_alloc(size, align, VM_NOSLEEP);
#endif
	
	if(NULL == *vaddr)  {
		cmn_err(CE_WARN, "st_alloc_kern_buf: no space for %u bytes\n",
			size);
		return -1;
	}

	ASSERT(! IS_KUSEG(*vaddr));
	bzero(*vaddr, size);		/* for debug */

	return 0;
}


void
st_free_kern_buf(caddr_t *vaddr, uint size)
{
	dprintf(30, ("Freeing %u bytes (%d pages) in st_free_kern_buf\n", 
		size, btoc(size)));

	ASSERT_ALWAYS(vaddr);
#if 0
	kvpfree(*vaddr, btoc(size)); 
#else
	kmem_contig_free(*vaddr, size);
#endif
	*vaddr = NULL;
}


int
st_retire_write(struct stpcb *sp, sthdr_t *sth, int tid)
{
	st_tx_t *tx = &(sp->tx[tid]);
	struct socket	*so = stpcbtoso(sp);
	sthdr_ra_t	*ra = &(sth->sth_ra);
	sthdr_rsr2_t	*rsr2 = &(sth->sth_rsr2);
	ushort		OpFlags = ra->OpFlags;
	ushort		opcode = OpFlags & ST_OPCODE_MASK;

	if(tx->tx_tlen != 0)  {
		switch (opcode) {
		default:
			dprintf(0, ("Bad opcode %s; st_retire_write\n",
				st_decode_opcode));
			return -1;
			/* NOTREACHED */
			break;

		case ST_RANSWER:
			if(OpFlags & ST_REJECT) {
		  		STSTAT(stps_rxrejects);
		  		so->so_error = EINVAL;	/* bad wr len */
			}
			else {
				dprintf(25, ("kx, not EAGAIN, state %s\n",
					st_decode_state(tx->tx_state)));
				tx->tx_state = STP_RTS_PINNED;
				return 0;
			}
			break;

		case ST_RSR:
			dprintf(10, ("Got RSR-2; tx ongoing (%u left);"
				" B_seq %d\n",
				tx->tx_tlen, rsr2->B_seq));
#ifdef			ST_HDR_DUMP
			dprintf(0, ("Dump header in st_retire_write\n"));
#endif 			/* ST_HDR_DUMP */
			ST_DUMP_HDR(sth);
			tx->tx_ACKed = 1;
			if((int) tx->tx_acked_Bnum < 
						(int) rsr2->B_seq)  {
				tx->tx_acked_Bnum = rsr2->B_seq;
			}
			return 0;
			/* NOTREACHED */
			break;
		}
	
	}

	ASSERT_ALWAYS((OpFlags & ST_REJECT) || tx->tx_tlen == 0);

	switch(opcode) {
		default:
			dprintf(0, ("Bad opcode %s; st_retire_write\n",
				st_decode_opcode));
			return -1;
			/* NOTREACHED */
			break;

		case ST_RSR:
 			if(rsr2->B_seq == tx->tx_max_Bnum_sent)  {
				dprintf(30, ("tlen %d; B_seq %d, "
					"max-bnum %d; woken\n",
					tx->tx_tlen, rsr2->B_seq, 
					tx->tx_max_Bnum_sent));
				dprintf(30, ("tx_tlen 0; wakeup "
					"so 0x%x, buf 0x%x\n",
					so, so->so_snd));
				/* sowwakeup(sp->s_so, NETEVENT_STPPUP); */
				SOCKET_SBWAIT_WAKEALL(&so->so_snd);
			}
			else {
				dprintf(5, ("tlen %d; B_seq %d, "
					"max-bnum %d; no wake\n",
					tx->tx_tlen, rsr2->B_seq, 
					tx->tx_max_Bnum_sent));
			}
			break;

		case ST_RANSWER:
			dprintf(0, ("tlen %d; write rejected\n",
				tx->tx_tlen));
			SOCKET_SBWAIT_WAKEALL(&so->so_snd);
			break;
	}

	return 0;
}


int
st_ask_for_ack(struct stpcb *sp, st_tid_t tid)
{
	int		error = 0;
	sthdr_rs2_t	*rs;
	struct	mbuf	*m0;
	if_st_tx_desc_t *ptxdesc;
	st_tx_t		*tx;
	struct  socket  *so    = stpcbtoso(sp);
	

	m0 = m_getclr(M_DONTWAIT, MT_HEADER);
	if (NULL == m0) {
		cmn_err(CE_PANIC, "No mbufs to send RS\n");
	}
	m0->m_len  = sizeof(struct st_io_s);
	m0->m_next = NULL;
	m0->m_off  = MMINOFF;

	rs      = (sthdr_rs2_t *) (mtod(m0, caddr_t) + 
	   			sp->s_stifp->xmit_desc_sthdr_off);
	ptxdesc = (if_st_tx_desc_t *) (mtod(m0, caddr_t) +
				sp->s_stifp->xmit_desc_ifhdr_off);

	if(error = st_rs_template(sp, ST_RS, 
				  (sthdr_t *) rs, ST_STATE_XFER)) {
		dprintf(0, ("st_rs_template returned error\n"));
		return error;
	}

	ptxdesc->flags = IF_ST_TX_CTL_MSG;
	ptxdesc->vc = 0;

	ASSERT(0 == tid);
	tx = &(sp->tx[tid]);
	rs->I_id = tx->tx_iid;
	rs->R_id = tx->tx_rid;

	dprintf(30, ("%s from port %d (%d) to port %d (%d), I_id %d, R_id %d\n",
		st_decode_opcode(rs->OpFlags),
		rs->R_Port, sp->s_vcd.vc_rport,
		rs->I_Port, sp->s_vcd.vc_lport,
		rs->I_id, rs->R_id));

#ifdef	ST_HDR_DUMP
	dprintf(0, ("Dump header in st_ask_for_ack\n"));
#endif 	/* ST_HDR_DUMP */
	ST_DUMP_HDR((sthdr_t *) rs);

	/* don't check for num-slots: we'll get slots on RSR */
	error = st_output(m0, sp->s_so);
	if(error)  {
		dprintf(0, ("st_ask_for_ack: error %d from st_output\n",
			error));
	}
	return error;
}



int
st_ask_for_slots(struct stpcb *sp)
{
	int		error = 0;
	sthdr_rs1_t	*rs;
	struct	mbuf	*m0;
	if_st_tx_desc_t *ptxdesc;
	

	m0 = m_getclr(M_DONTWAIT, MT_HEADER);
	if (NULL == m0) {
		cmn_err(CE_PANIC, "No mbufs to send RS-slots\n");
	}
	m0->m_len  = sizeof(struct st_io_s);
	m0->m_next = NULL;
	m0->m_off  = MMINOFF;

	rs      = (sthdr_rs1_t *) (mtod(m0, caddr_t) + 
	   			sp->s_stifp->xmit_desc_sthdr_off);
	ptxdesc = (if_st_tx_desc_t *) (mtod(m0, caddr_t) +
				sp->s_stifp->xmit_desc_ifhdr_off);

	if(error = st_rs_template(sp, ST_RS, 
				  (sthdr_t *) rs, ST_STATE_SLOTS)) {
		dprintf(0, ("st_rs_template returned error\n"));
		return error;
	}

	dprintf(15, ("ask_for_slots: RS sync num %d\n",
		rs->Sync));

	ptxdesc->flags = IF_ST_TX_CTL_MSG;
	ptxdesc->vc = 0;

	dprintf(30, ("%s from port %d (%d) to port %d (%d)\n",
		st_decode_opcode(rs->OpFlags),
		rs->R_Port, sp->s_vcd.vc_rport,
		rs->I_Port, sp->s_vcd.vc_lport));

#ifdef	ST_HDR_DUMP
	dprintf(0, ("Dump header in st_ask_for_slots\n"));
#endif 	/* ST_HDR_DUMP */
	ST_DUMP_HDR((sthdr_t *) rs);


	/* don't check for slots here */
	error = st_output(m0, sp->s_so);
	if(error)  {
		dprintf(0, ("st_ask_for_ack: error %d from st_output\n",
			error));
	}
	else {
		dprintf(20, ("st_ask_for_ack: sent RS-SLOTS\n"));
	}
	return error;
}

int
st_ack_last_recv(struct stpcb *sp, st_tid_t tid, uint32_t Sync)
{
	int		error = 0;
	sthdr_rsr2_t	*rsr;
	struct	mbuf	*m0;
	if_st_tx_desc_t *ptxdesc;
	st_rx_t		*rx;
	sthdr_t		*sth;
	sthdr_data_t 	*data;
	struct  socket  *so    = stpcbtoso(sp);
	st_ifnet_t      *stifp = (st_ifnet_t *) sp->s_stifp;
	

	ASSERT_ALWAYS(SOCKET_ISLOCKED(so));
	m0 = m_getclr(M_DONTWAIT, MT_HEADER);
	if (NULL == m0) {
		cmn_err(CE_PANIC, "No mbufs to send RSR\n");
	}
	m0->m_len  = sizeof(struct st_io_s);
	m0->m_next = NULL;
	m0->m_off  = MMINOFF;


	rsr     = (sthdr_rsr2_t *) (mtod(m0, caddr_t) + 
	   			sp->s_stifp->xmit_desc_sthdr_off);
	ptxdesc = (if_st_tx_desc_t *) (mtod(m0, caddr_t) +
				sp->s_stifp->xmit_desc_ifhdr_off);

	rx = &(sp->rx[tid]);
	rsr->R_id = rx->rx_rid;
	sp->last_RSR_R_id_sent = rsr->R_id;
	sp->last_RSR_I_id_sent = rsr->I_id;
	rsr->B_seq = get_max_set_bnum(rx);
	rsr->Sync = Sync;
	if(error = st_rs_template(sp, ST_RSR, 
				  (sthdr_t *) rsr, ST_STATE_XFER)) {
		dprintf(0, ("st_rs_template returned error\n"));
		return error;
	}

	dprintf(20, ("RSR-%d B_seq set to %d, Sync %d, slots %d\n",
		ST_STATE_XFER, rsr->B_seq, rsr->Sync, rsr->R_Slots));

	ptxdesc->flags = IF_ST_TX_CTL_MSG;
	ptxdesc->vc = 0;

	if(rx->hdr_for_RSR) {
		sth = mtod(rx->hdr_for_RSR, sthdr_t *);
		data = &(sth->sth_data);
		dprintf(30, ("RSR B_seq set to %d\n",  rsr->B_seq));
		if(rx->rx_data_len == 0)  {
			dprintf(30, ("Freeing hdr_for_RSR mbuf\n"));
			m_freem(rx->hdr_for_RSR);
			rx->hdr_for_RSR = NULL;


			while (rx->rx_num_mx > 0) {
				/* pre-decrement so can use as index into array */
				rx->rx_num_mx--;
				dprintf(20, ("ack_last_recv: Mx free "
					     "on mx %d\n", rx->rx_mx[rx->rx_num_mx]));
				ASSERT_ALWAYS(0 == 
					      (*stifp->if_st_clear_mx)(
						    sp->s_ifp, rx->rx_mx[rx->rx_num_mx]));
				sp->s_R_Mx_free(sp, rx->rx_mx[rx->rx_num_mx]);
			}
		}
	} 

	dprintf(20, ("%s from port %d (%d) to port %d (%d)\n",
		st_decode_opcode(rsr->OpFlags),
		rsr->R_Port, sp->s_vcd.vc_rport,
		rsr->I_Port, sp->s_vcd.vc_lport));

#ifdef	ST_HDR_DUMP
	dprintf(0, ("Dump header in st_ack_last_recv\n"));
#endif  /* ST_HDR_DUMP */
	ST_DUMP_HDR((sthdr_t *) rsr);

	/* no need to check for slots in RSR */
	error = st_output(m0, sp->s_so);
	if(error)  {
		dprintf(0, ("st_ack_last_recv: error %d from st_output\n",
			error));
	}
	return error;
}


void
st_init(void)
{

  	static int st_inited = 0;
  	DPRINTF(ST_DEBUG_ENTRY,("ENTRY: st_init()\n"));

  	stpintrq.ifq_maxlen = IFQ_MAXLEN;

  	if (st_inited == 0) {
		/* init_max_netprocs();  -- needed? */
    		stpcb_zone = kmem_zone_init(
					sizeof(struct stpcb), "stpcb");
		network_input_setup(AF_STP, st_input);
    		st_inited = 1;
  	}

  	if (st_hashtablesz == 0) {
    		st_hashtablesz = in_pcb_hashtablesize();
  	}
  	if (st_hashtablesz < ST_MINHASHTABLESZ)
    		st_hashtablesz = ST_MINHASHTABLESZ;

  	if (st_hashtablesz > ST_MINHASHTABLESZ)
    		st_hashtablesz = ST_MAXHASHTABLESZ;

  	st_hashtablesz *= 2;

  	/*
   	* We need to update this so we can maintain some accurate
   	* PCBSTATS on ST.  But, let's leave this as TCP for now. 
   	*/

  	in_pcbinitcb(&stpcb_head, st_hashtablesz, 0, TCP_PCBSTAT);

	init_st_bufx_tab();
	init_st_tid_tabs();
	bzero((void *) st_R_Mx_tab, MAX_st_R_Mx);
  	return;
}


int
st_start_write(struct stpcb *sp, int tid)
{
	st_tx_t		*tx;
	int		error;
	uint		timer_id;

	ASSERT_ALWAYS(tid == 0);
	tx = &(sp->tx[tid]);
	error = st_data_ctl_output(sp, ST_RTS, tid);

	if(! error)  {
		timer_id = TID_TO_TIMER_ID(tid);
		st_set_timer(sp, timer_id, TX_TIMEOUT_VAL);

		st_data_init_tx_state(tx, STP_RTS_PINNED);
	}

	return error;
}




void
st_fasttimo(void)
{
  	DPRINTF((ST_DEBUG_TIMERS | ST_DEBUG_ENTRY),
					("ENTRY: st_fasttimo()\n"));
  	return;
}

void
st_slowtimo(void)
{
	register struct inpcb *ip, *ipnxt;
	register struct stpcb *sp;
	register int i;
	register struct socket *so;
	int hash, ehash;
	struct in_pcbhead *hinp;

	ehash = (stpcb_head.inp_tablesz - 1) / 2;
	for (hash = 1; hash <= ehash; hash++) {
resync:

	hinp = &stpcb_head.inp_table[hash];
	INHHEAD_LOCK(hinp);
	ip = hinp->hinp_next;
	if (ip == (struct inpcb *)hinp) {
		INHHEAD_UNLOCK(hinp);
		continue;	/* get next bucket */
	}
	INPCB_HOLD(ip);
	for (; (ip != (struct inpcb *)hinp) && (ip->inp_hhead == hinp);
	       ip = ipnxt) {

		so = ip->inp_socket;
		ipnxt = ip->inp_next;
		INPCB_HOLD(ipnxt);
		INHHEAD_UNLOCK(hinp)
		SOCKET_LOCK(so);

		sp = intostpcb(ip);
		/* skip rexmt timer; now handled by fast timer */
		for (i = 0; i < STPT_NTIMERS; i++) {

			if (sp->s_timer[i] && --sp->s_timer[i] == 0) {

				(void) st_usrreq(sp->s_so,
				    PRU_SLOWTIMO, (struct mbuf *)0,
				    (struct mbuf *)(NULL+i), (struct mbuf *)0);
			}
		}
		if (!INPCB_RELE(ip)) {
			SOCKET_UNLOCK(so);
		}

		INHHEAD_LOCK(hinp);
		if (ipnxt->inp_next == 0) {
			so = ipnxt->inp_socket;
			INHHEAD_UNLOCK(hinp);
			SOCKET_LOCK(so);
			if (!INPCB_RELE(ipnxt)) 
				SOCKET_UNLOCK(so);

			hash++; 
			if (hash <= ehash) {
				goto resync;
			} else {
				goto out;
			}
		}
	}

	INHHEAD_UNLOCK(hinp);
	if (ip != (struct inpcb *)hinp) {
		so = ip->inp_socket;
		SOCKET_LOCK(so);
		if (!INPCB_RELE(ip)) {
			SOCKET_UNLOCK(so);
		}
	}
	}
out:
	return;
}

void
st_drain(void)
{
  DPRINTF(ST_DEBUG_ENTRY,("ENTRY: st_drain()\n"));
  return;
}


/* All of this should be moved into stvc_subr.c or something */

int
stvc_output(struct stpcb *sp, ushort OpFlags, short tid)
{
	struct  mbuf 	*m0;
	sthdr_t      	*sth;
	int		error = 0;
	ushort	opcode = OpFlags & ST_OPCODE_MASK;
	if_st_tx_desc_t *ptxdesc;
	struct  socket  *so    = stpcbtoso(sp);

	m0 = m_getclr(M_DONTWAIT, MT_HEADER);
	if (NULL == m0)
	  	return -1;

	m0->m_len = MSIZE;
	m0->m_next = NULL;
	m0->m_off = MMINOFF;

	if (sp->s_ifp == NULL) {
		if (error = st_findroute(sp->s_so))
			return error;
	}
	
	sth     = (sthdr_t *)         (mtod(m0, caddr_t) + 
				       sp->s_stifp->xmit_desc_sthdr_off);
	ptxdesc = (if_st_tx_desc_t *) (mtod(m0, caddr_t) +
				       sp->s_stifp->xmit_desc_ifhdr_off);

	ptxdesc->flags = IF_ST_TX_CTL_MSG;
	ptxdesc->vc = 0;
	if(error = stc_hdr_template(sp, (OpFlags|ST_INTERRUPT), sth, tid))  {
		dprintf(0, ("stc_hdr_template returned error\n"));
		return error;
	}	

#if 0
	dprintf(0, ("Setting VC timer; opcode %s \n", 
		st_decode_opcode(opcode))); 
	st_set_timer(sp, STPT_VC_TIMER, VC_TIMEOUT_VAL);
#endif

#ifdef	ST_HDR_DUMP
	dprintf(0, ("Dump header in stvc_output\n"));
#endif	/* ST_HDR_DUMP */
	ST_DUMP_HDR(sth);

	error = st_output(m0, sp->s_so);

	if(error)  {
		dprintf(10, ("Port %d failed to sent %s to Port %d, error=%d\n",
				sp->s_inp->inp_lport, 
				st_decode_opcode(opcode),
				sp->s_inp->inp_fport,
			        error));
	}
	else {
		dprintf(10, ("Port %d sent %s to Port %d\n",
				sp->s_inp->inp_lport, 
				st_decode_opcode(opcode),
				sp->s_inp->inp_fport));
	}

	return error;
}


int
st_data_ctl_output(struct stpcb *sp, ushort opcode, short tid)
{
	struct  mbuf 	*m0;
	sthdr_t      	*sth;
	int		error = 0;

	return(stvc_output(sp, opcode, tid));
}


int
stvc_input(struct stpcb *sp, ushort opFlags, struct mbuf *m) 
{
	struct socket *so = sp->s_so;
	struct inaddrpair *iap;
	int		error = 0;
	struct socket *so_old, *so_new;
	ushort opcode = opFlags & ST_OPCODE_MASK;

	sthdr_t *sth;

	dprintf(20, ("stvc_input, opcode 0x%x (%s)\n", 
		opcode, st_decode_opcode(opcode)));

 	so_old = sp->s_so;

	sth = &(mtod(m, struct st_io_s *)->sth);
#ifdef	ST_HDR_DUMP
	dprintf(0, ("Dump header in stvc_input\n"));
#endif	/* ST_HDR_DUMP */
	ST_DUMP_HDR(sth);

	iap = &(mtod(m, struct st_io_s *)->iap);
	ASSERT_ALWAYS(IS_VC_OP(opcode));

	dprintf(20, ("so 0x%x, sem 0x%x before stc_vc_fsm \n", 
		so, so->so_sem));

	if(stc_vc_fsm(&sp, opcode, sth, iap)) {
		dprintf(0, ("stc_vc_fsm returned "
			"bad status in stvc_input\n"));
		error = -1;
	}

	dprintf(20, ("so 0x%x (0x%x) , sem 0x%x (0x%x) after stc_vc_fsm \n", 
		sp->s_so, so, sp->s_so->so_sem, so->so_sem));


	if (sp != NULL) {
	  	/* stvc_detach has been called -- and thus, 
			stpcb is gone bye bye. */
	  	so_new = sp->s_so;
	  
	  	if(so_old != so_new)  {
		  	/* We do not INPCB_RELE the new socket as 
		     	the corresponding INPCB_RELE comes from
		     	PRU_DETACH */

		  	if(0 == (INPCB_RELE(sotoinpcb(so_new)))) {
		  		SOCKET_UNLOCK(so_new);
			}
	  	}
	  	else {
			if(0 == (INPCB_RELE(sotoinpcb(so))))  {
				SOCKET_UNLOCK(so);
			}
	  	}
	}

	return error;
}



int
stdata_input(struct stpcb *sp, ushort OpFlags, struct mbuf *m) 
{
	struct socket *so, *so1, *so2;
	int		error = 0;
	int		tid = -1;
	st_rx_t		*rx;
	sthdr_t 	*sth;
	ushort	opcode = OpFlags & ST_OPCODE_MASK;

	ASSERT_ALWAYS(SOCKET_ISLOCKED(sp->s_so));
	ASSERT_ALWAYS(IS_DATA_OP(opcode));
	ASSERT_ALWAYS(m != NULL);

	dprintf(30, ("stdata_input, sp 0x%x, opcode (%s), mbuf-len %d\n",
		sp, st_decode_opcode(opcode), m_length(m)));

 	so1 = so = sp->s_so;
	sth = &(mtod(m, struct st_io_s *)->sth);
#ifdef	ST_HDR_DUMP
	dprintf(0, ("Dump header in stdata_input\n"));
#endif	/* ST_HDR_DUMP */
	ST_DUMP_HDR(sth);

	switch (opcode) {
		default: {
			dprintf(0, ("Unknown DATA opcode in stdata_input\n"));
			return -1;
			/* NOTREACHED */
			break;
		}

		case ST_RTS:  {
			sthdr_rts_t	*rts = (sthdr_rts_t *) sth;
			st_tid_t	kid;
			st_kx_t		*kx;

			tid = 0;
			if(! rx_is_valid(sp, tid))  {
				kid = sp->s_kidalloc(sp);
				/* 1 outstanding kx for now */
				if(kid)  {
					dprintf(5, 
						("Got kid %d\n", kid));
				}
				if(kid != -1) {
					kx = &(sp->kx[kid]);
					bcopy(rts, &kx->saved_rts,
						sizeof(sthdr_rts_t));
					dprintf(30, ("Got an RTS len %d, Id %d \n", 
						rts->tlen, rts->I_id));
					if (kx->saved_rts.I_id == 0){
						st_dump_hdr(sth); 	/* really want a dump! */
						st_dump_pcb(sp);
						st_dump_tx(&(sp->tx[0]));
						st_dump_rx(&(sp->rx[0]));
						ASSERT_ALWAYS(0);
					}

					error = st_send_WAIT(
							sp, sth, tid);
					if(!error && 
						so->so_rcv.sb_flags 
						& SB_NOTIFY)  {
						dprintf(5, ("Waking up "
						"readers on KX\n"));
						sorwakeup(so, NETEVENT_STPUP);
						so->so_rcv.sb_flags &=
							~SB_SEL;
					}
				}
				else {
					dprintf(0, ("No k-id for %d in "
						"write() before read(); " 
						"Dropping RTS\n", tid));
					error = st_send_REJECT(
						sp, rts, 
						ST_RANSWER | ST_REJECT,
						tid);
				}
				if(error)  {
					dprintf(0, 
					("st_send_WAIT/REJECT: "
					"bad status for "
					"write-before-read\n"));
				}
				goto release_and_quit;
			}
			else {
				rx = &(sp->rx[tid]);
				rx->rx_tlen = rts->tlen;
				rx->rx_cts_len = rts->tlen;
				rx->rx_data_len = rts->tlen;
				dprintf(30, ("Got an RTS len %d \n", 
					rts->tlen));
			}
			break;
		}

		case ST_CTS:  {
			sthdr_cts_t *cts = &(sth->sth_cts);
	
			tid = find_tid_index(cts->I_id, sp, 
						MAX_TX_ENTRIES, TX);
			break;
		}
		case ST_RANSWER:  {
			sthdr_ra_t *ra = &(sth->sth_ra);

			tid = find_tid_index(ra->I_id, sp, 
						MAX_TX_ENTRIES, TX);
			dprintf(5, ("Got an RANSWER, tid %d\n", tid));
			break;
		}
		case ST_DATA:  {
			sthdr_data_t *data = &(sth->sth_data);

			tid = find_tid_index(data->R_id, sp, 
						MAX_RX_ENTRIES, RX);
			break;
		}
		case ST_RS:  {
			sthdr_rs1_t	*rs1 = &(sth->sth_rs1);
			sthdr_rs2_t	*rs2 = &(sth->sth_rs2);

			dprintf(10, ("Got an RS, R_id %d, I_id %d\n", 
				rs2->R_id, rs2->I_id));
			tid = 0;
			if(rs1->minus_one != (uint32_t) -1) {
				tid = find_tid_index(rs2->R_id, sp, 
						MAX_TX_ENTRIES, RX);
			}
			
			break;
		}
		case ST_RSR:  {
			sthdr_rsr1_t	*rsr1 = &(sth->sth_rsr1);
			sthdr_rsr2_t	*rsr2 = &(sth->sth_rsr2);


			dprintf(20, ("Got an RSR, R_id %d, I_id %d\n",
				rsr2->R_id, rsr2->I_id));
			tid = 0;
			if(rsr1->minus_one != (uint32_t) -1)  {
				tid = find_tid_index(rsr2->I_id, sp, 
						MAX_TX_ENTRIES, TX);
			}
			
			break;
		}
	}

	if(tid != 0)  {
		dprintf(0, ("WARN: bad tid; opcode %s, dropping\n",
			st_decode_opcode(opcode)));
		st_dump_hdr(sth); 	/* really want a dump! */
		st_dump_pcb(sp);
		st_dump_tx(&(sp->tx[0]));
		st_dump_rx(&(sp->rx[0]));
		error = -1;
		goto release_and_quit;
	}

	if(opcode == ST_DATA)  {
		dprintf(5, ("stdata_input: reducing mlen by st_io_s sz (%d)\n", 
			sizeof(struct st_io_s)));


		m->m_len -= sizeof(struct st_io_s);
		m->m_off += sizeof(struct st_io_s);

#if 0
		M_ADJ(m, sizeof(struct st_io_s));
#endif

		rx = &(sp->rx[tid]);

		rx->rx_buf.temp_mbuf = m;
	}

	if(st_data_fsm(&sp, OpFlags, sth, tid)) {
		dprintf(0, ("st_data_fsm returned "
			"bad status in stdata_input (opc %s)\n",
			st_decode_opcode(opcode)));
		error = -1;
	}

	so2 = so = sp->s_so;
	ASSERT_ALWAYS(so1 == so2);


release_and_quit:
	if(!INPCB_RELE(sotoinpcb(so)))
		SOCKET_UNLOCK(so);
	return error;
}

struct stpcb *
stvc_newconn(struct stpcb *sp_old, struct inaddrpair *iap) 
{
	struct socket *so_old, *so_new;
	struct inpcb  *inp_new, *inp_old;
	struct stpcb  *sp_new;

	so_old = sp_old->s_so;
	so_new = sonewconn(so_old, NULL);

	if (so_new == NULL) {
		if(!INPCB_RELE(sotoinpcb(so_old)))
			SOCKET_UNLOCK(so_old);
		return(NULL);
	}

	sp_new  = sotostpcb(so_new);
	inp_new = sotoinpcb(so_new);
	inp_old = sotoinpcb(so_old);


	ASSERT_ALWAYS(inp_old->inp_hashflags == INPFLAGS_LISTEN);

	/* We need to double check to make sure
	   we actually need these */
	/*	INHHEAD_LOCK(&stpcb_head); */
	INPCB_HOLD(inp_new);

	inp_new->inp_laddr = inp_old->inp_laddr;
	inp_new->inp_lport = inp_old->inp_lport;

	bcopy(&(sp_old->s_vcd), &(sp_new->s_vcd), sizeof(st_vcd_t));
	bcopy(&(sp_old->tx), &(sp_new->tx), 
			MAX_TX_ENTRIES * sizeof(st_tx_t));
	bcopy(&(sp_old->rx), &(sp_new->rx), 
			MAX_RX_ENTRIES * sizeof(st_rx_t));

	sp_new->s_flags = sp_old->s_flags;

	/*	INHHEAD_UNLOCK(&stpcb_head); */

#if 1
	if (INPCB_RELE(sotoinpcb(so_old)) == 0) {
		SOCKET_UNLOCK(so_old);
	}	
#endif
	{
		struct in_addr     laddr;
		struct sockaddr_in sin;


		laddr = inp_new->inp_laddr;
		sin.sin_family = AF_INET;
		sin.sin_addr = iap->iap_faddr;
		sin.sin_port = iap->iap_fport;

		/* Obtain rendevous port here */

		if (in_pcbsetaddrx(inp_new, &sin, laddr,
				   (struct inaddrpair *) 0)) {
			if(! INPCB_RELE(inp_new))
				SOCKET_UNLOCK(so_new);
			return(NULL);
		}
	}

	stpcb_init(sp_new);
	return(sp_new);

}

/* __inline does not work.  We may consider transforming this into
   macrodefs. */

void
stvc_isdisconnected(struct stpcb *sp) 
{
	ASSERT(sp);
	ASSERT(sp->s_so);
	st_cancel_timers(sp);
	soisdisconnected(sp->s_so);
	in_pcbdisconnect(sotoinpcb(sp->s_so));
	SOCKET_SBWAIT_WAKEALL(&sp->s_so->so_snd);
	SOCKET_SBWAIT_WAKEALL(&sp->s_so->so_rcv);
}

void
stvc_isdisconnecting(struct stpcb *sp) 
{
	ASSERT(sp);
	ASSERT(sp->s_so);
	soisdisconnecting(sp->s_so);
	st_set_timer(sp, STPT_VC_TIMER, VC_TIMEOUT_VAL);
}

void
stvc_isconnected(struct stpcb *sp) 
{
	ASSERT(sp);
	ASSERT(sp->s_so);
	st_cancel_timers(sp);
	soisconnected(sp->s_so);
}

void
stvc_isconnecting(struct stpcb *sp) 
{
	ASSERT(sp);
	ASSERT(sp->s_so);
	soisconnecting(sp->s_so);
}

int
stvc_bind(struct stpcb *sp, struct mbuf *nam) 
{
	int error;

	ASSERT(sp);
	ASSERT(sp->s_inp);
	ASSERT(nam);
	error = in_pcbbind(sp->s_inp, nam);
	sp->s_vcd.vc_lport = sp->s_inp->inp_lport;
	return(error);
}

int
stvc_listen(struct stpcb *sp) 
{
	struct inpcb *inp; 
	int error = 0;

	ASSERT(sp);

	inp = sp->s_inp;

	if (inp->inp_lport == 0)
		error = stvc_bind(sp, (struct mbuf *) 0);
	
	if (error == 0) {
		in_pcblisten(sp->s_inp);
		error = stc_vc_fsm(&sp, ST_VOP_ULISTEN, NULL, NULL);
	}
	return(error);
}

int
stvc_unlisten(struct stpcb *sp) 
{
	int error = 0;

	ASSERT(sp);


	in_pcbunlisten(sp->s_inp);
	error = stc_vc_fsm(&sp, ST_VOP_ULISTEN, NULL, NULL);
	return(error);
}


int
stvc_connect(struct stpcb *sp, struct mbuf *nam) 
{

extern	uint	SHAC_bufx_lows[], SHAC_bufx_highs[];

	struct inpcb *inp;
	int error = 0;

	ASSERT(sp);

	inp = sp->s_inp;	

	if (inp->inp_lport == 0) {
		error = stvc_bind(sp, (struct mbuf *) 0);
		if (error)
			return(error);
	}

	if (inp->inp_hashflags & INPFLAGS_LISTEN) {
		error = stvc_unlisten(sp);
		if (error) 
			return(error);
	}

	if (error = in_pcbconnect(sp->s_inp, nam)) 
		return(error);
	
	sp->s_vcd.vc_rport = sp->s_inp->inp_fport;

	if (error = st_findroute(sp->s_so)) 
		return(error);

	soisconnecting(sp->s_so);

	if (sp->s_keyalloc(sp, (ushort) ST_RCONNECT)) {
		cmn_err(CE_PANIC,
			"Cannot allocate key in ST_RCONNECT\n");
	}


	if (sp->s_stifp->if_st_clear_port) {
	  error = (*sp->s_stifp->if_st_clear_port)(sp->s_ifp,
						   sp->s_vcd.vc_lport);
	  if (error) 
	    return error;
	}	  


	if (sp->s_stifp->if_st_set_port) {
		st_port_t 	st_port;
		uint		bufsize = (sp->s_vcd.vc_lbufsize)? 
				sp->s_vcd.vc_lbufsize : ST_LOG_BUFSZ;
		u_char		spray = (sp->tx[0].tx_spray_width) ?
					(sp->tx[0].tx_spray_width) : 1;
		uint		num_bufxes;

		/* TODO: Find correct values for this */

		bzero(&st_port, sizeof(st_port_t));
		st_port.bufx_base    = SHAC_bufx_lows[spray];
		num_bufxes = SHAC_bufx_highs[spray] - st_port.bufx_base + 1;
		/* need to mention the # of bufxes we need, not the
		*  range that is valid bufxes */
		st_port.bufx_range  = num_bufxes;
		dprintf(30, ("set_port: bufx_range set to (0x%x), "
			" lo 0x%x, hi 0x%x, num %d, spray %d\n",
			st_port.bufx_range, st_port.bufx_base, 
			SHAC_bufx_highs[spray], num_bufxes, spray));
		st_port.src_bufsize = log2(spray) + bufsize;
		dprintf(10, ("bufsize set to %u (log %u) in set port\n",
			st_port.src_bufsize, 
			(1 << st_port.src_bufsize)));

		/* make sure this code follows the code in stc_vc_fsm */
		if ( sp->s_flags & STP_SF_BYPASS ) {
		    	st_port.vc_fifo_credit[0] = VC0_FIFO_CREDITS;
		    	st_port.vc_fifo_credit[1] = VC1_FIFO_CREDITS;
		    	st_port.vc_fifo_credit[2] = VC2_FIFO_CREDITS;
		    	st_port.vc_fifo_credit[3] = VC3_FIFO_CREDITS;
		    	if ( sp->s_flags & STP_SF_USERSLOTS ) {
				st_port.ddq_size = 128 * sp->s_vcd.vc_max_lslots;
				sp->s_vcd.vc_true_max_lslots = sp->s_vcd.vc_max_lslots;
		    	} else {
				st_port.ddq_size = 128 * ST_DDQ_NUM_SLOTS;
				sp->s_vcd.vc_true_max_lslots = ST_DDQ_NUM_SLOTS;
		    	}
		    	st_port.ddq_addr = NULL;
		    	st_port.key = sp->s_vcd.vc_lkey;
		} 
		else {
		    	st_port.vc_fifo_credit[0] = VC0_FIFO_CREDITS;
		    	st_port.vc_fifo_credit[1] = VC1_FIFO_CREDITS;
		    	st_port.vc_fifo_credit[2] = VC2_FIFO_CREDITS;
		    	st_port.vc_fifo_credit[3] = VC3_FIFO_CREDITS;
			ASSERT_ALWAYS(sp->s_vcd.vc_max_lslots % 16);
			/** CHANGE THIS 
		    	st_port.ddq_size = 
					128 * sp->s_vcd.vc_max_lslots;
			sp->s_vcd.vc_true_max_lslots = sp->s_vcd.vc_max_lslots;
			**/
		    	st_port.ddq_size = 
					128 * ST_DDQ_NUM_SLOTS;
			sp->s_vcd.vc_true_max_lslots = ST_DDQ_NUM_SLOTS;
		    	st_port.ddq_addr = NULL;
		    	st_port.key = sp->s_vcd.vc_lkey;
		}

		error = (*sp->s_stifp->if_st_set_port)(sp->s_ifp,
				       sp->s_vcd.vc_lport, &st_port);
		if (error) 
			return(error);
	}

	error = stc_vc_fsm(&sp, ST_RCONNECT, NULL, NULL);

	return(error);
}




void
stpcb_init(struct stpcb *sp)
{
	/* initialize the structures of a new stpcb */
	sp->s_iidalloc = alloc_zero_iid;
	sp->s_iidfree = dealloc_zero_iid;
	sp->s_ridalloc = alloc_zero_rid;
	sp->s_ridfree = dealloc_zero_rid;
	sp->s_kidalloc = alloc_zero_kid;
	sp->s_kidfree = dealloc_zero_kid;
	sp->s_keyalloc = alloc_key;
	sp->s_R_Mx_alloc = alloc_R_Mx;
	sp->s_R_Mx_free = dealloc_R_Mx;
	sv_init(&(sp->s_vcd.slot_sync), SV_FIFO, "ST-SLOTS");
	sp->s_vcd.asked_for_slots = 0;
	sp->s_vcd.vc_max_lslots = sp->s_vcd.vc_max_lslots ?
			sp->s_vcd.vc_max_lslots : ST_DEFAULT_NUM_SLOTS;
}



/* tell other side how many slots are left */
int
st_sync_up(struct stpcb *sp, sthdr_rs1_t *rs1)
{
	struct	mbuf	*m0;
	int		error = 0;
	if_st_tx_desc_t	*ptxdesc;
	sthdr_rsr1_t	*rsr1;

	m0 = m_getclr(M_DONTWAIT, MT_HEADER);
	if (NULL == m0) {
		return (-1);
	}

	m0->m_len  = sizeof(struct st_io_s);
	m0->m_next = NULL;
	m0->m_off  = MMINOFF;

	rsr1 = (sthdr_rsr1_t *) (mtod(m0, caddr_t) +
				sp->s_stifp->xmit_desc_sthdr_off);

	ptxdesc = (if_st_tx_desc_t *) (mtod(m0, caddr_t) +
				sp->s_stifp->xmit_desc_ifhdr_off);

	rsr1->Sync = rs1->Sync;

	if(error = st_rs_template(sp, ST_RSR, 
				(sthdr_t *) rsr1, ST_STATE_SLOTS))  {
		dprintf(0, ("st_rs_template erred in st_sync_up\n"));
		return error;
	}

	ptxdesc->flags = IF_ST_TX_CTL_MSG;
	ptxdesc->vc = 0;


#ifdef	ST_HDR_DUMP
	dprintf(0, ("Dump header in st_sync_up\n"));
#endif 	/* ST_HDR_DUMP */
	ST_DUMP_HDR((sthdr_t *) rsr1);

	/* don't need a check for rslots here */
	if(error = st_output(m0, sp->s_so))  {
		dprintf(0, ("Port %d failed to sent RSR-SLOTS to Port %d\n",
				sp->s_inp->inp_lport, 
				sp->s_inp->inp_fport));
	}
	else {
		dprintf(30, ("Port %d sent ST_RSR-SLOTS to Port %d\n",
				sp->s_inp->inp_lport, 
				sp->s_inp->inp_fport));

	}

	return error;
}


/* get info about a particular block */
int
st_ask_for_block_info(struct stpcb *sp, uint32_t bnum, st_tid_t tid)
{
	struct	mbuf	*m0;
	int		error = 0;
	if_st_tx_desc_t	*ptxdesc;
	sthdr_rs3_t	*rs3;
	st_tx_t		*tx;

	m0 = m_getclr(M_DONTWAIT, MT_HEADER);
	if (NULL == m0) {
		return (-1);
	}

	m0->m_len  = sizeof(struct st_io_s);
	m0->m_next = NULL;
	m0->m_off  = MMINOFF;

	rs3 = (sthdr_rs3_t *) (mtod(m0, caddr_t) +
				sp->s_stifp->xmit_desc_sthdr_off);

	ptxdesc = (if_st_tx_desc_t *) (mtod(m0, caddr_t) +
				sp->s_stifp->xmit_desc_ifhdr_off);


	if(error = st_rs_template(sp, ST_RS, 
				(sthdr_t *) rs3, ST_STATE_BLOCK))  {
		dprintf(0, ("st_rs_template erred in st_ask_for_block_info\n"));
		return error;
	}

	rs3->B_num = bnum;
	ASSERT(0 == tid);
	tx = &(sp->tx[tid]);
	rs3->I_id = tx->tx_iid;
	rs3->R_id = tx->tx_rid;

	ptxdesc->flags = IF_ST_TX_CTL_MSG;
	ptxdesc->vc = 0;


#ifdef	ST_HDR_DUMP
	dprintf(0, ("Dump header in st_ask_for_block_info\n"));
#endif 	/* ST_HDR_DUMP */
	ST_DUMP_HDR((sthdr_t *) rs3);

	/* don't need a check for rslots here */
	if(error = st_output(m0, sp->s_so))  {
		dprintf(0, ("Port %d failed to sent RS-BLK to Port %d\n",
				sp->s_inp->inp_lport, 
				sp->s_inp->inp_fport));
	}
	else {
		dprintf(30, ("Port %d sent ST_RS-BLOCK to Port %d\n",
				sp->s_inp->inp_lport, 
				sp->s_inp->inp_fport));

	}

	return error;
}


/* tell other side whether particular block reached ok */
int
st_ack_a_block(struct stpcb *sp, st_tid_t tid, sthdr_rs3_t *rs3)
{
	struct	mbuf	*m0;
	int		error = 0;
	if_st_tx_desc_t	*ptxdesc;
	sthdr_rsr3_t	*rsr3;
	st_rx_t		*rx = &(sp->rx[tid]);;

	m0 = m_getclr(M_DONTWAIT, MT_HEADER);
	if (NULL == m0) {
		return (-1);
	}

	m0->m_len  = sizeof(struct st_io_s);
	m0->m_next = NULL;
	m0->m_off  = MMINOFF;

	rsr3 = (sthdr_rsr3_t *) (mtod(m0, caddr_t) +
				sp->s_stifp->xmit_desc_sthdr_off);

	ptxdesc = (if_st_tx_desc_t *) (mtod(m0, caddr_t) +
				sp->s_stifp->xmit_desc_ifhdr_off);


	rsr3->Sync = rs3->Sync;
	rsr3->B_num = -1;
	if(IS_SET_BNUM(rx, rs3->B_num))  {
		rsr3->B_num = rs3->B_num;
	}
	rsr3->B_seq = get_max_set_bnum(rx);
	dprintf(10, ("RSR3: B_seq is %d, B_num %d\n",
		rsr3->B_seq, rsr3->B_num));
	/* st_dump_bnum_tab(rx); */
	rsr3->I_id = rx->rx_iid;
	rsr3->R_id = rx->rx_rid;

	if(error = st_rs_template(sp, ST_RSR, 
				(sthdr_t *) rsr3, ST_STATE_BLOCK))  {
		dprintf(0, ("st_rs_template erred in st_ack_a_block\n"));
		return error;
	}

	ptxdesc->flags = IF_ST_TX_CTL_MSG;
	ptxdesc->vc = 0;


#ifdef	ST_HDR_DUMP
	dprintf(0, ("Dump header in st_ack_a_block\n"));
#endif 	/* ST_HDR_DUMP */
	ST_DUMP_HDR((sthdr_t *) rsr3);

	/* don't need a check for rslots here */
	if(error = st_output(m0, sp->s_so))  {
		dprintf(0, ("Port %d failed to sent RSR-BLOCK to Port %d\n",
				sp->s_inp->inp_lport, 
				sp->s_inp->inp_fport));
	}
	else {
		dprintf(30, ("Port %d sent ST_RSR-BLOCK to Port %d\n",
				sp->s_inp->inp_lport, 
				sp->s_inp->inp_fport));

	}

	return error;
}



int
st_process_RSR(struct stpcb *sp, sthdr_t *sth)
{
	sthdr_rsr1_t	*rsr1 =  &(sth->sth_rsr1);
	sthdr_rsr2_t	*rsr2 =  &(sth->sth_rsr2);
	sthdr_rsr3_t	*rsr3 =  &(sth->sth_rsr3);
	uchar_t		type;
	int		error = 0;

	if(rsr1->minus_one == (uint32_t) -1)  {
		type = ST_STATE_SLOTS;
		dprintf(15, ("Got RSR, type ST_STATE_SLOTS\n"));
	}
	else if(rsr2->minus_one == (uint32_t) -1)  {
		type = ST_STATE_XFER;
		dprintf(15, ("Got RSR, type ST_STATE_XFER\n"));
	}
	else {
		type = ST_STATE_BLOCK;
	}

	/* CHECK_SLOTS(sp); */
	if(! GOOD_SLOTS(sp))  {
		SLOTS_PANIC(sp);
	}

	if(SLOTS_ENABLED(sp))  {
		dprintf(10, ("RSR: vc_rslots was %d, max %d, in %d\n",
			sp->s_vcd.vc_rslots, sp->s_vcd.vc_max_rslots,
			rsr1->R_Slots));
		sp->s_vcd.vc_rslots += rsr1->R_Slots;
		ASSERT_ALWAYS(rsr1->R_Slots <= sp->s_vcd.vc_max_rslots);

		dprintf(10, ("RSR: rslots inc to %d, max %d\n", 
			sp->s_vcd.vc_rslots, sp->s_vcd.vc_max_rslots));
		/* ASSERT_ALWAYS(sp->s_vcd.vc_rslots <= 
					sp->s_vcd.vc_max_rslots); */
		if(sp->s_vcd.vc_rslots > sp->s_vcd.vc_max_rslots)  {
			cmn_err(CE_PANIC, "Too many slots; got %d "
				"total %d, max %d\n",
				rsr1->R_Slots, sp->s_vcd.vc_rslots,
				sp->s_vcd.vc_max_rslots);
		}
	}

	switch(type) {
		default: {
			cmn_err(CE_PANIC, "Unknown type of rsr received\n");
			break;
		}
		case ST_STATE_SLOTS: {
			dprintf(10, ("Got %d slots in SLOTS RSR\n",
				rsr1->R_Slots));
			if(SLOTS_AVAILBABLE(sp)) {
				dprintf(10, ("No process waiting for slots\n"));
			}
			break;
		}
		case ST_STATE_XFER: {
			st_tid_t	iid;
			st_tx_t		*tx;

			iid = find_tid_index(rsr2->I_id, sp, 
						MAX_TX_ENTRIES, TX);
			ASSERT_ALWAYS(iid == 0);
			tx = &(sp->tx[iid]);
			sp->last_RSR_R_id_recvd = rsr2->R_id;
			sp->last_RSR_I_id_recvd = rsr2->I_id;
			dprintf(20, ("state xfer RSR, sync %d\n",
				rsr2->Sync));
			error = st_retire_write(sp, sth, iid);
			break;
		}
		case ST_STATE_BLOCK: {
			break;
		}
	}

	return error;
}

int
st_send_CTS(struct stpcb *sp, int tid, uint16_t mx)
{
	st_rx_t		*rx;
	int		error = 0;
	int		len_ctsed;

	rx = &(sp->rx[tid]);

	ASSERT_ALWAYS(SOCKET_ISLOCKED(sp->s_so));
	ASSERT(MX_IS_SET(st_R_Mx_tab, mx));

	/* THIS DOES NOT ACCOUNT FOR FIRST BLOCK BEING PARTIAL */
	len_ctsed = min((1 << rx->rx_blocksize), rx->rx_cts_len);
	rx->rx_cts_len -= len_ctsed;

	ASSERT(sp->s_stifp);
	ASSERT(sp->s_stifp->if_st_set_mx);

	dprintf(30, ("CTS-ing %d bytes on CTS num %d len remaining: %u\n",
		     len_ctsed, rx->cur_bnum, rx->rx_cts_len));

	/* MAYBE TODO: remove rx_mx_tmp and rewrite st_data_ctl_output to take
	 * a partially formated ST header - which is later filled in by 
	 * the template function.
	 */
	rx->rx_mx_tmp = mx;

	if(!(error = st_data_ctl_output(sp, ST_CTS, tid))) {
		/* This has a potential race if the data comes back before we
		 * change the state to STP_DATA_RECV. (not likely!)
		 */
		rx->rx_state = STP_DATA_RECV;
		st_set_timer(sp, RID_TO_TIMER_ID(tid),
			     RX_TIMEOUT_VAL);
	}
	else {
		cmn_err(CE_WARN, "Could not send CTS for blk %u\n", rx->cur_bnum);
	}
	

	return error;
}


int
st_respond_to_RTS(struct stpcb *sp, sthdr_t *sth, int tid)
{
#define	RDY_TO_CTS(sp, sth)	1  /* hack!!! */
#define	DEFER_CTS(sp, sth)	0  /* hack!!! */

	sthdr_rts_t 	*rts = &(sth->sth_rts);
	st_rx_t		*rx = NULL;
	uint		num_cts_req;
	uint		num_cts_sent;
	int		error = 0;

	ASSERT(rx_is_valid(sp, tid));
	ASSERT(sp->s_vcd.vc_lport == rts->R_Port);
	ASSERT(sp->s_vcd.vc_rport ==  rts->I_Port);

	rx = &(sp->rx[tid]);

 	num_cts_req = (rts->CTS_req)? rts->CTS_req 
					: ST_NUM_CTS_OUTSTANDING;
	dprintf(30, ("Got RTS; Rport %d, Iport %d, id %d, len %d, CTS_req %d, Max_Block %d\n",
		rts->R_Port, rts->I_Port,
		rts->I_id, rts->tlen, rts->CTS_req, rts->Max_Block));
	
	if(rts->tlen > rx->rx_buf.payload_len)  {
		dprintf(10, ("RTS-len (%u) > read-len (%u); state %s; rejecting\n",
			rts->tlen, rx->rx_buf.payload_len,
			st_decode_state(rx->rx_state)));
		error = st_send_REJECT(sp, rts, 
					ST_RANSWER | ST_REJECT, tid);
	}
	else {
		rx->rx_iid = rts->I_id;
		rx->cur_bnum = 0;
	
		rx->rx_tlen = rts->tlen;
		rx->rx_cts_len = rts->tlen;
		rx->rx_data_len = rts->tlen;

		rx->rx_blocksize = sp->blocksize ? sp->blocksize : BLOCK_SIZE;
		rx->rx_blocksize = min(rts->Max_Block, rx->rx_blocksize);

		rx->rx_flags = (rts->OpFlags & ST_SENDSTATE) ?
			(rx->rx_flags | RX_RTS_SENDSTATE) :
			rx->rx_flags;

		if(RDY_TO_CTS(sp, sth))  {  /* send a CTS or RA? */
			dprintf(10, ("st_respond_to_RTS: need to set bufx/off/F_off\n"));
			dprintf(10, ("st_respond_to_RTS: rx_cts_len = %d, rx_data_len = %d\n", 
				     rx->rx_cts_len, rx->rx_data_len));

			num_cts_req = min(rts->CTS_req, rx->rx_num_mx);

			dprintf(10, ("\tCTS_req = %d, rx_num_mx = %d\n",
				     rts->CTS_req, rx->rx_num_mx));

			/* protect against other end setting CTS_req == 0 */
			if (num_cts_req == 0)
				num_cts_req = 1;

			/* Send CTS's for all Mx's available */
			num_cts_sent = 0;
			while(rx->rx_cts_len && num_cts_sent < num_cts_req) {
				error = st_send_CTS(sp, tid, rx->rx_mx[num_cts_sent]);
				if (error)
					break;
				num_cts_sent++;
			}
			dprintf(30, ("Done sending %d CTSes\n", num_cts_sent));
		}
		else {
			if ( !(error = stvc_output(sp, ST_RANSWER, tid)))
				rx->rx_state = STP_RTS_PINNED;
		}
	}

	return error;
}



int
st_process_DATA(struct stpcb *sp, sthdr_t *sth, int tid, 
						struct mbuf *payload)
{
	sthdr_data_t	*data = &(sth->sth_data);
	uint		payload_len;
	st_rx_t		*rx;
	struct	socket 	*so;
	struct mbuf	*m0;
	int		error = 0;
	uint		blocksize;
	int		last_recorded_bnum;
	uint		bsize_off;
	
	rx = &(sp->rx[tid]);
	ASSERT_ALWAYS(rx->rx_buf.bufx_flags & BUF_BUFX);
	ASSERT_ALWAYS(NULL != rx->rx_buf.test_bufaddr);
	ASSERT_ALWAYS(SOCKET_ISLOCKED(stpcbtoso(sp)));

	blocksize = rx->rx_blocksize;
	bsize_off = data->B_num * (1 << blocksize);

	if(sp->s_stifp == NULL)  {
		printf("st_ifnet is NULL\n");
		return EPROTONOSUPPORT;
	}
	payload_len = min((rx->rx_tlen - bsize_off), (1 << blocksize));

	dprintf(30, ("Got data bnum %d, sz %d\n", 
		data->B_num, payload_len));

	STSTAT_ADD(stps_datarxtotal, payload_len);
	dprintf(30, ("pending payload len reduced from %d by %d\n",
		rx->rx_data_len, payload_len));
	rx->rx_data_len -= payload_len;
	so = stpcbtoso(sp);
	so->so_rcv.sb_cc += payload_len;
	dprintf(30, ("so_rcv.sb_cc is %d\n", so->so_rcv.sb_cc));
	ASSERT_ALWAYS((int) rx->rx_data_len >= 0);

	SET_BNUM_TAB(rx, data->B_num);

	last_recorded_bnum = -1;
	if(rx->hdr_for_RSR)  {
		sthdr_t		*saved_sth = mtod(
					rx->hdr_for_RSR, sthdr_t *);
		sthdr_data_t	*saved_data = &(saved_sth->sth_data);
		last_recorded_bnum = saved_data->B_num;
	}
	else {
		MGET(m0, M_DONTWAIT, MT_HEADER);
		if (NULL == m0) {
			cmn_err(CE_PANIC, 
			"Run out of mbufs to store ST hdr\n");
		}
		m0->m_len  = sizeof(struct st_io_s);
		m0->m_next = NULL;
		m0->m_off  = MMINOFF;
		rx->hdr_for_RSR = m0;
	}
	
	if((int) data->B_num > last_recorded_bnum)  {
		bcopy(sth, mtod(rx->hdr_for_RSR, caddr_t), 
						sizeof(struct st_io_s));
	}
	if(rx->rx_data_len == 0)  {
		/* sorwakeup(so, NETEVENT_STPPUP); */
		SOCKET_SBWAIT_WAKEALL(&so->so_rcv);
	}
	else if (rx->rx_cts_len) {
		dprintf(20, ("rx->rx_data_len=%u, rx-buf-payload_len %u, max-bnum %u, "
			     "blksz %u; CTS-ing\n",
			     rx->rx_data_len, rx->rx_buf.payload_len, rx->cur_bnum, blocksize));
		error = st_send_CTS(sp, tid, data->R_Mx);
	}

	if((data->OpFlags & ST_FLAG_MASK & ST_SENDSTATE) || 
			(0 == rx->rx_data_len && 
				rx->rx_flags & RX_RTS_SENDSTATE))  {
		/* last block? st_sorecv ack */
		dprintf(10, ("st_ack_last_recv from process data "
			"Bnum %d, sync %d\n",
			data->B_num, data->Sync));
		/* CHECK: possible interop problem */
		ASSERT_ALWAYS(rx->hdr_for_RSR);
		error = st_ack_last_recv(sp, tid, data->Sync);
	}


	m_freem(payload);
	return error;
}


int
st_send_WAIT(struct stpcb *sp, sthdr_t *sth, int tid)
{
	sthdr_ra_t	*ra = &(sth->sth_ra);
	int		error = 0;

	/* STSTAT(stps_txwaits); */
	
	ra->I_Key = sp->s_vcd.vc_rkey;
	if(error = st_data_ctl_output(sp, ST_RANSWER, tid)) {
		cmn_err(CE_WARN, "Could not send RANSWER of RTS\n");
	}

	return error;
}


int
st_setup_buf(struct stpcb *sp, st_buf_t *buf_ptr, uio_t	*uio,
					u_char xfer_dir, st_tid_t tid)
{
	static		uchar_t		out_vcnum = 0;

	st_ifnet_t      *stifp = (st_ifnet_t *) sp->s_stifp;
	int		error = 0, i;
	paddr_t		*pbuf = NULL;
	int		dma_direction;		
	int		num_frags;
	uint		spray_width;
	uint		buflen, log_bufsz = 0;
	st_rx_t		*rx;
	st_tx_t		*tx;
	uint		len = uio->uio_resid;
	opaque_t	*dma_cookies = NULL;
	char		side[5];
	alenlist_t	alen = NULL;
	alenaddr_t	addr;
	size_t		length;
	size_t		bufsize = (sp->s_vcd.vc_lbufsize)? 
				(1 << sp->s_vcd.vc_lbufsize) : 
					(1 << ST_LOG_BUFSZ);

	dprintf(30, ("st_setup_buf: len %u, uio 0x%x, xfer_dir 0x%x\n",
		len, uio, xfer_dir));
	ASSERT_ALWAYS(buf_ptr->bufx_flags == BUF_NONE);
	if(xfer_dir == ST_BUFX_ALLOW_RECV)  {
		rx = &(sp->rx[tid]);
		dma_direction = B_WRITE;
		dprintf(10, ("setup_buf: rx spray is %d at 0x%x\n", 
			rx->rx_spray_width,
			&(rx->rx_spray_width)));
		spray_width = (rx->rx_spray_width)?
				rx->rx_spray_width : 1;
		sprintf(side, "%s", "dest  ");
	}
	else if(xfer_dir == ST_BUFX_ALLOW_SEND)  {
		tx = &(sp->tx[tid]);
		dma_direction = B_READ;
		dprintf(10, ("setup_buf: tx spray is %d\n", 
			tx->tx_spray_width));
		spray_width = (tx->tx_spray_width) ?
				tx->tx_spray_width : 1;
		sprintf(side, "%s", "src  ");
	}
	else {
		dprintf(0, ("Unknown xfer direction in setup_buf\n"));
		error = EINVAL;
		goto quit_setup_buf;
	}

	buf_ptr->payload_len = len;
	/* buf_ptr->st_addr_bufx.bufaddr = addr; */
	buf_ptr->test_bufaddr = uio->uio_iov->iov_base;
	dprintf(20, ("addr are: bufaddr 0x%x, test_bufaddr 0x%x\n",
		buf_ptr->st_addr_bufx.bufaddr, 
		buf_ptr->test_bufaddr));


	alen = alenlist_create(AL_LEAVE_CURSOR);
	ASSERT(NULL != alen);
	dma_cookies = kmem_zalloc(uio->uio_iovcnt * sizeof(opaque_t),
							KM_SLEEP);
	buflen = uio_to_frag_size(
		uio->uio_iov, uio->uio_iovcnt, &alen, 
					dma_direction, dma_cookies);
	if(-1 == buflen) {
		dprintf(0, ("%s: socket has bad user "
			"buffer's fragsize (%d)\n", 
			side, bufsize, buflen));
		/* undma already done */
		ASSERT(dma_cookies);
		kmem_free(dma_cookies, 
				uio->uio_iovcnt * sizeof(opaque_t));
		dma_cookies = NULL;
		error = EINVAL;
		goto quit_setup_buf;
	}

	dprintf(30, ("%s: uio fraglen is %u, bufsize %d\n", 
		side, buflen, bufsize));
	if(buflen < bufsize)  {
		/** see PV 666465 **/
		dprintf(0, ("%s: socket port bufsize (%d) larger than user "
			"buffer's fragsize (%d)\n", 
			side, bufsize, buflen));
		ASSERT_ALWAYS(alen);
		print_uio_frags(&alen);
		error = EINVAL;
		goto quit_setup_buf;
	}
	else {
		buflen = bufsize;
	}

	ASSERT(buflen >= NBPP);
	log_bufsz = log2(buflen);

	dprintf(30, ("%s: bufsz is %u, log is %u\n", 
		side, buflen, log_bufsz));
	
	if(xfer_dir == ST_BUFX_ALLOW_RECV)  {
		rx->rx_local_bufsize = log_bufsz;
		if(len/(1 << log_bufsz)  > 8 * BNUM_TAB_SIZE)  {
			dprintf(0, ("rx: receive buffer size too large "
				"for BNUM-table\n"));
			dprintf(0, ("Need %d entries, have %d (bufsz 0x%x)\n",
				len/(1 << log_bufsz),
				8 * BNUM_TAB_SIZE,
				(1 << log_bufsz)));
			error = ENOMEM;
			goto quit_setup_buf;
		}
	}
	else {
		tx->tx_local_bufsize = log_bufsz;
		if(len/(1 << log_bufsz)  > 8 * BNUM_TAB_SIZE)  {
			dprintf(0, ("tx: send buffer size too large "
				"for BNUM-tab (at O2K receiver)\n"));
			dprintf(0, ("Need %d entries, have %d (bufsz 0x%x)\n",
				len/(1 << log_bufsz),
				8 * BNUM_TAB_SIZE,
				(1 << log_bufsz)));
			error = ENOMEM;
			goto quit_setup_buf;
		}
		tx->tx_max_Bnum_required = len/(1 << log_bufsz) - 1;
		if(len & ~((1 << log_bufsz) - 1)) {
			tx->tx_max_Bnum_required++;
		}
		dprintf(10, ("Tx: len %d, lgbufsz %d, max bnum %d\n",
			len, log_bufsz, tx->tx_max_Bnum_required));
	}


	ASSERT_ALWAYS(ALENLIST_SUCCESS == alenlist_cursor_init(
							alen, 0, NULL));
	num_frags = 0;
	while(ALENLIST_SUCCESS == (alenlist_get(alen, NULL, 0,
						&addr, &length, 0))) {
		if(length % buflen 
				|| ((buflen-1) & addr))  {
			num_frags += length/buflen + 1;
		}
		else {
			num_frags += length/buflen;
		}
		dprintf(20, ("paddr 0x%x, len %u, num_frags made %u\n",
			addr, length, num_frags));
	}
	buf_ptr->num_bufx = num_frags;

	dprintf(15, ("%s Using %d bufxes, size %u\n", 
		side, buf_ptr->num_bufx, buflen));

	if(error = st_bufx_alloc(
			&(buf_ptr->st_addr_bufx.bufx_t.bufx), 
			buf_ptr->num_bufx, buflen,
			&sp->s_ifp, 1, spray_width, 
					&(buf_ptr->bufx_cookie))) {
		dprintf(0, ("st_bufx_alloc failed!\n"));
		error = ENOMEM;
		goto quit_setup_buf;
	}
	buf_ptr->bufx_flags |= BUF_BUFX;
	
	dprintf(20, ("bufx 0x%x, nbufs %u, cookie %u in setup\n",
		buf_ptr->st_addr_bufx.bufx_t.bufx,
		buf_ptr->num_bufx,
		buf_ptr->bufx_cookie));

	if(error = st_bufx_map(
			buf_ptr->st_addr_bufx.bufx_t.bufx,
				uio->uio_iov, uio->uio_iovcnt, 
				buf_ptr->bufx_cookie, dma_direction,
					dma_cookies, buflen, &alen)) {
		cmn_err(CE_WARN, "st_bufx_map error %d\n", error);
		error = EFAULT;
		goto quit_setup_buf;
	}

	/* ASSERT(0); */

	
	buf_ptr->bufx_flags |= BUF_MAPPED;

	num_frags = st_bufx_to_nfrags(buf_ptr->st_addr_bufx.bufx_t.bufx,
					buf_ptr->bufx_cookie);
	dprintf(20, ("num_frags is %d\n", num_frags));
	ASSERT(num_frags);
	if (spray_width && (num_frags % spray_width)) {
		cmn_err(CE_WARN, "spray width %d doesn't match with "
			"the %d phys-frags (len %u) obtained\n",
			spray_width, num_frags, len);
		error = EINVAL;
		goto quit_setup_buf;
	}

	pbuf = kmem_zalloc(num_frags * sizeof(paddr_t *), KM_SLEEP);
	ASSERT(pbuf);
	if(error = (st_bufx_to_phys(pbuf, 
				buf_ptr->st_addr_bufx.bufx_t.bufx,
					buf_ptr->bufx_cookie)))  {
		dprintf(0, ("Bad status %u from st_bufx_to_phys\n",
			error));
		if(st_bufx_free(buf_ptr->st_addr_bufx.bufx_t.bufx,
				buf_ptr->num_bufx, &sp->s_ifp, 1, 
					buf_ptr->bufx_cookie))   {
			cmn_err(CE_PANIC, 
			"st_bufx_free error in bad-map case!\n");
		}
		error = EFAULT;
		goto quit_setup_buf;
	}

	

#if 0
	printf("vaddr frags are:\n");
	print_btop(buf_ptr->st_addr_bufx.bufx_t.bufx, 
						buf_ptr->bufx_cookie);
#endif  /* 0 */

	buflen = st_bufx_to_frag_size(buf_ptr->st_addr_bufx.bufx_t.bufx,
					buf_ptr->bufx_cookie);

	if(NBPP < buflen)  {
		dprintf(30, ("set_buf: buflen (%u) > NBPP\n", buflen));
	}


	buf_ptr->st_addr_bufx.bufx_t.offset = 
		((buflen - 1) & ((__psint_t) uio->uio_iov->iov_base));
	if(buf_ptr->st_addr_bufx.bufx_t.offset)  {
		dprintf(10, ("%s: buflen 0x%x; base 0x%x, off 0x%x\n",
			side, buflen, uio->uio_iov->iov_base,
			buf_ptr->st_addr_bufx.bufx_t.offset));
		
	}

	dprintf(10, ("%s: Alloced %d bufxes, start 0x%x fragsz %u\n", 
		side, buf_ptr->num_bufx, 
		buf_ptr->st_addr_bufx.bufx_t.bufx, buflen));

	dprintf(30, ("%s payload 0x%x mapped; bufx %u, off %u (0x%x)\n",
		side, buf_ptr->test_bufaddr, 
		buf_ptr->st_addr_bufx.bufx_t.bufx,
		buf_ptr->st_addr_bufx.bufx_t.offset,
		buf_ptr->st_addr_bufx.bufx_t.offset));

	

	dprintf(30, ("Before mx: bufx is 0x%x\n",
		buf_ptr->st_addr_bufx.bufx_t.bufx));
	

	if(xfer_dir == ST_BUFX_ALLOW_RECV) {
		st_mx_t		*mx = &rx->rx_mx_template;

		mx->base_spray 	= spray_width - 1;
		mx->bufsize	= log2(spray_width) + log_bufsz;
		mx->bufx_base   = buf_ptr->st_addr_bufx.bufx_t.bufx;
		mx->key         = sp->s_vcd.vc_lkey;
		mx->bufx_range	= num_frags;

		mx->flags 	= 0;
		mx->poison	= 0;
		mx->port	= sp->s_vcd.vc_lport;
		mx->stu_num	= 0;

		/* init of Mx in shac */
		for(i = 0; i < rx->rx_num_mx; i++) {
			if ((*stifp->if_st_set_mx) (sp->s_ifp, rx->rx_mx[i], mx)) {
				dprintf(0, ("Bad status from if_st_set_mx\n"));
				error = EINVAL;
				goto quit_setup_buf;
			}
		}

		dprintf(30, ("mx_template: rmx 0x%x, bufsz %u, spray 0x%x, "
			     "bufx_base 0x%x (0x%x), bufx_range %d, port %d\n", 
			     rx->rx_mx, mx->bufsize, mx->base_spray,
			     mx->bufx_base, buf_ptr->st_addr_bufx.bufx_t.bufx,
			     mx->bufx_range, mx->port));
	}

	if(stifp && stifp->if_st_set_bufx)  {
		if((*stifp->if_st_set_bufx)(sp->s_ifp,
				buf_ptr->st_addr_bufx.bufx_t.bufx,
				pbuf, num_frags, log_bufsz, xfer_dir, 
				sp->s_vcd.vc_lport)) {
			dprintf(0, ("if_st_set_bufx: bad status\n"));
			error = EINVAL;
			goto quit_setup_buf;
		}
		dprintf(30, ("%s: bufx set: base 0x%x, bufsz %u, "
				"num_frags %u \n", 
			side, buf_ptr->st_addr_bufx.bufx_t.bufx,
			log_bufsz, num_frags));
	}

	/* "special delivery RESY" VC-num, for dung-beetles */
	if(! error && ST_BUFX_ALLOW_SEND == xfer_dir) {
		if(STP_VCD_OUT_VC_ROT & sp->s_vcd.vc_flags) {
			if(spray_width > 1)  {
				cmn_err(CE_WARN, "VC rotation and "
					"spraying are mutually "
					"exclusive\n");
				error = EINVAL;		
				goto quit_setup_buf;
			}
			else if((1 << log_bufsz) * spray_width 
						> VC3_UPPER_LIMIT) {
				sp->s_vcd.vc_out_vcnum = 3;
			}
			else {
				do {
					out_vcnum = ((out_vcnum + 1) & 0x03);
				} while(0 == out_vcnum);
				sp->s_vcd.vc_out_vcnum = out_vcnum;
			}
		}
		else {
			if(! sp->s_vcd.vc_out_vcnum)  {
				sp->s_vcd.vc_out_vcnum = DEFAULT_DATA_VC_NUM;
			}
		}
	}



quit_setup_buf:
	if(pbuf)  {
		kmem_free(pbuf, num_frags * sizeof(paddr_t *));
	}
	if(error)  {
		cmn_err(CE_WARN, "st_setup_buf returned error %d\n",
			error);
		dprintf(30, ("Error %d; buf flags 0x%x, cookies 0x%x\n",
				error, buf_ptr->bufx_flags,
							dma_cookies));
		if(dma_cookies && !(buf_ptr->bufx_flags & BUF_MAPPED)) {
			for(i = 0; i < uio->uio_iovcnt; i++)  {
				dprintf(10, 
				("unpinning %d bytes, starting 0x%x\n",
				uio->uio_iov[i].iov_len,
				uio->uio_iov[i].iov_base));
				fast_undma(uio->uio_iov[i].iov_base, 
					uio->uio_iov[i].iov_len,
					xfer_dir, &(dma_cookies[i]));
			}
			/* buf_ptr->bufx_flags &= ~BUF_MAPPED; */
		}
	}
	if(dma_cookies)  {
		kmem_free(dma_cookies, 
				uio->uio_iovcnt * sizeof(opaque_t));
	}
	if(alen)  {
		alenlist_destroy(alen);
	}
	return error;
}


int
st_teardown_buf(struct stpcb *sp, st_buf_t *buf_ptr, u_char xfer_dir,
							st_tid_t tid)
{
	st_ifnet_t      *stifp = (st_ifnet_t *) sp->s_stifp;
	int		error = 0;
	int		dma_direction;
	int		num_frags;
	char		side[5];


	dprintf(30, ("Clear/unmap/free bufx %u\n", 
		buf_ptr->st_addr_bufx.bufx_t.bufx));
	if(xfer_dir == ST_BUFX_ALLOW_RECV)  {
		dma_direction = B_WRITE;
		sprintf(side, "%s", "dest ");
	}
	else if(xfer_dir == ST_BUFX_ALLOW_SEND)  {
		dma_direction = B_READ;
		sprintf(side, "%s", "src  ");
	}
	else {
		error = EINVAL;
		goto quit_teardown_buf;
	}

	num_frags = st_bufx_to_nfrags(buf_ptr->st_addr_bufx.bufx_t.bufx,
					buf_ptr->bufx_cookie);
	ASSERT(num_frags);

	if(xfer_dir == ST_BUFX_ALLOW_RECV && stifp && 
					stifp->if_st_clear_mx)  {
		st_rx_t		*rx = &(sp->rx[tid]);

		while (rx->rx_num_mx > 0) {
			/* pre-decrement so can use as index into array */
			rx->rx_num_mx--;
			dprintf(30, ("rmx for clr-mx is 0x%x\n", rx->rx_mx[rx->rx_num_mx]));
			ASSERT_ALWAYS(0 == 
				      (*stifp->if_st_clear_mx)(
					    sp->s_ifp, rx->rx_mx[rx->rx_num_mx]));
			sp->s_R_Mx_free(sp, rx->rx_mx[rx->rx_num_mx]);
		}
	}

	if(stifp && stifp->if_st_clear_bufx)  {
		ASSERT_ALWAYS(0 == (*stifp->if_st_clear_bufx)
			(sp->s_ifp, buf_ptr->st_addr_bufx.bufx_t.bufx, 
					num_frags, xfer_dir));
	}

	if(BUF_MAPPED & buf_ptr->bufx_flags) {
		if(st_bufx_unmap(buf_ptr->st_addr_bufx.bufx_t.bufx,
				buf_ptr->num_bufx,
				buf_ptr->bufx_cookie, dma_direction)) {
			cmn_err(CE_PANIC, "st_bufx_unmap error!\n");
		}
	}

	buf_ptr->bufx_flags &= ~BUF_MAPPED;

	if(BUF_BUFX & buf_ptr->bufx_flags) {
		dprintf(10, ("%s: Releasing %u bufs, starting %u\n",
			side, buf_ptr->num_bufx,
			buf_ptr->st_addr_bufx.bufx_t.bufx));
		if(st_bufx_free(buf_ptr->st_addr_bufx.bufx_t.bufx,
				buf_ptr->num_bufx, &sp->s_ifp, 1, 
				buf_ptr->bufx_cookie))   {
			cmn_err(CE_PANIC, "st_bufx_free screwed up!\n");
		}
	}

	buf_ptr->bufx_flags = BUF_NONE;  

quit_teardown_buf:
	return error;
}
