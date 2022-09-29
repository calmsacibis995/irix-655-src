/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: Scale.h,v $ $Revision: 0.5 $ $Date: 1995/01/10 02:08:50 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmScale_h
#define _XmScale_h


#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __sgi

/*  Resource definitions for SGI "percent done" extension */
/*
 * The slidingMode, editable, and sliderVisual will be implemented
 * by OSF in Motif 2.0.  Slanted, however, we'll have to continue
 * to support.
 */

#ifndef XmNslidingMode
#define XmNslidingMode "slidingMode"
#endif
#ifndef XmNeditable 
#define XmNeditable "editable"
#endif
#ifndef XmNsliderVisual
#define XmNsliderVisual "sliderVisual"
#endif
#ifndef SgNslanted 
#define SgNslanted "slanted"
#endif

/* Valid values for slidingMode */
/*
 * These lines are duplicated in ScrollBar.h.  Allow the programmer
 * to include these files in either order
 */
#ifndef _XmScrollBar_h
typedef enum {
  XmSLIDER,
  XmTHERMOMETER
} XmSlidingMode;

#define Xmslider "slider"
#define Xmthermometer "thermometer"
#endif /* _XmScrollBar_h */


/* Valid values for sliderVisual */
/*
 * These lines are duplicated in ScrollBar.h.  Allow the programmer
 * to include these files in either order
 */
#ifndef _XmScrollBar_h
typedef enum {
  XmSHADOWED,
  XmFLAT_FOREGROUND,
  XmETCHED_LINE
} XmSliderVisual;

#define Xmshadowed "shadowed"
#define Xmflat_foreground "flat_foreground"
#define Xmetched_line "etched_line"
#endif /* _XmScrollBar_h */


#ifndef XmCSlidingMode
#define XmCSlidingMode "SlidingMode"
#endif
#ifndef XmCEditable 
#define XmCEditable "Editable"
#endif
#ifndef XmCSliderVisual
#define XmCSliderVisual "SliderVisual"
#endif
#ifndef SgCSlanted 
#define SgCSlanted "Slanted"
#endif

#ifndef XmRSlidingMode
#define XmRSlidingMode "SlidingMode"
#endif
#ifndef XmRSliderVisual
#define XmRSliderVisual "SliderVisual"
#endif

#endif /* __sgi */

/* Class record constants */

externalref WidgetClass xmScaleWidgetClass;

/* fast XtIsSubclass define */
#ifndef XmIsScale
#define XmIsScale(w) XtIsSubclass (w, xmScaleWidgetClass)
#endif

typedef struct _XmScaleClassRec * XmScaleWidgetClass;
typedef struct _XmScaleRec      * XmScaleWidget;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern void XmScaleSetValue() ;
extern void XmScaleGetValue() ;
extern Widget XmCreateScale() ;

#else

extern void XmScaleSetValue( 
                        Widget w,
                        int value) ;
extern void XmScaleGetValue( 
                        Widget w,
                        int *value) ;
extern Widget XmCreateScale( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmScale_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
