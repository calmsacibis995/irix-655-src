/* Definitions for SGI widget extensions */

#ifndef _SgiP_h
#define _SgiP_h

#ifdef __cplusplus
extern "C" {
#endif

/* Global Variables declared */
extern int sgIsWindowManager;		/* Only set by mwm */
extern int SGImode;	/* So it is resolved for apps linked with libChow */
/* unique contexts used for reference counting in _SG_XtSetValues(): */
extern XContext SGIrefCntContext;	/* context for the reference count */
extern XContext SGIsetValuesFcnContext;	/* context for the saved fcn */

/*
 * SGI has added a vendor extension pointer to all widget class and instance
 * records.  That helps to preserve compatible shared libraries and widget
 * subclasses.
 *
 * The following is an SGI class extension record.  Any class that needs
 * one will have one.  The record supports automatic intialization of SGI
 * sub-resources.  Therefore, it is the same for all widgets.
 *
 * The instance extension record is specific to each widget that needs one,
 * so it is declared in corresponding the <Widget>P.h file.
 */

#define _SgExtensionRecVersion	1L
typedef struct _sgClassExtensionRec {
	long        version;	/* Version, for future compatibility control */
	Cardinal    isize;	/* Size of the instance extension record */
	XtResource *rsrc_ptr;	/* Ptr to the resources data definition */
	int	    rsrc_num;	/* Number of resources */
} _SgClassExtensionRec, *_SgClassExtension;

#define _SgInstanceExtensionRecVersion	1L
typedef struct _sgInstanceExtensionRec {
	long	    version;	/* Version, for future compatibility control */
	/* OK to add extra fields as they become necessary. */
	/* Instance resource data starts here, after the common fields. */
} _SgInstanceExtensionRec, *_SgInstanceExtension;

#if 0
/*
 * Typedef for the resource buffer for each Xt Object.
 *
 * For each type of object for which we have extended resources, it is
 * necessary to:
 *
 *	* Replace the MakeCompilerHappy member with a genuine member
 *	* Add any other new resources (if there is more than one resource)
 *
 * Each valid typedef -- i.e. describing a real resource -- needs to
 * have a reference to it in the sizes table in SgiI.h.  Ones with just
 * the dummy entry should not.
 */
    typedef struct _sgApplicationShellRsrc {
	int	MakeCompilerHappy;	/* Replace w/1st real rsrc */
    } _SgApplicationShellRsrcRec, *_SgApplicationShellRsrc;
#define _SG_GetApplicationShellRsrcPtr(w) ((_SgApplicationShellRsrc)_SG_GetResourcePtr((Widget)(w),applicationShellWidgetClass))

    typedef struct _sgCompositeRsrc {
	int	MakeCompilerHappy;	/* Replace w/1st real rsrc */
    } _SgCompositeRsrcRec, *_SgCompositeRsrc;
#define _SG_GetCompositeRsrcPtr(w) ((_SgCompositeRsrc)_SG_GetResourcePtr((Widget)(w),compositeWidgetClass))

    typedef struct _sgConstraintRsrc {
	int	MakeCompilerHappy;	/* Replace w/1st real rsrc */
    } _SgConstraintRsrcRec, *_SgConstraintRsrc;
#define _SG_GetConstraintRsrcPtr(w) ((_SgConstraintRsrc)_SG_GetResourcePtr((Widget)(w),constraintWidgetClass))

    typedef struct _sgCoreRsrc {
	int	MakeCompilerHappy;	/* Replace w/1st real rsrc */
    } _SgCoreRsrcRec, *_SgCoreRsrc;
#define _SG_GetCoreRsrcPtr(w) ((_SgCoreRsrc)_SG_GetResourcePtr((Widget)(w),coreWidgetClass))

/* Extended Rsrc for Object */
    typedef struct _sgObjectRsrc {
	Boolean     sg_feature;     /* SGI extended features master switch */
	Boolean     sg_look;        /* "SGI look" master switch */
    } _SgObjectRsrcRec, *_SgObjectRsrc;
#define _SG_GetObjectRsrcPtr(w) ((_SgObjectRsrc)_SG_GetResourcePtr((Widget)(w), objectClass))
#define _SG_sgFeature(w) ((_SG_GetObjectRsrcPtr(w))->sg_feature)
#define _SG_sgLook(w) (((_SG_GetObjectRsrcPtr(w))->sg_look)&&((_SG_GetObjectRsrcPtr(w))->sg_feature))

    typedef struct _sgOverrideShellRsrc {
	int	MakeCompilerHappy;	/* Replace w/1st real rsrc */
    } _SgOverrideShellRsrcRec, *_SgOverrideShellRsrc;
#define _SG_GetOverrideShellRsrcPtr(w) ((_SgOverrideShellRsrc)_SG_GetResourcePtr((Widget)(w),overrideShellWidgetClass))

    typedef struct _sgRectObjRsrc {
	int	MakeCompilerHappy;	/* Replace w/1st real rsrc */
    } _SgRectObjRsrcRec, *_SgRectObjRsrc;
#define _SG_GetRectObjRsrcPtr(w) ((_SgRectObjRsrc)_SG_GetResourcePtr((Widget)(w),rectObjClass))

    typedef struct _sgShellRsrc {
	int	MakeCompilerHappy;	/* Replace w/1st real rsrc */
    } _SgShellRsrcRec, *_SgShellRsrc;
#define _SG_GetShellRsrcPtr(w) ((_SgShellRsrc)_SG_GetResourcePtr((Widget)(w),shellWidgetClass))

    typedef struct _sgTopLevelShellRsrc {
	int	MakeCompilerHappy;	/* Replace w/1st real rsrc */
    } _SgTopLevelShellRsrcRec, *_SgTopLevelShellRsrc;
#define _SG_GetTopLevelShellRsrcPtr(w) ((_SgTopLevelShellRsrc)_SG_GetResourcePtr((Widget)(w),topLevelShellWidgetClass))

    typedef struct _sgTransientShellRsrc {
	int	MakeCompilerHappy;	/* Replace w/1st real rsrc */
    } _SgTransientShellRsrcRec, *_SgTransientShellRsrc;
#define _SG_GetTransientShellRsrcPtr(w) ((_SgTransientShellRsrc)_SG_GetResourcePtr((Widget)(w),transientShellWidgetClass))

    typedef struct _sgVendorShellRsrc {
	int	MakeCompilerHappy;	/* Replace w/1st real rsrc */
    } _SgVendorShellRsrcRec, *_SgVendorShellRsrc;
#define _SG_GetVendorShellRsrcPtr(w) ((_SgVendorShellRsrc)_SG_GetResourcePtr((Widget)(w),vendorShellWidgetClass))

    typedef struct _sgWMShellRsrc {
	int	MakeCompilerHappy;	/* Replace w/1st real rsrc */
    } _SgWMShellRsrcRec, *_SgWMShellRsrc;
#define _SG_GetWMShellRsrcPtr(w) ((_SgWMShellRsrc)_SG_GetResourcePtr((Widget)(w),wmShellWidgetClass))

#endif

/*******************************************************************************
 *
 * Definitions for the SGI "shader" -- i.e. picking different shades of the
 * same color to get SGI-standard visual effects.
 */

typedef struct _shader {
    Display *dpy;
    Colormap cmap;
    Cardinal depth;
    Pixel    pixel;
    short    r,g,b;
    int      faceShade; /* point along the shade line where bkg color falls */
    GC       GCarray[9]; /* GCs for different shades of the base color*/
    struct _shader* next;
}*shaderptr;

/* indices to access GCs from a shader */
#  define tuShader_darkest   0
#  define tuShader_veryDark  1
#  define tuShader_dark      2
#  define tuShader_medium    3
#  define tuShader_light     4
#  define tuShader_veryLight 5
#  define tuShader_lightest  6
#  define tuShader_white     7

/* definitions for brighter and darker to be used for _sgAllocClosestColor */
#  define _SG_ANY_BRIGHTNESS	 0
#  define _SG_BRIGHTER		 1
#  define _SG_DARKER		-1


/********    Function Declarations Used From Widgets    ********/
#ifdef _NO_PROTO

    extern XtPointer _SG_GetResourcePtr()	;
    extern GC _sgFindGC()			;
    extern void _sgFindShader()			;
    Status _sgAllocClosestColor()	;

#else

    extern XtPointer _SG_GetResourcePtr(
			Widget       w		,
			WidgetClass  wc)	;
    extern GC _sgFindGC(Widget w, shaderptr shader1,int index);
    extern void _sgFindShader(Widget w, shaderptr* return_adr,Pixel p);
    Status _sgAllocClosestColor(Display *display, Screen *screen, Colormap cmap, XColor *cdef, int direction, XColor *cantbe);

#endif /* _NO_PROTO */
/********    End Function Declarations Used From Widgets    ********/



/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

    extern void    _SG_ClassPartInitialize()	;
    extern void    _SG_XtInitialize()           ;
    extern void    _SG_XtDestroyPrehook()	;
    extern void    _SG_XtDestroyPosthook()	;
    extern void    _SG_XtGetValuesHook()	;
    extern Boolean _SG_XtSetValues()		;
    extern int     _XmBrightness() 		; /* Sgi made this visible */

#else

    extern void _SG_ClassPartInitialize(void)	;

    extern void _SG_XtInitialize(
        		Widget       req	,
        		Widget       new_w	,
        		ArgList      args	,
        		Cardinal    *num_args)	;

    extern void _SG_XtDestroyPrehook(
			Widget       w)		;

    extern void _SG_XtDestroyPosthook(
			Widget       w)		;

    extern void _SG_XtGetValuesHook(
			Widget       new_w	,
			ArgList      args	,
			Cardinal    *num_args)	;

    extern Boolean _SG_XtSetValues(
			Widget       current	,
			Widget       req	,
			Widget       new_w	,
			ArgList      args	,
			Cardinal    *num_args)	;
    extern int _XmBrightness(			/* Sgi made this visible */
			XColor	    *color)	;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


/*
 * The following declare NOP routines are for libXm, so that Chow-ized
 * code can be linked with libXm.  The expectation is that the code will
 * never call these routines at run time.  The actual functions are
 * declared in Sgi.c.
 */
#if !(defined sgiLibChow && sgiLibChow)
#ifdef _NO_PROTO
extern void __SGI_RenderShadows();
#else

extern void _SGI_RenderShadows(Widget w ,Display* dpy,Drawable d,
                               shaderptr shader,Position x, Position y,
                               Dimension width,Dimension height,
                               int* state_array,
                               int shadow_thickness,
                               unsigned int shadow_style);

#endif  /* _NO_PROTO */
#endif  /* !(defined sgiLibChow && sgiLibChow) */


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _SgiP_h */
/* DON'T ADD STUFF AFTER THIS #endif */

