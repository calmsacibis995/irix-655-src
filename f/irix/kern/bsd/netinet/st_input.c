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
 *  Filename: st_input.c
 *  Description: routines that start off the processing of an incoming 
 *		packet using the ST protocol.
 *
 *  $Author: jmp $
 *
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
#include "sys/tcpipstats.h"
#include "sys/cmn_err.h"
#include "ksys/xthread.h"

#include "st.h"
#include "st_var.h"
#include "st_macros.h"
#include "st_debug.h"
#include "st_bufx.h"



extern 	struct 	inpcb stpcb_head;
extern struct ifqueue stpintrq;

static int new_thread_for_fsm = 1;

int
st_input_process(struct mbuf *m)
{
  	/* struct ifnet *ifp; */
	int  hlen;
      	ushort	opcode, OpFlags, flags;
    	struct st_io_s *pstio;
    	register struct inpcb *inp;

  	DPRINTF(ST_DEBUG_ENTRY,("ENTRY: st_input_process()\n"));
	
	dprintf(5, ("Thread 0x%x in st_input_process\n",
		curthreadp));

	/* ifp = mtod(m, struct ifheader *)->ifh_ifp; */
	hlen = mtod(m, struct ifheader *)->ifh_hdrlen;
	M_ADJ(m, hlen);
	
    	if (m == 0) {
      		dprintf(15, ("\t No mbuf in st_input_process\n"));
      		return -1;
    	}

    	pstio = mtod(m, struct st_io_s *);
	OpFlags = pstio->sth.sth_rc.OpFlags;
	opcode = OpFlags & ST_OPCODE_MASK;
	flags = OpFlags & ST_FLAG_MASK;

	dprintf(5, ("IN faddr: %x fport: %d laddr: %x lport: %d; sz %d (%d), typ %d ",
		    pstio->iap.iap_faddr,
		    pstio->iap.iap_fport,
		    pstio->iap.iap_laddr,
		    pstio->iap.iap_lport,
		    m_length(m), m->m_len, m->m_type));
		    
    	inp = in_pcblookupx(&stpcb_head, pstio->iap.iap_faddr, 
		pstio->iap.iap_fport, pstio->iap.iap_laddr, 
		pstio->iap.iap_lport, INPLOOKUP_ALL);

	dprintf(5, ("inp = 0x%x\n"));
	if (inp == 0) {
	  	if (opcode == ST_RCONNECT) {
	    		inp = in_pcblookupx(&stpcb_head, 
				pstio->iap.iap_faddr, 
				pstio->iap.iap_fport, 
				pstio->iap.iap_laddr, 
				pstio->iap.iap_lport, 
				(INPLOOKUP_LISTEN|INPLOOKUP_WILDCARD));
	  	}
/* 	  	else { */
/* 	    		cmn_err(CE_WARN,  */
/* 		    		"Non-Request Connect Message [%s] %x:%x from %x:%x\n", */
/* 		    		st_decode_opcode(opcode), */
/* 		    		pstio->iap.iap_faddr,  */
/* 				pstio->iap.iap_fport, */
/* 		    		pstio->iap.iap_laddr,  */
/* 				pstio->iap.iap_lport); */
/* 	  	} */
    	}


	/* HARP inharp_resolve() EIS-Beta1 Contingency Plan */

	if (inp == 0) {
	  struct in_addr any;


	  any.s_addr = INADDR_ANY;

	  inp = in_pcblookupx(&stpcb_head, 
			      /* pstio->iap.iap_faddr, */
			      any,
			      pstio->iap.iap_fport, 
			      /* pstio->iap.iap_laddr, */
			      any,
			      pstio->iap.iap_lport, 
			      INPLOOKUP_ALL|INPLOOKUP_WILDCARD);
	  
	}

    	if (inp == 0) {
	  	cmn_err(CE_WARN,
		  	"ST PCB Not Found [%s] %x:%x from %x:%x\n",
		  	st_decode_opcode(opcode),
		  	pstio->iap.iap_faddr, pstio->iap.iap_fport,
		  	pstio->iap.iap_laddr, pstio->iap.iap_lport);
	  	m_freem(m); 
    	}
    	else {
      		struct socket *so    = intosocket(inp);
      		struct stpcb  *stpcb = intostpcb(inp);

      		dprintf(10, ("st_input: Port %d got op %s from port %d "
			"on sock 0x%x, sp 0x%x\n",
		  	inp->inp_lport, 
			st_decode_opcode(opcode),
		      	pstio->iap.iap_fport, so, stpcb));
		
		STSTAT(stps_rxtotal);
		
		SOCKET_LOCK(so);
      		if(IS_VC_OP(opcode))  {
			dprintf(30, ("Got VC Op %s\n",
				st_decode_opcode(opcode)));
			if(SLOTS_ENABLED(stpcb) 
					&& CONSUMES_SLOT(OpFlags))  {
				stpcb->s_vcd.vc_lslots =
					(stpcb->s_vcd.vc_lslots + 1)
					% stpcb->s_vcd.vc_max_lslots;
				dprintf(10, ("op %s; lslots: inc to %d\n",
					st_decode_opcode(opcode),
					stpcb->s_vcd.vc_lslots));
			}
      			if(stvc_input(stpcb, OpFlags, m)) {
				dprintf(0, ("error from stvc_input\n"));
			}
      		}
      		else if(IS_DATA_OP(opcode))  {
			dprintf(30, ("Got DATA Op %s\n",
				st_decode_opcode(opcode)));
			if(SLOTS_ENABLED(stpcb)
					&& CONSUMES_SLOT(opcode))  {
				stpcb->s_vcd.vc_lslots =
					(stpcb->s_vcd.vc_lslots + 1)
					% stpcb->s_vcd.vc_max_lslots;
				dprintf(10, ("op %s; lslots: inc to %d\n",
					st_decode_opcode(opcode),
					stpcb->s_vcd.vc_lslots));
			}
      			if(stdata_input(stpcb, OpFlags, m)) {
				dprintf(0, 
					("error from stdata_input\n"));
			}
      		}
      		else  {
			cmn_err(CE_WARN, 
			"Ignoring unknown opcode %s in st_input_process\n");
      		}

		if(opcode != ST_DATA)  {
			dprintf(30, ("Freeing mbuf, opcode %s\n",
				st_decode_opcode(opcode)));
    			m_freem(m);
		}
		else {
			dprintf(20, ("DATA mbuf 0x%x will be freed in st_sorec\n", m));
		}
    	}
		
    	DPRINTF(ST_DEBUG_RECV, 
		("RECV: st_intr() dequeued packet; opcode %s\n",
			st_decode_opcode(opcode)));

	if(new_thread_for_fsm)  {
 		dprintf(5, ("Thread 0x%x exitting; status %d\n", 
			curthreadp, (stpintrq.ifq_len != 0)));
		xthread_exit();
		/* NOTREACHED */
	}

	return(stpintrq.ifq_len != 0);
}



void
st_input(struct mbuf *m, struct route *route)
{
	void	*stack;

	if(! m)  {
		dprintf(0, ("NULL mbuf in st_input\n"));
	}
	else if(! m_length(m)) {
		dprintf(0, ("Bad mbuf len %d in st_input; dropping!\n",
			 m_length(m)));
	}
	else {
		dprintf(20, ("st_input: mbuf len %d, type %d, route is 0x%x\n", 
			m_length(m), m->m_type, route));
		if(! new_thread_for_fsm)  {
			dprintf(5, ("0x%x not creating new thread\n",
				curthreadp));
			ASSERT_ALWAYS(0 == st_input_process(m));
		}
 		else {
			stack = kmem_alloc(2*KTHREAD_DEF_STACKSZ,
				VM_DIRECT | VM_NOSLEEP);
			if(! stack)  {
				cmn_err(CE_WARN, "Could not create stack for "
					"ST thread; dropping packet\n");
			}
			else {
				dprintf(5, ("Creating xthread\n"));
				xthread_create("ST-input", stack, 
					2*KTHREAD_DEF_STACKSZ,
					KT_STACK_MALLOC, 192,  /* priority */
					KT_PS | KT_BIND, 
					(xt_func_t *) st_input_process, m);
				dprintf(5, ("Done creating xthread\n"));
			}
		}
	}
	
	dprintf(5, ("Thread 0x%x returning from ST-i/p process\n",
		curthreadp));

  	return;
}
