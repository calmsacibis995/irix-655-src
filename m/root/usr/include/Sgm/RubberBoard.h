
/*******************************************************************************
///////   Copyright 1992, Silicon Graphics, Inc.  All Rights Reserved.   ///////
//                                                                            //
// This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;     //
// the contents of this file may not be disclosed to third parties, copied    //
// or duplicated in any form, in whole or in part, without the prior written  //
// permission of Silicon Graphics, Inc.                                       //
//                                                                            //
// RESTRICTED RIGHTS LEGEND:                                                  //
// Use,duplication or disclosure by the Government is subject to restrictions //
// as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data     //
// and Computer Software clause at DFARS 252.227-7013, and/or in similar or   //
// successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -    //
// rights reserved under the Copyright Laws of the United States.             //
//                                                                            //
*******************************************************************************/

#ifndef _SgRubberBoard_h
#define _SgRubberBoard_h


#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  RubberBoard Widget  */

externalref WidgetClass sgRubberBoardWidgetClass;

typedef struct _SgRubberBoardClassRec * SgRubberBoardWidgetClass;
typedef struct _SgRubberBoardRec      * SgRubberBoardWidget;

/* ifndef for Fast Subclassing  */

#ifndef SgIsRubberBoard
#define SgIsRubberBoard(w)	XtIsSubclass(w, sgRubberBoardWidgetClass)
#endif  /* SgIsRubberBoard */

/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget SgCreateRubberBoard() ;
extern Widget SgCreateRubberBoardDialog() ;

#else

extern Widget SgCreateRubberBoard( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;
extern Widget SgCreateRubberBoardDialog( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#define XmNinitialX      "initialX"
#define XmNinitialY      "initialY"
#define XmNinitialWidth  "initialWidth"
#define XmNinitialHeight "initialHeight"
#define XmNinitialParentWidth  "initialParentWidth"
#define XmNinitialParentHeight "initialParentHeight"

#define XmNfinalX      "finalX"
#define XmNfinalY      "finalY"
#define XmNfinalWidth  "finalWidth"
#define XmNfinalHeight "finalHeight"
#define XmNfinalParentWidth  "finalParentWidth"
#define XmNfinalParentHeight "finalParentHeight"

#define XmCFinalWidth  "FinalWidth"
#define XmCFinalHeight  "FinalHeight"
#define XmCInitialWidth  "InitialWidth"
#define XmCInitialHeight "InitialHeight"

#define XmNsetFinal    "setFinal"
#define XmNsetInitial  "setInitial"
#define XmCSetFinal    "SetFinal"
#define XmCSetInitial  "SetInitial"



#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif


#endif /* _SgRubberBoard_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
