/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile: Protocols.h,v $ $Revision: 0.9 $ $Date: 1996/09/17 05:08:40 $ */
/*
*  (c) Copyright 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmProtocols_h
#define _XmProtocols_h

#include <Xm/Xm.h>
#include <Xm/AtomMgr.h>

#include <X11/Xatom.h>  /* This defines _SGI_EXTRA_PREDEFINES if fast atoms are supported. */
#if defined(_SGI_EXTRA_PREDEFINES) && defined(FAST_ATOMS)
#include <X11/SGIFastAtom.h>
#else
#ifndef XSGIFastInternAtom
#define XSGIFastInternAtom(dpy,string,fast_name,how) XInternAtom(dpy,string,how)
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* should be in XmP.h */

#ifndef XmCR_WM_PROTOCOLS
#define XmCR_WM_PROTOCOLS 6666
#endif /* XmCR_WM_PROTOCOLS */

/* define the XM_PROTOCOLS atom for use in  routines */
#if defined(_SGI_EXTRA_PREDEFINES) && defined(FAST_ATOMS)
#define XM_WM_PROTOCOL_ATOM(shell) \
    XSGIFastInternAtom(XtDisplay(shell), "WM_PROTOCOLS", SGI_XA_WM_PROTOCOLS, FALSE)
#else
#define XM_WM_PROTOCOL_ATOM(shell) \
    XmInternAtom(XtDisplay(shell),"WM_PROTOCOLS",FALSE)
#endif


#define XmAddWMProtocols(shell, protocols, num_protocols) \
      XmAddProtocols(shell, XM_WM_PROTOCOL_ATOM(shell), \
			 protocols, num_protocols)

#define XmRemoveWMProtocols(shell, protocols, num_protocols) \
      XmRemoveProtocols(shell, XM_WM_PROTOCOL_ATOM(shell), \
			protocols, num_protocols)

#define XmAddWMProtocolCallback(shell, protocol, callback, closure) \
      XmAddProtocolCallback(shell, XM_WM_PROTOCOL_ATOM(shell), \
			    protocol, callback, closure)

#define XmRemoveWMProtocolCallback(shell, protocol, callback, closure) \
  XmRemoveProtocolCallback(shell, XM_WM_PROTOCOL_ATOM(shell), \
			    protocol, callback, closure)

#define XmActivateWMProtocol(shell, protocol) \
      XmActivateProtocol(shell, XM_WM_PROTOCOL_ATOM(shell), protocol)

#define XmDeactivateWMProtocol(shell, protocol) \
      XmDeactivateProtocol(shell, XM_WM_PROTOCOL_ATOM(shell), protocol)

#define XmSetWMProtocolHooks(shell, protocol, pre_h, pre_c, post_h, post_c) \
      XmSetProtocolHooks(shell, XM_WM_PROTOCOL_ATOM(shell), \
			 protocol, pre_h, pre_c, post_h, post_c)


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern void XmAddProtocols() ;
extern void XmRemoveProtocols() ;
extern void XmAddProtocolCallback() ;
extern void XmRemoveProtocolCallback() ;
extern void XmActivateProtocol() ;
extern void XmDeactivateProtocol() ;
extern void XmSetProtocolHooks() ;

#else

extern void XmAddProtocols( 
                        Widget shell,
                        Atom property,
                        Atom *protocols,
                        Cardinal num_protocols) ;
extern void XmRemoveProtocols( 
                        Widget shell,
                        Atom property,
                        Atom *protocols,
                        Cardinal num_protocols) ;
extern void XmAddProtocolCallback( 
                        Widget shell,
                        Atom property,
                        Atom proto_atom,
                        XtCallbackProc callback,
                        XtPointer closure) ;
extern void XmRemoveProtocolCallback( 
                        Widget shell,
                        Atom property,
                        Atom proto_atom,
                        XtCallbackProc callback,
                        XtPointer closure) ;
extern void XmActivateProtocol( 
                        Widget shell,
                        Atom property,
                        Atom proto_atom) ;
extern void XmDeactivateProtocol( 
                        Widget shell,
                        Atom property,
                        Atom proto_atom) ;
extern void XmSetProtocolHooks( 
                        Widget shell,
                        Atom property,
                        Atom proto_atom,
                        XtCallbackProc pre_hook,
                        XtPointer pre_closure,
                        XtCallbackProc post_hook,
                        XtPointer post_closure) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmProtocols_h */
