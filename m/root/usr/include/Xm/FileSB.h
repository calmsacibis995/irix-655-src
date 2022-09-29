/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: FileSB.h,v $ $Revision: 0.10 $ $Date: 1995/12/02 01:37:25 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmFSelect_h
#define _XmFSelect_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif


#if defined(__sgi)
#ifndef SgDIALOG_FINDER
#define SgDIALOG_FINDER 7931
#endif
  
#define SgDIALOG_VIEWER 100

typedef enum _SgViewerMode
{  
  SgVIEWER_NEVER,
  SgVIEWER_NONE,
  SgVIEWER_AUTOMATIC,
  SgVIEWER_EXPLICIT
} SgViewerMode;
  
#define SgNviewerMode "viewerMode"  
#define SgCViewerMode "ViewerMode"
#define SgRFSBViewerMode "FSBViewerMode"

#define SgNviewerFileName "viewerFileName" 
#define SgCViewerFileName "ViewerFileName" 

#define SgNviewerFilter "viewerFilter" 
#define SgCViewerFilter "ViewerFilter"

#define SgNbrowserFileMask "browserFileMask" 
#define SgCBrowserFileMask "BrowserFileMask"

#define SgNuseEnhancedFilterDialog "useEnhancedFilterDialog" 
#define SgCUseEnhancedFilterDialog "UseEnhancedFilterDialog"

#define SgNviewerUpdateDelay "viewerUpdateDelay"
#define SgCViewerUpdateDelay "ViewerUpdateDelay"

#define SgNcompletionDelay "completionDelay"
#define SgCCompletionDelay "CompletionDelay"

void SgFileSelectionBoxReplaceSuffix(Widget fsb, const char *file_type,
                                     const char *default_suffix);

String _SgFileSelectionBoxGetFTRLegend(Widget fsb, const char *file_type);

#endif /* __sgi */

/* Type definitions for FileSB resources: */

#ifdef _NO_PROTO

typedef void (*XmQualifyProc)() ;
typedef void (*XmSearchProc)() ;

#else

typedef void (*XmQualifyProc)( Widget, XtPointer, XtPointer) ;
typedef void (*XmSearchProc)( Widget, XtPointer) ;

#endif


/* Class record constants */

externalref WidgetClass xmFileSelectionBoxWidgetClass;

typedef struct _XmFileSelectionBoxClassRec * XmFileSelectionBoxWidgetClass;
typedef struct _XmFileSelectionBoxRec      * XmFileSelectionBoxWidget;


#ifndef XmIsFileSelectionBox
#define XmIsFileSelectionBox(w) (XtIsSubclass((w),xmFileSelectionBoxWidgetClass))
#endif


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmFileSelectionBoxGetChild() ;
extern void XmFileSelectionDoSearch() ;
extern Widget XmCreateFileSelectionBox() ;
extern Widget XmCreateFileSelectionDialog() ;

#else

extern Widget XmFileSelectionBoxGetChild( 
                        Widget fs,
#if NeedWidePrototypes
                        unsigned int which) ;
#else
                        unsigned char which) ;
#endif /* NeedWidePrototypes */
extern void XmFileSelectionDoSearch( 
                        Widget fs,
                        XmString dirmask) ;
extern Widget XmCreateFileSelectionBox( 
                        Widget p,
                        String name,
                        ArgList args,
                        Cardinal n) ;
extern Widget XmCreateFileSelectionDialog( 
                        Widget ds_p,
                        String name,
                        ArgList fsb_args,
                        Cardinal fsb_n) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmFSelect_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
