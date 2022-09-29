
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
#include "sys/errno.h"

#include "in.h"
#include "in_systm.h"
#include "ip.h"
#include "in_pcb.h"
#include "in_var.h"
#include "ip_icmp.h"

#include "sys/tcpipstats.h"
#include <sys/cmn_err.h>

#include "st.h"
#include "st_var.h"
#include "st_bufx.h"
#include "st_debug.h"
#include "st_if.h"


void st_if_attach(struct ifnet *ifp, st_ifnet_t *st_ifp) {
  /*
  ifp->st_ifnet = (void *) st_ifp;
  */
  register_an_interface(ifp, st_ifp);
}

