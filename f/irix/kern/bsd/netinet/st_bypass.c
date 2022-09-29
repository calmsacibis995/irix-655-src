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
 *  Filename: st_bypass.c
 *  Description: Bypass-specific routines for the ST protocol. 'Nuff said.
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
#include "in_var.h"

#include "sys/cmn_err.h"
#include "sys/tcpipstats.h"

#include "st.h"
#include "st_var.h"
#include "st_macros.h"
#include "st_if.h"
#include "st_bufx.h"
#include "st_debug.h"


int
st_bypass_setopt(
    int op,
    struct socket *so,
    int level,
    int optname,
    struct mbuf **mp
)
{
    int error = 0;
    struct inpcb *inp = sotoinpcb(so);
    register struct stpcb *sp = NULL;
    st_ifnet_t *stifp = NULL;
    register struct mbuf *m;

    ASSERT(SOCKET_ISLOCKED(so));
    sp = sotostpcb(so);
    stifp = (st_ifnet_t *) sp->s_stifp;

    dprintf(30, ("st_bypass_setopt entered\n"));

    if ( sp == NULL ) {
	return ECONNRESET;
    }

    if ( level != IPPROTO_STP ) {
	return EPROTO;
    }

    if ( op != PRCO_SETOPT ) {
	cmn_err(CE_WARN, "st_bypass_setopt: not a setsockopt\n");
	return EINVAL;
    }

    m = *mp;

    switch ( optname ) {

	default: {
	    dprintf(0, ("Unknown option in ST-bypass-setsockopt\n"));
	    error = EINVAL;
	    return error;
	} /* NOTREACHED */ break;

	case ST_BYPASS: {
	    int flag = *mtod(m, int *);

	    if ( sp->s_vc_state != STP_VCS_DISCONNECTED ) {
		dprintf(0, (
		    "ST Bypass status may not be changed after connection setup\n"
		));
		error = EPROTO;
		return error;
	    }
	
	    if ( flag ) {
		sp->s_flags |= STP_SF_BYPASS;
	    } else {
		sp->s_flags &= ~STP_SF_BYPASS;
	    }

	} break;	/* ST_BYPASS */

	case ST_L_KEY: {
	    uint32_t lkey = *mtod(m, uint32_t *);

	    if ( ! (sp->s_flags & STP_SF_BYPASS) ) {
		dprintf(0, (
		    "ST key value may only be changed on BYPASS jobs\n"
		));
		error = EPROTO;
		return error;
	    }

	   if ( sp->s_vc_state != STP_VCS_DISCONNECTED ) {
		dprintf(0, (
		    "ST key value may not be changed after connection setup\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    if ( lkey ) {
		sp->s_flags |= STP_SF_USERKEY;
		sp->s_vcd.vc_lkey = lkey;
	    } else {
		sp->s_flags &= ~STP_SF_USERKEY;
		sp->s_vcd.vc_lkey = 0;
	    }

	} break;	/* ST_L_KEY */

	case ST_L_NUMSLOTS: {
	    uint32_t slots = *mtod(m, uint32_t *);

	    if ( ! (sp->s_flags & STP_SF_BYPASS) ) {
		dprintf(0, (
		    "Number of slots may only be changed on BYPASS jobs\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    if ( sp->s_vc_state != STP_VCS_DISCONNECTED ) {
		dprintf(0, (
		    "Number of slots may not be changed after connection setup\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    if ( slots >= 0xffff ) {
		/* Ultimately, this could change.  SHAC
		 * supports DDQs far larger than 64k slots.
		 * This should probably come from the s_stifp.
		 *
		 * 0xffff is special in ST, it mean no slot
		 * accounting.
		 */
		dprintf(0, (
		    "Number of slots may not be greater than 65534\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    if ( slots && !(slots & 0xf) ) {
		dprintf(0, (
		    "Number of slots may not be divisible by 16\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    if ( slots ) {
		sp->s_flags |= STP_SF_USERSLOTS;
		sp->s_vcd.vc_max_lslots = slots;
		sp->s_vcd.vc_true_max_lslots = slots;
	    } else {
		sp->s_flags &= ~STP_SF_USERSLOTS;
		sp->s_vcd.vc_max_lslots = ST_DEFAULT_NUM_SLOTS;
		sp->s_vcd.vc_true_max_lslots = ST_DEFAULT_NUM_SLOTS;
	    }

	} break;	/* ST_L_NUMSLOTS */

	case ST_V_NUMSLOTS: {
	    uint32_t slots = *mtod(m, uint32_t *);

	    if ( ! (sp->s_flags & STP_SF_BYPASS) ) {
		dprintf(0, (
		    "Number of exported slots may only be changed on BYPASS jobs\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    if ( sp->s_vc_state != STP_VCS_DISCONNECTED ) {
		dprintf(0, (
		    "Number of exported slots may not be changed after connection setup\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    if ( slots > 0xffff ) {
		dprintf(0, (
		    "Number of exported slots may not be greater than 65535\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    if ( slots ) {
		sp->s_flags |= STP_SF_USERVISSLOTS;
		sp->s_vcd.vc_vslots = slots;
	    } else {
		sp->s_flags &= ~STP_SF_USERVISSLOTS;
		sp->s_vcd.vc_vslots = slots;
	    }

	} break;	/* ST_V_NUMSLOTS */

	case ST_BUFX_ALLOC: {
	    st_bufx_alloc_args_t *args;
	    uint                  kbase = 0;
	    uint64_t              kbase64 = 0;
	    uint                  kcookie = 0;
	    uint64_t              kcookie64 = 0;
	    uint64_t              range = 0;
	    char                 *uvbase = NULL;
	    char                 *uvcookie = NULL;

	    args = mtod(m, st_bufx_alloc_args_t *);
	    range = args->bufx_range;
	    uvbase = (char *)args->bufx_base_addr;
	    uvcookie = (char *)args->bufx_cookie_addr;

	    /*
	    printf("ST_BUFX_ALLOC:       bufx_range = %d (0x%x)\n", range, range);
	    printf("ST_BUFX_ALLOC:    bufx_base_ptr = 0x%x\n", uvbase);
	    printf("ST_BUFX_ALLOC: bufx_cookie_addr = 0x%x\n", uvcookie);
	    */

	    if ( ! (sp->s_flags & STP_SF_BYPASS) ) {
		dprintf(0, (
		    "ST_BUFX_ALLOC: Bufx's may only be allocated for BYPASS jobs\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "ST_BUFX_ALLOC: Bufx's may only be allocated after a connection is established\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    if (
		(uvbase == NULL)
		|| !IS_KUSEG(uvbase)
		|| !IS_KUSEG(uvbase+sizeof(uint64_t))
		|| (uvcookie == NULL)
		|| !IS_KUSEG(uvcookie)
		|| !IS_KUSEG(uvcookie+sizeof(uint64_t))
	    ) {
		dprintf(0, (
		    "ST_BUFX_ALLOC: Invalid user pointer\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    error = st_bufx_alloc(
		&kbase,		/* base_bufx */
		range,		/* num_bufx */
		NBPP,		/* bufsz */
		&sp->s_ifp,	/* ifs */
		1,		/* num_ifs */
		1,		/* spray_width */
		&kcookie	/* cookie */
	    );

	    if ( !error ) {
		kbase64 = kbase;
		/*
		printf("ST_BUFX_ALLOC: bufx_base=0x%x\n", kbase);
		*/
		error = copyout(&kbase64, uvbase, sizeof(uint64_t));
	    }

	    if ( !error ) {
		kcookie64 = kcookie;
		/*
		printf("ST_BUFX_ALLOC: bufx_cookie=0x%x\n", kcookie);
		*/
		error = copyout(&kcookie64, uvcookie, sizeof(uint64_t));
	    }

	} break;	/* ST_BUFX_ALLOC */

	case ST_BUFX_FREE: {
	    st_bufx_alloc_args_t *args;
	    uint64_t range;
	    uint64_t kbase;
	    uint64_t kcookie;

	    args = mtod(m, st_bufx_alloc_args_t *);
	    range = args->bufx_range;
	    kbase = args->bufx_base;
	    kcookie = args->bufx_cookie;

	    /*
	    printf("ST_BUFX_FREE:  bufx_range = 0x%x\n", range);
	    printf("ST_BUFX_FREE:   bufx_base = 0x%x\n", kbase);
	    printf("ST_BUFX_FREE: bufx_cookie = 0x%x\n", kcookie);
	    */

	    if ( ! (sp->s_flags & STP_SF_BYPASS) ) {
		dprintf(0, (
		    "ST_BUFX_FREE: Bufx's may only be freed for BYPASS jobs\n"
		));
		printf( "ST_BUFX_FREE: Bufx's may only be freed for BYPASS jobs\n");
		error = EPROTO;
		return error;
	    }
	    
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "ST_BUFX_FREE: Bufx's may only be freed after a connection is established\n"
		));
		printf( "ST_BUFX_FREE: Bufx's may only be freed after a connection is established\n");
		error = EPROTO;
		return error;
	    }
	    
	    error = st_bufx_free(
		kbase,		/* bufx_base */
		range,		/* num_bufx */
		&sp->s_ifp,	/* ifs */
		1,		/* num_ifs */
		kcookie		/* cookie */
	    );

	    if ( error ) {
		printf("ST_BUFX_FREE: st_bufx_free() returned error\n");
		return error;
	    }

	} break;	/* ST_BUFX_FREE */

	case ST_BUFX_MAP: {
	    alenaddr_t		addr;
	    uio_t               auio;
	    iovec_t             aiov;
	    alenlist_t          alen;
	    st_bufx_map_args_t *args;
	    uint64_t            base;
	    char               *buffer;
	    uint                buflen;
	    size_t		bufsize;
	    uint64_t            bufx_cookie;
	    opaque_t            dma_cookie;
	    int                 dma_direction;
	    uint64_t            flags;
	    uint16_t            kmx;
	    uint64_t            length;
	    uint                log_bufsz;
	    st_mx_t             mx;
	    int                 num_frags;
	    st_buf_t            payload;
	    paddr_t            *pbuf;
	    uint64_t            range;
	    st_tx_t             tx;
	    char               *uvmx;

	    args = mtod(m, st_bufx_map_args_t *);

	    bufsize = sp->s_vcd.vc_lbufsize ? (1 << sp->s_vcd.vc_lbufsize) : (1 << ST_LOG_BUFSZ);

	    base = args->bufx_base;
	    range = args->bufx_range;
	    flags = args->flags;
	    buffer = (char *)args->buffer;
	    length = args->length;
	    bufx_cookie = args->bufx_cookie;
	    uvmx = (char *)args->mx_addr;
	    bufx_cookie = args->bufx_cookie;

	    /*
	    printf("ST_BUFX_MAP:       flags = 0x%x\n", flags);
	    printf("ST_BUFX_MAP:      buffer = 0x%x\n", buffer);
	    printf("ST_BUFX_MAP:      length = %d (0x%x)\n", length, length);
	    printf("ST_BUFX_MAP:        base = %d (0x%x)\n", base, base);
	    printf("ST_BUFX_MAP: bufx_cookie = %d (0x%x)\n", bufx_cookie, bufx_cookie);
	    printf("ST_BUFX_MAP:     mx_addr = 0x%x\n", uvmx);
	    */

	    if ( ! (sp->s_flags & STP_SF_BYPASS) ) {
		dprintf(0, (
		    "ST_BUFX_MAP: Bufx's may only be mapped for BYPASS jobs\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "ST_BUFX_MAP: Bufx's may only be mapped after a connection is established\n"
		));
		error = EPROTO;
		return error;
	    }
	    
	    if (
		(uvmx == NULL)
		|| !IS_KUSEG(uvmx)
		|| !IS_KUSEG(uvmx+sizeof(uint64_t))
		|| (buffer == NULL)
		|| !IS_KUSEG(buffer)
		|| !IS_KUSEG(buffer+length)
	    ) {
		dprintf(0, (
		    "ST_BUFX_MAP: Invalid user pointer\n"
		));
		error = EPROTO;
		return error;
	    }

	    if ( length == 0 ) {
		dprintf(0, (
		    "ST_BUFX_MAP: Can't pin 0 bytes, can you?\n"
		));
		error = EPROTO;
		return error;
	    }

	    if (
		( (length <= 32) && (((__psint_t)buffer) & (CACHE_SLINE_SIZE-1)) )
		||  (((__psint_t)buffer) & 0x7)
	    ) {
		dprintf(0, (
		    "ST_BUFX_MAP: Can't pin 0 bytes, can you?\n"
		));
		error = EPROTO;
		return error;
	    }

	    /* This junk was just a mnemonic device to help me follow the
	     * variable names as they go from send() down through
	     * st_setup_buf().  It will eventually go away.
	     */

	    aiov.iov_base = buffer;
	    aiov.iov_len = length;
	    auio.uio_iov = &aiov;
	    auio.uio_iovcnt = 1;
	    auio.uio_segflg = UIO_USERSPACE;
	    auio.uio_offset = 0;
	    auio.uio_resid = aiov.iov_len;
	    auio.uio_sigpipe = 0;
	    auio.uio_pio = 0;
	    auio.uio_pbuf = 0;
	    auio.uio_resid = length;
	    payload.bufx_flags = BUF_ADDR;
	    payload.payload_len = auio.uio_resid;
	    payload.uio = &auio;
	    tx.tx_tlen = payload.payload_len;
	    tx.tx_buf.uio = payload.uio;
	    tx.tx_spray_width = 1;
	    tx.tx_ACKed = 0;
	    tx.tx_buf.bufx_flags = BUF_NONE;
	    tx.tx_buf.payload_len = tx.tx_buf.uio->uio_resid;
	    tx.tx_buf.test_bufaddr = tx.tx_buf.uio->uio_iov->iov_base;

	    if ( flags & ST_BUFX_MAP_TX ) {
		dma_direction = B_READ;
	    } else if ( flags & ST_BUFX_MAP_RX) {
		dma_direction = B_WRITE;
	    } else {
		error = EPROTO;
		return error;
	    }

	    alen = alenlist_create(AL_LEAVE_CURSOR);

	    buflen = uio_to_frag_size(auio.uio_iov, auio.uio_iovcnt, &alen, dma_direction, &dma_cookie);

	    if ( buflen == -1 ) {
		dprintf(0, ("ST_BUFX_MAP: uio_to_frag_size() returned -1\n"));
		alenlist_destroy(alen);
		error = EINVAL;
		return error;
	    }

	    if ( buflen < bufsize ) {
		dprintf(0, ("ST_BUFX_MAP: buflen < bufsize\n"));
		alenlist_destroy(alen);
		error = EINVAL;
		return error;
	    } else {
		buflen = bufsize;
	    }

	    if ( buflen < NBPP ) {
		dprintf(0, ("ST_BUFX_MAP: buflen < NBPP\n"));
		alenlist_destroy(alen);
		error = EINVAL;
		return error;
	    }
	    log_bufsz = log2(buflen);

	    ASSERT_ALWAYS(ALENLIST_SUCCESS == alenlist_cursor_init(alen, 0, NULL));
	    num_frags = 0;
	    while(ALENLIST_SUCCESS == (alenlist_get(alen, NULL, 0, &addr, &length, 0))) {
		if ( length % buflen || ((buflen-1) & addr) ) {
		    num_frags += length/buflen + 1;
		} else {
		    num_frags += length/buflen;
		}
		dprintf(20, ("paddr 0x%x, len %u, num_frags made %u\n", addr, length, num_frags));
	    }

	    /* There needs to be a test that the buffer fits in the
	     * bufx range we have.
	     *
	     * Something like (num_frags <= range)
	     */

	    tx.tx_buf.num_bufx = num_frags;
	    tx.tx_buf.bufx_flags |= BUF_BUFX;

	    error = st_bufx_map(
		base,		/* bufx_base */
		auio.uio_iov,	/* iov */
		auio.uio_iovcnt,/* iovcnt */
		bufx_cookie,	/* cookie */
		dma_direction,	/* xfer_dir */
		&dma_cookie,	/* dma_cookies */
		buflen,		/* min_frag_size */
		&alen		/* alen */
	    );

	    if ( !error ) {
		tx.tx_buf.bufx_flags |= BUF_MAPPED;

		num_frags = st_bufx_to_nfrags(base, bufx_cookie);
		/* printf("ST_BUFX_MAP: num_frags = %d (should be 1)\n", num_frags); */

		pbuf = kmem_zalloc(num_frags * sizeof(paddr_t *), KM_SLEEP);
		ASSERT(pbuf);
		error = st_bufx_to_phys(pbuf, base, bufx_cookie);
		ASSERT(!error);
	    } else {
		panic("st_bufx_map()");
	    }

	    if ( !error ) {
		buflen = st_bufx_to_frag_size(base, bufx_cookie);
		/* printf("ST_BUFX_MAP: buflen = %d (0x%x)\n", buflen, buflen); */

		kmx = INVALID_R_Mx;

		if ( flags & ST_BUFX_MAP_RX) {
		    mx.base_spray = 0;
		    mx.bufsize = log_bufsz;
		    mx.bufx_base = base;
		    mx.key = sp->s_vcd.vc_lkey;
		    mx.bufx_range = num_frags;
		    mx.flags = 0;
		    mx.poison = 0;
		    mx.port = sp->s_vcd.vc_lport;
		    mx.stu_num = 0;

		    kmx = sp->s_R_Mx_alloc(sp);

		    if ( kmx == (uint16_t)INVALID_R_Mx ) {
			printf("ST_BUFX_MAP: Got a bad Mx\n");
		    } else {
			error = (*stifp->if_st_set_mx)(sp->s_ifp, kmx, &mx);
		    }
		}

		error = (*stifp->if_st_set_bufx)(
		    sp->s_ifp,
		    base,
		    pbuf,
		    num_frags,
		    log_bufsz,
		    ST_BUFX_ALLOW_SEND,
		    sp->s_vcd.vc_lport
		);

	    } else {
		panic("st_bufx_to_phys()");
	    }

	    if ( !error ) {
		error = copyout(&kmx, uvmx, sizeof(uint16_t));
	    } else {
		panic("if_st_set_bufx/if_st_set_mx");
	    }

	} break;	/* ST_BUFX_MAP */

	case ST_BUFX_UNMAP: {
	    st_bufx_map_args_t *args;
	    uint64_t            base;
	    char               *buffer;
	    uint64_t            bufx_cookie;
	    int                 dma_direction;
	    uint64_t            flags;
	    uint16_t            kmx;
	    uint64_t            length;
	    int                 num_frags;
	    char               *uvmx;
	    uint32_t            xfer_dir;

	    args = mtod(m, st_bufx_map_args_t *);

	    flags = args->flags;
	    buffer = (char *)args->buffer;
	    length = args->length;
	    base = args->bufx_base;
	    bufx_cookie = args->bufx_cookie;
	    uvmx = (char *)args->mx_addr;
	    kmx = args->mx;

	    /*
	    printf("ST_BUFX_UNMAP:       flags = 0x%x\n", flags);
	    printf("ST_BUFX_UNMAP:      buffer = 0x%x\n", buffer);
	    printf("ST_BUFX_UNMAP:      length = 0x%x\n", length);
	    printf("ST_BUFX_UNMAP:   bufx_base = 0x%x\n", base);
	    printf("ST_BUFX_UNMAP: bufx_cookie = 0x%x\n", bufx_cookie);
	    printf("ST_BUFX_UNMAP:          mx = 0x%x\n", kmx);
	    */

	    if ( flags & ST_BUFX_MAP_TX ) {
		dma_direction = B_READ;
		xfer_dir = ST_BUFX_ALLOW_SEND;
	    } else if ( flags & ST_BUFX_MAP_RX) {
		dma_direction = B_WRITE;
		xfer_dir = ST_BUFX_ALLOW_RECV;
	    } else {
		printf("ST_BUFX_UNMAP: bad flags\n");
		error = EPROTO;
		return error;
	    }

	    num_frags = st_bufx_to_nfrags(base, bufx_cookie);
	    ASSERT(num_frags);

	    if (
		(xfer_dir == ST_BUFX_ALLOW_RECV)
		&& stifp
		&& stifp->if_st_clear_mx
		&& (kmx != (uint16_t)INVALID_R_Mx) 
	    ) {
		ASSERT_ALWAYS(0 == (*stifp->if_st_clear_mx)(sp->s_ifp, kmx));
		sp->s_R_Mx_free(sp, kmx);
	    }

	    if ( stifp && stifp->if_st_clear_bufx ) {
		ASSERT_ALWAYS(0 == (*stifp->if_st_clear_bufx)(
		    sp->s_ifp, base, num_frags, xfer_dir
		));
	    }

	    if ( st_bufx_unmap(base, num_frags, bufx_cookie, dma_direction) ) {
		cmn_err(CE_PANIC, "st_bufx_unmap error!\n");
	    }

	} break;	/* ST_BUFX_UNMAP */

    } /* switch */

    if ( error ) {
	printf("st_bypass_setopt about to return error = %d, optname = %d\n", error, optname);
    }
    return error;

} /* st_bypass_setopt() */


int
st_bypass_getopt(
    int op,
    struct socket *so,
    int level,
    int optname,
    struct mbuf **mp
)
{
    int error = 0;
    struct inpcb *inp = sotoinpcb(so);
    register struct stpcb *sp = NULL;
    st_ifnet_t *stifp = NULL;
    register struct mbuf *m;

    ASSERT(SOCKET_ISLOCKED(so));
    sp = sotostpcb(so);
    stifp = (st_ifnet_t *) sp->s_stifp;

    dprintf(30, ("st_bypass_getopt entered\n"));

    if ( sp == NULL ) {
	return ECONNRESET;
    }

    if ( level != IPPROTO_STP ) {
	return EPROTO;
    }

    if ( op != PRCO_GETOPT ) {
	cmn_err(CE_WARN, "st_bypass_getopt: not a getsockopt\n");
	return EINVAL;
    }

    m = *mp;

    switch ( optname ) {

	default: {
	    dprintf(0, ("Unknown option in ST-bypass-setsockopt\n"));
	    error = EINVAL;
	    return error;
	} /* NOTREACHED */ break;

	case ST_BYPASS: {
	    uint flag;

	    flag = (sp->s_flags & STP_SF_BYPASS) ? 1 : 0;
	    *mtod(m, uint *) = flag;
	    m->m_len = sizeof(uint);
	} break; /* ST_BYPASS */

	case ST_L_KEY: {
	    *mtod(m, uint32_t *) = sp->s_vcd.vc_lkey;
	    m->m_len = sizeof(uint32_t);
	} break; /* ST_L_KEY */

	case ST_R_KEY: {
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "remote key not available until connected\n"
		));
		error = EPROTO;
	    } else {
		*mtod(m, uint32_t *) = sp->s_vcd.vc_rkey;
		m->m_len = sizeof(uint32_t);
	    }
	} break; /* ST_R_KEY */

	case ST_L_PORT: {
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "local port not available until connected\n"
		));
		error = EPROTO;
	    } else {
		*mtod(m, uint32_t *) = sp->s_vcd.vc_lport;
		m->m_len = sizeof(uint32_t);
	    }
	} break; /* ST_L_PORT */

	case ST_R_PORT: {
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "remote port not available until connected\n"
		));
		error = EPROTO;
	    } else {
		*mtod(m, uint32_t *) = sp->s_vcd.vc_rport;
		m->m_len = sizeof(uint32_t);
	    }
	} break; /* ST_R_PORT */

	case ST_L_NUMSLOTS: {
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "local num slots not available until connected\n"
		));
		error = EPROTO;
	    } else {
		*mtod(m, uint32_t *) = sp->s_vcd.vc_true_max_lslots;
		if ( sp->s_vcd.vc_true_max_lslots == 0 ) {
		    printf("Error in ST_L_NUMSLOTS, dumping...\n");
		    st_dump_pcb(sp);
		}
		m->m_len = sizeof(uint32_t);
	    }
	} break; /* ST_L_NUMSLOTS */

	case ST_R_NUMSLOTS: {
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "remote num slots not available until connected\n"
		));
		error = EPROTO;
	    } else {
		*mtod(m, uint32_t *) = sp->s_vcd.vc_true_max_rslots;
		m->m_len = sizeof(uint32_t);
	    }
	} break; /* ST_R_NUMSLOTS */

	case ST_V_NUMSLOTS: {
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "remote num slots not available until connected\n"
		));
		error = EPROTO;
	    } else {
		*mtod(m, uint32_t *) = sp->s_vcd.vc_vslots;
		m->m_len = sizeof(uint32_t);
	    }
	} break; /* ST_V_NUMSLOTS */

	case ST_L_MAXSTU: {
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "local max STU size not available until connected\n"
		));
		error = EPROTO;
	    } else {
		*mtod(m, uint32_t *) = sp->s_vcd.vc_lmaxstu;
		m->m_len = sizeof(uint32_t);
	    }
	} break; /* ST_L_MAXSTU */

	case ST_R_MAXSTU: {
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "remote max STU size not available until connected\n"
		));
		error = EPROTO;
	    } else {
		*mtod(m, uint32_t *) = sp->s_vcd.vc_rmaxstu;
		m->m_len = sizeof(uint32_t);
	    }
	} break; /* ST_R_MAXSTU */

	case ST_L_BUFSIZE: {
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "local buf size not available until connected\n"
		));
		error = EPROTO;
	    } else {
		*mtod(m, uint32_t *) = (sp->s_vcd.vc_lbufsize) ?
		    sp->s_vcd.vc_lbufsize : ST_LOG_BUFSZ;
		m->m_len = sizeof(uint32_t);
	    }
	} break; /* ST_L_BUFSIZE */

	case ST_R_BUFSIZE: {
	    if ( sp->s_vc_state != STP_VCS_CONNECTED ) {
		dprintf(0, (
		    "remote buf size not available until connected\n"
		));
		error = EPROTO;
	    } else {
		*mtod(m, uint32_t *) = sp->s_vcd.vc_rbufsize;
		m->m_len = sizeof(uint32_t);
	    }
	} break; /* ST_R_BUFSIZE */

    } /* switch */

    return error;

} /* st_bypass_setopt() */
