/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: ToggleB.h,v $ $Revision: 0.8 $ $Date: 1995/01/10 02:12:31 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/***********************************************************************
 *
 * Toggle Widget
 *
 ***********************************************************************/
#ifndef _XmToggle_h
#define _XmToggle_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

externalref WidgetClass xmToggleButtonWidgetClass;

typedef struct _XmToggleButtonClassRec *XmToggleButtonWidgetClass;
typedef struct _XmToggleButtonRec      *XmToggleButtonWidget;

/*fast subclass define */
#ifndef XmIsToggleButton
#define XmIsToggleButton(w)     XtIsSubclass(w, xmToggleButtonWidgetClass)
#endif /* XmIsToggleButton */


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Boolean XmToggleButtonGetState() ;
extern void XmToggleButtonSetState() ;
extern Widget XmCreateToggleButton() ;
#ifdef __sgi
extern Pixel SgGetLocatePixel();
#endif /* __sgi */

#else

extern Boolean XmToggleButtonGetState( 
                        Widget w) ;
extern void XmToggleButtonSetState( 
                        Widget w,
#if NeedWidePrototypes
                        int newstate,
                        int notify) ;
#else
                        Boolean newstate,
                        Boolean notify) ;
#endif /* NeedWidePrototypes */
extern Widget XmCreateToggleButton( 
                        Widget parent,
                        char *name,
                        Arg *arglist,
                        Cardinal argCount) ;
#ifdef __sgi
extern Pixel SgGetLocatePixel(
			      Widget w,
			      Pixel  bkg_pixel );
#endif /* __sgi */

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmToggle_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
