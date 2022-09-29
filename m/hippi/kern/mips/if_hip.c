/*
 * if_hip.c
 *
 * A software module that provides a TCP/IP interface into HIPPI.  
 * Requires either hippi.c (Challenge HIO HIPPI) or hps (Origin
 * XIO HIPPI-Serial) driver.
 *
 * #ifdefs HIO_HIPPI and XIO_HIPPI control hw-specific code.
 *
 * Copyright 1994 Silicon Graphics, Inc.  All rights reserved.
 *
 */

#ident "$Revision: 1.50 $"

#include "sys/tcp-param.h"
#include "sys/param.h"
#include "sys/debug.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/sysmacros.h"
#include "sys/buf.h"
#include "sys/ioctl.h"
#include "sys/socket.h"
#include "sys/mbuf.h"
#include "sys/errno.h"
#include "sys/immu.h"
#include "sys/cpu.h"
#include "sys/sbd.h"
#include "sys/protosw.h"
#include "sys/poll.h"		/* XXX */
#include "sys/kmem.h"
#include "sys/kabi.h"
#ifdef HIO_HIPPI
#include "sys/EVEREST/everest.h"
#endif

#include "net/if.h"
#include "net/if_types.h"
#ifndef IFT_HIPPI
#define IFT_HIPPI 0x2f
#endif
#include "net/raw.h"
#include "net/soioctl.h"
#include "net/netisr.h"

#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/if_ether.h"
#include "netinet/ip.h"

#include "sys/hippi.h"
#ifdef HIO_HIPPI
#include "sys/hippidev.h"
#else /* XIO_HIPPI */
#include "sys/PCI/pciio.h"
#include "sys/PCI/bridge.h"
#include "sys/hps_ext.h"
#include "fwvers.h"
#include "sys/hippi_firm.h"
#include "sys/hps_priv.h"
#endif

#include "sys/if_hip.h"
#include "sys/idbgentry.h"

extern __uint32_t in_cksum_sub( ushort *, int, __uint32_t );	/* XXX */
extern struct ifqueue	ipintrq;		/* XXX */

#ifdef HIO_HIPPI
ifhip_vars_t ifhip_device[ EV_MAX_HIPADAPS ];
#endif

int	ifhip_output( struct ifnet *, struct mbuf *, struct sockaddr *,
		      struct rtentry * );
int     ifhip_hw_write(ifhip_vars_t *ifhip_devp, struct mbuf *m, 
                       struct sockaddr *dst);
int     hip_mbuf_write(struct ifnet *ifp, struct mbuf *m, u_short sumoff);
int	ifhip_ifioctl( struct ifnet *, int, void * );
int	ifhip_watchdog( struct ifnet * );
void	ifhip_intr( ifhip_vars_t * );

void	ifhip_drain( ifhip_vars_t * );

static struct mbuf *ifhip_unaligned_mbuf( struct mbuf * );

static int	harpresolve( struct sockaddr_in *, u_char *, u_int * );
static int	harpioctl( int, struct harpreq * );
static int	harpresolve_ula( u_char *, u_int * );


static int ifhip_num_big = HIP_MAX_BIG -1 ;
static int ifhip_num_small = HIP_MAX_SML -1 ;

int	ifhip_bad_mbufs    =  0;	/* had to copy mbufs */
#ifdef DEBUG
int     ifhip_debug        =  0;
int	ifhip_out_hdrmov   =  0;	/* had to move data in hdr mbuf */
int	ifhip_out_allocm   =  0;	/* had to alloc mbuf hdr */
int	ifhip_bad_cksum_in =  0;	/* h/w discovered bad cksum */

void hhelp_idbg();
void hdevp_idbg(ifhip_vars_t *hippi_devp);
#ifdef HIO_HIPPI
void hudevp_idbg(short unit);
#endif
void hmbuf_idbg(struct mbuf *buf);
void hlmbuf_idbg(ifhip_vars_t *hippi_devp);
void hsmbuf_idbg(ifhip_vars_t *hippi_devp);
void hdmbufl_idbg(ifhip_vars_t *hippi_devp);
#endif /* DEBUG */



/* =====================================================================
 * 	Private idbg routines
 */

/*
 * returns a printable string version of an internet address
 */
void
saddrtoa(__uint32_t s_addr, char *buf)
{
	u_char n, *p;
	char *b;
	int i;
	struct in_addr in;

	in.s_addr = s_addr;

	if ( s_addr == NULLADDR_TYPE ) {
	  bcopy(" NULLADDR_TYPE ", buf, 19);
	} else {

	  p = (u_char *)&in;
	  b = buf;
	  for (i = 0; i < 4; i++) {
	    if (i)
	      *b++ = '.';
	    n = *p;
	    if (n > 99) {
	      *b++ = '0' + (n / 100);
	      n %= 100;
	    }
	    if (*p > 9) {
	      *b++ = '0' + (n / 10);
	      n %= 10;
	    }
	    *b++ = '0' + n;
	    p++;
	  }
	  *b = 0;
	}
}

void
hip_ntoa(struct in_addr in, char *buf)
{
  saddrtoa(in.s_addr, buf);
}



static char *
hip_hexconv(unsigned char *cp, int len, int dots)
{
	register int idx = 0;
	register int qqq;
	int maxi = 48;
	static const char digits[] = "0123456789ABCDEF";
	static char qstr[100];

	bzero(qstr, maxi);
	if (len < maxi) {
	  maxi = len;	  
	  for (idx = 0, qqq = 0; qqq<maxi; qqq++) {
	    if (((qqq%32) == 0) && (qqq != 0))
	      qstr[idx++] = '\n';
	    qstr[idx++] = digits[cp[qqq] >> 4];  
	    qstr[idx++] = digits[cp[qqq] & 0xf];
	    if ((dots) && (qqq < maxi-1) )
	      qstr[idx++] = '.';
	  }
	  qstr[idx] = 0;         
	}

	else {	/* len >= maxi = returns Err! */
	  qstr[0] = 'E'; qstr[1] = 'r'; qstr[2] = 'r';qstr[3] = '!'; 
	  qstr[4] = 0;         
	}

	return qstr;	  
}


void
hip_dumphipaddr(hip_address_t hipaddr)
{
	qprintf("I-Field = 0x%x,  ula  = %s \n", hipaddr.hi_I, 		
		hip_hexconv(hipaddr.ula, HIPPI_ULA_SIZE, TRUE) );
}

/* Compares a hippi address returning 1 if they are the same, else 0. 
 * difference is that it does a field wise compare vs
 * all the memory.
 */
int
samehipaddr(hip_address_t *hipaddr1, hip_address_t *hipaddr2, int fullIfield) 
{
 if(fullIfield)
    return( (bcmp( &hipaddr1->hi_I, &hipaddr2->hi_I, HIPPI_IFLD_SIZE) == 0) &&
            (bcmp( &hipaddr1->ula,  &hipaddr2->ula,  HIPPI_ULA_SIZE ) == 0)  );
  else /* only partial I-field: the switchaddress of it */
    return( (bcmp( &hipaddr1->fields.swaddr, 
		   &hipaddr2->fields.swaddr,        HIPPI_SWADDR_S) == 0 ) &&
	    (bcmp( &hipaddr1->ula,  &hipaddr2->ula, HIPPI_ULA_SIZE) == 0 )   );
}


#ifdef IFHIP_DEBUG
static void
hip_dumpif(struct ifnet ifp)
{
  qprintf("--------------------- IFNET Structure ----------------------\n");
  qprintf("if_name \"%s\" \t\tif_unit %d \tif_mtu %d \tif_flags 0x%x\n",
	  ifp.if_name, ifp.if_unit, ifp.if_mtu, ifp.if_flags);
  qprintf("ifq_len %3d   \t\tifq_maxlen %d \tifq_drops %d \tif_timer %d\n",
	  ifp.if_snd.ifq_len, ifp.if_snd.ifq_maxlen, ifp.if_snd.ifq_drops,
	  ifp.if_timer);
  qprintf("if_ipackets %d \t\tif_ierrors %d \tif_opackets %d \tif_oerrors %d\n",
	  ifp.if_ipackets, ifp.if_ierrors, ifp.if_opackets, 
	  ifp.if_oerrors);
  qprintf("if_collisions %d \tif_ibytes %d \tif_obytes %d \tif_odrops %d\n",
	  ifp.if_collisions, ifp.if_ibytes, ifp.if_obytes, ifp.if_odrops);
}


static void 
ifhip_idbg(ifhip_vars_t *ifhip_devp)
{
/*     struct hip_ulatab * srvp; */
/*     char   buf[20]; */

    qprintf("-------------- Per device variables for if_hip interfaces --------------\n");
    if (ifhip_devp == NULL) {
	qprintf ("Invalid argument. USAGE: ifhip(ifhip_vars_t *ifhip_devp)\n");
	return;
    }
/*     srvp = ifhip_devp->ihip_arpsrv_entry; */

/*     hip_ntoa(ifhip_devp->ihip_myipaddr, buf); */
/*     qprintf("ihip_myipaddr       = %s\n", buf); */

    qprintf("struct ifnet hi_if  = 0x%x,  hi_rawif      = 0x%x\n",
	    &ifhip_devp->hi_if, &ifhip_devp->hi_rawif);
#ifdef XIO
    qprintf("ifhip_devp->hps_dev = 0x%x,  hi_mbuf_mutex = 0x%x\n",
	    ifhip_devp->hps_devp, &ifhip_devp->hi_mbuf_mutex);
#endif

    qprintf("ifhip addrp = 0x%x, ", &ifhip_devp->hi_haddr);
    hip_dumphipaddr( ifhip_devp->hi_haddr );

    qprintf("hi_in_sml_num = %d, \thi_in_sml_h = %d    hi_in_sml_t   = %d\n",
	    ifhip_devp->hi_in_sml_num, ifhip_devp->hi_in_sml_h);

/*     qprintf("         Me_Srvr? :               %d, calculated = %d\n", */
/* 	    ifhip_devp->ihip_me_srvr,  */
/* 	    !bcmp( &ifhip_devp->ihip_arpsrv, &ifhip_devp->hi_haddr,  */
/* 		  sizeof(hip_address_t) ) ); */

/*     qprintf("ARP server hipaddr:               "); */
/*     hip_dumphipaddr( ifhip_devp->ihip_arpsrv); */

 
/*     if( srvp == NULL) */
/*       qprintf("ihip_arpsrv_entry structure not inititalised -> NULL\n"); */
/*     else { */
/*       qprintf("ARP server table entry hipaddr:   "); */
/*       hip_dumphipaddr( srvp->hipaddr ); */
/*     } */
	      
     hip_dumpif( ifhip_devp->hi_if    );
    /* hip_dumpif( &ifhip_devp->hi_rawif ); */
    
    qprintf("--------------- END IF HIP idbg ------------------------\n");
}
#endif /* IFHIP_DEBUG */

static void
ifhip_fplesnap_idbg(fplesnap_t *fplesnap)
{
  char    *buf;

  if (fplesnap == NULL) {
    qprintf ("Invalid argument.- it is NULL  USAGE: ifhip_fplesnap(fplesnap_t *fplesnap)\n");
    return;
  }
  qprintf("+------- Decoding   0x%x: the FP - LE - SNAP header --------\n",  fplesnap);

  buf = hip_hexconv((unsigned char *)fplesnap, sizeof(fplesnap_t), FALSE);
  printf("| Mem: %s\n",buf);

  if( fplesnap->fp.hfp_flags & HFP_FLAGS_P)
    qprintf("| FLAGS: P set -> D1 present, ");
  else
    qprintf("| FLAGS: P not set -> D1 not prsnt, ");
  if( fplesnap->fp.hfp_flags & HFP_FLAGS_B )
    qprintf("| B  set     -> D2 starts on or bfr 2nd burst\n");
  else
    qprintf("| B  not set -> D2 starts at beg 2nd burst\n");

  qprintf("| hfp_ulp_id, %d\t", fplesnap->fp.hfp_ulp_id & 0xFF);
  qprintf("D1 area size = %d, D2 offset = %d, D2 size = %u \n", 
	  fplesnap->fp.hfp_d1d2off & HFP_D1SZ_MASK,
	  fplesnap->fp.hfp_d1d2off & HFP_D2OFF_MASK,
	  fplesnap->fp.hfp_d2size);
  qprintf(
   "+--------------------------- LE portion of the headdes -------------------\n");

/*  These we don't set today and so its printout is more confusing than anything */

/*   qprintf("| Forwarding class (should be 0) 3 bit value = 0x%x\n", */
/* 	  fplesnap->le.fcwm & HLE_FC_MASK); */
/*   if( fplesnap->le.fcwm & HLE_W ) */
/*     qprintf("| Double wide bit ON : 64bit datapath,    1600M bit/sec, "); */
/*   else */
/*     qprintf("| Double wide bit OFF: 32bit datapath,    800M bit/sec, "); */
/*   switch(fplesnap->le.fcwm & HLE_MT_MASK) { */
/*   case MAT_DATA: */
/*     qprintf("M_tpe = DATA\n"); */
/*     break; */
/*   default: */
/*     qprintf("M_Type = %d, NonDATA\n",  fplesnap->le.fcwm & HLE_MT_MASK); */
/*   break; */
/*   } */

  qprintf("| Switch addresses : dest: 0x%s, ",
	  hip_hexconv(fplesnap->le.dest_swaddr, HIPPI_SWADDR_S, TRUE));
  qprintf(                                 "\tsource: 0x%s, \n",
	  hip_hexconv(fplesnap->le.src_swaddr,  HIPPI_SWADDR_S, TRUE));
  qprintf("| ULA's: dest: %s, ", 
	  hip_hexconv(fplesnap->le.dest, HIPPI_ULA_SIZE, TRUE ));
  qprintf(                    "\tsource: %s\n", 
	  hip_hexconv(fplesnap->le.src , HIPPI_ULA_SIZE, TRUE ));

/*   qprintf("| Address types    : dest: 0x%x, \t\tsource: 0x%x, \n", */
/* 	  fplesnap->le.swat & HLE_DAT_MASK,  */
/* 	  fplesnap->le.swat & HLE_SAT_MASK); */
/*   qprintf("|ULA's: dest: 0x%x, \tsource: 0x%x\n",  */
/* 	  fplesnap->le.dest, fplesnap->le.src); */
  qprintf(
   "+--------------------------------------------------------------------------\n");
}

static void
ifhip_dump_I_fplesnap(I_fplesnap_t *ifplesnap)
{
  fplesnap_t fplesnap;
  char       *buf;

  qprintf(
   "+--------------------------------------------------------------------------\n");
  buf = hip_hexconv((unsigned char *)ifplesnap, sizeof(I_fplesnap_t), FALSE);
  printf("| Mem: %s\n",buf);

  bcopy(&ifplesnap->fp,   &fplesnap.fp,   sizeof(hippi_fp_t));
  bcopy(&ifplesnap->le,   &fplesnap.le,   sizeof(hippi_le_t));
  bcopy(&ifplesnap->snap, &fplesnap.snap, sizeof(hippi_snap_t));
  qprintf("+-------       Leading I-Field: 0x%x       --------\n", ifplesnap->I);

  ifhip_fplesnap_idbg(&fplesnap);
}

/*****************************************************************************
 * End debugging and helper routines  -   Start of real HIPPI IFNET routines *
 *****************************************************************************/

void
ifhip_attach( hippi_vars_t *hippi_devp )
{
	int	s;
	int	unit = hippi_devp->unit;
	ifhip_vars_t *ifhip_devp;
	struct ifnet *ifhip_ifp;

#ifdef HIO_HIPPI
	ifhip_devp = & ifhip_device[unit];
#else /* XIO_HIPPI */
	ifhip_devp = (ifhip_vars_t *)kmem_zalloc(sizeof (ifhip_vars_t),
						 KM_SLEEP);
	hippi_devp->ifhps_devp = ifhip_devp;
	ifhip_devp->hps_devp = hippi_devp;
#endif
	ifhip_ifp = &ifhip_devp->hi_if;

#ifdef XIO_HIPPI
	/* If card has MAC addr programmed, use that as src ULA */
	if (hippi_devp->mac_addr[0] == 8) {
	    bcopy (hippi_devp->mac_addr, ifhip_devp->hi_haddr.ula, HIPPI_ULA_SIZE);
	}
#endif

#ifdef DEBUG
	printf( "ifhip_attach:  unit = %d, devp = 0x%llx\n", unit,
		ifhip_devp);
#endif

	s = splimp();

	initnlock( & ifhip_harplock, "ifhiphar" );
#ifdef XIO_HIPPI
	mutex_init(&ifhip_devp->hi_mbuf_mutex, MUTEX_DEFAULT, "hi_mbuf");
#endif
	/* Set up network interface for HIPPI card.  These tell
	 * the operating system what routines to call to do
	 * various things.
	 */
	ifhip_ifp->if_unit   = unit;		/* unit number */
	ifhip_ifp->if_name   = IFHIP_NAME;	/* name (try netstat -i) */
	ifhip_ifp->if_type   = IFT_HIPPI;
	ifhip_ifp->if_mtu    = ifhip_mtusize ? ifhip_mtusize : IFHIP_DEFAULT_MTU;
	ifhip_ifp->if_ioctl  = ifhip_ifioctl;	/* ioctl system calls */
	ifhip_ifp->if_output = ifhip_output;	/* send a packet */

	ifhip_ifp->if_flags = ( ifhip_cksum & 2 ) ?
		IFF_CKSUM|IFF_DRVRLOCK :
		IFF_DRVRLOCK;

	ifhip_ifp->if_snd.ifq_maxlen = IFHIP_MAX_OUTQ;

	/* Set TCP default widow sizes.
	 */
	ifhip_ifp->if_sendspace = HIPPI_DEFAULT_SENDSPACE;
	ifhip_ifp->if_recvspace = HIPPI_DEFAULT_RECVSPACE;

	ifhip_ifp->if_baudrate.ifs_log2 = 2;
	ifhip_ifp->if_baudrate.ifs_value = 200*1000*1000;

	/* Register the HIPPI card as a network interface.
	 */
	if_attach( ifhip_ifp );
	{
		char buf[25];
		vertex_hdl_t hipdev;
		graph_error_t rc;
		sprintf(buf, "hippi/%d", unit);
		rc = hwgraph_traverse(GRAPH_VERTEX_NONE, buf, &hipdev);
		if (rc == GRAPH_SUCCESS) {
			sprintf(buf,"%s%d", IFHIP_NAME, unit);
			if_hwgraph_alias_add(hipdev, buf);
		}
	}

	/* Register the HIPPI card as a raw interface.
	 */
	rawif_attach( & ifhip_devp->hi_rawif, ifhip_ifp,
		(caddr_t) ifhip_devp->hi_haddr.ula,
		(caddr_t) &etherbroadcastaddr[0],
		sizeof(ifhip_devp->hi_haddr.ula),
		IFHIP_SNHDR_LEN /* header length */,
		30 /* XXX: src address offset */,
		22 /* XXX: dst address offset */ );
	
	splx( s );

#ifdef IFHIP_DEBUG
	dprintf( 2, ( "ifhip_attach: adding idbg routines\n" ));
	/* no matter if we add this multiple time (i.e. for each interface)
         * idbg checks for multiplt entries and so we are ok.
	 */
	idbg_addfunc("ifhip",          ifhip_idbg);
	idbg_addfunc("ifhip_fplesnap", ifhip_fplesnap_idbg);

	idbg_addfunc("hhelp",hhelp_idbg);
	idbg_addfunc("hdevp",hdevp_idbg);
#ifdef HIO_HIPPI
	idbg_addfunc("hudevp",hudevp_idbg);
#endif
	idbg_addfunc("hmbuf", hmbuf_idbg);
	idbg_addfunc("hlmbuf",hlmbuf_idbg);
	idbg_addfunc("hsmbuf",hsmbuf_idbg);
	idbg_addfunc("hdmbufl",hdmbufl_idbg);
#endif
}


void
ifhip_shutdown( hippi_vars_t *hippi_devp )
{
	struct mbuf *m;
#ifdef HIO_HIPPI
	ifhip_vars_t *ifhip_devp = & ifhip_device[hippi_devp->unit];
#else /* XIO_HIPPI */
	ifhip_vars_t *ifhip_devp = hippi_devp->ifhps_devp;
	int  s;
#endif

#ifdef DEBUG
	if ( ifhip_devp->hi_if.if_flags & IFF_DEBUG )
		printf("ifhip_shutdown(%d) called.\n", hippi_devp->unit );
#endif

	/* HIO driver uses devslock to protect everything, so no
	 * further locking needed here. XIO driver uses lowlevel
	 * locking. 
	 */
#ifdef XIO_HIPPI
	IFQ_LOCK( &ifhip_devp->hi_if.if_snd, s );
#endif
	ifhip_devp->hi_if.if_flags &= ~IFF_UP;

	/* Drain transmit queue.
	 */
	do {
		IF_DEQUEUE_NOLOCK( &ifhip_devp->hi_if.if_snd, m );
		if ( m )
			m_freem( m );
	} while ( m );
#ifdef XIO_HIPPI
	IFQ_UNLOCK( &ifhip_devp->hi_if.if_snd, s );
#endif
	/* Drain input buffers */
	ifhip_drain( ifhip_devp );
}


static void
#ifdef HIO_HIPPI
ifhip_ifinit( int unit )
#else /* XIO_HIPPI */
ifhip_ifinit( struct ifnet *ifp )
#endif
{
	ifhip_vars_t *ifhip_devp;
	hippi_vars_t *hippi_devp;
#ifdef HIO_HIPPI
	ifhip_devp = & ifhip_device[unit];
	hippi_devp = & hippi_device[unit];
#else /* XIO_HIPPI */
	ifhip_devp = (ifhip_vars_t *)ifp;
	hippi_devp = ifhip_devp->hps_devp;
#endif

#ifdef DEBUG
	if ( ifhip_devp->hi_if.if_flags & IFF_DEBUG )
		printf("ifhip_ifinit(%d) called.\n", hippi_devp->unit );
#endif
	
	if ( ifhip_devp->hi_if.if_flags & IFF_UP ) {
#ifdef XIO_HIPPI
		hps_setparams( hippi_devp, 0,
			hippi_devp->hi_hwflags | HIP_FLAG_IF_UP );
#else  /* HIO_HIPPI */
		hippi_hwflags( hippi_devp,
			hippi_devp->hi_hwflags | HIP_FLAG_IF_UP );
		mutex_lock( & hippi_devp->devslock, PZERO );
#endif
		ifhip_fillin( hippi_devp );
#ifdef HIO_HIPPI
		mutex_unlock( & hippi_devp->devslock );
#endif
	}
	else {
#ifdef XIO_HIPPI
		hps_setparams( hippi_devp, 0,
			hippi_devp->hi_hwflags & ~HIP_FLAG_IF_UP );
#else /* HIO_HIPPI */
		hippi_hwflags( hippi_devp,
			hippi_devp->hi_hwflags & ~HIP_FLAG_IF_UP );
		mutex_lock( & hippi_devp->devslock, PZERO );
#endif
		ifhip_drain( ifhip_devp );
#ifdef HIO_HIPPI
		mutex_unlock( & hippi_devp->devslock );
#endif
	}

	/* Wake up board so it gets input buffers */
#ifdef HIO_HIPPI

	hippi_wakeup( hippi_devp );

#else /* XIO_HIPPI */

	if (hippi_devp->hi_dsleep->flags & HIPFW_FLAG_SLEEP)
            hps_wake_dst (hippi_devp);
#ifdef USE_MAILBOX
	/* tell firmware we've got some descriptors for it */
	hps_dst_d2b_rdy(hippi_devp);
#endif

#endif /* XIO_HIPPI */
}

/*
 * ifhip_le_input()
 *
 * Procedure Description:
 *   Gets the buffers, if we have a SNAP (which is what we expect to have)
 *   we look at the ETHERTYPE. In case of IP put it on the ifq, ARP pass it to 
 *   hiparp_input(). Also deal with the snooping interace
 *
 * Callers:
 *   From device dependent driver after identifying the ULP as LE.
 *
 * Parameter Description:
 *   hippi_devp - The hippi device struct on which this LE packet was received
 *   b2hp       - The Board to host descriptor pointer for the incomming packet
 * 
 * Returns:
 * 
 */
void
ifhip_le_input( hippi_vars_t *hippi_devp, volatile hip_b2h_t *b2hp )
{
	int 	i;
#ifdef HIO_HIPPI
	ifhip_vars_t *ifhip_devp = &ifhip_device[ hippi_devp->unit ];
#else /* XIO_HIPPI */
	ifhip_vars_t *ifhip_devp = hippi_devp->ifhps_devp;
#endif
	int	pages = b2hp->b2h_pages;
	int	words = b2hp->b2h_words;
	int	len = B2H_LE_LEN(b2hp);
	int	pad_len = 0;
	int	netisrbit = -1;
	u_int	port;
	__uint32_t	cksum = B2H_LE_CKSUM(b2hp);
	struct 	mbuf *m, *m1, **m_prvnxt;
	struct	ifqueue	*ifq = 0;
	fplesnap_t *fple;
	struct ifnet *ifp;
	
	ifp = &ifhip_devp->hi_if;

	ASSERT( len > 0 );

        IFDPRINTF( ifp, ("ifhip_le_input on hip%d\n",ifhip_devp->hi_if.if_unit));

#ifdef XIO_HIPPI
	mutex_lock(&ifhip_devp->hi_mbuf_mutex, PZERO);

	/* check for prior shutdown */
	if ( !ifhip_devp->hi_in_smls[ifhip_devp->hi_in_sml_h] ) {
		mutex_unlock(&ifhip_devp->hi_mbuf_mutex);
		return;
	}
#endif

	/* get small buffers
	 */
	i = ifhip_devp->hi_in_sml_h;
	m_prvnxt = &m;
	while ( words > 0 ) {
		int	blen = min( MLEN, 4*words );

		m1 = ifhip_devp->hi_in_smls[i];
		*m_prvnxt = m1;
		m_prvnxt = & m1->m_next;
		m1->m_len = blen;
		len -= blen;
		words -= blen/4;
		i = (i+1)%HIP_MAX_SML;
		ASSERT( ifhip_devp->hi_in_sml_num > 0 );
		ifhip_devp->hi_in_sml_num--;
	}
	ifhip_devp->hi_in_sml_h = i;

	ASSERT(words == 0); 
	/* not the case when words*4 was > than MLEN, 
         * -> we would loose data */

	/* get page sized buffers
	 */
	i = ifhip_devp->hi_in_big_h;
	while ( pages-- > 0 ) {
		m1 = ifhip_devp->hi_in_bigs[i];
		*m_prvnxt = m1;
		m_prvnxt = & m1->m_next;
		i = (i+1)%HIP_MAX_BIG;
		ASSERT( ifhip_devp->hi_in_big_num > 0 );
		ifhip_devp->hi_in_big_num--;
		if ( pages > 0 )
			len -= HIP_BIG_SIZE;
		else
			m1->m_len = len;
	}
	ifhip_devp->hi_in_big_h = i;
#ifdef XIO_HIPPI
	mutex_unlock(&ifhip_devp->hi_mbuf_mutex);
#endif

	len = B2H_LE_LEN(b2hp);

	fple = mtod(m, fplesnap_t *);

	/* if packet data is not word multiple it's been padded */
	if (fple->fp.hfp_d2size & 0x3) {
	    pad_len = 0x4 - (fple->fp.hfp_d2size & 0x3);

	    /* correct last mbuf len for firmware padding  */
	    m1->m_len -= pad_len;
	}

	ASSERT( fple->fp.hfp_ulp_id == HIPPI_ULP_LE );

	/* Check for common SNAP type
	 */
	if ( *( (__uint32_t *) &fple->snap.ssap ) == htonl(0xAAAA0300) &&
	     *( (u_short *) &fple->snap.org[1] ) == htons(0x0000) ) {
	
	   port = ntohs(fple->snap.ethertype);
	
	   switch( port ) {
	   case ETHERTYPE_IP:
	     /* Do we need to anty cksum the headder? */
		if ( ifhip_cksum & 1 ) {
		  /* master.d/if_hip on-board TCP/UDP cksums: ifhip_cksum
		   * 0 = disabled.
		   * 1 = compute checksum on the board for received frames 
		   *     but not transmitted frames.
		   * 2 = compute checksum on the board for transmitted 
		   *     frames but notreceived frames.
		   * 3 = compute checksums for both transmitted and 
		   *     receive frames.
		   */
			struct ip *ip;
			int	hlen, slop;

			ip = (struct ip *)(fple+1);
			hlen = ip->ip_hl<<2;

			if ( ! (ip->ip_off & (IP_OFFMASK|IP_MF)) &&
			    (ip->ip_p==IPPROTO_TCP||ip->ip_p==IPPROTO_UDP)) {
				int dma_len = m1->m_len + pad_len;

				/* compute checksum of the psuedo-header
				 */
				cksum += (ip->ip_len-hlen
					  + htons((ushort)ip->ip_p)
					  + (ip->ip_src.s_addr >> 16)
					  + (ip->ip_src.s_addr & 0xffff)
					  + (ip->ip_dst.s_addr >> 16)
					  + (ip->ip_dst.s_addr & 0xffff));
				
				/* roll out DMA'ed stuff that isn't part
				 * of checksum
				 */
				cksum += 0xffff ^
					in_cksum_sub( (ushort *)fple,
					sizeof( struct fplesnap )+hlen,
					0xffff );

				slop = B2H_LE_LEN(b2hp) - ip->ip_len -
					sizeof( struct fplesnap );
				ASSERT( slop >= 0 );
				if (slop > 0 || (dma_len & 4) ) {
				   u_char *src = mtod(m1,u_char *)+
					dma_len - slop;
				
				   slop += (dma_len & 4);

				   while ( slop > 1 ) {
					slop -= 2;
					cksum += 0xffff^ *(u_short*)(src+slop);
				   }
				   if ( slop > 0  )
					cksum += 0xffff ^ src[0];
				}
				
				cksum = (cksum & 0xffff) + (cksum >> 16);
				cksum = 0xffff & (cksum + (cksum >> 16));

				if ( cksum == 0xffff )
					m->m_flags |= M_CKSUMMED;
#ifdef DEBUG
				else
					ifhip_bad_cksum_in++;
#endif
			}
		}

		netisrbit = NETISR_IP;
		ifq = &ipintrq;
		break;

	   case ETHERTYPE_ARP:

		if ( ifhip_devp->hi_if.if_flags & IFF_DEBUG )
			printf("if_hip: received an ARP packet. dropping.\n");
		break;

	   default:
		/* XXX: DLPI ? */
		break;
	   }
	}
	else {
		if ( ifhip_devp->hi_if.if_flags & IFF_DEBUG )
			printf("if_hip: Can't understand SNAP header\n");
		ifhip_devp->hi_if.if_ipackets++;
		ifhip_devp->hi_if.if_ierrors++;
		goto drop;
	}

	if ( RAWIF_SNOOPING( &ifhip_devp->hi_rawif ) &&
		snoop_match( &ifhip_devp->hi_rawif,
			     mtod(m,caddr_t), m->m_len ) ) {

		struct mbuf *m1, *ms, *mt;
		int len;		/* m bytes to copy */
		int lenoff;
		int curlen;

		IFNET_LOCK( &ifhip_devp->hi_if);

		len = m_length(m);
		lenoff = 0;
		curlen = len+sizeof(struct ifheader)+sizeof(struct snoopheader)
			+sizeof(hippi_i_t);
		if (curlen > VCL_MAX)
			curlen = VCL_MAX;
		ms = m_vget(M_DONTWAIT, MAX(curlen, sizeof(struct ifheader)
			+ sizeof(struct snoopheader) + IFHIP_SNHDR_LEN ),
			MT_DATA);
		if (0 != ms) {
			IF_INITHEADER(mtod(ms,caddr_t),
				      &ifhip_devp->hi_if,
				      sizeof(struct ifheader)
				      + sizeof(struct snoopheader)
				      + IFHIP_SNHDR_LEN );
			curlen = m_datacopy(m, lenoff,
					    curlen - sizeof(struct ifheader)
					    - sizeof(struct snoopheader)
					    - sizeof(hippi_i_t),
					    mtod(ms,caddr_t)
					    + sizeof(struct ifheader)
					    + sizeof(struct snoopheader)
					    + sizeof(hippi_i_t) );

			/* XXX: I-field isn't passed up by card so we just
			 * put in a zero.  Maybe someday.
 			 * We need to account for this in the length passed
 			 * into snoop_input() below.
			 */
			*((hippi_i_t *) (mtod(ms,caddr_t)
				      +sizeof(struct ifheader)
				      +sizeof(struct snoopheader))) = 0;
			  
			mt = ms;
			for (;;) {
				lenoff += curlen;
				len -= curlen;
				if (len <= 0)
					break;
				curlen = MIN(len,VCL_MAX);
				m1 = m_vget(M_DONTWAIT,curlen,MT_DATA);
				if (0 == m1) {
					m_freem(ms);
					ms = 0;
					break;
				}
				mt->m_next = m1;
				mt = m1;
				curlen = m_datacopy(m, lenoff, curlen,
						    mtod(m1, caddr_t));
			}
		}
		if (!ms) {
			snoop_drop( &ifhip_devp->hi_rawif, SN_PROMISC,
				   mtod(m,caddr_t), m->m_len);
		} else {
 			/* This wasn't really received but is in the message */
 			lenoff += sizeof(hippi_i_t);
			(void)snoop_input( &ifhip_devp->hi_rawif, SN_PROMISC,
					  mtod(m, caddr_t), ms,
					  (lenoff > IFHIP_SNHDR_LEN
					  ? lenoff - IFHIP_SNHDR_LEN : 0));
		}

		IFNET_UNLOCK( &ifhip_devp->hi_if);
	}

	ifhip_devp->hi_if.if_ipackets++;
	ifhip_devp->hi_if.if_ibytes += len;

	/* assume this is only going to hose our FP header. */
	IF_INITHEADER( mtod(m, caddr_t), &ifhip_devp->hi_if, 
		sizeof( struct fplesnap ) );

	if ( ifq == 0 ) {
		if ( RAWIF_DRAINING( &ifhip_devp->hi_rawif ) ) {
			drain_input( &ifhip_devp->hi_rawif,
				port, (caddr_t) &fple->le.src, m );
		}
		else {
			if ( ifhip_devp->hi_if.if_flags & IFF_DEBUG )
				printf("if_hip: no proto, port = 0x%x\n",port);
			ifhip_devp->hi_if.if_noproto++;
			goto drop;
		}
	}
	else {
		int s;

		IFQ_LOCK(ifq, s);
		if (IF_QFULL( ifq )) {
			ifhip_devp->hi_if.if_iqdrops++;
			ifhip_devp->hi_if.if_ierrors++;
			IF_DROP( ifq );
			IFQ_UNLOCK(ifq, s);
			goto drop;
		}
		IF_ENQUEUE_NOLOCK( ifq, m );
		IFQ_UNLOCK(ifq, s);
		if ( netisrbit >= 0 )
			schednetisr( netisrbit );
	}
	return;

drop:
	m_freem(m);
	return;
}


/* 
 * ifhip_ifioctl
 *
 * Function Description:
 *    Handle the hippi specific ioctl()s. ifhip_ifioctl() is usually called 
 *    to service an ifconfig(8) request.
 *   
 * Parameter Description:
 *   ifp  - is a pointer to this interface's ifnet structure,
 *   cmd  - is one of 
 *           SIOCSIFADDR SIOCSIFFLAGS SIOCSHARP SIOCGHARP SIOCDHARP SIOCGHARPTBL
 *   arg - cmd dependent interpretation
 *   
 * Returns:
 *   EINVAL - if not one of the above operations 
 *   XXX    - returned error from hiparp_ioctl()
 */

int ifhip_ifioctl( struct ifnet *ifp, int cmd, void *request )
{
#ifdef HIO_HIPPI
	ifhip_vars_t	*ifhip_devp = & ifhip_device[ ifp->if_unit ];
	hippi_vars_t	*hippi_devp = & hippi_device[ ifp->if_unit ];
#else /* XIO_HIPPI */
	ifhip_vars_t	*ifhip_devp = (ifhip_vars_t *)ifp;
	hippi_vars_t	*hippi_devp = ifhip_devp->hps_devp;
#endif
	int	error = 0;

	IFDPRINTF(ifp, ("ifhip_ifioctl(%d, ", ifp->if_unit) );

	switch (cmd) {

	case SIOCSIFADDR:
		IFDPRINTF(ifp, (" SIOCSIFADDR,"));

		if ( !(hippi_devp->hi_state & HI_ST_UP) ) {
			error = ENODEV;
			break;
		}

		/* User is setting link or protocol address.
		 */
		switch (_IFADDR_SA(request)->sa_family) {

		case AF_INET:

		        IFDPRINTF(ifp, (" AF_INET)\n"));

			/* Set my IP address
			 */
			ifp->if_flags |= (IFF_RUNNING|IFF_UP);

			/* Call ifhip_ifinit() to get hardware
			 * in correct mode.
			 */
#ifdef HIO_HIPPI
			ifhip_ifinit( ifp->if_unit );
#else /* XIO_HIPPI */
			ifhip_ifinit( ifp );
#endif
			break;

		case AF_RAW:
		        IFDPRINTF(ifp, (" AF_RAW)\n"));
			/* Set the 48-bit ULA.
			 */
			bcopy( _IFADDR_SA(request)->sa_data,
				ifhip_devp->hi_haddr.ula, HIPPI_ULA_SIZE );
			break;
		}
		break;

	case SIOCSIFFLAGS:
		IFDPRINTF(ifp, (" SIOCSIFFLAGS)\n"));

		if ( !(hippi_devp->hi_state & HI_ST_UP) ) {
			error = ENODEV;
			ifp->if_flags &= ~IFF_UP;
			IFDPRINTF(ifp, 
			  ("ifhip_ioctl:SIOCSIFFLAGS but HW down-> exiting!\n"));
			break;
		}

		/* Call ifhip_ifinit() to get hardware
		 * in correct mode.
		 */
#ifdef HIO_HIPPI
		ifhip_ifinit( ifp->if_unit );
#else /* XIO_HIPPI */
		ifhip_ifinit( ifp );
#endif
		break;
	
		/* HIPPI specific ioctl's.
		 */
	case SIOCSHARP:
	case SIOCGHARP:
	case SIOCDHARP:
	case SIOCGHARPTBL:
		IFDPRINTF(ifp, (" SIOC [S|G|D] HARP )\n"));

		/* HIPPI address resolution protocol 
		 */
		error = harpioctl( cmd, (struct harpreq *) request );
		break;

	default:
		IFDPRINTF(ifp, (" default -> EINVAL)\n"));
		error = EINVAL;
		break;
	}

	return error;
}


/* ifhip_output()
 *
 * Function Description:
 *   For every message, the interface needs to resolve the I-field from
 *   protocol address, tack on the appropriate FP-LE headers, and send the
 *   packet.
 *
 * Callers:
 *   This gets called for every packet to be sent over the interface.
 *   usually from ip_output - dereferenced through the ifnet output function ptr.
 *
 * Parameter Description:
 *   ifp is a pointer to this interface's ifnet structure,
 *   m   is the message to be sent, and 
 *   dst is the destination's protocol address.
 *
 * Returns:
 *   ENETDOWN     - if interface not in IFF_RUNNING and IFF_UP state OR
 *                - Don't have nescessary HIPARP server info, or we haven't
 *                  been able to register with it (i.e. it hasn't replied).)
 *   ENOBUFS      - no room in if_snd queue OR
 *                - no mbuf if we need a new one for headders
 *                - if the mbufs were unaligned and we couldn't get a new one to copy 
 *                  them into, aligned this time (see  ifhip_unaligned_mbuf())s
 *   0            - If ARP needs to go request it - packet will be sent/droped later
 *                  upon reply from ARPserver
 *   EHOSTUNREACH - I'm the server but don't have this client in my tables (i.e. not
 *                  registered with me. OR
 *                - Entry has been recently (< 15 min ) NAKed by HIPARP server
 *   ENOSPC       - No space left in ARP table when we wanted to start one.
 *   EAFNOSUPPORT - If it is not AF_INET
 *   XXX          - an error from ifhip_hw_write()
 *
 */

/*ARGSUSED*/
ifhip_output( struct ifnet *ifp, struct mbuf *m, struct sockaddr *dst,  
	      struct rtentry *rte)
{
#ifdef HIO_HIPPI
	ifhip_vars_t *ifhip_devp = & ifhip_device[ifp->if_unit];
	hippi_vars_t *hippi_devp = & hippi_device[ifp->if_unit];
#else /* XIO_HIPPI */
	ifhip_vars_t *ifhip_devp = (ifhip_vars_t *)ifp;
	int    s;
#endif
	struct mbuf        *m2;
	u_int               I;
	int	            length;
	I_fplesnap_t       *ifplesnap;
	int                 error = 0;
	hippi_ula_t         remote_ula;

	if ( (ifp->if_flags & (IFF_RUNNING|IFF_UP)) != (IFF_RUNNING|IFF_UP) ){
		m_freem(m);
		return ENETDOWN;
	}

#ifdef HIO_HIPPI
	if (IF_QFULL(&ifhip_devp->hi_if.if_snd)) {
		mutex_lock( & hippi_devp->devslock, PZERO );
		hippi_b2h(hippi_devp);	/* try to empty the queue */
#else /* XIO_HIPPI */
		IFQ_LOCK(&ifhip_devp->hi_if.if_snd, s);
#endif
		if (IF_QFULL(&ifhip_devp->hi_if.if_snd)) {
			m_freem(m);
			ifhip_devp->hi_if.if_odrops++;
			IF_DROP(&ifhip_devp->hi_if.if_snd);
#ifdef HIO_HIPPI
			mutex_unlock( & hippi_devp->devslock );
#else /* XIO_HIPPI */
			IFQ_UNLOCK( &ifhip_devp->hi_if.if_snd, s );
#endif
			return ENOBUFS;
		}
#ifdef XIO_HIPPI
		IFQ_UNLOCK( &ifhip_devp->hi_if.if_snd, s );
#else /* HIO_HIPPI */
		mutex_unlock( & hippi_devp->devslock );
	}
#endif
	length = m_length(m);

	/* Create HIPPI-FP/LE header.  Try to squeeze it in front
	 * of header already there.
	 */

	if ( m->m_off < MMAXOFF && (m->m_off&3)==0 &&
	     m->m_off >= (__psint_t)(sizeof(I_fplesnap_t)+MMINOFF) ) {

		ASSERT( ! M_HASCL(m) );
		
		/* Header can fit in front of first mbuf */

		m->m_off -= sizeof(I_fplesnap_t);
		m->m_len += sizeof(I_fplesnap_t);
		ifplesnap = mtod( m, I_fplesnap_t *);

		bzero((caddr_t)ifplesnap,sizeof(I_fplesnap_t));
	}
	else if ( m->m_off >= MMINOFF && m->m_off < MMAXOFF &&
		  (m->m_len&3)==0 && (m->m_off&3)==0 &&
		  m->m_len+sizeof(I_fplesnap_t) <= MLEN ) {
		
		__uint32_t	*d1,*d2;
		int 	i;

#ifdef DEBUG
		ifhip_out_hdrmov++;
		ASSERT( ! M_HASCL(m) );
#endif


		/* Header can fit if we tweak the first mbuf */

		d1 = mtod(m, __uint32_t *);
		d2 = d1 + ( MMAXOFF-m->m_len-m->m_off )/sizeof(__uint32_t);

		for ( i=(m->m_len-4)/4; i>=0; i-- )
			d2[ i ] = d1[ i ];
		
		m->m_off = MMAXOFF - m->m_len - sizeof(I_fplesnap_t);
		m->m_len +=  sizeof(I_fplesnap_t);
		ifplesnap = mtod( m, I_fplesnap_t *);

		bzero((caddr_t)ifplesnap,sizeof(I_fplesnap_t));
	}
	else {
#ifdef DEBUG
		ifhip_out_allocm++;
#endif
		m2 = m_getclr( M_DONTWAIT, MT_HEADER );
		if ( !m2 ) {
			m_freem( m );
			return ENOBUFS;
		}
		ifplesnap = mtod( m2, I_fplesnap_t *);
		m2->m_len = sizeof(I_fplesnap_t);
		m2->m_next = m;
		m2->m_flags |= (m->m_flags & M_COPYFLAGS);
		m = m2;
	}

	switch( dst->sa_family ) {
	case AF_INET:

	        /* Fill in common part of the FP-LE-SNAP header */
		ifplesnap->fp.hfp_ulp_id  = HIPPI_ULP_LE;
		ifplesnap->fp.hfp_flags   = HFP_FLAGS_P;
		ifplesnap->fp.hfp_d1d2off = 3<<HFP_D1SZ_SHFT;
		ifplesnap->fp.hfp_d2size  = length + 8; /* XXX */
		
		/* LE portion of the header: Part 1: all zero params bzero()ed above
		ifplesnap->le.fcwm     = 0; 802.2 fwd class = 0 (default), 
				            dbl wide = 0, M_Type = data = 0 
		ifplesnap->le.swat     = 0; XXX should be set: logical/src routing
		ifplesnap->le.resvd    = 0;
		ifplesnap->local_admin = 0; */

		/* LE portion of the header: Part 2: adresses*/
		bcopy( ifhip_devp->hi_haddr.ula, ifplesnap->le.src, HIPPI_ULA_SIZE);
		bcopy( ifhip_devp->hi_haddr.fields.swaddr, 
		       ifplesnap->le.src_swaddr, HIPPI_SWADDR_S);


		if( IFDEBUG( ifhip_devp) ) {
		  qprintf("Our address: \n");
		  hip_dumphipaddr(ifhip_devp->hi_haddr);
		}

		/* SNAP part of it - constant */
		*( (__uint32_t *) & ifplesnap->snap )     = htonl( 0xAAAA0300 );
		*( (__uint32_t *) & ifplesnap->snap + 1 ) = htonl( 0x00000800 );

		/*
		 * Resolve I-field and ULA from IP address.
		 */
		if (!harpresolve((struct sockaddr_in *)dst, remote_ula, &I )) {
			m_freem( m );
#ifdef DEBUG
		    if ( ifhip_devp->hi_if.if_flags & IFF_DEBUG ) {
			char buf[20];

			hip_ntoa(((struct sockaddr_in *)dst)->sin_addr, buf);

			printf( "ifhip_output: couldn't resolve I-field for:\n"
				"IP address 0x%x, %s,  %d, error = %d\n",
				((struct sockaddr_in *)dst)->sin_addr.s_addr,
				buf,
				((struct sockaddr_in *)dst)->sin_addr.s_addr,
				error);
		    }
#endif /* DEBUG */
			return EHOSTUNREACH; /* XXX: could be loop-back! */
		}
	      
		dprintf(5, ("ifhip_output: hiparp_resolve retuned: "
			    "remote_ula = %s, I = %x, error = %d \n",
			    hip_hexconv(remote_ula, HIPPI_ULA_SIZE, TRUE),
			    I, error) );

                /* Missing  pieces of the LE header: destination addreses */
		ifplesnap->I = I;
		bcopy(remote_ula, ifplesnap->le.dest, HIPPI_ULA_SIZE);
		bcopy(((char *)&I+1), ifplesnap->le.dest_swaddr,
		      HIPPI_SWADDR_S);

		if(IFDEBUG( ifhip_devp )) {
		  qprintf("Dest addr: I-Field = 0x%x,  ula  = %s \n", I,
			  hip_hexconv(remote_ula, HIPPI_ULA_SIZE, TRUE) );
		}

		if(IFDEBUG(ifhip_devp))  ifhip_dump_I_fplesnap( ifplesnap );


		/* Check for illegal mbuf chain (one that can't be DMA'ed directly).
		 */
		m2 = m;
		if ( (m2->m_off&3)==0 && (((m2->m_off+m2->m_len)&7)==0||!m2->m_next) ){
		  int	n=1;

		dprintf(5, ("ifhip_output: Illegal mbuf chain: not word aligned\n"));


		m2 = m2->m_next;
		while ( m2 && n<IFHIP_MAX_MBUF_CHAIN && 
			(m2->m_off&7)==0 && ((m2->m_len&7)==0||!m2->m_next) ) {
		  m2 = m2->m_next;
		  n++;
		}
		}
		if ( m2 ) {
		  m = ifhip_unaligned_mbuf( m );
		  if ( !m )
		    return ENOBUFS;
		}

		error = ifhip_hw_write(ifhip_devp, m, dst);
		  break;
	
	case AF_UNSPEC:
		/* Treat address like an ethernet packet.  Use destination
		 * address as desintation ULA.  Put ethertype in LLC/SNAP
		 * header.  Resolve I-field from ULA.
		 */
#define EP ((struct ether_header *)&dst->sa_data[0])
		if (!harpresolve_ula( EP->ether_dhost, &I )) {
			m_freem( m );
			return EHOSTUNREACH; /* XXX: loop-back!? */
		}

		ifplesnap->I              = I;
		ifplesnap->fp.hfp_flags   = HFP_FLAGS_P;
		ifplesnap->fp.hfp_d1d2off = 3<<HFP_D1SZ_SHFT;
		ifplesnap->fp.hfp_d2size  = length + 8; /* XXX */
		length += 32;

		bcopy( EP->ether_dhost, ifplesnap->le.dest, HIPPI_ULA_SIZE );
		bcopy( ifhip_devp->hi_haddr.ula, ifplesnap->le.src, HIPPI_ULA_SIZE );
		*( (__uint32_t *) & ifplesnap->snap ) = htonl( 0xAAAA0300 );
		*( (__uint32_t *) & ifplesnap->snap + 1 ) =
			htonl( EP->ether_type );
#undef EP
		break;

	/* XXX: AF_RAW ????  allow entire HIPPI-LE? HIPPI-FP?
	 * XXX: Andy's AF_802 or AF_ETHER???
	 */
	default:
		printf("%s%d: ifhip_output: Unsupported addr. family:0x%x.\n",
			ifp->if_name, ifp->if_unit, dst->sa_family );
		m_freem( m );
		return EAFNOSUPPORT;
	}
	return error;
}

/*
 * ifhip_hw_write()
 *
 * Function Description:
 *   The more hardware dependent write function from the ifnet layer.
 *   Assumes that the address resolution from IP -> HW address allready hapened.
 *   Calculates the chksum if nescessary and put it into mbuf.
 *   then calls hip_mbuf_write().
 *
 * Callers:
 *   1. ifhip_output()   when ARP had the address allready handy in its cache.
 *
 * Parameter Description:
 *   ifhip_devp  the IFNET HIPPI data structure (see if_hip.h)
 *   m           is the message to be sent, and 
 *   dst         is the destination's protocol address.
 *
 * Returns:
 *   EPROTONOSUPPORT - if it is not either IPPROTO_TCP or IPPROTO_UDP
 *   XXX             - the error returned from hip_mbuf_write()
 */
int
ifhip_hw_write(ifhip_vars_t *ifhip_devp, struct mbuf *m, struct sockaddr *dst)
{
	__uint32_t   	    cksum;
	struct mbuf        *m2;
	u_short	            sumoff = 0xFFFF;
	struct ifnet       *ifp = &ifhip_devp->hi_if;


	/* Let the board compute the TCP or UDP checksum.
	 * Start the checksum with the checksum of the psuedo-header
	 * and everthing else in the UDP or TCP header up to but
	 * not including the checksum.
	 */
	if ( (m->m_flags & M_CKSUMMED) && dst->sa_family == AF_INET ) {
		int	i;
		struct ip *ip;
		int hlen;

		ASSERT( m->m_len>=sizeof(I_fplesnap_t)+sizeof(struct ip));

		ip = (struct ip *) (mtod(m,caddr_t)+sizeof(I_fplesnap_t));
		hlen = ip->ip_hl<<2;

		/* compute checksum of the psuedo-header.
		 * XXX: make this more efficient...
		 */
		cksum = (ip->ip_len-hlen
			 + htons((ushort)ip->ip_p)
			 + ((ip->ip_src.s_addr >> 16) & 0xffff)
			 + (ip->ip_src.s_addr & 0xffff)
			 + ((ip->ip_dst.s_addr >> 16) & 0xffff)
			 + (ip->ip_dst.s_addr & 0xffff));

		/* compute checksum of all the stuff that is going to
		 * be DMA'ed but is NOT supposed to be in the checksum:
		 * 1) All the stuff at the beginning
		 * 2) The stuff at the end of the last mbuf rounded up to
		 *    a double word.
		 */
		cksum += 0xffff ^ in_cksum_sub( (ushort *)(mtod(m,u_long) &
			~IFHIP_OMBUF_ALIGN),
			(long)ip+hlen-(mtod(m,u_long) & ~IFHIP_OMBUF_ALIGN),
			0xffff );
		m2=m;
		while (m2->m_next)
			m2=m2->m_next;
		if ( (i= (m2->m_off+m2->m_len) & IFHIP_OMBUF_ALIGN ) != 0 ) {
			u_char	*src = mtod(m2,u_char *)+m2->m_len;

			i = IFHIP_OMBUF_ALIGN+1-i;

			while ( i > 1 ) {
				i -= 2;
				cksum += 0xffff ^ *(u_short *)(src+i);
			}
			if ( i > 0  )
				cksum += 0xffff ^ src[0];
		}

		cksum = (cksum & 0xffff) + (cksum >> 16);
		cksum = 0xffff & (cksum + (cksum >> 16));

		/* Find the UDP or TCP header after IP header, and
		 * the checksum within.
		 */
		switch (ip->ip_p) {
		case IPPROTO_TCP:
			sumoff = sizeof(I_fplesnap_t)+hlen+8*2;
			break;

		case IPPROTO_UDP:
			sumoff = sizeof(I_fplesnap_t)+hlen+3*2;
			break;

		default:
			printf("if_hip%d: can't output checksum proto %d\n",
			       ifp->if_unit, ip->ip_p );
			m_freem(m);
			return EPROTONOSUPPORT;
		}

		ASSERT( sumoff < m->m_len );

		*( mtod(m,u_short *) + (sumoff/sizeof(u_short)) ) = cksum;
	}

 	return hip_mbuf_write(ifp, m,  sumoff);
}


/* 
 * hip_mbuf_write(): 
 *
 * Function Description:
 *   takes a finished mbuf queue, puts it into the

XXX

 *   Checks IFqueu full, IF UP etc
 *   Handles RAWIFSNOOPING
 *   then fills the d2b with the next mbuf info.
 *
 * Callers:
 *   ifhip_hw_write()  - Regular packets from IP.
 *   hiparp_input()    - Used to send a reply to a reuest, since these are
 *                       non IP checksummed packets with only on e mbuf.
 *   hiparp_validate() - To send ARP messages used for validation.
 *
 * Parameter Description:
 *   ifp    - is a pointer to this interface's ifnet structure,
 *   m      - is the message to be sent, and 
 *   sumoff - is sizeof(I_fplesnap_t)+hlen+8*2 IF switch (ip->ip_p) 
 *            = case IPPROTO_TCP:   OR = case IPPROTO_UDP;
 *          - is  NOCHKSUM_SUMOFF = OxFFFF if we don't want the LINC to
 *            do cksums
 * Returns:
 *   ENETDOWN - if we can't lock the IFQ
 *   ENOBUFS  - if the if_snd queue is full
 *   0        - when we drop the packet since we can't grab the semaphore
 *              because HIPPI-FP got in the way.
 *            - If all went peachy
 */

int
hip_mbuf_write(struct ifnet *ifp, struct mbuf *m, u_short sumoff)
{
#ifdef HIO_HIPPI
	ifhip_vars_t *ifhip_devp = & ifhip_device[ifp->if_unit];
	hippi_vars_t *hippi_devp = & hippi_device[ifp->if_unit];
#else /* XIO_HIPPI */
	ifhip_vars_t *ifhip_devp = (ifhip_vars_t *)ifp;
	hippi_vars_t *hippi_devp = ifhip_devp->hps_devp;
	int    s;
#endif
	volatile hip_d2b_t *d2bp;
	int                 dma_chunks;
	paddr_t             physp;
	struct mbuf        *m2;

	dprintf(1, ("hip_mbuf_write: if_hip%d, mbuf = 0x%x\n", ifp->if_unit, m) );

#ifdef HIO_HIPPI
	mutex_lock( & hippi_devp->devslock, PZERO );
#else /* XIO_HIPPI */
	IFQ_LOCK(&ifhip_devp->hi_if.if_snd, s);
#endif
	/* Check this again now that we have the spin-lock.  This avoids the
	 * race condition where another CPU shuts down the HIPPI interface
	 * under us.
	 */
	if ( (ifp->if_flags & (IFF_RUNNING|IFF_UP)) != (IFF_RUNNING|IFF_UP) ){
#ifdef HIO_HIPPI
		mutex_unlock( & hippi_devp->devslock );
#else
		IFQ_UNLOCK( &ifhip_devp->hi_if.if_snd, s );
#endif
		m_freem( m );
		return ENETDOWN;
	}

	/* Attempt to grab source semaphore.
	 * Quietly drop packet if HIPPI-FP/PH gets in our way.
	 */
	if ( cpsema( &hippi_devp->src_sema ) == 0 ) {
#ifdef HIO_HIPPI
		mutex_unlock( & hippi_devp->devslock );
#else
		IFQ_UNLOCK( &ifhip_devp->hi_if.if_snd, s );
#endif
		m_freem( m );
		return 0;
	}

	if (IF_QFULL(&ifhip_devp->hi_if.if_snd)) {
		m_freem(m);
		ifhip_devp->hi_if.if_odrops++;
		IF_DROP(&ifhip_devp->hi_if.if_snd);
		vsema( &hippi_devp->src_sema );
#ifdef HIO_HIPPI
		mutex_unlock( & hippi_devp->devslock );
#else /* XIO_HIPPI */
		IFQ_UNLOCK( &ifhip_devp->hi_if.if_snd, s );
#endif
		return ENOBUFS;
	}

	if ( RAWIF_SNOOPING( &ifhip_devp->hi_rawif ) ) {

	   struct mbuf *m1, *ms, *mt;
	   int len;		/* m bytes to copy */
	   int lenoff;
	   int curlen;

	   IFNET_LOCK( &ifhip_devp->hi_if);

	   if ( snoop_match( &ifhip_devp->hi_rawif,
			     mtod(m,caddr_t), m->m_len ) ) {

		len = m_length(m);
		lenoff = 0;
		curlen = len+sizeof(struct ifheader)+sizeof(struct snoopheader);
		if (curlen > VCL_MAX)
			curlen = VCL_MAX;
		ms = m_vget(M_DONTWAIT, MAX(curlen, sizeof(struct ifheader)
				+ sizeof(struct snoopheader)
				+ IFHIP_SNHDR_LEN ),
			MT_DATA);
		if (0 != ms) {
			IF_INITHEADER(mtod(ms,caddr_t),
				      &ifhip_devp->hi_if,
				      sizeof(struct ifheader)
				      + sizeof(struct snoopheader)
				      + IFHIP_SNHDR_LEN );
			curlen = m_datacopy(m, lenoff,
					    curlen - sizeof(struct ifheader)
					    - sizeof(struct snoopheader),
					    mtod(ms,caddr_t)
					    + sizeof(struct ifheader)
					    + sizeof(struct snoopheader) );
			mt = ms;
			for (;;) {
				lenoff += curlen;
				len -= curlen;
				if (len <= 0)
					break;
				curlen = MIN(len,VCL_MAX);
				m1 = m_vget(M_DONTWAIT,curlen,MT_DATA);
				if (0 == m1) {
					m_freem(ms);
					ms = 0;
					break;
				}
				mt->m_next = m1;
				mt = m1;
				curlen = m_datacopy(m, lenoff, curlen,
						    mtod(m1, caddr_t));
			}
		}
		if (!ms) {
			snoop_drop( &ifhip_devp->hi_rawif, SN_PROMISC,
				   mtod(m,caddr_t), m->m_len);
		} else {
			(void)snoop_input( &ifhip_devp->hi_rawif, SN_PROMISC,
					  mtod(m, caddr_t), ms,
					  (lenoff > IFHIP_SNHDR_LEN
					  ? lenoff - IFHIP_SNHDR_LEN : 0));
		}
	    }

	    IFNET_UNLOCK( &ifhip_devp->hi_if);
	}


        /* 
	 *  Get next d2b (data 2 board) descriptor and fill it 
	 *  setup the HW descriptors with the mbuf's info
	 */ 

#ifdef HIO_HIPPI
	d2bp = hippi_devp->hi_d2bp_hd+1;
	if (d2bp>hippi_devp->hi_d2b_lim)
		d2bp = &hippi_devp->hi_d2b[0];
#else /* XIO_HIPPI */
	d2bp = D2B_NXT( hippi_devp->hi_d2b_prod );
#endif
	m2 = m;
	dma_chunks = 0;		/* # of address/len pairs */
	do {
		void *dp;
		int blen = m2->m_len;	/* length of this mbuf */

		if (0 == m2->m_len)
			continue;

		dp = mtod(m2, void *);
		physp = kvtophys(dp);

		while ( pnum(dp) !=  pnum((long)dp+blen-1) ) {
			int blen2;

			blen2 = NBPP - (physp & POFFMASK);
#ifdef HIO_HIPPI
#if _MIPS_SIM == _ABI64
			d2bp->ll = (u_long)blen2<<48 | (u_long)physp;
#else
			d2bp->l[0] = blen2<<16;
			d2bp->sg.addr = physp;
#endif
			if ( ++d2bp > hippi_devp->hi_d2b_lim )
				d2bp = &hippi_devp->hi_d2b[0];
#else /* XIO_HIPPI */
			d2bp->sg.len = blen2;
			d2bp->sg.addr = hippi_devp->dma_addr | (u_long)physp;
		        if (blen2 > HPS_PFTCH_THRESHOLD)
       			     d2bp->sg.addr |= PCI64_ATTR_PREF;
			d2bp = D2B_NXT( d2bp );
#endif
			dma_chunks++;
			blen -= blen2;
			dp = (void *)((long)dp + blen2);
			physp = kvtophys(dp);
		}
#ifdef HIO_HIPPI
#if _MIPS_SIM == _ABI64
		d2bp->ll = (u_long)blen<<48 | (u_long)physp;
#else
		d2bp->l[0] = blen<<16;
		d2bp->sg.addr = physp;
#endif
		if ( ++d2bp > hippi_devp->hi_d2b_lim )
			d2bp = &hippi_devp->hi_d2b[0];
#else /* XIO_HIPPI */
		d2bp->sg.len = blen;
		d2bp->sg.addr = hippi_devp->dma_addr | (u_long)physp;
	        if (blen > HPS_PFTCH_THRESHOLD)
       		     d2bp->sg.addr |= PCI64_ATTR_PREF;

		d2bp = D2B_NXT( d2bp );
#endif
		dma_chunks++;
	} while ( 0 != (m2=m2->m_next) );

	d2bp->hd.flags = HIP_D2B_BAD;

	ASSERT( dma_chunks <= IFHIP_MAX_MBUF_CHAIN );

	/* XXX: these should be written as single word writes instead
	 * of using structure fields.  more efficient.
	 */
#ifdef HIO_HIPPI
	hippi_devp->hi_d2bp_hd->hd.sumoff = sumoff;

	hippi_devp->hi_d2bp_hd->hd.chunks = dma_chunks;
	hippi_devp->hi_d2bp_hd->hd.stk    = HIP_STACK_LE;
	hippi_devp->hi_d2bp_hd->hd.fburst = 0;
	hippi_devp->hi_d2bp_hd->hd.flags  = HIP_D2B_RDY | HIP_D2B_IFLD;
	hippi_devp->hi_d2bp_hd            = d2bp;
#else /* XIO_HIPPI */	
	hippi_devp->hi_d2b_prod->hd.sumoff = sumoff;

	hippi_devp->hi_d2b_prod->hd.chunks = dma_chunks;
	hippi_devp->hi_d2b_prod->hd.stk    = HIP_STACK_LE;
	hippi_devp->hi_d2b_prod->hd.fburst = 0;
	hippi_devp->hi_d2b_prod->hd.flags  = HIP_D2B_RDY | HIP_D2B_IFLD;
	hippi_devp->hi_d2b_prod            = d2bp;
#endif
	IF_ENQUEUE_NOLOCK( &ifhip_devp->hi_if.if_snd, m );

	ifp->if_opackets++;
	/* XXX Those magic 32 bytes come from the old code, 
	 * still don't know why they are there */
	ifp->if_obytes += (m_length(m)+32);

	/* Wake up the board if it is asleep.
	 */
#ifdef HIO_HIPPI
	hippi_wakeup_nolock( hippi_devp );

	vsema( &hippi_devp->src_sema );
	mutex_unlock( & hippi_devp->devslock );

#else /* XIO_HIPPI */

        if (hippi_devp->hi_ssleep->flags & HIPFW_FLAG_SLEEP)
             hps_wake_src(hippi_devp);
#ifdef USE_MAILBOX
	/* tell firmware we've got some descriptors for it */
	hps_src_d2b_rdy(hippi_devp);
#endif
	vsema( &hippi_devp->src_sema );
	IFQ_UNLOCK( &ifhip_devp->hi_if.if_snd, s );

#endif /* XIO_HIPPI */

	return 0;
}


/*ARGSUSED*/
int
ifhip_watchdog( struct ifnet *ifp )
{
	return 0;
}


/*ARGSUSED*/
void
ifhip_le_odone( hippi_vars_t *hippi_devp,
		volatile struct hip_d2b_hd *hd, int status )
{
	struct mbuf *m;
#ifdef HIO_HIPPI
	ifhip_vars_t *ifhip_devp = & ifhip_device[ hippi_devp->unit ];

	IF_DEQUEUE_NOLOCK( &ifhip_devp->hi_if.if_snd, m );
	ASSERT( m != 0 );
#else /* XIO_HIPPI */
	ifhip_vars_t *ifhip_devp = hippi_devp->ifhps_devp;

	IF_DEQUEUE( &ifhip_devp->hi_if.if_snd, m );
	ASSERT( m != 0 );
	IFNET_LOCK(&ifhip_devp->hi_if);
#endif

	if ( status ) {
		if ( ifhip_devp->hi_if.if_flags & IFF_DEBUG ) {
			if ( status == B2H_OSTAT_REJ )
			   printf( "ifhip: transmit REJECT, I=0x%x\n",
				* mtod(m,hippi_i_t *) );
			else if ( status == B2H_OSTAT_TIMEO )
			   printf( "ifhip: transmit TIMEOUT, I=0x%x\n",
				* mtod(m,hippi_i_t *) );
			else
			   printf( "ifhip: send failed status = %d\n",
				status );
		}
		ifhip_devp->hi_if.if_oerrors++;
	}
#ifdef XIO_HIPPI
	IFNET_UNLOCK(&ifhip_devp->hi_if);
#endif

#ifdef DEBUG
	if ( m->m_len >= sizeof(hd) )
		bcopy( (void *)&hd, mtod(m, void *), sizeof(hd) );
#endif

	m_freem(m);
}


void
ifhip_drain( ifhip_vars_t *ifhip_devp )
{
	/* Drain HIPPI-LE input buffers
	 */
#ifdef XIO_HIPPI
	mutex_lock(&ifhip_devp->hi_mbuf_mutex, PZERO);
#endif
	while ( ifhip_devp->hi_in_sml_num > 0 ) {
		m_free( ifhip_devp->hi_in_smls[ ifhip_devp->hi_in_sml_h ] );
		ifhip_devp->hi_in_smls[ ifhip_devp->hi_in_sml_h ] = 0;
		ifhip_devp->hi_in_sml_h = (ifhip_devp->hi_in_sml_h+1)%
			HIP_MAX_SML;
		ifhip_devp->hi_in_sml_num--;
	}
	while ( ifhip_devp->hi_in_big_num > 0 ) {
		m_free( ifhip_devp->hi_in_bigs[ ifhip_devp->hi_in_big_h ] );
		ifhip_devp->hi_in_bigs[ ifhip_devp->hi_in_big_h ] = 0;
		ifhip_devp->hi_in_big_h = (ifhip_devp->hi_in_big_h+1)%
			HIP_MAX_BIG;
		ifhip_devp->hi_in_big_num--;
	}

	ifhip_devp->hi_in_sml_t = 0;
	ifhip_devp->hi_in_sml_h = 0;
	ifhip_devp->hi_in_big_t = 0;
	ifhip_devp->hi_in_big_h = 0;
#ifdef XIO_HIPPI
	mutex_unlock(&ifhip_devp->hi_mbuf_mutex);
#endif
}


/* Fill up board with input buffers.  It is assumed that interrupts are
 * off.
 */
ifhip_fillin( hippi_vars_t *hippi_devp )
{
	volatile hip_c2b_t *c2bp = 0;
	volatile hip_c2b_t *c2bp2 = 0;
	struct mbuf *m;
	int	filled = 0;
#ifdef HIO_HIPPI
	ifhip_vars_t *ifhip_devp = & ifhip_device[ hippi_devp->unit ];
#else
	ifhip_vars_t *ifhip_devp = hippi_devp->ifhps_devp;
#endif
	/* don't bother filling a down interface
	 */
	if ( ! (ifhip_devp->hi_if.if_flags & IFF_UP) )
		return 0;

#ifdef XIO_HIPPI
	mutex_lock(&ifhip_devp->hi_mbuf_mutex, PZERO);
	mutex_lock (&hippi_devp->dst_mutex, PZERO);
#endif

	/* For XIO these should be set only after the dst_mutex locked. */
	c2bp = hippi_devp->hi_c2bp;
	c2bp2 = c2bp;

	while ( ifhip_devp->hi_in_sml_num < ifhip_num_small ) {
		m = m_get( M_DONTWAIT, MT_DATA);
		if (!m) {
			goto fillexit;
		}
#ifdef DEBUG
		*mtod(m,__uint32_t *) = 0xBEADFACE;
#endif
		m->m_len = MLEN;
		ifhip_devp->hi_in_smls[ ifhip_devp->hi_in_sml_t ] = m;

		if ( ++c2bp2 >= & hippi_devp->hi_c2b[ HIP_C2B_LEN-1 ] ) {
#ifdef HIO_HIPPI
			c2bp2->c2bl[0] = HIP_C2B_WRAP<<8;
#else /* XIO_HIPPI */
			c2bp2->c2b_op = HIP_C2B_WRAP;
#endif
			c2bp2 = & hippi_devp->hi_c2b[0];
		}
#ifdef HIO_HIPPI
		c2bp2->c2bl[0] = HIP_C2B_EMPTY<<8;
#if _MIPS_SIM == _ABI64
		c2bp->c2bll = (u_long)MLEN<<48 |
			(u_long)(HIP_C2B_SML|HIP_STACK_LE)<<40 |
			(u_long) kvtophys( mtod(m,caddr_t) );
#else
		/* NOTE: the ordering of these two lines is important!
		 * You must put the "opcode" (HIP_C2B_SML) in LAST!!
		 */
		c2bp->c2b_addr = (u_int) kvtophys( mtod(m,caddr_t) );
		c2bp->c2bl[0] = (MLEN<<16)|((HIP_C2B_SML|HIP_STACK_LE)<<8);
#endif
#else /* XIO_HIPPI */
		c2bp2->c2b_op = HIP_C2B_EMPTY;
		/* NOTE: the ordering of these two lines is important!
		 * You must put the "opcode" (HIP_C2B_SML) in LAST!!
		 */
		c2bp->c2b_addr = hippi_devp->dma_addr | (u_long) kvtophys( mtod(m,caddr_t) );
		*(volatile __uint32_t *) &c2bp->c2b_param = (MLEN<<16)|((HIP_C2B_SML|HIP_STACK_LE)<<8);
#endif
		c2bp = c2bp2;

		ifhip_devp->hi_in_sml_num++;
		ifhip_devp->hi_in_sml_t = (ifhip_devp->hi_in_sml_t+1)%
			HIP_MAX_SML;
		filled++;
	}

	while ( ifhip_devp->hi_in_big_num < ifhip_num_big ) {

#if HIP_BIG_SIZE != VCL_MAX
 "oops problem with HIP_BIG_SIZE"
#endif
		m = m_vget( M_DONTWAIT, HIP_BIG_SIZE, MT_DATA );
		if (!m)
			break;
		ifhip_devp->hi_in_bigs[ ifhip_devp->hi_in_big_t ] = m;
#ifdef DEBUG
		*mtod(m,__uint32_t *) = 0xBEADFACE;
#endif
		if ( ++c2bp2 >= & hippi_devp->hi_c2b[ HIP_C2B_LEN-1 ] ) {
#ifdef HIO_HIPPI
			c2bp2->c2bl[0] = HIP_C2B_WRAP<<8;
#else
			c2bp2->c2b_op = HIP_C2B_WRAP;
#endif
			c2bp2 = & hippi_devp->hi_c2b[0];
		}
#ifdef HIO_HIPPI
		c2bp2->c2bl[0] = HIP_C2B_EMPTY<<8;
#if _MIPS_SIM == _ABI64
		c2bp->c2bll = (u_long) HIP_BIG_SIZE<<48 |
			(u_long) (HIP_C2B_BIG|HIP_STACK_LE)<<40|
			(u_long) kvtophys( mtod(m,caddr_t) );
#else
		/* NOTE: the ordering of these two lines is important!
		 * You must put the "opcode" (HIP_C2B_BIG) in LAST!!
		 */
		c2bp->c2b_addr = (u_int) kvtophys( mtod(m,caddr_t) );
		c2bp->c2bl[0] = HIP_BIG_SIZE<<16|(HIP_C2B_BIG|HIP_STACK_LE)<<8;
#endif
#else /* XIO_HIPPI */
		c2bp2->c2b_op = HIP_C2B_EMPTY;
		/* NOTE: the ordering of these two lines is important!
		 * You must put the "opcode" (HIP_C2B_BIG) in LAST!!
		 */
		c2bp->c2b_addr = hippi_devp->dma_addr | (u_long) kvtophys( mtod(m,caddr_t) );
		*(volatile __uint32_t *) &c2bp->c2b_param = 
		  HIP_BIG_SIZE<<16|(HIP_C2B_BIG|HIP_STACK_LE)<<8;
#endif
		c2bp = c2bp2;

		ifhip_devp->hi_in_big_num++;
		ifhip_devp->hi_in_big_t = (ifhip_devp->hi_in_big_t+1)%
			HIP_MAX_BIG;
		filled++;
	}
fillexit:
	hippi_devp->hi_c2bp = c2bp;
#ifdef XIO_HIPPI

#ifdef USE_MAILBOX
	/* tell firmware we've got some descriptors for it */
	hps_dst_d2b_rdy(hippi_devp);
#endif
	mutex_unlock (&hippi_devp->dst_mutex);
	mutex_unlock(&ifhip_devp->hi_mbuf_mutex);

#endif /* XIO_HIPPI */
	return filled;
}




static struct mbuf *
ifhip_unaligned_mbuf( struct mbuf *m0 )
{
	struct mbuf *m = m0;
	struct mbuf *m1,*m2;
	int	len, blen, blen1, blen2;
	caddr_t	c1,c2;

	ifhip_bad_mbufs++;

	dprintf(2, ("ifhip_unaligned_mbuf: # bad mbufs: %d\n",ifhip_bad_mbufs ));

	len = m_length(m0);
	m1 = m0;

	/* Get first mbuf in new chain.
	 */
	m = m_get( M_DONTWAIT, MT_DATA );
	if ( !m ) {
		m_freem( m0 );
		return NULL;
	}
	blen2 = MLEN;
	m->m_flags |= (m1->m_flags & M_COPYFLAGS);
	m->m_len = 0;
	c2 = mtod( m, caddr_t );
	m2 = m;

	while ( m1 ) {

		blen1 = m1->m_len;
		c1 = mtod( m1, caddr_t );

		while( blen1 > 0 ) {

			blen = MIN( blen1, blen2 );

			bcopy( c1, c2, blen );
			c1 += blen;
			c2 += blen;
			m->m_len += blen;
			blen1 -= blen;
			blen2 -= blen;
			len -= blen;

			if ( blen2 == 0 && blen1 > 0 ) {

				/* allocate an mbuf */
				if ( len > MLEN*2 ) {
				   blen2 = MIN( VCL_MAX, len );
				   m->m_next= m_vget(M_DONTWAIT,blen2,MT_DATA);
				}
				else {
				   blen2 = MLEN;
				   m->m_next= m_get( M_DONTWAIT, MT_DATA );
				}
				m = m->m_next;
				if ( ! m ) {
					/* out of bufs */
					m_freem(m2);
					m_freem(m0);
					return NULL;
				}

				c2 = mtod( m, caddr_t );
				ASSERT( ( (long)c2 & 7) == 0 );
				m->m_len = 0;
			}
		}

		m1 = m1->m_next;
	}

	m_freem( m0 );
	return m2;
}



/******************************************************************
 * HIPPI ARP Fucntions
 ******************************************************************/


harptab_t 	harptab[HARPTAB_SIZE];
lock_t		ifhip_harplock;

#define HARPTAB_HASH(a) \
        ((u_int)(a) % HARPTAB_SIZE)

#define HARPTAB_LOOK(ht,addr) { \
        int n,i; \
	i = HARPTAB_HASH(addr); \
        for (n = 0 ; n < HARPTAB_SIZE ; n++) \
                if (harptab[i].ht_iaddr.s_addr == addr) \
                        break; \
		else \
			i = (i<HARPTAB_SIZE-1) ? i+1 : 0; \
        if (n < HARPTAB_SIZE) \
		ht = & harptab[i]; \
	else \
                ht = 0; \
}


static int
harpresolve( struct sockaddr_in *destaddr, u_char *ula, u_int *I )
{
	harptab_t *ht;

	HARPTAB_LOOK(ht,destaddr->sin_addr.s_addr);

	if (ht) {
		int i;

		for (i=0; i<6; i++)
			ula[i] = ht->ht_ula[i];
		*I = ht->ht_I;

		/* since we're not using a lock to read,
		 * make sure entry is still valid after copying contents.
		 */
		if ( ht->ht_iaddr.s_addr == destaddr->sin_addr.s_addr )
			return 1;
		else
			return 0;
	}
	else
		return 0;
}

static int
harpresolve_ula( u_char *ula, u_int *I )
{
	int i, s;

	s = mutex_spinlock( & ifhip_harplock );

	for (i=0; i<HARPTAB_SIZE; i++)
		if ( ! bcmp( ula, harptab[i].ht_ula, HIPPI_ULA_SIZE ) )
			break;
	if (i<HARPTAB_SIZE) {
		*I = harptab[i].ht_I;
		mutex_spinunlock( & ifhip_harplock, s );
		return 1;
	}
	else {
		mutex_spinunlock( & ifhip_harplock, s );
		return 0;
	}
	/*NOTREACHED*/
}

static int
harpioctl( int cmd, struct harpreq *request )
{
	struct sockaddr_in *sin;
	harptab_t *ht;
	int i,n;
	int	s, error = 0;

	sin = (struct sockaddr_in *) &request->harp_pa;

	s = mutex_spinlock( & ifhip_harplock );

	switch (cmd) {
	case SIOCSHARP:
		if ( ! (request->harp_flags & HTF_PERM) ) {
			error = EINVAL;
			break;
		}

		if ( sin->sin_family != AF_INET ||
		     request->harp_ula.sa_family != AF_UNSPEC ) {
			error = EAFNOSUPPORT;
			break;
		}

		HARPTAB_LOOK(ht,sin->sin_addr.s_addr);
		if (!ht) {
			/* Find a free entry, as close to hash point
			 * as possible.
			 */
			i=HARPTAB_HASH(sin->sin_addr.s_addr);
			for (n=0; n<HARPTAB_SIZE; n++)
				if ( harptab[i].ht_iaddr.s_addr == 0 )
					break;
				else
					i=(i<HARPTAB_SIZE-1)?i+1:0;
			if ( n>=HARPTAB_SIZE ) {
				/* Table full.
				 */
				error = EADDRNOTAVAIL;	/* XXX */
				break;
			}
			ht = &harptab[i];
			/* Fill in entry with request.
			 */
			ht->ht_I = request->harp_swaddr; /* XXX: scrutinize */
			bcopy( request->harp_ula.sa_data, ht->ht_ula, 6 );
			ht->ht_flags = request->harp_flags &
						(HTF_PERM|HTF_SRCROUTE);
			ht->ht_iaddr.s_addr = sin->sin_addr.s_addr;
		}
		else {

			/* Fill in entry with request.
			 */
			ht->ht_I = request->harp_swaddr; /* XXX: scrutinize */
			bcopy( request->harp_ula.sa_data, ht->ht_ula, 6 );
			ht->ht_flags = request->harp_flags &
						(HTF_PERM|HTF_SRCROUTE);
		}

		break;

	case SIOCGHARP:

		if ( sin->sin_family != AF_INET ) {
			error = EAFNOSUPPORT;
			break;
		}

		HARPTAB_LOOK(ht,sin->sin_addr.s_addr);
		if (ht) {
			/* Fill in request with entry.
			 */
			bcopy(ht->ht_ula,request->harp_ula.sa_data,6);
			request->harp_swaddr = ht->ht_I;
			request->harp_flags = ht->ht_flags;
		}
		else {
			error = ENXIO;		/* XXX */
			break;
		}
		break;
		
	case SIOCDHARP:
		if ( sin->sin_family != AF_INET ) {
			error = EAFNOSUPPORT;
			break;
		}

		HARPTAB_LOOK(ht,sin->sin_addr.s_addr);
		if (ht)
			/* invalidate entry */
			ht->ht_iaddr.s_addr = 0;
		break;
	
	case SIOCGHARPTBL:
		if ( copyout( harptab, *(void **)request,
				sizeof(harptab) ) < 0 )
			error =  EFAULT;

		break;
		
	default:
		printf( "harpioctl: unknown cmd: %x\n", cmd );
		error = EINVAL;
	}

	mutex_spinunlock( & ifhip_harplock, s );

	return error;
}


#ifdef DEBUG
void
hhelp_idbg() {
	qprintf("idbg routines of hippi IP:\n"
		"hhelp\t\t: print this list\n"
		"hdevp [hippi_devp]\t: print hippi device struct fields\n"
		"hudevp [unit number]\t: print devp addr for unit (HIO only)\n"
		"hmbuf [mbuf addr]\t: print mbuf fields\n"
		"hlmbuf [hippi_devp]\t: print complete list of large mbufs\n"
		"hsmbuf [hippi_devp]\t: print complete list of small mbufs\n"
		"hdmbufl [hippi_devp]\t: print addrs for both sets of mbufs\n");
		
}

void
hdevp_idbg(ifhip_vars_t *hippi_devp) {

	qprintf("HIPPI device structure:\n"
		"ifnet: 0x%llx\n"
		"rawif: 0x%llx\n"
		"ula: %d\n"
#ifdef XIO_HIPPI
		"vars_t: 0x%llx\n"
		"mutex: 0x%llx\n"
#endif
		"small mbuf list: 0x%llx\n"
		"in_sml_num: %d\n"
		"in_sml_h: %d\n"
		"in_sml_t: %d\n"
		"large mbuf list: 0x%llx\n"
		"in_big_num: %d\n"
		"in_big_h: %d\n"
		"in_big_t: %d\n",
		&hippi_devp->hi_if,
		&hippi_devp->hi_rawif,
		hippi_devp->hi_haddr.ula,
#ifdef XIO_HIPPI
		hippi_devp->hps_devp,
		&hippi_devp->hi_mbuf_mutex,
#endif
		hippi_devp->hi_in_smls,
		hippi_devp->hi_in_sml_num,
		hippi_devp->hi_in_sml_h,
		hippi_devp->hi_in_sml_t,
		hippi_devp->hi_in_bigs,
		hippi_devp->hi_in_big_num,
		hippi_devp->hi_in_big_h,
		hippi_devp->hi_in_big_t);
}

#ifdef HIO_HIPPI
void
hudevp_idbg(short unit) {
	qprintf("devp for unit %d: 0x%llx\n", unit, &ifhip_device[unit]);

	hdevp_idbg(&ifhip_device[unit]);
}
#endif

void
hmbuf_idbg(struct mbuf *buf) {

	qprintf("addr: 0x%llx\n"
		"next: 0x%llx\n"
		"offset: 0x%x\n"
		"m_act: 0x%llx\n"
		"len: 0x%x "
		"flags: 0x%x "
		"type: 0x%x "
		"node: %d\n",
		buf,
		buf->m_next,
		buf->m_off,
		buf->m_act,
		buf->m_len,
		buf->m_flags,
		buf->m_type,
		buf->m_index);
}


void
d_mbufs(struct mbuf **buf_list, int num, int head, int tail, int list_size) {
	int i,
	    cur = 0;

	qprintf("Dumping mbufs from 0x%llx\n"
		"num: %d, head: %d, tail: %d",
		buf_list, num, head, tail);

	cur = head;
	for (i = 0; i < num; i++) {
		struct mbuf *buf = buf_list[cur++];
		qprintf("\nmbuf at index %d:\n", cur);
		hmbuf_idbg(buf);
		
		if (cur > list_size)
			cur = 0;
	}
}

void
hlmbuf_idbg(ifhip_vars_t *hippi_devp) {
	qprintf("Dumping large mbufs:\n");

	d_mbufs(hippi_devp->hi_in_bigs, hippi_devp->hi_in_big_num,
		hippi_devp->hi_in_big_h, hippi_devp->hi_in_big_t, HIP_MAX_BIG);
}

void
hsmbuf_idbg(ifhip_vars_t *hippi_devp) {
	qprintf("Dumping small mbufs:\n");

	d_mbufs(hippi_devp->hi_in_smls, hippi_devp->hi_in_sml_num,
		hippi_devp->hi_in_sml_h, hippi_devp->hi_in_sml_t, HIP_MAX_SML);
}

void
hdmbufl_idbg(ifhip_vars_t *hippi_devp) {
	int i;

	qprintf("Small mbuf list:\nnum: %d, head: %d, tail: %d\n",
		hippi_devp->hi_in_sml_num, hippi_devp->hi_in_sml_h,
		hippi_devp->hi_in_sml_t);
	for (i = 0; i < HIP_MAX_SML; i++) {
		qprintf("%d: 0x%llx\n", i, hippi_devp->hi_in_smls[i]);
	}

	qprintf("Large mbuf list:\nnum: %d, head: %d, tail: %d\n",
		hippi_devp->hi_in_big_num, hippi_devp->hi_in_big_h,
		hippi_devp->hi_in_big_t);
	for (i = 0; i < HIP_MAX_BIG; i++) {
		qprintf("%d: 0x%llx\n", i, hippi_devp->hi_in_bigs[i]);
	}
}
#endif /* DEBUG */
