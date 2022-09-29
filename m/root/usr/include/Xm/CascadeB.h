/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: CascadeB.h,v $ $Revision: 0.5 $ $Date: 1996/06/04 20:38:12 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmCascadeB_h
#define _XmCascadeB_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  K.Lee - Resource definitions for SGI "menu bar repeat" extension
 *  set XmNmenuBarRepeatTimeout to >0 (milliseconds) to enable
 *  double-click repeat feature
 */
#ifndef XmNmenuBarRepeatTimeout
#    define XmNmenuBarRepeatTimeout "menuBarRepeatTimeout"
#endif
#ifndef XmCMenuBarRepeatTimeout
#    define XmCMenuBarRepeatTimeout "MenuBarRepeatTimeout"
#endif

externalref WidgetClass xmCascadeButtonWidgetClass;

typedef struct _XmCascadeButtonRec      * XmCascadeButtonWidget;
typedef struct _XmCascadeButtonClassRec * XmCascadeButtonWidgetClass;

/* fast subclass define */
#ifndef XmIsCascadeButton 
#define XmIsCascadeButton(w) XtIsSubclass(w, xmCascadeButtonWidgetClass)
#endif /* XmIsCascadeButton */


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateCascadeButton() ;
extern void XmCascadeButtonHighlight() ;

#else

extern Widget XmCreateCascadeButton( 
                        Widget parent,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;
extern void XmCascadeButtonHighlight( 
                        Widget cb,
#if NeedWidePrototypes
                        int highlight) ;
#else
                        Boolean highlight) ;
#endif /* NeedWidePrototypes */

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmCascadeB_h */
/* DON'T ADD STUFF AFTER THIS #endif */
