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
 *  Filename: st_macros.h
 *  Description: various macros useful in ST-kernel implementation
 *
 *  $Author: kaushik $
 *  $Date: 1999/04/30 21:19:53 $
 *  $Revision: 1.4 $
 *  $Source: /proj/irix6.5f/isms/irix/kern/bsd/netinet/RCS/st_macros.h,v $
 *
 */

#ifndef 	__ST_MACROS_H__
#define 	__ST_MACROS_H__

/* ST macros, not to be used outside ST kernel module */

#ifdef	_KERNEL
#define	LOCK(x)			mutex_lock(&(x), PZERO)
#define	SPINLOCK(x)		mutex_spinlock(&(x))
#define	UNLOCK(x)		mutex_unlock(&(x))
#define	SPINUNLOCK(x, y)	mutex_spinunlock(&(x), y)

#define intostpcb(ip)   ((struct stpcb *)(ip)->inp_ppcb)
#define intosocket(ip)  ((struct socket *)(ip)->inp_socket)
#define sotostpcb(so)   (intostpcb(sotoinpcb(so)))
#define stpcbtoso(stpcb)   ((struct socket *)((stpcb)->s_so))


#define	IS_VC_TIMER(timer_id)	(timer_id < NUM_VC_TIMERS)

#define	IS_TX_TIMER(timer_id)	(timer_id > NUM_VC_TIMERS - 1 	\
				&& timer_id < NUM_VC_TIMERS 	\
					+ NUM_TX_TIMERS)

#define	IS_RX_TIMER(timer_id)	(timer_id > NUM_VC_TIMERS 	\
					+ NUM_TX_TIMERS - 1 	\
				&& timer_id < NUM_VC_TIMERS 	\
					+ NUM_TX_TIMERS 	\
					+ NUM_RX_TIMERS)

#define	IS_SLOT_TIMER(timer_id)	(timer_id > NUM_VC_TIMERS 	\
					+ NUM_TX_TIMERS		\
					+ NUM_RX_TIMERS -1	\
				&& timer_id < NUM_VC_TIMERS	\
					+ NUM_TX_TIMERS		\
					+ NUM_RX_TIMERS		\
					+ NUM_SLOT_TIMERS)
					

#define	TID_TO_TIMER_ID(tid)	(NUM_VC_TIMERS + tid)

#define	RID_TO_TIMER_ID(rid)	(NUM_VC_TIMERS + NUM_TX_TIMERS 	\
					+ rid)

#define	TIMER_ID_TO_TID(timer_id)	(timer_id - NUM_VC_TIMERS)

#define	TIMER_ID_TO_RID(timer_id)	(timer_id - NUM_VC_TIMERS \
						- NUM_TX_TIMERS)


#define	IS_TX_STATE(state)				\
		(state == STP_READY_FOR_RTS	||	\
		 (state >= STP_SEND_RTS_PINNING	&&	\
		 state <= STP_RA_RECEIVED))

#define	IS_RX_STATE(state)				\
		(state == STP_READY_FOR_RTS	||	\
		 (state >= STP_RTS_RECEIVED	&&	\
		 state <= STP_RA_SENT))


#define	IS_VC_OP(opcode)				\
		(((opcode >= ST_RCONNECT) && \
		(opcode <= ST_DCOMPLETE))  	\
				||			\
		((opcode >= ST_VOP_ULISTEN) && \
		(opcode <= ST_VOP_UDISCONNECT)))
	
#define	IS_DATA_OP(opcode)				\
		((opcode >= ST_RMB) && 	\
		(opcode <= ST_END_ACK))

#define	IS_TX_FSM_OP(opcode)				\
		(opcode == ST_RANSWER 	||		\
		opcode == ST_CTS)
		

#define	IS_RX_FSM_OP(opcode)				\
		(opcode == ST_RTS  	||		\
		opcode == ST_DATA)

#define IS_SYNC_OP(opcode)				\
		(opcode == ST_RS	||		\
		opcode == ST_RSR)

#define	KX_OUTSTANDING(sp)	((sp)->num_kid_allocated > 0)

#define	st_soreadable(so)	\
	(((so)->so_state & SS_ISCONNECTED) && 				\
		(soreadable(so) || KX_OUTSTANDING(sotostpcb(so))))

#define	st_sowriteable(so)	\
	(((so)->so_state & SS_ISCONNECTED) || 				\
		(((so)->so_state & SS_CANTSENDMORE) | (so)->so_error))


#define	st_same_key(k1, k2) 	((k1 == k2)? 1: 0)

#define	MX_SET(start, point) 		start[point] = 1
#define	MX_CLEAR(start, point)		start[point] = 0;
#define	MX_IS_SET(start, point)		(start[point] == 1)
#define	GOODMX(point)	(point != (uint16_t) INVALID_R_Mx \
				&& MX_IS_SET(st_R_Mx_tab, point))

#define	SLOTS_ENABLED(sp)				\
		(((u_int16_t) -1) != sp->s_vcd.vc_max_rslots)

#define OUT_OF_SLOTS(sp)				\
		(SLOTS_ENABLED(sp) &&			\
		(sp->s_vcd.vc_rslots <= ST_MIN_ALLOWED_SLOTS))

#define	WAIT_FOR_SLOTS(sp)		wait_for_slots(sp)
#define	SLOTS_AVAILBABLE(sp)		slots_available(sp)

#define	SET_BNUM_TAB(rx, bnum)					\
		ASSERT_ALWAYS(bnum < 8 * BNUM_TAB_SIZE);	\
		(rx)->bnum_tab[bnum/8] |= (1 << ((bnum) % 8))

#define	SET_RX_CTS_TAB(rx, bnum)				\
		ASSERT_ALWAYS(bnum < 8 * BNUM_TAB_SIZE);	\
		(rx)->CTS_tab[bnum/8] |= (1 << ((bnum) % 8))

#define	SET_TX_CTS_TAB(tx, bnum)				\
		ASSERT_ALWAYS(bnum < 8 * BNUM_TAB_SIZE);	\
		(tx)->data_CTS_tab[bnum/8] |= (1 << ((bnum) % 8))

#define	IS_SET_BNUM(rx, bnum)					\
		((rx)->bnum_tab[bnum/8] & (1 << ((bnum) % 8))) ? 1 : 0

#define	IS_SET_RX_CTS(rx, bnum)					\
		((rx)->CTS_tab[bnum/8] & (1 << ((bnum) % 8))) ? 1 : 0

#define	IS_SET_TX_CTS(tx, bnum)					\
		((tx)->data_CTS_tab[bnum/8] & 			\
				(1 << ((bnum) % 8))) ? 1 : 0
#define	GOOD_SLOTS(sp)						\
	((! SLOTS_ENABLED(sp))   ||				\
	(((int) sp->s_vcd.vc_rslots >= 0) &&			\
	(sp->s_vcd.vc_rslots <= sp->s_vcd.vc_max_rslots)))

#define	CHECK_SLOTS(sp)						\
		if(SLOTS_ENABLED(sp))	 			\
		ASSERT_ALWAYS((int) sp->s_vcd.vc_rslots > 0 &&	\
		sp->s_vcd.vc_rslots <= sp->s_vcd.vc_max_rslots)

#define	SLOTS_PANIC(sp)						\
	cmn_err(CE_PANIC, "bad slots; rslots %d, max %d\n", 	\
		sp->s_vcd.vc_rslots, sp->s_vcd.vc_max_rslots)

#define	CONSUMES_SLOT(OpFlags)					\
		(((OpFlags & ST_OPCODE_MASK) != ST_RCONNECT) 	\
			&&					\
		(((OpFlags & ST_FLAG_MASK) != ST_SILENT) 	\
		|| ((OpFlags & ST_OPCODE_MASK) != ST_DATA))	\
			&&					\
		((OpFlags & ST_OPCODE_MASK) != ST_RS) 		\
			&&					\
		((OpFlags & ST_OPCODE_MASK) != ST_RSR))
		
#endif	/* _KERNEL */
#endif 	/* __ST_MACROS_H__ */
