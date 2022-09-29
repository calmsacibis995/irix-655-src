////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////

#ifndef VKSIMPLEWINDOW_H
#define VKSIMPLEWINDOW_H

// VkSimpleWindow.h - declarations for a Motif toplevel window object

class VkApp;

#include <Vk/VkComponent.h>

#if _VK_MAJOR > 1
class VkDisplay;
class VkScreen;
#endif

////////////////////////////////////////////////////////////////////////////////
//
// NOTE: Quick Help is new in the IRIX 6.2 release.  There are two forms of
//       access:
//		* Resource-based access is documented in the man page and
//		  is fully supported.  Go ahead and set your resources.  For
//		  most programs, this is all the access they will ever need.
//
//		* The programmatic access to Quick Help is still experimental.
//		  in IRIX 6.2.
//
//			IT MAY CHANGE IN THE NEXT RELEASE.
//
//		  That is why it is not documented in the man pages.  For
//		  an example of current use, see the QuickHelp demo program.
//
////////////////////////////////////////////////////////////////////////////////

// Some constants to go along with the Quick Help calls.  These probably
// should have been defined inside the class, but it is too late now -- the
// class has already been released, so we cannot make the change without
// breaking compatitibility.
    enum VkDisplayWhere {VkNEAR_POINTER=-1, VkNEAR_WIDGET=-2};
    enum VkKeep         {VkKEEP, VkRESOURCE};
    enum VkWhichHelp    {VkMESSAGE_LINE_HELP, VkPOPUP_HELP, VkBOTH_HELP};
    typedef Boolean	delayTime;	// Delay, in 0.1 second units

//Quick Help callData structures
    typedef struct {
	Boolean libDisplay;	// If TRUE, library also posts help
	Widget widget;		// Widget that got the enter
	XmString msg;		// Help string the library has
	int x, y;		// Position the library has for popup
    } quickHelpEnterCallbackStruct;

    typedef struct {
	Boolean libDisplay;	// If TRUE, library also clears help
	Widget widget;		// Widget that got the leave
    } quickHelpLeaveCallbackStruct;


class VkSimpleWindow : public VkComponent {

    friend class VkApp;

#if _VK_MAJOR > 1
	friend class VkDisplay;
	friend class VkScreen;
#endif /* _VK_MAJOR > 1 */

    friend class VkQuickHelp;

  public:
#if _VK_MAJOR > 1
    VkSimpleWindow(const char *name, VkScreen *screen, ArgList argList = NULL, Cardinal argCount);
#else
    VkSimpleWindow(const char *name, ArgList argList = NULL, Cardinal argCount = NULL);
#endif	

    virtual ~VkSimpleWindow();

    void addView(Widget);
    void addView(VkComponent *);

    void removeView();

    virtual void open();
    virtual void raise();
    virtual void lower();
    virtual void iconize();  // obsolete
    virtual void iconify();
    virtual void show();
    virtual void hide();

    virtual void build();   // obsolete, don't use directly

    const char *getTitle();
    void setTitle(const char *);
    void setIconName(const char *);
    void setClassHint(const char *className);

    virtual const char* className();
    Boolean iconic() const { return (_iconState == CLOSED); }
    Boolean visible() const { return (_visibleState == VISIBLE); }
    int getVisualState();	// Generally use this, not iconic() or visible()
    virtual Widget     mainWindowWidget() const;
    virtual Widget     viewWidget() const;
    virtual operator Widget () const;

    void setWindowSizes(int minWidth, int minHeight,
			int maxWidth, int maxHeight);
    
    static VkSimpleWindow *getWindow(VkComponent *);
#if _VK_MAJOR > 1
	virtual VkScreen *getScreen() { return _vkScreen; }
#endif

    // NOTE: as stated at the top of this file, programmatic access to
    //       Quick Help is experimental in IRIX 6.2, and may change in a
    //	     later release.  Use at  your own risk.
    // 
    // Quick Help entry points.

	// Immediate display of quick help.
	// This allows for a geat deal of control -- the application can
	// fall back on this if the library is not doing what it needs.
	//
	//	delayTime	meaning
	//	---------	----------------------------------
	//	    0		do not delay
	//	    1		standard delay (== Boolean True)
	//	   >1		delay time, in 100'ths of a second
	//	   <1		undefined
	//
	   void displayHelpString (char *msgLine=NULL, char *popup=NULL,
				  int x=(int)VkNEAR_POINTER, int y=0,
				  delayTime delay=0);
	   void displayHelpResource (Widget w,
				  char *msgLine=NULL, char *popup=NULL,
				  VkWhichHelp which=VkBOTH_HELP,
				  int x=(int)VkNEAR_WIDGET, int y=0,
				  delayTime delay=0);
	   void unDisplayHelp (Widget w=NULL);

	// Update part of VkSimpleWindow's widget tree.  NULL => entire tree.
	   void quickHelpUpdate(Widget w=NULL);

	// Set the times for balloon help.  Any parameter that is -1 is
	// ignored.  This allows you te change just some of the times.
	//
	// NOTE: we do not want to include VkQuickHelp.h, because it is not
	//	 public.  These default times need to be kept in sync with
	//	 those in VkQuickHelp.c++ by hand.
	   void setPopupTime (int initialDelay=800, int upTime=3000,
			     int browseWaitTime=100, int browseVelocity=10,
			     int browseCancelTime=500);

	// Get the current popup times
	   void getPopupTime (int *initial, int *stayUp,
			      int *bInit, int *bVelocity, int *bCancel);

	// Override the default (resource) help strings
	   void setQuickHelp (Widget w, char *msgLine=NULL,
			      char *popup=NULL, VkKeep which=VkRESOURCE);
	// Quick Help callback names
	   static const char *const hideMsgLineCallback;
	   static const char *const showMsgLineCallback;
	   static const char *const hidePopupCallback;
	   static const char *const showPopupCallback;
    
  protected:

    enum IconState     { OPEN, CLOSED, ICON_UNKNOWN        };
    enum VisibleState  { HIDDEN, VISIBLE, VISIBLE_UNKNOWN  };
    enum StackingState { RAISED, LOWERED, STACKING_UNKNOWN };

    IconState      _iconState;
    VisibleState   _visibleState;
    StackingState  _stackingState;
    Widget         _mainWindowWidget;

    Boolean        _usingSetUpInterface; 
    virtual Widget setUpInterface(Widget );

    virtual void setUpWindowProperties();
    virtual void stateChanged(IconState);
    virtual void handleWmDeleteMessage();
    virtual void handleWmQuitMessage();    
    virtual void handleRawEvent(XEvent *event);
    virtual void afterRealizeHook();

    virtual void busy();
    virtual void veryBusy();
    virtual void notBusy();

#if _VK_MAJOR > 1
    // NOTE:  these are the Vk 1.5.2 extension records...
    //
    class VkQuickHelp   *_help;
#endif

  private:

	    Boolean handleRawHelpEvent(XEvent *event, Widget);
    static  void stateChangedEventHandler(Widget, XtPointer, XEvent *, Boolean *);
    static  void deleteWindowCallback(Widget, XtPointer, XtPointer);
    static  void quitAppCallback(Widget, XtPointer, XtPointer);

    static Boolean setupHelpCallback(XtPointer);

    Widget     _viewWidget;
    Window     _busyWindow;
    Boolean    _busy;
	
#if _VK_MAJOR > 1
    VkScreen   *_vkScreen;

    void VkSimpleWindowInitialize( const char  *name,
				   VkScreen    *screen,
				   ArgList      argList,
				   Cardinal     argCount);
#endif	

  private:

    // This class is not intended to be copied.  Strangers will get a
    // compile-time error.  Because there is no implementing code, friends
    // and inadvertant local use will get an ld-time error.  If a copy
    // constructor or operator= is ever needed, simply write them and
    // make these entries public.
    VkSimpleWindow(const VkSimpleWindow&);
    VkSimpleWindow &operator =(const VkSimpleWindow&);
};

extern void VkConfigureWM(VkSimpleWindow *w,
			  Boolean quit    = TRUE,
			  Boolean close   = TRUE,
			  Boolean border  = TRUE,
			  Boolean title   = TRUE,
			  Boolean resize  = TRUE,
			  Boolean iconify = TRUE,
			  Boolean menu    = TRUE);

#endif


// X11 window visual states.  These are consistent with those given in
// Xutil.h.  Note that, unlike the comments in Xutil.h suggest, these
// values pertain throughout the life of the window.  See the ICCCM for
// further details.

#ifndef WithdrawnState
#  define WithdrawnState 0	// Normal window that is not mapped
#endif

// Xutil.h uses WithdrawnState whether or not iconic.  ViewKit differentiates.
// The difference lies in what you would get if you simply mapped the window.
#define WithdrawnNormalState WithdrawnState
#define WithdrawnIconicState 2

#ifndef NormalState
#  define NormalState 1		/* Normal window that is mapped */
#endif

#ifndef IconicState
#  define IconicState 3		/* Iconic window that is mapped */
#endif
