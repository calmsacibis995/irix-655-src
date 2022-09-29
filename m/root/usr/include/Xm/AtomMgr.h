/* 
 *  @OSF_COPYRIGHT@
 *  COPYRIGHT NOTICE
 *  Copyright (c) 1990, 1991, 1992, 1993 Open Software Foundation, Inc.
 *  ALL RIGHTS RESERVED (MOTIF). See the file named COPYRIGHT.MOTIF for
 *  the full copyright text.
*/ 
/* 
 * HISTORY
 * $Log: AtomMgr.h,v $
 * Revision 0.5  1996/05/15 19:18:55  kenton
 * remove DragBS atom pairs stuff, use XInternAtoms instead
 *
 * Revision 1.6.4.3  1993/09/16  22:13:02  drk
 * 	Delete the XM_ATOM_CACHE symbol.
 * 	[1993/09/16  22:12:15  drk]
 *
 * Revision 1.6.4.2  1993/08/31  18:15:29  drk
 * 	Expunge _XmInitAtomPairs().
 * 	[1993/08/31  18:14:34  drk]
 * 
 * Revision 1.6.2.3  1993/07/23  02:45:24  yak
 * 	Expended copyright marker
 * 	[1993/07/23  01:10:00  yak]
 * 
 * Revision 1.6.2.2  1993/07/02  18:59:06  daniel
 * 	mv XM_ATOM_CACHE in XmP.h
 * 	[1993/07/02  18:57:56  daniel]
 * 
 * Revision 1.6  1992/03/13  16:23:18  devsrc
 * 	Converted to ODE
 * 
 * $EndLog$
*/ 
/*   $RCSfile: AtomMgr.h,v $ $Revision: 0.5 $ $Date: 1996/05/15 19:18:55 $ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmAtomMgr_h
#define _XmAtomMgr_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* As of X11r5 XInternAtom is cached by Xlib, so we can use it directly. */

#define XmInternAtom(display, name, only_if_exists) \
		XInternAtom(display, name, only_if_exists)
#define XmGetAtomName(display, atom) \
		XGetAtomName(display, atom)

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

/* This macro name is confusing, and of unknown benefit.
 * #define XmNameToAtom(display, atom) \
 *      XmGetAtomName(display, atom)
 */

#endif /* _XmAtomMgr_h */
